#ifndef __FIREFLY_TRANSACTION_H__
#define __FIREFLY_TRANSACTION_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "firefly-cbor.h"
#include "firefly-data.h"

// Use FfxDataError?
/*
typedef enum FfxTxStatus {
    FfxTxStatusOK                    = 0,
    FfxTxStatusBufferOverrun,
    FfxTxStatusBadData,
    FfxTxStatusOverflow,  // @TODO: Bad data?
    FfxTxStatusUnsupportedType,
} FfxTxStatus;
*/

typedef struct FfxTx {
    FfxData rlp;
    FfxDataError error;
    uint8_t type;
} FfxTx;

FfxTx ffx_tx_init(uint8_t *rlp, size_t length);

FfxTx ffx_tx_serializeUnsigned(FfxCborCursor tx, uint8_t *data,
  size_t length);


//FfxTxStatus ffx_tx_sign(uint8_t *data, size_t *length, FfxCborCursor *tx,
//  uint8_t *privkey);

//FfxTxStatus ffx_tx_serializeSigned(FfxCborCursor *tx, uint8_t *signature,);

/**
 *  Returns the chain ID of the %%tx%%.
 */
FfxDataResult ffx_tx_getChainId(FfxTx tx);

/**
 *  Returns the address of the %%tx%%. If the transaction type allows for
 *  NULL address, the result length will be 0.
 */
FfxDataResult ffx_tx_getAddress(FfxTx tx);

/**
 *  Returns the data of the %%tx%%.
 */
FfxDataResult ffx_tx_getData(FfxTx tx);

/**
 *  Returns the value of the %%tx%%.
 */
FfxDataResult ffx_tx_getValue(FfxTx tx);

/**
 *  Returns true if the transaction has a signature.
 */
bool ffx_tx_isSigned(FfxTx tx);

/**
 *  Dump the %%tx%% to the console.
 */
void ffx_tx_dump(FfxTx tx);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FIREFLY_TRANSACTION_H__ */
