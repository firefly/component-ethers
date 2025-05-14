Firefly Ethers
==============

Th Ethers.c library (aims to) provide a complete and easy-to-use C API
for interacting with the Ethereum (and ilk) blockchain.

It is used by the [Firefly Hardware Wallet](https://github.com/firefly/).


**Features**

- stack-based; only put on the heap what you want to

API
---

- Address Checksum and calculation
- BigInt
- BIP-32 HD Wallets and BIP39 mnemonic phrases
- CBOR Decoding and Encoding
- Signing and verifying; secp256k1, P-256
- Databases for Networks
- Decimal support for parsing and formatting strings like "1.2" ETH
- Hashing; KECCAK256, SHA-2 and SHA-2-HMAC (256-bit and 512-bit) and PBKDF2
- RLP Decoding and Encoding
- Transaction Parsing and Serializing

**Coming Soon:**

- ABI Decoding
- JSON Decoding and Encoding
- Token Address and Selector Databases


License
-------

MIT License.
