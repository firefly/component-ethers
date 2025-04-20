/**
 * 
 *  See: https://datatracker.ietf.org/doc/html/rfc8949
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "firefly-cbor.h"


#define MAX_LENGTH      (0x1000000)


///////////////////////////////
// Crawler - utils

static FfxCborType _getType(uint8_t header) {
    switch(header >> 5) {
        case 0:
            return FfxCborTypeNumber;
        case 2:
            return FfxCborTypeData;
        case 3:
            return FfxCborTypeString;
        case 4:
            return FfxCborTypeArray;
        case 5:
            return FfxCborTypeMap;
        case 7:
            switch(header & 0x1f) {
                case 20: case 21:
                    return FfxCborTypeBoolean;
                case 22:
                    return FfxCborTypeNull;
            }
            break;
    }
    return FfxCborTypeError;
}

static const uint8_t* _getBytes(FfxCborCursor *cursor, FfxCborType *type,
  uint64_t *value, size_t *safe, size_t *headerSize, FfxCborStatus *status) {

    *headerSize = 0;

    size_t length = cursor->length;
    size_t offset = cursor->offset;
    if (offset >= length) {
        *status = FfxCborStatusBufferOverrun;
        return NULL;
    }

    *value = 0;
    *safe = length - offset - 1;
    *status = FfxCborStatusOK;

    const uint8_t *data = &cursor->data[cursor->offset];

    uint8_t header = *data++;
    *headerSize = 1;

    *type = _getType(header);

    switch(*type) {
        case FfxCborTypeError:
            *status = FfxCborStatusUnsupportedType;
            return NULL;
        case FfxCborTypeNull:
            *value = 0;
            return data;
        case FfxCborTypeBoolean:
            *value = ((header & 0x1f) == 21) ? 1: 0;
            return data;
        default:
            break;
    }

    // Short value
    uint32_t count = header & 0x1f;
    if (count <= 23) {
        *value = count;
        return data;
    }

    // Indefinite lengths are not currently unsupported
    if (count > 27) {
        *status = FfxCborStatusUnsupportedType;
        return NULL;
    }

    // Count bytes
    // 24 => 0, 25 => 1, 26 => 2, 27 => 3
    count = 1 << (count - 24);
    if (count > *safe) {
        *status = FfxCborStatusBufferOverrun;
        return NULL;
    }

    *headerSize = 1 + count;

    // Read the count bytes as the value
    uint64_t v = 0;
    for (int i = 0; i < count; i++) {
        v = (v << 8) | *data++;
    }
    *value = v;

    // Remove the read bytes from the safe count
    *safe -= count;

    return data;
}


///////////////////////////////
// Crawler

void ffx_cbor_walk(FfxCborCursor *cursor, const uint8_t *data, size_t length) {
    cursor->data = data;
    cursor->length = length;
    cursor->offset = 0;
    cursor->containerCount = 0;
    cursor->containerIndex = 0;
}

void ffx_cbor_clone(FfxCborCursor *dst, FfxCborCursor *src) {
    memmove(dst, src, sizeof(FfxCborCursor));
}

bool ffx_cbor_isDone(FfxCborCursor *cursor) {
    return (cursor->offset == cursor->length);
}

FfxCborType ffx_cbor_getType(FfxCborCursor *cursor) {
    if (cursor->offset >= cursor->length) { return FfxCborTypeError; }
    return _getType(cursor->data[cursor->offset]);
}

bool ffx_cbor_checkType(FfxCborCursor *cursor, FfxCborType types) {
    return (ffx_cbor_getType(cursor) & types);
}

uint64_t ffx_cbor_getValue(FfxCborCursor *cursor, FfxCborStatus *error) {
    if (error) {
        if (*error) { return 0; }
        *error = FfxCborStatusOK;
    }

    FfxCborType type = 0;
    uint64_t value;
    size_t safe = 0, headLen = 0;
    FfxCborStatus status = FfxCborStatusOK;
    const uint8_t *data = _getBytes(cursor, &type, &value, &safe, &headLen, &status);
    if (status) {
        if (error) { *error = status; }
        return 0;
    }

    assert(data != NULL && type != FfxCborTypeError);

    switch (type) {
        case FfxCborTypeNull: case FfxCborTypeBoolean: case FfxCborTypeNumber:
            return value;
        default:
            break;
    }

    if (error) { *error = FfxCborStatusInvalidOperation; }
    return 0;
}

// @TODO: refactor these 3 functions

FfxCborData ffx_cbor_getData(FfxCborCursor *cursor, FfxCborStatus *error) {

    FfxCborData result = { .bytes = NULL, .length = 0 };

    if (error) {
        if (*error) { return result; }
        *error = FfxCborStatusOK;
    }

    FfxCborType type = 0;
    uint64_t value;
    size_t safe = 0, headLen = 0;
    FfxCborStatus status = FfxCborStatusOK;
    const uint8_t *data = _getBytes(cursor, &type, &value, &safe, &headLen, &status);
    if (status) {
        if (error) { *error = status; }
        return result;
    }

    assert(data != NULL && type != FfxCborTypeError);

    if (type != FfxCborTypeData && type != FfxCborTypeString) {
        if (error) { *error = FfxCborStatusInvalidOperation; }
        return result;
    }

    // Would read beyond our data
    if (value > safe) {
        if (error) { *error = FfxCborStatusBufferOverrun; }
        return result;
    }

    // Only support lengths up to 24 bits
    if (value >= MAX_LENGTH) {
        if (error) { *error = FfxCborStatusOverflow; }
        return result;
    }

    result.bytes = data;
    result.length = value;

    return result;
}
/*
FfxCborStatus ffx_cbor_copyData(FfxCborCursor *cursor, uint8_t *output, size_t length) {
    FfxCborType type = 0;
    uint64_t value;
    size_t safe = 0, headLen = 0;
    FfxCborStatus status = FfxCborStatusOK;
    const uint8_t *data = _getBytes(cursor, &type, &value, &safe, &headLen, &status);
    if (data == NULL) { return status; }

    if (type != FfxCborTypeData && type != FfxCborTypeString) {
        return FfxCborStatusInvalidOperation;
    }

    // Would read beyond our data
    if (value > safe) { return FfxCborStatusBufferOverrun; }

    // Only support lengths up to 16 bits
    if (value > 0xffffffff) { return FfxCborStatusOverflow; }

    status = FfxCborStatusOK;

    // Not enough space; copy what we can, but mark as truncated
    if (length < value) {
        status = FfxCborStatusTruncated;
        value = length;
    }

    //for (int i = 0; i < value; i++) { output[i] = data[i]; }
    memmove(output, data, value);

    return status;
}
*/

size_t ffx_cbor_getLength(FfxCborCursor *cursor, FfxCborStatus *error) {
    if (error) {
        if (*error) { return 0; }
        *error = FfxCborStatusOK;
    }

    FfxCborType type = 0;
    uint64_t value;
    size_t safe = 0, headLen = 0;;
    FfxCborStatus status = FfxCborStatusOK;
    const uint8_t *data = _getBytes(cursor, &type, &value, &safe, &headLen, &status);
    if (status) {
        if (error) { *error = status; }
        return 0;
    }

    assert(data != NULL && type != FfxCborTypeError);

    // Only support lengths up to 16 bits
    if (value > MAX_LENGTH) {
        if (error) { *error = FfxCborStatusOverflow; }
        return 0;
    }

    switch (type) {
        case FfxCborTypeData: case FfxCborTypeString:
        case FfxCborTypeArray: case FfxCborTypeMap:
            return value;
        default:
            break;
    }

    if (error) { *error = FfxCborStatusInvalidOperation; }
    return 0;
}

bool ffx_cbor_checkLength(FfxCborCursor *cursor, size_t length) {
    FfxCborStatus error = FfxCborStatusOK;
    size_t l = ffx_cbor_getLength(cursor, &error);
    if (error) { return 0; }
    return (l == length);
}


bool _ffx_cbor_next(FfxCborCursor *cursor, FfxCborStatus *error) {
    if (cursor->offset >= cursor->length) { return false; }

    FfxCborType type = 0;
    uint64_t value;
    size_t safe = 0, headLen = 0;
    FfxCborStatus status = FfxCborStatusOK;
    const uint8_t *data = _getBytes(cursor, &type, &value, &safe, &headLen, &status);
    if (status) {
        if (error) { *error = status; }
        return false;
    }

    assert(data != NULL);

    switch (type) {
        case FfxCborTypeError:
            assert(0);

        // Enters into the first element
        case FfxCborTypeArray: case FfxCborTypeMap:
            // ... falls-through ...

        case FfxCborTypeNull: case FfxCborTypeBoolean: case FfxCborTypeNumber:
            cursor->offset += headLen;
            break;

        case FfxCborTypeData: case FfxCborTypeString:
            cursor->offset += headLen + value;
            break;
    }

    return true;
}

static bool firstValue(FfxCborCursor *cursor, FfxCborCursor *key,
  FfxCborStatus *error) {

    if (*error) { return false; }
    *error = FfxCborStatusOK;

    FfxCborType type = 0;
    uint64_t value;
    size_t safe = 0, headLen = 0;
    FfxCborStatus status = FfxCborStatusOK;
    const uint8_t *data = _getBytes(cursor, &type, &value, &safe, &headLen, &status);
    if (status) {
        *error = status;
        return false;
    }

    assert(data != NULL && type != FfxCborTypeError);

    // Done; first value of an empty set
    if (value == 0) { return false; }

    if (value > MAX_LENGTH) {
        if (error) { *error = FfxCborStatusOverflow; }
        return false;
    }

    FfxCborCursor follow;
    ffx_cbor_clone(&follow, cursor);

    if (type == FfxCborTypeArray) {
        if (!_ffx_cbor_next(&follow, error)) { return false; }
        follow.containerCount = value;
        follow.containerIndex = 0;
        ffx_cbor_clone(cursor, &follow);
        return true;
    }

    if (type == FfxCborTypeMap) {
        if (!_ffx_cbor_next(&follow, error)) { return false; }
        if (key) {
            ffx_cbor_clone(key, &follow);
            key->containerCount = 0;
            key->containerIndex = 0;
        }
        if (!_ffx_cbor_next(&follow, error)) { return false; }
        follow.containerCount = -value;
        follow.containerIndex = 0;
        ffx_cbor_clone(cursor, &follow);
        return true;
    }

    *error = FfxCborStatusInvalidOperation;
    return false;
}

// @TODO: on error shold we reset the key?
static bool nextValue(FfxCborCursor *cursor, FfxCborCursor *key,
  FfxCborStatus *error) {

    if (*error) { return false; }

    bool hasKey = false;
    int32_t count = cursor->containerCount;

    if (count == 0) {
        if (error) { *error = FfxCborStatusInvalidOperation; }
        return false;
    }

    if (count < 0) {
        hasKey = true;
        count *= -1;
    }

    if (cursor->containerIndex + 1 == count) { return false; }
    cursor->containerIndex++;

    FfxCborCursor follow;
    ffx_cbor_clone(&follow, cursor);

    int32_t skip = 1;
    while (skip != 0) {
        FfxCborType type = ffx_cbor_getType(&follow);
        if (type == FfxCborTypeArray) {
            size_t length = ffx_cbor_getLength(&follow, error);
            if (*error) { return false; }
            skip += length;
        } else if (type == FfxCborTypeMap) {
            size_t length = ffx_cbor_getLength(&follow, error);
            if (*error) { return false; }
            skip += 2 * length;
        }

        if (!_ffx_cbor_next(&follow, error)) { return false; }

        skip--;
    }

    if (hasKey) {
        if (key) {
            ffx_cbor_clone(key, &follow);
            key->containerCount = 0;
            key->containerIndex = 0;
        }
        if (!_ffx_cbor_next(&follow, error)) { return false; }
    }

    ffx_cbor_clone(cursor, &follow);

    return true;
}

bool ffx_cbor_iterate(FfxCborCursor *cursor, FfxCborCursor *key,
  FfxCborStatus *error) {

    // An error MUST be passed in
    if (error == NULL) { return false; }

    if (*error == FfxCborStatusBeginIterator) {
        *error = FfxCborStatusOK;
        return firstValue(cursor, key, error);

    } else if (*error) {
        // Existing error!
        return false;

    }

    return nextValue(cursor, key, error);
}

static bool _keyCompare(const char *key, FfxCborCursor *cursor,
  FfxCborStatus *error) {

    size_t length = strlen(key);

    FfxCborData data = ffx_cbor_getData(cursor, error);
    if (*error) { return false; }

    if (length != data.length) { return false; }

    for (int i = 0; i < length; i++) {
        if (key[i] != data.bytes[i]) { return false; }
    }

    return true;
}

bool ffx_cbor_followKey(FfxCborCursor *cursor, const char *key,
  FfxCborStatus *error) {

    if (error) {
        if (*error) { return false; }
        *error = FfxCborStatusOK;
    }

    if (!ffx_cbor_checkType(cursor, FfxCborTypeMap)) {
        if (error) { *error = FfxCborStatusInvalidOperation; }
        return false;
    }

    FfxCborCursor follow, followKey;
    ffx_cbor_clone(&follow, cursor);

    FfxCborStatus status = FfxCborStatusBeginIterator;
    while (ffx_cbor_iterate(&follow, &followKey, &status)) {
        if (!ffx_cbor_checkType(&followKey, FfxCborTypeString)) {
            if (error) { *error = FfxCborStatusBadData; }
            return false;
        }

        if (_keyCompare(key, &followKey, &status)) {
            ffx_cbor_clone(cursor, &follow);
            return true;
        }
    }

    if (status && error) { *error = status; }

    return false;
}

bool ffx_cbor_followIndex(FfxCborCursor *cursor, size_t index,
  FfxCborStatus *error) {

    if (error) {
        if (*error) { return false; }
        *error = FfxCborStatusOK;
    }

    if (!ffx_cbor_checkType(cursor, FfxCborTypeArray | FfxCborTypeMap)) {
        if (error) { *error = FfxCborStatusInvalidOperation; }
        return false;
    }

    FfxCborCursor follow;
    ffx_cbor_clone(&follow, cursor);

    size_t i = 0;

    FfxCborStatus status = FfxCborStatusBeginIterator;
    while (ffx_cbor_iterate(&follow, NULL, &status)) {
        if (i == index) {
            ffx_cbor_clone(cursor, &follow);
            return true;
        }
        i++;
    }


    return false;
}

static void _dump(FfxCborCursor *cursor) {
    FfxCborType type = _getType(cursor->data[cursor->offset]);

    FfxCborStatus status = FfxCborStatusOK;

    switch(type) {
        case FfxCborTypeNumber: {
            uint64_t value = ffx_cbor_getValue(cursor, &status);
            if (status) { break; }

            printf("%lld", value);
            break;
        }

        case FfxCborTypeString: {
            FfxCborData data = ffx_cbor_getData(cursor, &status);
            if (status) { break; }

            printf("\"");
            for (int i = 0; i < data.length; i++) {
                char c = data.bytes[i];
                if (c == '\n') {
                    printf("\\n");
                } else if (c < 32 || c >= 127) {
                    printf("\\%0x2", c);
                } else if (c == '"') {
                    printf("\\\"");
                } else {
                    printf("%c", c);
                }
            }
            printf("\"");
            break;
        }

        case FfxCborTypeData: {
            FfxCborData data = ffx_cbor_getData(cursor, &status);
            if (status) { break; }

            printf("0x");
            for (int i = 0; i < data.length; i++) {
                printf("%02x", data.bytes[i]);
            }
            break;
        }

        case FfxCborTypeArray: {
            printf("[ ");

            bool first = true;

            FfxCborCursor follow;
            ffx_cbor_clone(&follow, cursor);

            FfxCborStatus status = FfxCborStatusBeginIterator;
            while (ffx_cbor_iterate(&follow, NULL, &status)) {
                if (!first) { printf(", "); }
                first = false;
                _dump(&follow);
            }

            if (status) { break; }

            if (!first) { printf(" "); }
            printf("]");
            break;
        }

        case FfxCborTypeMap: {
            printf("{ ");

            bool first = true;

            FfxCborCursor follow, followKey;
            ffx_cbor_clone(&follow, cursor);

            FfxCborStatus status = FfxCborStatusBeginIterator;
            while (ffx_cbor_iterate(&follow, &followKey, &status)) {
                if (!first) { printf(", "); }
                first = false;
                _dump(&followKey);
                printf(": ");
                _dump(&follow);
            }

            if (status) { break; }

            if (!first) { printf(" "); }
            printf(" }");
            break;
        }

        case FfxCborTypeBoolean: {
            uint64_t value = ffx_cbor_getValue(cursor, &status);
            if (status) { break; }

            printf("%s", value ? "true": "false");
            break;
        }

        case FfxCborTypeNull:
            printf("null");
            break;

        default:
            printf("ERROR");
    }

    // @TODO: Fill this in more
    if (status) {
        printf("<ERROR>");
    }
}

void ffx_cbor_dump(FfxCborCursor *cursor) {
    _dump(cursor);
    printf("\n");
}

///////////////////////////////
// Builder - utils

static FfxCborStatus _appendHeader(FfxCborBuilder *builder, FfxCborType type,
  uint64_t value) {

    size_t remaining = builder->length - builder->offset;

    if (value < 23) {
        if (remaining < 1) { return FfxCborStatusBufferOverrun; }
        builder->data[builder->offset++] = (type << 5) | value;
        return FfxCborStatusOK;
    }

    // Convert to 8 bytes
    size_t inset = 7;
    uint8_t bytes[8] = { 0 };
    for (int i = 7; i >= 0; i--) {
        uint64_t v = (value >> (uint64_t)(56 - (i * 8))) & 0xff;
        bytes[i] = v;
        if (v) { inset = i; }
    }

    // Align the offset to the closest power-of-two within bytes
    uint8_t counts[] = { 27, 27, 27, 27, 26, 26, 25, 24 };
    uint8_t count = counts[inset];
    inset = 8 - (1 << (count - 24));

    if (remaining < 1 + (8 - inset)) { return FfxCborStatusBufferOverrun; }

    size_t offset = builder->offset;
    builder->data[offset++] = (type << 5) | count;
    for (int i = inset; i < 8; i++) {
        builder->data[offset++] = bytes[i];
    }
    builder->offset = offset;

    return FfxCborStatusOK;
}


///////////////////////////////
// Builder

void ffx_cbor_build(FfxCborBuilder *builder, uint8_t *data, size_t length) {
    builder->data = data;
    builder->length = length;
    builder->offset = 0;
}

size_t ffx_cbor_getBuildLength(FfxCborBuilder *builder) {
    return builder->offset;
}

FfxCborStatus ffx_cbor_appendBoolean(FfxCborBuilder *builder, bool value) {
    size_t remaining = builder->length - builder->offset;
    if (remaining < 1) { return FfxCborStatusBufferOverrun; }
    size_t offset = builder->offset;
    builder->data[offset++] = (7 << 5) | (value ? 21: 20);
    builder->offset = offset;
    return FfxCborStatusOK;
}

FfxCborStatus ffx_cbor_appendNull(FfxCborBuilder *builder) {
    size_t remaining = builder->length - builder->offset;
    if (remaining < 1) { return FfxCborStatusBufferOverrun; }
    size_t offset = builder->offset;
    builder->data[offset++] = (7 << 5) | 22;
    builder->offset = offset;
    return FfxCborStatusOK;
}

FfxCborStatus ffx_cbor_appendNumber(FfxCborBuilder *builder, uint64_t value) {
    return _appendHeader(builder, 0, value);
}

FfxCborStatus ffx_cbor_appendData(FfxCborBuilder *builder, uint8_t *data, size_t length) {
    FfxCborStatus status = _appendHeader(builder, 2, length);
    if (status) { return status; }

    size_t remaining = builder->length - builder->offset;
    if (remaining < length) { return FfxCborStatusBufferOverrun; }

    memmove(&builder->data[builder->offset], data, length);
    builder->offset += length;

    return FfxCborStatusOK;
}

FfxCborStatus ffx_cbor_appendString(FfxCborBuilder *builder, char* str) {
    size_t length = strlen(str);

    FfxCborStatus status = _appendHeader(builder, 3, length);
    if (status) { return status; }

    size_t remaining = builder->length - builder->offset;
    if (remaining < length) { return FfxCborStatusBufferOverrun; }

    memmove(&builder->data[builder->offset], str, length);
    builder->offset += length;

    return FfxCborStatusOK;
}

FfxCborStatus ffx_cbor_appendArray(FfxCborBuilder *builder, size_t count) {
    return _appendHeader(builder, 4, count);
}

FfxCborStatus ffx_cbor_appendMap(FfxCborBuilder *builder, size_t count) {
    return _appendHeader(builder, 5, count);
}

FfxCborStatus ffx_cbor_appendArrayMutable(FfxCborBuilder *builder, FfxCborBuilderTag *tag) {
    size_t remaining = builder->length - builder->offset;
    if (remaining < 3) { return FfxCborStatusBufferOverrun; }

    builder->data[builder->offset++] = (4 << 5) | 25;

    *tag = builder->offset;

    builder->data[builder->offset++] = 0;
    builder->data[builder->offset++] = 0;

    return FfxCborStatusOK;
}

FfxCborStatus ffx_cbor_appendMapMutable(FfxCborBuilder *builder, FfxCborBuilderTag *tag) {
    size_t remaining = builder->length - builder->offset;
    if (remaining < 3) { return FfxCborStatusBufferOverrun; }

    builder->data[builder->offset++] = (5 << 5) | 25;

    *tag = builder->offset;

    builder->data[builder->offset++] = 0;
    builder->data[builder->offset++] = 0;

    return FfxCborStatusOK;
}

void ffx_cbor_adjustCount(FfxCborBuilder *builder, FfxCborBuilderTag tag,
  uint16_t count) {
    builder->data[tag++] = (count >> 16) & 0xff;
    builder->data[tag++] = count & 0xff;
}

FfxCborStatus ffx_cbor_appendCborRaw(FfxCborBuilder *builder, uint8_t *data,
  size_t length) {

    size_t remaining = builder->length - builder->offset;
    if (remaining < length) { return FfxCborStatusBufferOverrun; }

    size_t offset = builder->offset;
    memmove(&builder->data[offset], data, length);
    builder->offset = offset + length;

    return FfxCborStatusOK;
}

FfxCborStatus ffx_cbor_appendCborBuilder(FfxCborBuilder *dst, FfxCborBuilder *src) {
    return ffx_cbor_appendCborRaw(dst, src->data, src->offset);
}

