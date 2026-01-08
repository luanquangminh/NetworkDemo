#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "client.h"
#include "net_handler.h"
#include "../common/protocol.h"
#include "../../lib/cJSON/cJSON.h"

void print_help(void) {
    printf("\nCommands:\n");
    printf("  ls                    - List current directory\n");
    printf("  cd <id>               - Change to directory by ID\n");
    printf("  mkdir <name>          - Create new directory\n");
    printf("  upload <file>         - Upload local file\n");
    printf("  uploadfolder <folder> - Upload folder recursively\n");
    printf("  download <id> <file>  - Download file to local path\n");
    printf("  downloadfolder <id> <path> - Download folder recursively\n");
    printf("  chmod <id> <perm>     - Change permissions (e.g., 755)\n");
    printf("  delete <id>           - Delete file or directory\n");
    printf("  info <id>             - Show detailed file information\n");
    printf("  search <pattern> [-r] - Search files (wildcards: *, ?; -r for recursive)\n");
    printf("  rename <id> <name>    - Rename file or directory\n");
    printf("  copy <src_id> <dest_parent_id> [name] - Copy file to directory\n");
    printf("  move <id> <dest_parent_id> - Move file to directory\n");
    printf("  pwd                   - Print current directory\n");
    printf("  help                  - Show this help\n");
    printf("  quit                  - Exit\n");
}

int main(int argc, char** argv) {
    if (argc < 3) {
        printf("Usage: %s <server_ip> <port>\n", argv[0]);
        return 1;
    }

    const char* server_ip = argv[1];
    int port = atoi(argv[2]);

    printf("=== File Sharing Client ===\n");
    printf("Connecting to %s:%d...\n", server_ip, port);

    ClientConnection* conn = client_connect(server_ip, port);
    if (!conn) {
        printf("Failed to connect to server\n");
        return 1;
    }
    printf("Connected successfully!\n");

    // Login
    char username[64], password[64];
    printf("\nLogin required:\n");
    printf("Username: ");
    if (!fgets(username, sizeof(username), stdin)) {
        client_disconnect(conn);
        return 1;
    }
    username[strcspn(username, "\n")] = 0;

    printf("Password: ");
    if (!fgets(password, sizeof(password), stdin)) {
        client_disconnect(conn);
        return 1;
    }
    password[strcspn(password, "\n")] = 0;

    if (client_login(conn, username, password) < 0) {
        printf("Login failed. Disconnecting...\n");
        client_disconnect(conn);
        return 1;
    }

    print_help();

    // List root directory by default
    printf("\nListing root directory:\n");
    client_list_dir(conn, 0);

    // Command loop
    char command[512];
    char arg1[256], arg2[256];

    while (1) {
        printf("\n%s> ", conn->current_path);
        fflush(stdout);

        if (!fgets(command, sizeof(command), stdin)) break;

        // Reset arguments
        arg1[0] = '\0';
        arg2[0] = '\0';

        // Parse command and arguments
        char* cmd = strtok(command, " \t\n");
        if (!cmd) continue;

        if (strcmp(cmd, "quit") == 0 || strcmp(cmd, "exit") == 0) {
            break;
        } else if (strcmp(cmd, "help") == 0) {
            print_help();
        } else if (strcmp(cmd, "ls") == 0) {
            client_list_dir(conn, conn->current_directory);
        } else if (strcmp(cmd, "cd") == 0) {
            char* id_str = strtok(NULL, " \t\n");
            if (id_str) {
                client_cd(conn, atoi(id_str));
            } else {
                printf("Usage: cd <directory_id>\n");
            }
        } else if (strcmp(cmd, "mkdir") == 0) {
            char* name = strtok(NULL, " \t\n");
            if (name) {
                client_mkdir(conn, name);
            } else {
                printf("Usage: mkdir <name>\n");
            }
        } else if (strcmp(cmd, "upload") == 0) {
            char* path = strtok(NULL, " \t\n");
            if (path) {
                client_upload(conn, path);
            } else {
                printf("Usage: upload <local_file_path>\n");
            }
        } else if (strcmp(cmd, "uploadfolder") == 0) {
            char* path = strtok(NULL, " \t\n");
            if (path) {
                client_upload_folder(conn, path);
            } else {
                printf("Usage: uploadfolder <local_folder_path>\n");
            }
        } else if (strcmp(cmd, "download") == 0) {
            char* id_str = strtok(NULL, " \t\n");
            char* path = strtok(NULL, " \t\n");
            if (id_str && path) {
                client_download(conn, atoi(id_str), path);
            } else {
                printf("Usage: download <file_id> <local_path>\n");
            }
        } else if (strcmp(cmd, "downloadfolder") == 0) {
            char* id_str = strtok(NULL, " \t\n");
            char* path = strtok(NULL, " \t\n");
            if (id_str && path) {
                client_download_folder(conn, atoi(id_str), path);
            } else {
                printf("Usage: downloadfolder <folder_id> <local_path>\n");
            }
        } else if (strcmp(cmd, "chmod") == 0) {
            char* id_str = strtok(NULL, " \t\n");
            char* perm_str = strtok(NULL, " \t\n");
            if (id_str && perm_str) {
                // Convert octal string to integer
                int perm = (int)strtol(perm_str, NULL, 8);
                client_chmod(conn, atoi(id_str), perm);
            } else {
                printf("Usage: chmod <file_id> <permissions>\n");
                printf("Example: chmod 5 755\n");
            }
        } else if (strcmp(cmd, "delete") == 0 || strcmp(cmd, "rm") == 0) {
            char* id_str = strtok(NULL, " \t\n");
            if (id_str) {
                client_delete(conn, atoi(id_str));
            } else {
                printf("Usage: delete <file_id>\n");
            }
        } else if (strcmp(cmd, "info") == 0) {
            char* id_str = strtok(NULL, " \t\n");
            if (id_str) {
                client_file_info(conn, atoi(id_str));
            } else {
                printf("Usage: info <file_id>\n");
            }
        } else if (strcmp(cmd, "search") == 0 || strcmp(cmd, "find") == 0) {
            char* pattern = strtok(NULL, " \t\n");
            if (pattern) {
                int recursive = 0;

                // Check for -r flag
                char* next_arg = strtok(NULL, " \t\n");
                if (next_arg && (strcmp(next_arg, "-r") == 0 || strcmp(next_arg, "--recursive") == 0)) {
                    recursive = 1;
                }

                // Call search function
                cJSON* results = (cJSON*)client_search(conn, pattern, recursive, 100);

                if (results) {
                    cJSON* files = cJSON_GetObjectItem(results, "files");
                    cJSON* count_obj = cJSON_GetObjectItem(results, "count");

                    if (count_obj) {
                        int count = count_obj->valueint;
                        printf("\nFound %d file(s) matching '%s'%s:\n\n",
                               count, pattern, recursive ? " (recursive)" : "");

                        if (count > 0 && files) {
                            printf("%-6s %-4s %-35s %-10s %-50s\n",
                                   "ID", "Type", "Name", "Size", "Path");
                            printf("------------------------------------------------------------------------------------------------\n");

                            cJSON* file;
                            cJSON_ArrayForEach(file, files) {
                                int id = cJSON_GetObjectItem(file, "id")->valueint;
                                int is_dir = cJSON_GetObjectItem(file, "is_directory")->valueint;
                                const char* name = cJSON_GetStringValue(cJSON_GetObjectItem(file, "name"));
                                int size = cJSON_GetObjectItem(file, "size")->valueint;

                                // Get path if available
                                const char* path = "/";
                                cJSON* path_obj = cJSON_GetObjectItem(file, "path");
                                if (path_obj) {
                                    path = cJSON_GetStringValue(path_obj);
                                }

                                printf("%-6d %-4s %-35s %-10d %-50s\n",
                                       id, is_dir ? "DIR" : "FILE", name,
                                       is_dir ? 0 : size, path);
                            }
                            printf("\n");
                        }
                    }

                    cJSON_Delete(results);
                } else {
                    printf("Search failed\n");
                }
            } else {
                printf("Usage: search <pattern> [-r]\n");
                printf("Examples:\n");
                printf("  search test.txt      - Find exact match\n");
                printf("  search *.txt         - Find all .txt files\n");
                printf("  search test* -r      - Find files starting with 'test' (recursive)\n");
            }
        } else if (strcmp(cmd, "rename") == 0 || strcmp(cmd, "mv") == 0) {
            char* id_str = strtok(NULL, " \t\n");
            char* new_name = strtok(NULL, " \t\n");
            if (id_str && new_name) {
                client_rename(conn, atoi(id_str), new_name);
            } else {
                printf("Usage: rename <file_id> <new_name>\n");
            }
        } else if (strcmp(cmd, "copy") == 0 || strcmp(cmd, "cp") == 0) {
            char* src_id_str = strtok(NULL, " \t\n");
            char* dest_parent_str = strtok(NULL, " \t\n");
            char* new_name = strtok(NULL, " \t\n");
            if (src_id_str && dest_parent_str) {
                client_copy(conn, atoi(src_id_str), atoi(dest_parent_str), new_name);
            } else {
                printf("Usage: copy <source_id> <dest_parent_id> [new_name]\n");
            }
        } else if (strcmp(cmd, "move") == 0) {
            char* id_str = strtok(NULL, " \t\n");
            char* dest_parent_str = strtok(NULL, " \t\n");
            if (id_str && dest_parent_str) {
                client_move(conn, atoi(id_str), atoi(dest_parent_str));
            } else {
                printf("Usage: move <file_id> <dest_parent_id>\n");
            }
        } else if (strcmp(cmd, "pwd") == 0) {
            printf("Current directory: %s (ID: %d)\n", conn->current_path, conn->current_directory);
        } else {
            printf("Unknown command: '%s'\n", cmd);
            printf("Type 'help' for list of commands\n");
        }
    }

    printf("\nDisconnecting...\n");
    client_disconnect(conn);
    printf("Goodbye!\n");

    return 0;
}
