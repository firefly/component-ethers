#ifndef __FIREFLY_DECIMAL_H__
#define __FIREFLY_DECIMAL_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "firefly-bigint.h"


///////////////////////////////
// Unit conversion


// The maximum length of a 256-bit value, including:
//   - 1 byte, for any sign (@TODO)
//   - 78 bytes, for numeric content
//   - 1 byte, for any decimal point (decimals == 0 has no dot)
//   - 26 bytes for grouping comma
//   - 1 bytes NULL termination
#define FFX_ETHER_STRING_LENGTH       (1 + 78 + 1 + 26 + 1)


typedef enum FfxDecimalRound {
    FfxDecimalRoundTruncate = 0,
    FfxDecimalRoundUp,
    FfxDecimalRoundDown,
    FfxDecimalRoundFloor,
    FfxDecimalRoundCeiling,
} FfxDecimalRound;

typedef struct FfxDecimalFormat {
    // The number of decimal places to interpret the value with
    uint8_t decimals;

    // The minimum number of digits to preserse right of the decimal
    uint8_t minDecimals;

    // The maximum number of digits to preserse right of the decimal; if
    // arithmetic underflow occurs (precission loss), flags will be set
    uint8_t maxDecimals;

    // The number of items per group; default: 0 (no grouping)
    uint8_t groups;

    // Rounding strategy for truncation; default: truncate
    FfxDecimalRound round;

    // Character to use for the decimal point; default: '.'
    char decimalChr;

    // Character to use for group delimiting; default: ','
    char groupChr;

} FfxDecimalFormat;

typedef enum FfxDecimalFlags {
  FfxDecimalFlagNone = 0,
  FfxDecimalFlagRounded = (1 << 0),
  FfxDecimalFlagOverflow = (1 << 1),
} FfxDecimalFlag;

typedef struct FfxDecimalResult {
    // The output string
    const char *str;

    // Length of the string
    size_t length;

    // Number of actual decimals included
    size_t decimals;

    // The offset where the decimal point occurs
    size_t decimalOffset;

    // Any flags that occurred
    FfxDecimalFlag flags;

} FfxDecimalResult;


FfxDecimalResult ffx_decimal_formatValue(char *output, const FfxBigInt *value,
  FfxDecimalFormat format);

FfxBigInt ffx_decimal_parseValue(char *text, uint8_t decimals);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FIREFLY_DECIMAL_H__ */
