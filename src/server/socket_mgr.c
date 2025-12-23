#include "socket_mgr.h"
#include "../common/utils.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

int socket_create_server(int port) {
    // Validate port range
    if (port < 1024 || port > 65535) {
        log_error("Invalid port number: %d (must be 1024-65535)", port);
        return -1;
    }

    // Create TCP socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket() failed");
        log_error("Failed to create socket");
        return -1;
    }

    // Set SO_REUSEADDR option
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
        log_error("Failed to set SO_REUSEADDR");
        close(server_fd);
        return -1;
    }

    // Set socket options (keepalive, etc.)
    if (socket_set_options(server_fd) < 0) {
        log_error("Failed to set socket options");
        close(server_fd);
        return -1;
    }

    // Bind to INADDR_ANY on specified port
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind() failed");
        log_error("Failed to bind to port %d", port);
        close(server_fd);
        return -1;
    }

    // Start listening with backlog of 10
    if (listen(server_fd, 10) < 0) {
        perror("listen() failed");
        log_error("Failed to listen on socket");
        close(server_fd);
        return -1;
    }

    log_info("Server socket created and listening on port %d", port);
    return server_fd;
}

int socket_accept_client(int server_fd, struct sockaddr_in* client_addr) {
    if (!client_addr) {
        log_error("client_addr is NULL");
        return -1;
    }

    socklen_t addr_len = sizeof(*client_addr);
    memset(client_addr, 0, sizeof(*client_addr));

    int client_fd = accept(server_fd, (struct sockaddr*)client_addr, &addr_len);
    if (client_fd < 0) {
        if (errno != EINTR) {
            perror("accept() failed");
            log_error("Failed to accept client connection");
        }
        return -1;
    }

    char* client_ip = socket_get_client_ip(client_addr);
    log_info("Accepted connection from %s (fd=%d)", client_ip, client_fd);
    free(client_ip);

    return client_fd;
}

void socket_close(int socket_fd) {
    if (socket_fd < 0) {
        return;
    }

    // Shutdown both read and write
    shutdown(socket_fd, SHUT_RDWR);

    // Close file descriptor
    if (close(socket_fd) < 0) {
        perror("close() failed");
        log_error("Failed to close socket fd=%d", socket_fd);
    } else {
        log_info("Socket fd=%d closed", socket_fd);
    }
}

int socket_set_options(int socket_fd) {
    // Set SO_KEEPALIVE
    int keepalive = 1;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_KEEPALIVE, &keepalive, sizeof(keepalive)) < 0) {
        perror("setsockopt(SO_KEEPALIVE) failed");
        return -1;
    }

    // Set receive timeout (optional)
    struct timeval tv;
    tv.tv_sec = 300;  // 5 minutes
    tv.tv_usec = 0;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("setsockopt(SO_RCVTIMEO) failed");
        return -1;
    }

    // Set send timeout (optional)
    tv.tv_sec = 300;  // 5 minutes
    tv.tv_usec = 0;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) < 0) {
        perror("setsockopt(SO_SNDTIMEO) failed");
        return -1;
    }

    return 0;
}

char* socket_get_client_ip(struct sockaddr_in* addr) {
    if (!addr) {
        return str_duplicate("unknown");
    }

    char* ip_str = malloc(INET_ADDRSTRLEN);
    if (!ip_str) {
        return str_duplicate("unknown");
    }

    if (inet_ntop(AF_INET, &addr->sin_addr, ip_str, INET_ADDRSTRLEN) == NULL) {
        free(ip_str);
        return str_duplicate("unknown");
    }

    return ip_str;
}
