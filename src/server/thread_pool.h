#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <pthread.h>
#include <netinet/in.h>

#define MAX_CLIENTS 100

typedef enum {
    STATE_CONNECTED,
    STATE_AUTHENTICATED,
    STATE_TRANSFERRING,
    STATE_DISCONNECTED
} ClientState;

typedef struct {
    int client_socket;
    struct sockaddr_in client_addr;
    pthread_t thread_id;
    int user_id;
    int current_directory;
    ClientState state;
    int authenticated;
    char* pending_upload_uuid;
    long pending_upload_size;
} ClientSession;

// Initialize thread management
void thread_pool_init(void);

// Create new client handler thread
int thread_spawn_client(int client_socket, struct sockaddr_in* addr);

// Client handler function (thread entry point)
void* client_handler(void* arg);

// Cleanup single session
void cleanup_session(ClientSession* session);

// Shutdown all client threads
void thread_pool_shutdown(void);

// Get active client count
int thread_pool_active_count(void);

#endif // THREAD_POOL_H
