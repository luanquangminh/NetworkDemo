#include "client.h"
#include "net_handler.h"
#include "../common/utils.h"
#include "../common/protocol.h"
#include "../../lib/cJSON/cJSON.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>

ClientConnection* client_connect(const char* ip, int port) {
    ClientConnection* conn = malloc(sizeof(ClientConnection));
    if (!conn) return NULL;

    memset(conn, 0, sizeof(ClientConnection));
    strncpy(conn->server_ip, ip, sizeof(conn->server_ip) - 1);
    conn->server_port = port;
    strcpy(conn->current_path, "/");
    conn->current_directory = 0;

    conn->socket_fd = net_connect(ip, port);
    if (conn->socket_fd < 0) {
        free(conn);
        return NULL;
    }

    return conn;
}

void client_disconnect(ClientConnection* conn) {
    if (conn) {
        if (conn->socket_fd >= 0) {
            net_disconnect(conn->socket_fd);
        }
        free(conn);
    }
}

int client_login(ClientConnection* conn, const char* username, const char* password) {
    if (!conn || !username || !password) return -1;

    cJSON* json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "username", username);
    cJSON_AddStringToObject(json, "password", password);

    char* payload = cJSON_PrintUnformatted(json);
    Packet* pkt = packet_create(CMD_LOGIN_REQ, payload, strlen(payload));

    int result = packet_send(conn->socket_fd, pkt);

    free(payload);
    packet_free(pkt);
    cJSON_Delete(json);

    if (result < 0) return -1;

    Packet* response = net_recv_packet(conn->socket_fd);
    if (!response) return -1;

    cJSON* resp_json = cJSON_Parse(response->payload);
    if (!resp_json) {
        packet_free(response);
        return -1;
    }

    cJSON* status = cJSON_GetObjectItem(resp_json, "status");
    result = -1;

    if (status && strcmp(cJSON_GetStringValue(status), "OK") == 0) {
        conn->authenticated = 1;
        cJSON* user_id = cJSON_GetObjectItem(resp_json, "user_id");
        if (user_id) conn->user_id = user_id->valueint;

        // Extract is_admin flag
        cJSON* is_admin = cJSON_GetObjectItem(resp_json, "is_admin");
        conn->is_admin = (is_admin && is_admin->valueint == 1) ? 1 : 0;

        result = 0;
        printf("Login successful! User ID: %d, Admin: %s\n", conn->user_id, conn->is_admin ? "Yes" : "No");
    } else {
        cJSON* message = cJSON_GetObjectItem(resp_json, "message");
        if (message) {
            printf("Login failed: %s\n", cJSON_GetStringValue(message));
        }
    }

    cJSON_Delete(resp_json);
    packet_free(response);

    return result;
}

int client_list_dir(ClientConnection* conn, int dir_id) {
    if (!conn || !conn->authenticated) return -1;

    cJSON* json = cJSON_CreateObject();
    cJSON_AddNumberToObject(json, "user_id", conn->user_id);
    cJSON_AddNumberToObject(json, "directory_id", dir_id);

    char* payload = cJSON_PrintUnformatted(json);
    Packet* pkt = packet_create(CMD_LIST_DIR, payload, strlen(payload));

    int result = packet_send(conn->socket_fd, pkt);

    free(payload);
    packet_free(pkt);
    cJSON_Delete(json);

    if (result < 0) return -1;

    Packet* response = net_recv_packet(conn->socket_fd);
    if (!response) return -1;

    cJSON* resp_json = cJSON_Parse(response->payload);
    if (!resp_json) {
        packet_free(response);
        return -1;
    }

    cJSON* files = cJSON_GetObjectItem(resp_json, "files");
    if (files) {
        printf("\n%-6s %-4s %-30s %-10s %-10s\n", "ID", "Type", "Name", "Size", "Perms");
        printf("-------------------------------------------------------------------\n");

        cJSON* file;
        cJSON_ArrayForEach(file, files) {
            int id = cJSON_GetObjectItem(file, "id")->valueint;
            int is_dir = cJSON_GetObjectItem(file, "is_directory")->valueint;
            const char* name = cJSON_GetStringValue(cJSON_GetObjectItem(file, "name"));
            int size = cJSON_GetObjectItem(file, "size")->valueint;
            int perms = cJSON_GetObjectItem(file, "permissions")->valueint;

            printf("%-6d %-4s %-30s %-10d %03o\n",
                   id, is_dir ? "DIR" : "FILE", name, is_dir ? 0 : size, perms);
        }
        printf("\n");
        result = 0;
    } else {
        printf("Error: Unable to list directory\n");
        result = -1;
    }

    cJSON_Delete(resp_json);
    packet_free(response);

    return result;
}

void* client_list_dir_gui(ClientConnection* conn, int dir_id) {
    if (!conn || !conn->authenticated) return NULL;

    cJSON* json = cJSON_CreateObject();
    cJSON_AddNumberToObject(json, "user_id", conn->user_id);
    cJSON_AddNumberToObject(json, "directory_id", dir_id);

    char* payload = cJSON_PrintUnformatted(json);
    Packet* pkt = packet_create(CMD_LIST_DIR, payload, strlen(payload));

    int result = packet_send(conn->socket_fd, pkt);

    free(payload);
    packet_free(pkt);
    cJSON_Delete(json);

    if (result < 0) return NULL;

    Packet* response = net_recv_packet(conn->socket_fd);
    if (!response) return NULL;

    cJSON* resp_json = cJSON_Parse(response->payload);
    packet_free(response);

    if (!resp_json) return NULL;

    // Return the parsed JSON for GUI to use
    // GUI must call cJSON_Delete() when done
    return resp_json;
}

int client_mkdir(ClientConnection* conn, const char* name) {
    if (!conn || !conn->authenticated || !name) return -1;

    cJSON* json = cJSON_CreateObject();
    cJSON_AddNumberToObject(json, "user_id", conn->user_id);
    cJSON_AddNumberToObject(json, "parent_id", conn->current_directory);
    cJSON_AddStringToObject(json, "name", name);

    char* payload = cJSON_PrintUnformatted(json);
    Packet* pkt = packet_create(CMD_MAKE_DIR, payload, strlen(payload));

    int result = packet_send(conn->socket_fd, pkt);

    free(payload);
    packet_free(pkt);
    cJSON_Delete(json);

    if (result < 0) return -1;

    Packet* response = net_recv_packet(conn->socket_fd);
    if (!response) return -1;

    if (response->command == CMD_SUCCESS) {
        // Parse response to get new directory ID
        cJSON* resp_json = cJSON_Parse(response->payload);
        if (resp_json) {
            cJSON* dir_id_obj = cJSON_GetObjectItem(resp_json, "directory_id");
            if (dir_id_obj) {
                result = dir_id_obj->valueint;  // Return the new directory ID
                printf("Directory '%s' created successfully with ID %d\n", name, result);
            } else {
                result = 0;  // Fallback to 0 for success
            }
            cJSON_Delete(resp_json);
        } else {
            result = 0;  // Fallback to 0 for success
        }
        printf("Directory '%s' created successfully\n", name);
    } else {
        cJSON* resp_json = cJSON_Parse(response->payload);
        if (resp_json) {
            cJSON* message = cJSON_GetObjectItem(resp_json, "message");
            if (message) {
                printf("Error: %s\n", cJSON_GetStringValue(message));
            }
            cJSON_Delete(resp_json);
        }
        printf("Error: Failed to create directory\n");
        result = -1;
    }

    packet_free(response);
    return result;
}

int client_cd(ClientConnection* conn, int dir_id) {
    if (!conn || !conn->authenticated) return -1;

    cJSON* json = cJSON_CreateObject();
    cJSON_AddNumberToObject(json, "user_id", conn->user_id);
    cJSON_AddNumberToObject(json, "directory_id", dir_id);

    char* payload = cJSON_PrintUnformatted(json);
    Packet* pkt = packet_create(CMD_CHANGE_DIR, payload, strlen(payload));

    int result = packet_send(conn->socket_fd, pkt);

    free(payload);
    packet_free(pkt);
    cJSON_Delete(json);

    if (result < 0) return -1;

    Packet* response = net_recv_packet(conn->socket_fd);
    if (!response) return -1;

    if (response->command == CMD_SUCCESS) {
        conn->current_directory = dir_id;
        cJSON* resp_json = cJSON_Parse(response->payload);
        if (resp_json) {
            cJSON* path = cJSON_GetObjectItem(resp_json, "path");
            if (path) {
                strncpy(conn->current_path, cJSON_GetStringValue(path), sizeof(conn->current_path) - 1);
            }
            cJSON_Delete(resp_json);
        }
        printf("Changed to directory: %s\n", conn->current_path);
        result = 0;
    } else {
        printf("Error: Unable to change directory\n");
        result = -1;
    }

    packet_free(response);
    return result;
}

int client_upload(ClientConnection* conn, const char* local_path) {
    if (!conn || !conn->authenticated || !local_path) return -1;

    struct stat st;
    if (stat(local_path, &st) != 0) {
        printf("Error: File not found: %s\n", local_path);
        return -1;
    }

    const char* filename = strrchr(local_path, '/');
    filename = filename ? filename + 1 : local_path;

    cJSON* json = cJSON_CreateObject();
    cJSON_AddNumberToObject(json, "user_id", conn->user_id);
    cJSON_AddNumberToObject(json, "parent_id", conn->current_directory);
    cJSON_AddStringToObject(json, "name", filename);
    cJSON_AddNumberToObject(json, "size", st.st_size);

    char* payload = cJSON_PrintUnformatted(json);
    Packet* pkt = packet_create(CMD_UPLOAD_REQ, payload, strlen(payload));

    int result = packet_send(conn->socket_fd, pkt);

    free(payload);
    packet_free(pkt);
    cJSON_Delete(json);

    if (result < 0) return -1;

    Packet* response = net_recv_packet(conn->socket_fd);
    if (!response || response->command != CMD_SUCCESS) {
        if (response) packet_free(response);
        printf("Error: Upload request rejected\n");
        return -1;
    }
    packet_free(response);

    printf("Uploading file '%s' (%lld bytes)...\n", filename, (long long)st.st_size);

    if (net_send_file(conn->socket_fd, local_path) < 0) {
        printf("Error: File transfer failed\n");
        return -1;
    }

    response = net_recv_packet(conn->socket_fd);
    if (!response || response->command != CMD_SUCCESS) {
        if (response) packet_free(response);
        printf("Error: Upload failed\n");
        return -1;
    }

    printf("Upload successful!\n");
    packet_free(response);
    return 0;
}

int client_download(ClientConnection* conn, int file_id, const char* local_path) {
    if (!conn || !conn->authenticated || !local_path) return -1;

    cJSON* json = cJSON_CreateObject();
    cJSON_AddNumberToObject(json, "user_id", conn->user_id);
    cJSON_AddNumberToObject(json, "file_id", file_id);

    char* payload = cJSON_PrintUnformatted(json);
    Packet* pkt = packet_create(CMD_DOWNLOAD_REQ, payload, strlen(payload));

    int result = packet_send(conn->socket_fd, pkt);

    free(payload);
    packet_free(pkt);
    cJSON_Delete(json);

    if (result < 0) return -1;

    Packet* response = net_recv_packet(conn->socket_fd);
    if (!response) return -1;

    if (response->command != CMD_DOWNLOAD_RES) {
        printf("Error: Download request rejected\n");
        packet_free(response);
        return -1;
    }

    cJSON* resp_json = cJSON_Parse(response->payload);
    if (!resp_json) {
        packet_free(response);
        return -1;
    }

    cJSON* size_obj = cJSON_GetObjectItem(resp_json, "size");
    cJSON* name_obj = cJSON_GetObjectItem(resp_json, "name");

    if (!size_obj) {
        cJSON_Delete(resp_json);
        packet_free(response);
        return -1;
    }

    size_t file_size = size_obj->valueint;
    const char* name = name_obj ? cJSON_GetStringValue(name_obj) : "file";

    cJSON_Delete(resp_json);
    packet_free(response);

    printf("Downloading '%s' (%zu bytes)...\n", name, file_size);

    if (net_recv_file(conn->socket_fd, local_path, file_size) < 0) {
        printf("Error: Download failed\n");
        return -1;
    }

    printf("Download successful!\n");
    return 0;
}

int client_chmod(ClientConnection* conn, int file_id, int permissions) {
    if (!conn || !conn->authenticated) return -1;

    cJSON* json = cJSON_CreateObject();
    cJSON_AddNumberToObject(json, "user_id", conn->user_id);
    cJSON_AddNumberToObject(json, "file_id", file_id);
    cJSON_AddNumberToObject(json, "permissions", permissions);

    char* payload = cJSON_PrintUnformatted(json);
    Packet* pkt = packet_create(CMD_CHMOD, payload, strlen(payload));

    int result = packet_send(conn->socket_fd, pkt);

    free(payload);
    packet_free(pkt);
    cJSON_Delete(json);

    if (result < 0) return -1;

    Packet* response = net_recv_packet(conn->socket_fd);
    if (!response) return -1;

    if (response->command == CMD_SUCCESS) {
        printf("Permissions changed successfully\n");
        result = 0;
    } else {
        printf("Error: Failed to change permissions\n");
        result = -1;
    }

    packet_free(response);
    return result;
}

int client_delete(ClientConnection* conn, int file_id) {
    if (!conn || !conn->authenticated) return -1;

    cJSON* json = cJSON_CreateObject();
    cJSON_AddNumberToObject(json, "user_id", conn->user_id);
    cJSON_AddNumberToObject(json, "file_id", file_id);

    char* payload = cJSON_PrintUnformatted(json);
    Packet* pkt = packet_create(CMD_DELETE, payload, strlen(payload));

    int result = packet_send(conn->socket_fd, pkt);

    free(payload);
    packet_free(pkt);
    cJSON_Delete(json);

    if (result < 0) return -1;

    Packet* response = net_recv_packet(conn->socket_fd);
    if (!response) return -1;

    if (response->command == CMD_SUCCESS) {
        printf("File deleted successfully\n");
        result = 0;
    } else {
        cJSON* resp_json = cJSON_Parse(response->payload);
        if (resp_json) {
            cJSON* message = cJSON_GetObjectItem(resp_json, "message");
            if (message) {
                printf("Error: %s\n", cJSON_GetStringValue(message));
            }
            cJSON_Delete(resp_json);
        } else {
            printf("Error: Failed to delete file\n");
        }
        result = -1;
    }

    packet_free(response);
    return result;
}

int client_file_info(ClientConnection* conn, int file_id) {
    if (!conn || !conn->authenticated) return -1;

    cJSON* json = cJSON_CreateObject();
    cJSON_AddNumberToObject(json, "user_id", conn->user_id);
    cJSON_AddNumberToObject(json, "file_id", file_id);

    char* payload = cJSON_PrintUnformatted(json);
    Packet* pkt = packet_create(CMD_FILE_INFO, payload, strlen(payload));

    int result = packet_send(conn->socket_fd, pkt);

    free(payload);
    packet_free(pkt);
    cJSON_Delete(json);

    if (result < 0) return -1;

    Packet* response = net_recv_packet(conn->socket_fd);
    if (!response) return -1;

    if (response->command == CMD_SUCCESS) {
        cJSON* resp_json = cJSON_Parse(response->payload);
        if (resp_json) {
            printf("\n=== File Information ===\n");
            printf("ID:          %d\n", cJSON_GetObjectItem(resp_json, "id")->valueint);
            printf("Name:        %s\n", cJSON_GetStringValue(cJSON_GetObjectItem(resp_json, "name")));
            printf("Type:        %s\n", cJSON_GetStringValue(cJSON_GetObjectItem(resp_json, "type")));
            printf("Size:        %lld bytes\n", (long long)cJSON_GetObjectItem(resp_json, "size")->valueint);
            printf("Owner ID:    %d\n", cJSON_GetObjectItem(resp_json, "owner_id")->valueint);
            printf("Parent ID:   %d\n", cJSON_GetObjectItem(resp_json, "parent_id")->valueint);
            printf("Permissions: %03o (%s)\n",
                   cJSON_GetObjectItem(resp_json, "permissions")->valueint,
                   cJSON_GetStringValue(cJSON_GetObjectItem(resp_json, "permissions_str")));
            printf("Created:     %s\n", cJSON_GetStringValue(cJSON_GetObjectItem(resp_json, "created_at")));

            cJSON* phys_path = cJSON_GetObjectItem(resp_json, "physical_path");
            if (phys_path) {
                printf("Path:        %s\n", cJSON_GetStringValue(phys_path));
            }
            printf("\n");

            cJSON_Delete(resp_json);
        }
        result = 0;
    } else {
        printf("Error: Failed to get file info\n");
        result = -1;
    }

    packet_free(response);
    return result;
}

void* client_search(ClientConnection* conn, const char* pattern, int recursive, int limit) {
    if (!conn || !conn->authenticated || !pattern) {
        fprintf(stderr, "Invalid parameters for search\n");
        return NULL;
    }

    // Validate pattern length
    if (strlen(pattern) == 0 || strlen(pattern) > 255) {
        fprintf(stderr, "Pattern must be 1-255 characters\n");
        return NULL;
    }

    // Validate limit (use default if 0 or negative, cap at 1000)
    if (limit <= 0) {
        limit = 100;  // Default limit
    } else if (limit > 1000) {
        limit = 1000;  // Maximum limit
    }

    // Build JSON request
    cJSON* request = cJSON_CreateObject();
    cJSON_AddStringToObject(request, "pattern", pattern);
    cJSON_AddNumberToObject(request, "directory_id", conn->current_directory);
    cJSON_AddBoolToObject(request, "recursive", recursive);
    cJSON_AddNumberToObject(request, "limit", limit);

    char* request_str = cJSON_PrintUnformatted(request);
    Packet* req_pkt = packet_create(CMD_SEARCH_REQ, request_str, strlen(request_str));

    int result = packet_send(conn->socket_fd, req_pkt);

    free(request_str);
    packet_free(req_pkt);
    cJSON_Delete(request);

    if (result < 0) {
        fprintf(stderr, "Failed to send search request\n");
        return NULL;
    }

    // Receive response
    Packet* res_pkt = net_recv_packet(conn->socket_fd);
    if (!res_pkt) {
        fprintf(stderr, "Failed to receive search response\n");
        return NULL;
    }

    // Handle error response
    if (res_pkt->command == CMD_ERROR) {
        cJSON* error_json = cJSON_Parse(res_pkt->payload);
        if (error_json) {
            cJSON* msg = cJSON_GetObjectItem(error_json, "message");
            if (msg) {
                fprintf(stderr, "Search error: %s\n", cJSON_GetStringValue(msg));
            }
            cJSON_Delete(error_json);
        } else {
            fprintf(stderr, "Search failed with unknown error\n");
        }
        packet_free(res_pkt);
        return NULL;
    }

    // Verify correct response command
    if (res_pkt->command != CMD_SEARCH_RES) {
        fprintf(stderr, "Unexpected response command: 0x%02x\n", res_pkt->command);
        packet_free(res_pkt);
        return NULL;
    }

    // Parse and return response JSON
    cJSON* response_json = cJSON_Parse(res_pkt->payload);
    packet_free(res_pkt);

    if (!response_json) {
        fprintf(stderr, "Failed to parse search response\n");
        return NULL;
    }

    return response_json;  // Caller must free with cJSON_Delete()
}

int client_rename(ClientConnection* conn, int file_id, const char* new_name) {
    if (!conn || !conn->authenticated || !new_name) {
        fprintf(stderr, "Invalid parameters for rename\n");
        return -1;
    }

    // Build JSON request
    cJSON* request = cJSON_CreateObject();
    cJSON_AddNumberToObject(request, "file_id", file_id);
    cJSON_AddStringToObject(request, "new_name", new_name);

    char* request_str = cJSON_PrintUnformatted(request);
    Packet* req_pkt = packet_create(CMD_RENAME, request_str, strlen(request_str));

    int result = packet_send(conn->socket_fd, req_pkt);

    free(request_str);
    packet_free(req_pkt);
    cJSON_Delete(request);

    if (result < 0) {
        fprintf(stderr, "Failed to send rename request\n");
        return -1;
    }

    // Receive response
    Packet* res_pkt = net_recv_packet(conn->socket_fd);
    if (!res_pkt) {
        fprintf(stderr, "Failed to receive rename response\n");
        return -1;
    }

    result = -1;
    if (res_pkt->command == CMD_SUCCESS) {
        printf("File renamed successfully\n");
        result = 0;
    } else if (res_pkt->command == CMD_ERROR) {
        cJSON* error_json = cJSON_Parse(res_pkt->payload);
        if (error_json) {
            cJSON* msg = cJSON_GetObjectItem(error_json, "message");
            if (msg) {
                fprintf(stderr, "Rename error: %s\n", cJSON_GetStringValue(msg));
            }
            cJSON_Delete(error_json);
        }
    }

    packet_free(res_pkt);
    return result;
}

int client_copy(ClientConnection* conn, int source_id, int dest_parent_id, const char* new_name) {
    if (!conn || !conn->authenticated) {
        fprintf(stderr, "Invalid parameters for copy\n");
        return -1;
    }

    // Build JSON request
    cJSON* request = cJSON_CreateObject();
    cJSON_AddNumberToObject(request, "source_id", source_id);
    cJSON_AddNumberToObject(request, "dest_parent_id", dest_parent_id);
    if (new_name && strlen(new_name) > 0) {
        cJSON_AddStringToObject(request, "new_name", new_name);
    }

    char* request_str = cJSON_PrintUnformatted(request);
    Packet* req_pkt = packet_create(CMD_COPY, request_str, strlen(request_str));

    int result = packet_send(conn->socket_fd, req_pkt);

    free(request_str);
    packet_free(req_pkt);
    cJSON_Delete(request);

    if (result < 0) {
        fprintf(stderr, "Failed to send copy request\n");
        return -1;
    }

    // Receive response
    Packet* res_pkt = net_recv_packet(conn->socket_fd);
    if (!res_pkt) {
        fprintf(stderr, "Failed to receive copy response\n");
        return -1;
    }

    result = -1;
    if (res_pkt->command == CMD_SUCCESS) {
        printf("File copied successfully\n");
        result = 0;
    } else if (res_pkt->command == CMD_ERROR) {
        cJSON* error_json = cJSON_Parse(res_pkt->payload);
        if (error_json) {
            cJSON* msg = cJSON_GetObjectItem(error_json, "message");
            if (msg) {
                fprintf(stderr, "Copy error: %s\n", cJSON_GetStringValue(msg));
            }
            cJSON_Delete(error_json);
        }
    }

    packet_free(res_pkt);
    return result;
}

int client_move(ClientConnection* conn, int file_id, int new_parent_id) {
    if (!conn || !conn->authenticated) {
        fprintf(stderr, "Invalid parameters for move\n");
        return -1;
    }

    // Build JSON request
    cJSON* request = cJSON_CreateObject();
    cJSON_AddNumberToObject(request, "file_id", file_id);
    cJSON_AddNumberToObject(request, "new_parent_id", new_parent_id);

    char* request_str = cJSON_PrintUnformatted(request);
    Packet* req_pkt = packet_create(CMD_MOVE, request_str, strlen(request_str));

    int result = packet_send(conn->socket_fd, req_pkt);

    free(request_str);
    packet_free(req_pkt);
    cJSON_Delete(request);

    if (result < 0) {
        fprintf(stderr, "Failed to send move request\n");
        return -1;
    }

    // Receive response
    Packet* res_pkt = net_recv_packet(conn->socket_fd);
    if (!res_pkt) {
        fprintf(stderr, "Failed to receive move response\n");
        return -1;
    }

    result = -1;
    if (res_pkt->command == CMD_SUCCESS) {
        printf("File moved successfully\n");
        result = 0;
    } else if (res_pkt->command == CMD_ERROR) {
        cJSON* error_json = cJSON_Parse(res_pkt->payload);
        if (error_json) {
            cJSON* msg = cJSON_GetObjectItem(error_json, "message");
            if (msg) {
                fprintf(stderr, "Move error: %s\n", cJSON_GetStringValue(msg));
            }
            cJSON_Delete(error_json);
        }
    }

    packet_free(res_pkt);
    return result;
}

int client_upload_folder(ClientConnection* conn, const char* local_path) {
    if (!conn || !conn->authenticated || !local_path) return -1;

    DIR* dir = opendir(local_path);
    if (!dir) {
        printf("Error: Cannot open directory: %s\n", local_path);
        return -1;
    }

    // Extract folder name from path
    const char* folder_name = strrchr(local_path, '/');
    folder_name = folder_name ? folder_name + 1 : local_path;

    // Create remote directory
    printf("Creating remote directory: %s\n", folder_name);
    if (client_mkdir(conn, folder_name) < 0) {
        closedir(dir);
        return -1;
    }

    // Get the ID of the newly created directory
    // We need to list the current directory to find its ID
    int saved_dir = conn->current_directory;

    // TODO: This is a workaround - ideally mkdir should return the new dir ID
    // For now, we'll assume the last created directory is the one we want

    int files_uploaded = 0;
    int dirs_created = 1;
    int errors = 0;

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", local_path, entry->d_name);

        struct stat st;
        if (stat(full_path, &st) != 0) {
            printf("Warning: Cannot stat %s, skipping\n", entry->d_name);
            errors++;
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            // Recursive upload of subdirectory
            printf("Uploading folder: %s\n", entry->d_name);
            if (client_upload_folder(conn, full_path) < 0) {
                printf("Warning: Failed to upload folder %s\n", entry->d_name);
                errors++;
            } else {
                dirs_created++;
            }
        } else if (S_ISREG(st.st_mode)) {
            // Upload file
            printf("Uploading file: %s (%lld bytes)\n", entry->d_name, (long long)st.st_size);
            if (client_upload(conn, full_path) < 0) {
                printf("Warning: Failed to upload file %s\n", entry->d_name);
                errors++;
            } else {
                files_uploaded++;
            }
        }
    }

    closedir(dir);

    printf("\nFolder upload complete!\n");
    printf("Directories created: %d\n", dirs_created);
    printf("Files uploaded: %d\n", files_uploaded);
    if (errors > 0) {
        printf("Errors: %d\n", errors);
    }

    return errors > 0 ? -1 : 0;
}

int client_download_folder(ClientConnection* conn, int folder_id, const char* local_path) {
    if (!conn || !conn->authenticated || !local_path) return -1;

    // Create local directory
    if (mkdir(local_path, 0755) < 0 && errno != EEXIST) {
        printf("Error: Cannot create directory: %s\n", local_path);
        return -1;
    }

    printf("Downloading to: %s\n", local_path);

    // Save current directory and navigate to the folder
    int saved_dir = conn->current_directory;
    if (client_cd(conn, folder_id) < 0) {
        printf("Error: Cannot enter directory %d\n", folder_id);
        return -1;
    }

    // Get file listing for this directory (use current_directory after cd)
    cJSON* resp_json = (cJSON*)client_list_dir_gui(conn, conn->current_directory);
    if (!resp_json) {
        client_cd(conn, saved_dir);  // Restore directory
        printf("Error: Cannot list directory\n");
        return -1;
    }

    int files_downloaded = 0;
    int dirs_downloaded = 0;
    int errors = 0;

    cJSON* files = cJSON_GetObjectItem(resp_json, "files");
    if (files) {
        cJSON* file;
        cJSON_ArrayForEach(file, files) {
            int id = cJSON_GetObjectItem(file, "id")->valueint;
            int is_dir = cJSON_GetObjectItem(file, "is_directory")->valueint;
            const char* name = cJSON_GetStringValue(cJSON_GetObjectItem(file, "name"));

            char local_file_path[1024];
            snprintf(local_file_path, sizeof(local_file_path), "%s/%s", local_path, name);

            if (is_dir) {
                // Recursively download subdirectory
                printf("Downloading folder: %s\n", name);
                if (client_download_folder(conn, id, local_file_path) < 0) {
                    printf("Warning: Failed to download folder %s\n", name);
                    errors++;
                } else {
                    dirs_downloaded++;
                }
            } else {
                // Download file
                int size = cJSON_GetObjectItem(file, "size")->valueint;
                printf("Downloading file: %s (%d bytes)\n", name, size);
                if (client_download(conn, id, local_file_path) < 0) {
                    printf("Warning: Failed to download file %s\n", name);
                    errors++;
                } else {
                    files_downloaded++;
                }
            }
        }
    }

    cJSON_Delete(resp_json);

    // Restore original directory
    client_cd(conn, saved_dir);

    printf("\nFolder download complete!\n");
    printf("Directories downloaded: %d\n", dirs_downloaded);
    printf("Files downloaded: %d\n", files_downloaded);
    if (errors > 0) {
        printf("Errors: %d\n", errors);
    }

    return errors > 0 ? -1 : 0;
}

// Admin operations
void* client_admin_list_users(ClientConnection* conn) {
    if (!conn || !conn->authenticated) {
        fprintf(stderr, "Not connected or authenticated\n");
        return NULL;
    }

    // Send list users request (empty payload)
    cJSON* request = cJSON_CreateObject();
    char* request_str = cJSON_PrintUnformatted(request);
    
    Packet* req_pkt = packet_create(CMD_ADMIN_LIST_USERS, request_str, strlen(request_str));
    packet_send(conn->socket_fd, req_pkt);
    
    free(request_str);
    packet_free(req_pkt);
    cJSON_Delete(request);

    // Receive response
    Packet* res_pkt = net_recv_packet(conn->socket_fd);
    if (!res_pkt) {
        fprintf(stderr, "Failed to receive response\n");
        return NULL;
    }

    if (res_pkt->command == CMD_ERROR) {
        cJSON* error_json = cJSON_Parse(res_pkt->payload);
        if (error_json) {
            cJSON* msg = cJSON_GetObjectItem(error_json, "message");
            fprintf(stderr, "Error: %s\n", cJSON_GetStringValue(msg));
            cJSON_Delete(error_json);
        }
        packet_free(res_pkt);
        return NULL;
    }

    // Parse and return response
    cJSON* response_json = cJSON_Parse(res_pkt->payload);
    packet_free(res_pkt);

    return response_json;  // Caller must free with cJSON_Delete()
}

int client_admin_create_user(ClientConnection* conn, const char* username, const char* password, int is_admin) {
    if (!conn || !conn->authenticated) {
        fprintf(stderr, "Not connected or authenticated\n");
        return -1;
    }

    // Build request
    cJSON* request = cJSON_CreateObject();
    cJSON_AddStringToObject(request, "username", username);
    cJSON_AddStringToObject(request, "password", password);
    cJSON_AddNumberToObject(request, "is_admin", is_admin);
    
    char* request_str = cJSON_PrintUnformatted(request);
    Packet* req_pkt = packet_create(CMD_ADMIN_CREATE_USER, request_str, strlen(request_str));
    packet_send(conn->socket_fd, req_pkt);
    
    free(request_str);
    packet_free(req_pkt);
    cJSON_Delete(request);

    // Receive response
    Packet* res_pkt = net_recv_packet(conn->socket_fd);
    if (!res_pkt) {
        fprintf(stderr, "Failed to receive response\n");
        return -1;
    }

    if (res_pkt->command == CMD_ERROR) {
        cJSON* error_json = cJSON_Parse(res_pkt->payload);
        if (error_json) {
            cJSON* msg = cJSON_GetObjectItem(error_json, "message");
            fprintf(stderr, "Error: %s\n", cJSON_GetStringValue(msg));
            cJSON_Delete(error_json);
        }
        packet_free(res_pkt);
        return -1;
    }

    // Parse response to get new user_id
    cJSON* response_json = cJSON_Parse(res_pkt->payload);
    int user_id = -1;
    if (response_json) {
        cJSON* user_id_item = cJSON_GetObjectItem(response_json, "user_id");
        if (user_id_item) {
            user_id = user_id_item->valueint;
        }
        cJSON_Delete(response_json);
    }

    packet_free(res_pkt);
    return user_id;
}

int client_admin_delete_user(ClientConnection* conn, int user_id) {
    if (!conn || !conn->authenticated) {
        fprintf(stderr, "Not connected or authenticated\n");
        return -1;
    }

    // Build request
    cJSON* request = cJSON_CreateObject();
    cJSON_AddNumberToObject(request, "user_id", user_id);

    char* request_str = cJSON_PrintUnformatted(request);
    Packet* req_pkt = packet_create(CMD_ADMIN_DELETE_USER, request_str, strlen(request_str));
    packet_send(conn->socket_fd, req_pkt);

    free(request_str);
    packet_free(req_pkt);
    cJSON_Delete(request);

    // Receive response
    Packet* res_pkt = net_recv_packet(conn->socket_fd);
    if (!res_pkt) {
        fprintf(stderr, "Failed to receive response\n");
        return -1;
    }

    if (res_pkt->command == CMD_ERROR) {
        cJSON* error_json = cJSON_Parse(res_pkt->payload);
        if (error_json) {
            cJSON* msg = cJSON_GetObjectItem(error_json, "message");
            fprintf(stderr, "Error: %s\n", cJSON_GetStringValue(msg));
            cJSON_Delete(error_json);
        }
        packet_free(res_pkt);
        return -1;
    }

    packet_free(res_pkt);
    return 0;  // Success
}

int client_admin_update_user(ClientConnection* conn, int user_id, int is_admin, int is_active) {
    if (!conn || !conn->authenticated) {
        fprintf(stderr, "Not connected or authenticated\n");
        return -1;
    }

    // Build request
    cJSON* request = cJSON_CreateObject();
    cJSON_AddNumberToObject(request, "user_id", user_id);
    cJSON_AddNumberToObject(request, "is_admin", is_admin);
    cJSON_AddNumberToObject(request, "is_active", is_active);

    char* request_str = cJSON_PrintUnformatted(request);
    Packet* req_pkt = packet_create(CMD_ADMIN_UPDATE_USER, request_str, strlen(request_str));
    packet_send(conn->socket_fd, req_pkt);

    free(request_str);
    packet_free(req_pkt);
    cJSON_Delete(request);

    // Receive response
    Packet* res_pkt = net_recv_packet(conn->socket_fd);
    if (!res_pkt) {
        fprintf(stderr, "Failed to receive response\n");
        return -1;
    }

    if (res_pkt->command == CMD_ERROR) {
        cJSON* error_json = cJSON_Parse(res_pkt->payload);
        if (error_json) {
            cJSON* msg = cJSON_GetObjectItem(error_json, "message");
            fprintf(stderr, "Error: %s\n", cJSON_GetStringValue(msg));
            cJSON_Delete(error_json);
        }
        packet_free(res_pkt);
        return -1;
    }

    packet_free(res_pkt);
    return 0;  // Success
}
