#include "gui.h"
#include <string.h>
#include "../../../lib/cJSON/cJSON.h"

// Global flag to distinguish logout from quit
extern gboolean g_logout_requested;

// Forward declarations
static void on_back_clicked(GtkWidget *widget, AppState *state);
static GtkWidget* create_file_context_menu(AppState *state);
static GtkWidget* create_empty_space_context_menu(AppState *state);
static gboolean on_tree_view_button_press(GtkWidget *widget, GdkEventButton *event, AppState *state);
static void on_drag_data_get(GtkWidget *widget, GdkDragContext *context,
                             GtkSelectionData *data, guint info, guint time,
                             AppState *state);
static void on_drag_data_received(GtkWidget *widget, GdkDragContext *context,
                                   gint x, gint y, GtkSelectionData *data,
                                   guint info, guint time, AppState *state);
static gboolean on_drag_motion(GtkWidget *widget, GdkDragContext *context,
                                gint x, gint y, guint time, AppState *state);
// Directory tree functions (static - internal to main_window.c)
static void populate_tree_root(AppState *state);
static void load_tree_children(AppState *state, GtkTreeIter *parent_iter);
static void on_tree_cursor_changed(GtkTreeSelection *selection, AppState *state);
static void on_tree_row_expanded(GtkTreeView *tree_view, GtkTreeIter *iter,
                                  GtkTreePath *path, AppState *state);
static gboolean find_tree_iter_by_id(GtkTreeStore *store, GtkTreeIter *result,
                                      GtkTreeIter *parent, int dir_id);
// update_tree_selection, add_directory_to_tree, remove_directory_from_tree are exported in gui.h
static GtkWidget* create_tree_context_menu(AppState *state);
static gboolean on_tree_sidebar_button_press(GtkWidget *widget, GdkEventButton *event, AppState *state);

static void on_quit_activate(GtkWidget *widget, AppState *state) {
    gtk_main_quit();
}

static void on_logout_activate(GtkWidget *widget, AppState *state) {
    // Confirmation dialog
    GtkWidget *dialog = gtk_message_dialog_new(
        GTK_WINDOW(state->window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_QUESTION,
        GTK_BUTTONS_YES_NO,
        "Are you sure you want to logout?"
    );

    gint response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    if (response == GTK_RESPONSE_YES) {
        g_logout_requested = TRUE;

        // Clear navigation history on logout
        history_clear(&state->history);
        gtk_widget_set_sensitive(state->back_button, FALSE);

        if (state->conn) {
            client_disconnect(state->conn);
            state->conn = NULL;
        }

        gtk_main_quit();
    }
}

static void on_main_window_destroy(GtkWidget *widget, AppState *state) {
    // Free navigation history
    history_free(&state->history);

    if (state->conn) {
        client_disconnect(state->conn);
        state->conn = NULL;
    }
    gtk_main_quit();
}

// Right-click event handler
static gboolean on_tree_view_button_press(GtkWidget *widget, GdkEventButton *event, AppState *state) {
    // Check for right-click (button 3)
    if (event->button == 3 && event->type == GDK_BUTTON_PRESS) {
        // Get clicked row path
        GtkTreePath *path;
        if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(widget),
                                          event->x, event->y,
                                          &path, NULL, NULL, NULL)) {
            // Clicked on a file/folder item
            // Select the row
            GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(widget));
            gtk_tree_selection_select_path(selection, path);
            gtk_tree_path_free(path);

            // Show file context menu at pointer
            gtk_menu_popup_at_pointer(GTK_MENU(state->context_menu), (GdkEvent*)event);
            return TRUE; // Event handled
        } else {
            // Clicked on empty space - show empty space context menu
            gtk_menu_popup_at_pointer(GTK_MENU(state->empty_space_context_menu), (GdkEvent*)event);
            return TRUE; // Event handled
        }
    }
    return FALSE; // Let other handlers process
}

// Drag-and-drop signal handlers
static void on_drag_data_get(GtkWidget *widget, GdkDragContext *context,
                             GtkSelectionData *data, guint info, guint time,
                             AppState *state) {
    (void)widget;
    (void)context;
    (void)info;
    (void)time;

    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(state->tree_view));
    GtkTreeModel *model;
    GtkTreeIter iter;

    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
        gint file_id;
        gtk_tree_model_get(model, &iter, 0, &file_id, -1);

        // Send file ID as text data
        char file_id_str[32];
        snprintf(file_id_str, sizeof(file_id_str), "%d", file_id);
        gtk_selection_data_set_text(data, file_id_str, -1);
    }
}

static void on_drag_data_received(GtkWidget *widget, GdkDragContext *context,
                                   gint x, gint y, GtkSelectionData *data,
                                   guint info, guint time, AppState *state) {
    (void)widget;
    (void)info;

    gboolean success = FALSE;

    // Get drop target path
    GtkTreePath *path;
    GtkTreeViewDropPosition pos;
    if (gtk_tree_view_get_dest_row_at_pos(GTK_TREE_VIEW(state->tree_view),
                                          x, y, &path, &pos)) {
        GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(state->tree_view));
        GtkTreeIter iter;

        if (gtk_tree_model_get_iter(model, &iter, path)) {
            gint dest_id;
            gchar *dest_type;
            gtk_tree_model_get(model, &iter, 0, &dest_id, 3, &dest_type, -1);

            // Only allow drop on directories
            if (strcmp(dest_type, "Directory") == 0) {
                // Get dragged file ID from selection data
                const guchar *text_data = gtk_selection_data_get_text(data);
                if (text_data) {
                    int file_id = atoi((const char*)text_data);

                    // Prevent dropping on self
                    if (file_id != dest_id) {
                        // Execute move operation
                        if (client_move(state->conn, file_id, dest_id) == 0) {
                            success = TRUE;
                            show_info_dialog(state->window, "File moved successfully!");
                            refresh_file_list(state);
                        } else {
                            show_error_dialog(state->window, "Failed to move file.");
                        }
                    }
                }
            }

            g_free(dest_type);
        }

        gtk_tree_path_free(path);
    }

    gtk_drag_finish(context, success, FALSE, time);
}

static gboolean on_drag_motion(GtkWidget *widget, GdkDragContext *context,
                                gint x, gint y, guint time, AppState *state) {
    (void)state;

    GtkTreePath *path;
    GtkTreeViewDropPosition pos;

    // Check if hovering over a valid drop target
    if (gtk_tree_view_get_dest_row_at_pos(GTK_TREE_VIEW(widget),
                                          x, y, &path, &pos)) {
        GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(widget));
        GtkTreeIter iter;

        if (gtk_tree_model_get_iter(model, &iter, path)) {
            gchar *type;
            gtk_tree_model_get(model, &iter, 3, &type, -1);

            // Only directories are valid drop targets
            if (strcmp(type, "Directory") == 0) {
                gdk_drag_status(context, GDK_ACTION_MOVE, time);
                gtk_tree_view_set_drag_dest_row(GTK_TREE_VIEW(widget), path,
                                                GTK_TREE_VIEW_DROP_INTO_OR_AFTER);
                g_free(type);
                gtk_tree_path_free(path);
                return TRUE;
            }

            g_free(type);
        }

        gtk_tree_path_free(path);
    }

    // Not a valid drop target
    gdk_drag_status(context, 0, time);
    gtk_tree_view_set_drag_dest_row(GTK_TREE_VIEW(widget), NULL,
                                    GTK_TREE_VIEW_DROP_INTO_OR_AFTER);
    return FALSE;
}

// Create context menu for file operations
static GtkWidget* create_file_context_menu(AppState *state) {
    GtkWidget *menu = gtk_menu_new();

    // Download
    GtkWidget *download_item = gtk_menu_item_new_with_label("Download");
    g_signal_connect(download_item, "activate", G_CALLBACK(on_download_clicked), state);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), download_item);

    // Rename
    GtkWidget *rename_item = gtk_menu_item_new_with_label("Rename...");
    g_signal_connect(rename_item, "activate", G_CALLBACK(on_rename_clicked), state);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), rename_item);

    // Copy
    GtkWidget *copy_item = gtk_menu_item_new_with_label("Copy");
    g_signal_connect(copy_item, "activate", G_CALLBACK(on_copy_clicked), state);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), copy_item);

    // Paste
    GtkWidget *paste_item = gtk_menu_item_new_with_label("Paste");
    g_signal_connect(paste_item, "activate", G_CALLBACK(on_paste_clicked), state);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), paste_item);

    // Store reference to paste menu item in state
    state->paste_menu_item = paste_item;

    // Initially disabled (no clipboard data)
    gtk_widget_set_sensitive(paste_item, FALSE);

    // Separator
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

    // Permissions
    GtkWidget *chmod_item = gtk_menu_item_new_with_label("Permissions...");
    g_signal_connect(chmod_item, "activate", G_CALLBACK(on_chmod_clicked), state);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), chmod_item);

    // Separator
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

    // Delete
    GtkWidget *delete_item = gtk_menu_item_new_with_label("Delete");
    g_signal_connect(delete_item, "activate", G_CALLBACK(on_delete_clicked), state);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), delete_item);

    gtk_widget_show_all(menu);
    return menu;
}

// Create context menu for empty space operations
static GtkWidget* create_empty_space_context_menu(AppState *state) {
    GtkWidget *menu = gtk_menu_new();

    // New Folder
    GtkWidget *mkdir_item = gtk_menu_item_new_with_label("New Folder...");
    g_signal_connect(mkdir_item, "activate", G_CALLBACK(on_mkdir_clicked), state);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), mkdir_item);

    // Paste
    GtkWidget *paste_item = gtk_menu_item_new_with_label("Paste");
    g_signal_connect(paste_item, "activate", G_CALLBACK(on_paste_clicked), state);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), paste_item);

    // Store reference to paste menu item in state
    state->empty_space_paste_item = paste_item;

    // Initially disabled (no clipboard data)
    gtk_widget_set_sensitive(paste_item, FALSE);

    // Separator
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

    // Refresh
    GtkWidget *refresh_item = gtk_menu_item_new_with_label("Refresh");
    g_signal_connect_swapped(refresh_item, "activate", G_CALLBACK(refresh_file_list), state);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), refresh_item);

    // Upload
    GtkWidget *upload_item = gtk_menu_item_new_with_label("Upload...");
    g_signal_connect(upload_item, "activate", G_CALLBACK(on_upload_clicked), state);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), upload_item);

    gtk_widget_show_all(menu);
    return menu;
}

GtkWidget* create_main_window(AppState *state) {
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "File Sharing Client");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
    g_signal_connect(window, "destroy", G_CALLBACK(on_main_window_destroy), state);

    // Initialize navigation history
    history_init(&state->history);

    // Initialize clipboard state
    state->clipboard_file_id = 0;
    state->clipboard_file_name[0] = '\0';
    state->has_clipboard_data = 0;

    // Main vertical box
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    // Menu bar
    GtkWidget *menubar = gtk_menu_bar_new();

    // File menu
    GtkWidget *file_menu = gtk_menu_new();
    GtkWidget *file_item = gtk_menu_item_new_with_label("File");

    // Logout menu item
    GtkWidget *logout_item = gtk_menu_item_new_with_label("Logout");
    g_signal_connect(logout_item, "activate", G_CALLBACK(on_logout_activate), state);
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), logout_item);

    // Separator
    GtkWidget *separator = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), separator);

    // Quit menu item
    GtkWidget *quit_item = gtk_menu_item_new_with_label("Quit");
    g_signal_connect(quit_item, "activate", G_CALLBACK(on_quit_activate), state);
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), quit_item);

    gtk_menu_item_set_submenu(GTK_MENU_ITEM(file_item), file_menu);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), file_item);

    gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);

    // Toolbar
    GtkWidget *toolbar = gtk_toolbar_new();
    gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_BOTH);

    // Back button (leftmost position)
    GtkToolItem *back_btn = gtk_tool_button_new(NULL, "Back");
    gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(back_btn), "go-previous");
    gtk_widget_set_tooltip_text(GTK_WIDGET(back_btn), "Go back to previous directory");
    g_signal_connect(back_btn, "clicked", G_CALLBACK(on_back_clicked), state);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), back_btn, -1);

    // Store reference in state
    state->back_button = GTK_WIDGET(back_btn);

    // Set initial state to disabled (no history yet)
    gtk_widget_set_sensitive(state->back_button, FALSE);

    GtkToolItem *upload_btn = gtk_tool_button_new(NULL, "Upload");
    gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(upload_btn), "document-open");
    g_signal_connect(upload_btn, "clicked", G_CALLBACK(on_upload_clicked), state);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), upload_btn, -1);

    GtkToolItem *download_btn = gtk_tool_button_new(NULL, "Download");
    gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(download_btn), "document-save");
    g_signal_connect(download_btn, "clicked", G_CALLBACK(on_download_clicked), state);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), download_btn, -1);

    GtkToolItem *mkdir_btn = gtk_tool_button_new(NULL, "New Folder");
    gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(mkdir_btn), "folder-new");
    g_signal_connect(mkdir_btn, "clicked", G_CALLBACK(on_mkdir_clicked), state);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), mkdir_btn, -1);

    GtkToolItem *delete_btn = gtk_tool_button_new(NULL, "Delete");
    gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(delete_btn), "edit-delete");
    g_signal_connect(delete_btn, "clicked", G_CALLBACK(on_delete_clicked), state);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), delete_btn, -1);

    GtkToolItem *chmod_btn = gtk_tool_button_new(NULL, "Permissions");
    gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(chmod_btn), "emblem-system");
    g_signal_connect(chmod_btn, "clicked", G_CALLBACK(on_chmod_clicked), state);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), chmod_btn, -1);

    // Add separator
    GtkToolItem *separator_item = gtk_separator_tool_item_new();
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), separator_item, -1);

    // Search entry
    state->search_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(state->search_entry), "Search files...");
    gtk_entry_set_width_chars(GTK_ENTRY(state->search_entry), 20);
    gtk_entry_set_icon_from_icon_name(GTK_ENTRY(state->search_entry),
                                      GTK_ENTRY_ICON_PRIMARY, "edit-find");

    // Allow Enter key to trigger search
    g_signal_connect(state->search_entry, "activate",
                    G_CALLBACK(on_search_clicked), state);

    GtkToolItem *search_entry_item = gtk_tool_item_new();
    gtk_container_add(GTK_CONTAINER(search_entry_item), state->search_entry);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), search_entry_item, -1);

    // Recursive checkbox
    state->search_recursive_check = gtk_check_button_new_with_label("Recursive");
    GtkToolItem *recursive_item = gtk_tool_item_new();
    gtk_container_add(GTK_CONTAINER(recursive_item), state->search_recursive_check);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), recursive_item, -1);

    // Search button
    GtkToolItem *search_btn = gtk_tool_button_new(NULL, "Search");
    gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(search_btn), "edit-find");
    g_signal_connect(search_btn, "clicked", G_CALLBACK(on_search_clicked), state);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), search_btn, -1);

    gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 0);

    // Create horizontal paned (two-pane layout)
    state->paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);

    // Left pane: Directory tree
    GtkWidget *tree_scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(tree_scrolled),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(tree_scrolled, 200, -1);  // Min width 200px

    // Create directory tree store
    // Columns: ID (int), Name (string), Icon (string), IsLoaded (bool), HasChildren (bool)
    state->dir_tree_store = gtk_tree_store_new(5,
        G_TYPE_INT,      // Directory ID
        G_TYPE_STRING,   // Directory name
        G_TYPE_STRING,   // Icon name ("folder")
        G_TYPE_BOOLEAN,  // Is loaded (for lazy loading)
        G_TYPE_BOOLEAN   // Has children (for expand arrow)
    );

    // Create directory tree view
    state->tree_sidebar = gtk_tree_view_new_with_model(GTK_TREE_MODEL(state->dir_tree_store));
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(state->tree_sidebar), FALSE);

    // Tree column with icon and name
    GtkCellRenderer *tree_icon_renderer = gtk_cell_renderer_pixbuf_new();
    GtkCellRenderer *tree_text_renderer = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *tree_column = gtk_tree_view_column_new();
    gtk_tree_view_column_pack_start(tree_column, tree_icon_renderer, FALSE);
    gtk_tree_view_column_pack_start(tree_column, tree_text_renderer, TRUE);
    gtk_tree_view_column_add_attribute(tree_column, tree_icon_renderer, "icon-name", 2);
    gtk_tree_view_column_add_attribute(tree_column, tree_text_renderer, "text", 1);
    gtk_tree_view_append_column(GTK_TREE_VIEW(state->tree_sidebar), tree_column);

    // Connect tree signals
    GtkTreeSelection *tree_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(state->tree_sidebar));
    gtk_tree_selection_set_mode(tree_selection, GTK_SELECTION_SINGLE);
    g_signal_connect(tree_selection, "changed", G_CALLBACK(on_tree_cursor_changed), state);
    g_signal_connect(state->tree_sidebar, "row-expanded", G_CALLBACK(on_tree_row_expanded), state);

    // Enable right-click on tree
    gtk_widget_add_events(state->tree_sidebar, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(state->tree_sidebar, "button-press-event",
                    G_CALLBACK(on_tree_sidebar_button_press), state);

    gtk_container_add(GTK_CONTAINER(tree_scrolled), state->tree_sidebar);
    gtk_paned_pack1(GTK_PANED(state->paned), tree_scrolled, FALSE, TRUE);

    // Right pane: File list
    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);

    // File list (tree view)
    state->file_store = gtk_list_store_new(7,
        G_TYPE_INT,      // ID (0)
        G_TYPE_STRING,   // Icon (1)
        G_TYPE_STRING,   // Name (2)
        G_TYPE_STRING,   // Type (3)
        G_TYPE_STRING,   // Owner (4)
        G_TYPE_INT,      // Size (5)
        G_TYPE_STRING    // Permissions (6)
    );

    state->tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(state->file_store));
    g_signal_connect(state->tree_view, "row-activated",
                    G_CALLBACK(on_row_activated), state);

    // Enable right-click detection
    gtk_widget_add_events(state->tree_view, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(state->tree_view, "button-press-event",
                    G_CALLBACK(on_tree_view_button_press), state);

    // Setup drag-and-drop
    GtkTargetEntry target_entry = {"text/plain", GTK_TARGET_SAME_WIDGET, 0};

    // Enable drag source (allow dragging from tree view)
    gtk_tree_view_enable_model_drag_source(GTK_TREE_VIEW(state->tree_view),
                                           GDK_BUTTON1_MASK,
                                           &target_entry, 1,
                                           GDK_ACTION_MOVE);

    // Enable drop target (allow dropping onto tree view)
    gtk_tree_view_enable_model_drag_dest(GTK_TREE_VIEW(state->tree_view),
                                         &target_entry, 1,
                                         GDK_ACTION_MOVE);

    // Connect drag-and-drop signals
    g_signal_connect(state->tree_view, "drag-data-get",
                    G_CALLBACK(on_drag_data_get), state);
    g_signal_connect(state->tree_view, "drag-data-received",
                    G_CALLBACK(on_drag_data_received), state);
    g_signal_connect(state->tree_view, "drag-motion",
                    G_CALLBACK(on_drag_motion), state);

    // Columns
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;

    // Icon column
    renderer = gtk_cell_renderer_pixbuf_new();
    column = gtk_tree_view_column_new_with_attributes("", renderer,
        "icon-name", 1, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(state->tree_view), column);

    // Name column
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Name", renderer,
        "text", 2, NULL);
    gtk_tree_view_column_set_sort_column_id(column, 2);
    gtk_tree_view_append_column(GTK_TREE_VIEW(state->tree_view), column);

    // Type column
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Type", renderer,
        "text", 3, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(state->tree_view), column);

    // Owner column
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Owner", renderer,
        "text", 4, NULL);
    gtk_tree_view_column_set_sort_column_id(column, 4);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_column_set_min_width(column, 100);
    gtk_tree_view_append_column(GTK_TREE_VIEW(state->tree_view), column);

    // Size column
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Size", renderer,
        "text", 5, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(state->tree_view), column);

    // Permissions column
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Permissions", renderer,
        "text", 6, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(state->tree_view), column);

    gtk_container_add(GTK_CONTAINER(scrolled), state->tree_view);
    gtk_paned_pack2(GTK_PANED(state->paned), scrolled, TRUE, TRUE);

    // Set paned position (250px for tree, rest for file list)
    gtk_paned_set_position(GTK_PANED(state->paned), 250);

    // Add paned to vbox
    gtk_box_pack_start(GTK_BOX(vbox), state->paned, TRUE, TRUE, 0);

    // Create context menus
    state->context_menu = create_file_context_menu(state);
    state->empty_space_context_menu = create_empty_space_context_menu(state);
    state->tree_context_menu = create_tree_context_menu(state);

    // Populate directory tree with root
    populate_tree_root(state);

    // Status bar
    state->status_bar = gtk_statusbar_new();
    gtk_box_pack_start(GTK_BOX(vbox), state->status_bar, FALSE, FALSE, 0);

    // Update status
    guint context_id = gtk_statusbar_get_context_id(
        GTK_STATUSBAR(state->status_bar), "status");
    char status[256];
    snprintf(status, sizeof(status), "Connected as user %d | Current: %s",
             state->conn->user_id, state->current_path);
    gtk_statusbar_push(GTK_STATUSBAR(state->status_bar), context_id, status);

    state->window = window;
    return window;
}

// Directory tree implementation

// Recursive helper to find tree iter by directory ID
static gboolean find_tree_iter_by_id(GtkTreeStore *store, GtkTreeIter *result,
                                      GtkTreeIter *parent, int dir_id) {
    GtkTreeIter iter;
    gboolean valid;

    if (parent) {
        valid = gtk_tree_model_iter_children(GTK_TREE_MODEL(store), &iter, parent);
    } else {
        valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter);
    }

    while (valid) {
        gint id;
        gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 0, &id, -1);

        if (id == dir_id) {
            *result = iter;
            return TRUE;
        }

        // Recursively search children
        if (gtk_tree_model_iter_has_child(GTK_TREE_MODEL(store), &iter)) {
            if (find_tree_iter_by_id(store, result, &iter, dir_id)) {
                return TRUE;
            }
        }

        valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter);
    }

    return FALSE;
}

// Update tree selection to match current directory (exported to gui.h)
void update_tree_selection(AppState *state) {
    GtkTreeIter iter;

    if (find_tree_iter_by_id(state->dir_tree_store, &iter, NULL, state->current_directory)) {
        GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(state->tree_sidebar));

        // Block signal to prevent navigation loop
        g_signal_handlers_block_by_func(selection, G_CALLBACK(on_tree_cursor_changed), state);

        gtk_tree_selection_select_iter(selection, &iter);

        // Expand parent path and scroll to visible
        GtkTreePath *path = gtk_tree_model_get_path(GTK_TREE_MODEL(state->dir_tree_store), &iter);
        gtk_tree_view_expand_to_path(GTK_TREE_VIEW(state->tree_sidebar), path);
        gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(state->tree_sidebar), path, NULL, FALSE, 0, 0);
        gtk_tree_path_free(path);

        // Unblock signal
        g_signal_handlers_unblock_by_func(selection, G_CALLBACK(on_tree_cursor_changed), state);
    }
}

// Load children for a directory node (lazy loading)
static void load_tree_children(AppState *state, GtkTreeIter *parent_iter) {
    gint parent_id;
    gboolean is_loaded;

    gtk_tree_model_get(GTK_TREE_MODEL(state->dir_tree_store), parent_iter,
                      0, &parent_id,
                      3, &is_loaded,
                      -1);

    // Skip if already loaded
    if (is_loaded) {
        return;
    }

    // Get directory listing
    cJSON* resp_json = (cJSON*)client_list_dir_gui(state->conn, parent_id);
    if (!resp_json) {
        return;
    }

    cJSON* files = cJSON_GetObjectItem(resp_json, "files");
    if (files) {
        cJSON* file;
        cJSON_ArrayForEach(file, files) {
            int is_dir = cJSON_GetObjectItem(file, "is_directory")->valueint;

            // Only add directories to tree
            if (is_dir) {
                int id = cJSON_GetObjectItem(file, "id")->valueint;
                const char* name = cJSON_GetStringValue(cJSON_GetObjectItem(file, "name"));

                GtkTreeIter child_iter;
                gtk_tree_store_append(state->dir_tree_store, &child_iter, parent_iter);
                gtk_tree_store_set(state->dir_tree_store, &child_iter,
                    0, id,
                    1, name,
                    2, "folder",
                    3, FALSE,        // Not loaded yet
                    4, TRUE,         // Assume has children (will be verified on expand)
                    -1);
            }
        }
    }

    // Mark parent as loaded
    gtk_tree_store_set(state->dir_tree_store, parent_iter, 3, TRUE, -1);

    cJSON_Delete(resp_json);
}

// Populate tree with root directory
static void populate_tree_root(AppState *state) {
    // Clear existing tree
    gtk_tree_store_clear(state->dir_tree_store);

    // Get root directory listing
    cJSON* resp_json = (cJSON*)client_list_dir_gui(state->conn, state->current_directory);
    if (!resp_json) {
        return;
    }

    // Add current directory as root
    GtkTreeIter root_iter;
    gtk_tree_store_append(state->dir_tree_store, &root_iter, NULL);
    gtk_tree_store_set(state->dir_tree_store, &root_iter,
        0, state->current_directory,
        1, "/",  // Root name
        2, "folder",
        3, FALSE,  // Not loaded yet
        4, TRUE,   // Has children
        -1);

    // Load immediate children
    load_tree_children(state, &root_iter);

    // Expand and select root
    GtkTreePath *root_path = gtk_tree_model_get_path(GTK_TREE_MODEL(state->dir_tree_store), &root_iter);
    gtk_tree_view_expand_row(GTK_TREE_VIEW(state->tree_sidebar), root_path, FALSE);
    gtk_tree_path_free(root_path);

    update_tree_selection(state);
}

// Handle tree selection change (navigate to directory)
static void on_tree_cursor_changed(GtkTreeSelection *selection, AppState *state) {
    GtkTreeModel *model;
    GtkTreeIter iter;

    if (!gtk_tree_selection_get_selected(selection, &model, &iter)) {
        return;
    }

    gint dir_id;
    gtk_tree_model_get(model, &iter, 0, &dir_id, -1);

    // If already at this directory, do nothing
    if (dir_id == state->current_directory) {
        return;
    }

    // Save current directory to history
    history_push(&state->history, state->current_directory, state->current_path);

    // Navigate to selected directory
    if (client_cd(state->conn, dir_id) == 0) {
        state->current_directory = dir_id;
        strcpy(state->current_path, state->conn->current_path);
        refresh_file_list(state);

        // Update status bar
        guint context_id = gtk_statusbar_get_context_id(
            GTK_STATUSBAR(state->status_bar), "status");
        char status[256];
        snprintf(status, sizeof(status), "Current: %s", state->current_path);
        gtk_statusbar_push(GTK_STATUSBAR(state->status_bar), context_id, status);

        // Enable back button
        gtk_widget_set_sensitive(state->back_button, TRUE);
    } else {
        // Navigation failed, restore previous selection
        int dummy_id;
        char dummy_path[512];
        history_pop(&state->history, &dummy_id, dummy_path);
        update_tree_selection(state);
    }
}

// Handle tree row expansion (lazy load children)
static void on_tree_row_expanded(GtkTreeView *tree_view, GtkTreeIter *iter,
                                  GtkTreePath *path, AppState *state) {
    (void)tree_view;
    (void)path;

    load_tree_children(state, iter);
}

// Create context menu for tree operations
static GtkWidget* create_tree_context_menu(AppState *state) {
    GtkWidget *menu = gtk_menu_new();

    // New Folder
    GtkWidget *mkdir_item = gtk_menu_item_new_with_label("New Folder...");
    g_signal_connect(mkdir_item, "activate", G_CALLBACK(on_mkdir_clicked), state);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), mkdir_item);

    // Separator
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

    // Delete
    GtkWidget *delete_item = gtk_menu_item_new_with_label("Delete");
    g_signal_connect(delete_item, "activate", G_CALLBACK(on_delete_clicked), state);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), delete_item);

    gtk_widget_show_all(menu);
    return menu;
}

// Handle right-click on tree
static gboolean on_tree_sidebar_button_press(GtkWidget *widget, GdkEventButton *event, AppState *state) {
    if (event->button == 3 && event->type == GDK_BUTTON_PRESS) {
        GtkTreePath *path;
        if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(widget),
                                          event->x, event->y,
                                          &path, NULL, NULL, NULL)) {
            // Select the row
            GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(widget));
            gtk_tree_selection_select_path(selection, path);
            gtk_tree_path_free(path);

            // Show context menu
            gtk_menu_popup_at_pointer(GTK_MENU(state->tree_context_menu), (GdkEvent*)event);
            return TRUE;
        }
    }
    return FALSE;
}

// Add directory to tree under current directory (exported to gui.h)
void add_directory_to_tree(AppState *state, int dir_id, const char *name) {
    GtkTreeIter parent_iter;

    // Find parent directory in tree (current directory)
    if (find_tree_iter_by_id(state->dir_tree_store, &parent_iter, NULL, state->current_directory)) {
        // Add new directory as child
        GtkTreeIter child_iter;
        gtk_tree_store_append(state->dir_tree_store, &child_iter, &parent_iter);
        gtk_tree_store_set(state->dir_tree_store, &child_iter,
            0, dir_id,
            1, name,
            2, "folder",
            3, FALSE,  // Not loaded yet
            4, TRUE,   // May have children
            -1);

        // Expand parent to show new directory
        GtkTreePath *parent_path = gtk_tree_model_get_path(GTK_TREE_MODEL(state->dir_tree_store), &parent_iter);
        gtk_tree_view_expand_row(GTK_TREE_VIEW(state->tree_sidebar), parent_path, FALSE);
        gtk_tree_path_free(parent_path);
    }
}

// Remove directory from tree (exported to gui.h)
void remove_directory_from_tree(AppState *state, int dir_id) {
    GtkTreeIter iter;

    // Find directory in tree
    if (find_tree_iter_by_id(state->dir_tree_store, &iter, NULL, dir_id)) {
        // Remove from tree
        gtk_tree_store_remove(state->dir_tree_store, &iter);
    }
}

void show_search_results_dialog(GtkWidget *parent, cJSON *results, const char *pattern, int recursive) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "Search Results",
        GTK_WINDOW(parent),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Close", GTK_RESPONSE_CLOSE,
        NULL
    );

    gtk_window_set_default_size(GTK_WINDOW(dialog), 900, 500);

    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(content_area), vbox);

    // Get file count
    cJSON *count_obj = cJSON_GetObjectItem(results, "count");
    int count = count_obj ? count_obj->valueint : 0;

    // Header label
    char header[256];
    snprintf(header, sizeof(header), "Found %d file(s) matching '%s'%s",
             count, pattern, recursive ? " (recursive)" : "");
    GtkWidget *label = gtk_label_new(header);
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 5);

    if (count > 0) {
        // Create tree view for results
        GtkListStore *store = gtk_list_store_new(8,
            G_TYPE_INT,      // ID (0)
            G_TYPE_STRING,   // Icon (1)
            G_TYPE_STRING,   // Name (2)
            G_TYPE_STRING,   // Type (3)
            G_TYPE_STRING,   // Owner (4)
            G_TYPE_INT,      // Size (5)
            G_TYPE_STRING,   // Permissions (6)
            G_TYPE_STRING    // Path (7)
        );

        cJSON *files = cJSON_GetObjectItem(results, "files");
        if (files) {
            cJSON *file;
            cJSON_ArrayForEach(file, files) {
                int id = cJSON_GetObjectItem(file, "id")->valueint;
                int is_dir = cJSON_GetObjectItem(file, "is_directory")->valueint;
                const char *name = cJSON_GetStringValue(cJSON_GetObjectItem(file, "name"));
                int size = cJSON_GetObjectItem(file, "size")->valueint;
                int perms = cJSON_GetObjectItem(file, "permissions")->valueint;

                // Get owner
                const char *owner = "unknown";
                cJSON *owner_obj = cJSON_GetObjectItem(file, "owner");
                if (owner_obj) {
                    owner = cJSON_GetStringValue(owner_obj);
                }

                // Get path
                const char *path = "/";
                cJSON *path_obj = cJSON_GetObjectItem(file, "path");
                if (path_obj) {
                    path = cJSON_GetStringValue(path_obj);
                }

                // Format size
                char size_str[64];
                if (is_dir) {
                    strcpy(size_str, "-");
                } else {
                    if (size < 1024) {
                        snprintf(size_str, sizeof(size_str), "%d B", size);
                    } else if (size < 1024 * 1024) {
                        snprintf(size_str, sizeof(size_str), "%.1f KB", size / 1024.0);
                    } else {
                        snprintf(size_str, sizeof(size_str), "%.1f MB", size / (1024.0 * 1024.0));
                    }
                }

                // Format permissions
                char perms_str[16];
                snprintf(perms_str, sizeof(perms_str), "%03o", perms);

                GtkTreeIter iter;
                gtk_list_store_append(store, &iter);
                gtk_list_store_set(store, &iter,
                    0, id,
                    1, is_dir ? "folder" : "text-x-generic",
                    2, name,
                    3, is_dir ? "Directory" : "File",
                    4, owner,
                    5, size,
                    6, perms_str,
                    7, path,
                    -1);
            }
        }

        GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                       GTK_POLICY_AUTOMATIC,
                                       GTK_POLICY_AUTOMATIC);

        GtkWidget *tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
        g_object_unref(store);

        // Columns
        GtkCellRenderer *renderer;
        GtkTreeViewColumn *column;

        // Icon column
        renderer = gtk_cell_renderer_pixbuf_new();
        column = gtk_tree_view_column_new_with_attributes("", renderer,
            "icon-name", 1, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);

        // Name column
        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Name", renderer,
            "text", 2, NULL);
        gtk_tree_view_column_set_resizable(column, TRUE);
        gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);

        // Type column
        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Type", renderer,
            "text", 3, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);

        // Owner column
        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Owner", renderer,
            "text", 4, NULL);
        gtk_tree_view_column_set_resizable(column, TRUE);
        gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);

        // Size column
        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Size", renderer,
            "text", 5, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);

        // Permissions column
        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Permissions", renderer,
            "text", 6, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);

        // Path column
        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Path", renderer,
            "text", 7, NULL);
        gtk_tree_view_column_set_resizable(column, TRUE);
        gtk_tree_view_column_set_expand(column, TRUE);
        gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);

        gtk_container_add(GTK_CONTAINER(scrolled), tree_view);
        gtk_box_pack_start(GTK_BOX(vbox), scrolled, TRUE, TRUE, 0);
    } else {
        GtkWidget *no_results = gtk_label_new("No files found matching the search pattern.");
        gtk_box_pack_start(GTK_BOX(vbox), no_results, TRUE, TRUE, 5);
    }

    gtk_widget_show_all(content_area);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

void on_search_clicked(GtkWidget *widget, AppState *state) {
    (void)widget;  // Unused parameter

    const char *pattern = gtk_entry_get_text(GTK_ENTRY(state->search_entry));

    if (!pattern || strlen(pattern) == 0) {
        show_error_dialog(state->window, "Please enter a search pattern.");
        return;
    }

    // Get recursive flag
    int recursive = gtk_toggle_button_get_active(
        GTK_TOGGLE_BUTTON(state->search_recursive_check));

    // Call client search function
    cJSON *results = (cJSON*)client_search(state->conn, pattern, recursive, 100);

    if (results) {
        // Clear current file list
        gtk_list_store_clear(state->file_store);

        // Get results array (note: server returns "results" not "files")
        cJSON *results_array = cJSON_GetObjectItem(results, "results");
        cJSON *count_obj = cJSON_GetObjectItem(results, "count");
        int count = count_obj ? count_obj->valueint : 0;

        if (results_array && cJSON_IsArray(results_array)) {
            // Populate file list with search results
            cJSON *file;
            cJSON_ArrayForEach(file, results_array) {
                int id = cJSON_GetObjectItem(file, "id")->valueint;
                int is_dir = cJSON_GetObjectItem(file, "is_directory")->valueint;
                const char *name = cJSON_GetStringValue(cJSON_GetObjectItem(file, "name"));
                int size = cJSON_GetObjectItem(file, "size")->valueint;
                int perms = cJSON_GetObjectItem(file, "permissions")->valueint;

                // Get owner
                const char *owner = "unknown";
                cJSON *owner_obj = cJSON_GetObjectItem(file, "owner");
                if (owner_obj) {
                    owner = cJSON_GetStringValue(owner_obj);
                }

                GtkTreeIter iter;
                gtk_list_store_append(state->file_store, &iter);
                gtk_list_store_set(state->file_store, &iter,
                    0, id,
                    1, is_dir ? "folder" : "text-x-generic",
                    2, name,
                    3, is_dir ? "Directory" : "File",
                    4, owner,
                    5, is_dir ? 0 : size,
                    6, g_strdup_printf("%03o", perms),
                    -1);
            }
        }

        // Update status bar with search results count
        guint context_id = gtk_statusbar_get_context_id(
            GTK_STATUSBAR(state->status_bar), "status");
        char status[256];
        snprintf(status, sizeof(status), "Search: %d file(s) found for '%s'%s",
                 count, pattern, recursive ? " (recursive)" : "");
        gtk_statusbar_push(GTK_STATUSBAR(state->status_bar), context_id, status);

        cJSON_Delete(results);
    } else {
        show_error_dialog(state->window, "Search failed. Please try again.");
    }
}

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
