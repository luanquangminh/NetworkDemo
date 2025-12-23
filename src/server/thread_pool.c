#include "thread_pool.h"
#include "socket_mgr.h"
#include "commands.h"
#include "../common/utils.h"
#include "../common/protocol.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

// Global session array and mutex
static ClientSession* sessions[MAX_CLIENTS];
static pthread_mutex_t sessions_mutex = PTHREAD_MUTEX_INITIALIZER;
static int active_count = 0;

void thread_pool_init(void) {
    pthread_mutex_lock(&sessions_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        sessions[i] = NULL;
    }
    active_count = 0;
    pthread_mutex_unlock(&sessions_mutex);
    log_info("Thread pool initialized");
}

int thread_spawn_client(int client_socket, struct sockaddr_in* addr) {
    if (!addr) {
        log_error("Invalid client address");
        return -1;
    }

    pthread_mutex_lock(&sessions_mutex);

    // Find free slot
    int slot = -1;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (sessions[i] == NULL) {
            slot = i;
            break;
        }
    }

    if (slot == -1) {
        pthread_mutex_unlock(&sessions_mutex);
        log_error("Max clients reached (%d)", MAX_CLIENTS);
        return -1;
    }

    // Allocate session
    ClientSession* session = malloc(sizeof(ClientSession));
    if (!session) {
        pthread_mutex_unlock(&sessions_mutex);
        log_error("Failed to allocate session");
        return -1;
    }

    // Initialize session
    memset(session, 0, sizeof(ClientSession));
    session->client_socket = client_socket;
    memcpy(&session->client_addr, addr, sizeof(struct sockaddr_in));
    session->state = STATE_CONNECTED;
    session->authenticated = 0;
    session->user_id = -1;
    session->current_directory = -1;
    session->pending_upload_uuid = NULL;
    session->pending_upload_size = 0;

    // Create detached thread
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    if (pthread_create(&session->thread_id, &attr, client_handler, session) != 0) {
        pthread_attr_destroy(&attr);
        pthread_mutex_unlock(&sessions_mutex);
        free(session);
        log_error("Failed to create client handler thread");
        return -1;
    }

    pthread_attr_destroy(&attr);
    sessions[slot] = session;
    active_count++;

    pthread_mutex_unlock(&sessions_mutex);

    log_info("Spawned client handler thread (slot=%d, active=%d)", slot, active_count);
    return 0;
}

void* client_handler(void* arg) {
    ClientSession* session = (ClientSession*)arg;
    if (!session) {
        log_error("NULL session in client_handler");
        return NULL;
    }

    char* client_ip = socket_get_client_ip(&session->client_addr);
    log_info("Client handler started for %s (fd=%d)", client_ip, session->client_socket);

    session->state = STATE_CONNECTED;

    while (session->state != STATE_DISCONNECTED) {
        Packet pkt = {0};

        int result = packet_recv(session->client_socket, &pkt);
        if (result < 0) {
            if (result == -1) {
                log_info("Client %s disconnected", client_ip);
            } else {
                log_error("Packet receive error %d from %s", result, client_ip);
            }
            break;
        }

        log_debug("Received command 0x%02X from %s", pkt.command, client_ip);

        dispatch_command(session, &pkt);

        if (pkt.payload) {
            free(pkt.payload);
        }
    }

    free(client_ip);
    cleanup_session(session);
    return NULL;
}

void cleanup_session(ClientSession* session) {
    if (!session) {
        return;
    }

    char* client_ip = socket_get_client_ip(&session->client_addr);
    log_info("Cleaning up session for %s (fd=%d)", client_ip, session->client_socket);
    free(client_ip);

    // Close socket
    socket_close(session->client_socket);

    // Free pending upload UUID if exists
    if (session->pending_upload_uuid) {
        free(session->pending_upload_uuid);
        session->pending_upload_uuid = NULL;
    }

    // Remove from sessions array
    pthread_mutex_lock(&sessions_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (sessions[i] == session) {
            sessions[i] = NULL;
            active_count--;
            log_info("Session removed (slot=%d, active=%d)", i, active_count);
            break;
        }
    }
    pthread_mutex_unlock(&sessions_mutex);

    // Free session
    free(session);
}

void thread_pool_shutdown(void) {
    log_info("Shutting down thread pool...");

    pthread_mutex_lock(&sessions_mutex);

    // Signal all sessions to disconnect
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (sessions[i]) {
            sessions[i]->state = STATE_DISCONNECTED;
            shutdown(sessions[i]->client_socket, SHUT_RDWR);
        }
    }

    pthread_mutex_unlock(&sessions_mutex);

    // Give threads time to cleanup
    int wait_count = 0;
    while (active_count > 0 && wait_count < 50) {
        usleep(100000); // 100ms
        wait_count++;
    }

    pthread_mutex_lock(&sessions_mutex);

    // Force cleanup any remaining sessions
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (sessions[i]) {
            log_info("Force cleaning up session in slot %d", i);
            socket_close(sessions[i]->client_socket);
            if (sessions[i]->pending_upload_uuid) {
                free(sessions[i]->pending_upload_uuid);
            }
            free(sessions[i]);
            sessions[i] = NULL;
        }
    }

    active_count = 0;
    pthread_mutex_unlock(&sessions_mutex);

    log_info("Thread pool shutdown complete");
}

int thread_pool_active_count(void) {
    pthread_mutex_lock(&sessions_mutex);
    int count = active_count;
    pthread_mutex_unlock(&sessions_mutex);
    return count;
}
