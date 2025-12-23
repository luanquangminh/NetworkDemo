#include "gui.h"

GtkWidget* create_login_dialog(void) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "Login to File Sharing Server",
        NULL,
        GTK_DIALOG_MODAL,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Login", GTK_RESPONSE_OK,
        NULL
    );

    gtk_window_set_default_size(GTK_WINDOW(dialog), 400, 250);

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 20);
    gtk_container_add(GTK_CONTAINER(content), grid);

    // Server label and entry
    GtkWidget *server_label = gtk_label_new("Server:");
    gtk_widget_set_halign(server_label, GTK_ALIGN_END);
    GtkWidget *server_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(server_entry), "localhost");
    gtk_grid_attach(GTK_GRID(grid), server_label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), server_entry, 1, 0, 1, 1);

    // Port label and entry
    GtkWidget *port_label = gtk_label_new("Port:");
    gtk_widget_set_halign(port_label, GTK_ALIGN_END);
    GtkWidget *port_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(port_entry), "8080");
    gtk_grid_attach(GTK_GRID(grid), port_label, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), port_entry, 1, 1, 1, 1);

    // Username label and entry
    GtkWidget *username_label = gtk_label_new("Username:");
    gtk_widget_set_halign(username_label, GTK_ALIGN_END);
    GtkWidget *username_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(username_entry), "admin");
    gtk_grid_attach(GTK_GRID(grid), username_label, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), username_entry, 1, 2, 1, 1);

    // Password label and entry
    GtkWidget *password_label = gtk_label_new("Password:");
    gtk_widget_set_halign(password_label, GTK_ALIGN_END);
    GtkWidget *password_entry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(password_entry), FALSE);
    gtk_entry_set_text(GTK_ENTRY(password_entry), "admin");
    gtk_grid_attach(GTK_GRID(grid), password_label, 0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), password_entry, 1, 3, 1, 1);

    // Store entry widgets for retrieval
    g_object_set_data(G_OBJECT(dialog), "server_entry", server_entry);
    g_object_set_data(G_OBJECT(dialog), "port_entry", port_entry);
    g_object_set_data(G_OBJECT(dialog), "username_entry", username_entry);
    g_object_set_data(G_OBJECT(dialog), "password_entry", password_entry);

    gtk_widget_show_all(content);
    return dialog;
}
