/**
 *  Overview of the encoding:
 *
 *  The structured data is comprised of items, where each item is either:
 *    - Data (a series of bytes)
 *    - Array of items (either Data or Arrays; allowing arbitrary nesting)
 *
 *  Each item is a header, followed by its payload bytes.
 *
 *  The header starts with a single byte, depending on the top bits:
 *    - `0b0xxx xxxx`: a 1-byte Data with that value as its value
 *    - `0b10xx xxxx`: a Data with the X (see below) bytes as its value
 *    - `0b11xx xxxx`: an Array with X bytes of its children concatenated
 *
 *  The header length X, is based on the bottom 6-bits. If the value is
 *  55 or fewer, then that is X. Otherwise, consume that number of bytes
 *  as a big-endian represention of X.
 *
 *  The X MUST be **minimally** encoded.
 *
 *  Some interesting notes:
 *    - a zero-byte Data is 0b01000000 which is still a single byte
 *    - a zero-length Array is 0b10000000 which is a single byte
 *    - X MUST be minimally encoed, w; a length Xof 32 must be encoded using
 *      0x20, not 0x0020
 *
 *  Implementation:
 *    On the initial build:
 *      - Data is correctly RLP encoded (i.e. compact)
 *      - Zero-length Arrays are correctly RLP encoded (i.e. compact)
 *      - Arrays are encoded reserving 4 bytes for length, but store
 *        the count of children instead of the number of child bytes 
 *    On finalize, the structure is recursively travsersed, compacting
 *    any non-compact Array using the child count to determine total
 *    length and shifting the entire encoded data past the array left
 *    as needed (since 4 bytes were reserved and payloads larger than
 *    that are not supported, we always compact some amount).
 */

#include <string.h>

#include "firefly-rlp.h"

// DEBUG
#include <stdio.h>

#define TAG_ARRAY         (0xc0)
#define TAG_DATA          (0x80)

// This mask can be used to check the value type
#define TAG_MASK          (0xc0)

// This is not stored, just used as a hint to appendHeader that
// a non-compact 4-byte size should be used regardless
#define TAG_RESERVE       (0x00)

#define MAX_LENGTH      (0xffffff)
#define FIRST_ITER      (0xf000000)

/**
 *  RLP
 *
 *  If the top two bits are set, then it is an Array. If the top
 *  bit is set, then it is a data.
 *  If the bottom 6 bits are less than 55, that is the length in
 *  bytes. Otherwise, that is the number of bytes to additionally
 *  consume to get the length.
 */

///////////////////////////////
// Utils

static size_t readValue(const uint8_t *data, size_t count) {
    size_t v = 0;
    for (int i = 0; i < count; i++) {
        v <<= 8;
        v |= data[i];
    }
    return v;
}

typedef struct Header {
    FfxRlpType type;

    const uint8_t *data;
    size_t length;

    size_t headerSize;

    FfxDataError error;
} Header;

static Header getHeader(const uint8_t *data, size_t length, size_t offset) {
    if (offset >= length) {
        return (Header){ .error = FfxDataErrorBufferOverrun };
    }

    uint32_t byte = data[offset];

    // Single byte of data
    if ((byte & 0x80) == 0) {
        return (Header){
            .type = FfxRlpTypeData,
            .data = &data[offset],
            .length = 1,
            .headerSize = 0
        };
    }

    Header header = {
        .type = ((byte & 0xc0) == TAG_DATA) ? FfxRlpTypeData: FfxRlpTypeArray,
        .headerSize = 1
    };

    uint32_t v = (byte & 0x3f);
    if (v > 55) {
        v -= 55;

        // Overflow!
        if (v > 4) { return (Header){ .error = FfxDataErrorOverflow }; }

        // Buffer Overrun
        if (offset + 1 + v > length) {
            return (Header){ .error = FfxDataErrorBufferOverrun };
        }

        header.headerSize = 1 + v;
        v = readValue(&data[offset + 1], v);
    }

    if (v > MAX_LENGTH) { return (Header){ .error = FfxDataErrorOverflow }; }

    header.data = &data[offset + header.headerSize];
    header.length = v;

    if (offset + header.headerSize + header.length > length) {
        return (Header){ .error = FfxDataErrorBufferOverrun };
    }

    return header;
}

///////////////////////////////
// Walking

FfxRlpCursor ffx_rlp_walk(const uint8_t *data, size_t length) {
    return (FfxRlpCursor){ .data = data, .length = length };
}

FfxRlpType ffx_rlp_getType(FfxRlpCursor cursor) {
    if (cursor.error) { return FfxRlpTypeError; }
    Header header = getHeader(cursor.data, cursor.length, cursor.offset);
    if (header.error) { return FfxRlpTypeError; }
    return header.type;
}

FfxSizeResult ffx_rlp_getDataLength(FfxRlpCursor cursor) {
    if (cursor.error) { return (FfxSizeResult){ .error = cursor.error }; }

    Header header = getHeader(cursor.data, cursor.length, cursor.offset);
    if (header.error) { return (FfxSizeResult){ .error = cursor.error }; }

    if (header.type != FfxRlpTypeData) {
        return (FfxSizeResult){ .error = FfxDataErrorInvalidOperation };
    }

    return (FfxSizeResult){ .value = header.length };
}

FfxSizeResult ffx_rlp_getArrayCount(FfxRlpCursor cursor) {
    if (cursor.error) { return (FfxSizeResult){ .error = cursor.error }; }

    Header header = getHeader(cursor.data, cursor.length, cursor.offset);
    if (header.error) { return (FfxSizeResult){ .error = cursor.error }; }

    if (header.type != FfxRlpTypeArray) {
        return (FfxSizeResult){ .error = FfxDataErrorInvalidOperation };
    }

    size_t offset = cursor.offset + header.headerSize;
    size_t end = offset + header.length;

    size_t count = 0;
    while (offset < end) {
        // @TODO: Validate somewhere
        Header h = getHeader(cursor.data, cursor.length, offset);
        if (h.error) { return (FfxSizeResult){ .error = h.error }; }
        offset += h.headerSize + h.length;
        count++;
    }

    if (offset != end) {
        return (FfxSizeResult){ .error = FfxDataErrorBadData };
    }

    return (FfxSizeResult){ .value = count };
}


FfxRlpCursor ffx_rlp_followIndex(FfxRlpCursor cursor, size_t index) {
    FfxRlpIterator iter = ffx_rlp_iterate(cursor);
    if (iter.error) { return (FfxRlpCursor){ .error = iter.error }; }

    size_t count = 0;
    while (ffx_rlp_nextChild(&iter)) {
        if (iter.error) { return (FfxRlpCursor){ .error = iter.error }; }
        if (count == index) { return iter.child; }
        count++;
    }

    return (FfxRlpCursor){ .error = FfxDataErrorNotFound };
}

FfxDataResult ffx_rlp_getData(FfxRlpCursor cursor) {
    if (cursor.error) { return (FfxDataResult){ .error = cursor.error }; }

    Header header = getHeader(cursor.data, cursor.length, cursor.offset);
    if (header.error) { return (FfxDataResult){ .error = header.error }; }

    if (header.type != FfxRlpTypeData) {
        return (FfxDataResult){ .error = FfxDataErrorInvalidOperation };
    }

    return (FfxDataResult){
        .bytes = header.data,
        .length = header.length
    };
}

FfxRlpIterator ffx_rlp_iterate(FfxRlpCursor container) {

    if (container.error) {
        return (FfxRlpIterator){ .error = container.error };
    }

    size_t offset = container.offset;

    Header header = getHeader(container.data, container.length, offset);
    if (header.error) { return (FfxRlpIterator){ .error = header.error }; }
    if (header.type != FfxRlpTypeArray) {
        return (FfxRlpIterator){ .error = FfxDataErrorInvalidOperation };
    }

    offset += header.headerSize;

    return (FfxRlpIterator){
        .child = (FfxRlpCursor){
            .data = container.data,
            .length = container.length,
            .offset = FIRST_ITER,
        },
        ._nextOffset = offset,
        ._containerEnd = offset + header.length
    };
}

bool ffx_rlp_nextChild(FfxRlpIterator *iterator) {
    if (iterator->error) { return false; }

    // Done!
    if (iterator->_nextOffset == iterator->_containerEnd) { return false; }

    // The data read past our end; data is corrupt
    if (iterator->_nextOffset > iterator->_containerEnd) {
        iterator->error = FfxDataErrorBadData;
        return false;
    }

    iterator->child.offset = iterator->_nextOffset;

    Header header = getHeader(iterator->child.data, iterator->child.length,
      iterator->child.offset);

    // Some error occured
    if (header.error) {
        iterator->error = header.error;
        return false;
    }

    iterator->_nextOffset += header.headerSize + header.length;

    return true;
}

static void _dump(FfxRlpCursor cursor) {
    FfxRlpType type = ffx_rlp_getType(cursor);

    switch(type) {
        case FfxRlpTypeData: {
            FfxDataResult result = ffx_rlp_getData(cursor);
            if (result.error) {
                printf("<ERROR type=DATA status=%d>", result.error);
                break;
            }

            printf("0x");
            for (int i = 0; i < result.length; i++) {
                printf("%02x", result.bytes[i]);
            }
            break;
        }

        case FfxRlpTypeArray: {
            printf("[ ");

            bool first = true;
            FfxRlpIterator iter = ffx_rlp_iterate(cursor);
            if (iter.error) {
                printf("<ERROR type=ITER_BEGIN status=%d>", iter.error);
                break;
            }

            while (ffx_rlp_nextChild(&iter)) {
                if (!first) { printf(", "); }
                first = false;
                _dump(iter.child);
            }

            if (iter.error) {
                printf("<ERROR type=ITER_END status=%d>", iter.error);
                break;
            }

            if (!first) { printf(" "); }
            printf("]");
            break;
        }

        default:
            printf("<ERROR type=%d>", type);
            return;
    }
}

void ffx_rlp_dump(FfxRlpCursor cursor) {
    _dump(cursor);
    printf("\n");
}

///////////////////////////////
// Building - utils

static size_t getByteCount(size_t value) {
    if (value < 0x100) { return 1; }
    if (value < 0x10000) { return 2; }
    if (value < 0x1000000) { return 3; }
    return 4;
}

static bool appendByte(FfxRlpBuilder *rlp, uint8_t byte) {

    if (rlp->error) { return false; }

    if (rlp->length < rlp->offset + 1) {
        rlp->error = FfxDataErrorBufferOverrun;
        return false;
    }

    rlp->data[rlp->offset++] = byte;
    return true;
}

static bool appendBytes(FfxRlpBuilder *rlp, const uint8_t *data,
  size_t length) {

    if (rlp->error) { return false; }

    if (rlp->length < rlp->offset + length) {
        rlp->error = FfxDataErrorBufferOverrun;
        return false;
    }

    memmove(&rlp->data[rlp->offset], data, length);
    rlp->offset += length;
    return true;
}

// The TAG_RESERVE indicates we are leaving space in the Array. It
// currently has the number of items in the array, which will need
// to be swapped out for number of bytes in the final RLP.
static bool appendHeader(FfxRlpBuilder *rlp, uint8_t tag, size_t length) {

    if (rlp->error) { return false; }

    if (tag != TAG_RESERVE && length <= 55) {
        return appendByte(rlp, tag + length);
    }

    size_t byteCount = 0;
    if (tag == TAG_RESERVE) {
        byteCount = 4;
        tag = TAG_ARRAY;
    } else {
        byteCount = getByteCount(length);
    }

    if (!appendByte(rlp, tag + 55 + byteCount)) { return false; }

    for (int i = byteCount - 1; i >= 0; i--) {
        if (!appendByte(rlp, (length >> (8 * i)))) { return false; }
    }

    return true;
}


///////////////////////////////
// Building

FfxRlpBuilder ffx_rlp_build(uint8_t *data, size_t length) {
    return (FfxRlpBuilder){ .data = data, .length = length };
}

bool ffx_rlp_appendData(FfxRlpBuilder *rlp, const uint8_t *data,
  size_t length) {

    if (rlp->error) { return false; }

    if (length == 1 && data[0] <= 127) {
        return appendByte(rlp, data[0]);
    }

    if (!appendHeader(rlp, TAG_DATA, length)) { return false; }

    return appendBytes(rlp, data, length);
}

bool ffx_rlp_appendString(FfxRlpBuilder *rlp, const char *data) {
    return ffx_rlp_appendData(rlp, (uint8_t*)data, strlen(data));
}

bool ffx_rlp_appendArray(FfxRlpBuilder *rlp, size_t count) {
    // Zero-length arrays can be stored directly in their compact
    // representation. Otherwise we reserve 4 bytes where we include
    // the length in items to fix in finalize
    return appendHeader(rlp, count ? TAG_RESERVE: TAG_ARRAY, count);
}

FfxRlpBuilderTag ffx_rlp_appendMutableArray(FfxRlpBuilder *rlp) {

    size_t tag = rlp->offset;
    if (!appendHeader(rlp, TAG_RESERVE, 0)) { return 0; }
    return tag;
}

bool ffx_rlp_adjustCount(FfxRlpBuilder *rlp, FfxRlpBuilderTag tag,
  size_t count) {
    if (tag == 0 || rlp->error) { return false; }

    size_t offset = rlp->offset;
    rlp->offset = tag;
    bool status = appendHeader(rlp, TAG_RESERVE, count);
    rlp->offset = offset;

    return status;
}

static size_t finalize(FfxRlpBuilder *rlp) {
    uint8_t v = rlp->data[rlp->offset];

    if (v <= 127) { return 1; }

    // Data or non-4-byte Array are already compact
    if ((v & TAG_MASK) == TAG_DATA || v != (TAG_ARRAY + 55 + 4)) {
        v &= 0x3f;

        if (v <= 55) { return 1 + v; }
        v -= 55;

        // Overflow!
        if (v > 4) { return 0; }

        return 1 + v + readValue(&rlp->data[rlp->offset + 1], v);
    }

    // The base offset of this array and start of child data
    size_t baseOffset = rlp->offset;
    size_t dataOffset = baseOffset + 5;

    size_t count = readValue(&rlp->data[rlp->offset + 1], 4);
    rlp->offset = dataOffset;
    size_t length = 0;
    for (int i = 0; i < count; i++) {
        size_t l = finalize(rlp);
        if (l == 0) { return 0; }
        length += l;
        rlp->offset = dataOffset + length;
    }

    // Overwrite the 5-byte header with its compact form
    rlp->offset = baseOffset;
    if (!appendHeader(rlp, TAG_ARRAY, length)) { return 0; }

    // Compact the children, shifting the data left.
    if (rlp->offset != dataOffset) {
        memmove(&rlp->data[rlp->offset], &rlp->data[dataOffset],
          rlp->length - dataOffset);
    }

    // The header length and the content length
    return rlp->offset - baseOffset + length;
}

size_t ffx_rlp_finalize(FfxRlpBuilder *rlp) {
    if (rlp->error) { return 0; }

    // Store the non-compact length to minimize compaction memmoves
    rlp->length = rlp->offset;

    // Move to the start of the data
    rlp->offset = 0;

    // Finalize recursively
    size_t length = finalize(rlp);

    rlp->offset = length;
    return length;
}

