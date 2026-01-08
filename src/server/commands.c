#include "commands.h"
#include "storage.h"
#include "permissions.h"
#include "../common/utils.h"
#include "../common/crypto.h"
#include "../database/db_manager.h"
#include "../../lib/cJSON/cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

// Global database handle (defined in main.c)
extern Database* global_db;

void commands_init(void) {
    log_info("Command handlers initialized");
}

int dispatch_command(ClientSession* session, Packet* pkt) {
    log_debug("Dispatching command 0x%02X", pkt->command);

    // Commands requiring authentication
    if (pkt->command != CMD_LOGIN_REQ && !session->authenticated) {
        send_error(session, "Not authenticated");
        return -1;
    }

    switch (pkt->command) {
        case CMD_LOGIN_REQ:
            handle_login(session, pkt);
            break;
        case CMD_LIST_DIR:
            handle_list_dir(session, pkt);
            break;
        case CMD_CHANGE_DIR:
            handle_change_dir(session, pkt);
            break;
        case CMD_MAKE_DIR:
            handle_mkdir(session, pkt);
            break;
        case CMD_UPLOAD_REQ:
            handle_upload_req(session, pkt);
            break;
        case CMD_UPLOAD_DATA:
            handle_upload_data(session, pkt);
            break;
        case CMD_DOWNLOAD_REQ:
            handle_download(session, pkt);
            break;
        case CMD_CHMOD:
            handle_chmod(session, pkt);
            break;
        case CMD_DELETE:
            handle_delete(session, pkt);
            break;
        case CMD_FILE_INFO:
            handle_file_info(session, pkt);
            break;
        case CMD_SEARCH_REQ:
            handle_search(session, pkt);
            break;
        case CMD_RENAME:
            handle_rename(session, pkt);
            break;
        case CMD_COPY:
            handle_copy(session, pkt);
            break;
        case CMD_MOVE:
            handle_move(session, pkt);
            break;
        case CMD_ADMIN_LIST_USERS:
            handle_admin_list_users(session, pkt);
            break;
        case CMD_ADMIN_CREATE_USER:
            handle_admin_create_user(session, pkt);
            break;
        case CMD_ADMIN_DELETE_USER:
            handle_admin_delete_user(session, pkt);
            break;
        case CMD_ADMIN_UPDATE_USER:
            handle_admin_update_user(session, pkt);
            break;
        default:
            send_error(session, "Unknown command");
            return -1;
    }

    return 0;
}

void send_error(ClientSession* session, const char* message) {
    cJSON* json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "status", "ERROR");
    cJSON_AddStringToObject(json, "message", message);

    char* payload = cJSON_PrintUnformatted(json);
    Packet* response = packet_create(CMD_ERROR, payload, strlen(payload));

    packet_send(session->client_socket, response);

    free(payload);
    packet_free(response);
    cJSON_Delete(json);
}

void send_success(ClientSession* session, uint8_t cmd, const char* json_payload) {
    Packet* response = packet_create(cmd, json_payload, strlen(json_payload));
    packet_send(session->client_socket, response);
    packet_free(response);
}

void handle_login(ClientSession* session, Packet* pkt) {
    if (!pkt->payload) {
        send_error(session, "Empty payload");
        return;
    }

    cJSON* json = cJSON_Parse(pkt->payload);
    if (!json) {
        send_error(session, "Invalid JSON");
        return;
    }

    cJSON* username_item = cJSON_GetObjectItem(json, "username");
    cJSON* password_item = cJSON_GetObjectItem(json, "password");

    if (!username_item || !password_item) {
        send_error(session, "Missing credentials");
        cJSON_Delete(json);
        return;
    }

    const char* username = cJSON_GetStringValue(username_item);
    const char* password = cJSON_GetStringValue(password_item);

    log_info("Login attempt for user: %s", username);

    // Hash the password and verify against database
    char* password_hash = hash_password(password);
    if (!password_hash) {
        send_error(session, "Internal error");
        cJSON_Delete(json);
        return;
    }

    int user_id = 0;
    if (db_verify_user(global_db, username, password_hash, &user_id) == 0) {
        // Login successful
        session->authenticated = 1;
        session->user_id = user_id;
        session->current_directory = 0;  // Root
        session->state = STATE_AUTHENTICATED;

        // Check if user is admin
        int is_admin = db_is_admin(global_db, user_id);

        // Log successful login
        db_log_activity(global_db, user_id, "LOGIN", "User logged in successfully");

        cJSON* response_json = cJSON_CreateObject();
        cJSON_AddStringToObject(response_json, "status", "OK");
        cJSON_AddNumberToObject(response_json, "user_id", session->user_id);
        cJSON_AddNumberToObject(response_json, "is_admin", is_admin);

        char* response_payload = cJSON_PrintUnformatted(response_json);
        send_success(session, CMD_LOGIN_RES, response_payload);

        log_info("User '%s' logged in successfully (user_id=%d, is_admin=%d)", username, user_id, is_admin);

        free(response_payload);
        cJSON_Delete(response_json);
    } else {
        // Login failed
        send_error(session, "Invalid credentials");
        log_info("Login failed for user: %s", username);
    }

    free(password_hash);
    cJSON_Delete(json);
}

void handle_list_dir(ClientSession* session, Packet* pkt) {
    cJSON* json = cJSON_Parse(pkt->payload);
    int dir_id = session->current_directory;

    // Allow override via JSON payload
    if (json && cJSON_GetObjectItem(json, "directory_id")) {
        dir_id = cJSON_GetObjectItem(json, "directory_id")->valueint;
    }

    // Check READ permission on directory
    if (!check_permission(global_db, session->user_id, dir_id, ACCESS_READ)) {
        send_error(session, "Permission denied");
        db_log_activity(global_db, session->user_id, "ACCESS_DENIED", "LIST_DIR");
        if (json) cJSON_Delete(json);
        return;
    }

    FileEntry* entries = NULL;
    int count = 0;
    if (db_list_directory(global_db, dir_id, &entries, &count) < 0) {
        send_error(session, "Failed to list directory");
        if (json) cJSON_Delete(json);
        return;
    }

    cJSON* response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "status", "OK");
    cJSON* files_array = cJSON_AddArrayToObject(response, "files");

    for (int i = 0; i < count; i++) {
        // Fetch username from owner_id
        char owner_username[256] = "unknown";
        if (db_get_user_by_id(global_db, entries[i].owner_id,
                              owner_username, sizeof(owner_username)) != 0) {
            // If lookup fails, show "unknown"
            strcpy(owner_username, "unknown");
        }

        cJSON* item = cJSON_CreateObject();
        cJSON_AddNumberToObject(item, "id", entries[i].id);
        cJSON_AddStringToObject(item, "name", entries[i].name);
        cJSON_AddBoolToObject(item, "is_directory", entries[i].is_directory);
        cJSON_AddNumberToObject(item, "size", entries[i].size);
        cJSON_AddNumberToObject(item, "permissions", entries[i].permissions);
        cJSON_AddNumberToObject(item, "owner_id", entries[i].owner_id);
        cJSON_AddStringToObject(item, "owner", owner_username);
        cJSON_AddItemToArray(files_array, item);
    }

    char* payload = cJSON_PrintUnformatted(response);
    send_success(session, CMD_LIST_DIR, payload);

    free(entries);
    free(payload);
    if (json) cJSON_Delete(json);
    cJSON_Delete(response);

    db_log_activity(global_db, session->user_id, "LIST_DIR", NULL);
}

void handle_mkdir(ClientSession* session, Packet* pkt) {
    log_info("handle_mkdir called for user_id=%d", session->user_id);

    cJSON* json = cJSON_Parse(pkt->payload);
    if (!json) {
        log_error("handle_mkdir: Invalid JSON payload");
        send_error(session, "Invalid JSON");
        return;
    }

    cJSON* name_item = cJSON_GetObjectItem(json, "name");
    if (!name_item) {
        log_error("handle_mkdir: Missing 'name' parameter");
        send_error(session, "Missing 'name' parameter");
        cJSON_Delete(json);
        return;
    }

    const char* name = cJSON_GetStringValue(name_item);
    int parent_id = session->current_directory;

    log_info("handle_mkdir: name='%s', parent_id=%d", name, parent_id);

    // Allow override of parent directory
    cJSON* parent_item = cJSON_GetObjectItem(json, "parent_id");
    if (parent_item) {
        parent_id = parent_item->valueint;
        log_info("handle_mkdir: parent_id overridden to %d", parent_id);
    }

    // Check WRITE permission on parent directory
    if (!check_permission(global_db, session->user_id, parent_id, ACCESS_WRITE)) {
        log_error("handle_mkdir: Permission denied for user %d on parent %d", session->user_id, parent_id);
        send_error(session, "Permission denied");
        db_log_activity(global_db, session->user_id, "ACCESS_DENIED", "MKDIR");
        cJSON_Delete(json);
        return;
    }

    log_info("handle_mkdir: Permission check passed, creating directory");

    // Create directory entry in database (no physical path for directories)
    int new_dir_id = db_create_file(global_db, parent_id, name, "",
                                     session->user_id, 0, 1, 0755);

    if (new_dir_id < 0) {
        log_error("handle_mkdir: db_create_file failed, returned %d", new_dir_id);
        send_error(session, "Failed to create directory");
        cJSON_Delete(json);
        return;
    }

    log_info("handle_mkdir: Successfully created directory with id=%d", new_dir_id);

    cJSON* response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "status", "OK");
    cJSON_AddNumberToObject(response, "directory_id", new_dir_id);
    cJSON_AddStringToObject(response, "name", name);

    char* payload = cJSON_PrintUnformatted(response);
    send_success(session, CMD_SUCCESS, payload);

    free(payload);
    cJSON_Delete(json);
    cJSON_Delete(response);

    db_log_activity(global_db, session->user_id, "MAKE_DIR", name);
}

void handle_upload_req(ClientSession* session, Packet* pkt) {
    cJSON* json = cJSON_Parse(pkt->payload);
    if (!json) {
        send_error(session, "Invalid JSON");
        return;
    }

    cJSON* name_item = cJSON_GetObjectItem(json, "name");
    cJSON* size_item = cJSON_GetObjectItem(json, "size");

    if (!name_item || !size_item) {
        send_error(session, "Missing 'name' or 'size' parameter");
        cJSON_Delete(json);
        return;
    }

    const char* name = cJSON_GetStringValue(name_item);
    long size = size_item->valueint;
    int parent_id = session->current_directory;

    // Allow override of parent directory
    cJSON* parent_item = cJSON_GetObjectItem(json, "parent_id");
    if (parent_item) {
        parent_id = parent_item->valueint;
    }

    // Check WRITE permission on parent directory
    if (!check_permission(global_db, session->user_id, parent_id, ACCESS_WRITE)) {
        send_error(session, "Permission denied");
        db_log_activity(global_db, session->user_id, "ACCESS_DENIED", "UPLOAD");
        cJSON_Delete(json);
        return;
    }

    // Generate UUID for file storage
    char* uuid = generate_uuid();
    if (!uuid) {
        send_error(session, "Failed to generate UUID");
        cJSON_Delete(json);
        return;
    }

    // Create file entry in database
    int file_id = db_create_file(global_db, parent_id, name, uuid,
                                  session->user_id, size, 0, 0644);

    if (file_id < 0) {
        send_error(session, "Failed to create file entry");
        free(uuid);
        cJSON_Delete(json);
        return;
    }

    // Store UUID and size in session for upcoming upload
    if (session->pending_upload_uuid) {
        free(session->pending_upload_uuid);
    }
    session->pending_upload_uuid = uuid;
    session->pending_upload_size = size;
    session->state = STATE_TRANSFERRING;

    // Send READY response with file_id
    cJSON* response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "status", "READY");
    cJSON_AddNumberToObject(response, "file_id", file_id);
    cJSON_AddStringToObject(response, "uuid", uuid);

    char* payload = cJSON_PrintUnformatted(response);
    send_success(session, CMD_SUCCESS, payload);

    free(payload);
    cJSON_Delete(json);
    cJSON_Delete(response);

    log_info("Upload request accepted: file_id=%d, uuid=%s, size=%ld", file_id, uuid, size);
}

void handle_upload_data(ClientSession* session, Packet* pkt) {
    // Check that upload request was made
    if (!session->pending_upload_uuid) {
        send_error(session, "No pending upload. Send UPLOAD_REQ first");
        return;
    }

    // Verify payload exists and size matches
    if (!pkt->payload || pkt->data_length == 0) {
        send_error(session, "Empty upload data");
        free(session->pending_upload_uuid);
        session->pending_upload_uuid = NULL;
        session->state = STATE_AUTHENTICATED;
        return;
    }

    if (pkt->data_length != (uint32_t)session->pending_upload_size) {
        char error_msg[128];
        snprintf(error_msg, sizeof(error_msg),
                "Size mismatch. Expected %ld bytes, got %u bytes",
                session->pending_upload_size, pkt->data_length);
        send_error(session, error_msg);
        free(session->pending_upload_uuid);
        session->pending_upload_uuid = NULL;
        session->state = STATE_AUTHENTICATED;
        return;
    }

    // Write file to storage
    if (storage_write_file(session->pending_upload_uuid,
                          (uint8_t*)pkt->payload,
                          pkt->data_length) < 0) {
        send_error(session, "Failed to write file to storage");
        free(session->pending_upload_uuid);
        session->pending_upload_uuid = NULL;
        session->state = STATE_AUTHENTICATED;
        return;
    }

    // Log activity
    db_log_activity(global_db, session->user_id, "UPLOAD",
                   session->pending_upload_uuid);

    // Send success response
    cJSON* response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "status", "OK");
    cJSON_AddStringToObject(response, "message", "File uploaded successfully");

    char* payload = cJSON_PrintUnformatted(response);
    send_success(session, CMD_SUCCESS, payload);

    free(payload);
    cJSON_Delete(response);

    log_info("Upload completed: uuid=%s, size=%ld",
             session->pending_upload_uuid, session->pending_upload_size);

    // Clear pending upload
    free(session->pending_upload_uuid);
    session->pending_upload_uuid = NULL;
    session->pending_upload_size = 0;
    session->state = STATE_AUTHENTICATED;
}

void handle_download(ClientSession* session, Packet* pkt) {
    cJSON* json = cJSON_Parse(pkt->payload);
    if (!json) {
        send_error(session, "Invalid JSON");
        return;
    }

    cJSON* file_id_item = cJSON_GetObjectItem(json, "file_id");
    if (!file_id_item) {
        send_error(session, "Missing 'file_id' parameter");
        cJSON_Delete(json);
        return;
    }

    int file_id = file_id_item->valueint;

    // Check READ permission on file
    if (!check_permission(global_db, session->user_id, file_id, ACCESS_READ)) {
        send_error(session, "Permission denied");
        db_log_activity(global_db, session->user_id, "ACCESS_DENIED", "DOWNLOAD");
        cJSON_Delete(json);
        return;
    }

    // Get file entry from database
    FileEntry entry;
    if (db_get_file_by_id(global_db, file_id, &entry) < 0) {
        send_error(session, "File not found");
        cJSON_Delete(json);
        return;
    }

    // Check it's not a directory
    if (entry.is_directory) {
        send_error(session, "Cannot download a directory");
        cJSON_Delete(json);
        return;
    }

    // Read file from storage
    uint8_t* data = NULL;
    size_t size = 0;
    if (storage_read_file(entry.physical_path, &data, &size) < 0) {
        send_error(session, "Failed to read file from storage");
        cJSON_Delete(json);
        return;
    }

    // Send binary data with CMD_DOWNLOAD_RES
    Packet* response = packet_create(CMD_DOWNLOAD_RES, (char*)data, size);
    packet_send(session->client_socket, response);
    packet_free(response);

    free(data);
    cJSON_Delete(json);

    db_log_activity(global_db, session->user_id, "DOWNLOAD", entry.name);
    log_info("Download completed: file_id=%d, name=%s, size=%zu", file_id, entry.name, size);
}

void handle_change_dir(ClientSession* session, Packet* pkt) {
    cJSON* json = cJSON_Parse(pkt->payload);
    if (!json) {
        send_error(session, "Invalid JSON");
        return;
    }

    cJSON* dir_id_item = cJSON_GetObjectItem(json, "directory_id");
    if (!dir_id_item) {
        send_error(session, "Missing 'directory_id' parameter");
        cJSON_Delete(json);
        return;
    }

    int dir_id = dir_id_item->valueint;

    // Check EXECUTE permission on directory
    if (!check_permission(global_db, session->user_id, dir_id, ACCESS_EXECUTE)) {
        send_error(session, "Permission denied");
        db_log_activity(global_db, session->user_id, "ACCESS_DENIED", "CD");
        cJSON_Delete(json);
        return;
    }

    // Verify the directory exists and is actually a directory
    FileEntry entry;
    if (db_get_file_by_id(global_db, dir_id, &entry) < 0) {
        send_error(session, "Directory not found");
        cJSON_Delete(json);
        return;
    }

    if (!entry.is_directory) {
        send_error(session, "Not a directory");
        cJSON_Delete(json);
        return;
    }

    // Update session's current directory
    session->current_directory = dir_id;

    cJSON* response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "status", "OK");
    cJSON_AddNumberToObject(response, "directory_id", dir_id);
    cJSON_AddStringToObject(response, "name", entry.name);

    char* payload = cJSON_PrintUnformatted(response);
    send_success(session, CMD_SUCCESS, payload);

    free(payload);
    cJSON_Delete(json);
    cJSON_Delete(response);

    db_log_activity(global_db, session->user_id, "CHANGE_DIR", entry.name);
    log_info("Changed directory: user_id=%d, dir_id=%d, name=%s",
             session->user_id, dir_id, entry.name);
}

void handle_chmod(ClientSession* session, Packet* pkt) {
    if (!pkt->payload) {
        send_error(session, "Empty payload");
        return;
    }

    cJSON* json = cJSON_Parse(pkt->payload);
    if (!json) {
        send_error(session, "Invalid JSON");
        return;
    }

    cJSON* file_id_item = cJSON_GetObjectItem(json, "file_id");
    cJSON* perms_item = cJSON_GetObjectItem(json, "permissions");

    if (!file_id_item || !perms_item) {
        send_error(session, "Missing file_id or permissions");
        cJSON_Delete(json);
        return;
    }

    int file_id = file_id_item->valueint;
    int new_perms;

    // Handle string or number permissions
    if (cJSON_IsString(perms_item)) {
        new_perms = parse_permissions(cJSON_GetStringValue(perms_item));
    } else {
        new_perms = perms_item->valueint;
    }

    if (new_perms < 0 || new_perms > 0777) {
        send_error(session, "Invalid permissions value");
        cJSON_Delete(json);
        return;
    }

    // Get file entry
    FileEntry entry;
    if (db_get_file_by_id(global_db, file_id, &entry) < 0) {
        send_error(session, "File not found");
        cJSON_Delete(json);
        return;
    }

    // Only owner can change permissions
    if (entry.owner_id != session->user_id) {
        send_error(session, "Not owner");
        db_log_activity(global_db, session->user_id, "ACCESS_DENIED", "CHMOD - not owner");
        cJSON_Delete(json);
        return;
    }

    // Update permissions
    if (db_update_permissions(global_db, file_id, new_perms) < 0) {
        send_error(session, "Failed to update permissions");
        cJSON_Delete(json);
        return;
    }

    char* perm_str = format_permissions(new_perms);
    log_info("User %d changed permissions on file %d to %03o (%s)",
             session->user_id, file_id, new_perms, perm_str);

    cJSON* response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "status", "OK");
    cJSON_AddNumberToObject(response, "permissions", new_perms);
    cJSON_AddStringToObject(response, "permissions_str", perm_str);

    char* payload = cJSON_PrintUnformatted(response);
    send_success(session, CMD_SUCCESS, payload);

    db_log_activity(global_db, session->user_id, "CHMOD", entry.name);

    free(perm_str);
    free(payload);
    cJSON_Delete(json);
    cJSON_Delete(response);
}

void handle_delete(ClientSession* session, Packet* pkt) {
    if (!session->authenticated) {
        send_error(session, "Not authenticated");
        return;
    }

    cJSON* json = cJSON_Parse(pkt->payload);
    if (!json) {
        send_error(session, "Invalid request format");
        return;
    }

    cJSON* file_id_obj = cJSON_GetObjectItem(json, "file_id");
    if (!file_id_obj) {
        send_error(session, "Missing file_id");
        cJSON_Delete(json);
        return;
    }

    int file_id = file_id_obj->valueint;

    // Get file info to check ownership and get name for logging
    FileEntry entry;
    if (db_get_file_by_id(global_db, file_id, &entry) < 0) {
        send_error(session, "File not found");
        cJSON_Delete(json);
        return;
    }

    // Check if user owns the file
    if (entry.owner_id != session->user_id) {
        send_error(session, "Permission denied: not file owner");
        cJSON_Delete(json);
        return;
    }

    // Delete the file from database
    if (db_delete_file(global_db, file_id) < 0) {
        send_error(session, "Failed to delete file");
        cJSON_Delete(json);
        return;
    }

    // If it's a regular file (not directory), delete physical file
    if (!entry.is_directory && entry.physical_path[0] != '\0') {
        unlink(entry.physical_path);  // Delete physical file, ignore errors
    }

    log_info("User %d deleted %s (ID: %d)",
             session->user_id, entry.name, file_id);

    cJSON* response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "status", "OK");
    cJSON_AddStringToObject(response, "message", "File deleted successfully");

    char* payload = cJSON_PrintUnformatted(response);
    send_success(session, CMD_SUCCESS, payload);

    db_log_activity(global_db, session->user_id, "DELETE", entry.name);

    free(payload);
    cJSON_Delete(json);
    cJSON_Delete(response);
}

void handle_file_info(ClientSession* session, Packet* pkt) {
    if (!session->authenticated) {
        send_error(session, "Not authenticated");
        return;
    }

    cJSON* json = cJSON_Parse(pkt->payload);
    if (!json) {
        send_error(session, "Invalid request format");
        return;
    }

    cJSON* file_id_obj = cJSON_GetObjectItem(json, "file_id");
    if (!file_id_obj) {
        send_error(session, "Missing file_id");
        cJSON_Delete(json);
        return;
    }

    int file_id = file_id_obj->valueint;

    // Get file information
    FileEntry entry;
    if (db_get_file_by_id(global_db, file_id, &entry) < 0) {
        send_error(session, "File not found");
        cJSON_Delete(json);
        return;
    }

    // Build detailed response
    cJSON* response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "status", "OK");
    cJSON_AddNumberToObject(response, "id", entry.id);
    cJSON_AddStringToObject(response, "name", entry.name);
    cJSON_AddStringToObject(response, "type", entry.is_directory ? "directory" : "file");
    cJSON_AddNumberToObject(response, "size", entry.size);
    cJSON_AddNumberToObject(response, "owner_id", entry.owner_id);
    cJSON_AddNumberToObject(response, "parent_id", entry.parent_id);
    cJSON_AddNumberToObject(response, "permissions", entry.permissions);

    // Format permissions as readable string
    char* perm_str = format_permissions(entry.permissions);
    cJSON_AddStringToObject(response, "permissions_str", perm_str);

    cJSON_AddStringToObject(response, "created_at", entry.created_at);

    if (!entry.is_directory && entry.physical_path[0] != '\0') {
        cJSON_AddStringToObject(response, "physical_path", entry.physical_path);
    }

    char* payload = cJSON_PrintUnformatted(response);
    send_success(session, CMD_SUCCESS, payload);

    free(perm_str);
    free(payload);
    cJSON_Delete(json);
    cJSON_Delete(response);
}

// Admin command handlers
void handle_admin_list_users(ClientSession* session, Packet* pkt) {
    // Check admin authorization
    if (!db_is_admin(global_db, session->user_id)) {
        send_error(session, "Admin access required");
        log_info("Non-admin user %d attempted to list users", session->user_id);
        return;
    }

    char* json_result = NULL;
    if (db_list_users(global_db, &json_result) < 0) {
        send_error(session, "Failed to retrieve user list");
        return;
    }

    // Build response
    cJSON* response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "status", "OK");
    cJSON* users_array = cJSON_Parse(json_result);
    cJSON_AddItemToObject(response, "users", users_array);

    char* payload = cJSON_PrintUnformatted(response);
    send_success(session, CMD_SUCCESS, payload);

    log_info("Admin user %d listed all users", session->user_id);
    db_log_activity(global_db, session->user_id, "ADMIN_LIST_USERS", "Listed all users");

    free(json_result);
    free(payload);
    cJSON_Delete(response);
}

void handle_admin_create_user(ClientSession* session, Packet* pkt) {
    // Check admin authorization
    if (!db_is_admin(global_db, session->user_id)) {
        send_error(session, "Admin access required");
        log_info("Non-admin user %d attempted to create user", session->user_id);
        return;
    }

    if (!pkt->payload) {
        send_error(session, "Empty payload");
        return;
    }

    cJSON* json = cJSON_Parse(pkt->payload);
    if (!json) {
        send_error(session, "Invalid JSON");
        return;
    }

    cJSON* username_item = cJSON_GetObjectItem(json, "username");
    cJSON* password_item = cJSON_GetObjectItem(json, "password");
    cJSON* is_admin_item = cJSON_GetObjectItem(json, "is_admin");

    if (!username_item || !password_item) {
        send_error(session, "Missing username or password");
        cJSON_Delete(json);
        return;
    }

    const char* username = cJSON_GetStringValue(username_item);
    const char* password = cJSON_GetStringValue(password_item);
    int is_admin = is_admin_item ? is_admin_item->valueint : 0;

    // Input validation
    if (strlen(username) < 3 || strlen(username) > 32) {
        send_error(session, "Username must be 3-32 characters");
        cJSON_Delete(json);
        return;
    }

    if (strlen(password) < 4) {
        send_error(session, "Password must be at least 4 characters");
        cJSON_Delete(json);
        return;
    }

    // Check if user already exists
    if (db_user_exists(global_db, username)) {
        send_error(session, "Username already exists");
        cJSON_Delete(json);
        return;
    }

    // Hash password and create user
    char* password_hash = hash_password(password);
    if (!password_hash) {
        send_error(session, "Internal error");
        cJSON_Delete(json);
        return;
    }

    int new_user_id = db_create_user_admin(global_db, username, password_hash, is_admin);
    free(password_hash);

    if (new_user_id < 0) {
        send_error(session, "Failed to create user");
        cJSON_Delete(json);
        return;
    }

    // Build success response
    cJSON* response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "status", "OK");
    cJSON_AddNumberToObject(response, "user_id", new_user_id);
    cJSON_AddStringToObject(response, "username", username);

    char* payload = cJSON_PrintUnformatted(response);
    send_success(session, CMD_SUCCESS, payload);

    log_info("Admin user %d created new user: %s (id=%d, is_admin=%d)",
             session->user_id, username, new_user_id, is_admin);

    char log_desc[256];
    snprintf(log_desc, sizeof(log_desc), "Created user '%s' (id=%d, is_admin=%d)",
             username, new_user_id, is_admin);
    db_log_activity(global_db, session->user_id, "ADMIN_CREATE_USER", log_desc);

    free(payload);
    cJSON_Delete(json);
    cJSON_Delete(response);
}

void handle_admin_delete_user(ClientSession* session, Packet* pkt) {
    // Check admin authorization
    if (!db_is_admin(global_db, session->user_id)) {
        send_error(session, "Admin access required");
        log_info("Non-admin user %d attempted to delete user", session->user_id);
        return;
    }

    if (!pkt->payload) {
        send_error(session, "Empty payload");
        return;
    }

    cJSON* json = cJSON_Parse(pkt->payload);
    if (!json) {
        send_error(session, "Invalid JSON");
        return;
    }

    cJSON* user_id_item = cJSON_GetObjectItem(json, "user_id");
    if (!user_id_item) {
        send_error(session, "Missing user_id");
        cJSON_Delete(json);
        return;
    }

    int target_user_id = user_id_item->valueint;

    // Safety check: prevent admin from deleting themselves
    if (target_user_id == session->user_id) {
        send_error(session, "Cannot delete your own account");
        cJSON_Delete(json);
        return;
    }

    // Get username for logging before deletion
    char username[256] = {0};
    db_get_user_by_id(global_db, target_user_id, username, sizeof(username));

    // Delete user (db_delete_user has built-in protection for user ID 1)
    if (db_delete_user(global_db, target_user_id) < 0) {
        send_error(session, "Failed to delete user");
        cJSON_Delete(json);
        return;
    }

    // Build success response
    cJSON* response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "status", "OK");
    cJSON_AddStringToObject(response, "message", "User deleted successfully");

    char* payload = cJSON_PrintUnformatted(response);
    send_success(session, CMD_SUCCESS, payload);

    log_info("Admin user %d deleted user: %s (id=%d)", session->user_id, username, target_user_id);

    char log_desc[256];
    snprintf(log_desc, sizeof(log_desc), "Deleted user '%s' (id=%d)", username, target_user_id);
    db_log_activity(global_db, session->user_id, "ADMIN_DELETE_USER", log_desc);

    free(payload);
    cJSON_Delete(json);
    cJSON_Delete(response);
}

void handle_admin_update_user(ClientSession* session, Packet* pkt) {
    // Check admin authorization
    if (!db_is_admin(global_db, session->user_id)) {
        send_error(session, "Admin access required");
        log_info("Non-admin user %d attempted to update user", session->user_id);
        return;
    }

    if (!pkt->payload) {
        send_error(session, "Empty payload");
        return;
    }

    cJSON* json = cJSON_Parse(pkt->payload);
    if (!json) {
        send_error(session, "Invalid JSON");
        return;
    }

    cJSON* user_id_item = cJSON_GetObjectItem(json, "user_id");
    cJSON* is_admin_item = cJSON_GetObjectItem(json, "is_admin");
    cJSON* is_active_item = cJSON_GetObjectItem(json, "is_active");

    if (!user_id_item) {
        send_error(session, "Missing user_id");
        cJSON_Delete(json);
        return;
    }

    int target_user_id = user_id_item->valueint;
    int is_admin = is_admin_item ? is_admin_item->valueint : 0;
    int is_active = is_active_item ? is_active_item->valueint : 1;

    // Get username for logging
    char username[256] = {0};
    db_get_user_by_id(global_db, target_user_id, username, sizeof(username));

    // Update user (db_update_user has built-in protection for user ID 1)
    if (db_update_user(global_db, target_user_id, is_admin, is_active) < 0) {
        send_error(session, "Failed to update user");
        cJSON_Delete(json);
        return;
    }

    // Build success response
    cJSON* response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "status", "OK");
    cJSON_AddStringToObject(response, "message", "User updated successfully");

    char* payload = cJSON_PrintUnformatted(response);
    send_success(session, CMD_SUCCESS, payload);

    log_info("Admin user %d updated user: %s (id=%d, is_admin=%d, is_active=%d)",
             session->user_id, username, target_user_id, is_admin, is_active);

    char log_desc[256];
    snprintf(log_desc, sizeof(log_desc), "Updated user '%s' (id=%d, is_admin=%d, is_active=%d)",
             username, target_user_id, is_admin, is_active);
    db_log_activity(global_db, session->user_id, "ADMIN_UPDATE_USER", log_desc);

    free(payload);
    cJSON_Delete(json);
    cJSON_Delete(response);
}

// Helper: Build full VFS path by traversing parent_id chain
static void build_full_path(Database* db, int file_id, char* path, size_t size) {
    char components[32][256];
    int depth = 0;
    int current_id = file_id;

    // Traverse up to root
    while (current_id > 0 && depth < 32) {
        FileEntry entry;
        if (db_get_file_by_id(db, current_id, &entry) != 0) break;

        strncpy(components[depth++], entry.name, 255);
        components[depth-1][255] = '\0';
        current_id = entry.parent_id;
    }

    // Build path from root down
    path[0] = '\0';
    for (int i = depth - 1; i >= 0; i--) {
        if (i == depth - 1 && strcmp(components[i], "/") == 0) {
            // Skip root name if it's "/"
            continue;
        }
        if (strlen(path) > 0 && path[strlen(path)-1] != '/') {
            strncat(path, "/", size - strlen(path) - 1);
        }
        strncat(path, components[i], size - strlen(path) - 1);
    }

    // Ensure leading slash
    if (path[0] != '/' && strlen(path) > 0) {
        memmove(path + 1, path, strlen(path) + 1);
        path[0] = '/';
    } else if (path[0] == '\0') {
        strcpy(path, "/");
    }
}

// Handler: Search files
void handle_search(ClientSession* session, Packet* pkt) {
    if (!pkt->payload) {
        send_error(session, "Empty payload");
        return;
    }

    cJSON* json = cJSON_Parse(pkt->payload);
    if (!json) {
        send_error(session, "Invalid JSON");
        return;
    }

    // Extract parameters
    cJSON* pattern_item = cJSON_GetObjectItem(json, "pattern");
    cJSON* dir_item = cJSON_GetObjectItem(json, "directory_id");
    cJSON* recursive_item = cJSON_GetObjectItem(json, "recursive");
    cJSON* limit_item = cJSON_GetObjectItem(json, "limit");

    if (!pattern_item || !dir_item) {
        send_error(session, "Missing required fields");
        cJSON_Delete(json);
        return;
    }

    const char* pattern = cJSON_GetStringValue(pattern_item);
    int directory_id = dir_item->valueint;
    int recursive = recursive_item ? (recursive_item->valueint != 0) : 0;
    int limit = limit_item ? limit_item->valueint : 100;

    // Validate limit
    if (limit <= 0 || limit > 1000) {
        limit = 100;
    }

    // Validate pattern
    if (!pattern || strlen(pattern) == 0) {
        send_error(session, "Invalid search pattern");
        cJSON_Delete(json);
        return;
    }

    log_info("Search request from user %d: pattern='%s', dir=%d, recursive=%d, limit=%d",
             session->user_id, pattern, directory_id, recursive, limit);

    // Perform search
    FileEntry* entries = NULL;
    int count = 0;

    int result = db_search_files(global_db, directory_id, pattern, recursive,
                                  session->user_id, limit, &entries, &count);

    if (result != 0) {
        send_error(session, "Search failed");
        cJSON_Delete(json);
        return;
    }

    // Build response
    cJSON* response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "status", "OK");
    cJSON_AddNumberToObject(response, "count", count);

    cJSON* results_array = cJSON_AddArrayToObject(response, "results");

    for (int i = 0; i < count; i++) {
        // Build full path for each result
        char full_path[1024];
        build_full_path(global_db, entries[i].id, full_path, sizeof(full_path));

        // Fetch username from owner_id
        char owner_username[256] = "unknown";
        if (db_get_user_by_id(global_db, entries[i].owner_id,
                              owner_username, sizeof(owner_username)) != 0) {
            // If lookup fails, show "unknown"
            strcpy(owner_username, "unknown");
        }

        cJSON* item = cJSON_CreateObject();
        cJSON_AddNumberToObject(item, "id", entries[i].id);
        cJSON_AddStringToObject(item, "name", entries[i].name);
        cJSON_AddNumberToObject(item, "parent_id", entries[i].parent_id);
        cJSON_AddStringToObject(item, "path", full_path);
        cJSON_AddNumberToObject(item, "size", entries[i].size);
        cJSON_AddBoolToObject(item, "is_directory", entries[i].is_directory);
        cJSON_AddNumberToObject(item, "permissions", entries[i].permissions);
        cJSON_AddNumberToObject(item, "owner_id", entries[i].owner_id);
        cJSON_AddStringToObject(item, "owner", owner_username);
        cJSON_AddStringToObject(item, "created_at", entries[i].created_at);

        cJSON_AddItemToArray(results_array, item);
    }

    char* payload = cJSON_PrintUnformatted(response);
    send_success(session, CMD_SEARCH_RES, payload);

    free(entries);
    free(payload);
    cJSON_Delete(json);
    cJSON_Delete(response);

    log_info("Search completed for user %d: pattern='%s', found=%d",
             session->user_id, pattern, count);

    char log_desc[512];
    snprintf(log_desc, sizeof(log_desc), "Searched for '%s' (recursive=%d, found=%d)",
             pattern, recursive, count);
    db_log_activity(global_db, session->user_id, "SEARCH", log_desc);
}

// Rename file or directory
void handle_rename(ClientSession* session, Packet* pkt) {
    if (!session->authenticated) {
        send_error(session, "Not authenticated");
        return;
    }

    cJSON* json = cJSON_Parse(pkt->payload);
    if (!json) {
        send_error(session, "Invalid JSON");
        return;
    }

    cJSON* file_id_obj = cJSON_GetObjectItem(json, "file_id");
    cJSON* new_name_obj = cJSON_GetObjectItem(json, "new_name");

    if (!file_id_obj || !new_name_obj) {
        cJSON_Delete(json);
        send_error(session, "Missing file_id or new_name");
        return;
    }

    int file_id = file_id_obj->valueint;
    const char* new_name = cJSON_GetStringValue(new_name_obj);

    if (!new_name || strlen(new_name) == 0 || strlen(new_name) > 255) {
        cJSON_Delete(json);
        send_error(session, "Invalid new name");
        return;
    }

    // Rename in database
    int result = db_rename_file(global_db, file_id, new_name);

    if (result < 0) {
        cJSON_Delete(json);
        send_error(session, "Failed to rename file");
        return;
    }

    // Send success response
    char success_msg[256];
    snprintf(success_msg, sizeof(success_msg), "{\"message\":\"File renamed successfully\",\"file_id\":%d,\"new_name\":\"%s\"}", file_id, new_name);
    send_success(session, CMD_SUCCESS, success_msg);

    cJSON_Delete(json);

    log_info("User %d renamed file %d to '%s'", session->user_id, file_id, new_name);

    char log_desc[256];
    snprintf(log_desc, sizeof(log_desc), "Renamed file %d to '%s'", file_id, new_name);
    db_log_activity(global_db, session->user_id, "RENAME", log_desc);
}

// Copy file or directory
void handle_copy(ClientSession* session, Packet* pkt) {
    if (!session->authenticated) {
        send_error(session, "Not authenticated");
        return;
    }

    cJSON* json = cJSON_Parse(pkt->payload);
    if (!json) {
        send_error(session, "Invalid JSON");
        return;
    }

    cJSON* source_id_obj = cJSON_GetObjectItem(json, "source_id");
    cJSON* dest_parent_obj = cJSON_GetObjectItem(json, "dest_parent_id");
    cJSON* new_name_obj = cJSON_GetObjectItem(json, "new_name");

    if (!source_id_obj || !dest_parent_obj) {
        cJSON_Delete(json);
        send_error(session, "Missing source_id or dest_parent_id");
        return;
    }

    int source_id = source_id_obj->valueint;
    int dest_parent_id = dest_parent_obj->valueint;
    const char* new_name = new_name_obj ? cJSON_GetStringValue(new_name_obj) : "";

    // Copy in database (creates new entry, physical copy would be handled separately)
    int new_id = db_copy_file(global_db, source_id, dest_parent_id, new_name, session->user_id);

    if (new_id < 0) {
        cJSON_Delete(json);
        send_error(session, "Failed to copy file");
        return;
    }

    // Send success response with new file ID
    char success_msg[256];
    snprintf(success_msg, sizeof(success_msg), "{\"message\":\"File copied successfully\",\"source_id\":%d,\"new_id\":%d}", source_id, new_id);
    send_success(session, CMD_SUCCESS, success_msg);

    cJSON_Delete(json);

    log_info("User %d copied file %d to parent %d (new id: %d)", session->user_id, source_id, dest_parent_id, new_id);

    char log_desc[256];
    snprintf(log_desc, sizeof(log_desc), "Copied file %d to parent %d (new id: %d)", source_id, dest_parent_id, new_id);
    db_log_activity(global_db, session->user_id, "COPY", log_desc);
}

// Move file or directory
void handle_move(ClientSession* session, Packet* pkt) {
    if (!session->authenticated) {
        send_error(session, "Not authenticated");
        return;
    }

    cJSON* json = cJSON_Parse(pkt->payload);
    if (!json) {
        send_error(session, "Invalid JSON");
        return;
    }

    cJSON* file_id_obj = cJSON_GetObjectItem(json, "file_id");
    cJSON* new_parent_obj = cJSON_GetObjectItem(json, "new_parent_id");

    if (!file_id_obj || !new_parent_obj) {
        cJSON_Delete(json);
        send_error(session, "Missing file_id or new_parent_id");
        return;
    }

    int file_id = file_id_obj->valueint;
    int new_parent_id = new_parent_obj->valueint;

    // Move in database
    int result = db_move_file(global_db, file_id, new_parent_id);

    if (result < 0) {
        cJSON_Delete(json);
        send_error(session, "Failed to move file");
        return;
    }

    // Send success response
    char success_msg[256];
    snprintf(success_msg, sizeof(success_msg), "{\"message\":\"File moved successfully\",\"file_id\":%d,\"new_parent_id\":%d}", file_id, new_parent_id);
    send_success(session, CMD_SUCCESS, success_msg);

    cJSON_Delete(json);

    log_info("User %d moved file %d to parent %d", session->user_id, file_id, new_parent_id);

    char log_desc[256];
    snprintf(log_desc, sizeof(log_desc), "Moved file %d to parent %d", file_id, new_parent_id);
    db_log_activity(global_db, session->user_id, "MOVE", log_desc);
}
