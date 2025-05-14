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


FfxDataResult ffx_tx_init(uint8_t *rlp, size_t length);

FfxDataResult ffx_tx_serializeUnsigned(FfxCborCursor tx, uint8_t *data,
  size_t length);


//FfxDataResult ffx_tx_serializeSigned(FfxCborCursor *tx, uint8_t *signature);


uint8_t ffx_tx_getType(FfxDataResult tx);

/**
 *  Returns the chain ID of the %%tx%%.
 */
FfxDataResult ffx_tx_getChainId(FfxDataResult tx);

/**
 *  Returns the address of the %%tx%%. If the transaction type allows for
 *  NULL address, the result length will be 0.
 */
FfxDataResult ffx_tx_getAddress(FfxDataResult tx);

/**
 *  Returns the data of the %%tx%%.
 */
FfxDataResult ffx_tx_getData(FfxDataResult tx);

/**
 *  Returns the value of the %%tx%%.
 */
FfxDataResult ffx_tx_getValue(FfxDataResult tx);

/**
 *  Returns true if the transaction has a signature.
 */
bool ffx_tx_isSigned(FfxDataResult tx);

/**
 *  Dump the %%tx%% to the console.
 */
void ffx_tx_dump(FfxDataResult tx);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FIREFLY_TRANSACTION_H__ */
