#ifndef NET_HANDLER_H
#define NET_HANDLER_H

#include <stdint.h>
#include "../common/protocol.h"

// Network connection
int net_connect(const char* host, uint16_t port);
void net_disconnect(int sockfd);

// Protocol operations
int net_send_packet(int sockfd, Packet* pkt);
Packet* net_recv_packet(int sockfd);

// File transfer helpers
int net_send_file(int sockfd, const char* file_path);
int net_recv_file(int sockfd, const char* file_path, size_t file_size);

#endif // NET_HANDLER_H
