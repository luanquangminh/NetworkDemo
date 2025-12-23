#ifndef COMMANDS_H
#define COMMANDS_H

#include "thread_pool.h"
#include "../common/protocol.h"

// Initialize command handlers
void commands_init(void);

// Main command dispatcher
int dispatch_command(ClientSession* session, Packet* pkt);

// Individual command handlers
void handle_login(ClientSession* session, Packet* pkt);
void handle_list_dir(ClientSession* session, Packet* pkt);
void handle_change_dir(ClientSession* session, Packet* pkt);
void handle_mkdir(ClientSession* session, Packet* pkt);
void handle_upload_req(ClientSession* session, Packet* pkt);
void handle_upload_data(ClientSession* session, Packet* pkt);
void handle_download(ClientSession* session, Packet* pkt);
void handle_chmod(ClientSession* session, Packet* pkt);
void handle_delete(ClientSession* session, Packet* pkt);
void handle_file_info(ClientSession* session, Packet* pkt);

// Admin command handlers
void handle_admin_list_users(ClientSession* session, Packet* pkt);
void handle_admin_create_user(ClientSession* session, Packet* pkt);
void handle_admin_delete_user(ClientSession* session, Packet* pkt);
void handle_admin_update_user(ClientSession* session, Packet* pkt);

// Helper: Send error response
void send_error(ClientSession* session, const char* message);

// Helper: Send success response
void send_success(ClientSession* session, uint8_t cmd, const char* json_payload);

#endif // COMMANDS_H
