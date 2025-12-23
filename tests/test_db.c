#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include "../src/database/db_manager.h"
#include "../src/common/crypto.h"

#define TEST_DB "test_fileshare.db"
#define TEST_SCHEMA "src/database/db_init.sql"

void cleanup_test_db(void) {
    unlink(TEST_DB);
    unlink(TEST_DB "-shm");
    unlink(TEST_DB "-wal");
}

void test_db_init(void) {
    printf("[TEST] test_db_init...");

    cleanup_test_db();

    // Initialize database
    Database* db = db_init(TEST_DB);
    assert(db != NULL);

    // Initialize schema
    int result = db_init_schema(db, TEST_SCHEMA);
    assert(result == 0);

    // Verify default admin user exists
    int exists = db_user_exists(db, "admin");
    assert(exists == 1);

    db_close(db);

    printf(" PASSED\n");
}

void test_password_hashing(void) {
    printf("[TEST] test_password_hashing...");

    const char* password = "testpassword123";

    // Hash password
    char* hash1 = hash_password(password);
    assert(hash1 != NULL);
    assert(strlen(hash1) == 64);  // SHA256 hex string

    // Hash same password again
    char* hash2 = hash_password(password);
    assert(hash2 != NULL);

    // Hashes should be identical
    assert(strcmp(hash1, hash2) == 0);

    // Verify password
    assert(verify_password(password, hash1) == 1);
    assert(verify_password("wrongpassword", hash1) == 0);

    free(hash1);
    free(hash2);

    printf(" PASSED\n");
}

void test_user_operations(void) {
    printf("[TEST] test_user_operations...");

    cleanup_test_db();

    Database* db = db_init(TEST_DB);
    assert(db != NULL);
    db_init_schema(db, TEST_SCHEMA);

    // Test creating a new user
    char* password_hash = hash_password("mypassword");
    int user_id = db_create_user(db, "testuser", password_hash);
    assert(user_id > 0);
    free(password_hash);

    // Test user exists
    assert(db_user_exists(db, "testuser") == 1);
    assert(db_user_exists(db, "nonexistent") == 0);

    // Test user verification with correct password
    password_hash = hash_password("mypassword");
    int verified_user_id = 0;
    int result = db_verify_user(db, "testuser", password_hash, &verified_user_id);
    assert(result == 0);
    assert(verified_user_id == user_id);
    free(password_hash);

    // Test user verification with wrong password
    password_hash = hash_password("wrongpassword");
    result = db_verify_user(db, "testuser", password_hash, &verified_user_id);
    assert(result != 0);
    free(password_hash);

    // Test getting user by ID
    char username[256];
    result = db_get_user_by_id(db, user_id, username, sizeof(username));
    assert(result == 0);
    assert(strcmp(username, "testuser") == 0);

    // Test default admin user
    password_hash = hash_password("admin");
    result = db_verify_user(db, "admin", password_hash, &verified_user_id);
    assert(result == 0);
    assert(verified_user_id == 1);
    free(password_hash);

    db_close(db);

    printf(" PASSED\n");
}

void test_activity_logging(void) {
    printf("[TEST] test_activity_logging...");

    cleanup_test_db();

    Database* db = db_init(TEST_DB);
    assert(db != NULL);
    db_init_schema(db, TEST_SCHEMA);

    // Log some activities
    int result = db_log_activity(db, 1, "LOGIN", "User logged in");
    assert(result == 0);

    result = db_log_activity(db, 1, "UPLOAD", "Uploaded file test.txt");
    assert(result == 0);

    result = db_log_activity(db, 1, "DOWNLOAD", "Downloaded file data.csv");
    assert(result == 0);

    db_close(db);

    printf(" PASSED\n");
}

void test_file_operations(void) {
    printf("[TEST] test_file_operations...");

    cleanup_test_db();

    Database* db = db_init(TEST_DB);
    assert(db != NULL);
    db_init_schema(db, TEST_SCHEMA);

    // Create a directory
    int dir_id = db_create_file(db, 0, "documents", NULL, 1, 0, 1, 755);
    assert(dir_id > 0);

    // Create a file in the directory
    int file_id = db_create_file(db, dir_id, "test.txt", "/storage/file1.dat", 1, 1024, 0, 644);
    assert(file_id > 0);

    // Get file by ID
    FileEntry entry;
    int result = db_get_file_by_id(db, file_id, &entry);
    assert(result == 0);
    assert(entry.id == file_id);
    assert(entry.parent_id == dir_id);
    assert(strcmp(entry.name, "test.txt") == 0);
    assert(entry.size == 1024);
    assert(entry.is_directory == 0);
    assert(entry.permissions == 644);

    // List directory contents
    FileEntry* entries = NULL;
    int count = 0;
    result = db_list_directory(db, dir_id, &entries, &count);
    assert(result == 0);
    assert(count == 1);
    assert(entries != NULL);
    assert(strcmp(entries[0].name, "test.txt") == 0);
    free(entries);

    // Update file permissions
    result = db_update_permissions(db, file_id, 600);
    assert(result == 0);

    result = db_get_file_by_id(db, file_id, &entry);
    assert(result == 0);
    assert(entry.permissions == 600);

    // Delete file
    result = db_delete_file(db, file_id);
    assert(result == 0);

    // Verify file is deleted
    result = db_get_file_by_id(db, file_id, &entry);
    assert(result != 0);

    db_close(db);

    printf(" PASSED\n");
}

int main(void) {
    printf("========================================\n");
    printf("Running Phase 3 Database Tests\n");
    printf("========================================\n\n");

    test_db_init();
    test_password_hashing();
    test_user_operations();
    test_activity_logging();
    test_file_operations();

    cleanup_test_db();

    printf("\n========================================\n");
    printf("All database tests PASSED!\n");
    printf("========================================\n");

    return 0;
}
