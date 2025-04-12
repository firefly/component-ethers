#ifndef __FIREFLY_BIP39_H__
#define __FIREFLY_BIP39_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


///////////////////////////////
// BIP39 Wordlist - low-level functions

typedef struct FfxWordlistCursor {
    size_t index;
    size_t offset;
    size_t length;
} FfxWordlistCursor;


// Returns the index of the word (or -1 if not present)
int ffx_bip39_index(const char* const word);

// Returns the word at index (or NULL if outside the range [0, 2047])
const char* ffx_bip39_word(int index);

const char* ffx_bipd39_nextWord(FfxWordlistCursor *cursor, int *index);


///////////////////////////////
// Mnemonic

#define FFX_BIP39_SEED_LENGTH           (32)

typedef struct FfxMnemonic {
    size_t wordCount;
    size_t entropyLength;
    uint8_t entropy[33]; // Incudes checksum
} FfxMnemonic;


bool ffx_mnemonic_initPhrase(FfxMnemonic *mnemonic, const char* phrase);
bool ffx_mnemonic_initEntropy(FfxMnemonic *mnemonic, uint8_t* entropy,
  size_t length);

const char* ffx_mnemonic_getWord(FfxMnemonic *mnemonic, int index);

bool ffx_mnemonic_getSeed(FfxMnemonic *mnemonic, const char* password,
  uint8_t *seed);


///////////////////////////////
// HD Node

#define FfxHDNodeHardened      (0x80000000)

typedef struct FfxHDNode {
    // Hold either a privkey (key[0] = 0) or a pubkey (key[0] = 4)
    uint8_t key[65];

    uint8_t chaincode[32];

    uint32_t depth;
    uint32_t index;
} FfxHDNode;


//bool ffx_hdnode_initMnemonic(FfxHDNode *node, FfxMnemonic *mnemonic);
//bool ffx_hdnode_initPhrase(FfxHDNode *node, const char* phrase);
//bool ffx_hdnode_initEntropy(FfxHDNode *node, uint8_t *data, size_t length);
bool ffx_hdnode_initSeed(FfxHDNode *node, uint8_t *seed, size_t length);

void ffx_hdnode_clone(FfxHDNode *dst, FfxHDNode *src);

bool ffx_hdnode_deriveChild(FfxHDNode *node, uint32_t index);
bool ffx_hdnode_neuter(FfxHDNode *node);

bool ffx_hdnode_getPrivkey(FfxHDNode *node, uint8_t *privkey);
bool ffx_hdnode_getPubkey(FfxHDNode *node, bool compressed, uint8_t *pubkey);

//#define FFX_HDNODE_EXTENDED_KEY_MAX_LENGTH
//bool ffx-hdnode_getExtendedKey(FfxHDNode *node, char *extendedKey);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FIREFLY_BIP39_H__ */
