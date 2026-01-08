# File Search Feature Implementation Plan

**Plan ID:** 20260107-file-search-feature
**Created:** 2026-01-07
**Status:** Ready for Implementation

## Executive Summary

Add file search capability to file sharing application enabling users to search files by name with pattern matching (wildcards), case-insensitive matching, and optional recursive search. Implementation spans database layer (SQLite queries), server command handlers, client API, CLI interface, and GTK GUI integration.

**Key Components:**
- Database: Search query with LIKE operator, parameterized for security
- Protocol: New commands CMD_SEARCH_REQ (0x54), CMD_SEARCH_RES (0x55)
- Server: Search handler with permission checks
- Client: Search function for CLI and GUI
- GUI: GtkSearchEntry widget in toolbar with results display

**Estimated Effort:** 8-12 hours across 5 phases

---

## Table of Contents

1. [Architecture Overview](#architecture-overview)
2. [Protocol Specification](#protocol-specification)
3. [Implementation Phases](#implementation-phases)
4. [Security Considerations](#security-considerations)
5. [Testing Strategy](#testing-strategy)
6. [Code Examples](#code-examples)

---

## Architecture Overview

### Component Interaction Flow

```
User Input (CLI/GUI)
    ↓
Client API (client_search)
    ↓
Protocol Layer (CMD_SEARCH_REQ packet)
    ↓
Network (TCP Socket)
    ↓
Server Handler (handle_search)
    ↓
Permission Check (check_permission)
    ↓
Database Layer (db_search_files)
    ↓
SQLite Query (LIKE pattern match)
    ↓
Response (CMD_SEARCH_RES with file list)
    ↓
Client Display (CLI table or GUI TreeView)
```

### File Modifications Required

| File | Purpose | Lines Est. |
|------|---------|------------|
| `src/common/protocol.h` | Add command codes | +2 |
| `src/database/db_manager.h` | Add search function signature | +3 |
| `src/database/db_manager.c` | Implement search query | +80 |
| `src/server/commands.h` | Add handler declaration | +1 |
| `src/server/commands.c` | Implement search handler | +100 |
| `src/client/client.h` | Add client search function | +2 |
| `src/client/client.c` | Implement client search | +80 |
| `src/client/main.c` | Add CLI search command | +40 |
| `src/client/gui/main_window.c` | Add search widget to toolbar | +60 |
| `src/client/gui/file_operations.c` | Add search result handler | +50 |
| `src/database/db_init.sql` | Add index on file names | +1 |

**Total:** ~420 lines of new code

---

## Protocol Specification

### Command Codes

Add to `src/common/protocol.h`:

```c
#define CMD_SEARCH_REQ   0x54
#define CMD_SEARCH_RES   0x55
```

### SEARCH_REQ (0x54)

**Direction:** Client → Server
**Purpose:** Request file search by pattern

**Request Payload (JSON):**
```json
{
  "pattern": "*.txt",
  "directory_id": 0,
  "recursive": true,
  "limit": 100
}
```

**Fields:**
- `pattern` (string, required): Search pattern with wildcards (* or %)
- `directory_id` (int, optional): Directory to search in (default: current_directory)
- `recursive` (bool, optional): Search subdirectories (default: false)
- `limit` (int, optional): Max results (default: 100, max: 1000)

### SEARCH_RES (0x55)

**Direction:** Server → Client
**Purpose:** Return search results

**Response Payload (JSON):**
```json
{
  "status": "OK",
  "count": 2,
  "results": [
    {
      "id": 123,
      "name": "document.txt",
      "parent_id": 5,
      "path": "/documents/document.txt",
      "size": 1024,
      "is_directory": false,
      "permissions": 644,
      "owner_id": 1,
      "created_at": "2026-01-07 10:30:00"
    },
    {
      "id": 456,
      "name": "notes.txt",
      "parent_id": 8,
      "path": "/personal/notes.txt",
      "size": 2048,
      "is_directory": false,
      "permissions": 600,
      "owner_id": 1,
      "created_at": "2026-01-06 15:20:00"
    }
  ]
}
```

**Error Response:**
```json
{
  "status": "ERROR",
  "message": "Invalid search pattern"
}
```

---

## Implementation Phases

### Phase 1: Database Layer (2-3 hours)

**Objective:** Implement SQLite search query with security and performance optimizations

**Files Modified:**
- `src/database/db_manager.h`
- `src/database/db_manager.c`
- `src/database/db_init.sql`

**Tasks:**

1. **Add index for performance** (`db_init.sql`)
   ```sql
   CREATE INDEX IF NOT EXISTS idx_files_name ON files(name COLLATE NOCASE);
   ```

2. **Add function signature** (`db_manager.h`)
   ```c
   // Search files by pattern
   int db_search_files(Database* db, int base_dir_id, const char* pattern,
                       int recursive, int user_id, int limit,
                       FileEntry** entries, int* count);
   ```

3. **Implement search function** (`db_manager.c`)
   - Convert wildcards (* to %, ? to _)
   - Build parameterized query with LIKE
   - Handle recursive search with CTE or multiple queries
   - Apply permission filtering
   - Limit results
   - Build full path for each result

**Key Implementation Details:**

```c
int db_search_files(Database* db, int base_dir_id, const char* pattern,
                    int recursive, int user_id, int limit,
                    FileEntry** entries, int* count) {
    if (!db || !pattern || !entries || !count) return -1;

    pthread_mutex_lock(&db->mutex);

    // Convert wildcards: * -> %, ? -> _
    char sql_pattern[512];
    convert_wildcard_pattern(pattern, sql_pattern, sizeof(sql_pattern));

    sqlite3_stmt* stmt;
    const char* sql;

    if (recursive) {
        // Recursive CTE query
        sql = "WITH RECURSIVE file_tree(id, parent_id, name, physical_path, "
              "owner_id, size, is_directory, permissions, created_at, level) AS ("
              "  SELECT id, parent_id, name, physical_path, owner_id, size, "
              "         is_directory, permissions, created_at, 0 as level "
              "  FROM files WHERE id = ? "
              "  UNION ALL "
              "  SELECT f.id, f.parent_id, f.name, f.physical_path, f.owner_id, "
              "         f.size, f.is_directory, f.permissions, f.created_at, ft.level + 1 "
              "  FROM files f INNER JOIN file_tree ft ON f.parent_id = ft.id "
              ") "
              "SELECT * FROM file_tree "
              "WHERE name LIKE ? COLLATE NOCASE "
              "LIMIT ?";
    } else {
        // Non-recursive query
        sql = "SELECT id, parent_id, name, physical_path, owner_id, size, "
              "       is_directory, permissions, created_at "
              "FROM files "
              "WHERE parent_id = ? AND name LIKE ? COLLATE NOCASE "
              "LIMIT ?";
    }

    int rc = sqlite3_prepare_v2(db->conn, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        pthread_mutex_unlock(&db->mutex);
        return -1;
    }

    sqlite3_bind_int(stmt, 1, base_dir_id);
    sqlite3_bind_text(stmt, 2, sql_pattern, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, limit);

    // Allocate result array (realloc as needed)
    int capacity = 50;
    *entries = malloc(capacity * sizeof(FileEntry));
    *count = 0;

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        if (*count >= capacity) {
            capacity *= 2;
            *entries = realloc(*entries, capacity * sizeof(FileEntry));
        }

        FileEntry* entry = &(*entries)[*count];
        entry->id = sqlite3_column_int(stmt, 0);
        entry->parent_id = sqlite3_column_int(stmt, 1);
        strncpy(entry->name, (const char*)sqlite3_column_text(stmt, 2),
                sizeof(entry->name) - 1);

        const char* path = (const char*)sqlite3_column_text(stmt, 3);
        if (path) strncpy(entry->physical_path, path, sizeof(entry->physical_path) - 1);

        entry->owner_id = sqlite3_column_int(stmt, 4);
        entry->size = sqlite3_column_int64(stmt, 5);
        entry->is_directory = sqlite3_column_int(stmt, 6);
        entry->permissions = sqlite3_column_int(stmt, 7);

        const char* created = (const char*)sqlite3_column_text(stmt, 8);
        if (created) strncpy(entry->created_at, created, sizeof(entry->created_at) - 1);

        (*count)++;
    }

    sqlite3_finalize(stmt);
    pthread_mutex_unlock(&db->mutex);

    log_info("Search found %d results for pattern '%s'", *count, pattern);
    return 0;
}

// Helper: Convert shell wildcards to SQL wildcards
static void convert_wildcard_pattern(const char* input, char* output, size_t size) {
    size_t j = 0;
    for (size_t i = 0; input[i] && j < size - 1; i++) {
        if (input[i] == '*') {
            output[j++] = '%';
        } else if (input[i] == '?') {
            output[j++] = '_';
        } else if (input[i] == '%' || input[i] == '_') {
            // Escape SQL wildcards
            if (j < size - 2) {
                output[j++] = '\\';
                output[j++] = input[i];
            }
        } else {
            output[j++] = input[i];
        }
    }
    output[j] = '\0';
}
```

**Testing:**
- Unit test with direct database calls
- Test wildcard conversion: `test*.txt`, `*.pdf`, `file??.dat`
- Test case-insensitivity: `README` matches `readme.md`
- Test recursive vs non-recursive
- Test result limits
- Test SQL injection attempts

---

### Phase 2: Server Command Handler (2-3 hours)

**Objective:** Implement search handler with authentication and permission checks

**Files Modified:**
- `src/common/protocol.h` (add command codes)
- `src/server/commands.h`
- `src/server/commands.c`

**Tasks:**

1. **Add command codes** (`protocol.h`)
2. **Add handler declaration** (`commands.h`)
3. **Update dispatcher** (`commands.c`)
4. **Implement handler** (`commands.c`)

**Handler Implementation:**

```c
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

    // Parse request
    cJSON* pattern_item = cJSON_GetObjectItem(json, "pattern");
    if (!pattern_item || !cJSON_IsString(pattern_item)) {
        send_error(session, "Missing or invalid 'pattern' field");
        cJSON_Delete(json);
        return;
    }

    const char* pattern = cJSON_GetStringValue(pattern_item);

    // Validate pattern (prevent empty or dangerous patterns)
    if (strlen(pattern) == 0 || strlen(pattern) > 255) {
        send_error(session, "Invalid pattern length");
        cJSON_Delete(json);
        return;
    }

    // Get optional parameters
    int directory_id = session->current_directory;
    cJSON* dir_item = cJSON_GetObjectItem(json, "directory_id");
    if (dir_item && cJSON_IsNumber(dir_item)) {
        directory_id = dir_item->valueint;
    }

    int recursive = 0;
    cJSON* recursive_item = cJSON_GetObjectItem(json, "recursive");
    if (recursive_item && cJSON_IsBool(recursive_item)) {
        recursive = cJSON_IsTrue(recursive_item);
    }

    int limit = 100;  // Default limit
    cJSON* limit_item = cJSON_GetObjectItem(json, "limit");
    if (limit_item && cJSON_IsNumber(limit_item)) {
        limit = limit_item->valueint;
        if (limit > 1000) limit = 1000;  // Max limit
        if (limit < 1) limit = 1;
    }

    cJSON_Delete(json);

    // Check READ permission on base directory
    if (!check_permission(global_db, session->user_id, directory_id, ACCESS_READ)) {
        send_error(session, "Permission denied");
        db_log_activity(global_db, session->user_id, "ACCESS_DENIED", "SEARCH");
        return;
    }

    // Perform search
    FileEntry* entries = NULL;
    int count = 0;

    if (db_search_files(global_db, directory_id, pattern, recursive,
                        session->user_id, limit, &entries, &count) < 0) {
        send_error(session, "Search failed");
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

        cJSON* item = cJSON_CreateObject();
        cJSON_AddNumberToObject(item, "id", entries[i].id);
        cJSON_AddStringToObject(item, "name", entries[i].name);
        cJSON_AddNumberToObject(item, "parent_id", entries[i].parent_id);
        cJSON_AddStringToObject(item, "path", full_path);
        cJSON_AddNumberToObject(item, "size", entries[i].size);
        cJSON_AddBoolToObject(item, "is_directory", entries[i].is_directory);
        cJSON_AddNumberToObject(item, "permissions", entries[i].permissions);
        cJSON_AddNumberToObject(item, "owner_id", entries[i].owner_id);
        cJSON_AddStringToObject(item, "created_at", entries[i].created_at);

        cJSON_AddItemToArray(results_array, item);
    }

    char* payload = cJSON_PrintUnformatted(response);
    send_success(session, CMD_SEARCH_RES, payload);

    free(entries);
    free(payload);
    cJSON_Delete(response);

    log_info("Search completed for user %d: pattern='%s', found=%d",
             session->user_id, pattern, count);
    db_log_activity(global_db, session->user_id, "SEARCH", pattern);
}

// Helper: Build full path by traversing parent_id chain
static void build_full_path(Database* db, int file_id, char* path, size_t size) {
    char components[32][256];
    int depth = 0;
    int current_id = file_id;

    // Traverse up to root
    while (current_id > 0 && depth < 32) {
        FileEntry entry;
        if (db_get_file_by_id(db, current_id, &entry) != 0) break;

        strncpy(components[depth++], entry.name, 255);
        current_id = entry.parent_id;
    }

    // Build path from root down
    path[0] = '\0';
    for (int i = depth - 1; i >= 0; i--) {
        if (strlen(path) > 0) strcat(path, "/");
        strcat(path, components[i]);
    }

    if (path[0] != '/') {
        memmove(path + 1, path, strlen(path) + 1);
        path[0] = '/';
    }
}
```

**Update Dispatcher:**

```c
// In dispatch_command()
case CMD_SEARCH_REQ:
    handle_search(session, pkt);
    break;
```

**Testing:**
- Test with valid patterns
- Test permission denials
- Test invalid JSON payloads
- Test recursive vs non-recursive
- Test result limits
- Test activity logging

---

### Phase 3: Client API (1-2 hours)

**Objective:** Implement client-side search function for CLI and GUI

**Files Modified:**
- `src/client/client.h`
- `src/client/client.c`

**Tasks:**

1. **Add function signature** (`client.h`)
   ```c
   // Search operations
   void* client_search(ClientConnection* conn, const char* pattern,
                       int recursive);  // Returns cJSON* with results
   ```

2. **Implement search function** (`client.c`)

```c
void* client_search(ClientConnection* conn, const char* pattern, int recursive) {
    if (!conn || !conn->authenticated || !pattern) return NULL;

    cJSON* json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "pattern", pattern);
    cJSON_AddNumberToObject(json, "directory_id", conn->current_directory);
    cJSON_AddBoolToObject(json, "recursive", recursive);
    cJSON_AddNumberToObject(json, "limit", 100);

    char* payload = cJSON_PrintUnformatted(json);
    Packet* pkt = packet_create(CMD_SEARCH_REQ, payload, strlen(payload));

    int result = packet_send(conn->socket_fd, pkt);

    free(payload);
    packet_free(pkt);
    cJSON_Delete(json);

    if (result < 0) return NULL;

    // Receive response
    Packet* response = net_recv_packet(conn->socket_fd);
    if (!response) return NULL;

    if (response->command == CMD_ERROR) {
        cJSON* error_json = cJSON_Parse(response->payload);
        if (error_json) {
            cJSON* message = cJSON_GetObjectItem(error_json, "message");
            if (message) {
                printf("Search error: %s\n", cJSON_GetStringValue(message));
            }
            cJSON_Delete(error_json);
        }
        packet_free(response);
        return NULL;
    }

    if (response->command != CMD_SEARCH_RES) {
        packet_free(response);
        return NULL;
    }

    cJSON* resp_json = cJSON_Parse(response->payload);
    packet_free(response);

    return resp_json;  // Caller must cJSON_Delete()
}
```

**Testing:**
- Test successful searches
- Test network errors
- Test server errors
- Test memory leaks (valgrind)

---

### Phase 4: CLI Interface (1 hour)

**Objective:** Add search command to CLI client

**Files Modified:**
- `src/client/main.c`

**Tasks:**

1. **Add command to help text**
2. **Add command parser case**
3. **Implement search display**

```c
// In command loop
if (strcmp(cmd, "search") == 0 || strcmp(cmd, "find") == 0) {
    char pattern[256] = {0};
    int recursive = 0;

    // Parse arguments
    char* arg1 = strtok(NULL, " ");
    if (!arg1) {
        printf("Usage: search <pattern> [-r|--recursive]\n");
        printf("Examples:\n");
        printf("  search *.txt\n");
        printf("  search document -r\n");
        printf("  search test*.pdf --recursive\n");
        continue;
    }

    strncpy(pattern, arg1, sizeof(pattern) - 1);

    // Check for recursive flag
    char* arg2 = strtok(NULL, " ");
    if (arg2 && (strcmp(arg2, "-r") == 0 || strcmp(arg2, "--recursive") == 0)) {
        recursive = 1;
    }

    printf("Searching for '%s'%s...\n", pattern, recursive ? " (recursive)" : "");

    cJSON* results = (cJSON*)client_search(conn, pattern, recursive);
    if (!results) {
        printf("Search failed\n");
        continue;
    }

    cJSON* status = cJSON_GetObjectItem(results, "status");
    if (!status || strcmp(cJSON_GetStringValue(status), "OK") != 0) {
        printf("Search error\n");
        cJSON_Delete(results);
        continue;
    }

    cJSON* count_item = cJSON_GetObjectItem(results, "count");
    int count = count_item ? count_item->valueint : 0;

    printf("\nFound %d result%s:\n\n", count, count == 1 ? "" : "s");

    if (count == 0) {
        cJSON_Delete(results);
        continue;
    }

    // Display results in table format
    printf("%-8s %-30s %-10s %-15s %s\n",
           "ID", "Name", "Type", "Size", "Path");
    printf("%-8s %-30s %-10s %-15s %s\n",
           "--------", "------------------------------",
           "----------", "---------------", "----");

    cJSON* results_array = cJSON_GetObjectItem(results, "results");
    cJSON* item;
    cJSON_ArrayForEach(item, results_array) {
        int id = cJSON_GetObjectItem(item, "id")->valueint;
        const char* name = cJSON_GetStringValue(cJSON_GetObjectItem(item, "name"));
        int is_dir = cJSON_IsTrue(cJSON_GetObjectItem(item, "is_directory"));
        long size = cJSON_GetObjectItem(item, "size")->valueint;
        const char* path = cJSON_GetStringValue(cJSON_GetObjectItem(item, "path"));

        char size_str[16];
        if (is_dir) {
            strcpy(size_str, "DIR");
        } else {
            format_size(size, size_str, sizeof(size_str));
        }

        printf("%-8d %-30s %-10s %-15s %s\n",
               id, name, is_dir ? "Directory" : "File", size_str, path);
    }

    printf("\n");
    cJSON_Delete(results);
    continue;
}

// Helper function
static void format_size(long bytes, char* output, size_t size) {
    const char* units[] = {"B", "KB", "MB", "GB"};
    int unit = 0;
    double value = bytes;

    while (value >= 1024 && unit < 3) {
        value /= 1024;
        unit++;
    }

    snprintf(output, size, "%.1f %s", value, units[unit]);
}
```

**Testing:**
- Test search with various patterns
- Test recursive flag
- Test with no results
- Test with many results
- Test invalid patterns

---

### Phase 5: GUI Integration (2-3 hours)

**Objective:** Add search widget to GUI toolbar and display results

**Files Modified:**
- `src/client/gui/main_window.c`
- `src/client/gui/file_operations.c` (optional, for refactoring)

**Tasks:**

1. **Add search entry to toolbar** (`main_window.c`)
2. **Add signal handlers for search**
3. **Display results in existing TreeView**
4. **Add "Clear Search" button**

**GUI Implementation:**

```c
// In create_main_window(), after existing toolbar buttons

// Separator
GtkToolItem *separator = gtk_separator_tool_item_new();
gtk_toolbar_insert(GTK_TOOLBAR(toolbar), separator, -1);

// Search entry
GtkToolItem *search_item = gtk_tool_item_new();
state->search_entry = gtk_search_entry_new();
gtk_widget_set_size_request(state->search_entry, 200, -1);
gtk_entry_set_placeholder_text(GTK_ENTRY(state->search_entry), "Search files...");

g_signal_connect(state->search_entry, "activate",
                 G_CALLBACK(on_search_activated), state);
g_signal_connect(state->search_entry, "search-changed",
                 G_CALLBACK(on_search_changed), state);

gtk_container_add(GTK_CONTAINER(search_item), state->search_entry);
gtk_toolbar_insert(GTK_TOOLBAR(toolbar), search_item, -1);

// Search button
GtkToolItem *search_btn = gtk_tool_button_new(NULL, "Search");
gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(search_btn), "system-search");
g_signal_connect(search_btn, "clicked", G_CALLBACK(on_search_clicked), state);
gtk_toolbar_insert(GTK_TOOLBAR(toolbar), search_btn, -1);

// Recursive checkbox (toggle button)
GtkToolItem *recursive_item = gtk_tool_item_new();
state->recursive_check = gtk_check_button_new_with_label("Recursive");
gtk_container_add(GTK_CONTAINER(recursive_item), state->recursive_check);
gtk_toolbar_insert(GTK_TOOLBAR(toolbar), recursive_item, -1);

// Clear search button
GtkToolItem *clear_btn = gtk_tool_button_new(NULL, "Clear");
gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(clear_btn), "edit-clear");
g_signal_connect(clear_btn, "clicked", G_CALLBACK(on_clear_search_clicked), state);
gtk_toolbar_insert(GTK_TOOLBAR(toolbar), clear_btn, -1);
```

**Signal Handlers:**

```c
static void on_search_activated(GtkEntry *entry, AppState *state) {
    on_search_clicked(NULL, state);
}

static void on_search_changed(GtkSearchEntry *entry, AppState *state) {
    // Optional: debounced real-time search
    // For now, do nothing - require explicit search button click
}

static void on_search_clicked(GtkWidget *widget, AppState *state) {
    const char* pattern = gtk_entry_get_text(GTK_ENTRY(state->search_entry));

    if (!pattern || strlen(pattern) == 0) {
        show_error_dialog(state->window, "Please enter a search pattern");
        return;
    }

    int recursive = gtk_toggle_button_get_active(
        GTK_TOGGLE_BUTTON(state->recursive_check));

    // Perform search
    cJSON* results = (cJSON*)client_search(state->conn, pattern, recursive);
    if (!results) {
        show_error_dialog(state->window, "Search failed");
        return;
    }

    cJSON* status = cJSON_GetObjectItem(results, "status");
    if (!status || strcmp(cJSON_GetStringValue(status), "OK") != 0) {
        cJSON* message = cJSON_GetObjectItem(results, "message");
        const char* msg = message ? cJSON_GetStringValue(message) : "Unknown error";
        show_error_dialog(state->window, msg);
        cJSON_Delete(results);
        return;
    }

    // Clear current file list
    gtk_list_store_clear(state->file_store);

    // Populate with search results
    cJSON* results_array = cJSON_GetObjectItem(results, "results");
    cJSON* item;

    cJSON_ArrayForEach(item, results_array) {
        GtkTreeIter iter;
        gtk_list_store_append(state->file_store, &iter);

        int id = cJSON_GetObjectItem(item, "id")->valueint;
        const char* name = cJSON_GetStringValue(cJSON_GetObjectItem(item, "name"));
        int is_dir = cJSON_IsTrue(cJSON_GetObjectItem(item, "is_directory"));
        long size = cJSON_GetObjectItem(item, "size")->valueint;
        int perms = cJSON_GetObjectItem(item, "permissions")->valueint;
        const char* path = cJSON_GetStringValue(cJSON_GetObjectItem(item, "path"));

        char size_str[32];
        if (is_dir) {
            strcpy(size_str, "—");
        } else {
            format_size_human(size, size_str, sizeof(size_str));
        }

        char perm_str[16];
        snprintf(perm_str, sizeof(perm_str), "%03o", perms);

        gtk_list_store_set(state->file_store, &iter,
                          0, id,
                          1, is_dir ? "folder" : "text-x-generic",
                          2, name,
                          3, is_dir ? "Directory" : "File",
                          4, size_str,
                          5, perm_str,
                          -1);
    }

    // Update status bar
    cJSON* count_item = cJSON_GetObjectItem(results, "count");
    int count = count_item ? count_item->valueint : 0;

    guint context_id = gtk_statusbar_get_context_id(
        GTK_STATUSBAR(state->status_bar), "search");
    gtk_statusbar_pop(GTK_STATUSBAR(state->status_bar), context_id);

    char status_msg[256];
    snprintf(status_msg, sizeof(status_msg),
             "Search: '%s' - Found %d result%s",
             pattern, count, count == 1 ? "" : "s");
    gtk_statusbar_push(GTK_STATUSBAR(state->status_bar), context_id, status_msg);

    // Store search state
    state->in_search_mode = 1;
    strncpy(state->last_search_pattern, pattern, sizeof(state->last_search_pattern) - 1);

    cJSON_Delete(results);
}

static void on_clear_search_clicked(GtkWidget *widget, AppState *state) {
    gtk_entry_set_text(GTK_ENTRY(state->search_entry), "");
    state->in_search_mode = 0;

    // Refresh current directory listing
    refresh_file_list(state);
}

// Helper: Format size in human-readable format
static void format_size_human(long bytes, char* output, size_t size) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit = 0;
    double value = bytes;

    while (value >= 1024 && unit < 4) {
        value /= 1024;
        unit++;
    }

    if (unit == 0) {
        snprintf(output, size, "%ld %s", bytes, units[unit]);
    } else {
        snprintf(output, size, "%.1f %s", value, units[unit]);
    }
}
```

**Update AppState structure** (`gui.h`):

```c
typedef struct {
    GtkWidget *window;
    GtkWidget *tree_view;
    GtkWidget *status_bar;
    GtkWidget *search_entry;        // NEW
    GtkWidget *recursive_check;     // NEW
    GtkListStore *file_store;
    ClientConnection *conn;
    char current_path[512];
    int in_search_mode;             // NEW
    char last_search_pattern[256];  // NEW
} AppState;
```

**Testing:**
- Test search button click
- Test Enter key in search entry
- Test recursive checkbox
- Test clear button
- Test search with no results
- Test search result navigation (double-click)
- Test UI responsiveness during search

---

## Security Considerations

### 1. SQL Injection Prevention

**Risk:** User-controlled search pattern could inject SQL commands

**Mitigation:**
- ✅ Use parameterized queries (sqlite3_bind_text)
- ✅ Never concatenate user input into SQL strings
- ✅ Escape SQL wildcards (%, _) if used literally
- ✅ Validate pattern length and characters

**Example Attack Prevention:**
```c
// BAD (vulnerable):
sprintf(sql, "SELECT * FROM files WHERE name LIKE '%%%s%%'", pattern);

// GOOD (safe):
sqlite3_prepare_v2(db->conn, "SELECT * FROM files WHERE name LIKE ?", -1, &stmt, NULL);
sqlite3_bind_text(stmt, 1, pattern, -1, SQLITE_STATIC);
```

### 2. Path Traversal Prevention

**Risk:** Search results could expose files user shouldn't access

**Mitigation:**
- ✅ Check READ permission on base directory before search
- ✅ Filter results by ownership/permissions in query
- ✅ Validate directory_id exists and user has access
- ✅ Don't expose physical_path in responses (use VFS path)

### 3. Resource Exhaustion (DoS)

**Risk:** Malicious user searches with `*` pattern to load entire database

**Mitigation:**
- ✅ Enforce result limit (default 100, max 1000)
- ✅ Add query timeout (SQLite busy_timeout)
- ✅ Rate limit search requests per user
- ✅ Log excessive searches for monitoring

**Implementation:**
```c
// Set query timeout
sqlite3_busy_timeout(db->conn, 5000);  // 5 seconds

// Enforce max limit
if (limit > 1000) limit = 1000;
if (limit < 1) limit = 1;

// Validate pattern isn't just wildcards
if (strcmp(pattern, "*") == 0 || strcmp(pattern, "%") == 0) {
    return -1;  // Reject overly broad searches
}
```

### 4. Information Disclosure

**Risk:** Search reveals existence of files user can't access

**Mitigation:**
- ✅ Only return files user has READ permission for
- ✅ Don't show file count beyond permission boundary
- ✅ Log all search operations for audit

### 5. Pattern Validation

**Risk:** Malformed patterns crash server or cause undefined behavior

**Mitigation:**
- ✅ Validate pattern length (1-255 characters)
- ✅ Sanitize special characters
- ✅ Check for null bytes
- ✅ Reject invalid UTF-8

**Implementation:**
```c
bool validate_search_pattern(const char* pattern) {
    if (!pattern) return false;

    size_t len = strlen(pattern);
    if (len == 0 || len > 255) return false;

    // Check for null bytes in string
    if (memchr(pattern, '\0', len) != (pattern + len)) return false;

    // Optional: validate UTF-8
    // Optional: restrict to alphanumeric + wildcards + common chars

    return true;
}
```

---

## Testing Strategy

### Unit Tests

**Database Layer** (`test_db_search.c`):
```c
void test_search_exact_match() {
    // Setup: Create test files
    // Test: Search for exact name
    // Assert: Returns 1 result
}

void test_search_wildcard_star() {
    // Test: Pattern "test*.txt"
    // Assert: Matches test1.txt, test2.txt
}

void test_search_case_insensitive() {
    // Test: Pattern "README"
    // Assert: Matches readme.md, README.txt
}

void test_search_recursive() {
    // Setup: Files in nested directories
    // Test: Recursive search
    // Assert: Returns files from all subdirectories
}

void test_search_non_recursive() {
    // Test: Non-recursive search
    // Assert: Only returns files in base directory
}

void test_search_result_limit() {
    // Setup: 150 matching files
    // Test: Search with limit=100
    // Assert: Returns exactly 100 results
}

void test_search_sql_injection() {
    // Test: Pattern "'; DROP TABLE files; --"
    // Assert: No SQL execution, safe handling
}

void test_wildcard_conversion() {
    // Test: convert_wildcard_pattern("*.txt")
    // Assert: Returns "%.txt"
}
```

**Server Handler** (`test_search_handler.c`):
```c
void test_handle_search_success() {
    // Test: Valid search request
    // Assert: Returns CMD_SEARCH_RES with results
}

void test_handle_search_permission_denied() {
    // Test: Search in directory without READ permission
    // Assert: Returns error "Permission denied"
}

void test_handle_search_invalid_json() {
    // Test: Malformed JSON payload
    // Assert: Returns error "Invalid JSON"
}

void test_handle_search_missing_pattern() {
    // Test: Request without 'pattern' field
    // Assert: Returns error
}

void test_handle_search_logging() {
    // Test: Successful search
    // Assert: Activity logged in database
}
```

### Integration Tests

**Client-Server** (`test_search_integration.c`):
```c
void test_client_search_end_to_end() {
    // Setup: Start server, connect client
    // Test: client_search("*.txt", 0)
    // Assert: Returns valid JSON with results
}

void test_search_network_error() {
    // Test: Disconnect during search
    // Assert: client_search returns NULL
}

void test_search_large_result_set() {
    // Setup: 500 matching files
    // Test: Search with limit
    // Assert: Handles large response correctly
}
```

### GUI Tests

**Manual Testing Checklist:**
- [ ] Search entry accepts input
- [ ] Search button triggers search
- [ ] Enter key in search entry triggers search
- [ ] Recursive checkbox toggles correctly
- [ ] Results display in TreeView
- [ ] Status bar shows result count
- [ ] Clear button resets search and refreshes list
- [ ] Double-click on search result works
- [ ] Search with no results shows empty list
- [ ] Error dialog shows for invalid patterns
- [ ] UI remains responsive during search

### Performance Tests

```c
void test_search_performance() {
    // Setup: Database with 10,000 files
    // Test: Search with pattern "test*"
    // Assert: Completes in < 100ms
}

void test_search_recursive_performance() {
    // Setup: 100 directories, 1000 files
    // Test: Recursive search
    // Assert: Completes in < 500ms
}

void test_index_effectiveness() {
    // Test: EXPLAIN QUERY PLAN for search query
    // Assert: Uses idx_files_name index
}
```

### Security Tests

```bash
# SQL injection attempts
./test_client search "'; DROP TABLE files; --"
./test_client search "' OR '1'='1"
./test_client search "\\'; UNION SELECT * FROM users; --"

# DoS attempts
./test_client search "*"
./test_client search "%%%%%%%%"

# Path traversal
./test_client search "../../../etc/passwd"
./test_client search "../../.."

# Invalid UTF-8
./test_client search "\xFF\xFE\xFD"
```

---

## Code Examples

### Complete Database Function

See Phase 1 for full `db_search_files()` implementation.

### Complete Server Handler

See Phase 2 for full `handle_search()` implementation.

### Complete Client Function

See Phase 3 for full `client_search()` implementation.

### Complete CLI Command

See Phase 4 for full CLI search command implementation.

### Complete GUI Integration

See Phase 5 for full GUI search widget implementation.

---

## Rollout Plan

### Phase 0: Preparation (30 min)
- [ ] Review existing codebase patterns
- [ ] Set up testing environment
- [ ] Create feature branch: `git checkout -b feature/file-search`

### Phase 1: Database (2-3 hours)
- [ ] Add index to db_init.sql
- [ ] Implement db_search_files()
- [ ] Implement convert_wildcard_pattern()
- [ ] Implement build_full_path()
- [ ] Write unit tests
- [ ] Run tests: `make test_db_search`

### Phase 2: Server (2-3 hours)
- [ ] Add CMD_SEARCH_REQ and CMD_SEARCH_RES to protocol.h
- [ ] Add handle_search() declaration to commands.h
- [ ] Implement handle_search() in commands.c
- [ ] Update dispatch_command() switch
- [ ] Write unit tests
- [ ] Run tests: `make test_search_handler`

### Phase 3: Client API (1-2 hours)
- [ ] Add client_search() to client.h
- [ ] Implement client_search() in client.c
- [ ] Write unit tests
- [ ] Run integration tests

### Phase 4: CLI (1 hour)
- [ ] Add search command to main.c
- [ ] Implement format_size() helper
- [ ] Update help text
- [ ] Manual testing

### Phase 5: GUI (2-3 hours)
- [ ] Add search_entry and recursive_check to AppState
- [ ] Add widgets to toolbar in create_main_window()
- [ ] Implement on_search_clicked()
- [ ] Implement on_clear_search_clicked()
- [ ] Implement format_size_human()
- [ ] Manual testing

### Phase 6: Integration & Testing (1-2 hours)
- [ ] End-to-end testing (CLI)
- [ ] End-to-end testing (GUI)
- [ ] Performance testing
- [ ] Security testing
- [ ] Fix bugs

### Phase 7: Documentation & Deployment (30 min)
- [ ] Update protocol_spec.md
- [ ] Update api_reference.md
- [ ] Update README with search feature
- [ ] Commit changes
- [ ] Create pull request

---

## Risks & Mitigations

| Risk | Impact | Likelihood | Mitigation |
|------|--------|------------|------------|
| SQL injection vulnerability | Critical | Low | Use parameterized queries, validate input |
| Performance degradation on large DBs | High | Medium | Add index, enforce limits, optimize query |
| Recursive search causes infinite loop | High | Low | Track visited directories, limit depth |
| Search results leak unauthorized files | High | Low | Check permissions, filter by ownership |
| UI freezes during long search | Medium | Medium | Consider async search with progress bar |
| Wildcard conversion bugs | Medium | Low | Thorough unit testing, edge cases |

---

## Future Enhancements (Not in Scope)

1. **Advanced Filters**
   - File size range (e.g., "size:>1MB")
   - Date range (e.g., "modified:last-week")
   - File type filter (e.g., "type:pdf")
   - Owner filter (e.g., "owner:john")

2. **Search History**
   - Store recent searches
   - Quick access dropdown

3. **Saved Searches**
   - Save common searches
   - Named search presets

4. **Full-Text Content Search**
   - Search within file contents (not just names)
   - Requires file indexing

5. **Regular Expression Support**
   - Advanced pattern matching
   - Use SQLite REGEXP function

6. **Search Suggestions/Autocomplete**
   - Suggest file names as user types
   - Debounced real-time search

7. **Export Search Results**
   - Save results to CSV
   - Copy to clipboard

8. **Search Performance Analytics**
   - Log search durations
   - Optimize slow queries

---

## Success Criteria

- [ ] Search finds files by exact name
- [ ] Search supports wildcard patterns (*, ?)
- [ ] Search is case-insensitive
- [ ] Recursive search works across subdirectories
- [ ] Non-recursive search limited to current directory
- [ ] Results include full path
- [ ] Results limited to prevent overload
- [ ] Permission checks prevent unauthorized access
- [ ] SQL injection attempts safely handled
- [ ] CLI search command functional
- [ ] GUI search widget integrated in toolbar
- [ ] Search results display correctly
- [ ] All tests pass
- [ ] No memory leaks (valgrind clean)
- [ ] Performance acceptable (< 100ms for typical search)
- [ ] Documentation updated

---

## Questions & Decisions

### Resolved
- **Q:** Use LIKE or GLOB for pattern matching?
  - **A:** LIKE with COLLATE NOCASE for case-insensitivity

- **Q:** How to handle recursive search efficiently?
  - **A:** SQLite recursive CTE (Common Table Expression)

- **Q:** Should search be real-time or on-demand?
  - **A:** On-demand (button/Enter) to avoid excessive queries

### Unresolved
- **Q:** Should we add search depth limit for recursive searches?
  - **Recommendation:** Add max_depth parameter (default 10) in future iteration

- **Q:** Should search results be sorted?
  - **Recommendation:** Add ORDER BY clause, default sort by relevance (exact matches first) or name

---

## References

- SQLite LIKE documentation: https://www.sqlite.org/lang_expr.html#like
- SQLite recursive CTEs: https://www.sqlite.org/lang_with.html
- GTK3 GtkSearchEntry: https://developer.gnome.org/gtk3/stable/GtkSearchEntry.html
- OWASP SQL Injection Prevention: https://cheatsheetseries.owasp.org/cheatsheets/SQL_Injection_Prevention_Cheat_Sheet.html

---

**End of Plan**
