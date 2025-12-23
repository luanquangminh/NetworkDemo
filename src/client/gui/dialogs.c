#include "gui.h"

void show_error_dialog(GtkWidget *parent, const char *message) {
    GtkWidget *dialog = gtk_message_dialog_new(
        parent ? GTK_WINDOW(parent) : NULL,
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_ERROR,
        GTK_BUTTONS_OK,
        "%s", message
    );

    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

void show_info_dialog(GtkWidget *parent, const char *message) {
    GtkWidget *dialog = gtk_message_dialog_new(
        parent ? GTK_WINDOW(parent) : NULL,
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_INFO,
        GTK_BUTTONS_OK,
        "%s", message
    );

    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

GtkWidget* create_progress_dialog(GtkWidget *parent, const char *title) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        title,
        parent ? GTK_WINDOW(parent) : NULL,
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Cancel", GTK_RESPONSE_CANCEL,
        NULL
    );

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

    GtkWidget *label = gtk_label_new("Progress:");
    gtk_container_add(GTK_CONTAINER(content), label);

    GtkWidget *progress_bar = gtk_progress_bar_new();
    gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(progress_bar), TRUE);
    gtk_container_add(GTK_CONTAINER(content), progress_bar);

    g_object_set_data(G_OBJECT(dialog), "progress_bar", progress_bar);

    gtk_widget_show_all(content);
    return dialog;
}

GtkWidget* create_chmod_dialog(GtkWidget *parent, int current_perms) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "Change Permissions",
        parent ? GTK_WINDOW(parent) : NULL,
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Apply", GTK_RESPONSE_OK,
        NULL
    );

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 20);
    gtk_container_add(GTK_CONTAINER(content), grid);

    GtkWidget *label = gtk_label_new("Permissions (octal):");
    gtk_widget_set_halign(label, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);

    GtkWidget *entry = gtk_entry_new();
    char perms_str[8];
    snprintf(perms_str, sizeof(perms_str), "%o", current_perms);
    gtk_entry_set_text(GTK_ENTRY(entry), perms_str);
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "e.g., 755, 644");
    gtk_grid_attach(GTK_GRID(grid), entry, 1, 0, 1, 1);

    GtkWidget *help_label = gtk_label_new(
        "Examples:\n"
        "755 = rwxr-xr-x (owner: rwx, group: r-x, others: r-x)\n"
        "644 = rw-r--r-- (owner: rw-, group: r--, others: r--)"
    );
    gtk_label_set_line_wrap(GTK_LABEL(help_label), TRUE);
    gtk_grid_attach(GTK_GRID(grid), help_label, 0, 1, 2, 1);

    g_object_set_data(G_OBJECT(dialog), "perms_entry", entry);

    gtk_widget_show_all(content);
    return dialog;
}
