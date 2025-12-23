#include "gui.h"
#include "admin_dashboard.h"
#include "../net_handler.h"
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    // Show login dialog
    GtkWidget *login_dialog = create_login_dialog();
    gint response = gtk_dialog_run(GTK_DIALOG(login_dialog));

    if (response != GTK_RESPONSE_OK) {
        gtk_widget_destroy(login_dialog);
        return 0;
    }

    // Get connection details
    GtkWidget *server_entry = g_object_get_data(G_OBJECT(login_dialog), "server_entry");
    GtkWidget *port_entry = g_object_get_data(G_OBJECT(login_dialog), "port_entry");
    GtkWidget *username_entry = g_object_get_data(G_OBJECT(login_dialog), "username_entry");
    GtkWidget *password_entry = g_object_get_data(G_OBJECT(login_dialog), "password_entry");

    const char *server = gtk_entry_get_text(GTK_ENTRY(server_entry));
    const char *port_str = gtk_entry_get_text(GTK_ENTRY(port_entry));
    const char *username = gtk_entry_get_text(GTK_ENTRY(username_entry));
    const char *password = gtk_entry_get_text(GTK_ENTRY(password_entry));

    int port = atoi(port_str);

    // Connect to server
    ClientConnection *conn = client_connect(server, port);
    if (!conn) {
        show_error_dialog(NULL, "Failed to connect to server");
        gtk_widget_destroy(login_dialog);
        return 1;
    }

    // Login
    if (client_login(conn, username, password) < 0) {
        show_error_dialog(NULL, "Login failed. Invalid credentials.");
        client_disconnect(conn);
        gtk_widget_destroy(login_dialog);
        return 1;
    }

    gtk_widget_destroy(login_dialog);

    // Check if user is admin and route accordingly
    if (conn->is_admin) {
        // Show admin dashboard
        create_admin_dashboard(conn);
        gtk_main();
    } else {
        // Create regular file browser window
        AppState *state = g_new0(AppState, 1);
        state->conn = conn;
        state->current_directory = 0;
        strcpy(state->current_path, "/");

        state->window = create_main_window(state);
        gtk_widget_show_all(state->window);

        // Load initial directory
        refresh_file_list(state);

        gtk_main();

        // Cleanup
        g_free(state);
    }

    // Final cleanup (connection closed in window destroy handlers)
    return 0;
}
