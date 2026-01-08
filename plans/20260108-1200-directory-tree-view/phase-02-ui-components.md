# Phase 2: UI Components Implementation

**Date**: 2026-01-08
**Status**: In Progress
**Priority**: High

## Context

Implement the UI components for directory tree view including GtkPaned layout, tree view widgets, and tree store.

## Requirements

1. Add tree-related fields to AppState structure
2. Create two-pane layout with GtkPaned
3. Create directory tree view with GtkTreeStore
4. Set up tree view columns and renderers
5. Move existing file list to right pane

## Implementation Details

### gui.h Changes

Add to AppState structure:
```c
GtkWidget *tree_sidebar;        // Directory tree view widget
GtkTreeStore *dir_tree_store;   // Hierarchical directory model
GtkWidget *paned;               // Horizontal pane splitter
```

### main_window.c Changes

1. **Replace scrolled window section** with GtkPaned:
   - Create horizontal GtkPaned
   - Left pane: Directory tree in scrolled window
   - Right pane: Existing file list in scrolled window
   - Set default position to 250px

2. **Create tree store**:
   - 5 columns: ID, Name, Icon, IsLoaded, HasChildren
   - Initialize with root directory

3. **Create tree view**:
   - Single column showing icon + directory name
   - Connect "cursor-changed" signal for navigation
   - Connect "row-expanded" signal for lazy loading
   - Enable single selection mode

4. **Setup tree appearance**:
   - Hide headers
   - Set expand property
   - Configure drag-and-drop (optional, future enhancement)

## Code Structure

### New Signal Handlers
- `on_tree_cursor_changed()` - Handle tree selection, navigate to directory
- `on_tree_row_expanded()` - Load subdirectories on demand
- `on_tree_button_press()` - Right-click context menu (Phase 4)

### Helper Functions
- `populate_tree_root()` - Load root directory into tree
- `load_tree_children()` - Load children for expanded node
- `find_tree_iter_by_id()` - Locate tree node by directory ID
- `update_tree_selection()` - Highlight current directory in tree

## Todo List

- [ ] Update gui.h with new AppState fields
- [ ] Modify create_main_window() to create GtkPaned
- [ ] Create directory tree view with GtkTreeStore
- [ ] Setup tree columns and cell renderers
- [ ] Move file list to right pane
- [ ] Add forward declarations for signal handlers
- [ ] Compile and fix any errors

## Success Criteria

- Code compiles without errors
- Two-pane layout visible in GUI
- Tree view displays on left side
- File list displays on right side
- Splitter is adjustable

## Related Files

- `/Users/minhbohung111/workspace/projects/networkFinal/src/client/gui/gui.h`
- `/Users/minhbohung111/workspace/projects/networkFinal/src/client/gui/main_window.c`

## Next Steps

After UI components are created, proceed to Phase 3: Tree Logic & Synchronization
