#include "protocol.h"
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>

Packet* packet_create(uint8_t command, const char* payload, uint32_t length) {
    Packet* pkt = malloc(sizeof(Packet));
    if (!pkt) return NULL;

    pkt->magic[0] = MAGIC_BYTE_1;
    pkt->magic[1] = MAGIC_BYTE_2;
    pkt->command = command;
    pkt->data_length = length;

    if (payload && length > 0) {
        pkt->payload = malloc(length + 1);
        if (!pkt->payload) {
            free(pkt);
            return NULL;
        }
        memcpy(pkt->payload, payload, length);
        pkt->payload[length] = '\0';
    } else {
        pkt->payload = NULL;
    }

    return pkt;
}

int packet_encode(Packet* pkt, uint8_t* buffer, size_t buf_size) {
    if (!pkt || !buffer) return -1;

    size_t required_size = HEADER_SIZE + pkt->data_length;
    if (buf_size < required_size) return -1;

    // Magic bytes
    buffer[0] = MAGIC_BYTE_1;
    buffer[1] = MAGIC_BYTE_2;

    // Command
    buffer[2] = pkt->command;

    // Data length (network byte order)
    uint32_t net_length = htonl(pkt->data_length);
    memcpy(buffer + 3, &net_length, sizeof(uint32_t));

    // Payload
    if (pkt->payload && pkt->data_length > 0) {
        memcpy(buffer + HEADER_SIZE, pkt->payload, pkt->data_length);
    }

    return (int)required_size;
}

int packet_decode(uint8_t* buffer, size_t buf_size, Packet* pkt) {
    if (!buffer || !pkt || buf_size < HEADER_SIZE) return -1;

    // Verify magic bytes
    if (buffer[0] != MAGIC_BYTE_1 || buffer[1] != MAGIC_BYTE_2) {
        return -2;  // Invalid magic
    }

    pkt->magic[0] = buffer[0];
    pkt->magic[1] = buffer[1];
    pkt->command = buffer[2];

    // Get data length (convert from network byte order)
    uint32_t net_length;
    memcpy(&net_length, buffer + 3, sizeof(uint32_t));
    pkt->data_length = ntohl(net_length);

    // Validate payload size
    if (pkt->data_length > MAX_PAYLOAD_SIZE) {
        return -3;  // Payload too large
    }

    if (buf_size < HEADER_SIZE + pkt->data_length) {
        return -4;  // Buffer too small for payload
    }

    // Copy payload
    if (pkt->data_length > 0) {
        pkt->payload = malloc(pkt->data_length + 1);
        if (!pkt->payload) return -5;
        memcpy(pkt->payload, buffer + HEADER_SIZE, pkt->data_length);
        pkt->payload[pkt->data_length] = '\0';
    } else {
        pkt->payload = NULL;
    }

    return 0;
}

void packet_free(Packet* pkt) {
    if (pkt) {
        if (pkt->payload) {
            free(pkt->payload);
        }
        free(pkt);
    }
}

// Helper: Read full packet from socket
int packet_recv(int socket_fd, Packet* pkt) {
    uint8_t header[HEADER_SIZE];

    // Read header first
    ssize_t n = recv(socket_fd, header, HEADER_SIZE, MSG_WAITALL);
    if (n <= 0) return -1;
    if (n < HEADER_SIZE) return -2;

    // Verify magic
    if (header[0] != MAGIC_BYTE_1 || header[1] != MAGIC_BYTE_2) {
        return -3;
    }

    pkt->magic[0] = header[0];
    pkt->magic[1] = header[1];
    pkt->command = header[2];

    uint32_t net_length;
    memcpy(&net_length, header + 3, sizeof(uint32_t));
    pkt->data_length = ntohl(net_length);

    if (pkt->data_length > MAX_PAYLOAD_SIZE) {
        return -4;
    }

    // Read payload if present
    if (pkt->data_length > 0) {
        pkt->payload = malloc(pkt->data_length + 1);
        if (!pkt->payload) return -5;

        n = recv(socket_fd, pkt->payload, pkt->data_length, MSG_WAITALL);
        if (n < (ssize_t)pkt->data_length) {
            free(pkt->payload);
            pkt->payload = NULL;
            return -6;
        }
        pkt->payload[pkt->data_length] = '\0';
    } else {
        pkt->payload = NULL;
    }

    return 0;
}

// Helper: Send packet to socket
int packet_send(int socket_fd, Packet* pkt) {
    size_t total_size = HEADER_SIZE + pkt->data_length;
    uint8_t* buffer = malloc(total_size);
    if (!buffer) return -1;

    int encoded_size = packet_encode(pkt, buffer, total_size);
    if (encoded_size < 0) {
        free(buffer);
        return -2;
    }

    ssize_t sent = send(socket_fd, buffer, encoded_size, 0);
    free(buffer);

    return (sent == encoded_size) ? 0 : -3;
}
