#ifndef __FIREFLY_RLP_H__
#define __FIREFLY_RLP_H__

#include "firefly-data.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


/**
 *  Recursive-Length Prefix (RLP) Encoder
 *
 *  RLP-encoding is used for various purposes in Ethereum, such as
 *  serializing transactions, which is in turn used to create the
 *  necessary hashes for signing.
 *
 *  See: https://ethereum.org/en/developers/docs/data-structures-and-encoding/rlp/
 */

/*
typedef enum FfxRlpStatus {
    FfxRlpStatusOK = 0,

    FfxRlpStatusBufferOverrun = -31,
    FfxRlpStatusOverflow = -55,

    FfxRlpStatusBeginIterator = 20,
} FfxRlpStatus;
*/

typedef enum FfxRlpType {
    FfxRlpTypeError   = 0,
    FfxRlpTypeData    = (1 << 5),
    FfxRlpTypeArray   = (1 << 6)
} FfxRlpType;

typedef struct FfxRlpCursor {
    const uint8_t *data;
    size_t offset, length;

    FfxDataError error;
} FfxRlpCursor;

typedef struct FfxRlpIterator {
    FfxRlpCursor child;

    size_t _nextOffset, _containerEnd;

    FfxDataError error;
} FfxRlpIterator;

typedef struct FfxRlpBuilder {
    uint8_t *data;
    size_t offset, length;

    FfxDataError error;
} FfxRlpBuilder;

typedef size_t FfxRlpBuilderTag;


///////////////////////////////
// Walking

FfxRlpCursor ffx_rlp_walk(const uint8_t *data, size_t length);

FfxRlpType ffx_rlp_getType(FfxRlpCursor cursor);

/**
 *  Returns the length in bytes of a Data or the number of items
 *  of an Array.
 */
FfxSizeResult ffx_rlp_getDataLength(FfxRlpCursor cursor);

FfxSizeResult ffx_rlp_getArrayCount(FfxRlpCursor cursor);

/**
 *  Returns the data of a Data.
 */
FfxDataResult ffx_rlp_getData(FfxRlpCursor cursor);

FfxRlpCursor ffx_rlp_followIndex(FfxRlpCursor cursor, size_t index);

/**
 *  Iterates over an Array.
 *
 *  example:
 *    FfxRlpIterator iter = ffx_rlp_iterate(cursor);
 *    while(ffx_cbor_nextChild(&iter)) {
 *        FfxRlpCursor cursor = iter.child;
 *        // ...
 *    }
 *
 */
FfxRlpIterator ffx_rlp_iterate(FfxRlpCursor container);

/**
 *  Advances the cursor to the next item in the Array.
 *  See [ffx_rlp_iterate]] for usage.
 */
bool ffx_rlp_nextChild(FfxRlpIterator *iterator);

void ffx_rlp_dump(FfxRlpCursor cursor);


///////////////////////////////
// Building

/**
 *  Initializes a new RLP builder.
 *
 *  During the build, intermediate data is included in the %%data%%
 *  buffer, so the [[rlp_finalize]] MUST be called to complete RLP
 *  serialization.
 */
FfxRlpBuilder ffx_rlp_build(uint8_t *data, size_t length);

/**
 *  Remove all intermediate data and return the length of the RLP-encoded
 *  data.
 */
size_t ffx_rlp_finalize(FfxRlpBuilder *builder);

/**
 *  Returns the length of finalized RLP data.
 *
 *  This is the same value returned from [[ffx_rlp_finalize]].
 */
size_t ffx_rlp_getBuildLength(FfxRlpBuilder *builder);

/**
 *  Append a Data.
 */
bool ffx_rlp_appendData(FfxRlpBuilder *builder, const uint8_t *data,
  size_t length);

/**
 *  Append a Data, with the contents of the a NULL-terminated string.
 */
bool ffx_rlp_appendString(FfxRlpBuilder *builder, const char *data);

/**
 *  Append an Array, where the next %%count%% Items added will be
 *  added to this array.
 */
bool ffx_rlp_appendArray(FfxRlpBuilder *builder, size_t count);

FfxRlpBuilderTag ffx_rlp_appendMutableArray(FfxRlpBuilder *builder);

bool ffx_rlp_adjustCount(FfxRlpBuilder *builder, FfxRlpBuilderTag tag,
  size_t count);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FIREFLY_RLP_H__ */
