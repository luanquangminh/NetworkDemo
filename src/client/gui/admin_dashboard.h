#ifndef ADMIN_DASHBOARD_H
#define ADMIN_DASHBOARD_H

#include <gtk/gtk.h>
#include "../client.h"

typedef struct {
    GtkWidget *window;
    GtkWidget *tree_view;
    GtkListStore *user_store;
    GtkWidget *status_bar;
    ClientConnection *conn;
} AdminState;

// Create and show admin dashboard
GtkWidget* create_admin_dashboard(ClientConnection *conn);

// Refresh user list from server
void refresh_user_list(AdminState *state);

#endif // ADMIN_DASHBOARD_H
