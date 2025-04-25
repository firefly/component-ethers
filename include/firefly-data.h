#ifndef __FIREFLY_DATA_H__
#define __FIREFLY_DATA_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


typedef enum FfxDataError {
    FfxDataErrorNone = 0,

    FfxDataErrorOverflow,
    FfxDataErrorBadData,

    FfxDataErrorBufferOverrun,

    FfxDataErrorInvalidOperation,
    FfxDataErrorUnsupportedFeature,

    FfxDataErrorNotFound,
} FfxDataError;


typedef struct FfxData {
    const uint8_t *bytes;
    size_t length;
} FfxData;

typedef struct FfxDataResult {
    const uint8_t *bytes;
    size_t length;
    FfxDataError error;
} FfxDataResult;

typedef struct FfxValueResult {
    uint64_t value;
    FfxDataError error;
} FfxValueResult;

typedef struct FfxSizeResult {
    size_t value;
    FfxDataError error;
} FfxSizeResult;



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FIREFLY_TRANSACTION_H__ */
