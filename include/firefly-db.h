#ifndef __FIREFLY_DB_H__
#define __FIREFLY_DB_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "firefly-bigint.h"


const char* ffx_db_getNetworkName(FfxBigInt *chainId);
const char* ffx_db_getNetworkToken(FfxBigInt *chainId);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FIREFLY_DB_H__ */
