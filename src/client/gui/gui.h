#ifndef GUI_H
#define GUI_H

#include <gtk/gtk.h>
#include "../client.h"

// GUI application state
typedef struct {
    GtkWidget *window;
    GtkWidget *tree_view;
    GtkListStore *file_store;
    GtkWidget *status_bar;
    ClientConnection *conn;
    int current_directory;
    char current_path[512];
} AppState;

// GUI initialization and main
GtkWidget* create_login_dialog(void);
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

#endif // GUI_H
