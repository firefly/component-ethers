#ifndef __FIREFLY_BIGINT_H__
#define __FIREFLY_BIGINT_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// String length to hold the output of getString
// - sign: 1 char
// - decimal digits: 85 chars
// - NULL-terminator: 1 char
#define FFX_BIGINT_STRING_LENGTH       (1 + 85 + 1)

typedef enum FfxBigIntFlags {
    FfxBigIntFlagsNone             = 0,
    FfxBigIntFlagsCarry            = (1 << 0),
    FfxBigIntFlagsOverflow         = (1 << 1),
    FfxBigIntFlagsDivideByZero     = (1 << 2),
    FfxBigIntFlagsError            = (1 << 7),
} FfxBigIntFlags;

#define wordCount    (10)

typedef struct FfxBigInt {
    // Room to store 257 signed value in two's compliemnt
    uint32_t value[10];
    //uint8_t base;
    FfxBigIntFlags flags;
} FfxBigInt;


FfxBigInt ffx_bigint_initString(const char* str);
FfxBigInt ffx_bigint_initBytes(const uint8_t *value, size_t length);
FfxBigInt ffx_bigint_initU32(uint32_t value);

void ffx_bigint_clear(FfxBigInt *out);

void ffx_bigint_add(FfxBigInt *out, const FfxBigInt *a, const FfxBigInt *b);
void ffx_bigint_sub(FfxBigInt *out, const FfxBigInt *a, const FfxBigInt *b);
//void ffx_bigint_mul(FfxBigInt *out, const FfxBigInt *a, const FfxBigInt *b);
//void ffx_bigint_div(FfxBigInt *out, const FfxBigInt *a, const FfxBigInt *b);
//void ffx_bigint_mod(FfxBigInt *out, const FfxBigInt *a, const FfxBigInt *b);
//void ffx_bigint_divmod(FfxBigInt *outDiv, FfxBigInt *outMod,
//  const FfxBigInt *a, const FfxBigInt *b);

void ffx_bigint_addU32(FfxBigInt *out, const FfxBigInt *a, uint32_t b);
//void ffx_bigint_subU32(FfxBigInt *out, const FfxBigInt *a, uint32_t b);
void ffx_bigint_mulU32(FfxBigInt *out, const FfxBigInt *a, uint32_t b);
void ffx_bigint_divU32(FfxBigInt *out, const FfxBigInt *a, uint32_t b);
uint32_t ffx_bigint_modU32(const FfxBigInt *a, uint32_t b);
uint32_t ffx_bigint_divmodU32(FfxBigInt *outDiv, const FfxBigInt *a,
  uint32_t b);

void ffx_bigint_negate(FfxBigInt *out, const FfxBigInt *a);

void ffx_bigint_and(FfxBigInt *out, const FfxBigInt *a, const FfxBigInt *b);
void ffx_bigint_or(FfxBigInt *out, const FfxBigInt *a, const FfxBigInt *b);
void ffx_bigint_xor(FfxBigInt *out, const FfxBigInt *a, const FfxBigInt *b);
void ffx_bigint_not(FfxBigInt *out, const FfxBigInt *a);

//void ffx_bigint_shl(FfxBigInt *out, const FfxBigInt *a, uint16_t bits);
//void ffx_bigint_shr(FfxBigInt *out, const FfxBigInt *a, uint16_t bits);
//void ffx_bigint_sar(FfxBigInt *out, const FfxBigInt *a, uint16_t bits);

void ffx_bigint_setBit(FfxBigInt *out, const FfxBigInt *a, uint32_t bit);


int ffx_bigint_cmp(const FfxBigInt *a, const FfxBigInt *b);
int ffx_bigint_cmpU32(const FfxBigInt *a, uint32_t b);

bool ffx_bigint_eq(const FfxBigInt *a, const FfxBigInt *b);
bool ffx_bigint_lt(const FfxBigInt *a, const FfxBigInt *b);
bool ffx_bigint_lte(const FfxBigInt *a, const FfxBigInt *b);
bool ffx_bigint_gt(const FfxBigInt *a, const FfxBigInt *b);
bool ffx_bigint_gte(const FfxBigInt *a, const FfxBigInt *b);

bool ffx_bigint_isNegative(const FfxBigInt *a);
bool ffx_bigint_isZero(const FfxBigInt *a);

bool ffx_bigint_testBit(const FfxBigInt *a, uint32_t bit);
uint16_t ffx_bigint_bitcount(const FfxBigInt *a);

void ffx_bigint_dump(FfxBigInt *value);

size_t ffx_bigint_getString(const FfxBigInt *a, char *out);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FIREFLY_BIGINT_H__ */
