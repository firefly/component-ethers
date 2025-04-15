
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

void dumpBuffer(const char *header, const uint8_t *buffer, size_t length) {
    printf("%s 0x", header);
    for (int i = 0; i < length; i++) {
        printf("%02x", buffer[i]);
    }
    printf(" (length=%zu)\n", length);
}

int cmpbuf(const uint8_t *a, const uint8_t *b, size_t length) {
    for (int i = 0; i < length; i++) {
        int d = a[i] - b[i];
        if (d) { return d; }
    }

    return 0;
}


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

int onlyAscii(const char *text) {
    for (int i = strlen(text) - 1; i >= 0; i--) {
        if (text[i] < 32 || text[i] > 126) { return false; }
    }
    return true;
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


int test_accounts() {
    FfxCborCursor cursor;
    ffx_cbor_walk(&cursor, tests_accounts, sizeof(tests_accounts));

    size_t countPass = 0, countFail = 0;

    FfxCborStatus status = ffx_cbor_firstValue(&cursor, NULL);
    while (!status) {
        FfxCborCursor test;

        char name[128] = { 0 };
        const uint8_t *privkey = NULL;
        char address[128] = { 0 };

        ffx_cbor_clone(&test, &cursor);
        status = ffx_cbor_followKey(&test, "name");
        if (status) { break; }
        status = ffx_cbor_copyData(&test, (uint8_t*)name, sizeof(name));
        if (status) { break; }

        ffx_cbor_clone(&test, &cursor);
        status = ffx_cbor_followKey(&test, "privkey");
        if (status) { break; }
        status = ffx_cbor_getData(&test, &privkey, NULL);
        if (status) { break; }

        ffx_cbor_clone(&test, &cursor);
        status = ffx_cbor_followKey(&test, "address");
        if (status) { break; }
        status = ffx_cbor_copyData(&test, (uint8_t*)address, sizeof(address));
        if (status) { break; }

        int result = runTestAccounts(name, privkey, address);

        if (result) {
            printf("FAIL: %s\n", name);
            countFail++;
        } else {
            countPass++;
        }

        status = ffx_cbor_nextValue(&cursor, NULL);
    }

    if (status != FfxCborStatusOK && status != FfxCborStatusNotFound) {
        printf("CBOR Error: status=%d\n", status);
        countFail++;
    }

    printf("Accounts: pass=%zu fail=%zu\n", countPass, countFail);

    return countFail;
}

int test_hashes() {
    FfxCborCursor cursor;
    ffx_cbor_walk(&cursor, tests_hashes, sizeof(tests_hashes));

    size_t countPass = 0, countFail = 0;

    FfxCborStatus status = ffx_cbor_firstValue(&cursor, NULL);
    while (!status) {
        FfxCborCursor test;

        char name[128] = { 0 };
        const uint8_t *data = NULL, *sha256 = NULL, *sha512 = NULL, *keccak256 = NULL;
        size_t dataLen = 0, sha256Len = 0, sha512Len = 0, keccak256Len = 0;

        ffx_cbor_clone(&test, &cursor);
        status = ffx_cbor_followKey(&test, "name");
        if (status) { break; }
        status = ffx_cbor_copyData(&test, (uint8_t*)name, sizeof(name));
        if (status) { break; }

        ffx_cbor_clone(&test, &cursor);
        status = ffx_cbor_followKey(&test, "data");
        if (status) { break; }
        status =ffx_cbor_getData(&test, &data, &dataLen);
        if (status) { break; }

        ffx_cbor_clone(&test, &cursor);
        status = ffx_cbor_followKey(&test, "sha256");
        if (status) { break; }
        status = ffx_cbor_getData(&test, &sha256, &sha256Len);
        if (status) { break; }

        ffx_cbor_clone(&test, &cursor);
        status = ffx_cbor_followKey(&test, "sha512");
        if (status) { break; }
        status = ffx_cbor_getData(&test, &sha512, &sha512Len);
        if (status) { break; }

        ffx_cbor_clone(&test, &cursor);
        status = ffx_cbor_followKey(&test, "keccak256");
        if (status) { break; }
        status = ffx_cbor_getData(&test, &keccak256, &keccak256Len);
        if (status) { break; }

        int result = runTestHashes(name, data, dataLen, sha256, sha256Len,
          sha512, sha512Len, keccak256, keccak256Len);

        if (result) {
            printf("FAIL: %s\n", name);
            countFail++;
        } else {
            countPass++;
        }

        status = ffx_cbor_nextValue(&cursor, NULL);
    }

    if (status != FfxCborStatusOK && status != FfxCborStatusNotFound) {
        printf("CBOR Error: status=%d\n", status);
        countFail++;
    }

    printf("Hashes: pass=%zu fail=%zu\n", countPass, countFail);

    return countFail;
}

int test_mnemonics() {
    FfxCborCursor cursor;
    ffx_cbor_walk(&cursor, tests_mnemonics, sizeof(tests_mnemonics));

    size_t countPass = 0, countFail = 0, countSkip = 0;

    FfxCborStatus status = ffx_cbor_firstValue(&cursor, NULL);
    while (!status) {
        FfxCborCursor test;

        char name[128] = { 0 };
        char phrase[512] = { 0 };
        char password[256] = { 0 };
        const uint8_t *entropy = NULL, *seed = NULL;
        size_t passwordLen = 0, entropyLen = 0, seedLen = 0;

        ffx_cbor_clone(&test, &cursor);
        status = ffx_cbor_followKey(&test, "name");
        if (status) { break; }
        status = ffx_cbor_copyData(&test, (uint8_t*)name, sizeof(name));
        if (status) { break; }

        ffx_cbor_clone(&test, &cursor);
        status = ffx_cbor_followKey(&test, "phrase");
        if (status) { break; }
        status = ffx_cbor_copyData(&test, (uint8_t*)phrase, sizeof(phrase));
        if (status) { break; }

        ffx_cbor_clone(&test, &cursor);
        status = ffx_cbor_followKey(&test, "password");
        if (status) { break; }
        status = ffx_cbor_copyData(&test, (uint8_t*)password, sizeof(password));
        if (status) { break; }

        ffx_cbor_clone(&test, &cursor);
        status = ffx_cbor_followKey(&test, "entropy");
        if (status) { break; }
        status = ffx_cbor_getData(&test, &entropy, &entropyLen);
        if (status) { break; }

        ffx_cbor_clone(&test, &cursor);
        status = ffx_cbor_followKey(&test, "seed");
        if (status) { break; }
        status = ffx_cbor_getData(&test, &seed, &seedLen);
        if (status) { break; }

        if (!onlyAscii(phrase) || !onlyAscii(password)) {
            countSkip++;

        } else {
            int result = runTestMnemonics(name, phrase, password, passwordLen,
              entropy, entropyLen, seed, seedLen);

            if (result) {
                printf("FAIL: %s\n", name);
                countFail++;

            } else {
                countPass++;

                ffx_cbor_clone(&test, &cursor);
                status = ffx_cbor_followKey(&test, "nodes");
                if (status) { break; }


                status = ffx_cbor_firstValue(&test, NULL);
                while (!status) {
                    FfxCborCursor nodeTest;

                    char path[64] = { 0 };
                    const uint8_t *chaincode = NULL, *pubkey = NULL, *privkey = NULL;
                    uint64_t depth = 0, index = 0;

                    ffx_cbor_clone(&nodeTest, &test);
                    status = ffx_cbor_followKey(&nodeTest, "chaincode");
                    if (status) { break; }
                    status = ffx_cbor_getData(&nodeTest, &chaincode, NULL);
                    if (status) { break; }

                    ffx_cbor_clone(&nodeTest, &test);
                    status = ffx_cbor_followKey(&nodeTest, "pubkey");
                    if (status) { break; }
                    status = ffx_cbor_getData(&nodeTest, &pubkey, NULL);
                    if (status) { break; }

                    ffx_cbor_clone(&nodeTest, &test);
                    status = ffx_cbor_followKey(&nodeTest, "privkey");
                    if (status) { break; }
                    status = ffx_cbor_getData(&nodeTest, &privkey, NULL);
                    if (status) { break; }

                    ffx_cbor_clone(&nodeTest, &test);
                    status = ffx_cbor_followKey(&nodeTest, "depth");
                    if (status) { break; }
                    status = ffx_cbor_getValue(&nodeTest, &depth);
                    if (status) { break; }

                    ffx_cbor_clone(&nodeTest, &test);
                    status = ffx_cbor_followKey(&nodeTest, "index");
                    if (status) { break; }
                    status = ffx_cbor_getValue(&nodeTest, &index);
                    if (status) { break; }

                    FfxHDNode node = { 0 };
                    if (!ffx_hdnode_initSeed(&node, seed)) {
                        printf("bad seedd\n");
                        break;
                    }

                    ffx_cbor_clone(&nodeTest, &test);
                    status = ffx_cbor_followKey(&nodeTest, "_path");
                    if (status) { break; }
                    status = ffx_cbor_copyData(&nodeTest, (uint8_t*)path, sizeof(path));
                    if (status) { break; }

                    ffx_cbor_clone(&nodeTest, &test);
                    status = ffx_cbor_followKey(&nodeTest, "path");
                    if (status) { break; }

                    status = ffx_cbor_firstValue(&nodeTest, NULL);
                    while (!status) {
                        uint64_t index = 0;

                        status = ffx_cbor_getValue(&nodeTest, &index);
                        if (status) { break; }

                        ffx_hdnode_deriveChild(&node, index);

                        status = ffx_cbor_nextValue(&nodeTest, NULL);
                    }

                    int result = runTestMnemonicsNode(&node, chaincode,
                      privkey, pubkey, depth, index);

                    if (result) {
                        printf("FAIL: %s (%s)\n", name, path);
                        countFail++;

                    } else {
                        countPass++;
                    }

                    status = ffx_cbor_nextValue(&test, NULL);
                }
            }
        }

        status = ffx_cbor_nextValue(&cursor, NULL);
    }

    if (status != FfxCborStatusOK && status != FfxCborStatusNotFound) {
        printf("CBOR Error: status=%d\n", status);
        countFail++;
    }

    printf("Mnemonics: pass=%zu fail=%zu skip=%zu\n", countPass, countFail,
      countSkip);

    return countFail;
}

int main() {
    size_t countFail = 0;

    countFail += test_accounts();
    countFail += test_hashes();
    countFail += test_mnemonics();
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
