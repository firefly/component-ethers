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

  INCLUDE_DIRS
    "include"
)
