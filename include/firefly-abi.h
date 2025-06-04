#ifndef __FIREFLY_ABI_H__
#define __FIREFLY_ABI_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "firefly-cbor.h"
#include "firefly-data.h"

/**
 *  Notes
 *
 *  - CBOR encodes values less than 24 within the header itself as a
 *    short count, so common types are kept under 24.
 *  - All types (and therefore base types) fit within 7 bits, with the
 *    top 2 bits indicating the base type and the bottom 5 bits indicating
 *    the size (in bytes), except when the first 2 bits are 0, which
 *    indicate special types
 */
typedef enum FfxAbiBaseType {
    FfxAbiBaseTypeError         = 0,

    FfxAbiBaseTypeAddress       = 0b00001,
    FfxAbiBaseTypeBoolean       = 0b00010,

    FfxAbiBaseTypeBytes         = (1 << 5),
    FfxAbiBaseTypeInt           = (2 << 5),
    FfxAbiBaseTypeUint          = (3 << 5),

    FfxAbiBaseTypeByteString    = 0b01000,
    FfxAbiBaseTypeString        = 0b01001,

    FfxAbiBaseTypeArray         = 0b01100,
    FfxAbiBaseTypeTuple         = 0b01101,
} FfxAbiBaseType;


typedef enum FfxAbiType {
    FfxAbiTypeError         = 0,

    FfxAbiTypeAddress       = FfxAbiBaseTypeAddress,
    FfxAbiTypeBoolean       = FfxAbiBaseTypeBoolean,

    // These are common types worth keeping within the short count
    // and that ensure all sizes fit within the 5 bits (sans substraction)
    FfxAbiTypeBytes32       = 0b00100,
    FfxAbiTypeInt256        = 0b00101,
    FfxAbiTypeUint256       = 0b00110,

    FfxAbiTypeByteString    = FfxAbiBaseTypeByteString,
    FfxAbiTypeString        = FfxAbiBaseTypeString,

    FfxAbiTypeArray         = FfxAbiBaseTypeArray,
    FfxAbiTypeTuple         = FfxAbiBaseTypeTuple,

    FfxAbiTypeBytes1        = (FfxAbiBaseTypeBytes | 1),
    FfxAbiTypeBytes2        = (FfxAbiBaseTypeBytes | 2),
    FfxAbiTypeBytes3        = (FfxAbiBaseTypeBytes | 3),
    FfxAbiTypeBytes4        = (FfxAbiBaseTypeBytes | 4),
    FfxAbiTypeBytes5        = (FfxAbiBaseTypeBytes | 5),
    FfxAbiTypeBytes6        = (FfxAbiBaseTypeBytes | 6),
    FfxAbiTypeBytes7        = (FfxAbiBaseTypeBytes | 7),
    FfxAbiTypeBytes8        = (FfxAbiBaseTypeBytes | 8),
    FfxAbiTypeBytes9        = (FfxAbiBaseTypeBytes | 9),
    FfxAbiTypeBytes10       = (FfxAbiBaseTypeBytes | 10),
    FfxAbiTypeBytes11       = (FfxAbiBaseTypeBytes | 11),
    FfxAbiTypeBytes12       = (FfxAbiBaseTypeBytes | 12),
    FfxAbiTypeBytes13       = (FfxAbiBaseTypeBytes | 13),
    FfxAbiTypeBytes14       = (FfxAbiBaseTypeBytes | 14),
    FfxAbiTypeBytes15       = (FfxAbiBaseTypeBytes | 15),
    FfxAbiTypeBytes16       = (FfxAbiBaseTypeBytes | 16),
    FfxAbiTypeBytes17       = (FfxAbiBaseTypeBytes | 17),
    FfxAbiTypeBytes18       = (FfxAbiBaseTypeBytes | 18),
    FfxAbiTypeBytes19       = (FfxAbiBaseTypeBytes | 19),
    FfxAbiTypeBytes20       = (FfxAbiBaseTypeBytes | 20),
    FfxAbiTypeBytes21       = (FfxAbiBaseTypeBytes | 21),
    FfxAbiTypeBytes22       = (FfxAbiBaseTypeBytes | 22),
    FfxAbiTypeBytes23       = (FfxAbiBaseTypeBytes | 23),
    FfxAbiTypeBytes24       = (FfxAbiBaseTypeBytes | 24),
    FfxAbiTypeBytes25       = (FfxAbiBaseTypeBytes | 25),
    FfxAbiTypeBytes26       = (FfxAbiBaseTypeBytes | 26),
    FfxAbiTypeBytes27       = (FfxAbiBaseTypeBytes | 27),
    FfxAbiTypeBytes28       = (FfxAbiBaseTypeBytes | 28),
    FfxAbiTypeBytes29       = (FfxAbiBaseTypeBytes | 29),
    FfxAbiTypeBytes30       = (FfxAbiBaseTypeBytes | 30),
    FfxAbiTypeBytes31       = (FfxAbiBaseTypeBytes | 31),

    FfxAbiTypeInt8          = (FfxAbiBaseTypeInt | 1),
    FfxAbiTypeInt16         = (FfxAbiBaseTypeInt | 2),
    FfxAbiTypeInt24         = (FfxAbiBaseTypeInt | 3),
    FfxAbiTypeInt32         = (FfxAbiBaseTypeInt | 4),
    FfxAbiTypeInt40         = (FfxAbiBaseTypeInt | 5),
    FfxAbiTypeInt48         = (FfxAbiBaseTypeInt | 6),
    FfxAbiTypeInt56         = (FfxAbiBaseTypeInt | 7),
    FfxAbiTypeInt64         = (FfxAbiBaseTypeInt | 8),
    FfxAbiTypeInt72         = (FfxAbiBaseTypeInt | 9),
    FfxAbiTypeInt80         = (FfxAbiBaseTypeInt | 10),
    FfxAbiTypeInt88         = (FfxAbiBaseTypeInt | 11),
    FfxAbiTypeInt96         = (FfxAbiBaseTypeInt | 12),
    FfxAbiTypeInt104        = (FfxAbiBaseTypeInt | 13),
    FfxAbiTypeInt112        = (FfxAbiBaseTypeInt | 14),
    FfxAbiTypeInt120        = (FfxAbiBaseTypeInt | 15),
    FfxAbiTypeInt128        = (FfxAbiBaseTypeInt | 16),
    FfxAbiTypeInt136        = (FfxAbiBaseTypeInt | 17),
    FfxAbiTypeInt144        = (FfxAbiBaseTypeInt | 18),
    FfxAbiTypeInt152        = (FfxAbiBaseTypeInt | 19),
    FfxAbiTypeInt160        = (FfxAbiBaseTypeInt | 20),
    FfxAbiTypeInt168        = (FfxAbiBaseTypeInt | 21),
    FfxAbiTypeInt176        = (FfxAbiBaseTypeInt | 22),
    FfxAbiTypeInt184        = (FfxAbiBaseTypeInt | 23),
    FfxAbiTypeInt192        = (FfxAbiBaseTypeInt | 24),
    FfxAbiTypeInt200        = (FfxAbiBaseTypeInt | 25),
    FfxAbiTypeInt208        = (FfxAbiBaseTypeInt | 26),
    FfxAbiTypeInt216        = (FfxAbiBaseTypeInt | 27),
    FfxAbiTypeInt224        = (FfxAbiBaseTypeInt | 28),
    FfxAbiTypeInt232        = (FfxAbiBaseTypeInt | 29),
    FfxAbiTypeInt240        = (FfxAbiBaseTypeInt | 30),
    FfxAbiTypeInt248        = (FfxAbiBaseTypeInt | 31),

    FfxAbiTypeUint8         = (FfxAbiBaseTypeUint | 1),
    FfxAbiTypeUint16        = (FfxAbiBaseTypeUint | 2),
    FfxAbiTypeUint24        = (FfxAbiBaseTypeUint | 3),
    FfxAbiTypeUint32        = (FfxAbiBaseTypeUint | 4),
    FfxAbiTypeUint40        = (FfxAbiBaseTypeUint | 5),
    FfxAbiTypeUint48        = (FfxAbiBaseTypeUint | 6),
    FfxAbiTypeUint56        = (FfxAbiBaseTypeUint | 7),
    FfxAbiTypeUint64        = (FfxAbiBaseTypeUint | 8),
    FfxAbiTypeUint72        = (FfxAbiBaseTypeUint | 9),
    FfxAbiTypeUint80        = (FfxAbiBaseTypeUint | 10),
    FfxAbiTypeUint88        = (FfxAbiBaseTypeUint | 11),
    FfxAbiTypeUint96        = (FfxAbiBaseTypeUint | 12),
    FfxAbiTypeUint104       = (FfxAbiBaseTypeUint | 13),
    FfxAbiTypeUint112       = (FfxAbiBaseTypeUint | 14),
    FfxAbiTypeUint120       = (FfxAbiBaseTypeUint | 15),
    FfxAbiTypeUint128       = (FfxAbiBaseTypeUint | 16),
    FfxAbiTypeUint136       = (FfxAbiBaseTypeUint | 17),
    FfxAbiTypeUint144       = (FfxAbiBaseTypeUint | 18),
    FfxAbiTypeUint152       = (FfxAbiBaseTypeUint | 19),
    FfxAbiTypeUint160       = (FfxAbiBaseTypeUint | 20),
    FfxAbiTypeUint168       = (FfxAbiBaseTypeUint | 21),
    FfxAbiTypeUint176       = (FfxAbiBaseTypeUint | 22),
    FfxAbiTypeUint184       = (FfxAbiBaseTypeUint | 23),
    FfxAbiTypeUint192       = (FfxAbiBaseTypeUint | 24),
    FfxAbiTypeUint200       = (FfxAbiBaseTypeUint | 25),
    FfxAbiTypeUint208       = (FfxAbiBaseTypeUint | 26),
    FfxAbiTypeUint216       = (FfxAbiBaseTypeUint | 27),
    FfxAbiTypeUint224       = (FfxAbiBaseTypeUint | 28),
    FfxAbiTypeUint232       = (FfxAbiBaseTypeUint | 29),
    FfxAbiTypeUint240       = (FfxAbiBaseTypeUint | 30),
    FfxAbiTypeUint248       = (FfxAbiBaseTypeUint | 31),
} FfxAbiType;

typedef enum FfxParseError {
    FfxParseErrorNone = 0,

    FfxParseErrorBadChar,
    FfxParseErrorExpectString,
    FfxParseErrorExpectNumber,

    FfxParseErrorUnknownType,
} FfxParseError;

typedef struct FfxParseResult {
    FfxParseError error;
    size_t offset;
} FfxParseResult;

/**
 *  Returns the base type.
 */
FfxAbiBaseType ffx_abiType_getBaseType(FfxAbiType type);

/**
 *  Returns the number of bytes for a Bytes, Int or Uint.
 */
uint8_t ffx_abiType_getSize(FfxAbiType type);



/**
 *  ABI CBOR Format
 *
 *  e.g. transfer(address to, uint256 amount)
 *  {
 *    v: 1,
 *    name: "transfer",
 *    params: [
 *      [
 *        ((type << 8) | size),
 *        name?,
 *        components?: [ ]
 *      ]
 *    ]
 *  }
 */

typedef struct FfxAbiCursor {
    FfxCborCursor abi;

    uint8_t *data;
    size_t offset, length;

    FfxDataError error;
} FfxAbiCursor;

//typedef struct FfxAbiIterator {
//} FfxAbiIterator;

FfxAbiCursor ffx_abi_walk(FfxCborCursor abi, uint8_t *data, size_t length);

FfxDataError ffx_abi_validate(FfxAbiCursor cursor);

uint32_t ffx_abi_getSelector(FfxAbiCursor abi);
FfxDataResult ffx_abi_getFunctionName(FfxAbiCursor abi);

FfxAbiType ffx_abi_getType(FfxAbiCursor cursor);

FfxDataResult ffx_abi_getData(FfxAbiCursor cursor);
FfxSizeResult ffx_abi_getDataLength(FfxAbiCursor cursor);

FfxSizeResult ffx_abi_getContainerCount(FfxAbiCursor cursor);


FfxAbiCursor ffx_abi_followKey(FfxAbiCursor cursor, const char *key);
FfxAbiCursor ffx_abi_followIndex(FfxAbiCursor cursor, size_t index);


//FfxAbiIterator ffx_abi_iterate(FfxAbiCursor container);
//bool ffx_abi_nextChild(FfxAbiIterator *iterator);


void ffx_abi_dump(FfxAbiCursor cursor);


FfxParseResult ffx_abi_parseSignature(uint8_t *abiOut, size_t abiOutLength,
  const char* sig);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FIREFLY_ABI_H__ */
