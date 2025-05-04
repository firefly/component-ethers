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

/*
typedef struct FfxWordlistIterator {
    const char* word;
    size_t length;
    size_t index;

    size_t _nextOffset;
} FfxWordlistIterator;

FfxWordlistIterator ffx_bip39_iterate();
bool ffx_bip39_nextWord(FfxWordlistIterator *iterator);
*/

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


#define FFX_BIP39_SEED_LENGTH           (64)


typedef struct FfxMnemonic {
    size_t wordCount;
    size_t entropyLength;
    uint8_t entropy[33]; // Incudes checksum (but entropyLength does not)
} FfxMnemonic;


/**
 *  Initialize a %%mnemonic%% with the %%phrase%%.
 *
 *  Returns false on any error:
 *    - the phrase checksum does not match
 *    - an invalid word (in the English BIP39 wordlist) is present
 *    - the length of the phrase (in words) is invalid
 */
bool ffx_mnemonic_initPhrase(FfxMnemonic *mnemonic, const char* phrase);

/**
 *  Initialize a %%mnemonic%% with the %%entropy%% and length.
 *
 *  Returns false on an invalid length.
 */
bool ffx_mnemonic_initEntropy(FfxMnemonic *mnemonic, const uint8_t* entropy,
  size_t length);

/**
 *  Returns the %%index%% word of %%mnemonic%% or NULL if %%index%% is out
 *  of range.
 */
const char* ffx_mnemonic_getWord(FfxMnemonic *mnemonic, int index);

/**
 *  Returns the length of the phrase for %%mnemonic%% including the
 *  NULL-termination.
 */
size_t ffx_mnemonic_getPhraseLength(FfxMnemonic *mnemonic);

/**
 *  Writes the phrase for %%mnemonic%% to %%phraseOut%%, returning false
 *  if its %%length%% isn't long enough to hold the entire phrase.
 *
 *  The string is NULL-temrinated.
 */
bool ffx_mnemonic_getPhrase(FfxMnemonic *mnemonic, char* phraseOut,
  size_t length);

/**
 *  Writes the seed to %%seedOut%% (which MUST be of length
 *  [[FFX_BIP39_SEED_LENGTH]], for %%mnemonic%% with %%password%%.
 *
 *  If there is no password, use the default password of the empty string, "".
 */
bool ffx_mnemonic_getSeed(FfxMnemonic *mnemonic, const char* password,
  uint8_t *seedOut);


///////////////////////////////
// HD Node

/**
 *  This can be OR-ed (or added) to an index to derive a hardened child.
 */
#define FfxHDNodeHardened      (0x80000000)

/**
 *  The default path for a Mnemonic on Ethereum.
 *
 *  There are two standards commonly used to adjust this path per-account:
 *    - m/44'/60'/${ index }'/0/0   (used by Ledger)
 *    - m/44'/60'/0'/0/${ index }   (used by MetaMask)
 */
#define FfxDefaultMnemonicPath ("m/44'/60'/0'/0/0")

//#define FFX_HDNODE_EXTENDED_KEY_MAX_LENGTH

/**
 *  An Hierarchal-Deterministic Node.
 */
typedef struct FfxHDNode {
    // Hold either a privkey (key[0] = 0) or a pubkey (key[0] = 4)
    uint8_t key[65];

    uint8_t chaincode[32];

    uint32_t depth;
    uint32_t index;
} FfxHDNode;

/**
 *  Initialize an HDNode for %%seed%%.
 */
bool ffx_hdnode_initSeed(FfxHDNode *node, const uint8_t *seed);

//bool ffx_hdnode_initExtkey(FfxHDNode *node, const char *extkey);

/**
 *  Derives the %%index%% child of %%node%%, returning false on failure.
 */
bool ffx_hdnode_deriveChild(FfxHDNode *node, uint32_t index);

/**
 *  Derives the child given by %%path%% for %%node%%, returning false
 *  on failure.
 */
bool ffx_hdnode_derivePath(FfxHDNode *node, const char* path);

/**
 *  Derive the %%account%% for %%node%% using
 *  ``m/44'/60'/${ account }'/0/0``, returning false on faliure.
 *
 *  This is the derivation scheme used by Ledger.
 */
bool ffx_hdnode_deriveAccount(FfxHDNode *node, uint32_t account);

/**
 *  Derive the indexed %%account%% for %%node%% using
 *  ``m/44'/60'/0'/0/${ account }``, returning false on failure.
 *
 *  This is the derivation scheme used by MetaMask.
 */
bool ffx_hdnode_deriveIndexedAccount(FfxHDNode *node, uint32_t account);

/**
 *  Removes the ability to derive child private keys from %%node%%.
 */
bool ffx_hdnode_neuter(FfxHDNode *node);

/**
 *  Returns true if %%node%% is neutered and therefor only public keys can
 *  be derived.
 */
bool ffx_hdnode_isNeuter(FfxHDNode *node);

/**
 *  Writes the private key to %%privkeyOut%%, returning false on failure.
 *
 *  The length of %%privkeyOut%% must be [[FFX_PRIVKEY_LENGTH]].
 */
bool ffx_hdnode_getPrivkey(FfxHDNode *node, uint8_t *privkeyOut);

/**
 *  Writes the public key to %%pubkeyOut%%, returning false on failure.
 *
 *  The length of %%pubkeyOut%% must be:
 *    - if %%compressed%%, [[FFX_COMP_PUBKEY_LENGTH]] bytes
 *    - otherwise, [[FFX_PUBKEY_LENGTH]] bytes
 */
bool ffx_hdnode_getPubkey(FfxHDNode *node, bool compressed,
  uint8_t *pubkeyOut);

//bool ffx_hdnode_getExtendedKey(FfxHDNode *node, char *extkeyOut);




#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FIREFLY_BIP39_H__ */
