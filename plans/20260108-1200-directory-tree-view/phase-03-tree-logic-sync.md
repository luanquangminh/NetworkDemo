# Phase 3: Tree Logic & Synchronization

**Date**: 2026-01-08
**Status**: Pending
**Priority**: High

## Context

Implement tree population logic, lazy loading, and synchronization between tree view and file list.

## Requirements

1. Populate root directory on startup
2. Load subdirectories on expand (lazy loading)
3. Navigate to directory when tree item clicked
4. Update tree selection when navigating via file list
5. Sync tree when directories created/deleted

## Implementation Details

### Tree Population

**populate_tree_root()**
- Call `client_list_dir_gui()` for root (ID from conn)
- Filter response to only directories (is_directory = 1)
- Add each directory to tree store root level
- Set HasChildren = TRUE if directory may have subdirectories
- Set IsLoaded = FALSE for lazy loading

**load_tree_children()**
- Get directory ID from tree iter
- Check IsLoaded flag, skip if already loaded
- Call `client_list_dir_gui()` with directory ID
- Filter to directories only
- Add children to tree under parent iter
- Set IsLoaded = TRUE on parent

### Navigation Logic

**on_tree_cursor_changed()**
- Get selected tree iter
- Extract directory ID
- Save current directory to history
- Call `client_cd()` to navigate
- Update `state->current_directory` and `state->current_path`
- Call `refresh_file_list()` to update file list
- Update status bar
- Enable back button

**on_tree_row_expanded()**
- Get expanded tree iter
- Check IsLoaded flag
- If FALSE, call `load_tree_children()`
- Mark as loaded

### Synchronization

**update_tree_selection()**
- Search tree for node matching `state->current_directory`
- Use recursive search through tree store
- Select matching node
- Expand parent path if needed
- Scroll to make visible

**Update refresh_file_list()**
- After populating file list, call `update_tree_selection()`

**Update on_mkdir_clicked()**
- After successful mkdir, add directory to tree under current node
- Find current directory in tree
- Append new child node

**Update on_delete_clicked()**
- After successful delete, remove from tree if directory
- Find node by ID
- Remove iter from tree store

### Helper Functions

**find_tree_iter_by_id()**
```c
// Recursive search through tree store
// Returns TRUE if found, iter set to matching node
gboolean find_tree_iter_by_id(GtkTreeStore *store, GtkTreeIter *iter,
                               GtkTreeIter *parent, int dir_id)
```

**is_directory_in_tree()**
```c
// Check if directory ID exists in tree
gboolean is_directory_in_tree(AppState *state, int dir_id)
```

**get_directory_path_in_tree()**
```c
// Get GtkTreePath for directory ID
GtkTreePath* get_directory_path_in_tree(AppState *state, int dir_id)
```

## Todo List

- [ ] Implement populate_tree_root()
- [ ] Implement load_tree_children()
- [ ] Implement on_tree_cursor_changed()
- [ ] Implement on_tree_row_expanded()
- [ ] Implement find_tree_iter_by_id()
- [ ] Implement update_tree_selection()
- [ ] Update refresh_file_list() to sync tree
- [ ] Update on_mkdir_clicked() to add to tree
- [ ] Update on_delete_clicked() to remove from tree
- [ ] Connect signals in create_main_window()
- [ ] Test navigation and synchronization
- [ ] Compile and fix errors

## Success Criteria

- Tree populates with root directory on startup
- Expanding tree nodes loads subdirectories
- Clicking tree navigates to directory and updates file list
- Navigating via file list updates tree selection
- Creating directory adds to tree
- Deleting directory removes from tree
- Back button maintains tree selection

## Related Files

- `/Users/minhbohung111/workspace/projects/networkFinal/src/client/gui/main_window.c`
- `/Users/minhbohung111/workspace/projects/networkFinal/src/client/gui/file_operations.c`

## Risk Assessment

**Medium Risk**: Synchronization complexity
- Tree and file list may get out of sync if not careful
- Lazy loading state tracking requires attention

**Mitigation**:
- Test each sync point individually
- Add defensive checks for NULL iters
- Log tree operations during development

## Next Steps

After tree logic is working, proceed to Phase 4: Context Menu & Interactions
