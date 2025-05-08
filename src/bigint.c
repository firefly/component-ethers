#include <stdio.h>
#include <string.h>


#include "firefly-bigint.h"

// Allow 280-bit numbers; 28 usable bits per word in mp.
#define numWords      (10)

///////////////////////////////
// Public API

#define INT(v)          ((int)(v))
#define UINT64(v)       ((uint64_t)(v))

static const uint32_t MASK = 0x0fffffff;
//static const BITS = 28;

int getDecimal(char c) {
    if (c < '0' || c > '9') { return -1; }
    return (c - '0');
}

FfxBigInt ffx_bigint_initString(const char* str) {
    FfxBigInt result = { 0 };

    size_t len = strlen(str);

    FfxBigIntFlags flags = FfxBigIntFlagsNone;

    for (int i = 0; i < len; i += 9) {
        uint32_t mult = 1;

        // Read decimal values 9 at a time (1,000,000,000 is the largest
        // power-of-ten value that fits in a uint32_t)
        uint32_t word = 0;
        for (int j = 0; j < 9; j++) {
            if (i + j >= len) { break; }

            int c = getDecimal(str[i + j]);
            if (c < 0) {
                result.flags |= FfxBigIntFlagsError;
                break;
            }
            word = (word * 10) + c;
            mult *= 10;
        }

        ffx_bigint_mulU32(&result, &result, mult);
        flags |= result.flags;

        ffx_bigint_addU32(&result, &result, word);
        flags |= result.flags;
    }

    result.flags = flags;

    return result;
}

FfxBigInt ffx_bigint_initBytes(const uint8_t *value, size_t length) {
    FfxBigInt result = { 0 };

    int offset = numWords - 1;

    size_t bit = 0;
    for (int i = length - 1; i >= 0; i--) {
        result.value[offset] |= (value[i] << bit) & MASK;
        bit += 8;

        if (bit >= 28) {
            offset--;
            if (offset < 0) {
                result.flags |= FfxBigIntFlagsOverflow;
                break;
            }
            bit -= 28;
            result.value[offset] |= value[i] >> (8 - bit);
        }
    }

    return result;
}

FfxBigInt ffx_bigint_initNumber(uint32_t value) {
    FfxBigInt result = { 0 };
    result.value[numWords - 1] = value & MASK;
    result.value[numWords - 2] = value >> 28;
    return result;
}

void ffx_bigint_clear(FfxBigInt *out) {
    out->flags = FfxBigIntFlagsNone;
    for (int i = 0; i < numWords; i++) { out->value[i] = 0; }
}

void ffx_bigint_add(FfxBigInt *out, const FfxBigInt *a, const FfxBigInt *b) {
    out->flags = FfxBigIntFlagsNone;

    bool negA = ffx_bigint_isNegative(a);
    bool negB = (b ? ffx_bigint_isNegative(b): false);

    uint32_t carry = 0;
    for (int i = numWords; i >= 0; i--) {
        uint32_t sum = a->value[i] + (b ? b->value[i]: 0) + carry;
        out->value[i] = sum & MASK;
        carry = (sum >> 28);
    }

    if (carry) { out->flags |= FfxBigIntFlagsCarry; }

    // A and B are the same sign and the result differs in sign
    if ((negA ^ negB) == 0 && ffx_bigint_isNegative(out) != negA) {
        out->flags |= FfxBigIntFlagsOverflow;
    }
}

void ffx_bigint_addU32(FfxBigInt *out, const FfxBigInt *a, uint32_t b) {
    out->flags = FfxBigIntFlagsNone;

    bool negA = ffx_bigint_isNegative(a);

    // Add the top 4 bits of b first
    uint32_t carry1 = b >> 28;
    if (carry1) {
        out->value[numWords - 1] = a->value[numWords - 1];
        for (int i = numWords - 2; i >= 0; i--) {
            uint32_t sum = a->value[i] + carry1;
            out->value[i] = sum & MASK;
            carry1 = (sum >> 28);
            // @TODO: Should be able to bail early if no carry
        }
    }

    // Add the bottom 28 bits
    uint32_t carry0 = b & MASK;
    for (int i = numWords - 1; i >= 0; i--) {
        uint32_t sum = out->value[i] + carry0;
        out->value[i] = sum & MASK;
        carry0 = (sum >> 28);
        // @TODO: Should be able to bail early if no carry
    }

    if (carry0 || carry1) { out->flags |= FfxBigIntFlagsCarry; }

    // Adding carry flipped the sign.
    if (!negA && ffx_bigint_isNegative(out)) {
        out->flags |= FfxBigIntFlagsOverflow;
    }
}

void ffx_bigint_sub(FfxBigInt *out, const FfxBigInt *a, const FfxBigInt *b) {
    *out = *b;
    ffx_bigint_negate(out, out);
    ffx_bigint_add(out, a, out);
}

void ffx_bigint_subU32(FfxBigInt *out, const FfxBigInt *a, uint32_t b);

void ffx_bigint_mul(FfxBigInt *out, const FfxBigInt *a, const FfxBigInt *b);

void ffx_bigint_mulU32(FfxBigInt *out, const FfxBigInt *a, uint32_t b) {

    // a * 0 or 0 * b => 0
    if (b == 0 || ffx_bigint_isZero(a)) {
        ffx_bigint_clear(out);
        return;
    }

    // a * 1 => a
    if (b == 1) {
        *out = *a;
        return;
    }

    bool negOut = ffx_bigint_isNegative(out);
    if (negOut) {
        ffx_bigint_negate(out, a);
    } else {
        *out = *a;
    }

    FfxBigInt result = { 0 };
    uint64_t r = 0;

    for (int i = numWords - 1; i >= 0; i--) {
        r += (UINT64(a->value[i]) * UINT64(b));
        result.value[i] = r & MASK;
        r >>= UINT64(28);
    }

    if (r) { result.flags |= FfxBigIntFlagsOverflow; }

    if (negOut) { ffx_bigint_negate(out, out); }

    *out = result;
}

//void ffx_bigint_div(FfxBigInt *out, const FfxBigInt *a, const FfxBigInt *b) {
//    ffx_bigint_divmod(out, NULL, a, b);
//}

void ffx_bigint_divU32(FfxBigInt *out, const FfxBigInt *a, uint32_t b) {
    ffx_bigint_divmodU32(out, a, b);
}

//void ffx_bigint_mod(FfxBigInt *out, const FfxBigInt *a, const FfxBigInt *b) {
//    ffx_bigint_divmod(NULL, out, a, b);
//}

uint32_t ffx_bigint_modU32(const FfxBigInt *a, uint32_t b) {
    return ffx_bigint_divmodU32(NULL, a, b);
}
/*
void ffx_bigint_divmod(FfxBigInt *outDiv, FfxBigInt *outMod,
  const FfxBigInt *a, const FfxBigInt *b) {

    // Division by zero; Error!
    if (ffx_bigint_isZero(b)) {
        ffx_bigint_clear(outDiv);
        outDiv->flags = FfxBigIntFlagsDivideByZero;
        return;
    }

    int cmp = ffx_bigint_cmp(a, b);

    // Dividing by a bigger number; the result is 0
    if (cmp < 0) {
        ffx_bigint_clear(outDiv);
        return;
    }

    // Dividing a value by itself; the result is 1
    if (cmp == 0) {
        ffx_bigint_clear(outDiv);
        outDiv->value[numWords - 1] = 1;
        return;
    }
}
*/
uint32_t ffx_bigint_divmodU32(FfxBigInt *outDiv, const FfxBigInt *a,
  uint32_t b) {

    // a / 0 => Error!
    if (b == 0) {
        if (outDiv) {
            ffx_bigint_clear(outDiv);
            outDiv->flags = FfxBigIntFlagsDivideByZero;
        }
        return 0;
    }

    // a / 1 => a:0
    if (b == 1) {
        if (outDiv) { *outDiv = *a; }
        return 0;
    }

    // 0 / b => zero:0
    if (ffx_bigint_isZero(a)) {
        if (outDiv) { ffx_bigint_clear(outDiv); }
        return 0;
    }

    // a / (b = 2 ** k) => (a >> k):0 @TODO
    //if ((b & (b - 1)) == 0) {
    //}

    int cmp = ffx_bigint_cmpU32(a, b);

    // a_small / b_huge => 0:a
    if (cmp < 0) {
        uint32_t mod = (a->value[numWords - 2] << 28) | a->value[numWords - 1];
        if (outDiv) { ffx_bigint_clear(outDiv); }
        return mod;
    }

    // a / a => 1:0
    if (cmp == 0) {
        if (outDiv) {
            ffx_bigint_clear(outDiv);
            outDiv->value[numWords - 1] = 1;
        }
        return 0;
    }

    // Normalize to positive value
    FfxBigInt q = *a;
    bool negA = ffx_bigint_isNegative(&q);
    if (negA) { ffx_bigint_negate(&q, &q); }

    // Divide

    uint64_t w = 0;
    for (int i = 0; i < numWords; i++) {
        uint32_t t = 0;
        w = (w << 28) | a->value[i];
        if (w >= b) {
            t = (w / b);
            w -= UINT64(t) * UINT64(b);
        }
        q.value[i] = t;
    }

    // Fix the sign
    if (negA) { ffx_bigint_negate(&q, &q); }

    if (outDiv) { *outDiv = q; }

    // Clean up memory
    ffx_bigint_clear(&q);

    return w;
}

void ffx_bigint_negate(FfxBigInt *out, const FfxBigInt *a) {
    ffx_bigint_not(out, a);
    ffx_bigint_addU32(out, out, 1);
}

void ffx_bigint_and(FfxBigInt *out, const FfxBigInt *a, const FfxBigInt *b) {
    for (int i = 0; i < numWords; i++) {
        out->value[i] = a->value[i] & b->value[i];
    }
}

void ffx_bigint_or(FfxBigInt *out, const FfxBigInt *a, const FfxBigInt *b) {
    for (int i = 0; i < numWords; i++) {
        out->value[i] = a->value[i] | b->value[i];
    }
}

void ffx_bigint_xor(FfxBigInt *out, const FfxBigInt *a, const FfxBigInt *b) {
    for (int i = 0; i < numWords; i++) {
        out->value[i] = a->value[i] ^ b->value[i];
    }
}

void ffx_bigint_not(FfxBigInt *out, const FfxBigInt *a) {
    out->value[0] = a->value[0] ^ 1;
    for (int i = 1; i < numWords; i++) {
        out->value[i] = (~a->value[i]) & MASK;
    }
}

void ffx_bigint_shl(FfxBigInt *out, const FfxBigInt *a, uint16_t bits);
/* @TODO
{
    int s = bits / 28;
    if (s) {
        for (int i = 0; i < numWords; i++) {
            out->value[i] = ((i + s) < numWords) ? out->value[i + s]: 0;
        }
    }

    s = bits % 28;
    if (s) {
        uint32_t carry = 0;
        for (int i = numWords - 1; i >= 0; i--) {
            out->value[i] = (out->value[i] << s) | carry;
            carry = out->value[i] << s;
        }
    }
}
*/
void ffx_bigint_shr(FfxBigInt *out, const FfxBigInt *a, uint16_t bits);
void ffx_bigint_sar(FfxBigInt *out, const FfxBigInt *a, uint16_t bits);

void ffx_bigint_setBit(FfxBigInt *out, const FfxBigInt *a, uint32_t bit) {
    if (bit > 279) { return; }
    bit = 279 - bit;
    *out = *a;
    out->value[bit / 28] |= ((1 << 27) >> (bit % 28));
}

int ffx_bigint_cmp(const FfxBigInt *a, const FfxBigInt *b) {

    bool negA = ffx_bigint_isNegative(a);
    bool negB = ffx_bigint_isNegative(b);

    // Signs differ; the positive value is bigger
    if (negA != negB) {
        if (negA) { return -1; }
        return 1;
    }

    // Both are negative; compare backwards
    if (negA) {
        const FfxBigInt *tmp = a;
        a = b;
        b = tmp;
    }

    for (int i = 0; i < numWords; i++) {
        int delta = INT(a->value[i]) - INT(b->value[i]);
        if (delta > 0) { return 1; }
        if (delta < 0) { return -1; }
    }

    return 0;
}


int ffx_bigint_cmpU32(const FfxBigInt *a, uint32_t b) {
    if (ffx_bigint_isNegative(a)) { return -1; }
    for (int i = 0; i < numWords - 2; i++) {
        if (a->value[i]) { return 1; }
    }

    int delta = INT(a->value[numWords - 2]) - INT(b >> 28);
    if (delta > 0) { return 1; }
    if (delta < 0) { return -1; }

    delta = INT(a->value[numWords - 1]) - INT(b & MASK);
    if (delta > 0) { return 1; }
    if (delta < 0) { return -1; }

    return 0;
}

bool ffx_bigint_eq(const FfxBigInt *a, const FfxBigInt *b) {
    return ffx_bigint_cmp(a, b) == 0;
}

bool ffx_bigint_lt(const FfxBigInt *a, const FfxBigInt *b) {
    return ffx_bigint_cmp(a, b) < 0;
}

bool ffx_bigint_lte(const FfxBigInt *a, const FfxBigInt *b) {
    return ffx_bigint_cmp(a, b) <= 0;
}

bool ffx_bigint_gt(const FfxBigInt *a, const FfxBigInt *b) {
    return ffx_bigint_cmp(a, b) > 0;
}

bool ffx_bigint_gte(const FfxBigInt *a, const FfxBigInt *b) {
    return ffx_bigint_cmp(a, b) >= 0;
}

bool ffx_bigint_isNegative(const FfxBigInt *a) {
    return ffx_bigint_testBit(a, 279);
}

bool ffx_bigint_isZero(const FfxBigInt *a) {
    for (int i = 0; i < wordCount; i++) {
        if (a->value[i]) { return false; }
    }
    return true;
}

bool ffx_bigint_testBit(const FfxBigInt *a, uint32_t bit) {
    if (bit > 279) { return false; }
    bit = 279 - bit;
    return (a->value[bit / 28] & (0x8000000 >> (bit % 28))) != 0;
}

uint16_t ffx_bigint_bitcount(const FfxBigInt *a) {
    uint16_t count = 280;
    for (int i = 0; i < numWords; i++) {
        uint32_t v = a->value[i];
        if (v == 0) {
            count -= 28;
        } else {
            uint32_t mask = 0xf8000000;
            while ((mask & v) == 0) {
                count--;
                mask >>= 1;
            }
        }
    }
    return count;
}

// @TODO: Sign
size_t ffx_bigint_getString(const FfxBigInt *a, char *out) {
    memset(out, 0, FFX_BIGINT_STRING_LENGTH);

    FfxBigInt v = *a;

    int start = FFX_BIGINT_STRING_LENGTH - 2;
    int offset = start;

    out[start] = '0';

    while (offset >= 0 && !ffx_bigint_isZero(&v)) {
        uint32_t c = ffx_bigint_divmodU32(&v, &v, 1000000000);

        for (int j = 0; j < 9; j++) {
            uint32_t d = c % 10;
            if (d) { start = offset; }

            out[offset--] = d + '0';
            if (offset < 0) { break; }

            c /= 10;
        }
    }

    memmove(out, &out[start], FFX_BIGINT_STRING_LENGTH - start);

    ffx_bigint_clear(&v);

    return FFX_BIGINT_STRING_LENGTH - start - 1;
}

void ffx_bigint_dump(FfxBigInt *value) {
    printf("<BigInt hex=0x");
    bool nonzero = false;
    for (int i = 0; i < wordCount; i++) {
        for (int j = 0; j < 7; j++) {
            uint32_t v = (value->value[i] >> (24 - j * 4)) & 0xf;
            if (v) { nonzero = true; }
            if (nonzero) { printf("%x", (int)v); }
        }
    }
    if (!nonzero) { printf("0"); }

    char dec[FFX_BIGINT_STRING_LENGTH];
    ffx_bigint_getString(value, dec);

    printf(" dec=%s>\n", dec);
}
