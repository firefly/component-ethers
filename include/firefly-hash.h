#ifndef __FIREFLY_HASH_H__
#define __FIREFLY_HASH_H__

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#include <stddef.h>
#include <stdint.h>


#define FFX_KECCAK256_DIGEST_LENGTH          (32)

#define FFX_SHA256_DIGEST_LENGTH             (32)
#define FFX_SHA512_DIGEST_LENGTH             (64)


#define _ffx_sha3_max_permutation_size (25)
#define _ffx_sha3_max_rate_in_qwords (24)
#define _ffx_sha256_block_length (64)
#define _ffx_sha512_block_length (128)


typedef struct FfxKeccak256Context {
    /* 1600 bits algorithm hashing state */
    uint64_t hash[_ffx_sha3_max_permutation_size];

    /* 1536-bit buffer for leftovers */
    uint64_t message[_ffx_sha3_max_rate_in_qwords];

    /* count of bytes in the message[] buffer */
    uint16_t rest;
} FfxKeccak256Context;

typedef struct FfxSha256Context {
    int32_t tot_len;
    uint32_t len;
    uint8_t block[2 * _ffx_sha256_block_length];
    uint32_t h[8];
} FfxSha256Context;

typedef struct FfxSha512Context {
    uint32_t tot_len;
    uint32_t len;
    uint8_t block[2 * _ffx_sha512_block_length];
    uint64_t h[8];
} FfxSha512Context;

typedef struct FfxHmacSha256Context {
    FfxSha256Context ctx_inside;
    FfxSha256Context ctx_outside;

    uint8_t block_ipad[_ffx_sha256_block_length];
    uint8_t block_opad[_ffx_sha256_block_length];
} FfxHmacSha256Context;

typedef struct FfxHmacSha512Context {
    FfxSha512Context ctx_inside;
    FfxSha512Context ctx_outside;

    uint8_t block_ipad[_ffx_sha512_block_length];
    uint8_t block_opad[_ffx_sha512_block_length];
} FfxHmacSha512Context;



void ffx_hash_initKeccak256(FfxKeccak256Context *context);
void ffx_hash_updateKeccak256(FfxKeccak256Context *context,
  const uint8_t *data, size_t length);
void ffx_hash_finalKeccak256(FfxKeccak256Context *context, uint8_t* digest);

void ffx_hash_keccak256(uint8_t *digest, const uint8_t *data, size_t length);


void ffx_hash_initSha256(FfxSha256Context *context);
void ffx_hash_updateSha256(FfxSha256Context *context, const uint8_t *data,
  size_t length);
void ffx_hash_finalSha256(FfxSha256Context *context, uint8_t *digest);
void ffx_hash_sha256(uint8_t *digest, const uint8_t *data, size_t length);

void ffx_hmac_initSha256(FfxHmacSha256Context *context,
  const uint8_t *key, size_t length);
void ffx_hmac_updateSha256(FfxHmacSha256Context *context,
  const uint8_t *data, size_t length);
void ffx_hmac_finalSha256(FfxHmacSha256Context *context, uint8_t *hmac);
void ffx_hmac_sha256(uint8_t *digest, const uint8_t *key, size_t keyLen,
  const uint8_t *data, size_t dataLen);

void ffx_hash_initSha512(FfxSha512Context *context);
void ffx_hash_updateSha512(FfxSha512Context *context, const uint8_t *data,
  size_t length);
void ffx_hash_finalSha512(FfxSha512Context *context, uint8_t *digest);
void ffx_hash_sha512(uint8_t *digest, const uint8_t *data, size_t length);

void ffx_hmac_initSha512(FfxHmacSha512Context *context,
  const uint8_t *key, size_t length);
void ffx_hmac_updateSha512(FfxHmacSha512Context *context,
  const uint8_t *data, size_t length);
void ffx_hmac_finalSha512(FfxHmacSha512Context *context, uint8_t *hmac);
void ffx_hmac_sha512(uint8_t *digest, const uint8_t *key, size_t keyLen,
  const uint8_t *data, size_t dataLen);


#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* __FIREFLY_HASH__ */
