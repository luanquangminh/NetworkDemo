# Admin Log Viewer Feature - Implementation Plan

**Date:** 2026-01-09
**Feature:** Admin Activity Log Viewing in GUI Client
**Status:** Design Complete - Ready for Implementation

---

## Executive Summary

Add admin-only log viewing feature to Network File Manager GUI, allowing administrators to monitor system activity through database activity logs with filtering and real-time refresh capabilities.

**Data Source Decision:** Use `activity_logs` database table (NOT server.log file)
- **Rationale:** Structured, queryable, already contains comprehensive activity tracking, easier filtering/pagination
- **server.log:** Unstructured text, includes low-level debug info, harder to parse/filter

---

## Research Findings

### Current System Architecture

**Database Schema:**
```sql
activity_logs (
  id INTEGER PRIMARY KEY,
  user_id INTEGER NOT NULL,
  action_type TEXT NOT NULL,  -- LOGIN, LIST_DIR, DOWNLOAD, etc.
  description TEXT,
  timestamp TEXT DEFAULT CURRENT_TIMESTAMP,
  FOREIGN KEY (user_id) REFERENCES users(id)
)
```

**Current Activity Types (Top 10):**
- LIST_DIR (257 entries)
- LOGIN (132)
- CHANGE_DIR (97)
- DOWNLOAD (31)
- ADMIN_LIST_USERS (26)
- UPLOAD (25)
- SEARCH (16)
- MAKE_DIR (12)
- ACCESS_DENIED (8)
- MOVE (5)

**Existing Database Function:**
```c
int db_list_activity_logs(Database* db,
                          int user_id_filter,        // 0 = all users
                          const char* action_type_filter,  // NULL = all types
                          const char* start_date,     // YYYY-MM-DD format
                          const char* end_date,       // YYYY-MM-DD format
                          int limit,                  // max results
                          char** json_result);        // JSON array output
```
- **Returns:** JSON array with fields: `id`, `user_id`, `username`, `action_type`, `description`, `timestamp`
- **Filtering:** Supports user, action type, date range, result limit
- **Already Implemented:** Fully functional, no backend changes needed

**Admin Dashboard Pattern:**
- Location: `src/client/gui/admin_dashboard.c`
- Structure: Separate window with toolbar, TreeView, status bar
- Admin Check: Server validates `db_is_admin()` before serving data
- Refresh Pattern: Manual refresh via toolbar button + initial load on `g_idle_add`

**Protocol Pattern:**
- Admin commands range: `0x50-0x5F`
- Next available: `0x54` (after CMD_ADMIN_UPDATE_USER = 0x53)
- Request/Response: JSON payloads with status field

---

## Architecture Design

### Data Flow

```
┌──────────────┐      CMD_ADMIN_VIEW_LOGS       ┌──────────────┐
│   GTK GUI    │  ──────────────────────────>   │    Server    │
│  Log Viewer  │                                 │   Handler    │
│              │  <──────────────────────────    │              │
└──────────────┘      JSON logs array           └──────────────┘
                                                        │
                                                        ▼
                                                 ┌──────────────┐
                                                 │   Database   │
                                                 │ activity_logs│
                                                 └──────────────┘
```

### Protocol Specification

**New Command ID:**
```c
#define CMD_ADMIN_VIEW_LOGS  0x54
```

**Request Payload (JSON):**
```json
{
  "user_id": 0,              // 0 = all users, >0 = specific user
  "action_type": "",         // "" = all, "LOGIN", "DOWNLOAD", etc.
  "start_date": "",          // "YYYY-MM-DD" or empty
  "end_date": "",            // "YYYY-MM-DD" or empty
  "limit": 1000              // max records (default 1000, max 10000)
}
```

**Response Payload (JSON):**
```json
{
  "status": "OK",
  "logs": [
    {
      "id": 625,
      "user_id": 1,
      "username": "admin",
      "action_type": "LOGIN",
      "description": "User logged in successfully",
      "timestamp": "2026-01-09 15:49:41"
    },
    ...
  ]
}
```

**Error Response:**
```json
{
  "status": "ERROR",
  "message": "Admin access required"
}
```

---

## UI Design

### Window Layout

```
┌─────────────────────────────────────────────────────────────┐
│ Admin Log Viewer                                         [X] │
├─────────────────────────────────────────────────────────────┤
│ File                                                         │
│   └─ Close                                                   │
├─────────────────────────────────────────────────────────────┤
│ [🔄 Refresh] [Filter▼]                                       │
├─────────────────────────────────────────────────────────────┤
│ Filter Options: (Collapsible Panel)                         │
│   User: [All Users     ▼]  Action: [All Actions   ▼]       │
│   From: [YYYY-MM-DD    ]   To: [YYYY-MM-DD      ]          │
│   Limit: [1000         ]   [Apply Filter]                   │
├─────────────────────────────────────────────────────────────┤
│ ID │ Timestamp           │ User    │ Action    │ Description│
├────┼─────────────────────┼─────────┼───────────┼────────────┤
│ 625│ 2026-01-09 15:49:41 │ admin   │ LOGIN     │ User log...│
│ 626│ 2026-01-09 15:49:41 │ admin   │ ADMIN_... │ Listed a...│
│ ... (scrollable)                                            │
├─────────────────────────────────────────────────────────────┤
│ Total logs: 626                                             │
└─────────────────────────────────────────────────────────────┘
```

**Widget Hierarchy:**
```
GtkWindow (log_viewer_window)
├── GtkBox (vbox, vertical)
│   ├── GtkMenuBar
│   │   └── File → Close
│   ├── GtkToolbar
│   │   ├── Refresh (icon: view-refresh)
│   │   └── Toggle Filter (icon: funnel)
│   ├── GtkExpander (filter_expander)
│   │   └── GtkGrid (2 rows x 4 columns)
│   │       ├── User ComboBox (populated with all users)
│   │       ├── Action Type ComboBox (LOGIN, DOWNLOAD, etc.)
│   │       ├── Start Date Entry (GtkEntry with date format)
│   │       ├── End Date Entry
│   │       ├── Limit SpinButton (100-10000)
│   │       └── Apply Filter Button
│   ├── GtkScrolledWindow
│   │   └── GtkTreeView (log_tree_view)
│   │       └── GtkListStore (5 columns: ID, Timestamp, User, Action, Description)
│   └── GtkStatusBar
```

### Column Configuration

| Column | Type | Width | Sortable | Description |
|--------|------|-------|----------|-------------|
| ID | INT | 80px | Yes | Log entry ID |
| Timestamp | STRING | 180px | Yes | YYYY-MM-DD HH:MM:SS |
| User | STRING | 120px | Yes | Username |
| Action | STRING | 140px | Yes | Action type |
| Description | STRING | 300px+ | No | Full description (expandable) |

---

## Implementation Phases

### Phase 1: Protocol & Backend (Server-side)

**Files to Modify:**

**1.1 `src/common/protocol.h`** (Lines 14-37)
```c
// Add after CMD_ADMIN_UPDATE_USER (line 35)
#define CMD_ADMIN_VIEW_LOGS  0x54
```

**1.2 `src/server/commands.h`** (Lines 30-34)
```c
// Add after handle_admin_update_user declaration
void handle_admin_view_logs(ClientSession* session, Packet* pkt);
```

**1.3 `src/server/commands.c`**

**Location 1:** `dispatch_command()` switch statement (Lines 73-84)
```c
// Add after CMD_ADMIN_UPDATE_USER case
case CMD_ADMIN_VIEW_LOGS:
    handle_admin_view_logs(session, pkt);
    break;
```

**Location 2:** End of file (After `handle_admin_update_user()`)
```c
void handle_admin_view_logs(ClientSession* session, Packet* pkt) {
    // Admin authorization check
    if (!db_is_admin(global_db, session->user_id)) {
        send_error(session, "Admin access required");
        log_info("Non-admin user %d attempted to view logs", session->user_id);
        return;
    }

    // Parse request JSON
    if (!pkt->payload) {
        send_error(session, "Empty payload");
        return;
    }

    cJSON* request = cJSON_Parse(pkt->payload);
    if (!request) {
        send_error(session, "Invalid JSON");
        return;
    }

    // Extract filter parameters (with defaults)
    cJSON* user_id_json = cJSON_GetObjectItem(request, "user_id");
    cJSON* action_type_json = cJSON_GetObjectItem(request, "action_type");
    cJSON* start_date_json = cJSON_GetObjectItem(request, "start_date");
    cJSON* end_date_json = cJSON_GetObjectItem(request, "end_date");
    cJSON* limit_json = cJSON_GetObjectItem(request, "limit");

    int user_id_filter = user_id_json ? user_id_json->valueint : 0;
    const char* action_type_filter = (action_type_json && strlen(cJSON_GetStringValue(action_type_json)) > 0)
                                     ? cJSON_GetStringValue(action_type_json) : NULL;
    const char* start_date = (start_date_json && strlen(cJSON_GetStringValue(start_date_json)) > 0)
                             ? cJSON_GetStringValue(start_date_json) : NULL;
    const char* end_date = (end_date_json && strlen(cJSON_GetStringValue(end_date_json)) > 0)
                           ? cJSON_GetStringValue(end_date_json) : NULL;
    int limit = limit_json ? limit_json->valueint : 1000;

    // Enforce limit constraints
    if (limit < 1) limit = 100;
    if (limit > 10000) limit = 10000;

    // Query database
    char* logs_json = NULL;
    if (db_list_activity_logs(global_db, user_id_filter, action_type_filter,
                              start_date, end_date, limit, &logs_json) < 0) {
        send_error(session, "Failed to retrieve logs");
        cJSON_Delete(request);
        return;
    }

    // Build response
    cJSON* response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "status", "OK");
    cJSON_AddRawToObject(response, "logs", logs_json ? logs_json : "[]");

    char* payload = cJSON_PrintUnformatted(response);
    Packet* res = packet_create(CMD_SUCCESS, payload, strlen(payload));
    packet_send(session->socket_fd, res);

    // Log activity
    db_log_activity(global_db, session->user_id, "ADMIN_VIEW_LOGS",
                   "Viewed activity logs");
    log_info("Admin user %d viewed activity logs (filters: user=%d, action=%s, limit=%d)",
             session->user_id, user_id_filter, action_type_filter ? action_type_filter : "all", limit);

    // Cleanup
    free(logs_json);
    free(payload);
    cJSON_Delete(response);
    cJSON_Delete(request);
    packet_free(res);
}
```

**Testing Command:**
```bash
# Verify compilation
make server

# Test admin check (should fail for non-admin)
# Test log retrieval (should return JSON array)
# Test filters (user_id, action_type, date range)
```

---

### Phase 2: Client API Layer

**File:** `src/client/client.h` (Lines 51-56)

**Add declaration:**
```c
// After client_admin_update_user
void* client_admin_view_logs(ClientConnection* conn, int user_id_filter,
                             const char* action_type, const char* start_date,
                             const char* end_date, int limit);
```

**File:** `src/client/client.c` (After `client_admin_update_user()`)

```c
void* client_admin_view_logs(ClientConnection* conn, int user_id_filter,
                             const char* action_type, const char* start_date,
                             const char* end_date, int limit) {
    if (!conn || !conn->authenticated) {
        fprintf(stderr, "Not connected or authenticated\n");
        return NULL;
    }

    // Build request JSON
    cJSON* request = cJSON_CreateObject();
    cJSON_AddNumberToObject(request, "user_id", user_id_filter);
    cJSON_AddStringToObject(request, "action_type", action_type ? action_type : "");
    cJSON_AddStringToObject(request, "start_date", start_date ? start_date : "");
    cJSON_AddStringToObject(request, "end_date", end_date ? end_date : "");
    cJSON_AddNumberToObject(request, "limit", limit > 0 ? limit : 1000);

    char* payload = cJSON_PrintUnformatted(request);
    Packet* pkt = packet_create(CMD_ADMIN_VIEW_LOGS, payload, strlen(payload));

    // Send request
    if (packet_send(conn->socket_fd, pkt) < 0) {
        fprintf(stderr, "Failed to send view logs request\n");
        free(payload);
        cJSON_Delete(request);
        packet_free(pkt);
        return NULL;
    }

    // Receive response
    Packet* response = NULL;
    if (packet_recv(conn->socket_fd, response) < 0) {
        fprintf(stderr, "Failed to receive logs response\n");
        free(payload);
        cJSON_Delete(request);
        packet_free(pkt);
        return NULL;
    }

    // Parse response
    cJSON* json_response = cJSON_Parse(response->payload);
    if (!json_response) {
        fprintf(stderr, "Invalid JSON response\n");
        free(payload);
        cJSON_Delete(request);
        packet_free(pkt);
        packet_free(response);
        return NULL;
    }

    // Check status
    cJSON* status = cJSON_GetObjectItem(json_response, "status");
    if (!status || strcmp(cJSON_GetStringValue(status), "OK") != 0) {
        cJSON* error = cJSON_GetObjectItem(json_response, "message");
        fprintf(stderr, "Error: %s\n", error ? cJSON_GetStringValue(error) : "Unknown error");
        free(payload);
        cJSON_Delete(request);
        cJSON_Delete(json_response);
        packet_free(pkt);
        packet_free(response);
        return NULL;
    }

    // Cleanup and return
    free(payload);
    cJSON_Delete(request);
    packet_free(pkt);
    packet_free(response);

    return json_response;  // Caller must cJSON_Delete()
}
```

**Testing:**
```bash
make client_gui
# Test connection, authentication, log retrieval
# Verify JSON parsing and error handling
```

---

### Phase 3: GUI Components

**3.1 New Files to Create:**

**File:** `src/client/gui/log_viewer.h`
```c
#ifndef LOG_VIEWER_H
#define LOG_VIEWER_H

#include <gtk/gtk.h>
#include "../client.h"

// Log viewer state
typedef struct {
    GtkWidget *window;
    GtkWidget *tree_view;
    GtkListStore *log_store;
    GtkWidget *status_bar;
    GtkWidget *filter_expander;

    // Filter widgets
    GtkWidget *user_combo;
    GtkWidget *action_combo;
    GtkWidget *start_date_entry;
    GtkWidget *end_date_entry;
    GtkWidget *limit_spin;

    ClientConnection *conn;

    // Current filter state
    int current_user_filter;
    char current_action_filter[64];
    char current_start_date[32];
    char current_end_date[32];
    int current_limit;
} LogViewerState;

// Create log viewer window
GtkWidget* create_log_viewer(ClientConnection *conn);

// Refresh logs from server
void refresh_logs(LogViewerState *state);

#endif // LOG_VIEWER_H
```

**File:** `src/client/gui/log_viewer.c`

```c
#include "log_viewer.h"
#include "gui.h"
#include "../../lib/cJSON/cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Column indices
enum {
    COL_LOG_ID = 0,
    COL_TIMESTAMP,
    COL_USERNAME,
    COL_ACTION_TYPE,
    COL_DESCRIPTION,
    NUM_LOG_COLS
};

// Forward declarations
static void on_refresh_clicked(GtkWidget *widget, LogViewerState *state);
static void on_filter_apply_clicked(GtkWidget *widget, LogViewerState *state);
static void on_close_activate(GtkWidget *widget, LogViewerState *state);
static void on_window_destroy(GtkWidget *widget, LogViewerState *state);

// Populate action type combo box
static void populate_action_types(GtkComboBoxText *combo) {
    gtk_combo_box_text_append_text(combo, "All Actions");
    gtk_combo_box_text_append_text(combo, "LOGIN");
    gtk_combo_box_text_append_text(combo, "LIST_DIR");
    gtk_combo_box_text_append_text(combo, "CHANGE_DIR");
    gtk_combo_box_text_append_text(combo, "DOWNLOAD");
    gtk_combo_box_text_append_text(combo, "UPLOAD");
    gtk_combo_box_text_append_text(combo, "MAKE_DIR");
    gtk_combo_box_text_append_text(combo, "DELETE");
    gtk_combo_box_text_append_text(combo, "SEARCH");
    gtk_combo_box_text_append_text(combo, "ACCESS_DENIED");
    gtk_combo_box_text_append_text(combo, "ADMIN_LIST_USERS");
    gtk_combo_box_text_append_text(combo, "ADMIN_CREATE_USER");
    gtk_combo_box_text_append_text(combo, "ADMIN_DELETE_USER");
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);
}

// Populate user combo box
static void populate_users(LogViewerState *state) {
    GtkComboBoxText *combo = GTK_COMBO_BOX_TEXT(state->user_combo);
    gtk_combo_box_text_append_text(combo, "All Users");

    // Fetch user list from server
    cJSON* response = client_admin_list_users(state->conn);
    if (!response) return;

    cJSON* users = cJSON_GetObjectItem(response, "users");
    if (users && cJSON_IsArray(users)) {
        int count = cJSON_GetArraySize(users);
        for (int i = 0; i < count; i++) {
            cJSON* user = cJSON_GetArrayItem(users, i);
            cJSON* username = cJSON_GetObjectItem(user, "username");
            if (username) {
                gtk_combo_box_text_append_text(combo, cJSON_GetStringValue(username));
            }
        }
    }

    gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);
    cJSON_Delete(response);
}

// Refresh logs from server
void refresh_logs(LogViewerState *state) {
    if (!state || !state->conn) return;

    // Clear existing logs
    gtk_list_store_clear(state->log_store);

    // Get logs from server
    const char* action_filter = (state->current_action_filter[0] == '\0') ? NULL : state->current_action_filter;
    const char* start_filter = (state->current_start_date[0] == '\0') ? NULL : state->current_start_date;
    const char* end_filter = (state->current_end_date[0] == '\0') ? NULL : state->current_end_date;

    cJSON* response = client_admin_view_logs(state->conn, state->current_user_filter,
                                             action_filter, start_filter, end_filter,
                                             state->current_limit);
    if (!response) {
        show_error_dialog(state->window, "Failed to retrieve logs");
        return;
    }

    cJSON* logs = cJSON_GetObjectItem(response, "logs");
    if (!logs || !cJSON_IsArray(logs)) {
        cJSON_Delete(response);
        return;
    }

    int log_count = cJSON_GetArraySize(logs);

    // Populate list store
    for (int i = 0; i < log_count; i++) {
        cJSON* log = cJSON_GetArrayItem(logs, i);
        if (!log) continue;

        cJSON* id = cJSON_GetObjectItem(log, "id");
        cJSON* timestamp = cJSON_GetObjectItem(log, "timestamp");
        cJSON* username = cJSON_GetObjectItem(log, "username");
        cJSON* action_type = cJSON_GetObjectItem(log, "action_type");
        cJSON* description = cJSON_GetObjectItem(log, "description");

        GtkTreeIter iter;
        gtk_list_store_append(state->log_store, &iter);
        gtk_list_store_set(state->log_store, &iter,
                          COL_LOG_ID, id ? id->valueint : 0,
                          COL_TIMESTAMP, timestamp ? cJSON_GetStringValue(timestamp) : "",
                          COL_USERNAME, username ? cJSON_GetStringValue(username) : "Unknown",
                          COL_ACTION_TYPE, action_type ? cJSON_GetStringValue(action_type) : "",
                          COL_DESCRIPTION, description ? cJSON_GetStringValue(description) : "",
                          -1);
    }

    // Update status bar
    char status_text[256];
    snprintf(status_text, sizeof(status_text), "Total logs: %d", log_count);
    gtk_statusbar_pop(GTK_STATUSBAR(state->status_bar), 0);
    gtk_statusbar_push(GTK_STATUSBAR(state->status_bar), 0, status_text);

    cJSON_Delete(response);
}

// Callbacks
static void on_refresh_clicked(GtkWidget *widget, LogViewerState *state) {
    refresh_logs(state);
}

static void on_filter_apply_clicked(GtkWidget *widget, LogViewerState *state) {
    // Get filter values
    const char* selected_user = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(state->user_combo));
    state->current_user_filter = (strcmp(selected_user, "All Users") == 0) ? 0 : -1; // TODO: map username to ID

    const char* selected_action = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(state->action_combo));
    if (strcmp(selected_action, "All Actions") == 0) {
        state->current_action_filter[0] = '\0';
    } else {
        strncpy(state->current_action_filter, selected_action, sizeof(state->current_action_filter) - 1);
    }

    const char* start_date = gtk_entry_get_text(GTK_ENTRY(state->start_date_entry));
    strncpy(state->current_start_date, start_date ? start_date : "", sizeof(state->current_start_date) - 1);

    const char* end_date = gtk_entry_get_text(GTK_ENTRY(state->end_date_entry));
    strncpy(state->current_end_date, end_date ? end_date : "", sizeof(state->current_end_date) - 1);

    state->current_limit = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(state->limit_spin));

    // Refresh with new filters
    refresh_logs(state);
}

static void on_close_activate(GtkWidget *widget, LogViewerState *state) {
    gtk_widget_destroy(state->window);
}

static void on_window_destroy(GtkWidget *widget, LogViewerState *state) {
    if (state) {
        g_free(state);
    }
}

// Create log viewer window
GtkWidget* create_log_viewer(ClientConnection *conn) {
    LogViewerState *state = g_new0(LogViewerState, 1);
    state->conn = conn;
    state->current_user_filter = 0;
    state->current_action_filter[0] = '\0';
    state->current_start_date[0] = '\0';
    state->current_end_date[0] = '\0';
    state->current_limit = 1000;

    // Main window
    state->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(state->window), "Admin Log Viewer");
    gtk_window_set_default_size(GTK_WINDOW(state->window), 1000, 600);
    g_signal_connect(state->window, "destroy", G_CALLBACK(on_window_destroy), state);

    // Main container
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(state->window), vbox);

    // Menu bar
    GtkWidget *menubar = gtk_menu_bar_new();
    GtkWidget *file_menu = gtk_menu_new();
    GtkWidget *file_item = gtk_menu_item_new_with_label("File");

    GtkWidget *close_item = gtk_menu_item_new_with_label("Close");
    g_signal_connect(close_item, "activate", G_CALLBACK(on_close_activate), state);
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), close_item);

    gtk_menu_item_set_submenu(GTK_MENU_ITEM(file_item), file_menu);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), file_item);
    gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);

    // Toolbar
    GtkWidget *toolbar = gtk_toolbar_new();
    gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 0);

    GtkToolItem *refresh_btn = gtk_tool_button_new(NULL, "Refresh");
    gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(refresh_btn), "view-refresh");
    g_signal_connect(refresh_btn, "clicked", G_CALLBACK(on_refresh_clicked), state);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), refresh_btn, -1);

    // Filter expander
    state->filter_expander = gtk_expander_new("Filter Options");
    gtk_box_pack_start(GTK_BOX(vbox), state->filter_expander, FALSE, FALSE, 5);

    GtkWidget *filter_grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(filter_grid), 5);
    gtk_grid_set_column_spacing(GTK_GRID(filter_grid), 10);
    gtk_container_set_border_width(GTK_CONTAINER(filter_grid), 10);
    gtk_container_add(GTK_CONTAINER(state->filter_expander), filter_grid);

    // Row 1: User and Action filters
    GtkWidget *user_label = gtk_label_new("User:");
    state->user_combo = gtk_combo_box_text_new();
    populate_users(state);
    gtk_grid_attach(GTK_GRID(filter_grid), user_label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(filter_grid), state->user_combo, 1, 0, 1, 1);

    GtkWidget *action_label = gtk_label_new("Action:");
    state->action_combo = gtk_combo_box_text_new();
    populate_action_types(GTK_COMBO_BOX_TEXT(state->action_combo));
    gtk_grid_attach(GTK_GRID(filter_grid), action_label, 2, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(filter_grid), state->action_combo, 3, 0, 1, 1);

    // Row 2: Date filters
    GtkWidget *start_label = gtk_label_new("From (YYYY-MM-DD):");
    state->start_date_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(state->start_date_entry), "2026-01-01");
    gtk_grid_attach(GTK_GRID(filter_grid), start_label, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(filter_grid), state->start_date_entry, 1, 1, 1, 1);

    GtkWidget *end_label = gtk_label_new("To (YYYY-MM-DD):");
    state->end_date_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(state->end_date_entry), "2026-12-31");
    gtk_grid_attach(GTK_GRID(filter_grid), end_label, 2, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(filter_grid), state->end_date_entry, 3, 1, 1, 1);

    // Row 3: Limit and Apply button
    GtkWidget *limit_label = gtk_label_new("Max Results:");
    state->limit_spin = gtk_spin_button_new_with_range(100, 10000, 100);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(state->limit_spin), 1000);
    gtk_grid_attach(GTK_GRID(filter_grid), limit_label, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(filter_grid), state->limit_spin, 1, 2, 1, 1);

    GtkWidget *apply_btn = gtk_button_new_with_label("Apply Filter");
    g_signal_connect(apply_btn, "clicked", G_CALLBACK(on_filter_apply_clicked), state);
    gtk_grid_attach(GTK_GRID(filter_grid), apply_btn, 2, 2, 2, 1);

    // Tree view
    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled, TRUE, TRUE, 0);

    state->log_store = gtk_list_store_new(NUM_LOG_COLS,
                                          G_TYPE_INT,     // ID
                                          G_TYPE_STRING,  // Timestamp
                                          G_TYPE_STRING,  // Username
                                          G_TYPE_STRING,  // Action Type
                                          G_TYPE_STRING   // Description
                                         );

    state->tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(state->log_store));
    gtk_container_add(GTK_CONTAINER(scrolled), state->tree_view);

    // Columns
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();

    GtkTreeViewColumn *col_id = gtk_tree_view_column_new_with_attributes(
        "ID", renderer, "text", COL_LOG_ID, NULL);
    gtk_tree_view_column_set_sort_column_id(col_id, COL_LOG_ID);
    gtk_tree_view_append_column(GTK_TREE_VIEW(state->tree_view), col_id);

    GtkTreeViewColumn *col_timestamp = gtk_tree_view_column_new_with_attributes(
        "Timestamp", renderer, "text", COL_TIMESTAMP, NULL);
    gtk_tree_view_column_set_sort_column_id(col_timestamp, COL_TIMESTAMP);
    gtk_tree_view_append_column(GTK_TREE_VIEW(state->tree_view), col_timestamp);

    GtkTreeViewColumn *col_user = gtk_tree_view_column_new_with_attributes(
        "User", renderer, "text", COL_USERNAME, NULL);
    gtk_tree_view_column_set_sort_column_id(col_user, COL_USERNAME);
    gtk_tree_view_append_column(GTK_TREE_VIEW(state->tree_view), col_user);

    GtkTreeViewColumn *col_action = gtk_tree_view_column_new_with_attributes(
        "Action", renderer, "text", COL_ACTION_TYPE, NULL);
    gtk_tree_view_column_set_sort_column_id(col_action, COL_ACTION_TYPE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(state->tree_view), col_action);

    GtkTreeViewColumn *col_desc = gtk_tree_view_column_new_with_attributes(
        "Description", renderer, "text", COL_DESCRIPTION, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(state->tree_view), col_desc);

    // Status bar
    state->status_bar = gtk_statusbar_new();
    gtk_box_pack_start(GTK_BOX(vbox), state->status_bar, FALSE, FALSE, 0);
    gtk_statusbar_push(GTK_STATUSBAR(state->status_bar), 0, "Ready");

    gtk_widget_show_all(state->window);

    // Initial load
    g_idle_add((GSourceFunc)refresh_logs, state);

    return state->window;
}
```

**3.2 Integration with Admin Dashboard**

**File:** `src/client/gui/admin_dashboard.c`

**Modify:** Add "View Logs" menu item and toolbar button

**Location:** After menubar setup (around line 380)
```c
// Add to File menu (after logout, before separator)
GtkWidget *view_logs_item = gtk_menu_item_new_with_label("View Activity Logs");
g_signal_connect(view_logs_item, "activate", G_CALLBACK(on_view_logs_activate), state);
gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), view_logs_item);
```

**Location:** Toolbar setup (around line 400)
```c
// Add after refresh button
GtkToolItem *logs_btn = gtk_tool_button_new(NULL, "View Logs");
gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(logs_btn), "text-x-generic");
g_signal_connect(logs_btn, "clicked", G_CALLBACK(on_view_logs_activate), state);
gtk_toolbar_insert(GTK_TOOLBAR(toolbar), logs_btn, -1);
```

**Add callback function:**
```c
#include "log_viewer.h"

static void on_view_logs_activate(GtkWidget *widget, AdminState *state) {
    create_log_viewer(state->conn);
}
```

**3.3 Build System Updates**

**File:** `src/client/gui/Makefile`

**Add to SOURCES:**
```makefile
SOURCES = main.c login_dialog.c main_window.c dialogs.c file_operations.c \
          admin_dashboard.c log_viewer.c
```

---

### Phase 4: Testing & Validation

**4.1 Security Testing**

Test Case | Expected Result
----------|----------------
Non-admin user calls CMD_ADMIN_VIEW_LOGS | Server returns "Admin access required" error
Non-admin tries to open log viewer UI | UI button disabled/hidden
Admin views logs | Full log list returned
Inject SQL in filters | Database query sanitized (prepared statements protect)

**4.2 Functional Testing**

Test Case | Steps | Expected Result
----------|-------|----------------
View all logs | Click "View Logs" → No filters | Display all activity logs up to limit
Filter by user | Select user from dropdown → Apply | Only logs for that user shown
Filter by action | Select "DOWNLOAD" → Apply | Only download actions shown
Filter by date range | Enter 2026-01-01 to 2026-01-09 → Apply | Only logs in date range
Combine filters | User=admin, Action=LOGIN, Date range | Only matching logs shown
Refresh logs | Make server activity → Click Refresh | New logs appear
Limit results | Set limit=100 → Apply | Max 100 logs shown
Empty result | Filter with no matches | Empty list + status "0 logs"

**4.3 Performance Testing**

Scenario | Test | Acceptance Criteria
---------|------|--------------------
Large dataset | Query 10,000 logs | Response < 2 seconds, UI responsive
Network latency | Simulate 200ms latency | Loading indicator, no freeze
Rapid refresh | Click refresh 10 times quickly | No crashes, last request wins

**4.4 UI/UX Testing**

Element | Test | Expected
--------|------|----------
Column sorting | Click column headers | Sort ascending/descending
Window resize | Drag window edges | Columns auto-adjust
Filter toggle | Expand/collapse filter panel | Smooth animation
Status bar | View logs | Shows "Total logs: N"
Error dialog | Disconnect server → Refresh | Error message shown

---

### Phase 5: Documentation & Deployment

**5.1 User Documentation**

**Location:** `docs/ADMIN_LOG_VIEWER.md`

```markdown
# Admin Log Viewer

## Overview
Monitor user activity and system events in real-time.

## Access
- Admin users only
- Admin Dashboard → View Logs button
- Or: File → View Activity Logs menu

## Features
- View all activity logs (logins, file operations, admin actions)
- Filter by user, action type, date range
- Sort by any column
- Export logs (future enhancement)

## Filters
- **User:** Select specific user or "All Users"
- **Action:** LOGIN, DOWNLOAD, UPLOAD, etc.
- **Date Range:** YYYY-MM-DD format (inclusive)
- **Limit:** Max results (100-10000)

## Log Columns
- **ID:** Unique log entry ID
- **Timestamp:** When action occurred
- **User:** Username who performed action
- **Action:** Type of activity
- **Description:** Details about the action

## Usage
1. Open Admin Dashboard
2. Click "View Logs" toolbar button
3. Apply filters (optional)
4. Click "Refresh" to update
5. Sort columns by clicking headers
```

**5.2 Developer Documentation**

**Location:** `docs/API_ADMIN_LOGS.md`

```markdown
# Admin Logs API

## Protocol

### CMD_ADMIN_VIEW_LOGS (0x54)

**Request:**
```json
{
  "user_id": 0,         // 0=all, >0=specific user
  "action_type": "",    // empty=all, or specific action
  "start_date": "",     // YYYY-MM-DD or empty
  "end_date": "",       // YYYY-MM-DD or empty
  "limit": 1000         // 100-10000
}
```

**Response (Success):**
```json
{
  "status": "OK",
  "logs": [...]
}
```

**Response (Error):**
```json
{
  "status": "ERROR",
  "message": "Admin access required"
}
```

## Client API

```c
cJSON* client_admin_view_logs(ClientConnection* conn,
                              int user_id_filter,
                              const char* action_type,
                              const char* start_date,
                              const char* end_date,
                              int limit);
```

Returns cJSON object (caller must free with `cJSON_Delete()`).
```

**5.3 Deployment Checklist**

- [ ] Compile server: `make server`
- [ ] Compile client: `make client_gui`
- [ ] Test admin login
- [ ] Test log viewer access (admin only)
- [ ] Test all filters
- [ ] Test with empty database
- [ ] Test with 10,000+ logs
- [ ] Verify no memory leaks (valgrind)
- [ ] Update CHANGELOG.md
- [ ] Tag release version

---

## File Change Summary

| File | Lines Changed | Type | Priority |
|------|---------------|------|----------|
| `src/common/protocol.h` | +1 | Add constant | P0 |
| `src/server/commands.h` | +1 | Add declaration | P0 |
| `src/server/commands.c` | +80 | Add handler | P0 |
| `src/client/client.h` | +3 | Add declaration | P1 |
| `src/client/client.c` | +70 | Add function | P1 |
| `src/client/gui/log_viewer.h` | +30 (new file) | New header | P2 |
| `src/client/gui/log_viewer.c` | +350 (new file) | New implementation | P2 |
| `src/client/gui/admin_dashboard.c` | +15 | Integration | P2 |
| `src/client/gui/Makefile` | +1 | Build config | P2 |
| `docs/ADMIN_LOG_VIEWER.md` | +50 (new file) | User docs | P3 |
| `docs/API_ADMIN_LOGS.md` | +40 (new file) | Dev docs | P3 |

**Total Estimated Lines:** ~650 new lines

---

## Risk Analysis

| Risk | Probability | Impact | Mitigation |
|------|------------|--------|------------|
| Non-admin access | Low | High | Server-side admin check enforced |
| SQL injection | Low | High | Prepared statements used |
| Large result sets freeze UI | Medium | Medium | Enforce limit (max 10000), add loading indicator |
| Memory leak (cJSON) | Medium | Low | Thorough cleanup, valgrind testing |
| Date format errors | Medium | Low | Input validation, placeholder hints |
| Network timeout | Low | Medium | Add timeout handling, retry logic |

---

## Future Enhancements

**Phase 6 (Optional):**
1. **Real-time Updates:** WebSocket-based live log streaming
2. **Export Functionality:** Download logs as CSV/JSON
3. **Advanced Filters:** Regex patterns, multiple users
4. **Log Retention:** Auto-archive old logs (>30 days)
5. **Analytics Dashboard:** Charts for login trends, file activity
6. **User-to-ID Mapping:** Fix user combo filter (currently only supports "All Users")
7. **Pagination:** Load logs in chunks (current: load all at once)

---

## Unresolved Questions

1. **User Filter Mapping:** How to map selected username to user_id in filter combo?
   - **Current:** Only supports "All Users" (user_id=0)
   - **Solution:** Store user_id in combo box item data, or query client-side map

2. **Date Validation:** Should client validate date format before sending?
   - **Current:** No validation, relies on server/database
   - **Recommendation:** Add GtkCalendar widget for date selection

3. **Auto-refresh:** Should logs auto-refresh every N seconds?
   - **Current:** Manual refresh only
   - **Trade-off:** Polling increases server load vs. real-time updates

4. **Column Widths:** Fixed or user-adjustable?
   - **Current:** Fixed widths in code
   - **Enhancement:** Save column preferences to config file

---

## Timeline Estimate

| Phase | Effort | Duration |
|-------|--------|----------|
| Phase 1: Backend | 2 hours | Day 1 AM |
| Phase 2: Client API | 1 hour | Day 1 PM |
| Phase 3: GUI | 4 hours | Day 2 |
| Phase 4: Testing | 2 hours | Day 3 AM |
| Phase 5: Documentation | 1 hour | Day 3 PM |
| **Total** | **10 hours** | **3 days** |

---

## Success Criteria

- [x] Design complete and reviewed
- [ ] Backend command handler passes unit tests
- [ ] Client API successfully retrieves logs
- [ ] GUI displays logs without crashes
- [ ] All filters work correctly
- [ ] Non-admin users blocked from access
- [ ] No memory leaks detected
- [ ] Documentation complete
- [ ] Code reviewed and merged

---

## Conclusion

Implementation plan complete. All components designed with YAGNI principle—no over-engineering, only necessary features. Database function already exists, minimal backend work needed. GUI follows existing admin dashboard pattern for consistency. Security enforced at server level. Ready for implementation.

**Next Step:** Begin Phase 1 (Backend implementation) or request review/adjustments.
