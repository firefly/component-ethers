#ifndef __FIREFLY_CBOR_H__
#define __FIREFLY_CBOR_H__

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
typedef uint16_t FfxCborBuilderTag;

/**
 *  The supported types for CBOR data.
 */
typedef enum FfxCborType {
    FfxCborTypeError    = 0,
    FfxCborTypeNull     = 1,
    FfxCborTypeBoolean  = 2,
    FfxCborTypeNumber   = 3,
    FfxCborTypeString   = 4,
    FfxCborTypeData     = 5,
    FfxCborTypeArray    = 6,
    FfxCborTypeMap      = 7
} FfxCborType;

typedef enum FfxCborStatus {
   // Returned at the end of a map or array during iteration or
   // when attempting to follow a map with a non-existant key or
   // array past its index.
   FfxCborStatusNotFound          = 5,

   // No error
   FfxCborStatusOK                = 0,

   // Attempted to perform an operation on an item that is not
   // supported, such as reading data from a number or followIndex
   // on a string.
   FfxCborStatusInvalidOperation  = -30,

   // Attempted to read past the end of the cursor data or write past
   // the end of the builder data
   FfxCborStatusBufferOverrun     = -31,

   // Some data was discarded during a getData
   FfxCborStatusTruncated         = -32,

   // The CBOR data contains an unsupported value type(e.g. tags)
   FfxCborStatusUnsupportedType   = -52,

   // Value represented does not fit within a uint64
   FfxCborStatusOverflow          = -55,
} FfxCborStatus;


/**
 *  A cursor used to traverse and read CBOR-encoded data.
 *
 *  This should not be modified directly! Only use the provided API.
 */
typedef struct FfxCborCursor {
    const uint8_t *data;
    size_t length;
    size_t offset;

    // The containerCount is used while traversing a map or array.
    // - a negative size indicates a map container
    // - a positive size indicates an array container
    // - 0 indicates this is not a container
    int32_t containerCount;

    // The index within the container of the cursor.
    size_t containerIndex;

//    FfxCborStatus status;
} FfxCborCursor;


/**
 *  A builder used to create and write CBOR-encoded data.
 *
 *  This should not be modified directly! Only use the provided API.
 */
typedef struct FfxCborBuilder {
    uint8_t *data;
    size_t length;
    size_t offset;

//    FfxCborStatus status;
} FfxCborBuilder;


// Detects if an error occurred during crawl or build... @TODO
//FfxCborStatus ffx_cbor_getWalkStatus(FfxCborCursor *cursor);
//FfxCborStatus ffx_cbor_getBuildStatus(FfxCborBuilder *builder);
//FfxCborStatus ffx_cbor_verify(uint8_t *data, size_t *length);

void ffx_cbor_walk(FfxCborCursor *cursor, const uint8_t *data, size_t length);
void ffx_cbor_clone(FfxCborCursor *dst, FfxCborCursor *src);

/**
 *  Returns the type.
 */
FfxCborType ffx_cbor_getType(FfxCborCursor *cursor);

/**
 *  Returns the value for scalar types (Null, Boolean, Number).
 *
 *  Null is always 0. Boolean is either 0 for false, or 1 for true.
 */
FfxCborStatus ffx_cbor_getValue(FfxCborCursor *cursor, uint64_t *value);

/**
 *  Copies the data for data types (Data and String), up to length bytes.
 */
FfxCborStatus ffx_cbor_copyData(FfxCborCursor *cursor, uint8_t *data,
  size_t length);

/**
 *  Exposes the underlying shared data buffer and length for data
 *  types (Data and String) without copying.
 *
 *  Do NOT modify these values.
 */
FfxCborStatus ffx_cbor_getData(FfxCborCursor *cursor, const uint8_t **data,
  size_t *length);

/**
 *  For Array and Map types, returns the number of values, and for
 *  Data and String types returns the length in bytes.
 */
FfxCborStatus ffx_cbor_getLength(FfxCborCursor *cursor, size_t *count);

/**
 *  Moves the %%cursor%% to the value for %%key%% within a Map.
 *
 *  If the Map does not have %%key%%, returns CborStatusNotFound.
 */
FfxCborStatus ffx_cbor_followKey(FfxCborCursor *cursor, const char *key);

/**
 *  Moves the %%cursor%% to the %%index%% value within an Array.
 *
 *  If outside the bounds of the Array, returns CborStatusNotFound.
 */
FfxCborStatus ffx_cbor_followIndex(FfxCborCursor *cursor, size_t index);

bool ffx_cbor_isDone(FfxCborCursor *cursor);

/**
 *  Moves %%cursor%% to the first value within a Map or Array.
 *
 *  If the container is a Map and %%key%% is not NULL, then it
 *  is updated to point to the key's value.
 *
 *  If the continaer is empty, returns CborStatusNotFound.
 */
FfxCborStatus ffx_cbor_firstValue(FfxCborCursor *cursor, FfxCborCursor *key);

/**
 *  Moves %%cursor%% to the next value within a Map or Array.
 *
 *  If the container is a Map and %%key%% is not NULL, then it
 *  is updated to point to the key's value.
 *
 *  If there are no more entries, returns CborStatusNotFound.
 */
FfxCborStatus ffx_cbor_nextValue(FfxCborCursor *cursor, FfxCborCursor *key);

// Low-level; not for normal use
FfxCborStatus _ffx_cbor_next(FfxCborCursor *cursor);
//uint8_t* cbor_raw(CborCursor *cursor, uint8_t *type, uint64_t *count);

/**
 *  Dumps the structured CBOR data to the console.
 */
void ffx_cbor_dump(FfxCborCursor *cursor);


/**
 *  Initialize a CBOR builder.
 */
void ffx_cbor_build(FfxCborBuilder *builder, uint8_t *data, size_t length);

/**
 *  The current length of the CBOR output.
 */
size_t ffx_cbor_getBuildLength(FfxCborBuilder *builder);

/**
 *  Append a boolean.
 */
FfxCborStatus ffx_cbor_appendBoolean(FfxCborBuilder *builder, bool value);

/**
 *  Append a null value.
 */
FfxCborStatus ffx_cbor_appendNull(FfxCborBuilder *builder);

/**
 *  Append a numeric value.
 */
FfxCborStatus ffx_cbor_appendNumber(FfxCborBuilder *builder, uint64_t value);

/**
 *  Append data.
 */
FfxCborStatus ffx_cbor_appendData(FfxCborBuilder *builder, uint8_t *data,
  size_t length);

/**
 *  Append a string.
 */
FfxCborStatus ffx_cbor_appendString(FfxCborBuilder *builder, char* str);

/**
 *  Begin an Array of %%count%% items long.
 *
 *  After calling this, the next %%count%% ffx_cbor_append* calls will
 *  place the item into this array.
 */
FfxCborStatus ffx_cbor_appendArray(FfxCborBuilder *builder, size_t count);

/**
 *  Begin a Map of %%count%% items long.
 *
 *  After calling this, the next %%count%% * 2 append calls will
 *  place the item into this mapping.
 *
 *  Each item should be use [[ffx_cbor_appendString]] for the key
 *  followed by any ffx_cbor_append*.
 */
FfxCborStatus ffx_cbor_appendMap(FfxCborBuilder *builder, size_t count);

/**
 *  Appends a Dynamic Array.
 *
 *  Similar to [[ffx_fbor_appendArray]], but will reserve additional
 *  space for the count (initially 0). The [[ffx_cbor_adjustCount]]
 *  must be used later to update the count to the correct value using
 *  the value stored in %%tag%%.
 */
FfxCborStatus ffx_cbor_appendArrayMutable(FfxCborBuilder *builder,
  FfxCborBuilderTag *tag);

/**
 *  Appends a Dynamic Map.
 *
 *  Similar to [[ffx_fbor_appendMap]], but will reserve additional
 *  space for the count (initially 0). The [[ffx_cbor_adjustCount]]
 *  must be used later to update the count to the correct value using
 *  the value stored in %%tag%%.
 */
FfxCborStatus ffx_cbor_appendMapMutable(FfxCborBuilder *builder,
  FfxCborBuilderTag *tag);

/**
 *  Update the count of a Dynamic Array or Dynamic Map
 */
void ffx_cbor_adjustCount(FfxCborBuilder *builder, FfxCborBuilderTag tag,
  uint16_t count);

/**
 *  Append raw CBOR-encoded %%data%% to an entry in %%builder%%.
 */
FfxCborStatus ffx_cbor_appendCborRaw(FfxCborBuilder *builder, uint8_t *data,
  size_t length);

/**
 *  Append an entire CborBuilder %%src%% to an entry in %%dst%%.
 */
FfxCborStatus ffx_cbor_appendCborBuilder(FfxCborBuilder *dst,
  FfxCborBuilder *src);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FIREFLY_CBOR_H__ */
