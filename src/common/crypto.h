#ifndef CRYPTO_H
#define CRYPTO_H

// Hash password using SHA-256
char* hash_password(const char* password);

// Verify password against hash
int verify_password(const char* password, const char* hash);

#endif
