#ifndef __DB_NETWORKS_H__
#define __DB_NETWORKS_H__

// This file is generated! Do NOT modify manually. See export-db-networks.mjs.

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stddef.h>
#include <stdint.h>

const char _ffx_db_networkStrings[] =
    "mainnet\0ETH\0"           // 0x00000000  Chain ID: 1
    "Optimism\0ETH\0"          // 0x0000000c  Chain ID: 10
    "Polygon\0POL\0"           // 0x00000019  Chain ID: 137
    "Base\0ETH\0"              // 0x00000025  Chain ID: 8453
    "Arbitrum\0ETH\0"          // 0x0000002e  Chain ID: 42161
    "Linea\0ETH\0"             // 0x0000003b  Chain ID: 59144
    "Sepolia\0sETH\0"          // 0x00000045  Chain ID: 11155111
;

const size_t _ffx_db_networkCount = 14;

const uint32_t _ffx_db_networkIndex[] = {
    0x00000001, 0x00000000, 0x0000000a, 0x0000000c, 0x00000089, 0x00000019,
    0x00002105, 0x00000025, 0x0000a4b1, 0x0000002e, 0x0000e708, 0x0000003b,
    0x00aa36a7, 0x00000045,
};

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __DB_NETWORKS_H__ */