#ifndef SOCKET_MGR_H
#define SOCKET_MGR_H

#include <netinet/in.h>

// Create and configure server socket
int socket_create_server(int port);

// Accept incoming client connection
int socket_accept_client(int server_fd, struct sockaddr_in* client_addr);

// Close socket gracefully
void socket_close(int socket_fd);

// Set socket options (timeout, keepalive)
int socket_set_options(int socket_fd);

// Get client IP as string
char* socket_get_client_ip(struct sockaddr_in* addr);

#endif // SOCKET_MGR_H
