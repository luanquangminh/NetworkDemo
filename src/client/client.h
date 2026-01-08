#ifndef CLIENT_H
#define CLIENT_H

#include <stdint.h>
#include "../common/protocol.h"

// Connection state
typedef struct {
    int socket_fd;
    char server_ip[64];
    int server_port;
    int authenticated;
    int user_id;
    int is_admin;
    int current_directory;
    char current_path[512];
} ClientConnection;

// Connection management
ClientConnection* client_connect(const char* ip, int port);
void client_disconnect(ClientConnection* conn);

// Authentication
int client_login(ClientConnection* conn, const char* username, const char* password);

// File operations
int client_list_dir(ClientConnection* conn, int dir_id);
void* client_list_dir_gui(ClientConnection* conn, int dir_id);  // Returns cJSON* with file list for GUI
int client_mkdir(ClientConnection* conn, const char* name);
int client_cd(ClientConnection* conn, int dir_id);
int client_upload(ClientConnection* conn, const char* local_path);
int client_download(ClientConnection* conn, int file_id, const char* local_path);
int client_chmod(ClientConnection* conn, int file_id, int permissions);

// Recursive operations
int client_upload_folder(ClientConnection* conn, const char* local_path);
int client_download_folder(ClientConnection* conn, int folder_id, const char* local_path);

// Additional operations
int client_delete(ClientConnection* conn, int file_id);
int client_file_info(ClientConnection* conn, int file_id);

// Search operations
void* client_search(ClientConnection* conn, const char* pattern, int recursive, int limit);

// File management operations
int client_rename(ClientConnection* conn, int file_id, const char* new_name);
int client_copy(ClientConnection* conn, int source_id, int dest_parent_id, const char* new_name);
int client_move(ClientConnection* conn, int file_id, int new_parent_id);

// Admin operations
void* client_admin_list_users(ClientConnection* conn);  // Returns cJSON* with user list
int client_admin_create_user(ClientConnection* conn, const char* username, const char* password, int is_admin);
int client_admin_delete_user(ClientConnection* conn, int user_id);
int client_admin_update_user(ClientConnection* conn, int user_id, int is_admin, int is_active);

#endif // CLIENT_H
