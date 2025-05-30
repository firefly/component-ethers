#ifndef __FIREFLY_CBOR_H__
#define __FIREFLY_CBOR_H__


#include "firefly-data.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 *  A tag can be used to modify the length of an array or map when
 *  its length is unknown (or may change) after it has been appended.
 */
typedef size_t FfxCborTag;

/**
 *  The supported types for CBOR data.
 */
typedef enum FfxCborType {
    FfxCborTypeError    = 0,
    FfxCborTypeNull     = (1 << 0),
    FfxCborTypeBoolean  = (1 << 1),
    FfxCborTypeNumber   = (1 << 2),
    FfxCborTypeString   = (1 << 3),
    FfxCborTypeData     = (1 << 4),
    FfxCborTypeArray    = (1 << 5),
    FfxCborTypeMap      = (1 << 6)
} FfxCborType;

/**
 *  A cursor used to traverse and read CBOR-encoded data.
 *
 *  This should not be modified directly! Only use the provided API.
 */
typedef struct FfxCborCursor {
    const uint8_t *data;
    size_t offset, length;

    FfxDataError error;
} FfxCborCursor;

typedef struct FfxCborIterator {
    // The current child; updated on each call to nextValue
    FfxCborCursor child;

    // For Arrays, key.status = FfxDataErrorNotFound, for maps this
    // is a cursor pointing to the key
    FfxCborCursor key;

    FfxDataError error;

    FfxCborCursor container;

    size_t _containerCount;
    size_t _containerIndex;
} FfxCborIterator;


/**
 *  A builder used to create and write CBOR-encoded data.
 *
 *  This should not be modified directly! Only use the provided API.
 */
typedef struct FfxCborBuilder {
    uint8_t *data;
    size_t offset, length;

    FfxDataError error;
} FfxCborBuilder;


FfxCborCursor ffx_cbor_walk(const uint8_t *data, size_t length);

/**
 *  Validates:
 *    - All Maps have String keys
 *    - All Maps and Arrays are complete
 *    - All container lengths are a safe length
 *    - The entire contents is consumed?
 */
FfxDataError ffx_cbor_validate(FfxCborCursor cursor);

/**
 *  Returns the data type of the %%cursor%%.
 */
FfxCborType ffx_cbor_getType(FfxCborCursor cursor);

/**
 *  Returns true if the cursor type matches any of %%types%%.
 */
bool ffx_cbor_checkType(FfxCborCursor cursor, FfxCborType types);

/**
 *  Returns the value for scalar types (Null, Boolean, Number).
 *
 *  If .error is non-zero, .value = 0.
 *
 *  The values:
 *    - Null = 0
 *    - Boolean: false = 0, true = 1
 *    - Numbers: the value (negative values are currently unsupported)
 */
FfxValueResult ffx_cbor_getValue(FfxCborCursor cursor);

/**
 *  Returns the data for data types (Data and Strings).
 *
 *  If .error is non-zero, the .data = NULL and the .length = 0.
 *
 *  This points to the underlying CBOR data and MUST not be modified.
 */
FfxDataResult ffx_cbor_getData(FfxCborCursor cursor);

/**
 *  Returns the number of items for container types (Array and Map) and
 *  returns the length for data types (Data and Strings).
 *
 *  If .error is non-zero, the .value = 0.
 */
FfxSizeResult ffx_cbor_getDataLength(FfxCborCursor cursor);

FfxSizeResult ffx_cbor_getContainerCount(FfxCborCursor cursor);

/**
 *  Returns true if type matches one of %%types%% and the %%length%% matches.
 */
bool ffx_cbor_checkLength(FfxCborCursor cursor, FfxCborType types,
  size_t length);

/**
 *  Returns a cursor pointing to the value for %%key%%.
 *
 *  If .error == FfxDataErrorNotFound, the %%key%% does not exist in the Map.
 */
FfxCborCursor ffx_cbor_followKey(FfxCborCursor cursor, const char *key);

/**
 *  Returns a cursor pointing to the %%index%% item.
 *
 *  If .error = FfxDataErrorNotFound, the %%index%% is out of range of
 *  the Array.
 */
FfxCborCursor ffx_cbor_followIndex(FfxCborCursor cursor, size_t index);

/**
 *  Iterates over a container type (Array or Map).
 *
 *  example:
 *    FfxCborIterator iter = ffx_cbor_iterate(cursor);
 *    while(ffx_cbor_nextChild(&iter)) {
 *        FfxCborCursor cursor = iter.child;
 *        // ...
 *    }
 *
 */
FfxCborIterator ffx_cbor_iterate(FfxCborCursor container);

/**
 *  Advances the cursor to the next item in the container.
 *  See [ffx_cbor_iterate]] for usage.
 */
bool ffx_cbor_nextChild(FfxCborIterator *iterator);

//bool ffx_cbor_hasNext(FfxCborIterator *iterator);

/**
 *  Dumps the structured CBOR data to the console via printf.
 */
void ffx_cbor_dump(FfxCborCursor cursor);


/**
 *  Initialize a CBOR builder.
 */
FfxCborBuilder ffx_cbor_build(uint8_t *data, size_t length);

/**
 *  The current length of the CBOR output.
 */
size_t ffx_cbor_getBuildLength(FfxCborBuilder *cbor);

/**
 *  Append a boolean.
 */
bool ffx_cbor_appendBoolean(FfxCborBuilder *cbor, bool value);

/**
 *  Append a null value.
 */
bool ffx_cbor_appendNull(FfxCborBuilder *cbor);

/**
 *  Append a numeric value.
 */
bool ffx_cbor_appendNumber(FfxCborBuilder *cbor, uint64_t value);

/**
 *  Append data.
 */
bool ffx_cbor_appendData(FfxCborBuilder *cbor, const uint8_t *data,
  size_t length);

/**
 *  Append a string.
 */
bool ffx_cbor_appendString(FfxCborBuilder *cbor, const char* str);

bool ffx_cbor_appendStringData(FfxCborBuilder *cbor, const uint8_t* data,
  size_t length);

/**
 *  Begin an Array of %%count%% items long.
 *
 *  After calling this, the next %%count%% ffx_cbor_append* calls will
 *  place the item into this array.
 */
bool ffx_cbor_appendArray(FfxCborBuilder *cbor, size_t count);

/**
 *  Begin a Map of %%count%% items long.
 *
 *  After calling this, the next %%count%% * 2 append calls will
 *  place the item into this mapping.
 *
 *  Each item should be use [[ffx_cbor_appendString]] for the key
 *  followed by any ffx_cbor_append*.
 */
bool ffx_cbor_appendMap(FfxCborBuilder *cbor, size_t count);

/**
 *  Appends a Dynamic Array.
 *
 *  Similar to [[ffx_fbor_appendArray]], but will reserve additional
 *  space for the count (initially 0). The [[ffx_cbor_adjustCount]]
 *  must be used later to update the count to the correct value using
 *  the value stored in %%tag%%.
 */
FfxCborTag ffx_cbor_appendArrayMutable(FfxCborBuilder *cbor);

/**
 *  Appends a Dynamic Map.
 *
 *  Similar to [[ffx_fbor_appendMap]], but will reserve additional
 *  space for the count (initially 0). The [[ffx_cbor_adjustCount]]
 *  must be used later to update the count to the correct value using
 *  the value stored in %%tag%%.
 */
FfxCborTag ffx_cbor_appendMapMutable(FfxCborBuilder *cbor);

/**
 *  Update the count of a Dynamic Array or Dynamic Map
 */
void ffx_cbor_adjustCount(FfxCborBuilder *cbor, FfxCborTag tag, size_t count);

/**
 *  Append raw CBOR-encoded %%data%% to an entry in %%builder%%.
 */
bool ffx_cbor_appendCborRaw(FfxCborBuilder *cbor, uint8_t *data,
  size_t length);

/**
 *  Append an entire CborBuilder %%src%% to an entry in %%dst%%.
 */
bool ffx_cbor_appendCborBuilder(FfxCborBuilder *dst, FfxCborBuilder *src);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FIREFLY_CBOR_H__ */
