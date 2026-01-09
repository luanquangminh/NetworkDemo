# View Logs Feature - Implementation Plan

**Feature:** Admin GUI Activity Logs Viewer
**Date:** 2026-01-09
**Status:** Planning Complete

---

## Executive Summary

Add activity log viewing capability to admin dashboard for monitoring user actions. Database already tracks logs in `activity_logs` table. Need server API endpoint + GTK3 GUI viewer with filtering.

---

## 1. Database Schema Analysis

### Existing Schema (db_init.sql)
```sql
CREATE TABLE IF NOT EXISTS activity_logs (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    action_type TEXT NOT NULL,
    description TEXT,
    timestamp TEXT DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(id)
);
CREATE INDEX IF NOT EXISTS idx_logs_user ON activity_logs(user_id);
```

**Current logging locations:**
- `handle_login()`: "LOGIN" action
- `handle_list_dir()`: "ACCESS_DENIED" on permission fail
- `handle_admin_list_users()`: "ADMIN_LIST_USERS"
- Additional locations in commands.c

**Schema observations:**
- `user_id`: Foreign key to users table (for JOIN operations)
- `action_type`: Categorized action (LOGIN, ADMIN_LIST_USERS, ACCESS_DENIED, etc.)
- `description`: Free-form detail text
- `timestamp`: Auto-generated SQL timestamp
- Index on `user_id` exists, may need additional index on `timestamp` for date filtering

**Required database functions:** Add to `db_manager.c`
```c
int db_list_activity_logs(Database* db, int user_id_filter,
                          const char* action_type_filter,
                          const char* start_date, const char* end_date,
                          int limit, char** json_result);
```

**Query pattern:**
```sql
SELECT al.id, al.user_id, u.username, al.action_type,
       al.description, al.timestamp
FROM activity_logs al
LEFT JOIN users u ON al.user_id = u.id
WHERE (? = 0 OR al.user_id = ?)
  AND (? IS NULL OR al.action_type LIKE ?)
  AND (? IS NULL OR al.timestamp >= ?)
  AND (? IS NULL OR al.timestamp <= ?)
ORDER BY al.timestamp DESC
LIMIT ?
```

---

## 2. Server-Side API Design

### New Protocol Command
**Add to `protocol.h`:**
```c
#define CMD_ADMIN_LIST_LOGS    0x54
```

### Server Handler
**Add to `commands.h`:**
```c
void handle_admin_list_logs(ClientSession* session, Packet* pkt);
```

**Add to `commands.c`:**
```c
void handle_admin_list_logs(ClientSession* session, Packet* pkt) {
    // 1. Check admin authorization (same pattern as handle_admin_list_users)
    if (!db_is_admin(global_db, session->user_id)) {
        send_error(session, "Admin access required");
        return;
    }

    // 2. Parse JSON filters: user_id, action_type, start_date, end_date, limit
    cJSON* json = cJSON_Parse(pkt->payload);
    int user_id_filter = 0;  // 0 = all users
    const char* action_type = NULL;
    const char* start_date = NULL;
    const char* end_date = NULL;
    int limit = 100;  // default

    // Extract filters from JSON...

    // 3. Call db_list_activity_logs()
    char* json_result = NULL;
    if (db_list_activity_logs(global_db, user_id_filter, action_type,
                              start_date, end_date, limit, &json_result) < 0) {
        send_error(session, "Failed to retrieve logs");
        cJSON_Delete(json);
        return;
    }

    // 4. Build response
    cJSON* response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "status", "OK");
    cJSON* logs_array = cJSON_Parse(json_result);
    cJSON_AddItemToObject(response, "logs", logs_array);

    char* payload = cJSON_PrintUnformatted(response);
    send_success(session, CMD_SUCCESS, payload);

    log_info("Admin user %d viewed activity logs", session->user_id);
    db_log_activity(global_db, session->user_id, "ADMIN_VIEW_LOGS", "Viewed activity logs");

    free(json_result);
    free(payload);
    cJSON_Delete(response);
    cJSON_Delete(json);
}
```

**Update `dispatch_command()` in commands.c:**
```c
case CMD_ADMIN_LIST_LOGS:
    handle_admin_list_logs(session, pkt);
    break;
```

---

## 3. Client-Side API

**Add to `client.h`:**
```c
// Admin operations - logs
void* client_admin_list_logs(ClientConnection* conn, int user_id_filter,
                             const char* action_type, const char* start_date,
                             const char* end_date, int limit);
// Returns cJSON* with log list
```

**Add to `client.c`:**
```c
void* client_admin_list_logs(ClientConnection* conn, int user_id_filter,
                             const char* action_type, const char* start_date,
                             const char* end_date, int limit) {
    // 1. Build JSON request with filters
    cJSON* request = cJSON_CreateObject();
    cJSON_AddNumberToObject(request, "user_id", user_id_filter);
    if (action_type) cJSON_AddStringToObject(request, "action_type", action_type);
    if (start_date) cJSON_AddStringToObject(request, "start_date", start_date);
    if (end_date) cJSON_AddStringToObject(request, "end_date", end_date);
    cJSON_AddNumberToObject(request, "limit", limit);

    char* payload = cJSON_PrintUnformatted(request);

    // 2. Send CMD_ADMIN_LIST_LOGS packet
    Packet* pkt = packet_create(CMD_ADMIN_LIST_LOGS, payload, strlen(payload));
    packet_send(conn->socket_fd, pkt);

    // 3. Receive response
    Packet response;
    packet_recv(conn->socket_fd, &response);

    cJSON* json = cJSON_Parse(response.payload);

    // 4. Return cJSON* (caller must free)
    free(payload);
    packet_free(pkt);
    packet_free(&response);
    cJSON_Delete(request);

    return json;  // Returns cJSON* or NULL on error
}
```

---

## 4. GTK3 GUI Design

### UI Layout (New Window/Tab)

**Option A: Separate Window (Recommended)**
- Keep admin_dashboard.c focused on user management
- Create new `logs_viewer.c` + `logs_viewer.h`
- Launch from admin dashboard toolbar/menu

**Option B: Notebook Tabs**
- Add GtkNotebook to admin_dashboard.c
- Tab 1: User Management (existing)
- Tab 2: Activity Logs (new)

**Recommendation: Option A** - cleaner separation, reusable component

### Logs Viewer Window Components

```
+--------------------------------------------------+
| Logs Viewer                                    [X]|
+--------------------------------------------------+
| File | [Refresh] [Export]                         |
+--------------------------------------------------+
| Filters:                                          |
|   User: [Dropdown: All Users / alice / bob]      |
|   Action: [Dropdown: All / LOGIN / UPLOAD / ...] |
|   Date From: [YYYY-MM-DD]  To: [YYYY-MM-DD]      |
|   [Apply Filters]                                 |
+--------------------------------------------------+
|  ID | User    | Action Type | Description | Time |
| ================================================  |
| 150 | alice   | LOGIN       | User log... | ...  |
| 149 | bob     | UPLOAD      | Uploaded... | ...  |
| 148 | admin   | ADMIN_VI... | Viewed u... | ...  |
|                                                   |
+--------------------------------------------------+
| Total logs: 148 | Showing: 100             Ready |
+--------------------------------------------------+
```

### GTK3 Widget Hierarchy

```c
typedef struct {
    GtkWidget *window;
    GtkWidget *tree_view;
    GtkListStore *log_store;
    GtkWidget *status_bar;

    // Filter widgets
    GtkWidget *user_combo;
    GtkWidget *action_combo;
    GtkWidget *date_from_entry;
    GtkWidget *date_to_entry;

    ClientConnection *conn;
} LogsViewerState;
```

**Column definitions:**
```c
enum {
    COL_LOG_ID = 0,
    COL_USER_ID,
    COL_USERNAME,
    COL_ACTION_TYPE,
    COL_DESCRIPTION,
    COL_TIMESTAMP,
    LOG_NUM_COLS
};
```

---

## 5. Implementation Tasks (TODO)

### Phase 1: Database Layer
- [ ] **Task 1.1**: Add `db_list_activity_logs()` function to `db_manager.c`
  - Implement parameterized query with filters
  - Handle NULL/optional filters properly
  - Return JSON array of log entries with username JOIN
  - Add error handling for invalid dates

- [ ] **Task 1.2**: Add function declaration to `db_manager.h`

- [ ] **Task 1.3**: Consider adding timestamp index for performance
  ```sql
  CREATE INDEX IF NOT EXISTS idx_logs_timestamp ON activity_logs(timestamp);
  ```

### Phase 2: Server API
- [ ] **Task 2.1**: Add `CMD_ADMIN_LIST_LOGS` to `protocol.h`

- [ ] **Task 2.2**: Implement `handle_admin_list_logs()` in `commands.c`
  - Admin authorization check
  - Parse JSON filters from client
  - Call db_list_activity_logs()
  - Build and send response
  - Log admin action

- [ ] **Task 2.3**: Add handler to `dispatch_command()` switch

- [ ] **Task 2.4**: Add `handle_admin_list_logs()` declaration to `commands.h`

### Phase 3: Client API
- [ ] **Task 3.1**: Add `client_admin_list_logs()` to `client.c`
  - Build request JSON with filters
  - Send CMD_ADMIN_LIST_LOGS packet
  - Receive and parse response
  - Return cJSON* to caller

- [ ] **Task 3.2**: Add function declaration to `client.h`

### Phase 4: GUI Implementation
- [ ] **Task 4.1**: Create `logs_viewer.h`
  ```c
  #ifndef LOGS_VIEWER_H
  #define LOGS_VIEWER_H

  #include <gtk/gtk.h>
  #include "../client.h"

  typedef struct {
      GtkWidget *window;
      GtkWidget *tree_view;
      GtkListStore *log_store;
      GtkWidget *status_bar;
      GtkWidget *user_combo;
      GtkWidget *action_combo;
      GtkWidget *date_from_entry;
      GtkWidget *date_to_entry;
      ClientConnection *conn;
  } LogsViewerState;

  GtkWidget* create_logs_viewer(ClientConnection *conn);
  void refresh_logs(LogsViewerState *state);

  #endif
  ```

- [ ] **Task 4.2**: Create `logs_viewer.c` with main functions
  - `create_logs_viewer()`: Build window layout
  - `refresh_logs()`: Fetch and display logs
  - `on_apply_filters_clicked()`: Apply filters and refresh
  - `on_refresh_clicked()`: Clear filters and refresh
  - `populate_user_combo()`: Load user list for filter
  - `populate_action_combo()`: Load action types

- [ ] **Task 4.3**: Build GTK3 UI components
  - Create main window (800x600)
  - Add toolbar with Refresh button
  - Add filter panel with:
    - User dropdown (GtkComboBoxText)
    - Action type dropdown (GtkComboBoxText)
    - Date range entries (GtkEntry with placeholders)
    - "Apply Filters" button
  - Add scrolled window with tree view
  - Configure columns: ID, User, Action, Description, Timestamp
  - Add status bar

- [ ] **Task 4.4**: Implement `refresh_logs()` logic
  - Clear existing log_store
  - Get filter values from widgets
  - Call `client_admin_list_logs()` with filters
  - Parse cJSON response
  - Populate GtkListStore with log entries
  - Update status bar with count

- [ ] **Task 4.5**: Add callback for filter application
  ```c
  static void on_apply_filters_clicked(GtkWidget *widget, LogsViewerState *state) {
      refresh_logs(state);
  }
  ```

- [ ] **Task 4.6**: Add "View Logs" button/menu to admin_dashboard.c
  ```c
  GtkToolItem *logs_btn = gtk_tool_button_new(NULL, "View Logs");
  gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(logs_btn), "view-list");
  g_signal_connect(logs_btn, "clicked", G_CALLBACK(on_view_logs_clicked), state);
  ```

- [ ] **Task 4.7**: Implement launcher callback
  ```c
  static void on_view_logs_clicked(GtkWidget *widget, AdminState *state) {
      create_logs_viewer(state->conn);
  }
  ```

### Phase 5: Build System
- [ ] **Task 5.1**: Update `src/client/gui/Makefile` to include logs_viewer.o

- [ ] **Task 5.2**: Update root Makefile if needed

- [ ] **Task 5.3**: Compile and test build

### Phase 6: Testing
- [ ] **Task 6.1**: Unit test `db_list_activity_logs()`
  - Test with various filter combinations
  - Test date range filtering
  - Test limit parameter
  - Test with empty database

- [ ] **Task 6.2**: Test server endpoint
  - Test admin authorization check
  - Test non-admin rejection
  - Test filter parameter parsing
  - Test response format

- [ ] **Task 6.3**: Test client API
  - Test with valid filters
  - Test with NULL filters (all logs)
  - Test error handling

- [ ] **Task 6.4**: GUI integration testing
  - Test window launch from admin dashboard
  - Test filter application
  - Test log display
  - Test refresh functionality
  - Test with large log datasets (pagination?)

- [ ] **Task 6.5**: End-to-end testing
  - Perform various actions (login, upload, admin ops)
  - Verify logs appear in viewer
  - Test filtering by user
  - Test filtering by action type
  - Test date range filtering

---

## 6. Technical Considerations

### Pagination Strategy
**Current:** Single query with LIMIT parameter
**Risk:** Large datasets (>1000 logs) may cause performance issues
**Recommendation:**
- Start with LIMIT=100
- Add "Load More" button if needed
- Or implement offset-based pagination later

### Date Format
**Database:** SQLite TEXT with ISO8601 format (YYYY-MM-DD HH:MM:SS)
**GUI Input:** GtkEntry with placeholder "YYYY-MM-DD"
**Validation:** Add client-side regex validation before sending to server

### Action Type Filtering
**Options:**
1. **Static dropdown** - Hardcoded list (LOGIN, UPLOAD, DOWNLOAD, etc.)
2. **Dynamic dropdown** - Query `SELECT DISTINCT action_type FROM activity_logs`

**Recommendation:** Start with static dropdown (simpler, faster)

### User Filter
**Implementation:**
- Query users table via existing `client_admin_list_users()`
- Populate dropdown on window init
- Include "All Users" option (user_id=0)

### Memory Management
**Critical:** cJSON* returned by `client_admin_list_logs()` must be freed by caller
```c
cJSON* logs = client_admin_list_logs(conn, 0, NULL, NULL, NULL, 100);
// ... use logs ...
cJSON_Delete(logs);  // DON'T FORGET
```

### Error Handling
- Network timeout: Show error dialog
- Server error: Parse error message from response
- Invalid filters: Client-side validation before sending
- Empty results: Show "No logs found" message

---

## 7. File Changes Summary

### New Files
- `src/client/gui/logs_viewer.h` (GUI header)
- `src/client/gui/logs_viewer.c` (GUI implementation)

### Modified Files
- `src/database/db_manager.h` (add db_list_activity_logs declaration)
- `src/database/db_manager.c` (add db_list_activity_logs implementation)
- `src/common/protocol.h` (add CMD_ADMIN_LIST_LOGS)
- `src/server/commands.h` (add handle_admin_list_logs declaration)
- `src/server/commands.c` (add handler + dispatch case)
- `src/client/client.h` (add client_admin_list_logs declaration)
- `src/client/client.c` (add client_admin_list_logs implementation)
- `src/client/gui/admin_dashboard.c` (add "View Logs" toolbar button)
- `src/client/gui/Makefile` (add logs_viewer.o to build)

---

## 8. Code Patterns Reference

### Database Query Pattern
Follow `db_list_users()` in db_manager.c:
- Use prepared statements with sqlite3_prepare_v2
- Build JSON manually or use cJSON
- Lock mutex for thread safety
- Return JSON string via char** out parameter

### Server Handler Pattern
Follow `handle_admin_list_users()` in commands.c:
- Check `db_is_admin()` first
- Parse request JSON with cJSON
- Call database function
- Build response with status + data
- Log admin action via `db_log_activity()`

### Client API Pattern
Follow `client_admin_list_users()` in client.c:
- Build request JSON
- Create packet with `packet_create()`
- Send via `packet_send()`
- Receive via `packet_recv()`
- Parse response JSON
- Return cJSON* to caller

### GUI Pattern
Follow admin_dashboard.c structure:
- Use state struct for window/widgets
- Use GtkListStore for table data
- Use `gtk_tree_view_new_with_model()` for display
- Connect signals with `g_signal_connect()`
- Use `show_error_dialog()` from dialogs.c for errors

---

## 9. Estimated Effort

**Total:** ~8-12 hours

| Phase | Effort | Priority |
|-------|--------|----------|
| 1. Database Layer | 2h | High |
| 2. Server API | 1.5h | High |
| 3. Client API | 1h | High |
| 4. GUI Implementation | 3-4h | High |
| 5. Build System | 0.5h | Medium |
| 6. Testing | 2-3h | High |

---

## 10. Future Enhancements

### Not in Initial Scope
- **Export to CSV**: Add export button to save logs
- **Real-time updates**: WebSocket/polling for live log feed
- **Advanced search**: Full-text search in description field
- **Log retention policy**: Auto-delete old logs after N days
- **Pagination UI**: Next/Previous buttons for large datasets
- **Column sorting**: Click column headers to sort
- **Log details dialog**: Double-click row for full details

---

## 11. Security Considerations

- **Authorization**: MUST check `db_is_admin()` on server side
- **SQL Injection**: Use parameterized queries (prepared statements)
- **Log privacy**: Consider redacting sensitive info in descriptions
- **Rate limiting**: Add throttling for log queries (future)

---

## 12. Unresolved Questions

1. **Pagination strategy**: Should we implement offset-based pagination from start or add later if needed?
2. **Log retention**: Is there a policy for deleting old logs? Should we add one?
3. **Action type enum**: Should we define a formal enum for action_type or keep it free-form strings?
4. **Export format**: If we add export, should it be CSV, JSON, or both?
5. **Real-time requirement**: Do admins need live log updates or is manual refresh sufficient?

---

## 13. Dependencies

**External:**
- GTK3 (already used)
- SQLite3 (already used)
- cJSON (already used)

**Internal:**
- Existing admin authentication system
- Database activity logging infrastructure
- Client-server protocol framework

**No new dependencies required.**

---

## Approval & Sign-off

**Ready for implementation:** Yes
**Blockers:** None
**Next step:** Begin Phase 1 (Database Layer)

---

*Plan created: 2026-01-09*
*Last updated: 2026-01-09*
