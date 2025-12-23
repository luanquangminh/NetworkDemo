#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include "socket_mgr.h"
#include "thread_pool.h"
#include "commands.h"
#include "storage.h"
#include "../common/protocol.h"
#include "../common/utils.h"
#include "../database/db_manager.h"

static volatile int running = 1;
static int server_fd = -1;

// Global database handle
Database* global_db = NULL;

void shutdown_handler(int sig) {
    (void)sig;
    printf("\nShutting down server...\n");
    running = 0;
    if (server_fd >= 0) {
        socket_close(server_fd);
    }
    if (global_db) {
        db_close(global_db);
        global_db = NULL;
    }
}

int main(int argc, char** argv) {
    int port = DEFAULT_PORT;

    if (argc > 1) {
        port = atoi(argv[1]);
    }

    // Setup signal handlers
    signal(SIGINT, shutdown_handler);
    signal(SIGTERM, shutdown_handler);

    // Initialize logging
    log_init("server.log");

    // Initialize database
    global_db = db_init("fileshare.db");
    if (!global_db) {
        log_error("Failed to initialize database");
        return 1;
    }

    // Initialize database schema
    if (db_init_schema(global_db, "src/database/db_init.sql") < 0) {
        log_error("Failed to initialize database schema");
        db_close(global_db);
        return 1;
    }

    // Initialize storage
    if (storage_init("storage") < 0) {
        log_error("Failed to initialize storage");
        db_close(global_db);
        return 1;
    }

    // Initialize command handlers
    commands_init();

    // Initialize thread pool
    thread_pool_init();

    // Create server socket
    server_fd = socket_create_server(port);
    if (server_fd < 0) {
        log_error("Failed to create server socket");
        return 1;
    }

    log_info("Server listening on port %d", port);
    printf("File Sharing Server started on port %d\n", port);
    printf("Press Ctrl+C to shutdown\n");

    // Main accept loop
    while (running) {
        struct sockaddr_in client_addr;
        int client_fd = socket_accept_client(server_fd, &client_addr);

        if (client_fd < 0) {
            if (running) {
                log_error("Failed to accept client");
            }
            continue;
        }

        char* client_ip = socket_get_client_ip(&client_addr);
        log_info("Client connected from %s", client_ip);
        printf("Client connected from %s\n", client_ip);

        // Spawn handler thread
        if (thread_spawn_client(client_fd, &client_addr) < 0) {
            log_error("Failed to spawn client handler");
            socket_close(client_fd);
        }

        free(client_ip);
    }

    // Cleanup
    printf("Shutting down client handlers...\n");
    thread_pool_shutdown();

    // Close database if not already closed
    if (global_db) {
        db_close(global_db);
        global_db = NULL;
    }

    log_info("Server shutdown complete");
    log_close();

    printf("Server stopped.\n");
    return 0;
}
