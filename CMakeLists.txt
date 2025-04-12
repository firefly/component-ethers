cmake_minimum_required(VERSION 3.16)

idf_component_register(
  SRCS
    "src/address.c"
    "src/bip32.c"
    "src/cbor.c"
    "src/ecc.c"
    "src/hmac.c"
    "src/keccak.c"
    "src/rlp.c"
    "src/sha2.c"
    "src/tx.c"

  INCLUDE_DIRS
    "include"
)
