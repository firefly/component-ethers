#ifndef __FIREFLY_DB_H__
#define __FIREFLY_DB_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "firefly-bigint.h"

/*
FfxDbNetwork {
    const char* name;

    uint32_t chainIdU32;
    FfxBigInt chainId
} FfxDbNetwork;
*/

//const char* ffx_db_getNetworkName(FfxBigInt *chainId);
//const char* ffx_db_getNetworkNameU32(uint32_t chainId);
const char* ffx_db_getNetworkName(FfxBigInt *chainId);
const char* ffx_db_getNetworkToken(FfxBigInt *chainId);

//FfxDbNetwork ffx_db_getToken(FfxBigInt *chainId, uint8_t *address);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FIREFLY_DB_H__ */
