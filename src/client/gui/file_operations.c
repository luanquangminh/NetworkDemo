#include "gui.h"
#include <string.h>
#include "cJSON.h"

void refresh_file_list(AppState *state) {
    gtk_list_store_clear(state->file_store);

    // Get file list from server
    cJSON* resp_json = (cJSON*)client_list_dir_gui(state->conn, state->current_directory);
    if (!resp_json) {
        show_error_dialog(state->window, "Failed to list directory");
        return;
    }

    cJSON* files = cJSON_GetObjectItem(resp_json, "files");
    if (files) {
        cJSON* file;
        cJSON_ArrayForEach(file, files) {
            int id = cJSON_GetObjectItem(file, "id")->valueint;
            int is_dir = cJSON_GetObjectItem(file, "is_directory")->valueint;
            const char* name = cJSON_GetStringValue(cJSON_GetObjectItem(file, "name"));
            int size = cJSON_GetObjectItem(file, "size")->valueint;
            int perms = cJSON_GetObjectItem(file, "permissions")->valueint;

            GtkTreeIter iter;
            gtk_list_store_append(state->file_store, &iter);
            gtk_list_store_set(state->file_store, &iter,
                0, id,
                1, is_dir ? "folder" : "text-x-generic",  // Icon name
                2, name,
                3, is_dir ? "Directory" : "File",
                4, is_dir ? 0 : size,
                5, g_strdup_printf("%03o", perms),
                -1);
        }
    }

    cJSON_Delete(resp_json);
}

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
            }
        }

        g_free(type);
    }
}

void on_upload_clicked(GtkWidget *widget, AppState *state) {
    GtkWidget *dialog = gtk_file_chooser_dialog_new(
        "Upload File",
        GTK_WINDOW(state->window),
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Upload", GTK_RESPONSE_ACCEPT,
        NULL
    );

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

        if (client_upload(state->conn, filename) == 0) {
            show_info_dialog(state->window, "File uploaded successfully!");
            refresh_file_list(state);
        } else {
            show_error_dialog(state->window, "Upload failed");
        }

        g_free(filename);
    }

    gtk_widget_destroy(dialog);
}

void on_download_clicked(GtkWidget *widget, AppState *state) {
    GtkTreeSelection *selection = gtk_tree_view_get_selection(
        GTK_TREE_VIEW(state->tree_view));
    GtkTreeModel *model;
    GtkTreeIter iter;

    if (!gtk_tree_selection_get_selected(selection, &model, &iter)) {
        show_error_dialog(state->window, "Please select a file to download");
        return;
    }

    gint file_id;
    gchar *name;
    gtk_tree_model_get(model, &iter, 0, &file_id, 2, &name, -1);

    GtkWidget *dialog = gtk_file_chooser_dialog_new(
        "Save File",
        GTK_WINDOW(state->window),
        GTK_FILE_CHOOSER_ACTION_SAVE,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Save", GTK_RESPONSE_ACCEPT,
        NULL
    );

    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), name);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *save_path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

        if (client_download(state->conn, file_id, save_path) == 0) {
            show_info_dialog(state->window, "File downloaded successfully!");
        } else {
            show_error_dialog(state->window, "Download failed");
        }

        g_free(save_path);
    }

    g_free(name);
    gtk_widget_destroy(dialog);
}

void on_mkdir_clicked(GtkWidget *widget, AppState *state) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "Create Directory",
        GTK_WINDOW(state->window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Create", GTK_RESPONSE_OK,
        NULL
    );

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Directory name");
    gtk_container_add(GTK_CONTAINER(content), entry);
    gtk_widget_show_all(content);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        const char *name = gtk_entry_get_text(GTK_ENTRY(entry));

        if (strlen(name) > 0) {
            if (client_mkdir(state->conn, name) == 0) {
                show_info_dialog(state->window, "Directory created successfully!");
                refresh_file_list(state);
            } else {
                show_error_dialog(state->window, "Failed to create directory");
            }
        }
    }

    gtk_widget_destroy(dialog);
}

void on_delete_clicked(GtkWidget *widget, AppState *state) {
    (void)widget;  // Suppress unused warning

    GtkTreeSelection *selection = gtk_tree_view_get_selection(
        GTK_TREE_VIEW(state->tree_view));
    GtkTreeModel *model;
    GtkTreeIter iter;

    if (!gtk_tree_selection_get_selected(selection, &model, &iter)) {
        show_error_dialog(state->window, "Please select a file to delete");
        return;
    }

    gint file_id;
    gchar *name;
    gtk_tree_model_get(model, &iter, 0, &file_id, 2, &name, -1);

    // Confirmation dialog
    GtkWidget *dialog = gtk_message_dialog_new(
        GTK_WINDOW(state->window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_QUESTION,
        GTK_BUTTONS_YES_NO,
        "Delete '%s'?", name
    );
    gtk_message_dialog_format_secondary_text(
        GTK_MESSAGE_DIALOG(dialog),
        "This action cannot be undone.");

    gint response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    if (response == GTK_RESPONSE_YES) {
        if (client_delete(state->conn, file_id) == 0) {
            show_info_dialog(state->window, "File deleted successfully!");
            refresh_file_list(state);
        } else {
            show_error_dialog(state->window, "Failed to delete file");
        }
    }

    g_free(name);
}

void on_chmod_clicked(GtkWidget *widget, AppState *state) {
    GtkTreeSelection *selection = gtk_tree_view_get_selection(
        GTK_TREE_VIEW(state->tree_view));
    GtkTreeModel *model;
    GtkTreeIter iter;

    if (!gtk_tree_selection_get_selected(selection, &model, &iter)) {
        show_error_dialog(state->window, "Please select a file");
        return;
    }

    gint file_id;
    gchar *perms_str;
    gtk_tree_model_get(model, &iter, 0, &file_id, 5, &perms_str, -1);

    // Parse current permissions (e.g., "755")
    int current_perms = strtol(perms_str, NULL, 8);
    g_free(perms_str);

    GtkWidget *chmod_dialog = create_chmod_dialog(state->window, current_perms);

    if (gtk_dialog_run(GTK_DIALOG(chmod_dialog)) == GTK_RESPONSE_OK) {
        GtkWidget *entry = g_object_get_data(G_OBJECT(chmod_dialog), "perms_entry");
        const char *new_perms_str = gtk_entry_get_text(GTK_ENTRY(entry));
        int new_perms = strtol(new_perms_str, NULL, 8);

        if (client_chmod(state->conn, file_id, new_perms) == 0) {
            show_info_dialog(state->window, "Permissions changed successfully!");
            refresh_file_list(state);
        } else {
            show_error_dialog(state->window, "Failed to change permissions");
        }
    }

    gtk_widget_destroy(chmod_dialog);
}
