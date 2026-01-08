# Phase 4: Context Menu & Interactions

**Date**: 2026-01-08
**Status**: Pending
**Priority**: Medium

## Context

Add context menu for tree operations and refine user interactions.

## Requirements

1. Right-click context menu on tree directories
2. Menu items: New Folder, Delete
3. Visual feedback for current directory
4. Proper cursor and selection behavior

## Implementation Details

### Context Menu

**create_tree_context_menu()**
- Create GtkMenu with two items:
  - "New Folder..." - Creates subdirectory under selected directory
  - "Delete" - Deletes selected directory (with confirmation)
- Store reference in AppState if needed

**on_tree_button_press()**
- Check for right-click (button 3)
- Get path at position
- Select the row
- Show context menu at pointer
- Return TRUE to prevent other handlers

### Menu Handlers

**on_tree_mkdir_clicked()**
- Get selected directory from tree
- Show input dialog for new folder name
- Call `client_mkdir()` with selected directory as parent
- Add new folder to tree under selected node
- Expand parent if collapsed

**on_tree_delete_clicked()**
- Get selected directory from tree
- Show confirmation dialog
- Call `client_delete()` with directory ID
- Remove from tree store
- If current directory deleted, navigate to parent

### Visual Enhancements

**Highlight Current Directory**
- Use custom cell renderer to bold or highlight current directory
- Or use background color to indicate current location
- Update highlight when current_directory changes

**Tree Selection Behavior**
- Single selection mode
- Show selection even when unfocused
- Maintain selection after navigation

## Todo List

- [ ] Create tree context menu
- [ ] Implement on_tree_button_press()
- [ ] Implement on_tree_mkdir_clicked()
- [ ] Implement on_tree_delete_clicked()
- [ ] Add visual highlight for current directory
- [ ] Connect button-press-event signal
- [ ] Test context menu operations
- [ ] Compile and fix errors

## Success Criteria

- Right-click on tree shows context menu
- "New Folder" creates subdirectory under selected directory
- "Delete" removes directory from tree and file list
- Current directory visually distinguished in tree
- Menu operations sync with file list

## Related Files

- `/Users/minhbohung111/workspace/projects/networkFinal/src/client/gui/main_window.c`
- `/Users/minhbohung111/workspace/projects/networkFinal/src/client/gui/file_operations.c`

## Risk Assessment

**Low Risk**: Context menu is standard GTK pattern
- Similar to existing file list context menu
- Operations already implemented in client API

## Next Steps

After context menu is complete, proceed to Phase 5: Testing & Verification
