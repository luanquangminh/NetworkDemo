#include "src/common/crypto.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    const char* target = "8c6976e5b5410415bde908bd4dee15dfb167a9c873fc4bb8a81f6f2ab448a918";

    const char* passwords[] = {
        "admin", "password", "123456", "admin123", "test",
        "root", "pass", "changeme", "default", "a", "1234"
    };

    int i;
    for (i = 0; i < sizeof(passwords)/sizeof(passwords[0]); i++) {
        char* hash = hash_password(passwords[i]);
        if (hash) {
            printf("'%s' -> %s", passwords[i], hash);
            if (strcmp(hash, target) == 0) {
                printf(" *** MATCH! ***");
            }
            printf("\n");
            free(hash);
        }
    }

    return 0;
}
