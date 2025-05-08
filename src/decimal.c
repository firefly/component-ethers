#include <string.h>

#include "firefly-decimal.h"

#include <stdio.h>

FfxDecimalResult ffx_decimal_formatValue(char *out, const FfxBigInt *value,
  FfxDecimalFormat fmt) {

    // @TODO: negative values

    if (ffx_bigint_bitcount(value) > 256 || ffx_bigint_isNegative(value)) {
        return (FfxDecimalResult){ .flags = FfxDecimalFlagOverflow };
    }

    // Normalize and set format defaults
    if (fmt.decimalChr == 0) { fmt.decimalChr = '.'; }
    if (fmt.groupChr == 0) { fmt.groupChr = ','; }
    if (fmt.groups && groups < 3) { fmt.groups = 3; }
    if (fmt.maxDecimals > fmt.decimals) { fmt.maxDecimals = fmt.decimals; }
    if (fmt.minDecimals > fmt.decimals) { fmt.minDecimals = fmt.decimals; }
    if (fmt.maxDecimals < fmt.minDecimals) {
        fmt.maxDecimals = fmt.minDecimals;
    }

    FfxDecimalResult result = { .str = out, .decimals = fmt.decimals };

    // Round, zero-pad and trim 256-bit value into out
    {
        FfxBigInt rounded = *value;
        int truncate = fmt.decimals - fmt.maxDecimals;
        uint32_t m = 0;
        while (truncate > 0) {
            m = ffx_bigint_divmodU32(&rounded, &rounded, 10);
            if (m) { result.flags |= FfxDecimalFlagRounded; }
            result.decimals--;
            truncate--;
        }

        // Need to apply rounding strategy
        if (result.flags & FfxDecimalFlagRounded) {
            switch (fmt.round) {
                case FfxDecimalRoundTruncate:
                case FfxDecimalRoundFloor:
                    break;
                case FfxDecimalRoundUp:
                    if (m < 5) { break; }
                    ffx_bigint_addU32(&rounded, &rounded, 1);
                    break;
                case FfxDecimalRoundDown:
                    if (m <= 5) { break; }
                    ffx_bigint_addU32(&rounded, &rounded, 1);
                    break;
                case FfxDecimalRoundCeiling:
                    ffx_bigint_addU32(&rounded, &rounded, 1);
                    break;
            }
        } else {
            // No rounding, trim trailing-zeros 
            FfxBigInt check = { 0 };
            while (result.decimals > fmt.minDecimals) {
                m = ffx_bigint_divmodU32(&check, &rounded, 10);
                if (m) { break; }
                result.decimals--;
                rounded = check;
            }
        }

        // Get the decimal string
        char str[FFX_BIGINT_STRING_LENGTH];
        size_t length = ffx_bigint_getString(&rounded, str);

        // Left pad with zeros
        memset(out, '0', FFX_ETHER_STRING_LENGTH);

        // Right-align the value, including the NULL-termination
        size_t dst = FFX_ETHER_STRING_LENGTH - length - 1;
        memcpy(&out[dst], str, length + 1);
    }

    // "00000000123456789\0"

    result.length = FFX_ETHER_STRING_LENGTH - 1;

    // Insert a decimal poiont (shift the string to the left, clobbering v[0])
    result.decimalOffset = FFX_ETHER_STRING_LENGTH - result.decimals - 2;
    memmove(out, &out[1], result.decimalOffset);
    out[result.decimalOffset] = fmt.decimalChr;

    // "0000000012.3456789\0"

    // Trim the leading zeros (leaving at least 1)
    {
        size_t start = 0;
        while (out[start] == '0' && out[start + 1] != fmt.decimalChr) { start++; }
        if (start) {
            memmove(out, &out[start], FFX_ETHER_STRING_LENGTH - start);
            //end -= start;
            result.decimalOffset -= start;
            result.length -= start;
        }
    }

    // Insert commas to group the whole component
    if (fmt.groups) {
        int dec = result.decimalOffset;
        while (dec > fmt.groups) {
            dec -= fmt.groups;
            memmove(&out[dec + 1], &out[dec], FFX_ETHER_STRING_LENGTH - dec - 1);
            out[dec] = ',';
            result.length++;
            result.decimalOffset++;
        }
    }

    // Trim the trailing decimal point when decimals is 0
    if (fmt.decimals == 0) { out[result.decimalOffset] = '\0'; }

    return result;
}
