#ifndef SERVER_H
#define SERVER_H

#include <stdint.h>

#define MAX_CLIENTS 100
#define SERVER_BACKLOG 20

typedef struct {
    int socket_fd;
    uint16_t port;
    int is_running;
} Server;

// Server lifecycle
Server* server_create(uint16_t port);
int server_start(Server* srv);
void server_stop(Server* srv);
void server_destroy(Server* srv);

#endif // SERVER_H
