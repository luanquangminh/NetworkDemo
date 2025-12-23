# Admin Dashboard Implementation Plan

## Executive Summary

This plan outlines implementation of admin dashboard for GTK GUI client with user management capabilities (create, delete, change permissions). Implementation follows YAGNI, KISS, DRY principles with security-first approach.

---

## 1. Current System Analysis

### 1.1 Authentication System
- **Location**: `src/server/commands.c::handle_login()`
- **Flow**: Username/password → SHA256 hash → DB verification → user_id returned
- **Session**: Stored in `ClientSession` struct (thread_pool.h)
- **Database**: `users` table with columns: id, username, password_hash, is_active, created_at
- **Gap**: No role/admin flag in users table

### 1.2 Database Schema (db_init.sql)
```sql
users: id, username, password_hash, is_active, created_at
files: id, parent_id, name, physical_path, owner_id, size, is_directory, permissions, created_at
activity_logs: id, user_id, action_type, description, timestamp
```
**Missing**: User roles/admin flag, user management APIs

### 1.3 GUI Architecture
- **Framework**: GTK 3
- **Entry Point**: `src/client/gui/main.c`
- **Login**: `src/client/gui/login_dialog.c::create_login_dialog()` - 4 fields (server, port, username, password)
- **Main Window**: `src/client/gui/main_window.c::create_main_window()` - File browser with toolbar
- **App State**: `AppState` struct in gui.h (window, tree_view, file_store, status_bar, conn, current_directory, current_path)
- **Pattern**: Dialog → Login → Main Window

### 1.4 Backend API (commands.h/c)
**Existing Commands**:
- CMD_LOGIN_REQ/RES (0x01/0x02)
- CMD_LIST_DIR, CMD_CHANGE_DIR, CMD_MAKE_DIR
- CMD_UPLOAD_REQ/DATA, CMD_DOWNLOAD_REQ/RES
- CMD_DELETE, CMD_CHMOD, CMD_FILE_INFO

**Available DB Functions** (db_manager.h):
- `db_create_user()` - CREATE
- `db_verify_user()` - Already used for login
- `db_get_user_by_id()` - READ
- `db_user_exists()` - CHECK

**Missing APIs**:
- List all users
- Delete user
- Update user role/permissions
- Get user role

---

## 2. Architecture Design

### 2.1 Database Schema Changes

**File**: `src/database/db_init.sql`

Add `is_admin` column to users table:
```sql
ALTER TABLE users ADD COLUMN is_admin INTEGER DEFAULT 0;
UPDATE users SET is_admin = 1 WHERE id = 1; -- Make default admin user an admin
```

**Migration approach**: Create `db_migration_v2.sql` with ALTER statements, execute on startup if version mismatch.

### 2.2 Protocol Extensions

**New Commands** (protocol.h):
```c
#define CMD_ADMIN_LIST_USERS   0x50
#define CMD_ADMIN_CREATE_USER  0x51
#define CMD_ADMIN_DELETE_USER  0x52
#define CMD_ADMIN_UPDATE_USER  0x53
```

**Request/Response Format** (JSON):
```json
// CMD_ADMIN_LIST_USERS Request
{}

// Response
{
  "status": "OK",
  "users": [
    {"id": 1, "username": "admin", "is_active": 1, "is_admin": 1, "created_at": "..."},
    {"id": 2, "username": "user1", "is_active": 1, "is_admin": 0, "created_at": "..."}
  ]
}

// CMD_ADMIN_CREATE_USER Request
{"username": "newuser", "password": "pass123", "is_admin": 0}

// Response
{"status": "OK", "user_id": 3, "username": "newuser"}

// CMD_ADMIN_DELETE_USER Request
{"user_id": 2}

// Response
{"status": "OK", "message": "User deleted"}

// CMD_ADMIN_UPDATE_USER Request
{"user_id": 2, "is_admin": 1, "is_active": 0}

// Response
{"status": "OK", "message": "User updated"}
```

### 2.3 GUI Design

**Admin Dashboard Layout**:
```
┌─────────────────────────────────────────┐
│  Admin Dashboard - File Sharing Server  │
├─────────────────────────────────────────┤
│  [Create User] [Delete User] [Refresh]  │ ← Toolbar
├─────────────────────────────────────────┤
│  ID │ Username │ Active │ Admin │ Date  │ ← TreeView (GtkListStore)
│  1  │ admin    │   ✓    │   ✓   │ ...   │
│  2  │ alice    │   ✓    │       │ ...   │
│  3  │ bob      │        │       │ ...   │
├─────────────────────────────────────────┤
│  Connected as admin                      │ ← Status bar
└─────────────────────────────────────────┘
```

**Files to Create**:
1. `src/client/gui/admin_dashboard.c` - Main dashboard window
2. `src/client/gui/admin_dashboard.h` - Header with AdminState struct
3. `src/client/gui/admin_dialogs.c` - Create/Edit user dialogs

**Files to Modify**:
1. `src/client/gui/login_dialog.c` - Add admin login detection (check is_admin from login response)
2. `src/client/gui/gui.h` - Add admin dashboard function declarations
3. `src/client/gui/main.c` - Route to admin dashboard if admin user
4. `src/client/gui/Makefile` - Add new source files

---

## 3. Implementation Steps

### Phase 1: Database & Backend (Server-side)

#### Step 1.1: Database Schema Migration
**File**: `src/database/db_migration_v2.sql` (NEW)
```sql
-- Add admin flag to users table
ALTER TABLE users ADD COLUMN is_admin INTEGER DEFAULT 0;

-- Update existing admin user
UPDATE users SET is_admin = 1 WHERE username = 'admin';
```

**File**: `src/database/db_manager.h`
**Action**: Add new function declarations:
```c
// User management functions
int db_list_users(Database* db, UserEntry** users, int* count);
int db_delete_user(Database* db, int user_id);
int db_update_user(Database* db, int user_id, int is_admin, int is_active);
int db_get_user_role(Database* db, int user_id, int* is_admin);
```

**File**: `src/database/db_manager.c`
**Action**: Implement the above functions (~100 lines, similar to db_list_directory pattern)

#### Step 1.2: Protocol Extension
**File**: `src/common/protocol.h`
**Action**: Add command constants (4 lines):
```c
#define CMD_ADMIN_LIST_USERS   0x50
#define CMD_ADMIN_CREATE_USER  0x51
#define CMD_ADMIN_DELETE_USER  0x52
#define CMD_ADMIN_UPDATE_USER  0x53
```

#### Step 1.3: Server Command Handlers
**File**: `src/server/commands.h`
**Action**: Add function declarations:
```c
void handle_admin_list_users(ClientSession* session, Packet* pkt);
void handle_admin_create_user(ClientSession* session, Packet* pkt);
void handle_admin_delete_user(ClientSession* session, Packet* pkt);
void handle_admin_update_user(ClientSession* session, Packet* pkt);
```

**File**: `src/server/commands.c`
**Action**:
1. Modify `handle_login()` to return is_admin flag in response JSON
2. Implement 4 admin command handlers (~200 lines total)
3. Add admin authorization check helper:
```c
static int is_admin_user(ClientSession* session) {
    int is_admin = 0;
    db_get_user_role(global_db, session->user_id, &is_admin);
    return is_admin;
}
```
4. Update `dispatch_command()` switch statement with new cases

#### Step 1.4: Client API Functions
**File**: `src/client/client.h`
**Action**: Add admin function declarations:
```c
void* client_admin_list_users(ClientConnection* conn);  // Returns cJSON*
int client_admin_create_user(ClientConnection* conn, const char* username, const char* password, int is_admin);
int client_admin_delete_user(ClientConnection* conn, int user_id);
int client_admin_update_user(ClientConnection* conn, int user_id, int is_admin, int is_active);
```

**File**: `src/client/client.c`
**Action**: Implement 4 admin client functions (~150 lines, follow pattern from client_list_dir_gui)

---

### Phase 2: GUI Implementation (Client-side)

#### Step 2.1: Admin State Structure
**File**: `src/client/gui/admin_dashboard.h` (NEW)
```c
#ifndef ADMIN_DASHBOARD_H
#define ADMIN_DASHBOARD_H

#include <gtk/gtk.h>
#include "../client.h"

typedef struct {
    GtkWidget *window;
    GtkWidget *tree_view;
    GtkListStore *user_store;
    GtkWidget *status_bar;
    ClientConnection *conn;
} AdminState;

GtkWidget* create_admin_dashboard(ClientConnection *conn);
void refresh_user_list(AdminState *state);

#endif
```

#### Step 2.2: Admin Dashboard Window
**File**: `src/client/gui/admin_dashboard.c` (NEW, ~300 lines)

**Structure**:
```c
#include "admin_dashboard.h"
#include "dialogs.h"

// Callbacks
static void on_create_user_clicked(GtkWidget *widget, AdminState *state);
static void on_delete_user_clicked(GtkWidget *widget, AdminState *state);
static void on_refresh_clicked(GtkWidget *widget, AdminState *state);
static void on_user_row_activated(GtkTreeView *tree_view, GtkTreePath *path,
                                   GtkTreeViewColumn *column, AdminState *state);

// Main window creation
GtkWidget* create_admin_dashboard(ClientConnection *conn) {
    // Allocate AdminState
    // Create window (800x600)
    // Create toolbar with buttons
    // Create user list TreeView with columns: ID, Username, Active (checkbox), Admin (checkbox), Created
    // Create status bar
    // Connect signals
    // Initial refresh
    // Return window
}

void refresh_user_list(AdminState *state) {
    // Call client_admin_list_users()
    // Parse cJSON response
    // Clear list store
    // Populate list store with users
    // Update status bar
}
```

#### Step 2.3: Admin Dialogs
**File**: `src/client/gui/admin_dialogs.c` (NEW, ~200 lines)

**Functions**:
```c
GtkWidget* create_user_dialog(GtkWidget *parent);
GtkWidget* create_edit_user_dialog(GtkWidget *parent, int user_id, const char *username,
                                     int is_admin, int is_active);
```

**Dialog Layout** (Create User):
```
┌────────────────────────────┐
│  Create New User           │
├────────────────────────────┤
│  Username: [_________]     │
│  Password: [_________]     │
│  [x] Admin User            │
│                            │
│     [Cancel]  [Create]     │
└────────────────────────────┘
```

#### Step 2.4: Login Flow Modification
**File**: `src/client/gui/login_dialog.c`
**Action**: No changes needed to dialog creation

**File**: `src/client/gui/main.c`
**Action**: Modify `main()` function:
```c
// After successful login
cJSON* response = ...; // Parse login response
int is_admin = cJSON_GetObjectItem(response, "is_admin")->valueint;

if (is_admin) {
    // Show admin dashboard
    GtkWidget *admin_window = create_admin_dashboard(conn);
    gtk_widget_show_all(admin_window);
} else {
    // Show regular file browser
    AppState *state = ...;
    GtkWidget *window = create_main_window(state);
    gtk_widget_show_all(window);
}
```

#### Step 2.5: Header Updates
**File**: `src/client/gui/gui.h`
**Action**: Add include and declarations:
```c
#include "admin_dashboard.h"

// In existing file, add:
GtkWidget* create_user_dialog(GtkWidget *parent);
GtkWidget* create_edit_user_dialog(GtkWidget *parent, int user_id, const char *username,
                                     int is_admin, int is_active);
```

#### Step 2.6: Build Configuration
**File**: `src/client/gui/Makefile`
**Action**: Add to SOURCES:
```makefile
SOURCES = main.c login_dialog.c main_window.c file_operations.c dialogs.c \
          admin_dashboard.c admin_dialogs.c
```

---

### Phase 3: Security Hardening

#### Step 3.1: Authorization Checks
**File**: `src/server/commands.c`
**Implementation**:
```c
// Add to all handle_admin_* functions:
if (!is_admin_user(session)) {
    send_error(session, "Admin privileges required");
    db_log_activity(global_db, session->user_id, "ACCESS_DENIED", "ADMIN_ACTION");
    return;
}
```

#### Step 3.2: Input Validation
**Locations**: All admin command handlers
**Checks**:
- Username: 3-32 chars, alphanumeric + underscore
- Password: 6+ chars
- User ID: Positive integer, exists in DB
- Prevent self-deletion (admin can't delete own account)
- Prevent deleting user ID 1 (primary admin)

#### Step 3.3: Activity Logging
**File**: `src/server/commands.c`
**Action**: Add logging to all admin operations:
```c
db_log_activity(global_db, session->user_id, "CREATE_USER", new_username);
db_log_activity(global_db, session->user_id, "DELETE_USER", deleted_username);
db_log_activity(global_db, session->user_id, "UPDATE_USER", updated_username);
```

---

## 4. File Modification Summary

### New Files (7)
1. `src/database/db_migration_v2.sql` - Schema migration
2. `src/client/gui/admin_dashboard.h` - Admin dashboard header
3. `src/client/gui/admin_dashboard.c` - Admin dashboard implementation
4. `src/client/gui/admin_dialogs.c` - User management dialogs

### Modified Files (10)
1. `src/common/protocol.h` - Add admin command constants
2. `src/database/db_manager.h` - Add admin function declarations
3. `src/database/db_manager.c` - Implement admin functions
4. `src/server/commands.h` - Add admin handler declarations
5. `src/server/commands.c` - Implement admin handlers + modify login handler
6. `src/client/client.h` - Add admin API declarations
7. `src/client/client.c` - Implement admin API functions
8. `src/client/gui/gui.h` - Add admin includes/declarations
9. `src/client/gui/main.c` - Add admin dashboard routing
10. `src/client/gui/Makefile` - Add new source files

---

## 5. Testing Strategy

### 5.1 Unit Tests
**File**: `tests/test_admin.c` (NEW)
**Tests**:
- DB schema migration verification
- db_list_users() returns correct data
- db_create_user() with admin flag
- db_delete_user() prevents deleting self
- db_update_user() changes admin status
- Authorization check rejects non-admin

### 5.2 Integration Tests
**Manual Test Script**: `tests/admin_test.sh` (NEW)
```bash
#!/bin/bash
# 1. Start server
# 2. Login as admin via GUI
# 3. Verify admin dashboard appears
# 4. Create new user (admin=false)
# 5. Create new admin user (admin=true)
# 6. Edit user (toggle admin, toggle active)
# 7. Delete user
# 8. Verify activity logs
# 9. Login as non-admin, verify no dashboard access
```

### 5.3 Security Tests
- Attempt admin commands as non-admin user (expect rejection)
- Attempt to delete user ID 1 (expect rejection)
- Attempt SQL injection in username field
- Attempt XSS in username field (GUI rendering)

---

## 6. Security Considerations

### 6.1 Authentication
- **Current**: SHA256 password hashing (adequate for project scope)
- **Improvement** (future): Use bcrypt/scrypt for password hashing

### 6.2 Authorization
- **Implement**: Role-Based Access Control (RBAC) via is_admin flag
- **Check**: All admin commands require is_admin=1
- **Audit**: Log all admin actions to activity_logs

### 6.3 Input Validation
- **Username**: Regex `^[a-zA-Z0-9_]{3,32}$`
- **Password**: Minimum 6 characters (enforced client + server)
- **User ID**: Must exist in database, cannot be negative

### 6.4 Session Management
- **Current**: ClientSession tracks user_id and authenticated state
- **Enhancement**: Add session_id for better tracking (future)

### 6.5 Privilege Escalation Prevention
- Non-admin users cannot access CMD_ADMIN_* commands
- Admin users cannot delete themselves (prevents lockout)
- User ID 1 (primary admin) cannot be deleted

### 6.6 Data Protection
- Passwords never stored in plaintext
- Password hashes transmitted over connection (consider TLS in production)
- Sensitive admin operations logged for audit trail

---

## 7. Implementation TODO Checklist

### Phase 1: Database & Backend (4-6 hours)
- [ ] Create `db_migration_v2.sql` with ALTER TABLE statements
- [ ] Add is_admin column to users table
- [ ] Implement `db_list_users()` in db_manager.c
- [ ] Implement `db_delete_user()` with safety checks
- [ ] Implement `db_update_user()` for role changes
- [ ] Implement `db_get_user_role()` helper
- [ ] Add admin command constants to protocol.h
- [ ] Implement `handle_admin_list_users()` in commands.c
- [ ] Implement `handle_admin_create_user()` with validation
- [ ] Implement `handle_admin_delete_user()` with safety checks
- [ ] Implement `handle_admin_update_user()` for role/status changes
- [ ] Modify `handle_login()` to return is_admin flag
- [ ] Add authorization helper `is_admin_user()`
- [ ] Update `dispatch_command()` switch with admin cases
- [ ] Add activity logging to all admin operations
- [ ] Implement client-side admin API in client.c

### Phase 2: GUI Implementation (6-8 hours)
- [ ] Create `admin_dashboard.h` with AdminState struct
- [ ] Create `admin_dashboard.c` with main window layout
- [ ] Implement `create_admin_dashboard()` function
- [ ] Implement `refresh_user_list()` to populate TreeView
- [ ] Create toolbar with Create/Delete/Refresh buttons
- [ ] Create user list TreeView with 5 columns
- [ ] Implement `on_create_user_clicked()` callback
- [ ] Implement `on_delete_user_clicked()` callback
- [ ] Implement `on_refresh_clicked()` callback
- [ ] Implement `on_user_row_activated()` for editing
- [ ] Create `admin_dialogs.c` for user management dialogs
- [ ] Implement `create_user_dialog()` with username/password/admin fields
- [ ] Implement `create_edit_user_dialog()` for updating users
- [ ] Modify `main.c` to route admins to dashboard after login
- [ ] Update `gui.h` with admin function declarations
- [ ] Update `gui/Makefile` to compile new files
- [ ] Add status bar updates for operation feedback
- [ ] Implement error handling and user feedback dialogs

### Phase 3: Testing & Security (3-4 hours)
- [ ] Write unit tests for DB functions
- [ ] Write integration test script
- [ ] Test admin login flow
- [ ] Test user creation (admin and non-admin)
- [ ] Test user deletion with safety checks
- [ ] Test user role updates
- [ ] Test authorization (non-admin rejection)
- [ ] Test input validation (SQL injection, XSS)
- [ ] Verify activity logging for all operations
- [ ] Test edge cases (delete self, delete user 1, etc.)
- [ ] Compile and verify build process
- [ ] Manual GUI testing on macOS

### Phase 4: Documentation (1-2 hours)
- [ ] Update README.md with admin dashboard usage
- [ ] Document admin API in docs/API.md
- [ ] Create admin user guide in docs/ADMIN_GUIDE.md
- [ ] Add security notes to docs/SECURITY.md
- [ ] Update database schema documentation

---

## 8. Risk Assessment

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| **Schema migration fails** | Low | High | Test migration on copy of DB first; provide rollback script |
| **Non-admin bypasses auth** | Medium | Critical | Implement authorization checks at server level, not client |
| **Admin deletes self** | Medium | High | Prevent self-deletion in server logic |
| **GUI crashes on user list** | Low | Medium | Validate JSON responses; handle NULL pointers |
| **Compilation errors (GTK)** | Medium | Low | Test incremental builds; ensure GTK3 installed |
| **Password stored plaintext** | Low | Critical | Already using SHA256; document limitation |

---

## 9. Future Enhancements (Out of Scope)

1. **Password Reset**: Forgot password mechanism
2. **User Groups**: RBAC with multiple roles (viewer, editor, admin)
3. **Audit Dashboard**: View activity logs in GUI
4. **Bulk Operations**: Import/export users via CSV
5. **Password Policies**: Enforce complexity, expiration
6. **2FA**: Two-factor authentication for admin users
7. **TLS Encryption**: Secure communication channel
8. **User Quotas**: Limit storage per user
9. **User Sessions**: Display active sessions, force logout
10. **Email Notifications**: Alert on admin actions

---

## 10. Estimated Effort

| Phase | Hours | Dependencies |
|-------|-------|--------------|
| Database & Backend | 4-6 | None |
| GUI Implementation | 6-8 | Phase 1 complete |
| Testing & Security | 3-4 | Phase 2 complete |
| Documentation | 1-2 | Phase 3 complete |
| **Total** | **14-20 hours** | Sequential execution |

---

## 11. Success Criteria

1. **Functional**:
   - Admin user can login and see dashboard instead of file browser
   - Admin can create new users (admin and non-admin)
   - Admin can delete users (with safety checks)
   - Admin can toggle admin/active status
   - Non-admin users cannot access admin features

2. **Security**:
   - All admin commands require is_admin=1
   - All admin operations logged to activity_logs
   - Input validation prevents SQL injection
   - Cannot delete user ID 1 or self

3. **Usability**:
   - Dashboard loads user list in <1 second
   - Clear feedback for all operations (success/error dialogs)
   - Intuitive button labels and layout
   - Consistent with existing GUI style

4. **Quality**:
   - No compilation warnings
   - No memory leaks (valgrind clean)
   - All unit tests pass
   - Manual integration tests pass

---

## 12. Open Questions

1. **Should regular file browser be accessible from admin dashboard?**
   - Recommendation: Add "File Browser" menu item to admin dashboard for convenience

2. **Should non-admin users ever become admin via GUI?**
   - Current plan: Yes, via "Edit User" dialog (admin can promote)

3. **Should user deletion cascade delete their files?**
   - Recommendation: Keep files but mark as orphaned (owner_id points to deleted user)
   - Alternative: Transfer ownership to admin before deletion

4. **Should we support password change in admin dashboard?**
   - Current plan: No (create new user instead)
   - Future enhancement: Add "Change Password" dialog

5. **Should active users (logged in) be prevented from deletion?**
   - Recommendation: Allow deletion but log warning in activity_logs
   - Enhancement: Track active sessions and warn admin

---

## Appendix A: Database Schema (After Migration)

```sql
CREATE TABLE users (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    username TEXT UNIQUE NOT NULL,
    password_hash TEXT NOT NULL,
    is_active INTEGER DEFAULT 1,
    is_admin INTEGER DEFAULT 0,  -- NEW COLUMN
    created_at TEXT DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE files (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    parent_id INTEGER DEFAULT 0,
    name TEXT NOT NULL,
    physical_path TEXT UNIQUE,
    owner_id INTEGER NOT NULL,
    size INTEGER DEFAULT 0,
    is_directory INTEGER DEFAULT 0,
    permissions INTEGER DEFAULT 755,
    created_at TEXT DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (owner_id) REFERENCES users(id),
    FOREIGN KEY (parent_id) REFERENCES files(id)
);

CREATE TABLE activity_logs (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    action_type TEXT NOT NULL,
    description TEXT,
    timestamp TEXT DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(id)
);
```

---

## Appendix B: Admin Command Flow Diagrams

### User Creation Flow
```
GUI: Create User Dialog
  → User enters: username, password, is_admin checkbox
  → Click "Create"
    → client_admin_create_user(conn, username, password, is_admin)
      → Packet: CMD_ADMIN_CREATE_USER + JSON payload
        → Server: handle_admin_create_user()
          → Check: session->user_id is admin
          → Validate: username format, password length
          → Hash password (SHA256)
          → db_create_user(username, hash)
          → db_log_activity("CREATE_USER", username)
          → Response: {"status": "OK", "user_id": X}
      → GUI: Show success dialog
      → refresh_user_list()
```

### User Deletion Flow
```
GUI: Select user row + Click "Delete"
  → Show confirmation dialog
  → If confirmed:
    → client_admin_delete_user(conn, user_id)
      → Packet: CMD_ADMIN_DELETE_USER + JSON payload
        → Server: handle_admin_delete_user()
          → Check: session->user_id is admin
          → Check: user_id != session->user_id (no self-delete)
          → Check: user_id != 1 (protect primary admin)
          → db_get_user_by_id(user_id) → get username for logging
          → db_delete_user(user_id)
          → db_log_activity("DELETE_USER", username)
          → Response: {"status": "OK"}
      → GUI: Show success dialog
      → refresh_user_list()
```

---

**Plan Version**: 1.0
**Created**: 2025-12-23
**Author**: Planning Agent
**Estimated Lines of Code**: ~1500 new + ~200 modified
**Target Completion**: 2-3 working days for solo developer
