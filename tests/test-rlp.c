#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "./rlp.h"

int main() {
    uint8_t test[512] = { 0 };

    RlpBuilder rlp;
    rlp_build(&rlp, test, sizeof(test));

    rlp_appendArray(&rlp, 4);
    rlp_appendString(&rlp, "Hello");
    rlp_appendArray(&rlp, 2);
    rlp_appendString(&rlp, "World");
    rlp_appendString(&rlp, "12345");
    rlp_appendString(&rlp, "");
    rlp_appendArray(&rlp, 0);

    size_t length = rlp_finalize(&rlp);

    printf("RLP: length=%zu\n0x", length);
    for (int i = 0; i < length; i++) { printf("%02x", test[i]); }
    printf("\n");
}
