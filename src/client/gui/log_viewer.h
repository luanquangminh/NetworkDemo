#ifndef LOG_VIEWER_H
#define LOG_VIEWER_H

#include <gtk/gtk.h>
#include "../client.h"

// Log viewer state
typedef struct {
    GtkWidget *window;
    GtkWidget *tree_view;
    GtkListStore *log_store;
    GtkWidget *status_bar;
    GtkWidget *filter_expander;

    // Filter widgets
    GtkWidget *user_combo;
    GtkWidget *action_combo;
    GtkWidget *start_date_entry;
    GtkWidget *end_date_entry;
    GtkWidget *limit_spin;

    ClientConnection *conn;

    // Current filter state
    int current_user_filter;
    char current_action_filter[64];
    char current_start_date[32];
    char current_end_date[32];
    int current_limit;
} LogViewerState;

// Create log viewer window
GtkWidget* create_log_viewer(ClientConnection *conn);

// Refresh logs from server
void refresh_logs(LogViewerState *state);

#endif // LOG_VIEWER_H
