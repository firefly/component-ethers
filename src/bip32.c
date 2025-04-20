#include <string.h>

#include "firefly-bip32.h"
#include "firefly-crypto.h"
#include "firefly-hash.h"

#include "bip39-en.h"

// DEBUG
#include <stdio.h>
void dumpBuffer(const char*, uint8_t*, size_t);

#define WORDLIST            wordlist_en

// "Bitcoin seed"
const uint8_t MasterSecret[] = { 66, 105, 116, 99, 111, 105, 110, 32, 115, 101, 101, 100 };


static bool nextWord(FfxWordlistCursor *cursor) {
    if (cursor->index >= 2047) { return false; }

    if (cursor->length != 0) {
        cursor->index++;
        cursor->offset += cursor->length + 1;
    }

    cursor->length = strlen(&WORDLIST[cursor->offset]);

    return true;
}

static int wordToIndex(const char* const word, size_t length) {
    FfxWordlistCursor cursor = { 0 };

    while (nextWord(&cursor)) {
        if (length != cursor.length) { continue; }
        if (!strncmp(word, &WORDLIST[cursor.offset], length)) {
            return cursor.index;
        }
    }

    return -1;
}

int ffx_bip39_index(const char* const word) {
    return wordToIndex(word, strlen(word));
}

const char* ffx_bip39_word(int index) {
    FfxWordlistCursor cursor = { 0 };

    while (nextWord(&cursor)) {
        if (cursor.index == index) { return &WORDLIST[cursor.offset]; }
    }

    return NULL;
}

const char* ffx_bip39_nextWord(FfxWordlistCursor *cursor, int *index) {
    if (!nextWord(cursor)) {
        *index = -1;
        return NULL;
    }

    if (index) { *index = cursor->index; }
    return &WORDLIST[cursor->offset];
}

bool ffx_mnemonic_initPhrase(FfxMnemonic *mnemonic, const char* phrase) {
    memset(mnemonic, 0, sizeof(FfxMnemonic));

    size_t length = strlen(phrase);

    size_t wordCount = 0;

    bool lastws = true;
    size_t wordStart = 0;
    for (int i = 0; i < 1 + length; i++) {
        char c = phrase[i];
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\0') {
            if (lastws) { continue; }
            lastws = true;

            int index = wordToIndex(&phrase[wordStart], i - wordStart);
            if (index == -1) { return false; }

            int bit = wordCount * 11;

            for (int b = 0; b < 11; b++) {
                if (index & (0b10000000000 >> b)) {
                    mnemonic->entropy[bit / 8] |= (0x80 >> (bit % 8));
                }
                bit++;
            }

            wordCount++;
        } else {
            if (wordCount >= 24) { return false; }
            if (lastws) { wordStart = i; }
            lastws = false;
        }
    }

    if (wordCount < 12 || wordCount % 3) { return false; }
    size_t entropyLength = 16 + 4 * ((wordCount - 12) / 3);

    uint8_t digest[FFX_SHA256_DIGEST_LENGTH];
    ffx_hash_sha256(digest, mnemonic->entropy, entropyLength);

    int bits = (8 * entropyLength / 32);
    int mask = ((1 << bits) - 1) << (8 - bits);
    if ((mnemonic->entropy[entropyLength] & mask) != (digest[0] & mask)) {
        return false;
    }

    mnemonic->wordCount = wordCount;
    mnemonic->entropyLength = entropyLength;

    return true;
}

bool ffx_mnemonic_initEntropy(FfxMnemonic *mnemonic, const uint8_t* entropy,
  size_t length) {
    memset(mnemonic, 0, sizeof(FfxMnemonic));

    if (length < 16 || length > 32 || length % 4) { return false; }

    mnemonic->wordCount = 12 + 3 * ((length - 16) / 4);
    mnemonic->entropyLength = length;

    memmove(mnemonic->entropy, entropy, length);

    // Add the checksum
    uint8_t digest[FFX_SHA256_DIGEST_LENGTH];
    ffx_hash_sha256(digest, mnemonic->entropy, length);

    int bits = (8 * length / 32);
    int mask = ((1 << bits) - 1) << (8 - bits);
    mnemonic->entropy[length] = digest[0] & mask;

    return true;
}

const char* ffx_mnemonic_getWord(FfxMnemonic *mnemonic, int index) {
    if (index < 0 || index >= mnemonic->wordCount) { return NULL; }

    int wordIndex = 0;
    for (int i = 0; i < 11; i++) {
        size_t bit = (index * 11) + i;
        wordIndex <<= 1;
        if (mnemonic->entropy[bit / 8] & (0x80 >> (bit % 8))) {
            wordIndex |= 1;
        }
    }

    return ffx_bip39_word(wordIndex);
}


static const char saltPrefix[] = "mnemonic";

bool ffx_mnemonic_getSeed(FfxMnemonic *mnemonic, const char* password,
  uint8_t *seed) {
    for (int i = strlen(password) - 1; i >= 0; i--) {
        if (password[i] < 32 || password[i] > 126) {
            printf("Unsupported password: '%s' (%lu bytes)\n", password,
              strlen(password));
            return false;
        }
    }

    char phrase[9 * mnemonic->wordCount];
    memset(phrase, 0, sizeof(phrase));

    size_t offset = 0;
    for (int i = 0; i < mnemonic->wordCount; i++) {
        if (i > 0) { phrase[offset++] = ' '; }
        const char *word = ffx_mnemonic_getWord(mnemonic, i);
        size_t wordLength = strlen(word);
        strcpy(&phrase[offset], word);
        offset += wordLength;
    }

    char salt[strlen(saltPrefix) + strlen(password) + 1];
    memset(salt, 0, sizeof(salt));
    strcpy(salt, saltPrefix);
    strcpy(&salt[strlen(saltPrefix)], password);

    ffx_pbkdf2_sha512(seed, FFX_BIP39_SEED_LENGTH, 2048,
      (const uint8_t*)phrase, offset, (const uint8_t*)salt, strlen(salt));

    return 1;
}

///////////////////////////////
//


bool ffx_hdnode_initSeed(FfxHDNode *node, const uint8_t *seed) {
    memset(node, 0, sizeof(FfxHDNode));

    uint8_t I[64] = { 0 };
    ffx_hmac_sha512(I, MasterSecret, sizeof(MasterSecret), seed,
      FFX_BIP39_SEED_LENGTH);

    // Check the private key is good (use the node->key temporarily)
    uint8_t check[65];
    bool status = ffx_pk_computePubkeySecp256k1(check, I);
    if (!status) { return false; }

    memset(node->key, 0, 65);
    memcpy(&node->key[1], I, 32);
    memcpy(node->chaincode, &I[32], 32);

    return true;
}

void ffx_hdnode_clone(FfxHDNode *dst, FfxHDNode *src) {
    memmove(dst, src, sizeof(FfxHDNode));
}

// See: ecc.c
void _ffx_pk_modAddSecp256k1(uint8_t *_result, uint8_t *_a, uint8_t *_b);
void _ffx_pk_addPointSecp256k1(uint8_t *_result, uint8_t *_a, uint8_t *_b);

bool ffx_hdnode_deriveChild(FfxHDNode *node, uint32_t index) {
    if (node->depth == 0xffffffff) { return false; }

    // Used to:
    //  - temporality build up the serI data (37 bytes)
    //  - then hold the hashed value (64 bytes)
    uint8_t I[64] = { 0 };

    if (node->key[0] == 0) {
        // Private key derivation

        if (index & FfxHDNodeHardened) {
            // Data = 0x00 || ser_256(k_par)
            memcpy(I, node->key, 33);

        } else {
            // Data = ser_p(point(k_par))
            uint8_t pubkey[65];
            if(!ffx_pk_computePubkeySecp256k1(pubkey, &node->key[1])) {
                return false;
            }
            ffx_pk_compressPubkeySecp256k1(I, pubkey);
        }

    } else {
        // Neutered key derivation
        if (index & FfxHDNodeHardened) { return false; }
    }

    I[33] = (index >> 24) & 0xff;
    I[34] = (index >> 16) & 0xff;
    I[35] = (index >>  8) & 0xff;
    I[36] = (index >>  0) & 0xff;

    ffx_hmac_sha512(I, node->chaincode, 32, I, 37);

    uint8_t pubkey[65] = { 0 };
    if (node->key[0] == 0) {
        // (IL + k_par) % n
        uint8_t key[32] = { 0 };
        _ffx_pk_modAddSecp256k1(key, I, &node->key[1]);
        if(!ffx_pk_computePubkeySecp256k1(pubkey, key)) { return false; }
        memcpy(&node->key[1], key, sizeof(key));
    } else {
        // Point(IL) + K_par
        if(!ffx_pk_computePubkeySecp256k1(pubkey, I)) { return false; }
        _ffx_pk_addPointSecp256k1(node->key, pubkey, node->key);
    }

    memcpy(node->chaincode, &I[32], 32);
    node->depth++;
    node->index = index;

    return true;
}

bool ffx_hdnode_derivePath(FfxHDNode *_node, const char* path) {
    FfxHDNode node;
    ffx_hdnode_clone(&node, _node);

    size_t length = strlen(path) + 1;
    uint32_t index = 0, count = 0;
    for (int i = 0; i < length; i++) {
        char c = path[i];

        if (c == '/' || c == '\0') {
            // Did not contain any actual numbers in the component
            if (count == 0) { return false; }

            ffx_hdnode_deriveChild(&node, index);

            // Reset
            count = 0;
            index = 0;

        } else if (c >= '0' && c <= '9') {
            // Would go out of range
            if (index > 214748364) { return false; }

            // Cannot include numbers after a tick
            if (index & 0x80000000) { return false; }

            count++;
            index *= 10;
            index += (c - '0');

        } else if (c == '\'') {
            // Cannot include a tick after a tick
            if (index & 0x80000000) { return false; }

            index |= 0x80000000;

        } else if (c == 'm') {
            // Must be the first component and on a root node
            if (i != 0 || node.depth != 0) { return false; }

            // If contains components, m must stand alone
            if (length > 2 && path[1] != '/') { return false; }

            // m/XXX; skip m/
            i++;

        } else {
            // Invalid character
            return false;
        }
    }

    ffx_hdnode_clone(_node, &node);
    return true;
}

bool ffx_hdnode_neuter(FfxHDNode *node) {
    if (node->key[0] != 0) { return true; }
    return ffx_pk_computePubkeySecp256k1(node->key, &node->key[1]);
}

bool ffx_hdnode_isNeuter(FfxHDNode *node) {
    return (node->key[0] != 0);
}

bool ffx_hdnode_getPrivkey(FfxHDNode *node, uint8_t *privkey) {
    if (node->key[0] != 0) { return false; }
    memcpy(privkey, &node->key[1], 32);
    return true;
}

bool ffx_hdnode_getPubkey(FfxHDNode *node, bool compressed, uint8_t *pubkey) {
    if (node->key[0] == 0) {
        if (!compressed) {
            return ffx_pk_computePubkeySecp256k1(pubkey, &node->key[1]);
        }

        uint8_t pub[65];
        bool status = ffx_pk_computePubkeySecp256k1(pub, &node->key[1]);
        if (!status) { return false; }

        ffx_pk_compressPubkeySecp256k1(pubkey, pub);
        return true;
    }

    if (node->key[0] != 4) { return false; }

    if (!compressed) {
        memcpy(node->key, pubkey, 65);
        return true;
    }

    ffx_pk_compressPubkeySecp256k1(pubkey, node->key);
    return true;
}

