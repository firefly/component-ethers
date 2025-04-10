#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "./cbor.h"

void print(CborCursor *cursor) {
    switch(cbor_getType(cursor)) {
        case CborTypeError:
            printf("CBOR: Error\n");
            break;
        case CborTypeNull:
            printf("CBOR: null\n");
            break;
        case CborTypeBoolean: {
            uint64_t value = 0;
            CborStatus status = cbor_getValue(cursor, &value);
            printf("CBOR[%d]: boolean v=%s\n", status, value ? "true": "false");
            break;
        }
        case CborTypeNumber: {
            uint64_t value = 0;
            CborStatus status = cbor_getValue(cursor, &value);
            printf("CBOR[%d]: number v=%lld\n", status, value);
            break;
        }
        case CborTypeData: {
            size_t length = 0;
            CborStatus status = cbor_getLength(cursor, &length);
            if (status) {
                printf("CBOR[%d]: data (invalid)\n", status);
            } else {
                uint8_t data[length + 1];
                memset(data, 0, length + 1);
                status = cbor_getData(cursor, data, sizeof(data));
                printf("CBOR[%d]: data len=%zu v=%s\n", status, length,
                  (char*)data);
            }
            break;
        }
        case CborTypeArray: {
            size_t length = 0;
            CborStatus status = cbor_getLength(cursor, &length);
            printf("CBOR[%d]: Array len=%zu\n", status, length);
            break;
        }
        case CborTypeMap: {
            size_t length = 0;
            CborStatus status = cbor_getLength(cursor, &length);
            printf("CBOR[%d]: Map len=%zu\n", status, length);
            break;
        }
    }
}

int main() {
    //uint8_t data[] = { ((0 << 5) | (25)), 0x16, 0x16 };
//      0x65, 0x68, 0x65, 0x6c, 0x6c, 0x6f

    uint8_t data[] = {
        //163, 101, 104, 101, 108, 108, 111, 24, 123, 100, 116, 101, 115, 116, 161, 99, 102, 111, 111, 99, 98, 97, 114, 101, 101, 109, 112, 116, 121, 246
        134, 1, 2, 3, 132, 99, 102, 111, 111, 99, 98, 97, 114, 161, 100, 116, 101, 115, 116, 245, 131, 24, 100, 24, 101, 24, 102, 5, 6
    };

    CborCursor cursor = { 0 };
    cbor_init(&cursor, data, sizeof(data));

    while (!cbor_isDone(&cursor)) {
        print(&cursor);
        CborStatus status = _cbor_next(&cursor);
        if (status) {
            printf("STATUS=%d\n", status);
            break;
        }
    }

    printf("====================\n");

    // [ 1, 2, 3, [ "foo", "bar", { test: true }, [ 100, 101, 102 ] ], 5, 6 ]
    cbor_init(&cursor, data, sizeof(data));

    uint64_t value;
    cbor_firstValue(&cursor, NULL);
    print(&cursor);
    cbor_nextValue(&cursor, NULL);
    print(&cursor);
    cbor_nextValue(&cursor, NULL);
    print(&cursor);
    cbor_nextValue(&cursor, NULL);
    print(&cursor);

    CborCursor inner;
    cbor_clone(&inner, &cursor);
    cbor_firstValue(&inner, NULL);
    print(&inner);
    cbor_nextValue(&inner, NULL);
    print(&inner);
    cbor_nextValue(&inner, NULL);
    print(&inner);

    CborCursor inner2, key;
    cbor_clone(&inner2, &inner);
    cbor_firstValue(&inner2, &key);
    print(&key);
    print(&inner2);

    cbor_nextValue(&inner, NULL);
    print(&inner);

    cbor_nextValue(&cursor, NULL);
    print(&cursor);

    cbor_nextValue(&cursor, NULL);
    print(&cursor);

    printf("BAR\n");

    cbor_init(&cursor, data, sizeof(data));
    cbor_followIndex(&cursor, 3);
    cbor_followIndex(&cursor, 2);
    CborStatus s = cbor_followKey(&cursor, "test");
    print(&cursor);
/*
    uint64_t value = 0;
    cbor_getValue(&cursor, &value);

    printf("type: %d\n", cbor_getType(&cursor));
    size_t count = 0;
    cbor_getLength(&cursor, &count);
    printf("length: %zu\n", count);

    uint8_t output[10] = { 0 };
    CborStatus status = cbor_getData(&cursor, output, 3);
    printf("str: %s e=%d\n", (char*)output, status);
*/
}

