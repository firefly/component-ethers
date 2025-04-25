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
    size_t length;
    size_t offset;

    FfxDataError error;
} FfxCborCursor;

typedef struct FfxCborIterator {
    // The current child; updated on each call to nextValue
    FfxCborCursor child;

    // For Arrays, key.status = FfxDataErrorNotFound
    FfxCborCursor key;

    // If an error occurred during iteration (which also halts iteration)
    //FfxDataError error; use the child Cursor's error

    FfxDataError error;

    FfxCborCursor container;

    size_t containerCount;
    size_t containerIndex;
} FfxCborIterator;


/**
 *  A builder used to create and write CBOR-encoded data.
 *
 *  This should not be modified directly! Only use the provided API.
 */
typedef struct FfxCborBuilder {
    uint8_t *data;
    size_t length;
    size_t offset;

    FfxDataError error;
} FfxCborBuilder;


// @TODO:
//FfxCborStatus ffx_cbor_verify(uint8_t *data, size_t *length);

FfxCborCursor ffx_cbor_walk(const uint8_t *data, size_t length);

FfxCborCursor ffx_cbor_clone(FfxCborCursor *cursor);

/**
 *  Validates:
 *    - All Maps have String keys
 *    - All Maps and Arrays are complete
 *    - All container lengths are a safe length
 *    - The entire contents is consumed?
 */
FfxDataError ffx_cbor_validate(FfxCborCursor *cursor);


/**
 *
 *  Components
 *    - [0-9]+ => array index
 *    - [a-z_][a-z0-9-_] => map key
 *    - $ => next character is literal and a map key
 *    - :(number|string|boolean|null|data|array|map) => type must match
 *    - (<|<=|=|=>|>)[0-9]+ => array constraint; length must match
 *
 *  Path Examples:
 *    - "foo/bar/:number" => { foo: { bar: 34 } }
 *    - "foo:map/bar>3/2/:boolean" => { foo: [0, 1, true, 3 ] }
 *    - "$:test/#12=2/:number" => { foo: }
 */
//FfxCborStatus ffx_cbor_queryData(FfxCborCursor *cursor, const char *path,
//  uint8_t *data, size_t *length);

//FfxCborStatus ffx_cbor_queryValue(FfxCborCursor *cursor, const char *path,
//  uint64_t *value);

//FfxCborStatus ffx_cbor_query(FfxCborCursor *cursor, const char *path,
//  FfxCborCursor *cursorOut);

/**
 *  Returns the type.
 */
FfxCborType ffx_cbor_getType(FfxCborCursor *cursor);

/**
 *  Returns true of the cursor type matxhes any of %%types%%.
 */
bool ffx_cbor_checkType(FfxCborCursor *cursor, FfxCborType types);

/**
 *  Returns the value for scalar types (Null, Boolean, Number).
 *
 *  Null is always 0. Boolean is either 0 for false, or 1 for true.
 */
FfxValueResult ffx_cbor_getValue(FfxCborCursor *cursor);

/**
 *  Exposes the underlying shared data buffer and length for data
 *  types (Data and String) without copying.
 *
 *  Do NOT modify these values.
 */
FfxDataResult ffx_cbor_getData(FfxCborCursor *cursor);

/**
 *  For Array and Map types, returns the number of values, and for
 *  Data and String types returns the length in bytes.
 */
FfxSizeResult ffx_cbor_getLength(FfxCborCursor *cursor);

bool ffx_cbor_checkLength(FfxCborCursor *cursor, FfxCborType types,
  size_t length);

/**
 *  Returns a cursor pointing to the value for %%key%%.
 *
 *  If the Map does not have %%key%%, returns .error = FfxDataErrorNotFound.
 */
FfxCborCursor ffx_cbor_followKey(FfxCborCursor *cursor, const char *key);

/**
 *  Returns a cursor pointing to the %%index%% item.
 *
 *  If outside the bounds of the Array, returns .error = FfxDataErrorNotFound.
 */
FfxCborCursor ffx_cbor_followIndex(FfxCborCursor *cursor, size_t index);

/**
 *  Iterates over an Array or Map.
 *
 *  example:
 *    FfxCborIterator iter = ffx_cbor_iterate(&cursor);
 *    while(ffx_cbor_nextChild(&iter)) {
 *        FfxCborCursor cursor = iter.child;
 *        // ...
 *    }
 *
 */
FfxCborIterator ffx_cbor_iterate(FfxCborCursor *container);

bool ffx_cbor_nextChild(FfxCborIterator *iterator);
//bool ffx_cbor_hasNext(FfxCborIterator *iterator);

//bool ffx_cbor_isDone(FfxCborCursor *cursor);

// Low-level; not for normal use. May be removed
bool _ffx_cbor_next(FfxCborCursor *cursor, FfxDataError *error);

/**
 *  Dumps the structured CBOR data to the console.
 */
void ffx_cbor_dump(FfxCborCursor *cursor);


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
bool ffx_cbor_appendData(FfxCborBuilder *cbor, uint8_t *data, size_t length);

/**
 *  Append a string.
 */
bool ffx_cbor_appendString(FfxCborBuilder *cbor, char* str);

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
