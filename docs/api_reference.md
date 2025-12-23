# API Reference

## Common Module

### Protocol Functions (protocol.h)

#### `packet_create()`
```c
Packet* packet_create(uint8_t command, const char* payload, uint32_t length);
```
Creates a new packet with specified command and payload.

**Parameters:**
- `command`: Command identifier (see protocol.h for constants)
- `payload`: Payload data (can be NULL if length is 0)
- `length`: Payload size in bytes

**Returns:** Pointer to allocated Packet structure, or NULL on error

**Notes:** Caller must free packet using `packet_free()`

---

#### `packet_encode()`
```c
int packet_encode(Packet* pkt, uint8_t* buffer, size_t buf_size);
```
Encodes a packet into binary format for network transmission.

**Parameters:**
- `pkt`: Packet to encode
- `buffer`: Destination buffer
- `buf_size`: Size of destination buffer

**Returns:** Number of bytes written, or -1 on error

**Notes:** Buffer must be at least `HEADER_SIZE + pkt->data_length` bytes

---

#### `packet_decode()`
```c
int packet_decode(uint8_t* buffer, size_t buf_size, Packet* pkt);
```
Decodes binary data into a Packet structure.

**Parameters:**
- `buffer`: Source buffer containing encoded packet
- `buf_size`: Size of source buffer
- `pkt`: Destination packet structure

**Returns:** 0 on success, -1 on error

**Notes:** Allocates memory for payload; caller must use `packet_free()`

---

#### `packet_free()`
```c
void packet_free(Packet* pkt);
```
Frees memory allocated for packet payload.

**Parameters:**
- `pkt`: Packet to free

---

### Utility Functions (utils.h)

#### `log_init()`
```c
void log_init(const char* log_file);
```
Initializes logging system.

**Parameters:**
- `log_file`: Path to log file

---

#### `log_close()`
```c
void log_close(void);
```
Closes logging system and flushes buffers.

---

#### `log_info()`
```c
void log_info(const char* format, ...);
```
Logs informational message.

**Parameters:**
- `format`: Printf-style format string
- `...`: Variable arguments

---

#### `log_error()`
```c
void log_error(const char* format, ...);
```
Logs error message.

**Parameters:**
- `format`: Printf-style format string
- `...`: Variable arguments

---

#### `log_debug()`
```c
void log_debug(const char* format, ...);
```
Logs debug message.

**Parameters:**
- `format`: Printf-style format string
- `...`: Variable arguments

---

#### `generate_uuid()`
```c
char* generate_uuid(void);
```
Generates a UUID v4 string.

**Returns:** Allocated UUID string (36 chars + null), or NULL on error

**Notes:** Caller must free returned string

---

#### `get_timestamp()`
```c
char* get_timestamp(void);
```
Gets current timestamp in ISO 8601 format.

**Returns:** Allocated timestamp string, or NULL on error

**Notes:** Caller must free returned string

---

#### `str_duplicate()`
```c
char* str_duplicate(const char* str);
```
Duplicates a string.

**Parameters:**
- `str`: String to duplicate

**Returns:** Allocated copy of string, or NULL on error

**Notes:** Caller must free returned string

---

## Server Module

### Server Functions (server.h)

#### `server_create()`
```c
Server* server_create(uint16_t port);
```
Creates a new server instance.

**Parameters:**
- `port`: TCP port to bind to

**Returns:** Server instance, or NULL on error

---

#### `server_start()`
```c
int server_start(Server* srv);
```
Starts the server and begins accepting connections.

**Parameters:**
- `srv`: Server instance

**Returns:** 0 on normal shutdown, -1 on error

---

#### `server_stop()`
```c
void server_stop(Server* srv);
```
Stops the server gracefully.

**Parameters:**
- `srv`: Server instance

---

#### `server_destroy()`
```c
void server_destroy(Server* srv);
```
Destroys server and frees resources.

**Parameters:**
- `srv`: Server instance

---

### Socket Management (socket_mgr.h)

#### `socket_create_and_bind()`
```c
int socket_create_and_bind(uint16_t port);
```
Creates and binds a TCP socket.

**Parameters:**
- `port`: Port number to bind

**Returns:** Socket file descriptor, or -1 on error

---

#### `socket_send_data()`
```c
int socket_send_data(int sockfd, const uint8_t* data, size_t length);
```
Sends data over socket.

**Parameters:**
- `sockfd`: Socket file descriptor
- `data`: Data to send
- `length`: Number of bytes to send

**Returns:** Number of bytes sent, or -1 on error

---

#### `socket_recv_data()`
```c
int socket_recv_data(int sockfd, uint8_t* buffer, size_t buf_size);
```
Receives data from socket.

**Parameters:**
- `sockfd`: Socket file descriptor
- `buffer`: Destination buffer
- `buf_size`: Buffer size

**Returns:** Number of bytes received, 0 on connection close, -1 on error

---

### Thread Pool (thread_pool.h)

#### `thread_pool_create()`
```c
ThreadPool* thread_pool_create(int num_threads);
```
Creates a thread pool.

**Parameters:**
- `num_threads`: Number of worker threads

**Returns:** Thread pool instance, or NULL on error

---

#### `thread_pool_add_task()`
```c
int thread_pool_add_task(ThreadPool* pool, void (*function)(void*), void* arg);
```
Adds a task to the thread pool queue.

**Parameters:**
- `pool`: Thread pool instance
- `function`: Task function pointer
- `arg`: Argument to pass to task function

**Returns:** 0 on success, -1 on error

---

## Client Module

### Client Functions (client.h)

#### `client_create()`
```c
Client* client_create(const char* host, uint16_t port);
```
Creates a client instance.

**Parameters:**
- `host`: Server hostname or IP address
- `port`: Server port

**Returns:** Client instance, or NULL on error

---

#### `client_connect()`
```c
int client_connect(Client* client);
```
Connects to the server.

**Parameters:**
- `client`: Client instance

**Returns:** 0 on success, -1 on error

---

#### `client_login()`
```c
int client_login(Client* client, const char* username, const char* password);
```
Authenticates with the server.

**Parameters:**
- `client`: Client instance
- `username`: Username
- `password`: Password

**Returns:** 0 on success, -1 on error

---

## Database Module

### Database Manager (db_manager.h)

#### `db_manager_create()`
```c
DBManager* db_manager_create(const char* db_path);
```
Creates database manager and opens database.

**Parameters:**
- `db_path`: Path to SQLite database file

**Returns:** Database manager instance, or NULL on error

---

#### `db_user_authenticate()`
```c
int db_user_authenticate(DBManager* mgr, const char* username, const char* password_hash);
```
Authenticates a user.

**Parameters:**
- `mgr`: Database manager instance
- `username`: Username
- `password_hash`: Password hash (SHA-256)

**Returns:** User ID on success, -1 on failure

---

#### `db_file_create()`
```c
int db_file_create(DBManager* mgr, const char* name, int parent_id, int owner_id,
                   const char* physical_path, int is_directory, int permissions);
```
Creates a file entry in virtual file system.

**Parameters:**
- `mgr`: Database manager instance
- `name`: File/directory name
- `parent_id`: Parent directory ID (0 for root)
- `owner_id`: User ID of owner
- `physical_path`: Path in storage directory
- `is_directory`: 1 for directory, 0 for file
- `permissions`: Unix-style permissions (e.g., 755)

**Returns:** File ID on success, -1 on error

---

## Constants

### Protocol Constants
- `MAGIC_BYTE_1` = 0xFA
- `MAGIC_BYTE_2` = 0xCE
- `DEFAULT_PORT` = 8080
- `MAX_PAYLOAD_SIZE` = 16777216 (16 MB)
- `HEADER_SIZE` = 7

### Command IDs
See protocol.h for complete list (0x01-0xFF)

### Status Codes
- `STATUS_OK` = 0
- `STATUS_ERROR` = 1
- `STATUS_AUTH_FAIL` = 2
- `STATUS_PERM_DENIED` = 3
- `STATUS_NOT_FOUND` = 4
- `STATUS_EXISTS` = 5
