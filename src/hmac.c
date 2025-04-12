/**
 *  Original work by Olivier Gay; see license below.
 *
 *  The only changes are some API name changes to retain library
 *  naming conventions and prevent name collisions. And some unused
 *  functionality is removed.
 *
 *  See original work at: https://github.com/ogay/hmac
 *
 *  Modified by RicMoo<me@ricmoo.com> in 2025.
 */

/*
 * HMAC-SHA-224/256/384/512 implementation
 * Last update: 06/15/2005
 * Issue date:  06/15/2005
 *
 * Copyright (C) 2005 Olivier Gay <olivier.gay@a3.epfl.ch>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <string.h>

#include "firefly-hash.h"


#define SHA256_DIGEST_SIZE (FFX_SHA256_DIGEST_LENGTH)
#define SHA512_DIGEST_SIZE (FFX_SHA512_DIGEST_LENGTH)


#define SHA256_BLOCK_SIZE  (_ffx_sha256_block_length)
#define SHA512_BLOCK_SIZE  (_ffx_sha512_block_length)

static void sha256_init(FfxSha256Context * ctx) {
    ffx_hash_initSha256(ctx);
}
static void sha256_update(FfxSha256Context *ctx, const uint8_t *message,
                   size_t len) {
    ffx_hash_updateSha256(ctx, message, len);
}
static void sha256_final(FfxSha256Context *ctx, uint8_t *digest) {
    ffx_hash_finalSha256(ctx, digest);
}
static void sha256(const uint8_t *message, size_t len,
            uint8_t *digest) {
    ffx_hash_sha256(digest, message, len);
}


/* HMAC-SHA-256 functions */

static void hmac_sha256_init(FfxHmacSha256Context *ctx, const uint8_t *key,
                      size_t key_size)
{
    size_t fill;
    size_t num;

    const uint8_t *key_used;
    uint8_t key_temp[SHA256_DIGEST_SIZE];
    int i;

    if (key_size == SHA256_BLOCK_SIZE) {
        key_used = key;
        num = SHA256_BLOCK_SIZE;
    } else {
        if (key_size > SHA256_BLOCK_SIZE){
            num = SHA256_DIGEST_SIZE;
            sha256(key, key_size, key_temp);
            key_used = key_temp;
        } else { /* key_size > SHA256_BLOCK_SIZE */
            key_used = key;
            num = key_size;
        }
        fill = SHA256_BLOCK_SIZE - num;

        memset(ctx->block_ipad + num, 0x36, fill);
        memset(ctx->block_opad + num, 0x5c, fill);
    }

    for (i = 0; i < (int) num; i++) {
        ctx->block_ipad[i] = key_used[i] ^ 0x36;
        ctx->block_opad[i] = key_used[i] ^ 0x5c;
    }

    sha256_init(&ctx->ctx_inside);
    sha256_update(&ctx->ctx_inside, ctx->block_ipad, SHA256_BLOCK_SIZE);

    sha256_init(&ctx->ctx_outside);
    sha256_update(&ctx->ctx_outside, ctx->block_opad,
                  SHA256_BLOCK_SIZE);
}


static void hmac_sha256_update(FfxHmacSha256Context *ctx, const uint8_t *message,
                        size_t message_len)
{
    sha256_update(&ctx->ctx_inside, message, message_len);
}

static void hmac_sha256_final(FfxHmacSha256Context *ctx, uint8_t *mac,
                       size_t mac_size)
{
    uint8_t digest_inside[SHA256_DIGEST_SIZE];
    uint8_t mac_temp[SHA256_DIGEST_SIZE];

    sha256_final(&ctx->ctx_inside, digest_inside);
    sha256_update(&ctx->ctx_outside, digest_inside, SHA256_DIGEST_SIZE);
    sha256_final(&ctx->ctx_outside, mac_temp);
    memcpy(mac, mac_temp, mac_size);
}

static void hmac_sha256(const uint8_t *key, size_t key_size,
          const uint8_t *message, size_t message_len,
          uint8_t *mac, unsigned mac_size)
{
    FfxHmacSha256Context ctx;

    hmac_sha256_init(&ctx, key, key_size);
    hmac_sha256_update(&ctx, message, message_len);
    hmac_sha256_final(&ctx, mac, mac_size);
}

void ffx_hmac_initSha256(FfxHmacSha256Context *context,
  const uint8_t *key, size_t length) {

    hmac_sha256_init(context, key, length);
}

void ffx_hmac_updateSha256(FfxHmacSha256Context *context,
  const uint8_t *data, size_t length) {
    hmac_sha256_update(context, data, length);
}

void ffx_hmac_finalSha256(FfxHmacSha256Context *context, uint8_t *hmac) {
    hmac_sha256_final(context, hmac, SHA256_DIGEST_SIZE);
}

void ffx_hmac_sha256(uint8_t *digest, uint8_t *key, size_t keyLen,
  uint8_t *data, size_t dataLen) {
    hmac_sha256(key, keyLen, data, dataLen, digest, SHA256_DIGEST_SIZE);
}



/* HMAC-SHA-512 functions */

static void sha512_init(FfxSha512Context *ctx) {
    ffx_hash_initSha512(ctx);
}

static void sha512_update(FfxSha512Context *ctx, const uint8_t *message,
                   size_t len) {
    ffx_hash_updateSha512(ctx, message, len);
}
static void sha512_final(FfxSha512Context *ctx, uint8_t *digest) {
    ffx_hash_finalSha512(ctx, digest);
}

static void sha512(const uint8_t *message, size_t len,
            uint8_t *digest) {
    ffx_hash_sha512(digest, message, len);

}

static void hmac_sha512_init(FfxHmacSha512Context *ctx, const uint8_t *key,
                      size_t key_size)
{
    size_t fill;
    size_t num;

    const uint8_t *key_used;
    uint8_t key_temp[SHA512_DIGEST_SIZE];
    int i;

    if (key_size == SHA512_BLOCK_SIZE) {
        key_used = key;
        num = SHA512_BLOCK_SIZE;
    } else {
        if (key_size > SHA512_BLOCK_SIZE){
            num = SHA512_DIGEST_SIZE;
            sha512(key, key_size, key_temp);
            key_used = key_temp;
        } else { /* key_size > SHA512_BLOCK_SIZE */
            key_used = key;
            num = key_size;
        }
        fill = SHA512_BLOCK_SIZE - num;

        memset(ctx->block_ipad + num, 0x36, fill);
        memset(ctx->block_opad + num, 0x5c, fill);
    }

    for (i = 0; i < (int) num; i++) {
        ctx->block_ipad[i] = key_used[i] ^ 0x36;
        ctx->block_opad[i] = key_used[i] ^ 0x5c;
    }

    sha512_init(&ctx->ctx_inside);
    sha512_update(&ctx->ctx_inside, ctx->block_ipad, SHA512_BLOCK_SIZE);

    sha512_init(&ctx->ctx_outside);
    sha512_update(&ctx->ctx_outside, ctx->block_opad,
                  SHA512_BLOCK_SIZE);
}

static void hmac_sha512_update(FfxHmacSha512Context *ctx, const uint8_t *message,
                        size_t message_len)
{
    sha512_update(&ctx->ctx_inside, message, message_len);
}

static void hmac_sha512_final(FfxHmacSha512Context *ctx, uint8_t *mac,
                       size_t mac_size)
{
    uint8_t digest_inside[SHA512_DIGEST_SIZE];
    uint8_t mac_temp[SHA512_DIGEST_SIZE];

    sha512_final(&ctx->ctx_inside, digest_inside);
    sha512_update(&ctx->ctx_outside, digest_inside, SHA512_DIGEST_SIZE);
    sha512_final(&ctx->ctx_outside, mac_temp);
    memcpy(mac, mac_temp, mac_size);
}

static void hmac_sha512(const uint8_t *key, size_t key_size,
          const uint8_t *message, size_t message_len,
          uint8_t *mac, unsigned mac_size)
{
    FfxHmacSha512Context ctx;

    hmac_sha512_init(&ctx, key, key_size);
    hmac_sha512_update(&ctx, message, message_len);
    hmac_sha512_final(&ctx, mac, mac_size);
}

void ffx_hmac_initSha512(FfxHmacSha512Context *context,
  const uint8_t *key, size_t length) {

    hmac_sha512_init(context, key, length);
}

void ffx_hmac_updateSha512(FfxHmacSha512Context *context,
  const uint8_t *data, size_t length) {
    hmac_sha512_update(context, data, length);
}

void ffx_hmac_finalSha512(FfxHmacSha512Context *context, uint8_t *hmac) {
    hmac_sha512_final(context, hmac, SHA512_DIGEST_SIZE);
}

void ffx_hmac_sha512(uint8_t *digest, uint8_t *key, size_t keyLen,
  uint8_t *data, size_t dataLen) {

    hmac_sha512(key, keyLen, data, dataLen, digest, SHA512_DIGEST_SIZE);
}
