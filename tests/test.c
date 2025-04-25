#include <stdio.h>
#include <string.h>

#include "firefly-address.h"
#include "firefly-bip32.h"
#include "firefly-cbor.h"
#include "firefly-crypto.h"
#include "firefly-hash.h"
#include "firefly-tx.h"

#include "testcases-h/accounts.h"
#include "testcases-h/hashes.h"
#include "testcases-h/hmac.h"
#include "testcases-h/mnemonics.h"
#include "testcases-h/pbkdf.h"
#include "testcases-h/transactions.h"


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

int runTestTxs(const char* name, const uint8_t *privkey, FfxCborCursor *tx,
  const uint8_t *sig, const uint8_t *rlpUnsigned, size_t rlpUnsignedLength,
  const uint8_t *rlpSigned, size_t rlpSignedLength) {

    uint8_t bytes[1024];
    FfxTx result = ffx_tx_serializeUnsigned(tx, bytes, sizeof(bytes));

    if (result.status) {
        printf("Failed to serialize unsigned tx: status=%d\n", result.status);
        ffx_cbor_dump(tx);
        return 1;
    }

    if (result.rlp.length != rlpUnsignedLength ||
      cmpbuf(result.rlp.bytes, rlpUnsigned, rlpUnsignedLength)) {
        printf("Unsigned TX RLP did not match\n");
        ffx_cbor_dump(tx);
        dumpBuffer("Actual:   ", result.rlp.bytes, result.rlp.length);
        dumpBuffer("Expected: ", rlpUnsigned, rlpUnsignedLength);
        return 1;
    }

    return 0;
}


///////////////////////////////
// Test Data Macros

#define MIN(a,b)    ((a) < (b) ? (a): (b))

#define READ_STRING(NAME,MAXSIZE) \
    char (NAME)[MAXSIZE] = { 0 }; \
    { \
        FfxCborCursor child = ffx_cbor_followKey(&cursor, #NAME); \
        if (child.error) { \
            printf("BAD FOLLOW STRING: " #NAME "\n"); break; \
        } \
        FfxDataResult data = ffx_cbor_getData(&child); \
        if (data.error) { printf("BAD COPY: " #NAME "\n"); break; } \
        memcpy((NAME), data.bytes, MIN((MAXSIZE) - 1, data.length)); \
    }


#define READ_DATA(NAME) \
    FfxDataResult NAME = { 0 }; \
    { \
        FfxCborCursor child = ffx_cbor_followKey(&cursor, #NAME); \
        if (child.error) { \
            printf("BAD FOLLOW DATA: key=" #NAME "\n"); break; \
        } \
        (NAME) = ffx_cbor_getData(&child); \
        if ((NAME).error) { printf("BAD GET DATA: " #NAME "\n"); break; } \
    }

#define READ_VALUE(NAME) \
    uint64_t NAME = 0;  \
    { \
        FfxCborCursor child = ffx_cbor_followKey(&cursor, #NAME); \
        if (child.error) { \
            printf("BAD FOLLOW VALUE: " #NAME "\n"); break; \
        } \
        FfxValueResult result = ffx_cbor_getValue(&child); \
        if (result.error) { printf("BAD GET VALUE: " #NAME "\n"); break; } \
        NAME = result.value; \
    }


#define OPEN_ARRAY() \
    { \
        FfxCborIterator iter = ffx_cbor_iterate(&cursor); \
        while (ffx_cbor_nextChild(&iter)) { \
            FfxCborCursor cursor = iter.child;

#define CLOSE_ARRAY() \
        } \
        if (iter.error) { printf("status=%d\n", iter.error); } \
    }

#define OPEN_AND_READ_ARRAY(NAME) \
    { \
        FfxCborCursor follow = ffx_cbor_followKey(&cursor, #NAME); \
        if (follow.error) { printf("BAD FOLLOW ARRAY\n"); break; } \
        FfxCborIterator iter = ffx_cbor_iterate(&follow); \
        while (ffx_cbor_nextChild(&iter)) { \
            FfxCborCursor cursor = iter.child;

#define START_TESTS(NAME) \
    FfxDataError error = FfxDataErrorNone; \
    FfxCborCursor cursor = ffx_cbor_walk(tests_##NAME, sizeof(tests_##NAME)); \
    size_t countPass = 0, countFail = 0, countSkip = 0;

#define END_TESTS(NAME) \
    if (error) { \
        printf("CBOR Error: status=%d test=" #NAME "\n", error); \
        countFail++; \
    } \
    printf(#NAME ": pass=%zu fail=%zu skip=%zu\n", countPass, countFail, countSkip); \
    return countFail;


///////////////////////////////
// Test Suites

int test_accounts() {
    START_TESTS(accounts)

    OPEN_ARRAY()

        READ_STRING(name, 128)
        READ_DATA(privkey)
        READ_STRING(address, 128)

        int result = runTestAccounts(name, privkey.bytes, address);

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

        int result = runTestHashes(name, data.bytes, data.length,
          sha256.bytes, sha256.length, sha512.bytes, sha512.length,
          keccak256.bytes, keccak256.length);

        if (result) {
            printf("FAIL: %s\n", name);
            countFail++;
        } else {
            countPass++;
        }
    CLOSE_ARRAY()

    END_TESTS(hashes)
}

int test_hmac() {
    START_TESTS(hmac)

    OPEN_ARRAY()

        READ_STRING(name, 128)

        READ_DATA(data)
        READ_DATA(key)
        READ_DATA(hmac)
        READ_VALUE(algorithm)

        size_t length = algorithm / 8;

        uint8_t digest[length];
        memset(digest, 0, length);

        if (algorithm == 256) {
            ffx_hmac_sha256(digest, key.bytes, key.length,
              data.bytes, data.length);
        } else {
            ffx_hmac_sha512(digest, key.bytes, key.length,
              data.bytes, data.length);
        }

        int result = cmpbuf(digest, hmac.bytes, length);

        if (result) {
            printf("FAIL: %s\n", name);
            countFail++;
        } else {
            countPass++;
        }
    CLOSE_ARRAY()

    END_TESTS(hmac)
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
              strlen(password), entropy.bytes, entropy.length,
              seed.bytes, seed.length);

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

                    READ_STRING(_path, 128)

                    // Derive child by path
                    {
                        FfxHDNode node = { 0 };

                        if (!ffx_hdnode_initSeed(&node, seed.bytes)) {
                            printf("bad seed\n");
                            break;
                        }

                        if (!ffx_hdnode_derivePath(&node, _path)) {
                            printf("bad path\n");
                            break;
                        }

                        int result = runTestMnemonicsNode(&node,
                          chaincode.bytes, privkey.bytes, pubkey.bytes,
                          depth, index);

                        if (result) {
                            printf("FAIL: %s (%s)\n", name, _path);
                            countFail++;

                        } else {
                            countPass++;
                        }
                    }

                    // Derive child incrementally by components
                    {
                        FfxHDNode node = { 0 };

                        if (!ffx_hdnode_initSeed(&node, seed.bytes)) {
                            printf("bad seedd\n");
                            break;
                        }

                        OPEN_AND_READ_ARRAY(path)
                            FfxValueResult result = ffx_cbor_getValue(&cursor);
                            if (result.error) { break; }
                            ffx_hdnode_deriveChild(&node, result.value);
                        CLOSE_ARRAY()

                        int result = runTestMnemonicsNode(&node,
                          chaincode.bytes, privkey.bytes, pubkey.bytes,
                          depth, index);

                        if (result) {
                            printf("FAIL: %s (%s)\n", name, _path);
                            countFail++;

                        } else {
                            countPass++;
                        }
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
            ffx_pbkdf2_sha256(actKey, dkLength, iterations, password.bytes,
              password.length, salt.bytes, salt.length);
        } else {
            ffx_pbkdf2_sha512(actKey, dkLength, iterations, password.bytes,
              password.length, salt.bytes, salt.length);
        }

        int result = cmpbuf(actKey, key.bytes, dkLength);

        if (result) {
            printf("FAIL: %s\n", name);
            countFail++;
        } else {
            countPass++;
        }
    CLOSE_ARRAY()

    END_TESTS(pbkdf)
}

int test_transactions() {
    START_TESTS(transactions)

    OPEN_ARRAY()

        READ_STRING(name, 128)

        READ_DATA(privkey)
        READ_DATA(sig)
        READ_DATA(rlpUnsigned)
        READ_DATA(rlpSigned)

        FfxCborCursor tx = ffx_cbor_followKey(&cursor, "tx");
        if (tx.error) { break; }

//if (!strcmp(name, "random-22")) {

        int result = runTestTxs(name, privkey.bytes, &tx, sig.bytes,
          rlpUnsigned.bytes, rlpUnsigned.length,
          rlpSigned.bytes, rlpSigned.length);

        if (result) {
            printf("FAIL: %s\n", name);
            countFail++;
        } else {
            countPass++;
        }
//}
    CLOSE_ARRAY()

    END_TESTS(transactions)
}


///////////////////////////////
// Test Bootstrap

int main() {
    size_t countFail = 0;

    countFail += test_accounts();
    countFail += test_hashes();
    countFail += test_hmac();
    countFail += test_mnemonics();
    countFail += test_pbkdf();
    countFail += test_transactions();

    printf("Total: %zu failed\n", countFail);

    return countFail;
}
