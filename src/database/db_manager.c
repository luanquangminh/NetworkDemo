#include "db_manager.h"
#include "../common/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Database* db_init(const char* db_path) {
    Database* db = malloc(sizeof(Database));
    if (!db) {
        log_error("Failed to allocate database structure");
        return NULL;
    }

    pthread_mutex_init(&db->mutex, NULL);

    int rc = sqlite3_open(db_path, &db->conn);
    if (rc != SQLITE_OK) {
        log_error("Cannot open database: %s", sqlite3_errmsg(db->conn));
        free(db);
        return NULL;
    }

    // Enable WAL mode for better concurrency
    sqlite3_exec(db->conn, "PRAGMA journal_mode=WAL;", NULL, NULL, NULL);

    log_info("Database opened: %s", db_path);
    return db;
}

void db_close(Database* db) {
    if (db) {
        pthread_mutex_lock(&db->mutex);
        if (db->conn) {
            sqlite3_close(db->conn);
        }
        pthread_mutex_unlock(&db->mutex);
        pthread_mutex_destroy(&db->mutex);
        free(db);
        log_info("Database closed");
    }
}

int db_init_schema(Database* db, const char* schema_path) {
    FILE* f = fopen(schema_path, "r");
    if (!f) {
        log_error("Cannot open schema file: %s", schema_path);
        return -1;
    }

    // Read entire file
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* sql = malloc(size + 1);
    fread(sql, 1, size, f);
    sql[size] = '\0';
    fclose(f);

    pthread_mutex_lock(&db->mutex);

    char* err_msg = NULL;
    int rc = sqlite3_exec(db->conn, sql, NULL, NULL, &err_msg);

    pthread_mutex_unlock(&db->mutex);

    if (rc != SQLITE_OK) {
        log_error("Schema execution failed: %s", err_msg);
        sqlite3_free(err_msg);
        free(sql);
        return -1;
    }

    free(sql);
    log_info("Database schema initialized");
    return 0;
}

int db_create_user(Database* db, const char* username, const char* password_hash) {
    pthread_mutex_lock(&db->mutex);

    sqlite3_stmt* stmt;
    const char* sql = "INSERT INTO users (username, password_hash) VALUES (?, ?)";

    int rc = sqlite3_prepare_v2(db->conn, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        pthread_mutex_unlock(&db->mutex);
        return -1;
    }

    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, password_hash, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    int user_id = (rc == SQLITE_DONE) ? (int)sqlite3_last_insert_rowid(db->conn) : -1;

    sqlite3_finalize(stmt);
    pthread_mutex_unlock(&db->mutex);

    if (user_id > 0) {
        log_info("Created user: %s (id=%d)", username, user_id);
    }

    return user_id;
}

int db_verify_user(Database* db, const char* username, const char* password_hash, int* user_id) {
    pthread_mutex_lock(&db->mutex);

    sqlite3_stmt* stmt;
    const char* sql = "SELECT id FROM users WHERE username = ? AND password_hash = ? AND is_active = 1";

    int rc = sqlite3_prepare_v2(db->conn, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        pthread_mutex_unlock(&db->mutex);
        return -1;
    }

    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, password_hash, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    int result = -1;

    if (rc == SQLITE_ROW) {
        *user_id = sqlite3_column_int(stmt, 0);
        result = 0;  // Success
    }

    sqlite3_finalize(stmt);
    pthread_mutex_unlock(&db->mutex);

    return result;
}

int db_get_user_by_id(Database* db, int user_id, char* username, int username_size) {
    pthread_mutex_lock(&db->mutex);

    sqlite3_stmt* stmt;
    const char* sql = "SELECT username FROM users WHERE id = ?";

    int rc = sqlite3_prepare_v2(db->conn, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        pthread_mutex_unlock(&db->mutex);
        return -1;
    }

    sqlite3_bind_int(stmt, 1, user_id);

    rc = sqlite3_step(stmt);
    int result = -1;

    if (rc == SQLITE_ROW) {
        const char* name = (const char*)sqlite3_column_text(stmt, 0);
        strncpy(username, name, username_size - 1);
        username[username_size - 1] = '\0';
        result = 0;
    }

    sqlite3_finalize(stmt);
    pthread_mutex_unlock(&db->mutex);

    return result;
}

int db_user_exists(Database* db, const char* username) {
    pthread_mutex_lock(&db->mutex);

    sqlite3_stmt* stmt;
    const char* sql = "SELECT 1 FROM users WHERE username = ?";

    int rc = sqlite3_prepare_v2(db->conn, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        pthread_mutex_unlock(&db->mutex);
        return -1;
    }

    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    int exists = (rc == SQLITE_ROW) ? 1 : 0;

    sqlite3_finalize(stmt);
    pthread_mutex_unlock(&db->mutex);

    return exists;
}

int db_log_activity(Database* db, int user_id, const char* action_type, const char* description) {
    pthread_mutex_lock(&db->mutex);

    sqlite3_stmt* stmt;
    const char* sql = "INSERT INTO activity_logs (user_id, action_type, description) VALUES (?, ?, ?)";

    int rc = sqlite3_prepare_v2(db->conn, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        pthread_mutex_unlock(&db->mutex);
        return -1;
    }

    sqlite3_bind_int(stmt, 1, user_id);
    sqlite3_bind_text(stmt, 2, action_type, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, description ? description : "", -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    int result = (rc == SQLITE_DONE) ? 0 : -1;

    sqlite3_finalize(stmt);
    pthread_mutex_unlock(&db->mutex);

    return result;
}

// File operations - stub implementations for Phase 4
int db_create_file(Database* db, int parent_id, const char* name, const char* physical_path,
                   int owner_id, long size, int is_directory, int permissions) {
    pthread_mutex_lock(&db->mutex);

    sqlite3_stmt* stmt;
    const char* sql = "INSERT INTO files (parent_id, name, physical_path, owner_id, size, is_directory, permissions) "
                      "VALUES (?, ?, ?, ?, ?, ?, ?)";

    int rc = sqlite3_prepare_v2(db->conn, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        pthread_mutex_unlock(&db->mutex);
        return -1;
    }

    sqlite3_bind_int(stmt, 1, parent_id);
    sqlite3_bind_text(stmt, 2, name, -1, SQLITE_STATIC);

    // For directories, use NULL for physical_path to avoid UNIQUE constraint issues
    if (physical_path && physical_path[0] != '\0') {
        sqlite3_bind_text(stmt, 3, physical_path, -1, SQLITE_STATIC);
    } else {
        sqlite3_bind_null(stmt, 3);
    }

    sqlite3_bind_int(stmt, 4, owner_id);
    sqlite3_bind_int64(stmt, 5, size);
    sqlite3_bind_int(stmt, 6, is_directory);
    sqlite3_bind_int(stmt, 7, permissions);

    rc = sqlite3_step(stmt);
    int file_id = -1;

    if (rc == SQLITE_DONE) {
        file_id = (int)sqlite3_last_insert_rowid(db->conn);
        log_info("db_create_file: Successfully created file/dir '%s' with id=%d", name, file_id);
    } else {
        log_error("db_create_file: sqlite3_step failed with rc=%d: %s", rc, sqlite3_errmsg(db->conn));
        log_error("db_create_file: Parameters - parent_id=%d, name='%s', owner_id=%d, is_dir=%d",
                  parent_id, name, owner_id, is_directory);
    }

    sqlite3_finalize(stmt);
    pthread_mutex_unlock(&db->mutex);

    return file_id;
}

int db_get_file_by_id(Database* db, int file_id, FileEntry* entry) {
    pthread_mutex_lock(&db->mutex);

    sqlite3_stmt* stmt;
    const char* sql = "SELECT id, parent_id, name, physical_path, owner_id, size, is_directory, permissions, created_at "
                      "FROM files WHERE id = ?";

    int rc = sqlite3_prepare_v2(db->conn, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        pthread_mutex_unlock(&db->mutex);
        return -1;
    }

    sqlite3_bind_int(stmt, 1, file_id);

    rc = sqlite3_step(stmt);
    int result = -1;

    if (rc == SQLITE_ROW) {
        entry->id = sqlite3_column_int(stmt, 0);
        entry->parent_id = sqlite3_column_int(stmt, 1);
        strncpy(entry->name, (const char*)sqlite3_column_text(stmt, 2), sizeof(entry->name) - 1);
        const char* path = (const char*)sqlite3_column_text(stmt, 3);
        if (path) strncpy(entry->physical_path, path, sizeof(entry->physical_path) - 1);
        entry->owner_id = sqlite3_column_int(stmt, 4);
        entry->size = sqlite3_column_int64(stmt, 5);
        entry->is_directory = sqlite3_column_int(stmt, 6);
        entry->permissions = sqlite3_column_int(stmt, 7);
        const char* created = (const char*)sqlite3_column_text(stmt, 8);
        if (created) strncpy(entry->created_at, created, sizeof(entry->created_at) - 1);
        result = 0;
    }

    sqlite3_finalize(stmt);
    pthread_mutex_unlock(&db->mutex);

    return result;
}

int db_list_directory(Database* db, int parent_id, FileEntry** entries, int* count) {
    pthread_mutex_lock(&db->mutex);

    // First, count entries
    sqlite3_stmt* stmt;
    const char* count_sql = "SELECT COUNT(*) FROM files WHERE parent_id = ?";

    sqlite3_prepare_v2(db->conn, count_sql, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, parent_id);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        *count = sqlite3_column_int(stmt, 0);
    } else {
        *count = 0;
    }
    sqlite3_finalize(stmt);

    if (*count == 0) {
        *entries = NULL;
        pthread_mutex_unlock(&db->mutex);
        return 0;
    }

    // Allocate entries
    *entries = malloc(sizeof(FileEntry) * (*count));
    if (!*entries) {
        pthread_mutex_unlock(&db->mutex);
        return -1;
    }

    // Fetch entries
    const char* sql = "SELECT id, parent_id, name, physical_path, owner_id, size, is_directory, permissions, created_at "
                      "FROM files WHERE parent_id = ? ORDER BY is_directory DESC, name ASC";

    sqlite3_prepare_v2(db->conn, sql, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, parent_id);

    int i = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && i < *count) {
        (*entries)[i].id = sqlite3_column_int(stmt, 0);
        (*entries)[i].parent_id = sqlite3_column_int(stmt, 1);
        strncpy((*entries)[i].name, (const char*)sqlite3_column_text(stmt, 2), sizeof((*entries)[i].name) - 1);
        const char* path = (const char*)sqlite3_column_text(stmt, 3);
        if (path) strncpy((*entries)[i].physical_path, path, sizeof((*entries)[i].physical_path) - 1);
        (*entries)[i].owner_id = sqlite3_column_int(stmt, 4);
        (*entries)[i].size = sqlite3_column_int64(stmt, 5);
        (*entries)[i].is_directory = sqlite3_column_int(stmt, 6);
        (*entries)[i].permissions = sqlite3_column_int(stmt, 7);
        const char* created = (const char*)sqlite3_column_text(stmt, 8);
        if (created) strncpy((*entries)[i].created_at, created, sizeof((*entries)[i].created_at) - 1);
        i++;
    }

    sqlite3_finalize(stmt);
    pthread_mutex_unlock(&db->mutex);

    return 0;
}

int db_delete_file(Database* db, int file_id) {
    pthread_mutex_lock(&db->mutex);

    sqlite3_stmt* stmt;
    const char* sql = "DELETE FROM files WHERE id = ?";

    int rc = sqlite3_prepare_v2(db->conn, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        pthread_mutex_unlock(&db->mutex);
        return -1;
    }

    sqlite3_bind_int(stmt, 1, file_id);

    rc = sqlite3_step(stmt);
    int result = (rc == SQLITE_DONE) ? 0 : -1;

    sqlite3_finalize(stmt);
    pthread_mutex_unlock(&db->mutex);

    return result;
}

int db_update_permissions(Database* db, int file_id, int permissions) {
    pthread_mutex_lock(&db->mutex);

    sqlite3_stmt* stmt;
    const char* sql = "UPDATE files SET permissions = ? WHERE id = ?";

    int rc = sqlite3_prepare_v2(db->conn, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        pthread_mutex_unlock(&db->mutex);
        return -1;
    }

    sqlite3_bind_int(stmt, 1, permissions);
    sqlite3_bind_int(stmt, 2, file_id);

    rc = sqlite3_step(stmt);
    int result = (rc == SQLITE_DONE) ? 0 : -1;

    sqlite3_finalize(stmt);
    pthread_mutex_unlock(&db->mutex);

    return result;
}

// Admin user operations
int db_is_admin(Database* db, int user_id) {
    pthread_mutex_lock(&db->mutex);

    sqlite3_stmt* stmt;
    const char* sql = "SELECT is_admin FROM users WHERE id = ? AND is_active = 1";

    int rc = sqlite3_prepare_v2(db->conn, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        pthread_mutex_unlock(&db->mutex);
        return 0;
    }

    sqlite3_bind_int(stmt, 1, user_id);

    rc = sqlite3_step(stmt);
    int is_admin = 0;

    if (rc == SQLITE_ROW) {
        is_admin = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    pthread_mutex_unlock(&db->mutex);

    return is_admin;
}

int db_list_users(Database* db, char** json_result) {
    pthread_mutex_lock(&db->mutex);

    sqlite3_stmt* stmt;
    const char* sql = "SELECT id, username, is_active, is_admin, created_at FROM users ORDER BY id ASC";

    int rc = sqlite3_prepare_v2(db->conn, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        pthread_mutex_unlock(&db->mutex);
        return -1;
    }

    // Build JSON manually (simple approach without cJSON dependency in db layer)
    char* buffer = malloc(4096);
    if (!buffer) {
        sqlite3_finalize(stmt);
        pthread_mutex_unlock(&db->mutex);
        return -1;
    }

    strcpy(buffer, "[");
    int first = 1;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (!first) strcat(buffer, ",");
        first = 0;

        int id = sqlite3_column_int(stmt, 0);
        const char* username = (const char*)sqlite3_column_text(stmt, 1);
        int is_active = sqlite3_column_int(stmt, 2);
        int is_admin = sqlite3_column_int(stmt, 3);
        const char* created_at = (const char*)sqlite3_column_text(stmt, 4);

        char entry[512];
        snprintf(entry, sizeof(entry),
                 "{\"id\":%d,\"username\":\"%s\",\"is_active\":%d,\"is_admin\":%d,\"created_at\":\"%s\"}",
                 id, username, is_active, is_admin, created_at ? created_at : "");
        strcat(buffer, entry);
    }

    strcat(buffer, "]");

    sqlite3_finalize(stmt);
    pthread_mutex_unlock(&db->mutex);

    *json_result = buffer;
    return 0;
}

int db_delete_user(Database* db, int user_id) {
    // Safety check: prevent deletion of user ID 1 (primary admin)
    if (user_id == 1) {
        log_error("Cannot delete primary admin user (id=1)");
        return -1;
    }

    pthread_mutex_lock(&db->mutex);

    sqlite3_stmt* stmt;
    const char* sql = "DELETE FROM users WHERE id = ?";

    int rc = sqlite3_prepare_v2(db->conn, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        pthread_mutex_unlock(&db->mutex);
        return -1;
    }

    sqlite3_bind_int(stmt, 1, user_id);

    rc = sqlite3_step(stmt);
    int result = (rc == SQLITE_DONE) ? 0 : -1;

    sqlite3_finalize(stmt);
    pthread_mutex_unlock(&db->mutex);

    if (result == 0) {
        log_info("Deleted user with id=%d", user_id);
    }

    return result;
}

int db_update_user(Database* db, int user_id, int is_admin, int is_active) {
    // Safety check: prevent modification of user ID 1's admin status
    if (user_id == 1 && is_admin == 0) {
        log_error("Cannot remove admin status from primary admin user (id=1)");
        return -1;
    }

    pthread_mutex_lock(&db->mutex);

    sqlite3_stmt* stmt;
    const char* sql = "UPDATE users SET is_admin = ?, is_active = ? WHERE id = ?";

    int rc = sqlite3_prepare_v2(db->conn, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        pthread_mutex_unlock(&db->mutex);
        return -1;
    }

    sqlite3_bind_int(stmt, 1, is_admin);
    sqlite3_bind_int(stmt, 2, is_active);
    sqlite3_bind_int(stmt, 3, user_id);

    rc = sqlite3_step(stmt);
    int result = (rc == SQLITE_DONE) ? 0 : -1;

    sqlite3_finalize(stmt);
    pthread_mutex_unlock(&db->mutex);

    if (result == 0) {
        log_info("Updated user id=%d: is_admin=%d, is_active=%d", user_id, is_admin, is_active);
    }

    return result;
}

int db_create_user_admin(Database* db, const char* username, const char* password_hash, int is_admin) {
    pthread_mutex_lock(&db->mutex);

    sqlite3_stmt* stmt;
    const char* sql = "INSERT INTO users (username, password_hash, is_admin) VALUES (?, ?, ?)";

    int rc = sqlite3_prepare_v2(db->conn, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        pthread_mutex_unlock(&db->mutex);
        return -1;
    }

    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, password_hash, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, is_admin);

    rc = sqlite3_step(stmt);
    int user_id = (rc == SQLITE_DONE) ? (int)sqlite3_last_insert_rowid(db->conn) : -1;

    sqlite3_finalize(stmt);
    pthread_mutex_unlock(&db->mutex);

    if (user_id > 0) {
        log_info("Created user: %s (id=%d, is_admin=%d)", username, user_id, is_admin);
    }

    return user_id;
}
