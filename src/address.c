#include <string.h>

#include "firefly-address.h"
#include "firefly-hash.h"


void ffx_eth_checksumAddress(char *checksumOut, const uint8_t *address) {

    // Add the "0x" prefix and advance the pointer (so we can ignore it)
    checksumOut[0] = '0';
    checksumOut[1] = 'x';
    checksumOut += 2;

    // Place the ASCII representation of the address in checksumed
    const char * const HexNibbles = "0123456789abcdef";
    int offset = 0;
    for (int i = 0; i < 20; i++) {
        checksumOut[offset++] = HexNibbles[address[i] >> 4];
        checksumOut[offset++] = HexNibbles[address[i] & 0xf];
    }

    // Hash the ASCII representation
    uint8_t digest[FFX_KECCAK256_DIGEST_LENGTH] = { 0 };
    ffx_hash_keccak256(digest, (const uint8_t*)checksumOut, 40);

    // Uppercase any (alpha) nibble if the coresponding hash nibble >= 8
    for (int i = 0; i < 40; i += 2) {
        uint8_t c = digest[i >> 1];

        if (checksumOut[i] >= 'a' && ((c >> 4) >= 8)) {
            checksumOut[i] -= 0x20;
        }

        if (checksumOut[i + 1] >= 'a' && ((c & 0x0f) >= 8)) {
            checksumOut[i + 1] -= 0x20;
        }
    }
}

void ffx_eth_computeAddress(uint8_t *addressOut, const uint8_t *pubkey) {
    uint8_t hashed[32];
    ffx_hash_keccak256(hashed, &pubkey[1], 64);

    memcpy(addressOut, &hashed[12], 20);
}
