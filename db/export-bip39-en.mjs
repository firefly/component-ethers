import { readFileSync, writeFileSync } from "fs";
import { resolve } from "./utils.mjs";

const words = readFileSync(resolve("lang-en.txt")).toString().trim().split("\n");
if (words.length !== 2048) { throw new Error("bad list!"); }

const lines = [ ];
lines.push("#ifndef __BIP39_EN_H__");
lines.push("#define __BIP39_EN_H__");
lines.push("");
lines.push("#ifdef __cplusplus");
lines.push('extern "C" {');
lines.push("#endif /* __cplusplus */");
lines.push("");

lines.push("const char* wordlist_en = ");
for (let i = 0; i < words.length; i++) {
    lines.push(`    "${ words[i] }\\0"`);
}
lines.push("    ;");

lines.push("");
lines.push("#ifdef __cplusplus");
lines.push("}");
lines.push("#endif /* __cplusplus */");
lines.push("");
lines.push("#endif /* __BIP39_EN_H__ */");

writeFileSync(resolve("../src/bip39-en.h"), lines.join("\n"));
