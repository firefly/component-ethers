#!/bin/bash

gcc \
  -I../include -I../third-party/bitcoin-core-secp256k1/include \
  -DECMULT_WINDOW_SIZE=2 -DCOMB_BLOCKS=2 -DCOMB_TEETH=5 \
  -DENABLE_MODULE_ELLSWIFT=0 -DENABLE_MODULE_MUSIG=0 \
  -DENABLE_MODULE_SCHNORRSIG=0 -DENABLE_MODULE_EXTRAKEYS=0 \
  -DENABLE_MODULE_ECDH=0 \
  -DENABLE_MODULE_RECOVERY=1 \
  test.c \
  ../src/*.c \
  ../third-party/bitcoin-core-secp256k1/src/secp256k1.c \
  ../third-party/bitcoin-core-secp256k1/src/precomputed_ecmult.c \
  ../third-party/bitcoin-core-secp256k1/src/precomputed_ecmult_gen.c \
  && ./a.out


