#include "crypto.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/sha.h>

char* hash_password(const char* password) {
    if (!password) return NULL;

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char*)password, strlen(password), hash);

    char* hex = malloc(65);  // 64 hex chars + null
    if (!hex) return NULL;

    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        sprintf(hex + (i * 2), "%02x", hash[i]);
    }
    hex[64] = '\0';

    return hex;
}

int verify_password(const char* password, const char* expected_hash) {
    char* computed = hash_password(password);
    if (!computed) return 0;

    int result = (strcmp(computed, expected_hash) == 0);
    free(computed);

    return result;
}
