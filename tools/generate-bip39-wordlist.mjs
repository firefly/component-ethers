import { readFileSync } from "fs";

function log(line) {
    console.log(line);
}

(async function() {
    const words = readFileSync("lang-en.txt").toString().trim().split("\n");
    if (words.length !== 2048) { throw new Error("bad list!"); }

    log("#ifndef __BIP39_EN_H__");
    log("#define __BIP39_EN_H__");
    log("");
    log("#ifdef __cplusplus");
    log('extern "C" {');
    log("#endif /* __cplusplus */");
    log("");

    log("const char* wordlist_en = ");
    for (let i = 0; i < words.length; i++) {
        log(`    "${ words[i] }\\0"`);
    }
    log("    ;");

    log("");
    log("#ifdef __cplusplus");
    log("}");
    log("#endif /* __cplusplus */");
    log("");
    log("#endif /* __BIP39_EN_H__ */");
})();
