# Back Button Feature - Implementation Plan

**Created:** 2026-01-07
**Feature:** Add "back to previous directory" button to GUI client
**Files Modified:** 3 files in src/client/gui/

---

## 1. Current System Analysis

### 1.1 Directory Navigation Architecture

**Data Flow:**
```
User Action (double-click)
  → on_row_activated()
  → client_cd(conn, file_id) [server call]
  → Update state->current_directory & current_path
  → refresh_file_list(state) [fetch & display files]
  → Update status bar
```

**Key Components:**

| Component | Location | Purpose |
|-----------|----------|---------|
| `AppState` | gui.h | Holds current_directory (int), current_path (char[512]) |
| `on_row_activated()` | file_operations.c | Handles directory double-click navigation |
| `client_cd()` | client.c | Server API call to change directory |
| `refresh_file_list()` | file_operations.c | Fetches and displays directory contents |

**Current State Storage:**
- `state->current_directory`: Directory ID (integer)
- `state->current_path`: Full path string (e.g., "/folder1/subfolder2")
- `state->conn->current_directory`: Mirror in connection struct
- `state->conn->current_path`: Mirror in connection struct

### 1.2 Navigation Pattern

```c
// Current navigation (file_operations.c:56-69)
if (strcmp(type, "Directory") == 0) {
    if (client_cd(state->conn, file_id) == 0) {
        state->current_directory = file_id;           // Update ID
        strcpy(state->current_path, state->conn->current_path);  // Update path
        refresh_file_list(state);                     // Refresh UI
        // Update status bar...
    }
}
```

**Missing:** No tracking of previous directory before navigation.

---

## 2. Design Decisions

### 2.1 History Strategy: Stack-Based Navigation

**Chosen Approach:** Stack-based history (not single "parent" navigation)

**Rationale:**
- More intuitive UX (like web browser back button)
- Supports multiple levels of back navigation
- Users can navigate: / → A → B → C, then back to B, A, /
- Aligns with user mental model of "undo navigation"

**Alternative Considered:** Parent directory navigation (cd ..)
- Rejected: Doesn't support non-linear navigation patterns
- Example: User searches, navigates to /deep/folder, wants to return to previous context

### 2.2 History Structure

```c
// Directory history entry
typedef struct {
    int directory_id;      // Server-side directory ID
    char path[512];        // Full path string for display
} DirectoryHistoryEntry;

// History stack in AppState
typedef struct {
    DirectoryHistoryEntry *entries;  // Dynamic array
    int count;                        // Current number of entries
    int capacity;                     // Allocated capacity
} DirectoryHistory;
```

**Stack Properties:**
- **Max Size:** 50 entries (prevents unbounded memory growth)
- **Overflow Behavior:** When full, remove oldest entry (shift array)
- **Storage:** Dynamic array (realloc on growth)

### 2.3 UI Design

**Button Placement:** Toolbar, leftmost position (before Upload button)

```
[< Back] [Upload] [Download] [New Folder] [Delete] [Permissions] | [Search...]
```

**Button Properties:**
- **Label:** "Back"
- **Icon:** "go-previous" (standard GTK icon)
- **Initial State:** Disabled (no history)
- **Enabled When:** history.count > 0

**Visual Feedback:**
- Disabled (grayed out) when at root or no history
- Enabled (clickable) when history available
- Tooltip: "Go back to previous directory"

---

## 3. Implementation Phases

### Phase 1: Data Structure Updates

**File:** `src/client/gui/gui.h`

**Tasks:**
1. Add DirectoryHistoryEntry struct definition
2. Add DirectoryHistory struct definition
3. Add history field to AppState struct
4. Declare history management functions

**Code Changes:**

```c
// After line 6, add history structures
typedef struct {
    int directory_id;
    char path[512];
} DirectoryHistoryEntry;

typedef struct {
    DirectoryHistoryEntry *entries;
    int count;
    int capacity;
} DirectoryHistory;

// In AppState struct (after line 18), add:
typedef struct {
    GtkWidget *window;
    GtkWidget *tree_view;
    GtkListStore *file_store;
    GtkWidget *status_bar;
    GtkWidget *search_entry;
    GtkWidget *search_recursive_check;
    GtkWidget *back_button;              // NEW: Back button widget
    DirectoryHistory history;            // NEW: Navigation history
    ClientConnection *conn;
    int current_directory;
    char current_path[512];
} AppState;

// After line 51, add function declarations:
void history_init(DirectoryHistory *history);
void history_free(DirectoryHistory *history);
void history_push(DirectoryHistory *history, int dir_id, const char *path);
int history_pop(DirectoryHistory *history, int *dir_id, char *path);
int history_is_empty(DirectoryHistory *history);
void history_clear(DirectoryHistory *history);
```

**Validation:**
- Compile check: `make clean && make client`
- No runtime changes yet (history not used)

---

### Phase 2: History Management Functions

**File:** `src/client/gui/file_operations.c`

**Tasks:**
1. Implement history_init(): Allocate initial history array
2. Implement history_push(): Add entry to stack
3. Implement history_pop(): Remove and return last entry
4. Implement history_is_empty(): Check if stack empty
5. Implement history_clear(): Reset stack
6. Implement history_free(): Cleanup memory

**Code Implementation:**

```c
// Add at end of file_operations.c (after line 255)

#define HISTORY_INITIAL_CAPACITY 10
#define HISTORY_MAX_CAPACITY 50

void history_init(DirectoryHistory *history) {
    history->entries = malloc(HISTORY_INITIAL_CAPACITY * sizeof(DirectoryHistoryEntry));
    history->count = 0;
    history->capacity = HISTORY_INITIAL_CAPACITY;
}

void history_free(DirectoryHistory *history) {
    if (history->entries) {
        free(history->entries);
        history->entries = NULL;
    }
    history->count = 0;
    history->capacity = 0;
}

void history_push(DirectoryHistory *history, int dir_id, const char *path) {
    // Check if at max capacity - remove oldest if needed
    if (history->count >= HISTORY_MAX_CAPACITY) {
        // Shift all entries down (remove oldest)
        memmove(&history->entries[0], &history->entries[1],
                (HISTORY_MAX_CAPACITY - 1) * sizeof(DirectoryHistoryEntry));
        history->count = HISTORY_MAX_CAPACITY - 1;
    }

    // Grow array if needed
    if (history->count >= history->capacity) {
        history->capacity *= 2;
        if (history->capacity > HISTORY_MAX_CAPACITY) {
            history->capacity = HISTORY_MAX_CAPACITY;
        }
        history->entries = realloc(history->entries,
                                   history->capacity * sizeof(DirectoryHistoryEntry));
    }

    // Add new entry
    history->entries[history->count].directory_id = dir_id;
    strncpy(history->entries[history->count].path, path, 511);
    history->entries[history->count].path[511] = '\0';
    history->count++;
}

int history_pop(DirectoryHistory *history, int *dir_id, char *path) {
    if (history->count == 0) {
        return -1;  // Empty stack
    }

    history->count--;
    *dir_id = history->entries[history->count].directory_id;
    strcpy(path, history->entries[history->count].path);

    return 0;  // Success
}

int history_is_empty(DirectoryHistory *history) {
    return history->count == 0;
}

void history_clear(DirectoryHistory *history) {
    history->count = 0;
}
```

**Edge Cases Handled:**
- Max capacity overflow: Removes oldest entry
- Empty pop: Returns -1
- NULL path: Truncates safely with strncpy
- Memory growth: Doubles capacity up to max

**Testing:**
```c
// Unit test (manual verification in main.c)
DirectoryHistory h;
history_init(&h);
assert(history_is_empty(&h));

history_push(&h, 1, "/folder1");
history_push(&h, 2, "/folder1/folder2");
assert(h.count == 2);

int id; char path[512];
history_pop(&h, &id, path);
assert(id == 2 && strcmp(path, "/folder1/folder2") == 0);

history_free(&h);
```

---

### Phase 3: Integrate History Tracking

**File:** `src/client/gui/file_operations.c`

**Tasks:**
1. Modify on_row_activated() to push current directory before navigation
2. Update back button sensitivity after navigation

**Code Changes:**

```c
// Modify on_row_activated() (lines 41-73)
void on_row_activated(GtkTreeView *tree_view, GtkTreePath *path,
                     GtkTreeViewColumn *column, AppState *state) {
    GtkTreeModel *model = gtk_tree_view_get_model(tree_view);
    GtkTreeIter iter;

    if (gtk_tree_model_get_iter(model, &iter, path)) {
        gint file_id;
        gchar *type;

        gtk_tree_model_get(model, &iter,
                          0, &file_id,
                          3, &type,
                          -1);

        // If it's a directory, navigate into it
        if (strcmp(type, "Directory") == 0) {
            // NEW: Save current directory to history BEFORE navigation
            history_push(&state->history, state->current_directory, state->current_path);

            if (client_cd(state->conn, file_id) == 0) {
                state->current_directory = file_id;
                strcpy(state->current_path, state->conn->current_path);
                refresh_file_list(state);

                // Update status bar
                guint context_id = gtk_statusbar_get_context_id(
                    GTK_STATUSBAR(state->status_bar), "status");
                char status[256];
                snprintf(status, sizeof(status), "Current: %s", state->current_path);
                gtk_statusbar_push(GTK_STATUSBAR(state->status_bar), context_id, status);

                // NEW: Enable back button
                gtk_widget_set_sensitive(state->back_button, TRUE);
            } else {
                // Navigation failed, remove history entry
                int dummy_id;
                char dummy_path[512];
                history_pop(&state->history, &dummy_id, dummy_path);
            }
        }

        g_free(type);
    }
}
```

**Critical Logic:**
1. Push BEFORE navigation (saves "where we came from")
2. If navigation fails, pop the entry (undo the push)
3. Enable back button after successful navigation

---

### Phase 4: Back Button UI

**File:** `src/client/gui/main_window.c`

**Task 1: Initialize History in create_main_window()**

```c
// In create_main_window(), after line 49, add:
g_signal_connect(window, "destroy", G_CALLBACK(on_main_window_destroy), state);

// NEW: Initialize navigation history
history_init(&state->history);
```

**Task 2: Add Back Button to Toolbar**

```c
// In create_main_window(), after line 83, BEFORE upload button:
GtkToolItem *back_btn = gtk_tool_button_new(NULL, "Back");
gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(back_btn), "go-previous");
gtk_widget_set_tooltip_text(GTK_WIDGET(back_btn), "Go back to previous directory");
g_signal_connect(back_btn, "clicked", G_CALLBACK(on_back_clicked), state);
gtk_toolbar_insert(GTK_TOOLBAR(toolbar), back_btn, -1);

// Store reference in state
state->back_button = GTK_WIDGET(back_btn);

// Set initial state to disabled (no history yet)
gtk_widget_set_sensitive(state->back_button, FALSE);

// Then existing buttons (upload, download, etc.)
GtkToolItem *upload_btn = gtk_tool_button_new(NULL, "Upload");
...
```

**Task 3: Add Back Button Callback**

```c
// Add new function in main_window.c (after on_logout_activate)
static void on_back_clicked(GtkWidget *widget, AppState *state) {
    (void)widget;  // Unused

    // Pop previous directory from history
    int prev_dir_id;
    char prev_path[512];

    if (history_pop(&state->history, &prev_dir_id, prev_path) == 0) {
        // Navigate back
        if (client_cd(state->conn, prev_dir_id) == 0) {
            state->current_directory = prev_dir_id;
            strcpy(state->current_path, state->conn->current_path);
            refresh_file_list(state);

            // Update status bar
            guint context_id = gtk_statusbar_get_context_id(
                GTK_STATUSBAR(state->status_bar), "status");
            char status[256];
            snprintf(status, sizeof(status), "Current: %s", state->current_path);
            gtk_statusbar_push(GTK_STATUSBAR(state->status_bar), context_id, status);

            // Disable back button if no more history
            if (history_is_empty(&state->history)) {
                gtk_widget_set_sensitive(state->back_button, FALSE);
            }
        } else {
            // Navigation failed (directory may have been deleted)
            show_error_dialog(state->window,
                "Cannot navigate to previous directory. It may have been deleted.");

            // Clear history to prevent further errors
            history_clear(&state->history);
            gtk_widget_set_sensitive(state->back_button, FALSE);
        }
    }
}
```

**Task 4: Cleanup on Logout**

```c
// Modify on_logout_activate() (line 25-34)
if (response == GTK_RESPONSE_YES) {
    g_logout_requested = TRUE;

    // NEW: Clear navigation history on logout
    history_clear(&state->history);
    gtk_widget_set_sensitive(state->back_button, FALSE);

    if (state->conn) {
        client_disconnect(state->conn);
        state->conn = NULL;
    }

    gtk_main_quit();
}
```

**Task 5: Cleanup on Destroy**

```c
// Modify on_main_window_destroy() (line 37-43)
static void on_main_window_destroy(GtkWidget *widget, AppState *state) {
    // NEW: Free navigation history
    history_free(&state->history);

    if (state->conn) {
        client_disconnect(state->conn);
        state->conn = NULL;
    }
    gtk_main_quit();
}
```

**Task 6: Add Forward Declaration**

```c
// Add after line 7 in main_window.c
static void on_back_clicked(GtkWidget *widget, AppState *state);
```

---

### Phase 5: Edge Case Handling

**Scenario 1: Directory Deleted While in History**

**Problem:** User navigates A → B → C, then B is deleted, user clicks back.

**Solution:** Already handled in on_back_clicked():
```c
if (client_cd(state->conn, prev_dir_id) == 0) {
    // Success path
} else {
    // Error handling: show dialog, clear history
}
```

**Scenario 2: Root Directory**

**Problem:** User at root (/) has no parent, back button should be disabled.

**Solution:**
- History only populated when navigating forward
- At root with empty history → button disabled
- Works correctly with current implementation

**Scenario 3: Memory Leak**

**Problem:** History not freed on logout/exit.

**Solution:** Added cleanup in:
- on_main_window_destroy(): Calls history_free()
- on_logout_activate(): Calls history_clear() (doesn't free, but resets for relogin)

**Scenario 4: Rapid Navigation**

**Problem:** User navigates rapidly, history grows unbounded.

**Solution:** HISTORY_MAX_CAPACITY = 50, with oldest-entry removal on overflow.

**Scenario 5: Search → Navigate → Back**

**Problem:** User searches, navigates to result, expects back to return to pre-search location.

**Current Behavior:** Search doesn't change current_directory, so back works correctly.

**If Search Changed Directory:** Would need to push before search navigation too.

---

## 4. Testing Plan

### 4.1 Manual Testing Checklist

| Test Case | Steps | Expected Result |
|-----------|-------|-----------------|
| Basic Navigation | Navigate A → B → C, click back twice | Returns to B, then A |
| Button State | Start at root | Back button disabled |
| Button State | Navigate to subfolder | Back button enabled |
| Button State | Navigate forward, then back to root | Back button disabled after returning to root |
| Deep Navigation | Navigate 10 levels deep, click back 10 times | Returns to root, button disabled |
| Max History | Navigate 60 folders (> max 50) | Oldest history dropped, back works for last 50 |
| Logout | Navigate A → B, logout, login | History cleared, back button disabled |
| Deleted Directory | Navigate A → B → C, delete B via another client, click back | Error dialog, history cleared |
| Mixed Operations | Navigate, upload file, navigate back | Navigation works, file upload doesn't affect history |
| Search Navigation | Search, open result folder, click back | Returns to folder before search navigation |

### 4.2 Compilation Test

```bash
make clean
make client
./build/client_gui
```

**Expected:** No compiler errors, no warnings related to history code.

### 4.3 Memory Test (Valgrind)

```bash
valgrind --leak-check=full --track-origins=yes ./build/client_gui
# Perform navigation, logout, quit
# Check for "All heap blocks were freed"
```

**Expected:** No memory leaks from history management.

---

## 5. Code Review Checklist

- [ ] History initialized in create_main_window()
- [ ] History freed in on_main_window_destroy()
- [ ] History cleared on logout
- [ ] Back button disabled initially
- [ ] Back button enabled after forward navigation
- [ ] Back button disabled when history empty
- [ ] on_row_activated() pushes before navigation
- [ ] on_row_activated() pops if navigation fails
- [ ] on_back_clicked() handles failed navigation
- [ ] No memory leaks (all mallocs have corresponding frees)
- [ ] No buffer overflows (strncpy with size, null termination)
- [ ] Max capacity enforced (50 entries)
- [ ] Header declarations match implementations

---

## 6. Potential Enhancements (Future Work)

**Not included in this implementation:**

1. **Forward Button:**
   - Requires tracking "undone" backs
   - Use case: Back → Back → Forward
   - Complexity: Medium

2. **History Dropdown:**
   - Show full history in dropdown menu
   - Click any entry to jump directly
   - Complexity: Medium

3. **Persistent History:**
   - Save history to file on exit
   - Restore on login
   - Complexity: Low

4. **Visual History Indicator:**
   - Breadcrumb navigation bar
   - Shows full path as clickable segments
   - Complexity: High

5. **Keyboard Shortcut:**
   - Alt+Left for back
   - Alt+Right for forward
   - Complexity: Low

---

## 7. Summary

**Files Modified:**
1. `src/client/gui/gui.h` - Add history structures and function declarations
2. `src/client/gui/file_operations.c` - Implement history functions, track navigation
3. `src/client/gui/main_window.c` - Add back button UI, implement callback

**Lines of Code Added:** ~150 LOC

**Dependencies:** None (uses existing GTK and client APIs)

**Breaking Changes:** None (backward compatible)

**Performance Impact:** Negligible (50 entries × 516 bytes = ~25KB memory)

**User Experience Impact:** Significant improvement in navigation UX

---

## 8. Implementation Order

**Recommended sequence:**

1. **Session 1:** Phase 1 + Phase 2 (data structures + history functions)
   - Compile test, no runtime changes

2. **Session 2:** Phase 3 (integrate history tracking)
   - Functional but no UI

3. **Session 3:** Phase 4 (back button UI)
   - Full feature complete

4. **Session 4:** Phase 5 + Testing
   - Edge cases, manual testing

**Total Estimated Time:** 2-3 hours for experienced developer

---

**Plan Status:** Ready for implementation
**Next Step:** Begin Phase 1 (Data Structure Updates in gui.h)
