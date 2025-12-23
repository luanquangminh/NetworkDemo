#include "admin_dashboard.h"
#include "gui.h"
#include "../../lib/cJSON/cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Column indices for user list
enum {
    COL_USER_ID = 0,
    COL_USERNAME,
    COL_IS_ACTIVE,
    COL_IS_ADMIN,
    COL_CREATED_AT,
    NUM_COLS
};

// Forward declarations
static void on_create_user_clicked(GtkWidget *widget, AdminState *state);
static void on_delete_user_clicked(GtkWidget *widget, AdminState *state);
static void on_refresh_clicked(GtkWidget *widget, AdminState *state);
static void on_user_row_activated(GtkTreeView *tree_view, GtkTreePath *path,
                                   GtkTreeViewColumn *column, AdminState *state);
static void on_window_destroy(GtkWidget *widget, AdminState *state);

// Refresh user list from server
void refresh_user_list(AdminState *state) {
    if (!state || !state->conn) return;

    // Clear existing list
    gtk_list_store_clear(state->user_store);

    // Get users from server
    cJSON* response = client_admin_list_users(state->conn);
    if (!response) {
        show_error_dialog(state->window, "Failed to retrieve user list");
        return;
    }

    cJSON* status = cJSON_GetObjectItem(response, "status");
    if (!status || strcmp(cJSON_GetStringValue(status), "OK") != 0) {
        show_error_dialog(state->window, "Error retrieving users");
        cJSON_Delete(response);
        return;
    }

    cJSON* users = cJSON_GetObjectItem(response, "users");
    if (!users || !cJSON_IsArray(users)) {
        cJSON_Delete(response);
        return;
    }

    int user_count = cJSON_GetArraySize(users);

    // Populate list store
    for (int i = 0; i < user_count; i++) {
        cJSON* user = cJSON_GetArrayItem(users, i);
        if (!user) continue;

        cJSON* id = cJSON_GetObjectItem(user, "id");
        cJSON* username = cJSON_GetObjectItem(user, "username");
        cJSON* is_active = cJSON_GetObjectItem(user, "is_active");
        cJSON* is_admin = cJSON_GetObjectItem(user, "is_admin");
        cJSON* created_at = cJSON_GetObjectItem(user, "created_at");

        GtkTreeIter iter;
        gtk_list_store_append(state->user_store, &iter);
        gtk_list_store_set(state->user_store, &iter,
                          COL_USER_ID, id ? id->valueint : 0,
                          COL_USERNAME, username ? cJSON_GetStringValue(username) : "",
                          COL_IS_ACTIVE, is_active ? (is_active->valueint ? "Yes" : "No") : "No",
                          COL_IS_ADMIN, is_admin ? (is_admin->valueint ? "Yes" : "No") : "No",
                          COL_CREATED_AT, created_at ? cJSON_GetStringValue(created_at) : "",
                          -1);
    }

    // Update status bar
    char status_text[256];
    snprintf(status_text, sizeof(status_text), "Total users: %d", user_count);
    gtk_statusbar_pop(GTK_STATUSBAR(state->status_bar), 0);
    gtk_statusbar_push(GTK_STATUSBAR(state->status_bar), 0, status_text);

    cJSON_Delete(response);
}

// Callback: Create user button clicked
static void on_create_user_clicked(GtkWidget *widget, AdminState *state) {
    // Show create user dialog
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "Create New User",
        GTK_WINDOW(state->window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "Cancel", GTK_RESPONSE_CANCEL,
        "Create", GTK_RESPONSE_OK,
        NULL
    );

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 5);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 5);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 10);
    gtk_container_add(GTK_CONTAINER(content), grid);

    // Username field
    GtkWidget *username_label = gtk_label_new("Username:");
    GtkWidget *username_entry = gtk_entry_new();
    gtk_grid_attach(GTK_GRID(grid), username_label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), username_entry, 1, 0, 1, 1);

    // Password field
    GtkWidget *password_label = gtk_label_new("Password:");
    GtkWidget *password_entry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(password_entry), FALSE);
    gtk_grid_attach(GTK_GRID(grid), password_label, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), password_entry, 1, 1, 1, 1);

    // Admin checkbox
    GtkWidget *admin_check = gtk_check_button_new_with_label("Admin User");
    gtk_grid_attach(GTK_GRID(grid), admin_check, 0, 2, 2, 1);

    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        const char *username = gtk_entry_get_text(GTK_ENTRY(username_entry));
        const char *password = gtk_entry_get_text(GTK_ENTRY(password_entry));
        int is_admin = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(admin_check)) ? 1 : 0;

        if (strlen(username) < 3) {
            show_error_dialog(state->window, "Username must be at least 3 characters");
        } else if (strlen(password) < 4) {
            show_error_dialog(state->window, "Password must be at least 4 characters");
        } else {
            int user_id = client_admin_create_user(state->conn, username, password, is_admin);
            if (user_id > 0) {
                show_info_dialog(state->window, "User created successfully");
                refresh_user_list(state);
            } else {
                show_error_dialog(state->window, "Failed to create user");
            }
        }
    }

    gtk_widget_destroy(dialog);
}

// Callback: Delete user button clicked
static void on_delete_user_clicked(GtkWidget *widget, AdminState *state) {
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(state->tree_view));
    GtkTreeIter iter;
    GtkTreeModel *model;

    if (!gtk_tree_selection_get_selected(selection, &model, &iter)) {
        show_error_dialog(state->window, "Please select a user to delete");
        return;
    }

    int user_id;
    char *username;
    gtk_tree_model_get(model, &iter,
                       COL_USER_ID, &user_id,
                       COL_USERNAME, &username,
                       -1);

    // Confirmation dialog
    char confirm_msg[256];
    snprintf(confirm_msg, sizeof(confirm_msg),
             "Are you sure you want to delete user '%s' (ID: %d)?", username, user_id);

    GtkWidget *confirm_dialog = gtk_message_dialog_new(
        GTK_WINDOW(state->window),
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_QUESTION,
        GTK_BUTTONS_YES_NO,
        "%s", confirm_msg
    );

    int response = gtk_dialog_run(GTK_DIALOG(confirm_dialog));
    gtk_widget_destroy(confirm_dialog);

    if (response == GTK_RESPONSE_YES) {
        if (client_admin_delete_user(state->conn, user_id) == 0) {
            show_info_dialog(state->window, "User deleted successfully");
            refresh_user_list(state);
        } else {
            show_error_dialog(state->window, "Failed to delete user");
        }
    }

    g_free(username);
}

// Callback: Refresh button clicked
static void on_refresh_clicked(GtkWidget *widget, AdminState *state) {
    refresh_user_list(state);
}

// Callback: User row double-clicked (edit user)
static void on_user_row_activated(GtkTreeView *tree_view, GtkTreePath *path,
                                   GtkTreeViewColumn *column, AdminState *state) {
    GtkTreeModel *model = gtk_tree_view_get_model(tree_view);
    GtkTreeIter iter;

    if (!gtk_tree_model_get_iter(model, &iter, path)) return;

    int user_id;
    char *username;
    char *is_active_str;
    char *is_admin_str;

    gtk_tree_model_get(model, &iter,
                       COL_USER_ID, &user_id,
                       COL_USERNAME, &username,
                       COL_IS_ACTIVE, &is_active_str,
                       COL_IS_ADMIN, &is_admin_str,
                       -1);

    int current_is_active = (strcmp(is_active_str, "Yes") == 0) ? 1 : 0;
    int current_is_admin = (strcmp(is_admin_str, "Yes") == 0) ? 1 : 0;

    // Show edit dialog
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "Edit User",
        GTK_WINDOW(state->window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "Cancel", GTK_RESPONSE_CANCEL,
        "Save", GTK_RESPONSE_OK,
        NULL
    );

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 5);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 5);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 10);
    gtk_container_add(GTK_CONTAINER(content), grid);

    // Username label (read-only)
    GtkWidget *username_label = gtk_label_new("Username:");
    GtkWidget *username_value = gtk_label_new(username);
    gtk_grid_attach(GTK_GRID(grid), username_label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), username_value, 1, 0, 1, 1);

    // Active checkbox
    GtkWidget *active_check = gtk_check_button_new_with_label("Active");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(active_check), current_is_active);
    gtk_grid_attach(GTK_GRID(grid), active_check, 0, 1, 2, 1);

    // Admin checkbox
    GtkWidget *admin_check = gtk_check_button_new_with_label("Admin");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(admin_check), current_is_admin);
    gtk_grid_attach(GTK_GRID(grid), admin_check, 0, 2, 2, 1);

    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        int new_is_active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(active_check)) ? 1 : 0;
        int new_is_admin = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(admin_check)) ? 1 : 0;

        if (client_admin_update_user(state->conn, user_id, new_is_admin, new_is_active) == 0) {
            show_info_dialog(state->window, "User updated successfully");
            refresh_user_list(state);
        } else {
            show_error_dialog(state->window, "Failed to update user");
        }
    }

    gtk_widget_destroy(dialog);
    g_free(username);
    g_free(is_active_str);
    g_free(is_admin_str);
}

// Callback: Window destroyed
static void on_window_destroy(GtkWidget *widget, AdminState *state) {
    if (state->conn) {
        client_disconnect(state->conn);
    }
    g_free(state);
    gtk_main_quit();
}

// Create admin dashboard window
GtkWidget* create_admin_dashboard(ClientConnection *conn) {
    AdminState *state = g_new0(AdminState, 1);
    state->conn = conn;

    // Create main window
    state->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(state->window), "Admin Dashboard - User Management");
    gtk_window_set_default_size(GTK_WINDOW(state->window), 800, 600);
    g_signal_connect(state->window, "destroy", G_CALLBACK(on_window_destroy), state);

    // Main container
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(state->window), vbox);

    // Toolbar
    GtkWidget *toolbar = gtk_toolbar_new();
    gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 0);

    GtkToolItem *create_btn = gtk_tool_button_new(NULL, "Create User");
    gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(create_btn), "list-add");
    g_signal_connect(create_btn, "clicked", G_CALLBACK(on_create_user_clicked), state);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), create_btn, -1);

    GtkToolItem *delete_btn = gtk_tool_button_new(NULL, "Delete User");
    gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(delete_btn), "list-remove");
    g_signal_connect(delete_btn, "clicked", G_CALLBACK(on_delete_user_clicked), state);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), delete_btn, -1);

    GtkToolItem *refresh_btn = gtk_tool_button_new(NULL, "Refresh");
    gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(refresh_btn), "view-refresh");
    g_signal_connect(refresh_btn, "clicked", G_CALLBACK(on_refresh_clicked), state);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), refresh_btn, -1);

    // User list tree view
    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled, TRUE, TRUE, 0);

    state->user_store = gtk_list_store_new(NUM_COLS,
                                            G_TYPE_INT,    // ID
                                            G_TYPE_STRING, // Username
                                            G_TYPE_STRING, // Active
                                            G_TYPE_STRING, // Admin
                                            G_TYPE_STRING  // Created At
                                           );

    state->tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(state->user_store));
    gtk_container_add(GTK_CONTAINER(scrolled), state->tree_view);
    g_signal_connect(state->tree_view, "row-activated", G_CALLBACK(on_user_row_activated), state);

    // Columns
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();

    GtkTreeViewColumn *col_id = gtk_tree_view_column_new_with_attributes(
        "ID", renderer, "text", COL_USER_ID, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(state->tree_view), col_id);

    GtkTreeViewColumn *col_username = gtk_tree_view_column_new_with_attributes(
        "Username", renderer, "text", COL_USERNAME, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(state->tree_view), col_username);

    GtkTreeViewColumn *col_active = gtk_tree_view_column_new_with_attributes(
        "Active", renderer, "text", COL_IS_ACTIVE, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(state->tree_view), col_active);

    GtkTreeViewColumn *col_admin = gtk_tree_view_column_new_with_attributes(
        "Admin", renderer, "text", COL_IS_ADMIN, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(state->tree_view), col_admin);

    GtkTreeViewColumn *col_created = gtk_tree_view_column_new_with_attributes(
        "Created At", renderer, "text", COL_CREATED_AT, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(state->tree_view), col_created);

    // Status bar
    state->status_bar = gtk_statusbar_new();
    gtk_box_pack_start(GTK_BOX(vbox), state->status_bar, FALSE, FALSE, 0);
    gtk_statusbar_push(GTK_STATUSBAR(state->status_bar), 0, "Ready");

    // Initial refresh
    refresh_user_list(state);

    gtk_widget_show_all(state->window);
    return state->window;
}
