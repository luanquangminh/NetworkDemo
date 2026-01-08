#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <stddef.h>

#define MAGIC_BYTE_1 0xFA
#define MAGIC_BYTE_2 0xCE

#define DEFAULT_PORT 8080
#define MAX_PAYLOAD_SIZE (16 * 1024 * 1024)  // 16MB max
#define HEADER_SIZE 7

// Command IDs
#define CMD_LOGIN_REQ    0x01
#define CMD_LOGIN_RES    0x02
#define CMD_LIST_DIR     0x10
#define CMD_CHANGE_DIR   0x11
#define CMD_MAKE_DIR     0x12
#define CMD_UPLOAD_REQ   0x20
#define CMD_UPLOAD_DATA  0x21
#define CMD_DOWNLOAD_REQ 0x30
#define CMD_DOWNLOAD_RES 0x31
#define CMD_DELETE       0x40
#define CMD_CHMOD        0x41
#define CMD_FILE_INFO    0x42
#define CMD_SEARCH_REQ   0x43
#define CMD_SEARCH_RES   0x44
#define CMD_RENAME       0x45
#define CMD_COPY         0x46
#define CMD_MOVE         0x47
#define CMD_ADMIN_LIST_USERS   0x50
#define CMD_ADMIN_CREATE_USER  0x51
#define CMD_ADMIN_DELETE_USER  0x52
#define CMD_ADMIN_UPDATE_USER  0x53
#define CMD_ERROR        0xFF
#define CMD_SUCCESS      0xFE

// Response Status Codes
#define STATUS_OK           0
#define STATUS_ERROR        1
#define STATUS_AUTH_FAIL    2
#define STATUS_PERM_DENIED  3
#define STATUS_NOT_FOUND    4
#define STATUS_EXISTS       5

typedef struct {
    uint8_t magic[2];
    uint8_t command;
    uint32_t data_length;
    char* payload;
} Packet;

// Function prototypes
int packet_encode(Packet* pkt, uint8_t* buffer, size_t buf_size);
int packet_decode(uint8_t* buffer, size_t buf_size, Packet* pkt);
void packet_free(Packet* pkt);
Packet* packet_create(uint8_t command, const char* payload, uint32_t length);

// Helper functions for socket I/O
int packet_recv(int socket_fd, Packet* pkt);
int packet_send(int socket_fd, Packet* pkt);

#endif // PROTOCOL_H
