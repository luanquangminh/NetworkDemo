#include "net_handler.h"
#include "../common/utils.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int net_connect(const char* host, uint16_t port) {
    struct addrinfo hints, *result, *rp;
    int sockfd = -1;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    char port_str[16];
    snprintf(port_str, sizeof(port_str), "%d", port);

    if (getaddrinfo(host, port_str, &hints, &result) != 0) {
        return -1;
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sockfd == -1) continue;

        if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) != -1) {
            break;
        }

        close(sockfd);
        sockfd = -1;
    }

    freeaddrinfo(result);
    return sockfd;
}

void net_disconnect(int sockfd) {
    if (sockfd >= 0) {
        shutdown(sockfd, SHUT_RDWR);
        close(sockfd);
    }
}

int net_send_packet(int sockfd, Packet* pkt) {
    if (!pkt) return -1;
    return packet_send(sockfd, pkt);
}

Packet* net_recv_packet(int sockfd) {
    Packet* pkt = malloc(sizeof(Packet));
    if (!pkt) return NULL;

    memset(pkt, 0, sizeof(Packet));

    if (packet_recv(sockfd, pkt) < 0) {
        free(pkt);
        return NULL;
    }

    return pkt;
}

int net_send_file(int sockfd, const char* file_path) {
    FILE* fp = fopen(file_path, "rb");
    if (!fp) return -1;

    char buffer[8192];
    size_t bytes_read;
    int result = 0;

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
        Packet* pkt = packet_create(CMD_UPLOAD_DATA, buffer, bytes_read);
        if (!pkt) {
            result = -1;
            break;
        }

        if (packet_send(sockfd, pkt) < 0) {
            packet_free(pkt);
            result = -1;
            break;
        }

        packet_free(pkt);
    }

    fclose(fp);
    return result;
}

int net_recv_file(int sockfd, const char* file_path, size_t file_size) {
    FILE* fp = fopen(file_path, "wb");
    if (!fp) return -1;

    size_t total_received = 0;
    int result = 0;

    while (total_received < file_size) {
        Packet pkt = {0};
        if (packet_recv(sockfd, &pkt) < 0) {
            result = -1;
            break;
        }

        if (pkt.command != CMD_DOWNLOAD_RES && pkt.data_length > 0) {
            fwrite(pkt.payload, 1, pkt.data_length, fp);
            total_received += pkt.data_length;
        }

        if (pkt.payload) free(pkt.payload);

        if (pkt.command == CMD_SUCCESS || pkt.command == CMD_ERROR) {
            break;
        }
    }

    fclose(fp);
    return result;
}
