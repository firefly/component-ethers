import { readFileSync, writeFileSync } from "fs"
import { resolve } from "path";
import { gunzipSync } from "zlib";

import { encode } from "/users/ricmoo/development/firefly/firefly-js/lib.esm/cbor.js";

function loadTests(tag) {
   const filename = resolve("testcases-json/", tag + ".json.gz");
   return JSON.parse(gunzipSync(readFileSync(filename)).toString());
}

const textDecoder = new TextDecoder();
function toUtf8String(data) {
    return textDecoder.decode(data);
}

function zpad(_v, width) {
    let v = _v.toString(16);
    while (v.length < width) { v = "0" + v; }
    return `0x${ v }`;
}

function saveTests(tag, tests, format) {
    const cbor = encode(tests);

    if (format === "cbor") {
        const filename = resolve("testcases-cbor/", tag + ".cbor");
        console.log(`Writing: ${ tag }.cbor (${ cbor.length } bytes)`);
        writeFileSync(filename, cbor);

    } else if (format === "h") {
        const output = [ ];

        output.push(`#ifndef __TEST_${ tag.toUpperCase() }_H__`);
        output.push(`#define __TEST_${ tag.toUpperCase() }_H__`);
        output.push(``);
        output.push(`#ifdef __cplusplus`);
        output.push(`extern "C" {`);
        output.push(`#endif /* __cplusplus */`);
        output.push(``);
        output.push(`#include <stdint.h>`);
        output.push(``);
        output.push(`const uint8_t tests_${ tag }[] = {`);

        let i = 0;
        while (i < cbor.length) {
            const line = [ ];
            while (i < cbor.length && line.length < 8) {
                line.push(zpad(cbor[i++], 2) + ", ");
            }
            output.push("    " + line.join(""));
        }

        output.push(`};`);
        output.push(``);
        output.push(`#ifdef __cplusplus`);
        output.push(`}`);
        output.push(`#endif /* __cplusplus */`);
        output.push(``);
        output.push(`#endif /* __TEST_${ tag.toUpperCase() }_H__ */`);

        const filename = resolve("testcases-h/", tag + ".h");
        console.log(`Writing: ${ tag }.h (${ cbor.length } bytes)`);
        writeFileSync(filename, output.join("\n"));
    }
}

function B(hex) {
    return new Uint8Array(Buffer.from(hex.substring(2), "hex"));
}

const BN_256 = BigInt(256);
function BN(value) {
    value = BigInt(value);

    const bytes = [ ];
    while (value) {
        bytes.unshift(Number(value % BN_256));
        value /= BN_256;
    }
    return new Uint8Array(bytes);
}

function BS(sig) {
    return new Uint8Array(0);
}

(async function() {
    const tag = "accounts";

    const tests = loadTests(tag);

    const output = [ ];
    for (const { name, privateKey, address } of tests) {
        output.push({
            name, address, privkey: B(privateKey)
        });
    }

    //saveTests(tag, output, "cbor");
    saveTests(tag, output, "h");
})();

(async function() {
    const tag = "hashes";

    const tests = loadTests(tag);

    const output = [ ];
    for (const { name, data, sha256, sha512, keccak256 } of tests) {
        output.push({
            name, data: B(data),
            sha256: B(sha256), sha512: B(sha512), keccak256: B(keccak256)
        });
    }

    //saveTests(tag, output, "cbor");
    saveTests(tag, output, "h");
})();

(async function() {
    const tag = "hmac";

    const tests = loadTests(tag);

    const output = [ ];
    for (const { name, data, key, algorithm, hmac } of tests) {
        output.push({
            name, data: B(data), key: B(key), hmac: B(hmac),
            algorithm: ((algorithm === "sha256") ? 256: 512)
        });
    }

    //saveTests(tag, output, "cbor");
    saveTests(tag, output, "h");
})();

(async function() {
    const tag = "mnemonics";

    const tests = loadTests(tag);

    const output = [ ];
    for (const { name, locale, phrase, password, entropy, seed, nodes } of tests) {
        if (locale !== "en") { continue; }
        output.push({
            name, password: B(password), entropy: B(entropy), seed: B(seed),
            phrase: toUtf8String(B(phrase)),
            nodes: nodes.map((n) => {
                return {
                    chaincode: B(n.chainCode),
                    depth: n.depth,
                    index: n.index,
                    pubkey: B(n.publicKey),
                    privkey: B(n.privateKey),
                    _path: n.path,
                    path: n.path.split("/").slice(1).map((c) => {
                        let v = parseInt(c);
                        if (c.endsWith("'")) { v += 0x80000000; }
                        return v;
                    })
                };
            })
        });
    }

    //saveTests(tag, output, "cbor");
    saveTests(tag, output, "h");
})();

(async function() {
    const tag = "pbkdf";

    const tests = loadTests(tag);

    const output = [ ];
    for (const test of tests) {
        output.push({
            name: test.name, password: B(test.password),
            salt: B(test.salt), dkLength: test.dkLen,
            iterations: test.pbkdf2.iterations,
            algorithm: ((test.pbkdf2.algorithm === "sha256") ? 256: 512),
            key: B(test.pbkdf2.key)
        });
    }

    //saveTests(tag, output, "cbor");
    saveTests(tag, output, "h");
})();

(async function() {
    const tag = "transactions";

    const tests = loadTests(tag);

    const output = [ ];
    for (const { name, transaction, privateKey, signedLondon,
      signatureLondon, unsignedLondon } of tests) {

//        if ((transaction.accessList || []).length) { // @TODO: include
//            continue;
//        }

        output.push({
            name, privkey: B(privateKey), sig: BS(signatureLondon),
            rlpUnsigned: B(unsignedLondon), rlpSigned: B(signedLondon),
            tx: {
                type: BN(2),
                chainId: BN(transaction.chainId || 0),
                to: B(transaction.to || "0x"),
                nonce: BN(transaction.nonce || 0),
                gasLimit: BN(transaction.gasLimit || 0),
                maxFeePerGas: BN(transaction.maxFeePerGas || 0),
                maxPriorityFeePerGas: BN(transaction.maxPriorityFeePerGas || 0),
                data: B(transaction.data || "0x"),
                value: BN(transaction.value || 0),
                accessList: (transaction.accessList || []).map((a) => {
                    return [
                        B(a.address), a.storageKeys.map(B)
                    ];
                })
            }
        });
    }

//    saveTests(tag, output, "cbor");
    saveTests(tag, output, "h");
})();
