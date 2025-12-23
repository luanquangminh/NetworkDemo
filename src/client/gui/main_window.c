#include "gui.h"

static void on_quit_activate(GtkWidget *widget, AppState *state) {
    gtk_main_quit();
}

GtkWidget* create_main_window(AppState *state) {
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "File Sharing Client");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // Main vertical box
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    // Menu bar
    GtkWidget *menubar = gtk_menu_bar_new();

    // File menu
    GtkWidget *file_menu = gtk_menu_new();
    GtkWidget *file_item = gtk_menu_item_new_with_label("File");
    GtkWidget *quit_item = gtk_menu_item_new_with_label("Quit");
    g_signal_connect(quit_item, "activate", G_CALLBACK(on_quit_activate), state);
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), quit_item);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(file_item), file_menu);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), file_item);

    gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);

    // Toolbar
    GtkWidget *toolbar = gtk_toolbar_new();
    gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_BOTH);

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

    gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 0);

    // Scrolled window for file list
    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);

    // File list (tree view)
    state->file_store = gtk_list_store_new(6,
        G_TYPE_INT,      // ID
        G_TYPE_STRING,   // Icon
        G_TYPE_STRING,   // Name
        G_TYPE_STRING,   // Type
        G_TYPE_INT,      // Size
        G_TYPE_STRING    // Permissions
    );

    state->tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(state->file_store));
    g_signal_connect(state->tree_view, "row-activated",
                    G_CALLBACK(on_row_activated), state);

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

    // Size column
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Size", renderer,
        "text", 4, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(state->tree_view), column);

    // Permissions column
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Permissions", renderer,
        "text", 5, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(state->tree_view), column);

    gtk_container_add(GTK_CONTAINER(scrolled), state->tree_view);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled, TRUE, TRUE, 0);

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
