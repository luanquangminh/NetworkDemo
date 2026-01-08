# Phase 1: Analysis & Design

**Date**: 2026-01-08
**Status**: Completed
**Priority**: High

## Context

Current implementation has single file list view with navigation via double-clicking folders or back button. Need to add directory tree sidebar for quick access to directory hierarchy.

## Key Insights

### Current Structure
- `AppState` manages GUI state with `tree_view` (file list), `file_store` (GtkListStore)
- File list shows files and directories in current location
- Navigation via `on_row_activated()` which calls `client_cd()` and `refresh_file_list()`
- History stack manages back navigation
- Context menu for file operations (download, rename, copy, paste, delete, chmod)

### Required Changes
1. Split main window into two panes using `GtkPaned`
2. Add tree view components to `AppState`
3. Create tree store with hierarchical data model
4. Implement lazy loading for directory expansion
5. Sync tree selection with current directory

## Architecture

### Data Model
```
GtkTreeStore with columns:
- Column 0: Directory ID (gint)
- Column 1: Directory name (gchar*)
- Column 2: Icon name (gchar*) - "folder"
- Column 3: Is loaded (gboolean) - for lazy loading
- Column 4: Has children (gboolean) - for expand arrow
```

### UI Layout
```
Window
├── MenuBar
├── Toolbar
└── GtkPaned (horizontal)
    ├── Left Pane: Directory Tree (GtkTreeView with GtkTreeStore)
    │   └── Shows only directories, hierarchical
    └── Right Pane: File List (existing GtkTreeView with GtkListStore)
        └── Shows current directory contents
```

### Component Flow
1. **Startup**: Load root directory in tree, populate with immediate children
2. **Row Expand**: Load children on demand when user expands node
3. **Tree Click**: Navigate to directory, update file list, highlight in tree
4. **File List Navigate**: Update tree selection to match current directory
5. **Directory Create**: Add to tree under current parent
6. **Directory Delete**: Remove from tree

## Related Code Files

- `/Users/minhbohung111/workspace/projects/networkFinal/src/client/gui/gui.h` - AppState structure
- `/Users/minhbohung111/workspace/projects/networkFinal/src/client/gui/main_window.c` - Window creation
- `/Users/minhbohung111/workspace/projects/networkFinal/src/client/gui/file_operations.c` - File operations

## Implementation Steps

1. Update `gui.h` to add tree components to AppState
2. Modify `create_main_window()` to create two-pane layout
3. Create tree view with GtkTreeStore
4. Implement tree population functions
5. Connect signals for tree interactions
6. Update file operations to sync tree

## Todo List

- [x] Analyze current codebase structure
- [x] Design data model for tree store
- [x] Design UI layout architecture
- [x] Identify required code changes
- [x] Document implementation steps

## Success Criteria

- Clear understanding of current architecture
- Well-defined data model for tree
- Documented UI layout design
- Identified all required code changes

## Risk Assessment

**Low Risk**:
- Adding new UI components without modifying existing logic
- Using standard GTK widgets (GtkPaned, GtkTreeStore)

**Medium Risk**:
- Synchronization between tree and file list
- Lazy loading implementation complexity

**Mitigation**:
- Implement incremental changes with compile checks
- Test synchronization thoroughly
- Use existing `client_list_dir_gui()` API for loading

## Security Considerations

- No security concerns (UI-only changes)
- Uses existing client API functions with authentication

## Next Steps

Proceed to Phase 2: UI Components Implementation
