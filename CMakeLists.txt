cmake_minimum_required(VERSION 3.16)

idf_component_register(
  SRCS
    "src/address.c"
    "src/bigint.c"
    "src/bip32.c"
    "src/cbor.c"
    "src/db.c"
    "src/decimal.c"
    "src/ecc.c"
    "src/hmac.c"
    "src/keccak.c"
    "src/rlp.c"
    "src/pbkdf2.c"
    "src/sha2.c"
    "src/tx.c"

    "third-party/bitcoin-core-secp256k1/src/secp256k1.c"
    "third-party/bitcoin-core-secp256k1/src/precomputed_ecmult.c"
    "third-party/bitcoin-core-secp256k1/src/precomputed_ecmult_gen.c"

  INCLUDE_DIRS
    "include"
    "third-party/bitcoin-core-secp256k1/include"
)

target_compile_definitions(${COMPONENT_LIB} PRIVATE
  ECMULT_WINDOW_SIZE=2
  COMB_BLOCKS=2
  COMB_TEETH=5
  ENABLE_MODULE_ELLSWIFT=0
  ENABLE_MODULE_MUSIG=0
  ENABLE_MODULE_SCHNORRSIG=0
  ENABLE_MODULE_EXTRAKEYS=0
  ENABLE_MODULE_ECDH=0
  ENABLE_MODULE_RECOVERY=1
)

