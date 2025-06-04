#ifndef __FIREFLY_ADDRESS_H__
#define __FIREFLY_ADDRESS_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "firefly-ecc.h"


typedef struct FfxAddress {
    uint8_t data[20];
} FfxAddress;

typedef struct FfxChecksumAddress {
    // '0' 'x' [ 40 case-sensitive nibbles ] '\0'
    char text[43];
} FfxChecksumAddress;


/**
 *  Returns the EIP-155 %%checksumed%% address of %%address%.
 */
FfxChecksumAddress ffx_eth_checksumAddress(const FfxAddress *address);


/**
 *  Returns the address bytes for %%pubkey%%.
 */
FfxAddress ffx_eth_getAddress(const FfxEcPubkey *pubkey);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FIREFLY_ADDRESS_H__ */
