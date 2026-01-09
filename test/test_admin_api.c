#include "src/client/client.h"
#include "lib/cJSON/cJSON.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    // Connect
    ClientConnection* conn = client_connect("localhost", 8080);
    if (!conn) {
        printf("Connection failed\n");
        return 1;
    }
    printf("✓ Connected\n");

    // Login
    if (client_login(conn, "admin", "admin") < 0) {
        printf("✗ Login failed\n");
        client_disconnect(conn);
        return 1;
    }
    printf("✓ Logged in (user_id=%d, is_admin=%d)\n", conn->user_id, conn->is_admin);

    // Test admin list users
    printf("Testing client_admin_list_users...\n");
    cJSON* response = client_admin_list_users(conn);
    if (!response) {
        printf("✗ client_admin_list_users returned NULL\n");
        client_disconnect(conn);
        return 1;
    }

    char* json_str = cJSON_Print(response);
    printf("✓ Response: %s\n", json_str);
    free(json_str);
    cJSON_Delete(response);

    client_disconnect(conn);
    printf("✓ Test passed!\n");
    return 0;
}
