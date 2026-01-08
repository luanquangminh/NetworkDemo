#ifndef GUI_H
#define GUI_H

#include <gtk/gtk.h>
#include "../client.h"
#include "../../../lib/cJSON/cJSON.h"

// Directory history entry for back navigation
typedef struct {
    int directory_id;
    char path[512];
} DirectoryHistoryEntry;

// Directory history stack for back navigation
typedef struct {
    DirectoryHistoryEntry *entries;
    int count;
    int capacity;
} DirectoryHistory;

// GUI application state
typedef struct {
    GtkWidget *window;
    GtkWidget *tree_view;
    GtkListStore *file_store;
    GtkWidget *status_bar;
    GtkWidget *search_entry;
    GtkWidget *search_recursive_check;
    GtkWidget *back_button;
    DirectoryHistory history;
    ClientConnection *conn;
    int current_directory;
    char current_path[512];
} AppState;

// Login result structure
typedef struct {
    ClientConnection *conn;
    int is_cancelled;
} LoginResult;

// GUI initialization and main
GtkWidget* create_login_dialog(void);
LoginResult* perform_login(GtkWidget *parent_window);
void login_result_free(LoginResult *result);
GtkWidget* create_main_window(AppState *state);

// Dialogs
void show_error_dialog(GtkWidget *parent, const char *message);
void show_info_dialog(GtkWidget *parent, const char *message);
GtkWidget* create_progress_dialog(GtkWidget *parent, const char *title);
GtkWidget* create_chmod_dialog(GtkWidget *parent, int current_perms);

// File operations
void refresh_file_list(AppState *state);
void on_upload_clicked(GtkWidget *widget, AppState *state);
void on_download_clicked(GtkWidget *widget, AppState *state);
void on_mkdir_clicked(GtkWidget *widget, AppState *state);
void on_delete_clicked(GtkWidget *widget, AppState *state);
void on_chmod_clicked(GtkWidget *widget, AppState *state);
void on_row_activated(GtkTreeView *tree_view, GtkTreePath *path,
                     GtkTreeViewColumn *column, AppState *state);

// Search operations
void on_search_clicked(GtkWidget *widget, AppState *state);
void show_search_results_dialog(GtkWidget *parent, cJSON *results, const char *pattern, int recursive);

// History management functions
void history_init(DirectoryHistory *history);
void history_free(DirectoryHistory *history);
void history_push(DirectoryHistory *history, int dir_id, const char *path);
int history_pop(DirectoryHistory *history, int *dir_id, char *path);
int history_is_empty(DirectoryHistory *history);
void history_clear(DirectoryHistory *history);

#endif // GUI_H
