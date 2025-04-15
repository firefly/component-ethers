
#include <stdio.h>
#include <string.h>

#include "firefly-address.h"
#include "firefly-bip32.h"
#include "firefly-cbor.h"
#include "firefly-crypto.h"
#include "firefly-hash.h"

#include "testcases-h/accounts.h"
#include "testcases-h/hashes.h"
#include "testcases-h/mnemonics.h"
#include "testcases-h/pbkdf.h"


///////////////////////////////
// Utilities

static void dumpBuffer(const char *header, const uint8_t *buffer, size_t length) {
    printf("%s 0x", header);
    for (int i = 0; i < length; i++) {
        printf("%02x", buffer[i]);
    }
    printf(" (length=%zu)\n", length);
}

static int onlyAscii(const char *text) {
    for (int i = strlen(text) - 1; i >= 0; i--) {
        if (text[i] < 32 || text[i] > 126) { return false; }
    }
    return true;
}

static int cmpbuf(const uint8_t *a, const uint8_t *b, size_t length) {
    for (int i = 0; i < length; i++) {
        int d = a[i] - b[i];
        if (d) { return d; }
    }

    return 0;
}


///////////////////////////////
// Testcase Check Functions

int runTestAccounts(const char* name, const uint8_t *privkey,
  const char* address) {

    uint8_t pubkey[65] = { 0 };
    if (!ffx_pk_computePubkeySecp256k1(pubkey, privkey)) {
        dumpBuffer("failed to compute public key:", privkey, 32);
        return 1;
    }

    char checksum[FFX_ADDRESS_STRING_LENGTH] = { 0 };
    {
        uint8_t address[FFX_ADDRESS_LENGTH] = { 0 };
        ffx_eth_computeAddress(address, pubkey);
        ffx_eth_checksumAddress(checksum, address);
    }

    if (strncmp(address, checksum, 42)) { return 1; }

    return 0;
}

int runTestHashes(const char* name, const uint8_t *data, size_t dataLength,
    const uint8_t *sha256, size_t sha256Length,
    const uint8_t *sha512, size_t sha512Length,
    const uint8_t *keccak256, size_t keccak256Length) {

    uint8_t digest[64];

    ffx_hash_sha256(digest, data, dataLength);
    if (cmpbuf(digest, sha256, 32)) { return 1; }

    ffx_hash_sha512(digest, data, dataLength);
    if (cmpbuf(digest, sha512, 64)) { return 1; }

    ffx_hash_keccak256(digest, data, dataLength);
    if (cmpbuf(digest, keccak256, 32)) { return 1; }

    return 0;
}

int runTestMnemonicsNode(FfxHDNode *node, const uint8_t *chaincode,
  const uint8_t *privkey, const uint8_t *pubkey, uint32_t depth,
  uint32_t index) {

    if (node->depth != depth) {
        printf("depth does not match actual=%d expected=%d\n", node->depth,
          depth);
        return 1;
    }

    if (node->index != index) {
        printf("index does not match actual=%d expected=%d\n", node->index,
          index);
        return 1;
    }

    if (cmpbuf(node->chaincode, chaincode, 32)) {
        printf("chaincode does not match\n");
        dumpBuffer("Actual:  ", node->chaincode, 32);
        dumpBuffer("Expected:", chaincode, 32);
        return 1;
    }

    uint8_t key[65];

    ffx_hdnode_getPrivkey(node, key);
    if (cmpbuf(key, privkey, 32)) {
        printf("privkey does not match\n");
        dumpBuffer("Actual:  ", key, 32);
        dumpBuffer("Expected:", privkey, 32);
        return 1;
    }

    ffx_hdnode_getPubkey(node, true, key);
    if (cmpbuf(key, pubkey, 33)) {
        printf("pubkey does not match\n");
        dumpBuffer("Actual:  ", key, 33);
        dumpBuffer("Expected:", pubkey, 33);
        return 1;
    }

    return 0;
}

int runTestMnemonics(const char* name, const char* phrase,
  const char *password, size_t passwordLength,
  const uint8_t *expEntropy, size_t expEntropyLength,
  const uint8_t *expSeed, size_t expSeedLength) {

    // Test Mnemonic from Phrase

    FfxMnemonic mnemonic = { 0 };
    if (!ffx_mnemonic_initPhrase(&mnemonic, phrase)) {
        printf("initPhrase failed\n");
        return 1;
    }

    if (mnemonic.entropyLength != expEntropyLength||
      cmpbuf(mnemonic.entropy, expEntropy, expEntropyLength)) {
        printf("initPhrase did not match entropy\n");
        return 1;
    }

    // Test Mnemonic from Entropy

    if (!ffx_mnemonic_initEntropy(&mnemonic, expEntropy, expEntropyLength)) {
        printf("initEntrop failed\n");
        return 1;
    }

    if (mnemonic.entropyLength != expEntropyLength||
      cmpbuf(mnemonic.entropy, expEntropy, expEntropyLength)) {
        printf("initEntrop did not match entropy\n");
        return 1;
    }

    // Test Seed

    uint8_t actSeed[FFX_BIP39_SEED_LENGTH] = { 0 };

    if (!ffx_mnemonic_getSeed(&mnemonic, password, actSeed)) {
        printf("getSeed failed\n");
        return 1;
    }

    if (cmpbuf(actSeed, expSeed, FFX_BIP39_SEED_LENGTH)) {
        printf("getSeed did not match seed\n");
        dumpBuffer("Actual:  ", actSeed, FFX_BIP39_SEED_LENGTH);
        dumpBuffer("Expected:", expSeed, FFX_BIP39_SEED_LENGTH);
        return 1;
    }

    return 0;
}


///////////////////////////////
// Test Data Macros

#define READ_STRING(NAME,MAXSIZE) \
    char (NAME)[MAXSIZE] = { 0 }; \
    { \
        FfxCborCursor test; \
        ffx_cbor_clone(&test, &cursor); \
        status = ffx_cbor_followKey(&test, #NAME); \
        if (status) { printf("BAD FOLLOW STRING: " #NAME "\n"); break; } \
        status = ffx_cbor_copyData(&test, (uint8_t*)(NAME), MAXSIZE); \
        if (status) { printf("BAD COPY: " #NAME "\n"); break; } \
    }


#define READ_DATA(NAME) \
    const uint8_t *NAME = NULL; \
    size_t NAME##Length = 0; \
    { \
        FfxCborCursor test; \
        ffx_cbor_clone(&test, &cursor); \
        status = ffx_cbor_followKey(&test, #NAME); \
        if (status) { printf("BAD FOLLOW DATA: " #NAME "\n"); break; } \
        status = ffx_cbor_getData(&test, &NAME, &NAME##Length); \
        if (status) { printf("BAD GET DATA: " #NAME "\n"); break; } \
    }

#define READ_VALUE(NAME) \
    uint64_t NAME = 0;  \
    { \
        FfxCborCursor test; \
        ffx_cbor_clone(&test, &cursor); \
        status = ffx_cbor_followKey(&test, #NAME); \
        if (status) { printf("BAD FOLLOW VALUE: " #NAME "\n"); break; } \
        status = ffx_cbor_getValue(&test, &NAME); \
        if (status) { printf("BAD GET VALUE: " #NAME "\n"); break; } \
    }


#define OPEN_ARRAY() \
    { \
        FfxCborCursor store; \
        ffx_cbor_clone(&store, &cursor); \
        status = ffx_cbor_firstValue(&cursor, NULL); \
        while (!status) {

#define CLOSE_ARRAY() \
            status = ffx_cbor_nextValue(&cursor, NULL); \
        } \
        ffx_cbor_clone(&cursor, &store); \
    }

#define OPEN_AND_READ_ARRAY(NAME) \
    { \
        FfxCborCursor store; \
        ffx_cbor_clone(&store, &cursor); \
        status = ffx_cbor_followKey(&cursor, #NAME); \
        if (status) { printf("BAD FOLLOW ARRAY\n"); break; } \
        status = ffx_cbor_firstValue(&cursor, NULL); \
        while (!status) {

#define START_TESTS(NAME) \
    FfxCborCursor cursor; \
    ffx_cbor_walk(&cursor, tests_##NAME, sizeof(tests_##NAME)); \
    FfxCborStatus status = FfxCborStatusOK; \
    size_t countPass = 0, countFail = 0, countSkip = 0;

#define END_TESTS(NAME) \
    if (status != FfxCborStatusOK && status != FfxCborStatusNotFound) { \
        printf("CBOR Error: status=%d test=" #NAME "\n", status); \
        countFail++; \
    } \
    printf(#NAME ": pass=%zu fail=%zu skip%zu\n", countPass, countFail, countSkip); \
    return countFail;


///////////////////////////////
// Test Suites

int test_accounts() {
    START_TESTS(accounts)

    OPEN_ARRAY()

        READ_STRING(name, 128)
        READ_DATA(privkey)
        READ_STRING(address, 128)

        int result = runTestAccounts(name, privkey, address);

        if (result) {
            printf("FAIL: %s\n", name);
            countFail++;
        } else {
            countPass++;
        }
    CLOSE_ARRAY()

    END_TESTS(accounts)
}


int test_hashes() {
    START_TESTS(hashes)

    OPEN_ARRAY()

        READ_STRING(name, 128)

        READ_DATA(data)
        READ_DATA(sha256)
        READ_DATA(sha512)
        READ_DATA(keccak256)

        int result = runTestHashes(name, data, dataLength, sha256,
          sha256Length, sha512, sha512Length, keccak256, keccak256Length);

        if (result) {
            printf("FAIL: %s\n", name);
            countFail++;
        } else {
            countPass++;
        }
    CLOSE_ARRAY()

    END_TESTS(hashes)
}

int test_mnemonics() {
    START_TESTS(mnemonics)

    OPEN_ARRAY()
        READ_STRING(name, 128)
        READ_STRING(phrase, 512)
        READ_STRING(password, 256)
        READ_DATA(entropy)
        READ_DATA(seed)

        if (!onlyAscii(phrase) || !onlyAscii(password)) {
            countSkip++;

        } else {
            int result = runTestMnemonics(name, phrase, password,
            strlen(password), entropy, entropyLength, seed, seedLength);

            if (result) {
                printf("FAIL: %s\n", name);
                countFail++;

            } else {
                countPass++;

                OPEN_AND_READ_ARRAY(nodes)
                    READ_DATA(chaincode)
                    READ_DATA(pubkey)
                    READ_DATA(privkey)
                    READ_VALUE(depth)
                    READ_VALUE(index)

                    FfxHDNode node = { 0 };
                    if (!ffx_hdnode_initSeed(&node, seed)) {
                        printf("bad seedd\n");
                        break;
                    }

                    READ_STRING(_path, 128)

                    OPEN_AND_READ_ARRAY(path)
                        uint64_t index = 0;
                        status = ffx_cbor_getValue(&cursor, &index);
                        if (status) { break; }
                        ffx_hdnode_deriveChild(&node, index);
                    CLOSE_ARRAY()

                    int result = runTestMnemonicsNode(&node, chaincode,
                      privkey, pubkey, depth, index);

                    if (result) {
                        printf("FAIL: %s (%s)\n", name, _path);
                        countFail++;

                    } else {
                        countPass++;
                    }
                CLOSE_ARRAY()
            }
        }
    CLOSE_ARRAY()

    END_TESTS(mnemonics)
}

int test_pbkdf() {
    START_TESTS(pbkdf)

    OPEN_ARRAY()

        READ_STRING(name, 128)
        READ_DATA(password)
        READ_DATA(salt)
        READ_DATA(key)
        READ_VALUE(iterations)
        READ_VALUE(dkLength)
        READ_VALUE(algorithm)

        uint8_t actKey[dkLength];
        memset(actKey, 0, dkLength);
        if (algorithm == 256) {
            ffx_pbkdf2_sha256(actKey, dkLength, iterations, password,
              passwordLength, salt, saltLength);
        } else {
            ffx_pbkdf2_sha512(actKey, dkLength, iterations, password,
              passwordLength, salt, saltLength);
        }

        int result = cmpbuf(actKey, key, dkLength);

        if (result) {
            printf("FAIL: %s\n", name);
            countFail++;
        } else {
            countPass++;
        }
    CLOSE_ARRAY()

    END_TESTS(pbkdf)
}


///////////////////////////////
// Test Bootstrap

int main() {
    size_t countFail = 0;

    countFail += test_accounts();
    countFail += test_hashes();
    countFail += test_mnemonics();
    countFail += test_pbkdf();
/*

    FfxHDNode node = { 0 };

    uint8_t seed[] = { 11, 139, 217, 220, 95, 249, 13, 135, 198, 68, 130, 225, 29, 136, 171, 230, 5, 132, 33, 94, 28, 143, 70, 111, 81, 255, 171, 43, 75, 41, 193, 240, 237, 235, 21, 148, 41, 174, 213, 207, 156, 218, 173, 167, 124, 175, 61, 219, 76, 41, 140, 60, 54, 39, 64, 232, 188, 238, 100, 247, 90, 61, 116, 106 };
    if (!ffx_hdnode_initSeed(&node, seed, 64)) {
        printf("bad seedd\n");
    }
    printf("\n");

    uint8_t pubkey[65];

    dumpBuffer("Chaincode0: ", node.chaincode, 32);
    dumpBuffer("Key0: ", node.key, 65);
    if (!ffx_hdnode_getPubkey(&node, false, pubkey)) {
        printf("failed getPubkey\n");
    }
    dumpBuffer("PubKey0: ", pubkey, 65);
    if (!ffx_hdnode_getPubkey(&node, true, pubkey)) {
        printf("failed getPubkey\n");
    }
    dumpBuffer("PubKey0-comp: ", pubkey, 33);
    printf("\n");

    if (!ffx_hdnode_deriveChild(&node, 0x1)) {
        printf("failed derive\n");
    }

    dumpBuffer("Chaincode1: ", node.chaincode, 32);
    dumpBuffer("Key1: ", node.key, 65);
    if (!ffx_hdnode_getPubkey(&node, false, pubkey)) {
        printf("failed getPubkey\n");
    }
    dumpBuffer("PubKey1: ", pubkey, 65);
    if (!ffx_hdnode_getPubkey(&node, true, pubkey)) {
        printf("failed getPubkey\n");
    }
    dumpBuffer("PubKey1-comp: ", pubkey, 33);
    printf("\n");

    if (!ffx_hdnode_deriveChild(&node, 0x1)) {
        printf("failed derive\n");
    }

    dumpBuffer("Chaincode2: ", node.chaincode, 32);
    dumpBuffer("Key2: ", node.key, 65);
    if (!ffx_hdnode_getPubkey(&node, false, pubkey)) {
        printf("failed getPubkey\n");
    }
    dumpBuffer("PubKey2: ", pubkey, 65);
    if (!ffx_hdnode_getPubkey(&node, true, pubkey)) {
        printf("failed getPubkey\n");
    }
    dumpBuffer("PubKey2-comp: ", pubkey, 33);
    printf("\n");
*/
    return countFail;
}
