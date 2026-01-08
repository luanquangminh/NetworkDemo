# Right-Click Functionality Diagnostic Report
**Date:** 2026-01-08
**Issue:** Right-click context menu not working in GTK GUI client
**Reported By:** User

---

## Executive Summary

**Root Cause Identified:** Right-click event handling is completely missing from the GUI implementation. No button-press event handlers or context menu widgets exist in the codebase.

**Impact:** Users cannot access file operations (rename, copy, move, delete, download, chmod) via right-click context menus, despite these operations being implemented in the backend.

**Priority:** Medium - Affects user experience but workaround exists (toolbar buttons)

---

## Evidence Chain

### 1. Current Implementation Analysis

**File:** `/src/client/gui/main_window.c`
- **Lines 186-188:** Tree view created with only `row-activated` signal (double-click)
- **No button-press-event handler:** Missing signal connection for right-click detection
- **No context menu widget:** No GtkMenu created for file operations

**File:** `/src/client/gui/file_operations.c`
- File operation handlers exist but only connected to toolbar buttons
- Functions available: `on_upload_clicked`, `on_download_clicked`, `on_delete_clicked`, `on_chmod_clicked`, `on_mkdir_clicked`
- **Missing:** Handlers for `rename`, `copy`, `move` in GUI (exist in CLI only)

**File:** `/src/client/gui/gui.h`
- **Lines 54-62:** Function declarations for existing operations
- **Missing:** Declarations for right-click handlers and context menu creation

### 2. Backend API Availability

**Available but not exposed in GUI:**
- `client_rename(conn, file_id, new_name)` - Protocol CMD_RENAME (0x45)
- `client_copy(conn, source_id, dest_parent_id, new_name)` - Protocol CMD_COPY (0x46)
- `client_move(conn, file_id, new_parent_id)` - Protocol CMD_MOVE (0x47)

**Source Evidence:**
- `src/client/client.h:47-49` - Function declarations
- `src/client/client.c:593-748` - Full implementations
- `src/common/protocol.h:29-31` - Protocol command definitions
- `src/server/commands.c:64-70` - Server handlers

### 3. Search Results - No Right-Click Implementation

**Pattern Search:** `button.*press|context.*menu|popup.*menu|right.*click`
- **Result:** No matches in `/src/client/gui/`
- **Conclusion:** Zero right-click event handling code exists

**GtkMenu Search:** Found only menubar usage (File → Logout/Quit)
- No popup context menus implemented
- No `gtk_menu_popup_at_pointer` calls
- No `GDK_BUTTON_PRESS_MASK` event handling

---

## Root Cause Analysis

### Missing Components

1. **Event Handler Connection** (CRITICAL)
   - Tree view lacks `button-press-event` signal connection
   - No event mask set to capture mouse button events
   - Current: Only double-click (row-activated) works

2. **Context Menu Widget** (CRITICAL)
   - No GtkMenu widget created for right-click operations
   - Menu items for file operations not defined
   - No popup logic implemented

3. **GUI Operation Handlers** (HIGH)
   - Rename, Copy, Move operations exist in backend but no GUI handlers
   - Current GUI operations: Upload, Download, Delete, Chmod, Mkdir
   - Missing GUI wrappers: Rename, Copy, Move

4. **Selection Context** (MEDIUM)
   - Context menu needs to know which file was right-clicked
   - Current selection model only used by toolbar buttons
   - Need to store right-clicked item for menu actions

---

## Technical Implementation Requirements

### Phase 1: Enable Right-Click Detection

**File:** `src/client/gui/main_window.c`

**Required Changes:**
```c
// Add after line 188 (after row-activated signal connection)
gtk_widget_add_events(state->tree_view, GDK_BUTTON_PRESS_MASK);
g_signal_connect(state->tree_view, "button-press-event",
                G_CALLBACK(on_tree_view_button_press), state);
```

**New Handler Function Needed:**
```c
static gboolean on_tree_view_button_press(GtkWidget *widget,
                                          GdkEventButton *event,
                                          AppState *state) {
    // Check for right-click (button 3)
    if (event->button == 3 && event->type == GDK_BUTTON_PRESS) {
        // Get clicked row path
        GtkTreePath *path;
        if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(widget),
                                          event->x, event->y,
                                          &path, NULL, NULL, NULL)) {
            // Select the row
            GtkTreeSelection *selection = gtk_tree_view_get_selection(
                GTK_TREE_VIEW(widget));
            gtk_tree_selection_select_path(selection, path);
            gtk_tree_path_free(path);

            // Show context menu at pointer
            gtk_menu_popup_at_pointer(GTK_MENU(state->context_menu),
                                     (GdkEvent*)event);
            return TRUE; // Event handled
        }
    }
    return FALSE; // Let other handlers process
}
```

### Phase 2: Create Context Menu

**Add to AppState (gui.h):**
```c
typedef struct {
    // ... existing fields ...
    GtkWidget *context_menu;  // Right-click popup menu
} AppState;
```

**Menu Creation Function:**
```c
static GtkWidget* create_file_context_menu(AppState *state) {
    GtkWidget *menu = gtk_menu_new();

    // Download
    GtkWidget *download_item = gtk_menu_item_new_with_label("Download");
    g_signal_connect(download_item, "activate",
                    G_CALLBACK(on_download_clicked), state);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), download_item);

    // Rename
    GtkWidget *rename_item = gtk_menu_item_new_with_label("Rename...");
    g_signal_connect(rename_item, "activate",
                    G_CALLBACK(on_rename_clicked), state);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), rename_item);

    // Copy
    GtkWidget *copy_item = gtk_menu_item_new_with_label("Copy...");
    g_signal_connect(copy_item, "activate",
                    G_CALLBACK(on_copy_clicked), state);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), copy_item);

    // Move
    GtkWidget *move_item = gtk_menu_item_new_with_label("Move...");
    g_signal_connect(move_item, "activate",
                    G_CALLBACK(on_move_clicked), state);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), move_item);

    // Separator
    gtk_menu_shell_append(GTK_MENU_SHELL(menu),
                         gtk_separator_menu_item_new());

    // Permissions
    GtkWidget *chmod_item = gtk_menu_item_new_with_label("Permissions...");
    g_signal_connect(chmod_item, "activate",
                    G_CALLBACK(on_chmod_clicked), state);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), chmod_item);

    // Separator
    gtk_menu_shell_append(GTK_MENU_SHELL(menu),
                         gtk_separator_menu_item_new());

    // Delete
    GtkWidget *delete_item = gtk_menu_item_new_with_label("Delete");
    g_signal_connect(delete_item, "activate",
                    G_CALLBACK(on_delete_clicked), state);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), delete_item);

    gtk_widget_show_all(menu);
    return menu;
}
```

**Integration Point:**
Add in `create_main_window()` after tree view creation:
```c
// After line 234 (after scrolled window added)
state->context_menu = create_file_context_menu(state);
```

### Phase 3: Implement Missing GUI Handlers

**File:** `src/client/gui/file_operations.c`

**Required New Functions:**

#### 1. Rename Handler
```c
void on_rename_clicked(GtkWidget *widget, AppState *state) {
    GtkTreeSelection *selection = gtk_tree_view_get_selection(
        GTK_TREE_VIEW(state->tree_view));
    GtkTreeModel *model;
    GtkTreeIter iter;

    if (!gtk_tree_selection_get_selected(selection, &model, &iter)) {
        show_error_dialog(state->window, "Please select a file to rename");
        return;
    }

    gint file_id;
    gchar *current_name;
    gtk_tree_model_get(model, &iter, 0, &file_id, 2, &current_name, -1);

    // Create rename dialog
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "Rename File",
        GTK_WINDOW(state->window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Rename", GTK_RESPONSE_OK,
        NULL
    );

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entry), current_name);
    gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);

    GtkWidget *label = gtk_label_new("New name:");
    gtk_box_pack_start(GTK_BOX(content), label, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(content), entry, FALSE, FALSE, 5);
    gtk_widget_show_all(content);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        const char *new_name = gtk_entry_get_text(GTK_ENTRY(entry));

        if (strlen(new_name) > 0 && strcmp(new_name, current_name) != 0) {
            if (client_rename(state->conn, file_id, new_name) == 0) {
                show_info_dialog(state->window, "File renamed successfully!");
                refresh_file_list(state);
            } else {
                show_error_dialog(state->window, "Failed to rename file");
            }
        }
    }

    gtk_widget_destroy(dialog);
    g_free(current_name);
}
```

#### 2. Copy Handler
```c
void on_copy_clicked(GtkWidget *widget, AppState *state) {
    GtkTreeSelection *selection = gtk_tree_view_get_selection(
        GTK_TREE_VIEW(state->tree_view));
    GtkTreeModel *model;
    GtkTreeIter iter;

    if (!gtk_tree_selection_get_selected(selection, &model, &iter)) {
        show_error_dialog(state->window, "Please select a file to copy");
        return;
    }

    gint source_id;
    gchar *source_name;
    gtk_tree_model_get(model, &iter, 0, &source_id, 2, &source_name, -1);

    // Create copy dialog
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "Copy File",
        GTK_WINDOW(state->window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Copy", GTK_RESPONSE_OK,
        NULL
    );

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

    // New name entry
    GtkWidget *name_label = gtk_label_new("New name:");
    GtkWidget *name_entry = gtk_entry_new();
    char default_name[256];
    snprintf(default_name, sizeof(default_name), "%s_copy", source_name);
    gtk_entry_set_text(GTK_ENTRY(name_entry), default_name);

    // Note: For simplicity, copying to current directory
    // Could add directory picker for destination
    GtkWidget *note = gtk_label_new("(Will copy to current directory)");

    gtk_box_pack_start(GTK_BOX(content), name_label, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(content), name_entry, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(content), note, FALSE, FALSE, 5);
    gtk_widget_show_all(content);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        const char *new_name = gtk_entry_get_text(GTK_ENTRY(name_entry));

        if (strlen(new_name) > 0) {
            if (client_copy(state->conn, source_id,
                           state->current_directory, new_name) == 0) {
                show_info_dialog(state->window, "File copied successfully!");
                refresh_file_list(state);
            } else {
                show_error_dialog(state->window, "Failed to copy file");
            }
        }
    }

    gtk_widget_destroy(dialog);
    g_free(source_name);
}
```

#### 3. Move Handler
```c
void on_move_clicked(GtkWidget *widget, AppState *state) {
    GtkTreeSelection *selection = gtk_tree_view_get_selection(
        GTK_TREE_VIEW(state->tree_view));
    GtkTreeModel *model;
    GtkTreeIter iter;

    if (!gtk_tree_selection_get_selected(selection, &model, &iter)) {
        show_error_dialog(state->window, "Please select a file to move");
        return;
    }

    gint file_id;
    gchar *name;
    gtk_tree_model_get(model, &iter, 0, &file_id, 2, &name, -1);

    // Create dialog for destination directory ID
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "Move File",
        GTK_WINDOW(state->window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Move", GTK_RESPONSE_OK,
        NULL
    );

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

    GtkWidget *label = gtk_label_new("Destination directory ID:");
    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Enter directory ID");

    gtk_box_pack_start(GTK_BOX(content), label, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(content), entry, FALSE, FALSE, 5);
    gtk_widget_show_all(content);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        const char *dest_str = gtk_entry_get_text(GTK_ENTRY(entry));

        if (strlen(dest_str) > 0) {
            int dest_id = atoi(dest_str);
            if (client_move(state->conn, file_id, dest_id) == 0) {
                show_info_dialog(state->window, "File moved successfully!");
                refresh_file_list(state);
            } else {
                show_error_dialog(state->window,
                    "Failed to move file. Check destination ID.");
            }
        }
    }

    gtk_widget_destroy(dialog);
    g_free(name);
}
```

**Add to gui.h:**
```c
void on_rename_clicked(GtkWidget *widget, AppState *state);
void on_copy_clicked(GtkWidget *widget, AppState *state);
void on_move_clicked(GtkWidget *widget, AppState *state);
```

---

## Validation Criteria

### Functional Tests
- [ ] Right-click on file shows context menu
- [ ] Right-click on directory shows context menu
- [ ] Right-click on empty space does nothing
- [ ] Context menu appears at mouse pointer location
- [ ] Selecting menu item performs correct action
- [ ] Menu items disabled when no selection exists

### Operations Tests
- [ ] Download via context menu works
- [ ] Rename via context menu updates file list
- [ ] Copy creates duplicate with new name
- [ ] Move transfers file to destination
- [ ] Permissions dialog opens and saves changes
- [ ] Delete confirms and removes file

### Edge Cases
- [ ] Right-click during file transfer doesn't crash
- [ ] Menu closes when clicking elsewhere
- [ ] ESC key closes menu without action
- [ ] Invalid destination IDs show error
- [ ] Empty rename/copy names rejected

---

## Implementation Priority

### P0 - Critical (Must Have)
1. Button-press event handler connection
2. Basic context menu with Download, Delete, Permissions
3. Menu popup at pointer location

### P1 - High (Should Have)
4. Rename operation GUI handler
5. Selection context handling

### P2 - Medium (Nice to Have)
6. Copy operation GUI handler
7. Move operation with directory picker
8. Menu item sensitivity based on file type

### P3 - Low (Future Enhancement)
9. Icons in context menu items
10. Keyboard shortcuts (Ctrl+C, Delete, F2 for rename)
11. Multi-select context menu operations

---

## Risks and Mitigation

### Risk 1: Event Handling Conflicts
**Issue:** Button-press-event might interfere with row selection
**Mitigation:** Return FALSE from handler when not right-click, allowing default selection behavior

### Risk 2: Memory Leaks
**Issue:** Context menu recreated vs reused
**Mitigation:** Create once in main_window, destroy on window cleanup

### Risk 3: Move Operation UX
**Issue:** Entering directory ID is not user-friendly
**Enhancement:** Implement directory browser dialog (future)

---

## Files Requiring Changes

### Modified Files
1. `/src/client/gui/gui.h`
   - Add context_menu to AppState
   - Add function declarations for rename/copy/move

2. `/src/client/gui/main_window.c`
   - Add button-press-event handler
   - Create context menu widget
   - Connect signals

3. `/src/client/gui/file_operations.c`
   - Implement on_rename_clicked
   - Implement on_copy_clicked
   - Implement on_move_clicked

### No New Files Required
All changes fit within existing architecture.

---

## Estimated Effort

- **Investigation:** ✅ Complete (1 hour)
- **Implementation:** 3-4 hours
  - Phase 1 (Right-click detection): 1 hour
  - Phase 2 (Context menu): 1 hour
  - Phase 3 (Rename/Copy/Move): 1.5 hours
  - Testing & Debug: 0.5 hour
- **Total:** ~4-5 hours

---

## Conclusion

Right-click functionality is completely absent due to missing event handlers and context menu widgets. Backend APIs for rename/copy/move exist and are functional (used by CLI), but have no GUI exposure. Implementation is straightforward following GTK3 patterns already used in the codebase for menu creation.

**Recommendation:** Implement in phases, starting with basic context menu (Download, Delete, Permissions) using existing handlers, then add rename/copy/move in subsequent iteration.

**Next Steps:**
1. Confirm implementation approach with stakeholders
2. Implement Phase 1-2 for basic context menu
3. Add Phase 3 operations incrementally
4. Perform validation testing per criteria above
