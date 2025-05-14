#include <string.h>

#include "firefly-db.h"

#include "db-networks.h"

const char* ffx_db_getNetworkNameU32(uint32_t chainId) {
    size_t count = _ffx_db_networkCount;
    for (int i = 0; i < count; i++) {
        uint32_t id = _ffx_db_networkIndex[2 * i];
        if (id == chainId) {
            return &_ffx_db_networkStrings[_ffx_db_networkIndex[2 * i + 1]];
        }
    }
    return NULL;
}

const char* ffx_db_getNetworkName(FfxBigInt *chainId) {
    size_t count = _ffx_db_networkCount;
    for (int i = 0; i < count; i++) {
        uint32_t id = _ffx_db_networkIndex[2 * i];
        if (ffx_bigint_cmpU32(chainId, id) == 0) {
            return &_ffx_db_networkStrings[_ffx_db_networkIndex[2 * i + 1]];
        }
    }
    return NULL;
}

const char* ffx_db_getNetworkToken(FfxBigInt *chainId) {
    const char *result = ffx_db_getNetworkName(chainId);
    if (result == NULL) { return NULL; }
    return &result[strlen(result) + 1];
}

/*
const char* ffx_db_getNetwork(FfxBigInt *chainId) {
    if (ffx_bigint_cmpU32(chainId, 0x7fffffff) >= 0) {
        return NULL;
    }
    return ffx_db_getNetworkU32(ffx_bigint_getU32(chainId).value);
}
*/
