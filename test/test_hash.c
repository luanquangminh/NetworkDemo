#include "src/common/crypto.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    char* hash = hash_password("admin123");
    if (hash) {
        printf("Hash of 'admin123': %s\n", hash);
        free(hash);
    }

    hash = hash_password("test");
    if (hash) {
        printf("Hash of 'test': %s\n", hash);
        free(hash);
    }

    return 0;
}
