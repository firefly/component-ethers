/**
 * 
 *  See: https://datatracker.ietf.org/doc/html/rfc8949
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "firefly-cbor.h"


#define MAX_LENGTH      (0xffffff)

#define FIRST_ITER      (0xf000000)

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

typedef struct CursorInfo {
    const uint8_t *data;
    FfxCborType type;
    uint64_t value;
    size_t safe, headerSize;
    FfxDataError error;
} CursorInfo;

static CursorInfo getInfo(const FfxCborCursor *cursor) {

    CursorInfo result = { 0 };

    size_t length = cursor->length;
    size_t offset = cursor->offset;
    if (offset >= length) {
        return (CursorInfo){ .error = FfxDataErrorBufferOverrun };
    }

    result.safe = length - offset - 1;

    const uint8_t *data = &cursor->data[cursor->offset];

    uint8_t header = *data++;
    result.headerSize = 1;

    result.type = _getType(header);

    switch(result.type) {
        case FfxCborTypeError:
            return (CursorInfo){ .error = FfxDataErrorUnsupportedFeature };
        case FfxCborTypeNull:
            result.value = 0;
            return result;
        case FfxCborTypeBoolean:
            result.value = (((header & 0x1f) == 21) ? 1: 0);
            return result;
        default:
            break;
    }

    // Short value
    uint32_t count = header & 0x1f;
    if (count <= 23) {
        result.data = data;
        result.value = count;
        return result;
    }

    // Indefinite lengths are not currently unsupported
    if (count > 27) {
        return (CursorInfo){ .error = FfxDataErrorUnsupportedFeature };
    }

    // Count bytes
    // 24 => 0, 25 => 1, 26 => 2, 27 => 3
    count = 1 << (count - 24);
    if (count > result.safe) {
        return (CursorInfo){ .error = FfxDataErrorBufferOverrun };
    }

    result.headerSize = 1 + count;

    // Read the count bytes as the value
    uint64_t v = 0;
    for (int i = 0; i < count; i++) {
        v = (v << 8) | *data++;
    }
    result.data = data;
    result.value = v;

    // Remove the read bytes from the safe count
    result.safe -= count;

    return result;
}


///////////////////////////////
// Crawler

FfxCborCursor ffx_cbor_walk(const uint8_t *data, size_t length) {
    return (FfxCborCursor){ .data = data, .length = length };
}

/*
FfxCborCursor ffx_cbor_clone(const FfxCborCursor *cursor) {
    FfxCborCursor result = { 0 };
    memcpy(&result, cursor, sizeof(FfxCborCursor));
    return result;
}
*/
//bool ffx_cbor_isDone(FfxCborCursor *cursor) {
//    return (cursor->offset == cursor->length);
//}

FfxCborType ffx_cbor_getType(FfxCborCursor cursor) {
    if (cursor.offset >= cursor.length) { return FfxCborTypeError; }
    return _getType(cursor.data[cursor.offset]);
}

bool ffx_cbor_checkType(FfxCborCursor cursor, FfxCborType types) {
    return (ffx_cbor_getType(cursor) & types);
}

FfxValueResult ffx_cbor_getValue(FfxCborCursor cursor) {

    CursorInfo info = getInfo(&cursor);
    if (info.error) { return (FfxValueResult){ .error = info.error }; }
    assert(info.data != NULL && info.type != FfxCborTypeError);

    switch (info.type) {
        case FfxCborTypeNull: case FfxCborTypeBoolean: case FfxCborTypeNumber:
            return (FfxValueResult){ .value = info.value };
        default:
            break;
    }

    return (FfxValueResult){ .error = FfxDataErrorInvalidOperation };
}

// @TODO: refactor these 3 functions

FfxDataResult ffx_cbor_getData(FfxCborCursor cursor) {

    CursorInfo info = getInfo(&cursor);
    if (info.error) { return (FfxDataResult){ .error = info.error }; }
    assert(info.data != NULL && info.type != FfxCborTypeError);

    if (info.type != FfxCborTypeData && info.type != FfxCborTypeString) {
        return (FfxDataResult){ .error = FfxDataErrorInvalidOperation };
    }

    // Would read beyond our data
    if (info.value > info.safe) {
        return (FfxDataResult){ .error = FfxDataErrorBufferOverrun };
    }

    // Only support lengths up to 24 bits
    if (info.value >= MAX_LENGTH) {
        return (FfxDataResult){ .error = FfxDataErrorOverflow };
    }

    return (FfxDataResult){ .bytes = info.data, .length = info.value };
}

FfxSizeResult ffx_cbor_getLength(FfxCborCursor cursor) {

    CursorInfo info = getInfo(&cursor);
    if (info.error) { return (FfxSizeResult){ .error = info.error }; }
    assert(info.data != NULL && info.type != FfxCborTypeError);

    // Only support lengths up to 16 bits
    if (info.value > MAX_LENGTH) {
        return (FfxSizeResult){ .error = FfxDataErrorOverflow };
    }

    switch (info.type) {
        case FfxCborTypeData: case FfxCborTypeString:
        case FfxCborTypeArray: case FfxCborTypeMap:
            return (FfxSizeResult){ .value = info.value };
        default:
            break;
    }

    return (FfxSizeResult){ .error = FfxDataErrorInvalidOperation };
}

bool ffx_cbor_checkLength(FfxCborCursor cursor, FfxCborType types,
  size_t length) {

    if (cursor.error) { return false; }
    if (!ffx_cbor_checkType(cursor, types)) { return false; }
    FfxSizeResult result = ffx_cbor_getLength(cursor);
    if (result.error) { return false; }
    return (result.value == length);
}

// Make fully private?
bool _ffx_cbor_next(FfxCborCursor *cursor, FfxDataError *error) {
    if (cursor->offset >= cursor->length) { return false; }

    CursorInfo info = getInfo(cursor);
    if (info.error) {
        if (error) { *error = info.error; }
        return false;
    }
    assert(info.data != NULL && info.type != FfxCborTypeError);

    switch (info.type) {
        case FfxCborTypeError:
            assert(0);

        // Enters into the first element
        case FfxCborTypeArray: case FfxCborTypeMap:
            // ... falls-through ...

        case FfxCborTypeNull: case FfxCborTypeBoolean: case FfxCborTypeNumber:
            cursor->offset += info.headerSize;
            break;

        case FfxCborTypeData: case FfxCborTypeString:
            cursor->offset += info.headerSize + info.value;
            break;
    }

    return true;
}

static bool firstValue(FfxCborIterator *iter) {

    CursorInfo info = getInfo(&iter->container);
    if (info.error) {
        iter->error = info.error;
        return false;
    }
    assert(info.data != NULL && info.type != FfxCborTypeError);

    // Done; first value of an empty set
    if (info.value == 0) { return false; }

    if (info.value > MAX_LENGTH) {
        iter->error = FfxDataErrorOverflow;
        return false;
    }

    FfxCborCursor follow = iter->container;

    if (info.type == FfxCborTypeArray) {
        if (!_ffx_cbor_next(&follow, &iter->error)) { return false; }

        iter->_containerCount = info.value;
        iter->_containerIndex = 0;
        iter->child = follow;
        iter->key = (FfxCborCursor){ .error = FfxDataErrorNotFound };
        return true;
    }

    if (info.type == FfxCborTypeMap) {
        if (!_ffx_cbor_next(&follow, &iter->error)) { return false; }

        if (!ffx_cbor_checkType(follow, FfxCborTypeString)) {
            iter->error = FfxDataErrorBadData;
            return false;
        }
        iter->key = follow;

        if (!_ffx_cbor_next(&follow, &iter->error)) { return false; }
        iter->_containerCount = info.value;
        iter->_containerIndex = 0;
        iter->child = follow;
        return true;
    }

    iter->error = FfxDataErrorInvalidOperation;
    return false;
}

static bool nextValue(FfxCborIterator *iter) {

    bool hasKey = (ffx_cbor_getType(iter->container) == FfxCborTypeMap);
    int32_t count = iter->_containerCount;

    if (count == 0) {
        iter->error = FfxDataErrorInvalidOperation;
        return false;
    }

    if (iter->_containerIndex + 1 == count) { return false; }
    iter->_containerIndex++;

    FfxCborCursor follow = iter->child;

    int32_t skip = 1;
    while (skip != 0) {
        FfxCborType type = ffx_cbor_getType(follow);
        if (type == FfxCborTypeArray) {
            FfxSizeResult result = ffx_cbor_getLength(follow);
            if (result.error) { return false; }
            skip += result.value;
        } else if (type == FfxCborTypeMap) {
            FfxSizeResult result = ffx_cbor_getLength(follow);
            if (result.error) { return false; }
            skip += 2 * result.value;
        }

        if (!_ffx_cbor_next(&follow, &iter->error)) { return false; }

        skip--;
    }

    if (hasKey) {
        if (!ffx_cbor_checkType(follow, FfxCborTypeString)) {
            iter->error = FfxDataErrorBadData;
            return false;
        }
        iter->key = follow;

        if (!_ffx_cbor_next(&follow, &iter->error)) { return false; }
    } else {
        iter->key = (FfxCborCursor){ .error = FfxDataErrorNotFound };
    }

    iter->child = follow;

    return true;
}

FfxCborIterator ffx_cbor_iterate(FfxCborCursor container) {
    if (container.error) {
        return (FfxCborIterator){ .error = container.error };
    }

    return (FfxCborIterator){
        .container = container,
        ._containerIndex = FIRST_ITER
    };
}

bool ffx_cbor_nextChild(FfxCborIterator *iter) {
    if (iter->error) { return false; }

    if (iter->_containerIndex == FIRST_ITER) { return firstValue(iter); }

    return nextValue(iter);
}

static bool _keyCompare(const char *key, FfxCborCursor cursor,
  FfxDataError *error) {

    size_t length = strlen(key);

    FfxDataResult data = ffx_cbor_getData(cursor);
    if (data.error) { return false; }

    if (length != data.length) { return false; }

    for (int i = 0; i < length; i++) {
        if (key[i] != data.bytes[i]) { return false; }
    }

    return true;
}

FfxCborCursor ffx_cbor_followKey(FfxCborCursor cursor, const char *key) {

    if (!ffx_cbor_checkType(cursor, FfxCborTypeMap)) {
        return (FfxCborCursor){ .error = FfxDataErrorInvalidOperation };
    }

    FfxCborIterator iter = ffx_cbor_iterate(cursor);
    while (ffx_cbor_nextChild(&iter)) {
        if (iter.error) { break; }
        if (_keyCompare(key, iter.key, &iter.error)) { return iter.child; }
    }

    if (iter.error) { return (FfxCborCursor){ .error = iter.error }; }

    return (FfxCborCursor){ .error = FfxDataErrorNotFound };
}

FfxCborCursor ffx_cbor_followIndex(FfxCborCursor cursor, size_t index) {

    if (!ffx_cbor_checkType(cursor, FfxCborTypeArray | FfxCborTypeMap)) {
        return (FfxCborCursor){ .error = FfxDataErrorInvalidOperation };
    }

    size_t i = 0;

    FfxCborIterator iter = ffx_cbor_iterate(cursor);
    while (ffx_cbor_nextChild(&iter)) {
        if (iter.error) {
            return (FfxCborCursor){ .error = iter.error };
        }
        if (i == index) { return iter.child; }
        i++;
    }

    return (FfxCborCursor){ .error = FfxDataErrorNotFound };
}

static void _dump(const FfxCborCursor cursor) {
    FfxCborType type = _getType(cursor.data[cursor.offset]);

    switch(type) {
        case FfxCborTypeNumber: {
            FfxValueResult result = ffx_cbor_getValue(cursor);
            if (result.error) { break; }

            printf("%lld", result.value);
            break;
        }

        case FfxCborTypeString: {
            FfxDataResult data = ffx_cbor_getData(cursor);
            if (data.error) { break; }

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
            FfxDataResult data = ffx_cbor_getData(cursor);
            if (data.error) { break; }

            printf("0x");
            for (int i = 0; i < data.length; i++) {
                printf("%02x", data.bytes[i]);
            }
            break;
        }

        case FfxCborTypeArray: {
            printf("[ ");

            bool first = true;

            FfxCborIterator iter = ffx_cbor_iterate(cursor);
            while (ffx_cbor_nextChild(&iter)) {
                if (!first) { printf(", "); }
                first = false;
                _dump(iter.child);
            }

            if (iter.child.error) {
                printf("<ERROR status=%d>", iter.child.error);
                break;
            }

            if (!first) { printf(" "); }
            printf("]");
            break;
        }

        case FfxCborTypeMap: {
            printf("{ ");

            bool first = true;

            FfxCborIterator iter = ffx_cbor_iterate(cursor);
            while (ffx_cbor_nextChild(&iter)) {
                if (!first) { printf(", "); }
                first = false;
                _dump(iter.key);
                printf(": ");
                _dump(iter.child);
            }

            if (iter.child.error) {
                printf("<ERROR status=%d>", iter.child.error);
                break;
            }

            if (!first) { printf(" "); }
            printf(" }");
            break;
        }

        case FfxCborTypeBoolean: {
            FfxValueResult result = ffx_cbor_getValue(cursor);
            if (result.error) { break; }

            printf("%s", result.value ? "true": "false");
            break;
        }

        case FfxCborTypeNull:
            printf("null");
            break;

        default:
            printf("<ERROR type=%d>", type);
            return;
    }
}

void ffx_cbor_dump(FfxCborCursor cursor) {
    _dump(cursor);
    printf("\n");
}

///////////////////////////////
// Builder - utils

static bool _appendHeader(FfxCborBuilder *cbor, FfxCborType type,
  uint64_t value) {

    if (cbor->error) { return false; }

    if (value < 23) {
        if (cbor->length < cbor->offset + 1) {
            cbor->error = FfxDataErrorBufferOverrun;
            return false;
        }
        cbor->data[cbor->offset++] = (type << 5) | value;
        return true;
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

    if (cbor->length < cbor->offset + 1 + (8 - inset)) {
        cbor->error = FfxDataErrorBufferOverrun;
        return false;
    }

    size_t offset = cbor->offset;
    cbor->data[offset++] = (type << 5) | count;
    for (int i = inset; i < 8; i++) {
        cbor->data[offset++] = bytes[i];
    }
    cbor->offset = offset;

    return true;
}


///////////////////////////////
// Builder

FfxCborBuilder ffx_cbor_build(uint8_t *data, size_t length) {
    return (FfxCborBuilder){ .data = data, .length = length };
}

size_t ffx_cbor_getBuildLength(FfxCborBuilder *cbor) {
    return cbor->offset;
}

bool ffx_cbor_appendBoolean(FfxCborBuilder *cbor, bool value) {
    if (cbor->error) { return false; }

    if (cbor->length < cbor->offset + 1) {
        cbor->error = FfxDataErrorBufferOverrun;
        return false;
    }

    size_t offset = cbor->offset;
    cbor->data[offset++] = (7 << 5) | (value ? 21: 20);
    cbor->offset = offset;

    return true;
}

bool ffx_cbor_appendNull(FfxCborBuilder *cbor) {
    if (cbor->error) { return false; }

    if (cbor->length < cbor->offset + 1) {
        cbor->error = FfxDataErrorBufferOverrun;
        return false;
    }

    size_t offset = cbor->offset;
    cbor->data[offset++] = (7 << 5) | 22;
    cbor->offset = offset;

    return true;
}

bool ffx_cbor_appendNumber(FfxCborBuilder *cbor, uint64_t value) {
    return _appendHeader(cbor, 0, value);
}

bool ffx_cbor_appendData(FfxCborBuilder *cbor, uint8_t *data, size_t length) {
    if (!_appendHeader(cbor, 2, length)) { return false; }

    if (cbor->length < cbor->offset + length) {
        cbor->error = FfxDataErrorBufferOverrun;
        return false;
    }

    memmove(&cbor->data[cbor->offset], data, length);
    cbor->offset += length;

    return FfxDataErrorNone;
}

bool ffx_cbor_appendString(FfxCborBuilder *cbor, char* str) {
    if (cbor->error) { return false; }

    size_t length = strlen(str);

    if (!_appendHeader(cbor, 3, length)) { return false; }

    if (cbor->length - cbor->offset < length) {
        cbor->error = FfxDataErrorBufferOverrun;
        return false;
    }

    memmove(&cbor->data[cbor->offset], str, length);
    cbor->offset += length;

    return true;
}

bool ffx_cbor_appendArray(FfxCborBuilder *cbor, size_t count) {
    return _appendHeader(cbor, 4, count);
}

bool ffx_cbor_appendMap(FfxCborBuilder *cbor, size_t count) {
    return _appendHeader(cbor, 5, count);
}

FfxCborTag ffx_cbor_appendArrayMutable(FfxCborBuilder *cbor) {
    if (cbor->error) { return 0; }

    if (cbor->length < cbor->offset + 3) { // @TODO: Should we alloc 4 bytes?
        cbor->error = FfxDataErrorBufferOverrun;
        return 0;
    }

    cbor->data[cbor->offset++] = (4 << 5) | 25;

    FfxCborTag tag = cbor->offset;

    cbor->data[cbor->offset++] = 0;
    cbor->data[cbor->offset++] = 0;

    return tag;
}

FfxCborTag ffx_cbor_appendMapMutable(FfxCborBuilder *cbor) {
    if (cbor->error) { return 0; }

    if (cbor->length < cbor->offset + 3) {  // @TODO: Use 4 bytes?
        cbor->error = FfxDataErrorBufferOverrun;
        return 0;
    }

    cbor->data[cbor->offset++] = (5 << 5) | 25;

    FfxCborTag tag = cbor->offset;

    cbor->data[cbor->offset++] = 0;
    cbor->data[cbor->offset++] = 0;

    return tag;
}

void ffx_cbor_adjustCount(FfxCborBuilder *cbor, FfxCborTag tag, size_t count) {
    if (cbor->error) { return; }

    cbor->data[tag++] = (count >> 16) & 0xff;
    cbor->data[tag++] = count & 0xff;
}

bool ffx_cbor_appendCborRaw(FfxCborBuilder *cbor, uint8_t *data,
  size_t length) {
    if (cbor->error) { return 0; }

    if (cbor->length < cbor->offset + length) {
        cbor->error = FfxDataErrorBufferOverrun;
        return false;
    }

    size_t offset = cbor->offset;
    memmove(&cbor->data[offset], data, length);
    cbor->offset = offset + length;

    return FfxDataErrorNone;
}

bool ffx_cbor_appendCborBuilder(FfxCborBuilder *dst, FfxCborBuilder *src) {
    return ffx_cbor_appendCborRaw(dst, src->data, src->offset);
}
