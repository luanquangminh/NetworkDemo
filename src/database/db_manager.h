#ifndef DB_MANAGER_H
#define DB_MANAGER_H

#include <sqlite3.h>
#include <pthread.h>

// Database handle
typedef struct {
    sqlite3* conn;
    pthread_mutex_t mutex;
} Database;

// File entry structure (for VFS operations)
typedef struct {
    int id;
    int parent_id;
    char name[256];
    char physical_path[64];
    int owner_id;
    long size;
    int is_directory;
    int permissions;
    char created_at[32];
} FileEntry;

// Initialize database connection
Database* db_init(const char* db_path);

// Close database
void db_close(Database* db);

// Execute schema initialization
int db_init_schema(Database* db, const char* schema_path);

// User operations
int db_create_user(Database* db, const char* username, const char* password_hash);
int db_verify_user(Database* db, const char* username, const char* password_hash, int* user_id);
int db_get_user_by_id(Database* db, int user_id, char* username, int username_size);
int db_user_exists(Database* db, const char* username);

// Admin user operations
int db_is_admin(Database* db, int user_id);
int db_list_users(Database* db, char** json_result);
int db_delete_user(Database* db, int user_id);
int db_update_user(Database* db, int user_id, int is_admin, int is_active);
int db_create_user_admin(Database* db, const char* username, const char* password_hash, int is_admin);

// Activity logging
int db_log_activity(Database* db, int user_id, const char* action_type, const char* description);

// File operations (stubs for Phase 4, but implement signature)
int db_create_file(Database* db, int parent_id, const char* name, const char* physical_path,
                   int owner_id, long size, int is_directory, int permissions);
int db_get_file_by_id(Database* db, int file_id, FileEntry* entry);
int db_list_directory(Database* db, int parent_id, FileEntry** entries, int* count);
int db_delete_file(Database* db, int file_id);
int db_update_permissions(Database* db, int file_id, int permissions);

// Search operations
int db_search_files(Database* db, int base_dir_id, const char* pattern,
                    int recursive, int user_id, int limit,
                    FileEntry** entries, int* count);

// File management operations
int db_rename_file(Database* db, int file_id, const char* new_name);
int db_copy_file(Database* db, int source_id, int dest_parent_id, const char* new_name, int user_id);
int db_move_file(Database* db, int file_id, int new_parent_id);

#endif
