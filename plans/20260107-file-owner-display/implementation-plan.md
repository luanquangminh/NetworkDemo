# Implementation Plan: File Owner/Author Display in GUI

## Executive Summary

Add owner/author username column to GUI file list TreeView. Server already transmits `owner_id` in file listings; implementation requires fetching username from database and displaying in GUI.

---

## 1. Current System Analysis

### 1.1 Database Schema
**Location:** `src/database/db_init.sql`

```sql
CREATE TABLE files (
    id INTEGER PRIMARY KEY,
    owner_id INTEGER NOT NULL,  -- ✓ Owner information exists
    FOREIGN KEY (owner_id) REFERENCES users(id)
)

CREATE TABLE users (
    id INTEGER PRIMARY KEY,
    username TEXT UNIQUE NOT NULL
)
```

**Status:** ✓ Database tracks file ownership via `owner_id`

### 1.2 Protocol & Data Structures
**Location:** `src/database/db_manager.h`

```c
typedef struct {
    int owner_id;      // ✓ Owner ID present
    char name[256];
    // ... other fields
} FileEntry;
```

**Status:** ✓ `FileEntry` includes `owner_id` field

### 1.3 Server-Side File Listing
**Location:** `src/server/commands.c:175-224` (`handle_list_dir`)

```c
for (int i = 0; i < count; i++) {
    cJSON_AddNumberToObject(item, "owner_id", entries[i].owner_id);  // ✓ Sent to client
}
```

**Status:** ✓ Server already sends `owner_id` in JSON response

### 1.4 Client-Side Processing
**Location:** `src/client/gui/main_window.c:176-183`

```c
state->file_store = gtk_list_store_new(6,
    G_TYPE_INT,      // ID
    G_TYPE_STRING,   // Icon
    G_TYPE_STRING,   // Name
    G_TYPE_STRING,   // Type
    G_TYPE_INT,      // Size
    G_TYPE_STRING    // Permissions
);
```

**Current Columns:**
1. ID (hidden)
2. Icon
3. Name
4. Type
5. Size
6. Permissions

**Status:** ⚠ No owner column exists; needs to be added

---

## 2. Gap Analysis

### What's Missing
1. **Server-side:** Username retrieval logic (owner_id → username mapping)
2. **Protocol/JSON:** Username not included in LS_RESPONSE
3. **GUI:** Owner column not defined in TreeView
4. **Client parsing:** Code to extract and display owner username

### What Works
- Database schema supports ownership tracking
- Server sends `owner_id` in file listings
- Database has `db_get_user_by_id()` function for username lookup

---

## 3. Design Solution

### 3.1 Approach: Server-Side Username Resolution

**Rationale:**
- Centralized logic (single source of truth)
- Reduces client complexity
- Consistent with existing architecture (server sends ready-to-display data)
- Follows pattern used in search results (commands.c:1128 includes owner_id)

### 3.2 Column Placement
Position: Between "Type" and "Size" columns

**Reasoning:**
- Logical grouping: Name → Type → Owner → Size → Permissions
- Owner relates more to file metadata than permissions
- Matches search results dialog layout (which includes path after permissions)

### 3.3 UI Considerations
- **Column Width:** 100px default (usernames typically 3-32 chars)
- **Sortable:** Yes (alphabetical sort by username)
- **Resizable:** Yes (user preference)
- **Alignment:** Left-aligned text

---

## 4. Implementation Steps

### Phase 1: Server-Side Changes

#### Step 1.1: Modify `handle_list_dir` in `src/server/commands.c`
**Lines:** 175-224

**Changes:**
```c
for (int i = 0; i < count; i++) {
    // Fetch username from owner_id
    char owner_username[256] = "unknown";
    db_get_user_by_id(global_db, entries[i].owner_id,
                      owner_username, sizeof(owner_username));

    cJSON* item = cJSON_CreateObject();
    cJSON_AddNumberToObject(item, "owner_id", entries[i].owner_id);
    cJSON_AddStringToObject(item, "owner", owner_username);  // NEW
    // ... existing fields
}
```

**Testing:**
- Verify username resolution for existing files
- Handle edge cases (deleted users → display "unknown" or user ID)
- Performance check (N+1 query concern for large directories)

#### Step 1.2: Performance Optimization (Optional)
If directory listings are slow with many files, consider batch username lookup or caching.

**Option A:** JOIN query in `db_list_directory`
```sql
SELECT f.*, u.username
FROM files f
LEFT JOIN users u ON f.owner_id = u.id
WHERE f.parent_id = ?
```

**Option B:** Keep current approach (simpler, adequate for typical directory sizes <100 files)

**Decision:** Start with Option B (loop-based lookup). Optimize if performance issues arise.

---

### Phase 2: GUI Client Changes

#### Step 2.1: Update TreeView Model in `src/client/gui/main_window.c`
**Lines:** 176-183

**Before:**
```c
state->file_store = gtk_list_store_new(6, ...);
```

**After:**
```c
state->file_store = gtk_list_store_new(7,  // Increment count
    G_TYPE_INT,      // 0: ID
    G_TYPE_STRING,   // 1: Icon
    G_TYPE_STRING,   // 2: Name
    G_TYPE_STRING,   // 3: Type
    G_TYPE_STRING,   // 4: Owner (NEW)
    G_TYPE_INT,      // 5: Size
    G_TYPE_STRING    // 6: Permissions
);
```

**Impact:** All column indices shift after position 3

#### Step 2.2: Add Owner Column to TreeView
**Lines:** 199-222 (after Type column, before Size column)

```c
// Owner column (NEW)
renderer = gtk_cell_renderer_text_new();
column = gtk_tree_view_column_new_with_attributes("Owner", renderer,
    "text", 4, NULL);  // Column index 4
gtk_tree_view_column_set_sort_column_id(column, 4);
gtk_tree_view_column_set_resizable(column, TRUE);
gtk_tree_view_column_set_min_width(column, 100);
gtk_tree_view_append_column(GTK_TREE_VIEW(state->tree_view), column);
```

**Note:** Update subsequent column references:
- Size column: `"text", 5` (was 4)
- Permissions column: `"text", 6` (was 5)

#### Step 2.3: Update File List Population Logic
**Search for:** Code that populates file_store with data (likely in `refresh_file_list()` or similar)

**Required Changes:**
```c
// Extract owner from JSON response
const char* owner = "unknown";
cJSON* owner_obj = cJSON_GetObjectItem(file, "owner");
if (owner_obj) {
    owner = cJSON_GetStringValue(owner_obj);
}

gtk_list_store_set(store, &iter,
    0, id,
    1, icon,
    2, name,
    3, type,
    4, owner,      // NEW
    5, size,
    6, perms_str,
    -1);
```

#### Step 2.4: Update Search Results Dialog (Optional but Recommended)
**Location:** `src/client/gui/main_window.c:243-392` (`show_search_results_dialog`)

**Current:** Search results show path but not owner
**Recommendation:** Add owner column for consistency (search results already include `owner_id` from server)

---

### Phase 3: Client Library Changes (if needed)

#### Check `client_list_dir_gui()` in `src/client/client.c`
**Purpose:** Ensure JSON response parsing doesn't filter out owner field

**Expected:** Function should pass through entire JSON response from server (no changes needed)

---

## 5. Testing Checklist

### Unit Tests
- [ ] Server sends `owner` field in LS_RESPONSE JSON
- [ ] Username resolution handles deleted users gracefully
- [ ] TreeView model accepts 7 columns without errors
- [ ] Column indices correctly updated (no off-by-one errors)

### Integration Tests
- [ ] File list displays owner usernames correctly
- [ ] Owner column sorts alphabetically
- [ ] Owner column resizes properly
- [ ] Files owned by different users show different usernames
- [ ] Files owned by deleted users show fallback text ("unknown" or user ID)

### GUI Tests
- [ ] Owner column appears between Type and Size
- [ ] Column header says "Owner"
- [ ] Text is readable (not truncated unless column too narrow)
- [ ] Clicking column header sorts by owner
- [ ] Layout doesn't break with long usernames

### Edge Cases
- [ ] Empty directory (no files to display)
- [ ] Directory with 100+ files (performance acceptable)
- [ ] Files owned by admin vs. regular users
- [ ] Root directory (owner_id = 0)

### Regression Tests
- [ ] Existing columns (Name, Type, Size, Permissions) still work
- [ ] File operations (upload, download, delete) unaffected
- [ ] Search functionality still works
- [ ] Navigation (cd, back button) still works

---

## 6. Potential Issues & Mitigations

### Issue 1: Performance with Large Directories
**Symptom:** Slow directory listings if N+1 queries for usernames
**Mitigation:**
- Short-term: Acceptable for directories <100 files
- Long-term: Implement SQL JOIN in `db_list_directory()` if needed

### Issue 2: Deleted User Handling
**Symptom:** `db_get_user_by_id()` fails for deleted users
**Mitigation:**
- Check return value of `db_get_user_by_id()`
- Display fallback text: `"[deleted user]"` or `"user_123"`

### Issue 3: Column Index Confusion
**Symptom:** Wrong data in columns due to index mismatch
**Mitigation:**
- Use enum for column indices:
```c
enum {
    COL_ID = 0,
    COL_ICON,
    COL_NAME,
    COL_TYPE,
    COL_OWNER,
    COL_SIZE,
    COL_PERMISSIONS,
    NUM_COLS
};
```

### Issue 4: UI Layout Breaking
**Symptom:** Window too narrow for all columns
**Mitigation:**
- Make Owner column resizable
- Set reasonable default width (100px)
- Consider making Size column show abbreviated units (KB/MB)

---

## 7. File Modification Summary

### Server-Side
1. **`src/server/commands.c`** (handle_list_dir)
   - Add username lookup loop
   - Add `owner` field to JSON response

### Client-Side (GUI)
2. **`src/client/gui/main_window.c`**
   - Update `gtk_list_store_new()` call (6 → 7 columns)
   - Add Owner column definition
   - Update column indices for Size and Permissions
   - Update file population code to extract and set owner field

3. **`src/client/gui/gui.h`** (if column enums defined)
   - Add `COL_OWNER` enum value
   - Update `NUM_COLS` constant

### Search Results (Optional)
4. **`src/client/gui/main_window.c`** (show_search_results_dialog)
   - Add owner column to search results TreeView
   - Extract owner from search result JSON

---

## 8. Implementation Order

### Recommended Sequence
1. **Server changes first** (handle_list_dir)
   - Test with curl/API client to verify JSON output
2. **GUI model update** (gtk_list_store_new)
   - Compile and run to verify no crashes
3. **GUI column addition** (Owner column definition)
   - Visual verification of column appearance
4. **Data population** (extract and display owner)
   - End-to-end testing with real files
5. **Search results enhancement** (optional)
   - Consistency improvement

### Compile After Each Step
```bash
make clean && make
```

### Testing After Each Step
- **Step 1:** Check server logs for username resolution
- **Step 2:** Verify GUI launches without errors
- **Step 3:** Check Owner column header appears
- **Step 4:** Verify usernames display correctly
- **Step 5:** Test search results consistency

---

## 9. Alternative Approaches (Not Recommended)

### Alt 1: Client-Side Username Resolution
**Pros:** No server changes needed
**Cons:**
- Requires client to cache user list
- Extra round-trip for user info
- Inconsistent with architecture (server sends display-ready data)

### Alt 2: Show Only User ID
**Pros:** Zero code changes (owner_id already sent)
**Cons:**
- Poor UX (user IDs meaningless to users)
- Doesn't meet requirement ("which file belongs to which author/user")

---

## 10. Success Criteria

### Functional
- Owner username displays in file list for all files
- Column is sortable and resizable
- Handles edge cases (deleted users, root directory)

### Non-Functional
- Directory listing latency increase <100ms
- No visual layout breakage
- Code follows existing patterns (e.g., search results already show owner_id)

### User Experience
- Users can immediately identify file owners
- Column placement is intuitive
- Information is accurate and up-to-date

---

## 11. Estimated Effort

### Development
- Server changes: **1-2 hours**
- GUI changes: **2-3 hours**
- Testing: **2 hours**
- **Total: 5-7 hours**

### Risk Level
**Low** - Straightforward feature addition with clear precedent (search results handling)

---

## 12. Future Enhancements (Out of Scope)

1. **User Avatars/Icons** - Show user icons in Owner column
2. **Ownership Transfer** - Right-click menu to change file owner (admin feature)
3. **Filter by Owner** - Dropdown to show only files owned by specific user
4. **Owner Tooltip** - Hover shows full user info (email, created date, etc.)
5. **Batch Username Lookup** - Optimize server query to fetch all usernames in one query

---

## Appendix A: Code References

### Database Functions (Available)
- `db_get_user_by_id()` - src/database/db_manager.c:136
- `db_list_directory()` - src/database/db_manager.c:301

### Similar Implementations (Reference)
- Search results include owner_id: src/server/commands.c:1128
- Search dialog with 7 columns: src/client/gui/main_window.c:272
- Permission column display: src/client/gui/main_window.c:218

### GTK Documentation
- GtkListStore: https://docs.gtk.org/gtk3/class.ListStore.html
- GtkTreeView: https://docs.gtk.org/gtk3/class.TreeView.html
- GtkTreeViewColumn: https://docs.gtk.org/gtk3/class.TreeViewColumn.html

---

## Appendix B: Database Schema Verification

```bash
# Verify files table has owner_id
sqlite3 fileshare.db "PRAGMA table_info(files);" | grep owner_id

# Sample query to test username resolution
sqlite3 fileshare.db "
SELECT f.name, u.username
FROM files f
LEFT JOIN users u ON f.owner_id = u.id
LIMIT 5;
"
```

---

## Document Metadata
- **Created:** 2026-01-07
- **Feature:** File Owner Display in GUI
- **Status:** Planning Phase
- **Priority:** Medium
- **Complexity:** Low-Medium
