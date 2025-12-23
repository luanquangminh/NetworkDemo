# File Sharing System - Client Completion Plan

**Project:** Linux C File Sharing System (FTP-like)
**Target:** Complete CLI client + Implement GTK+ 3 GUI client
**Date:** 2025-12-19
**Status:** Server complete (100%), CLI client 85.7% complete

---

## Executive Summary

**Current State:**
- Server: Phases 0-5 complete, fully functional
- CLI Client: Basic operations working (ls, cd, mkdir, upload, download, chmod, pwd)
- Missing: Recursive folder operations, progress indicators, robust error handling, GUI client

**Objectives:**
1. Complete CLI client to 100% (add missing features + fix transient test failure)
2. Implement full GTK+ 3 GUI client per Phase 6 specifications
3. Share common networking code between CLI and GUI
4. Comprehensive testing for both clients

**Estimated Effort:**
- CLI Completion: 8-12 hours
- GUI Implementation: 24-32 hours
- Testing & Integration: 8-12 hours
- **Total: 40-56 hours (5-7 working days)**

---

## Part 1: CLI Client Completion

### 1.1 Current Status Analysis

**Working Features:**
- ✅ Authentication (login with username/password)
- ✅ Directory listing (ls)
- ✅ Directory navigation (cd by ID)
- ✅ Create directory (mkdir)
- ✅ Single file upload
- ✅ Single file download
- ✅ Change permissions (chmod)
- ✅ Show current path (pwd)
- ✅ Basic error messages

**Missing Features:**
- ❌ Recursive folder upload
- ❌ Recursive folder download
- ❌ Progress indicators for large transfers
- ❌ Detailed error handling with status codes
- ❌ Resume interrupted transfers
- ❌ Delete files/folders
- ❌ File metadata display (timestamps, owner)
- ❌ Connection timeout handling

**Known Issues:**
- 1 test failing: mkdir (transient/timing issue)
- No visual feedback during large file transfers
- Error messages too generic

### 1.2 Implementation Tasks

#### Task 1.2.1: Recursive Folder Upload
**Priority:** HIGH
**Complexity:** MEDIUM
**Duration:** 2-3 hours

**Files to Modify:**
- `src/client/client.h` - Add `client_upload_folder()` declaration
- `src/client/client.c` - Implement recursive upload logic

**Implementation Details:**
```c
// In client.h
int client_upload_folder(ClientConnection* conn, const char* local_path);

// In client.c
int client_upload_folder(ClientConnection* conn, const char* local_path) {
    // 1. Open directory with opendir()
    // 2. For each entry (readdir):
    //    - Skip "." and ".."
    //    - If directory: create remote dir, recurse
    //    - If file: upload file
    // 3. Handle errors gracefully
    // 4. Return summary (files uploaded, errors)
}
```

**Algorithm:**
1. Validate local path exists and is directory
2. Create remote directory if not exists
3. Iterate entries using `readdir()`
4. For directories: create remote + recursive call
5. For files: call existing `client_upload()`
6. Track statistics (total files, bytes, errors)
7. Display progress summary

**Error Handling:**
- Permission denied on local file → skip, log error
- Network failure → abort, report partial progress
- Remote directory creation fails → abort subtree

**Testing:**
- Empty folder
- Folder with 100+ files
- Nested folders (5 levels deep)
- Mixed permissions (some unreadable)
- Very large folder tree

---

#### Task 1.2.2: Recursive Folder Download
**Priority:** HIGH
**Complexity:** MEDIUM
**Duration:** 2-3 hours

**Files to Modify:**
- `src/client/client.h` - Add `client_download_folder()` declaration
- `src/client/client.c` - Implement recursive download logic

**Implementation Details:**
```c
int client_download_folder(ClientConnection* conn, int remote_dir_id,
                           const char* local_path) {
    // 1. Create local directory if not exists
    // 2. List remote directory contents
    // 3. For each entry:
    //    - If directory: mkdir locally, recurse
    //    - If file: download file
    // 4. Preserve permissions
}
```

**Algorithm:**
1. Call `client_list_dir()` to get entries
2. Parse response JSON for files/folders
3. Create local directory structure
4. Download files preserving permissions
5. Recursively process subdirectories
6. Track progress (files downloaded, bytes)

**Error Handling:**
- Local disk full → abort, cleanup partial
- Remote permission denied → skip, continue
- Network timeout → retry 3 times, then abort

**Testing:**
- Empty remote folder
- Large folder (500+ files, 1GB+)
- Deep nesting
- Permission denied scenarios

---

#### Task 1.2.3: Progress Indicators
**Priority:** HIGH
**Complexity:** MEDIUM
**Duration:** 3-4 hours

**Files to Modify:**
- `src/client/net_handler.h` - Add callback typedef
- `src/client/net_handler.c` - Modify `net_send_file()` and `net_recv_file()`
- `src/client/client.c` - Implement progress callbacks

**Implementation Details:**
```c
// In net_handler.h
typedef void (*TransferProgressCallback)(size_t bytes_transferred,
                                         size_t total_bytes,
                                         void* user_data);

int net_send_file_ex(int sockfd, const char* file_path,
                     TransferProgressCallback callback, void* user_data);
int net_recv_file_ex(int sockfd, const char* file_path, size_t file_size,
                     TransferProgressCallback callback, void* user_data);
```

**Progress Display (CLI):**
```
Uploading file.mp4... [=========>          ] 45.2% (2.5 MB / 5.5 MB) 1.2 MB/s
```

**Implementation:**
1. Modify file transfer functions to accept callback
2. Call callback every 64KB or 100ms (whichever first)
3. Callback receives: bytes_done, total_bytes
4. CLI prints progress bar with \r (carriage return)
5. Calculate transfer speed (bytes/sec)
6. Estimate time remaining (ETA)

**Progress Bar Formula:**
```c
int percent = (bytes_done * 100) / total_bytes;
int bar_width = 40;
int filled = (percent * bar_width) / 100;
// Print [=====>    ] with filled bars
```

**Testing:**
- Small file (< 1KB) - should complete quickly
- Medium file (10 MB) - smooth progress
- Large file (500 MB) - consistent updates
- Slow network simulation

---

#### Task 1.2.4: Enhanced Error Handling
**Priority:** MEDIUM
**Complexity:** LOW
**Duration:** 2-3 hours

**Files to Modify:**
- `src/client/client.c` - All command functions

**Improvements:**
1. **Parse status codes from server:**
   - STATUS_OK (0) → success
   - STATUS_AUTH_FAIL (2) → "Authentication failed"
   - STATUS_PERM_DENIED (3) → "Permission denied"
   - STATUS_NOT_FOUND (4) → "File/directory not found"
   - STATUS_EXISTS (5) → "Already exists"

2. **Network error handling:**
   - Connection timeout → "Connection timed out, retrying..."
   - Connection lost → "Connection lost, reconnect? (y/n)"
   - DNS failure → "Cannot resolve hostname"

3. **File operation errors:**
   - Local file not found → specific message
   - Disk full → "Local disk full"
   - Permission denied → "Permission denied: <path>"

**Implementation Pattern:**
```c
// After receiving response
cJSON* status_obj = cJSON_GetObjectItem(resp_json, "status");
if (status_obj) {
    int status_code = status_obj->valueint;
    switch (status_code) {
        case STATUS_PERM_DENIED:
            printf("Error: Permission denied. Check file permissions.\n");
            break;
        case STATUS_NOT_FOUND:
            printf("Error: File or directory not found.\n");
            break;
        // ... handle all codes
    }
}
```

---

#### Task 1.2.5: Additional Commands
**Priority:** MEDIUM
**Complexity:** LOW
**Duration:** 2-3 hours

**New Commands to Add:**

1. **delete/rm** - Delete file or directory
```c
int client_delete(ClientConnection* conn, int file_id, int recursive);
```

2. **info** - Show file metadata
```c
int client_info(ClientConnection* conn, int file_id);
// Display: name, size, permissions, owner, created_at, modified_at
```

3. **cd ..** - Navigate to parent directory
```c
// Enhance client_cd() to support special cases:
// - "cd .." → parent directory
// - "cd /" → root directory
// - "cd ~" → user home directory (if implemented)
```

4. **clear** - Clear screen
```c
system("clear");  // Unix/Linux
```

**Files to Modify:**
- `src/client/client.h` - Add declarations
- `src/client/client.c` - Implement commands
- `src/client/main.c` - Add command parsing

---

#### Task 1.2.6: Fix Transient Test Failure
**Priority:** HIGH
**Complexity:** LOW
**Duration:** 1-2 hours

**Issue:** mkdir test occasionally fails

**Investigation Steps:**
1. Review test code in `tests/` directory
2. Check for race conditions (timing-dependent)
3. Verify server response handling
4. Add logging to identify failure point

**Potential Causes:**
- Network delay causing timeout
- Test cleanup not complete before next test
- Server response not fully received
- JSON parsing issue with special characters

**Fix Strategy:**
- Add retry logic with exponential backoff
- Increase test timeout if appropriate
- Ensure proper cleanup between tests
- Add more detailed test logging

---

### 1.3 CLI Client File Structure (Final)

```
src/client/
├── client.h                 # Client API declarations
├── client.c                 # Implementation (all commands)
├── net_handler.h            # Network layer API
├── net_handler.c            # Socket operations, file transfer
├── main.c                   # CLI interface, command loop
├── progress.h               # NEW: Progress display utilities
├── progress.c               # NEW: Progress bar implementation
└── Makefile                 # Build configuration
```

---

## Part 2: GTK+ 3 GUI Client Implementation

### 2.1 GTK+ 3 Overview

**What is GTK+ 3?**
- Cross-platform GUI toolkit for Linux/Unix
- Written in C with object-oriented design (GObject)
- Event-driven architecture
- Native look and feel

**Key Concepts:**
1. **Widgets:** UI components (buttons, labels, windows)
2. **Signals:** Events (clicked, changed, destroyed)
3. **Containers:** Layout managers (boxes, grids)
4. **Main Loop:** Event processing loop

**Required Libraries:**
- `gtk+-3.0` (GTK+ 3 core)
- `glib-2.0` (data structures, utilities)
- `gio-2.0` (I/O, file operations)

---

### 2.2 Architecture Design

**Design Principles:**
1. **Reuse networking code:** Share net_handler with CLI
2. **MVC pattern:** Separate UI from business logic
3. **Threaded file transfers:** Keep UI responsive
4. **Error dialogs:** User-friendly error messages

**Component Structure:**
```
GUI Client
├── main.c                   # GTK application entry point
├── gui_main.c               # Main window, UI setup
├── gui_callbacks.c          # Event handlers (button clicks, etc.)
├── gui_dialogs.c            # Dialogs (login, upload, error)
├── gui_file_list.c          # File browser (TreeView)
├── gui_transfer.c           # Upload/download with progress
├── net_handler.c            # SHARED with CLI (no changes)
└── gui.h                    # GUI component declarations
```

**Shared vs GUI-Specific:**

| Module | Shared | GUI-Specific | Notes |
|--------|--------|--------------|-------|
| `net_handler.c` | ✓ | | Reuse existing code |
| `client.c` | ✓ | | May need thread-safe wrappers |
| `progress.c` | ✓ | | CLI version; GUI uses GtkProgressBar |
| `gui_*.c` | | ✓ | All GUI components |

---

### 2.3 GUI Implementation Tasks

#### Task 2.3.1: Project Setup & Build System
**Priority:** HIGH
**Complexity:** LOW
**Duration:** 1-2 hours

**Files to Create:**
- `src/client-gui/Makefile`
- `src/client-gui/main.c` (minimal GTK app)

**Makefile Configuration:**
```makefile
CC = gcc
CFLAGS = -Wall -Wextra -pthread -I../common -I../../lib/cJSON \
         $(shell pkg-config --cflags gtk+-3.0)
LDFLAGS = -L../common
LIBS = -lcommon -lpthread $(shell pkg-config --libs gtk+-3.0)

SRCS = main.c gui_main.c gui_callbacks.c gui_dialogs.c \
       gui_file_list.c gui_transfer.c ../client/net_handler.c
OBJS = $(SRCS:.c=.o)

TARGET = ../../build/client-gui

all: $(TARGET)

$(TARGET): $(OBJS) ../common/libcommon.a
	mkdir -p ../../build
	$(CC) $(OBJS) $(LDFLAGS) $(LIBS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
```

**Minimal main.c (smoke test):**
```c
#include <gtk/gtk.h>

static void activate(GtkApplication* app, gpointer user_data) {
    GtkWidget* window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "File Sharing Client");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);

    GtkWidget* label = gtk_label_new("Hello, GTK+!");
    gtk_container_add(GTK_CONTAINER(window), label);

    gtk_widget_show_all(window);
}

int main(int argc, char** argv) {
    GtkApplication* app = gtk_application_new("com.fileshare.client",
                                              G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);

    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}
```

**Testing:**
```bash
cd src/client-gui
make
../../build/client-gui  # Should show window with "Hello, GTK+!"
```

---

#### Task 2.3.2: Login Dialog
**Priority:** HIGH
**Complexity:** MEDIUM
**Duration:** 3-4 hours

**Files to Create:**
- `src/client-gui/gui_dialogs.h`
- `src/client-gui/gui_dialogs.c`

**UI Design:**
```
+-----------------------------------+
| Connect to Server            [X]  |
+-----------------------------------+
| Server:   [localhost        ]     |
| Port:     [8080            ]      |
| Username: [                 ]     |
| Password: [                 ]     |
|                                   |
|     [Cancel]        [Connect]     |
+-----------------------------------+
```

**Implementation:**
```c
// gui_dialogs.h
typedef struct {
    char server[64];
    int port;
    char username[64];
    char password[64];
    int connected;
} LoginCredentials;

gboolean show_login_dialog(GtkWindow* parent, LoginCredentials* creds);

// gui_dialogs.c
gboolean show_login_dialog(GtkWindow* parent, LoginCredentials* creds) {
    GtkWidget* dialog = gtk_dialog_new_with_buttons(
        "Connect to Server",
        parent,
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Connect", GTK_RESPONSE_ACCEPT,
        NULL);

    // Create grid layout
    GtkWidget* grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 10);

    // Server entry
    GtkWidget* server_label = gtk_label_new("Server:");
    GtkWidget* server_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(server_entry), "localhost");
    gtk_grid_attach(GTK_GRID(grid), server_label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), server_entry, 1, 0, 1, 1);

    // Port entry
    GtkWidget* port_label = gtk_label_new("Port:");
    GtkWidget* port_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(port_entry), "8080");
    gtk_grid_attach(GTK_GRID(grid), port_label, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), port_entry, 1, 1, 1, 1);

    // Username entry
    GtkWidget* user_label = gtk_label_new("Username:");
    GtkWidget* user_entry = gtk_entry_new();
    gtk_grid_attach(GTK_GRID(grid), user_label, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), user_entry, 1, 2, 1, 1);

    // Password entry
    GtkWidget* pass_label = gtk_label_new("Password:");
    GtkWidget* pass_entry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(pass_entry), FALSE);
    gtk_entry_set_input_purpose(GTK_ENTRY(pass_entry),
                                GTK_INPUT_PURPOSE_PASSWORD);
    gtk_grid_attach(GTK_GRID(grid), pass_label, 0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), pass_entry, 1, 3, 1, 1);

    // Add grid to dialog content area
    GtkWidget* content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_add(GTK_CONTAINER(content), grid);
    gtk_widget_show_all(dialog);

    // Run dialog
    gint result = gtk_dialog_run(GTK_DIALOG(dialog));

    if (result == GTK_RESPONSE_ACCEPT) {
        // Extract values
        strncpy(creds->server,
                gtk_entry_get_text(GTK_ENTRY(server_entry)), 63);
        creds->port = atoi(gtk_entry_get_text(GTK_ENTRY(port_entry)));
        strncpy(creds->username,
                gtk_entry_get_text(GTK_ENTRY(user_entry)), 63);
        strncpy(creds->password,
                gtk_entry_get_text(GTK_ENTRY(pass_entry)), 63);
        creds->connected = 1;
    } else {
        creds->connected = 0;
    }

    gtk_widget_destroy(dialog);
    return creds->connected;
}
```

**Integration:**
- Show login dialog on startup
- Connect to server with credentials
- Display error dialog if connection fails
- Store connection in application state

---

#### Task 2.3.3: Main Window Layout
**Priority:** HIGH
**Complexity:** MEDIUM
**Duration:** 4-5 hours

**Files to Create:**
- `src/client-gui/gui_main.h`
- `src/client-gui/gui_main.c`

**UI Layout:**
```
+----------------------------------------------------------+
| File Sharing Client                                 [_][□][X]
+----------------------------------------------------------+
| File  Edit  View  Help                                   |
+----------------------------------------------------------+
| [↑] [🏠] [⟳] | Current Path: /home/alice/documents        |
+----------------------------------------------------------+
|  Name          | Type   | Size     | Permissions | Modified |
|----------------------------------------------------------|
|  ..            | UP     | -        | -           | -        |
|  Documents/    | Folder | -        | rwxr-xr-x   | 2025-... |
|  report.pdf    | File   | 2.5 MB   | rw-r--r--   | 2025-... |
|  photos/       | Folder | -        | rwxr-x---   | 2025-... |
|  todo.txt      | File   | 1.2 KB   | rw-rw-r--   | 2025-... |
|                                                           |
+----------------------------------------------------------+
| [Upload File] [Upload Folder] [Download] [New Folder]   |
+----------------------------------------------------------+
| Status: Connected as alice                    No activity |
+----------------------------------------------------------+
```

**Components:**
1. **Menu Bar:** File (Connect, Disconnect, Quit), Help (About)
2. **Toolbar:** Navigation buttons (Up, Home, Refresh)
3. **Path Bar:** Current directory display
4. **File List:** GtkTreeView with columns
5. **Action Buttons:** Upload, Download, etc.
6. **Status Bar:** Connection status, activity indicator

**Implementation Structure:**
```c
// gui_main.h
typedef struct {
    GtkWidget* window;
    GtkWidget* tree_view;
    GtkWidget* path_label;
    GtkWidget* status_label;
    GtkWidget* progress_bar;

    ClientConnection* connection;
    int current_directory;
} AppState;

AppState* create_main_window(GtkApplication* app);
void refresh_file_list(AppState* state);
```

**GtkTreeView Setup:**
```c
// Create list store with columns
enum {
    COL_ID,          // Hidden column for file ID
    COL_NAME,        // File/folder name
    COL_TYPE,        // "File" or "Folder"
    COL_SIZE,        // Size in human-readable format
    COL_PERMISSIONS, // Unix permissions (e.g., "rwxr-xr-x")
    COL_MODIFIED,    // Timestamp
    NUM_COLS
};

GtkListStore* store = gtk_list_store_new(NUM_COLS,
    G_TYPE_INT,     // COL_ID
    G_TYPE_STRING,  // COL_NAME
    G_TYPE_STRING,  // COL_TYPE
    G_TYPE_STRING,  // COL_SIZE
    G_TYPE_STRING,  // COL_PERMISSIONS
    G_TYPE_STRING); // COL_MODIFIED

GtkWidget* tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));

// Add columns
GtkCellRenderer* renderer = gtk_cell_renderer_text_new();
GtkTreeViewColumn* col;

col = gtk_tree_view_column_new_with_attributes(
    "Name", renderer, "text", COL_NAME, NULL);
gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), col);

// ... add remaining columns ...
```

---

#### Task 2.3.4: File List Display
**Priority:** HIGH
**Complexity:** MEDIUM
**Duration:** 3-4 hours

**Files to Create:**
- `src/client-gui/gui_file_list.h`
- `src/client-gui/gui_file_list.c`

**Functionality:**
1. Populate TreeView from server response
2. Format file sizes (B, KB, MB, GB)
3. Format permissions (octal → rwxr-xr-x)
4. Handle double-click to navigate folders
5. Context menu (right-click)

**Implementation:**
```c
void populate_file_list(AppState* state, int dir_id) {
    // Send LIST_DIR request
    cJSON* json = cJSON_CreateObject();
    cJSON_AddNumberToObject(json, "user_id", state->connection->user_id);
    cJSON_AddNumberToObject(json, "directory_id", dir_id);

    char* payload = cJSON_PrintUnformatted(json);
    Packet* pkt = packet_create(CMD_LIST_DIR, payload, strlen(payload));

    packet_send(state->connection->socket_fd, pkt);

    free(payload);
    packet_free(pkt);
    cJSON_Delete(json);

    // Receive response
    Packet* response = net_recv_packet(state->connection->socket_fd);
    if (!response) return;

    cJSON* resp_json = cJSON_Parse(response->payload);
    cJSON* files = cJSON_GetObjectItem(resp_json, "files");

    // Clear existing list
    GtkListStore* store = GTK_LIST_STORE(
        gtk_tree_view_get_model(GTK_TREE_VIEW(state->tree_view)));
    gtk_list_store_clear(store);

    // Add ".." entry for parent directory
    if (dir_id != 0) {
        GtkTreeIter iter;
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
            COL_ID, -1,
            COL_NAME, "..",
            COL_TYPE, "UP",
            COL_SIZE, "-",
            COL_PERMISSIONS, "-",
            COL_MODIFIED, "-",
            -1);
    }

    // Populate from JSON
    cJSON* file;
    cJSON_ArrayForEach(file, files) {
        GtkTreeIter iter;
        gtk_list_store_append(store, &iter);

        int id = cJSON_GetObjectItem(file, "id")->valueint;
        const char* name = cJSON_GetStringValue(
            cJSON_GetObjectItem(file, "name"));
        int is_dir = cJSON_GetObjectItem(file, "is_directory")->valueint;
        int size = cJSON_GetObjectItem(file, "size")->valueint;
        int perms = cJSON_GetObjectItem(file, "permissions")->valueint;

        char size_str[32];
        format_file_size(size, size_str, sizeof(size_str));

        char perm_str[16];
        format_permissions(perms, perm_str, sizeof(perm_str));

        gtk_list_store_set(store, &iter,
            COL_ID, id,
            COL_NAME, name,
            COL_TYPE, is_dir ? "Folder" : "File",
            COL_SIZE, is_dir ? "-" : size_str,
            COL_PERMISSIONS, perm_str,
            COL_MODIFIED, "2025-12-19",  // TODO: parse from server
            -1);
    }

    cJSON_Delete(resp_json);
    packet_free(response);
}

// Utility: Format file size
void format_file_size(long size, char* buffer, size_t buf_size) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit_idx = 0;
    double dsize = size;

    while (dsize >= 1024 && unit_idx < 4) {
        dsize /= 1024;
        unit_idx++;
    }

    if (unit_idx == 0) {
        snprintf(buffer, buf_size, "%ld %s", size, units[unit_idx]);
    } else {
        snprintf(buffer, buf_size, "%.1f %s", dsize, units[unit_idx]);
    }
}

// Utility: Format permissions
void format_permissions(int perms, char* buffer, size_t buf_size) {
    snprintf(buffer, buf_size, "%c%c%c%c%c%c%c%c%c",
        (perms & 0400) ? 'r' : '-',
        (perms & 0200) ? 'w' : '-',
        (perms & 0100) ? 'x' : '-',
        (perms & 0040) ? 'r' : '-',
        (perms & 0020) ? 'w' : '-',
        (perms & 0010) ? 'x' : '-',
        (perms & 0004) ? 'r' : '-',
        (perms & 0002) ? 'w' : '-',
        (perms & 0001) ? 'x' : '-');
}
```

**Double-Click Navigation:**
```c
static void on_row_activated(GtkTreeView* tree_view, GtkTreePath* path,
                             GtkTreeViewColumn* column, gpointer user_data) {
    AppState* state = (AppState*)user_data;

    GtkTreeModel* model = gtk_tree_view_get_model(tree_view);
    GtkTreeIter iter;

    if (gtk_tree_model_get_iter(model, &iter, path)) {
        gint id;
        gchar* type;

        gtk_tree_model_get(model, &iter,
            COL_ID, &id,
            COL_TYPE, &type,
            -1);

        if (g_strcmp0(type, "Folder") == 0) {
            // Navigate into folder
            state->current_directory = id;
            populate_file_list(state, id);
            update_path_display(state);
        } else if (g_strcmp0(type, "UP") == 0) {
            // Navigate to parent (TODO: get parent ID from server)
        }

        g_free(type);
    }
}

// Connect signal in setup
g_signal_connect(tree_view, "row-activated",
                 G_CALLBACK(on_row_activated), state);
```

---

#### Task 2.3.5: File Upload (GUI)
**Priority:** HIGH
**Complexity:** HIGH
**Duration:** 5-6 hours

**Files to Create:**
- `src/client-gui/gui_transfer.h`
- `src/client-gui/gui_transfer.c`

**Functionality:**
1. File chooser dialog
2. Threaded upload to keep UI responsive
3. Progress bar with percentage and speed
4. Cancel button
5. Completion notification

**UI Design (Progress Dialog):**
```
+-----------------------------------+
| Uploading...                 [X]  |
+-----------------------------------+
| File: large_video.mp4             |
| [==================>    ] 75%     |
| 750 MB / 1000 MB                  |
| Speed: 5.2 MB/s  ETA: 00:00:48   |
|                                   |
|             [Cancel]              |
+-----------------------------------+
```

**Implementation:**
```c
// gui_transfer.h
typedef struct {
    AppState* app_state;
    char local_path[512];
    char filename[256];
    size_t file_size;
    size_t bytes_transferred;
    gboolean cancelled;

    GtkWidget* dialog;
    GtkWidget* progress_bar;
    GtkWidget* status_label;
    GtkWidget* speed_label;

    time_t start_time;
} UploadContext;

void show_upload_file_dialog(AppState* state);
gpointer upload_thread_func(gpointer data);

// gui_transfer.c
void show_upload_file_dialog(AppState* state) {
    GtkWidget* dialog = gtk_file_chooser_dialog_new(
        "Select File to Upload",
        GTK_WINDOW(state->window),
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Upload", GTK_RESPONSE_ACCEPT,
        NULL);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char* filename = gtk_file_chooser_get_filename(
            GTK_FILE_CHOOSER(dialog));

        // Get file size
        struct stat st;
        if (stat(filename, &st) == 0) {
            // Create upload context
            UploadContext* ctx = g_new0(UploadContext, 1);
            ctx->app_state = state;
            strncpy(ctx->local_path, filename, sizeof(ctx->local_path) - 1);

            const char* basename = strrchr(filename, '/');
            basename = basename ? basename + 1 : filename;
            strncpy(ctx->filename, basename, sizeof(ctx->filename) - 1);

            ctx->file_size = st.st_size;
            ctx->bytes_transferred = 0;
            ctx->cancelled = FALSE;
            ctx->start_time = time(NULL);

            // Create progress dialog
            create_progress_dialog(ctx);

            // Start upload thread
            GThread* thread = g_thread_new("upload", upload_thread_func, ctx);
            g_thread_unref(thread);
        }

        g_free(filename);
    }

    gtk_widget_destroy(dialog);
}

void create_progress_dialog(UploadContext* ctx) {
    ctx->dialog = gtk_dialog_new_with_buttons(
        "Uploading...",
        GTK_WINDOW(ctx->app_state->window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Cancel", GTK_RESPONSE_CANCEL,
        NULL);

    GtkWidget* content = gtk_dialog_get_content_area(GTK_DIALOG(ctx->dialog));
    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(content), vbox);

    // File label
    char label_text[300];
    snprintf(label_text, sizeof(label_text), "File: %s", ctx->filename);
    GtkWidget* file_label = gtk_label_new(label_text);
    gtk_box_pack_start(GTK_BOX(vbox), file_label, FALSE, FALSE, 0);

    // Progress bar
    ctx->progress_bar = gtk_progress_bar_new();
    gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(ctx->progress_bar), TRUE);
    gtk_box_pack_start(GTK_BOX(vbox), ctx->progress_bar, FALSE, FALSE, 0);

    // Status label
    ctx->status_label = gtk_label_new("0 B / 0 B");
    gtk_box_pack_start(GTK_BOX(vbox), ctx->status_label, FALSE, FALSE, 0);

    // Speed label
    ctx->speed_label = gtk_label_new("Speed: 0 MB/s  ETA: calculating...");
    gtk_box_pack_start(GTK_BOX(vbox), ctx->speed_label, FALSE, FALSE, 0);

    gtk_widget_show_all(content);

    // Connect cancel signal
    g_signal_connect(ctx->dialog, "response",
                     G_CALLBACK(on_upload_cancel), ctx);

    gtk_widget_show(ctx->dialog);
}

gpointer upload_thread_func(gpointer data) {
    UploadContext* ctx = (UploadContext*)data;

    // Send upload request (metadata)
    cJSON* json = cJSON_CreateObject();
    cJSON_AddNumberToObject(json, "user_id",
                           ctx->app_state->connection->user_id);
    cJSON_AddNumberToObject(json, "parent_id",
                           ctx->app_state->current_directory);
    cJSON_AddStringToObject(json, "name", ctx->filename);
    cJSON_AddNumberToObject(json, "size", ctx->file_size);

    char* payload = cJSON_PrintUnformatted(json);
    Packet* pkt = packet_create(CMD_UPLOAD_REQ, payload, strlen(payload));

    packet_send(ctx->app_state->connection->socket_fd, pkt);

    free(payload);
    packet_free(pkt);
    cJSON_Delete(json);

    // Wait for READY response
    Packet* response = net_recv_packet(ctx->app_state->connection->socket_fd);
    if (!response || response->command != CMD_SUCCESS) {
        if (response) packet_free(response);
        g_idle_add(upload_error_callback, ctx);
        return NULL;
    }
    packet_free(response);

    // Upload file data with progress updates
    FILE* fp = fopen(ctx->local_path, "rb");
    if (!fp) {
        g_idle_add(upload_error_callback, ctx);
        return NULL;
    }

    char buffer[8192];
    size_t bytes_read;

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), fp)) > 0
           && !ctx->cancelled) {
        Packet* data_pkt = packet_create(CMD_UPLOAD_DATA, buffer, bytes_read);
        if (packet_send(ctx->app_state->connection->socket_fd, data_pkt) < 0) {
            packet_free(data_pkt);
            fclose(fp);
            g_idle_add(upload_error_callback, ctx);
            return NULL;
        }
        packet_free(data_pkt);

        ctx->bytes_transferred += bytes_read;

        // Update progress (use g_idle_add for thread safety)
        g_idle_add(update_upload_progress, ctx);

        // Throttle updates (every 100ms)
        g_usleep(100000);
    }

    fclose(fp);

    // Wait for final confirmation
    response = net_recv_packet(ctx->app_state->connection->socket_fd);
    if (!response || response->command != CMD_SUCCESS) {
        if (response) packet_free(response);
        g_idle_add(upload_error_callback, ctx);
        return NULL;
    }
    packet_free(response);

    // Upload complete
    g_idle_add(upload_complete_callback, ctx);

    return NULL;
}

gboolean update_upload_progress(gpointer data) {
    UploadContext* ctx = (UploadContext*)data;

    double fraction = (double)ctx->bytes_transferred / ctx->file_size;
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(ctx->progress_bar),
                                  fraction);

    // Update text
    char text[64];
    snprintf(text, sizeof(text), "%.1f%%", fraction * 100);
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(ctx->progress_bar), text);

    // Update status
    char status[128];
    char transferred_str[32], total_str[32];
    format_file_size(ctx->bytes_transferred, transferred_str,
                    sizeof(transferred_str));
    format_file_size(ctx->file_size, total_str, sizeof(total_str));
    snprintf(status, sizeof(status), "%s / %s", transferred_str, total_str);
    gtk_label_set_text(GTK_LABEL(ctx->status_label), status);

    // Calculate speed and ETA
    time_t elapsed = time(NULL) - ctx->start_time;
    if (elapsed > 0) {
        double speed = (double)ctx->bytes_transferred / elapsed;
        size_t remaining = ctx->file_size - ctx->bytes_transferred;
        int eta_seconds = (int)(remaining / speed);

        char speed_str[128];
        char speed_formatted[32];
        format_file_size((long)speed, speed_formatted,
                        sizeof(speed_formatted));
        snprintf(speed_str, sizeof(speed_str),
                "Speed: %s/s  ETA: %02d:%02d:%02d",
                speed_formatted,
                eta_seconds / 3600,
                (eta_seconds % 3600) / 60,
                eta_seconds % 60);
        gtk_label_set_text(GTK_LABEL(ctx->speed_label), speed_str);
    }

    return G_SOURCE_REMOVE;
}

gboolean upload_complete_callback(gpointer data) {
    UploadContext* ctx = (UploadContext*)data;

    gtk_widget_destroy(ctx->dialog);

    // Show success message
    GtkWidget* msg_dialog = gtk_message_dialog_new(
        GTK_WINDOW(ctx->app_state->window),
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_INFO,
        GTK_BUTTONS_OK,
        "Upload completed successfully!");
    gtk_dialog_run(GTK_DIALOG(msg_dialog));
    gtk_widget_destroy(msg_dialog);

    // Refresh file list
    populate_file_list(ctx->app_state, ctx->app_state->current_directory);

    g_free(ctx);
    return G_SOURCE_REMOVE;
}
```

---

#### Task 2.3.6: File Download (GUI)
**Priority:** HIGH
**Complexity:** HIGH
**Duration:** 4-5 hours

**Implementation:** Similar to upload with these differences:
1. Use GtkFileChooserDialog with SAVE action
2. Reverse data flow (recv instead of send)
3. Handle download response packet first (metadata)
4. Then receive binary data packets

**Code Structure:** (similar to upload, omitted for brevity)

---

#### Task 2.3.7: Folder Upload/Download (GUI)
**Priority:** MEDIUM
**Complexity:** HIGH
**Duration:** 6-8 hours

**Functionality:**
1. Folder chooser dialog
2. Recursive traversal (local → remote)
3. Multiple progress bars or file-by-file progress
4. Overall progress (X/Y files completed)

**UI Design:**
```
+-----------------------------------+
| Uploading Folder...          [X]  |
+-----------------------------------+
| Folder: my_documents              |
| Files: 45 / 120                   |
| Current: report.pdf               |
| [==================>    ] 37.5%   |
| Overall: 450 MB / 1200 MB         |
|                                   |
|             [Cancel]              |
+-----------------------------------+
```

**Implementation Strategy:**
1. Build file list recursively before starting
2. Upload files sequentially (not parallel)
3. Create remote directories as needed
4. Update progress after each file
5. Handle errors (skip file, show summary at end)

---

#### Task 2.3.8: Additional GUI Features
**Priority:** MEDIUM
**Complexity:** MEDIUM
**Duration:** 4-5 hours

**Features to Add:**

1. **Context Menu (Right-Click)**
```c
static gboolean on_tree_view_button_press(GtkWidget* widget,
                                         GdkEventButton* event,
                                         gpointer user_data) {
    if (event->type == GDK_BUTTON_PRESS && event->button == 3) {
        // Right-click
        GtkWidget* menu = gtk_menu_new();

        GtkWidget* download_item = gtk_menu_item_new_with_label("Download");
        g_signal_connect(download_item, "activate",
                        G_CALLBACK(on_menu_download), user_data);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), download_item);

        GtkWidget* delete_item = gtk_menu_item_new_with_label("Delete");
        g_signal_connect(delete_item, "activate",
                        G_CALLBACK(on_menu_delete), user_data);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), delete_item);

        // ... add more menu items ...

        gtk_widget_show_all(menu);
        gtk_menu_popup_at_pointer(GTK_MENU(menu), (GdkEvent*)event);

        return TRUE;
    }
    return FALSE;
}
```

2. **Keyboard Shortcuts**
- F5: Refresh
- Ctrl+U: Upload file
- Ctrl+D: Download selected
- Delete: Delete selected
- Backspace: Navigate to parent

3. **Drag-and-Drop Upload**
```c
// Enable drag-and-drop
gtk_drag_dest_set(tree_view, GTK_DEST_DEFAULT_ALL,
                  NULL, 0, GDK_ACTION_COPY);
gtk_drag_dest_add_uri_targets(tree_view);

g_signal_connect(tree_view, "drag-data-received",
                 G_CALLBACK(on_drag_data_received), state);

// Handler
static void on_drag_data_received(GtkWidget* widget,
                                  GdkDragContext* context,
                                  gint x, gint y,
                                  GtkSelectionData* data,
                                  guint info, guint time,
                                  gpointer user_data) {
    gchar** uris = gtk_selection_data_get_uris(data);
    if (uris) {
        for (int i = 0; uris[i] != NULL; i++) {
            gchar* filename = g_filename_from_uri(uris[i], NULL, NULL);
            if (filename) {
                // Upload file
                start_upload(user_data, filename);
                g_free(filename);
            }
        }
        g_strfreev(uris);
    }
    gtk_drag_finish(context, TRUE, FALSE, time);
}
```

4. **About Dialog**
```c
void show_about_dialog(GtkWindow* parent) {
    const gchar* authors[] = {"Your Name", NULL};

    gtk_show_about_dialog(parent,
        "program-name", "File Sharing Client",
        "version", "1.0.0",
        "comments", "GTK+ 3 client for file sharing system",
        "authors", authors,
        "license-type", GTK_LICENSE_MIT_X11,
        NULL);
}
```

5. **Preferences Dialog**
- Default download location
- Transfer buffer size
- Auto-refresh interval
- Theme selection

---

### 2.4 GUI File Structure (Final)

```
src/client-gui/
├── main.c                   # GTK application entry point
├── gui_main.c               # Main window creation and layout
├── gui_main.h               # Main window structures
├── gui_callbacks.c          # Event handlers (buttons, menu items)
├── gui_callbacks.h          # Callback declarations
├── gui_dialogs.c            # Dialogs (login, error, progress)
├── gui_dialogs.h            # Dialog function declarations
├── gui_file_list.c          # TreeView population and formatting
├── gui_file_list.h          # File list structures
├── gui_transfer.c           # Upload/download with threading
├── gui_transfer.h           # Transfer context structures
├── gui_utils.c              # Utility functions (format size, etc.)
├── gui_utils.h              # Utility declarations
├── ../client/net_handler.c  # SHARED: Network operations
└── Makefile                 # Build configuration
```

---

## Part 3: Code Sharing Strategy

### 3.1 Shared Components

**Network Layer (net_handler.c):**
- ✅ Already implemented and working
- ✅ Used by both CLI and GUI without modification
- ✅ Thread-safe (no global state)

**Protocol Layer (common/):**
- ✅ packet_create, packet_send, packet_recv
- ✅ Command constants (CMD_*)
- ✅ Status codes (STATUS_*)

**Utilities (common/utils.c):**
- ✅ Logging functions
- ✅ UUID generation
- ✅ Timestamp formatting

### 3.2 Client-Specific Components

**CLI Client:**
- `client.c` - High-level operations with printf output
- `main.c` - Command loop, stdin parsing
- `progress.c` - Terminal progress bars

**GUI Client:**
- `gui_main.c` - GTK window management
- `gui_callbacks.c` - GTK event handlers
- `gui_transfer.c` - Threaded transfers with GTK progress bars

### 3.3 Build System Organization

```
Makefile (root)
├── make client         → build CLI client
├── make client-gui     → build GUI client
├── make all           → build both + server + tests
└── make clean         → clean all

src/client/Makefile     → CLI client only
src/client-gui/Makefile → GUI client only
```

**Root Makefile Addition:**
```makefile
.PHONY: client client-gui

client:
	$(MAKE) -C src/common
	$(MAKE) -C src/client

client-gui:
	$(MAKE) -C src/common
	$(MAKE) -C src/client-gui
```

---

## Part 4: Testing Strategy

### 4.1 CLI Client Tests

**Unit Tests:**
- Test each command function independently
- Mock network responses
- Verify error handling

**Integration Tests:**
1. **Basic Operations**
   - Connect, login, list, disconnect
   - Create directory, upload file, download file
   - Change permissions, verify changes

2. **Recursive Operations**
   - Upload empty folder
   - Upload folder with 100 files
   - Upload nested folders (5 levels)
   - Download folder tree
   - Verify all files transferred correctly

3. **Progress Indicators**
   - Upload 100MB file, verify progress updates
   - Upload 500MB file, measure update frequency
   - Cancel mid-transfer

4. **Error Handling**
   - Network disconnect during transfer
   - Permission denied
   - File not found
   - Disk full (local)

**Test Scripts:**
```bash
#!/bin/bash
# tests/test_cli_client.sh

# Setup
./build/server 8080 &
SERVER_PID=$!
sleep 1

# Test 1: Basic connection
./build/client localhost 8080 <<EOF
admin
admin
ls
quit
EOF

# Test 2: Upload
./build/client localhost 8080 <<EOF
admin
admin
upload tests/fixtures/test_file.txt
ls
quit
EOF

# Cleanup
kill $SERVER_PID
```

### 4.2 GUI Client Tests

**Manual Test Checklist:**
```
[ ] Application launches without errors
[ ] Login dialog appears on startup
[ ] Connect to server with valid credentials
[ ] Main window displays root directory
[ ] Double-click folder navigates correctly
[ ] Upload file shows progress dialog
[ ] Upload completes and file appears in list
[ ] Download file to local disk
[ ] Downloaded file content matches original
[ ] Create new folder via button
[ ] Delete file (confirmation dialog)
[ ] Right-click context menu appears
[ ] Keyboard shortcuts work (F5, Ctrl+U, etc.)
[ ] Drag-and-drop file to upload
[ ] Cancel upload mid-transfer
[ ] Disconnect and reconnect
[ ] Error dialogs display correctly
[ ] About dialog shows version info
```

**Automated Tests (using Xvfb + xdotool):**
```bash
# Start virtual display
Xvfb :99 -screen 0 1024x768x24 &
export DISPLAY=:99

# Launch GUI client
./build/client-gui &
GUI_PID=$!
sleep 2

# Simulate user actions with xdotool
xdotool search --name "File Sharing Client" windowactivate
xdotool type "localhost"
xdotool key Tab
xdotool type "8080"
# ... continue simulation ...

kill $GUI_PID
```

### 4.3 Performance Tests

**Metrics to Measure:**
1. **File Transfer Speed**
   - Upload: MB/s for various file sizes
   - Download: MB/s for various file sizes
   - Target: > 10 MB/s on localhost

2. **UI Responsiveness (GUI)**
   - File list population time (< 500ms for 1000 files)
   - Button click response (< 50ms)
   - Progress bar update rate (at least 10 Hz)

3. **Memory Usage**
   - CLI client: < 10 MB
   - GUI client: < 50 MB
   - No memory leaks during 1-hour stress test

4. **Concurrency**
   - Multiple GUI clients connected simultaneously
   - Upload from one, download from another
   - Verify no conflicts or corruption

**Stress Test:**
```bash
# Upload 10,000 small files
for i in {1..10000}; do
    echo "test content $i" > test_$i.txt
done
tar czf test_files.tar.gz test_*.txt
# Upload folder
./build/client localhost 8080 <<EOF
admin
admin
upload_folder test_files/
quit
EOF
```

### 4.4 Cross-Platform Testing

**Platforms:**
- Ubuntu 22.04 LTS (primary)
- Fedora 39
- macOS 13+ (if GTK+ 3 available via Homebrew)

**Compatibility Checks:**
- GTK+ 3 version differences
- File path handling (Unix paths)
- Socket behavior
- Compilation with different GCC versions

---

## Part 5: Implementation Roadmap

### Phase 1: CLI Completion (Week 1)

**Day 1-2: Recursive Operations**
- Task 1.2.1: Recursive folder upload
- Task 1.2.2: Recursive folder download
- Test with various folder structures

**Day 3: Progress & Error Handling**
- Task 1.2.3: Progress indicators
- Task 1.2.4: Enhanced error handling
- Test with large files

**Day 4: Additional Commands & Bug Fixes**
- Task 1.2.5: Add delete, info, cd .. commands
- Task 1.2.6: Fix transient test failure
- Full CLI test suite

**Deliverable:** CLI client 100% complete, all tests passing

---

### Phase 2: GUI Foundation (Week 2)

**Day 1: Setup & Basic Window**
- Task 2.3.1: Project setup, Makefile
- Task 2.3.2: Login dialog
- Task 2.3.3: Main window layout
- Test: Launch app, login, see empty file list

**Day 2: File List Display**
- Task 2.3.4: File list population
- Implement double-click navigation
- Test: Navigate folders, display formatting

**Deliverable:** GUI app connects, displays files, navigates

---

### Phase 3: GUI File Operations (Week 3)

**Day 1-2: Upload**
- Task 2.3.5: File upload with progress
- Test: Upload various file sizes

**Day 2-3: Download**
- Task 2.3.6: File download with progress
- Test: Download files, verify content

**Day 4: Folder Operations**
- Task 2.3.7: Recursive folder upload/download
- Test: Complex folder structures

**Deliverable:** Full upload/download functionality

---

### Phase 4: Polish & Testing (Week 4)

**Day 1-2: Additional Features**
- Task 2.3.8: Context menus, shortcuts, drag-drop
- Implement about dialog, preferences

**Day 3: Testing**
- Run full test suite
- Performance benchmarks
- Fix bugs found

**Day 4: Documentation**
- User guide for both clients
- Installation instructions
- Screenshot examples

**Deliverable:** Production-ready clients, documented

---

## Part 6: Risk Analysis & Mitigation

### 6.1 Technical Risks

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| GTK+ 3 version incompatibility | Medium | High | Test on multiple distros early |
| Threading bugs in GUI | Medium | High | Use GLib threading primitives, mutex locks |
| Memory leaks | Medium | Medium | Run Valgrind regularly |
| Network timeout handling | Low | Medium | Implement robust retry logic |
| Large file transfer crashes | Low | High | Chunked transfer, memory limits |

### 6.2 Schedule Risks

| Risk | Mitigation |
|------|------------|
| GTK+ learning curve steeper than expected | Allocate extra time (buffer days) |
| Test failures difficult to debug | Set up comprehensive logging early |
| Scope creep (feature requests) | Stick to Phase 6 spec, defer extras |

### 6.3 Quality Risks

| Risk | Mitigation |
|------|------------|
| Insufficient testing | Automated test suite + manual checklist |
| Poor error messages | User test feedback, iterate |
| Memory leaks | Valgrind + AddressSanitizer |

---

## Part 7: Success Criteria

### 7.1 CLI Client

✅ **Functional Requirements:**
- All basic commands work (ls, cd, mkdir, upload, download, chmod, pwd)
- Recursive folder upload/download
- Progress bars for large transfers
- Clear error messages with status codes
- All tests pass (0 failures)

✅ **Performance Requirements:**
- Transfer speed > 10 MB/s on localhost
- Memory usage < 10 MB
- No memory leaks (Valgrind clean)

✅ **Usability Requirements:**
- Help command lists all operations
- Intuitive command syntax
- Responsive (no blocking on operations)

---

### 7.2 GUI Client

✅ **Functional Requirements:**
- Login dialog on startup
- File browser with sortable columns
- Upload/download files and folders
- Progress bars with speed/ETA
- Context menu (right-click)
- Keyboard shortcuts
- Drag-and-drop upload
- Error dialogs for failures

✅ **Performance Requirements:**
- Transfer speed > 10 MB/s
- UI remains responsive during transfers
- File list loads < 500ms for 1000 items
- Memory usage < 50 MB

✅ **Usability Requirements:**
- Intuitive UI matching file manager conventions
- Visual feedback for all operations
- Error messages user-friendly
- No crashes or freezes

---

## Part 8: Unresolved Questions

1. **Server API Clarification:**
   - Does LIST_DIR response include parent_id for "cd .." implementation?
   - Are timestamps included in file metadata? Format?
   - How to get file owner username (not just ID)?

2. **Folder Operations:**
   - Should recursive upload preserve timestamps?
   - How to handle symbolic links?
   - Maximum folder depth limit?

3. **GUI Threading:**
   - Can we safely use GLib's GTask instead of manual threading?
   - Should we limit concurrent transfers (max 3 at once)?

4. **Error Recovery:**
   - Should interrupted transfers be resumable?
   - Should we implement client-side caching of directory listings?

5. **Cross-Platform:**
   - Do we need Windows support (GTK+ 3 on Windows)?
   - macOS testing required?

6. **Security:**
   - Should passwords be masked in memory (secure strings)?
   - Implement session timeout?

---

## Appendix A: GTK+ 3 Key Concepts

### A.1 Core Widgets

| Widget | Purpose | Example Use |
|--------|---------|-------------|
| GtkWindow | Top-level window | Main application window |
| GtkDialog | Modal dialog | Login, error messages |
| GtkBox | Layout container | Vertical/horizontal stacking |
| GtkGrid | 2D layout | Form layouts (label + entry) |
| GtkButton | Clickable button | Upload, Download buttons |
| GtkLabel | Text display | Status messages |
| GtkEntry | Text input | Username, password fields |
| GtkTreeView | List/tree view | File browser |
| GtkListStore | Data model | File entries data |
| GtkProgressBar | Progress indicator | Upload/download progress |
| GtkMenu | Popup menu | Context menu (right-click) |
| GtkFileChooserDialog | File picker | Select files to upload |

### A.2 Signal Handling

**Signals** are GTK's event system (like callbacks).

**Common Signals:**
- `clicked` - Button pressed
- `activate` - Menu item selected
- `row-activated` - TreeView row double-clicked
- `destroy` - Widget destroyed
- `response` - Dialog button pressed

**Connection:**
```c
g_signal_connect(button, "clicked", G_CALLBACK(on_button_clicked), user_data);
```

### A.3 Threading in GTK+

**Rule:** Only main thread can modify UI widgets.

**Solution:** Use `g_idle_add()` from worker threads:
```c
// Worker thread
gboolean update_ui(gpointer data) {
    // This runs in main thread
    gtk_label_set_text(label, "Updated!");
    return G_SOURCE_REMOVE;  // Don't repeat
}

// From worker thread
g_idle_add(update_ui, NULL);
```

### A.4 Memory Management

**GLib Objects:** Use `g_object_unref()` when done.
**GLib Strings:** Use `g_free()` (not `free()`).
**Standard C:** Use `free()` as usual.

**Example:**
```c
GtkApplication* app = gtk_application_new(...);
// ... use app ...
g_object_unref(app);  // Decrease reference count

gchar* str = g_strdup("test");
g_free(str);  // GLib string
```

---

## Appendix B: File Locations Reference

### B.1 Existing Files (CLI)

```
src/client/
├── client.c          # Command implementations (ls, cd, upload, etc.)
├── client.h          # ClientConnection struct, function declarations
├── main.c            # CLI command loop
├── net_handler.c     # Socket operations, packet send/recv
├── net_handler.h     # Network function declarations
└── Makefile          # Build script for CLI client
```

### B.2 New Files (CLI Enhancements)

```
src/client/
├── client.c          # MODIFY: Add recursive upload/download, delete, info
├── client.h          # MODIFY: Add new function declarations
├── main.c            # MODIFY: Add new command parsing
├── net_handler.c     # MODIFY: Add progress callbacks
├── net_handler.h     # MODIFY: Add callback typedef
├── progress.c        # NEW: Terminal progress bar implementation
└── progress.h        # NEW: Progress bar API
```

### B.3 New Files (GUI Client)

```
src/client-gui/       # NEW DIRECTORY
├── main.c            # GTK application entry
├── gui_main.c        # Main window
├── gui_main.h
├── gui_callbacks.c   # Event handlers
├── gui_callbacks.h
├── gui_dialogs.c     # Login, error, progress dialogs
├── gui_dialogs.h
├── gui_file_list.c   # TreeView management
├── gui_file_list.h
├── gui_transfer.c    # Threaded transfers
├── gui_transfer.h
├── gui_utils.c       # Formatting utilities
├── gui_utils.h
└── Makefile          # GUI build script
```

---

## Summary

**Total Effort:** 40-56 hours (5-7 working days)

**CLI Completion:** 8-12 hours
- Recursive operations
- Progress indicators
- Error handling
- Bug fixes

**GUI Implementation:** 24-32 hours
- GTK+ setup
- Main window & file list
- Upload/download with progress
- Additional features (menus, shortcuts, drag-drop)

**Testing:** 8-12 hours
- Unit tests
- Integration tests
- Performance benchmarks
- Manual testing

**Key Success Factors:**
1. Reuse existing network code (no duplication)
2. Implement threading correctly (UI responsiveness)
3. Comprehensive error handling
4. Thorough testing at each milestone
5. User-friendly error messages and feedback

**Next Steps:**
1. Review this plan with stakeholders
2. Resolve unresolved questions (Appendix)
3. Set up development environment (GTK+ 3 installation)
4. Begin Phase 1: CLI completion
