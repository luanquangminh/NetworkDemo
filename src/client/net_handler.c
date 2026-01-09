#include "net_handler.h"
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

    // Get file size
    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return -1;
    }
    long file_size = ftell(fp);
    if (file_size < 0) {
        fclose(fp);
        return -1;
    }
    if (fseek(fp, 0, SEEK_SET) != 0) {
        fclose(fp);
        return -1;
    }

    // Check if file size exceeds MAX_PAYLOAD_SIZE
    if ((size_t)file_size > MAX_PAYLOAD_SIZE) {
        fclose(fp);
        return -1;  // File too large for single packet
    }

    // Allocate buffer for entire file
    char* buffer = malloc((size_t)file_size);
    if (!buffer) {
        fclose(fp);
        return -1;
    }

    // Read entire file into buffer
    size_t bytes_read = fread(buffer, 1, (size_t)file_size, fp);
    fclose(fp);

    if (bytes_read != (size_t)file_size) {
        free(buffer);
        return -1;
    }

    // Send single packet with complete file data
    Packet* pkt = packet_create(CMD_UPLOAD_DATA, buffer, (uint32_t)file_size);
    free(buffer);

    if (!pkt) {
        return -1;
    }

    int result = packet_send(sockfd, pkt);
    packet_free(pkt);

    return (result < 0) ? -1 : 0;
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
