#ifndef __FIREFLY_TRANSACTION_H__
#define __FIREFLY_TRANSACTION_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "firefly-cbor.h"


typedef enum FfxTxStatus {
    FfxTxStatusOK                    = 0,
    FfxTxStatusBufferOverrun,
    FfxTxStatusBadData,
    FfxTxStatusOverflow,  // @TODO: Bad data?
    FfxTxStatusUnsupportedType,
} FfxTxStatus;

/*

typedef struct FfxTx {
    FfxTxData rlp;
    FfxTxStatus status;

    uint8_t type;
    uint32_t nonce;

    uint32_t gasLimit

    // NULL if no to address, otherwise 20 bytes
    uint8_t *to;

    FfxTxField data;
    FfxTxField value;
} FfxTx;
*/

// @TODO: Make this shared amoungst API
typedef struct FfxTxData {
    uint8_t *bytes;
    size_t length;
    //status OK, BadData, Truncated, Overrun, Overflow
} FfxTxData;

typedef struct FfxTx {
    FfxTxData rlp;
    FfxTxStatus status;
    uint8_t type;
} FfxTx;

FfxTx ffx_tx_init(uint8_t *rlp, size_t length);

FfxTx ffx_tx_serializeUnsigned(FfxCborCursor *tx, uint8_t *data,
  size_t length);



//FfxTxStatus ffx_tx_sign(uint8_t *data, size_t *length, FfxCborCursor *tx,
//  uint8_t *privkey);

//FfxTxStatus ffx_tx_serializeSigned(FfxCborCursor *tx, uint8_t *signature,);

uint8_t* ffx_tx_getAddress(FfxTx *tx);
uint8_t* ffx_tx_getData(FfxTx *tx);
uint8_t* ffx_tx_getValue(FfxTx *tx);
bool ffx_tx_isSigned(FfxTx *tx);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FIREFLY_TRANSACTION_H__ */
