#ifndef __FIREFLY_ECC_H__
#define __FIREFLY_ECC_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdbool.h>
#include <stdint.h>


typedef struct FfxEcPubkey {
    uint8_t data[65];
} FfxEcPubkey;

typedef struct FfxEcCompPubkey {
    uint8_t data[33];
} FfxEcCompPubkey;

typedef struct FfxEcPrivkey {
    uint8_t data[32];
} FfxEcPrivkey;

typedef struct FfxEcDigest {
    uint8_t data[32];
} FfxEcDigest;

typedef struct FfxEcSignature {
    uint8_t data[65];
} FfxEcSignature;


void ffx_ec_init(uint8_t *randomize);

bool ffx_ec_getPubkey(FfxEcPubkey *pubkeyOut, const FfxEcPrivkey *privkey);

bool ffx_ec_getCompPubkey(FfxEcCompPubkey *pubkeyOut,
  const FfxEcPrivkey *privkey);

bool ffx_ec_recover(FfxEcPubkey *pubkeyOut, const FfxEcDigest *digest,
  FfxEcSignature *sig);

bool ffx_ec_sign(FfxEcSignature *sigOut, const FfxEcPrivkey *privkey,
  const FfxEcDigest *digest);

bool ffx_ec_compressPubkey(FfxEcCompPubkey *pubkeyOut,
  const FfxEcPubkey *_pubkey);

bool ffx_ec_decompressPubkey(FfxEcPubkey *pubkeyOut,
  const FfxEcCompPubkey *_pubkey);

bool ffx_ec_modAddPrivkey(uint8_t *result32Out, const uint8_t *a32,
  const uint8_t *b32);
bool ffx_ec_modMulPrivkey(uint8_t *result32Out, const uint8_t *a32,
  const uint8_t *b32);
bool ffx_ec_addPointsCompPubkey(uint8_t *result33Out, const uint8_t *a33,
  const uint8_t *b33);

//bool ffx_ec_modAdd(FfxEcPrivkey *resultOut, FfxEcPrivkey *a, FfxEcPrivkey *b);
//bool ffx_ec_modMul(FfxEcPrivkey *resultOut, FfxEcPrivkey *a, FfxEcPrivkey *b);
//bool ffx_ec_addPoints(FfxEcPubkey *resultOut, const FfxEcPubkey *a,
//  const FfxEcPubkey *b);




#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FIREFLY_ECC_H__ */
