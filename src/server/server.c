#include "server.h"
#include "socket_mgr.h"
#include "thread_pool.h"
#include "../common/utils.h"
#include "../database/db_manager.h"
#include <stdlib.h>
#include <string.h>

Server* server_create(uint16_t port) {
    // Allocate Server structure
    Server* srv = (Server*)malloc(sizeof(Server));
    if (!srv) {
        log_error("Failed to allocate memory for Server structure");
        return NULL;
    }

    // Initialize fields
    srv->socket_fd = -1;
    srv->port = port;
    srv->is_running = 0;

    // Create socket and bind to port
    srv->socket_fd = socket_create_server(port);
    if (srv->socket_fd < 0) {
        log_error("Failed to create server socket on port %d", port);
        free(srv);
        return NULL;
    }

    // Initialize thread pool
    thread_pool_init();
    log_info("Thread pool initialized");

    log_info("Server created successfully on port %d", port);
    return srv;
}

int server_start(Server* srv) {
    if (!srv) {
        log_error("Server is NULL");
        return -1;
    }

    if (srv->socket_fd < 0) {
        log_error("Invalid server socket");
        return -1;
    }

    // Set is_running flag
    srv->is_running = 1;
    log_info("Server starting on port %d...", srv->port);

    // Main accept loop
    while (srv->is_running) {
        struct sockaddr_in client_addr;

        // Accept client connection
        int client_fd = socket_accept_client(srv->socket_fd, &client_addr);

        // Check if server was stopped during accept (EINTR)
        if (client_fd < 0) {
            if (!srv->is_running) {
                log_info("Server stopped during accept");
                break;
            }
            // Log error but continue accepting
            continue;
        }

        // Spawn thread for client handler
        if (thread_spawn_client(client_fd, &client_addr) < 0) {
            log_error("Failed to spawn client thread, closing connection");
            socket_close(client_fd);
        }
    }

    log_info("Server accept loop terminated");
    return 0;
}

void server_stop(Server* srv) {
    if (!srv) {
        log_error("Server is NULL");
        return;
    }

    log_info("Stopping server...");

    // Set is_running to 0 to stop accept loop
    srv->is_running = 0;

    // Close server socket to unblock accept() call
    if (srv->socket_fd >= 0) {
        socket_close(srv->socket_fd);
        srv->socket_fd = -1;
    }

    log_info("Server stopped");
}

void server_destroy(Server* srv) {
    if (!srv) {
        log_error("Server is NULL");
        return;
    }

    log_info("Destroying server...");

    // Stop server if running
    if (srv->is_running) {
        server_stop(srv);
    }

    // Shutdown thread pool (wait for active connections to finish)
    thread_pool_shutdown();
    log_info("Thread pool shutdown complete");

    // Free server structure
    free(srv);
    log_info("Server destroyed");
}
