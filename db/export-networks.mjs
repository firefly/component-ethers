import { readFileSync, writeFileSync } from "node:fs";

import { resolve } from "./utils.mjs"

const data = ((filename) => {
    console.log(`Reading: ${ filename }`);
    const data = JSON.parse(readFileSync(filename).toString());
    return data.sort((a, b) => a.chainId - b.chainId);
})(resolve("networks.json"))


const strings = [ ];
let stringsIndex = 0;

const indices = [ ];

function pad(length) {
    let v = "                ";
    while (v.length < length) { v += v; }
    return v.substring(0, length);
}

function zpad(v, length) {
    v = String(v);
    while (v.length < length) { v = "0" + v; }
    return v;
}

function checkStr(str) {
    for (let i = 0; i < str.length; i++) {
        const c = str.charCodeAt(i);
        if (c < 32 || c > 126) { throw new Error(); }
    }
    return str;
}

for (const { name, token, chainId } of data) {
    if (!name) { continue; }
    if (chainId > 0x7fffffff) {
        console.log("Skipping", { name, token, chainId });
        continue;
    }

    indices.push(chainId);
    indices.push(stringsIndex);

    const padding = pad(20 - name.length - token.length);
    strings.push(`"${ name }\\0${ token }\\0" ${ padding }// 0x${ zpad(stringsIndex.toString(16), 8) }  Chain ID: ${ chainId }`);
    stringsIndex += name.length + 1 + token.length + 1;
}

function hex(v, length) {
    v = v.toString(16);
    while (v.length < length) { v = "0" + v; }
    return `0x${ v }`
}

const lines = [ ];
lines.push(`#ifndef __DB_NETWORKS_H__`);
lines.push(`#define __DB_NETWORKS_H__`);
lines.push(``);
lines.push(`// This file is generated! Do NOT modify manually. See export-db-networks.mjs.`);
lines.push(``);
lines.push(`#ifdef __cplusplus`);
lines.push(`extern "C" {`);
lines.push(`#endif /* __cplusplus */`);
lines.push(``)
lines.push(`#include <stddef.h>`);
lines.push(`#include <stdint.h>`);
lines.push(``)
lines.push(`const char _ffx_db_networkStrings[] =`)
for (const str of strings) {
    lines.push("    " + str);
}
lines.push(`;`)
lines.push(``)
lines.push(`const size_t _ffx_db_networkCount = ${ indices.length };`)
lines.push(``)
lines.push(`const uint32_t _ffx_db_networkIndex[] = {`)
while (indices.length) {
    const l = [ ];
    while (indices.length) {
        l.push(`${ hex(indices.shift(), 8) },`);
        if (l.length > 5) { break; }
    }
    lines.push("    " + l.join(" "));
}
lines.push(`};`)
lines.push(``)
lines.push(`#ifdef __cplusplus`);
lines.push(`}`);
lines.push(`#endif /* __cplusplus */`);
lines.push(``);
lines.push(`#endif /* __DB_NETWORKS_H__ */`);

{
    const filename = resolve("../src/db-networks.h");
    console.log(`Writing: ${ filename }`);
    writeFileSync(filename, lines.join("\n"));
}
