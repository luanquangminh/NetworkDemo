#include "src/client/client.h"
#include <stdio.h>

int main() {
    ClientConnection* conn = client_connect("localhost", 8080);
    if (!conn) {
        printf("Connection failed\n");
        return 1;
    }

    printf("Connected successfully\n");

    int result = client_login(conn, "admin", "admin");
    if (result == 0) {
        printf("Login SUCCESS! User ID: %d, is_admin: %d\n", conn->user_id, conn->is_admin);
    } else {
        printf("Login FAILED\n");
    }

    client_disconnect(conn);
    return (result == 0) ? 0 : 1;
}
