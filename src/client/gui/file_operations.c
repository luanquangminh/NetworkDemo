#include "gui.h"
#include <string.h>
#include <stdlib.h>
#include "cJSON.h"

#define HISTORY_INITIAL_CAPACITY 10
#define HISTORY_MAX_CAPACITY 50

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

            // Extract owner from JSON response
            const char* owner = "unknown";
            cJSON* owner_obj = cJSON_GetObjectItem(file, "owner");
            if (owner_obj) {
                owner = cJSON_GetStringValue(owner_obj);
            }

            GtkTreeIter iter;
            gtk_list_store_append(state->file_store, &iter);
            gtk_list_store_set(state->file_store, &iter,
                0, id,
                1, is_dir ? "folder" : "text-x-generic",  // Icon name
                2, name,
                3, is_dir ? "Directory" : "File",
                4, owner,
                5, is_dir ? 0 : size,
                6, g_strdup_printf("%03o", perms),
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
            // Save current directory to history BEFORE navigation
            history_push(&state->history, state->current_directory, state->current_path);

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

                // Enable back button
                gtk_widget_set_sensitive(state->back_button, TRUE);
            } else {
                // Navigation failed, remove history entry
                int dummy_id;
                char dummy_path[512];
                history_pop(&state->history, &dummy_id, dummy_path);
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
    gtk_tree_model_get(model, &iter, 0, &file_id, 6, &perms_str, -1);

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

// History management functions
// Rename file/directory handler
void on_rename_clicked(GtkWidget *widget, AppState *state) {
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(state->tree_view));
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

// Copy file/directory handler
void on_copy_clicked(GtkWidget *widget, AppState *state) {
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(state->tree_view));
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

    // Note: Copying to current directory
    GtkWidget *note = gtk_label_new("(Will copy to current directory)");

    gtk_box_pack_start(GTK_BOX(content), name_label, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(content), name_entry, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(content), note, FALSE, FALSE, 5);
    gtk_widget_show_all(content);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        const char *new_name = gtk_entry_get_text(GTK_ENTRY(name_entry));

        if (strlen(new_name) > 0) {
            if (client_copy(state->conn, source_id, state->current_directory, new_name) == 0) {
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

void history_init(DirectoryHistory *history) {
    history->entries = malloc(HISTORY_INITIAL_CAPACITY * sizeof(DirectoryHistoryEntry));
    history->count = 0;
    history->capacity = HISTORY_INITIAL_CAPACITY;
}

void history_free(DirectoryHistory *history) {
    if (history->entries) {
        free(history->entries);
        history->entries = NULL;
    }
    history->count = 0;
    history->capacity = 0;
}

void history_push(DirectoryHistory *history, int dir_id, const char *path) {
    // Check if at max capacity - remove oldest if needed
    if (history->count >= HISTORY_MAX_CAPACITY) {
        // Shift all entries down (remove oldest)
        memmove(&history->entries[0], &history->entries[1],
                (HISTORY_MAX_CAPACITY - 1) * sizeof(DirectoryHistoryEntry));
        history->count = HISTORY_MAX_CAPACITY - 1;
    }

    // Grow array if needed
    if (history->count >= history->capacity) {
        history->capacity *= 2;
        if (history->capacity > HISTORY_MAX_CAPACITY) {
            history->capacity = HISTORY_MAX_CAPACITY;
        }
        history->entries = realloc(history->entries,
                                   history->capacity * sizeof(DirectoryHistoryEntry));
    }

    // Add new entry
    history->entries[history->count].directory_id = dir_id;
    strncpy(history->entries[history->count].path, path, 511);
    history->entries[history->count].path[511] = '\0';
    history->count++;
}

int history_pop(DirectoryHistory *history, int *dir_id, char *path) {
    if (history->count == 0) {
        return -1;  // Empty stack
    }

    history->count--;
    *dir_id = history->entries[history->count].directory_id;
    strcpy(path, history->entries[history->count].path);

    return 0;  // Success
}

int history_is_empty(DirectoryHistory *history) {
    return history->count == 0;
}

void history_clear(DirectoryHistory *history) {
    history->count = 0;
}
