# Phased Implementation Plan: Linux C File Sharing System

**Project:** File Sharing System (FTP-like)
**Stack:** C (Linux Sockets), SQLite, TCP/IP, pthread
**Architecture:** Client-Server with thread-per-client concurrency
**Created:** 2025-12-18
**Last Updated:** 2025-12-18

---

## Progress Tracking

| Phase | Status | Completed Date | Notes |
|-------|--------|----------------|-------|
| Phase 0 | ✅ Complete | 2025-12-18 | Directory structure, Makefile, headers, stubs, cJSON library |
| Phase 1 | ✅ Complete | 2025-12-18 | Socket manager, thread pool, graceful shutdown, logging |
| Phase 2 | ✅ Complete | 2025-12-18 | Packet codec, cJSON integration, LOGIN command, unit tests |
| Phase 3 | ✅ Complete | 2025-12-19 | SQLite integration, SHA-256 hashing, user auth, activity logging |
| Phase 4 | ✅ Complete | 2025-12-19 | Storage layer, LIST/MKDIR/UPLOAD/DOWNLOAD commands |
| Phase 5 | ✅ Complete | 2025-12-19 | Linux-style RWX permissions, CHMOD command, access logging |
| Phase 6 | 🔄 In Progress | - | Client GUI |

---

## Implementation Overview

**Total Phases:** 6
**Estimated Timeline:** 8-10 weeks
**Build System:** Makefile with incremental targets
**Testing Strategy:** Unit tests per module + Integration tests per phase

### Phase Dependency Chain
```
Phase 0 (Foundation)
    ↓
Phase 1 (Core Server) → Phase 2 (Protocol) → Phase 3 (File Ops) → Phase 4 (Permissions) → Phase 5 (Client GUI)
                            ↓
                        Database (parallel after Phase 1)
```

---

## Phase 0: Foundation & Build System Setup

**Duration:** 3-5 days
**Objective:** Project structure, build system, development environment

### Deliverables

#### Directory Structure
```
networkFinal/
├── Makefile                    # Root build orchestrator
├── README.md                   # Setup and build instructions
├── .gitignore                  # Ignore build artifacts
├── docs/                       # Documentation
│   ├── protocol_spec.md
│   └── api_reference.md
├── plans/                      # Existing design docs
├── src/
│   ├── server/
│   │   ├── main.c             # Server entry point
│   │   ├── server.h
│   │   ├── socket_mgr.c       # Socket lifecycle
│   │   ├── socket_mgr.h
│   │   ├── thread_pool.c      # Thread management
│   │   ├── thread_pool.h
│   │   └── Makefile
│   ├── client/
│   │   ├── main.c             # Client entry point
│   │   ├── client.h
│   │   ├── net_handler.c      # Network I/O
│   │   ├── net_handler.h
│   │   └── Makefile
│   ├── common/
│   │   ├── protocol.c         # Packet encode/decode
│   │   ├── protocol.h         # Shared definitions
│   │   ├── utils.c            # Logging, UUID generation
│   │   ├── utils.h
│   │   └── Makefile
│   └── database/
│       ├── db_init.sql        # Schema definition
│       ├── db_manager.c       # SQLite wrapper
│       ├── db_manager.h
│       └── Makefile
├── tests/
│   ├── test_protocol.c        # Unit tests for protocol
│   ├── test_db.c              # Database tests
│   └── Makefile
├── storage/                    # Physical file storage (runtime)
└── build/                      # Compiled binaries
    ├── server
    ├── client
    └── tests/
```

#### Files to Create

1. **Makefile (Root)**
   - Targets: `all`, `server`, `client`, `tests`, `clean`, `run-server`, `run-client`
   - Compiler flags: `-Wall -Wextra -pthread -lsqlite3`
   - Include paths for `src/common`

2. **src/common/protocol.h**
   ```c
   #define MAGIC_BYTE_1 0xFA
   #define MAGIC_BYTE_2 0xCE

   // Command IDs
   #define CMD_LOGIN_REQ    0x01
   #define CMD_LOGIN_RES    0x02
   #define CMD_LIST_DIR     0x10
   #define CMD_CHANGE_DIR   0x11
   #define CMD_MAKE_DIR     0x12
   #define CMD_UPLOAD_REQ   0x20
   #define CMD_UPLOAD_DATA  0x21
   #define CMD_DOWNLOAD_REQ 0x30

   typedef struct {
       uint8_t magic[2];
       uint8_t command;
       uint32_t data_length;
       char* payload;
   } Packet;

   int encode_packet(Packet* pkt, uint8_t* buffer, size_t buf_size);
   int decode_packet(uint8_t* buffer, size_t buf_size, Packet* pkt);
   ```

3. **src/common/utils.h**
   ```c
   void log_info(const char* format, ...);
   void log_error(const char* format, ...);
   char* generate_uuid(void);
   char* get_timestamp(void);
   ```

4. **src/database/db_init.sql**
   - CREATE TABLE statements for USERS, FILES, ACTIVITY_LOGS
   - Indexes on user_id, parent_id, owner_id

5. **.gitignore**
   ```
   build/
   storage/
   *.o
   *.log
   *.db
   .DS_Store
   ```

### Testing Milestone
- `make all` compiles without errors
- Directory structure validated
- `test_protocol` passes basic encode/decode tests

---

## Phase 1: Core Server Infrastructure

**Duration:** 1 week
**Dependencies:** Phase 0
**Objective:** TCP server accepts connections, spawns threads, handles graceful shutdown

### Implementation Tasks

#### 1.1 Socket Manager (`src/server/socket_mgr.c`)
```c
int socket_create_server(int port);
int socket_accept_client(int server_fd, struct sockaddr_in* client_addr);
void socket_close(int socket_fd);
```

**Logic:**
- `socket()` → `setsockopt(SO_REUSEADDR)` → `bind()` → `listen()`
- Error handling for each syscall
- Port validation (1024-65535)

#### 1.2 Thread Pool (`src/server/thread_pool.c`)
```c
typedef struct {
    int client_socket;
    struct sockaddr_in client_addr;
    pthread_t thread_id;
} ClientSession;

void* client_handler(void* arg);
void cleanup_session(ClientSession* session);
```

**Logic:**
- `pthread_create()` on each `accept()`
- Thread function enters infinite loop reading packets
- Detached threads with cleanup on disconnect
- Global list tracking active sessions (for shutdown)

#### 1.3 Server Main Loop (`src/server/main.c`)
```c
int main(int argc, char** argv) {
    signal(SIGINT, shutdown_handler);  // Ctrl+C graceful exit

    int server_fd = socket_create_server(PORT);
    log_info("Server listening on port %d", PORT);

    while (running) {
        int client_fd = socket_accept_client(server_fd, &addr);
        ClientSession* session = malloc(sizeof(ClientSession));
        session->client_socket = client_fd;
        pthread_create(&session->thread_id, NULL, client_handler, session);
    }

    cleanup_server();
}
```

#### 1.4 Synchronization Primitives (`src/common/utils.c`)
```c
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

void log_info(const char* format, ...) {
    pthread_mutex_lock(&log_mutex);
    // Write to server.log with timestamp
    pthread_mutex_unlock(&log_mutex);
}
```

### Files to Create
- `src/server/socket_mgr.c` + `.h`
- `src/server/thread_pool.c` + `.h`
- `src/server/main.c` + `server.h`
- `src/common/utils.c` (logging implementation)

### Testing Milestone
- Server starts and binds to port 8080
- `telnet localhost 8080` establishes connection
- Multiple `telnet` sessions work concurrently
- Ctrl+C triggers graceful shutdown (closes all sockets)
- No memory leaks (`valgrind --leak-check=full ./build/server`)

**Test Script:**
```bash
#!/bin/bash
./build/server &
SERVER_PID=$!
sleep 1
echo "Testing connection..."
timeout 2 telnet localhost 8080
kill -SIGINT $SERVER_PID
wait $SERVER_PID
echo "Server exited cleanly"
```

---

## Phase 2: Protocol Implementation

**Duration:** 1 week
**Dependencies:** Phase 1
**Objective:** Encode/decode packets, handle LOGIN command end-to-end

### Implementation Tasks

#### 2.1 Packet Codec (`src/common/protocol.c`)
```c
int encode_packet(Packet* pkt, uint8_t* buffer, size_t buf_size) {
    if (buf_size < 7 + pkt->data_length) return -1;

    buffer[0] = MAGIC_BYTE_1;
    buffer[1] = MAGIC_BYTE_2;
    buffer[2] = pkt->command;
    *(uint32_t*)(buffer + 3) = htonl(pkt->data_length);
    memcpy(buffer + 7, pkt->payload, pkt->data_length);

    return 7 + pkt->data_length;
}

int decode_packet(uint8_t* buffer, size_t buf_size, Packet* pkt) {
    if (buf_size < 7) return -1;
    if (buffer[0] != MAGIC_BYTE_1 || buffer[1] != MAGIC_BYTE_2) return -2;

    pkt->command = buffer[2];
    pkt->data_length = ntohl(*(uint32_t*)(buffer + 3));

    if (buf_size < 7 + pkt->data_length) return -3;

    pkt->payload = malloc(pkt->data_length);
    memcpy(pkt->payload, buffer + 7, pkt->data_length);

    return 0;
}
```

#### 2.2 JSON Payload Handling
**Approach:** Use `cJSON` library (lightweight, single-file include)

```c
// In Makefile
CFLAGS += -I./lib/cJSON

// Example: Login Request Payload
{
    "username": "alice",
    "password": "hashed_value"
}
```

**Add to utils:**
```c
char* json_get_string(cJSON* json, const char* key);
int json_get_int(cJSON* json, const char* key);
```

#### 2.3 Command Dispatcher (`src/server/thread_pool.c`)
```c
void* client_handler(void* arg) {
    ClientSession* session = (ClientSession*)arg;
    uint8_t buffer[8192];

    while (1) {
        int n = recv(session->client_socket, buffer, sizeof(buffer), 0);
        if (n <= 0) break;  // Client disconnected

        Packet pkt;
        if (decode_packet(buffer, n, &pkt) < 0) {
            log_error("Invalid packet from client");
            continue;
        }

        switch (pkt.command) {
            case CMD_LOGIN_REQ:
                handle_login(session, &pkt);
                break;
            // Other commands...
        }

        free(pkt.payload);
    }

    cleanup_session(session);
    return NULL;
}
```

#### 2.4 LOGIN Command Handler (Stub)
```c
void handle_login(ClientSession* session, Packet* pkt) {
    cJSON* json = cJSON_Parse(pkt->payload);
    char* username = json_get_string(json, "username");
    char* password = json_get_string(json, "password");

    // TODO: Verify against database (Phase 3)
    // For now, accept any credentials

    Packet response;
    response.command = CMD_LOGIN_RES;
    response.payload = "{\"status\": \"OK\", \"token\": \"dummy\"}";
    response.data_length = strlen(response.payload);

    uint8_t buf[1024];
    int len = encode_packet(&response, buf, sizeof(buf));
    send(session->client_socket, buf, len, 0);

    session->authenticated = 1;
    log_info("User '%s' logged in", username);

    cJSON_Delete(json);
}
```

### Files to Create
- `src/common/protocol.c` (full implementation)
- `src/server/commands.c` + `.h` (command handlers)
- `lib/cJSON/cJSON.c` + `.h` (external dependency)
- `tests/test_protocol.c` (unit tests)

### Testing Milestone
- Unit test: Encode → Decode round-trip preserves data
- Unit test: Invalid magic bytes rejected
- Integration test: Netcat sends LOGIN packet, receives response
  ```bash
  echo -ne '\xFA\xCE\x01\x00\x00\x00\x2A{"username":"test","password":"test"}' | nc localhost 8080
  ```
- `valgrind` shows no leaks in packet handling

---

## Phase 3: Database Integration

**Duration:** 1 week
**Dependencies:** Phase 2 (can start parallel after Phase 1)
**Objective:** SQLite setup, user authentication, activity logging

### Implementation Tasks

#### 3.1 Database Manager (`src/database/db_manager.c`)
```c
typedef struct {
    sqlite3* conn;
    pthread_mutex_t mutex;
} Database;

Database* db_init(const char* db_path);
void db_close(Database* db);

// User operations
int db_create_user(Database* db, const char* username, const char* password_hash);
int db_verify_user(Database* db, const char* username, const char* password_hash, int* user_id);
int db_user_is_active(Database* db, int user_id);

// Logging
int db_log_activity(Database* db, int user_id, const char* action, const char* description);
```

**Implementation Notes:**
- Mutex wraps all `sqlite3_exec()` and `sqlite3_step()` calls
- Use prepared statements for SQL injection prevention
- Connection opened once in `main()`, passed to threads

#### 3.2 Schema Initialization (`src/database/db_init.sql`)
```sql
CREATE TABLE IF NOT EXISTS users (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    username TEXT UNIQUE NOT NULL,
    password_hash TEXT NOT NULL,
    is_active INTEGER DEFAULT 1,
    created_at TEXT DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS files (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    parent_id INTEGER DEFAULT 0,  -- 0 = root
    name TEXT NOT NULL,
    physical_path TEXT UNIQUE,    -- UUID
    owner_id INTEGER NOT NULL,
    size INTEGER DEFAULT 0,
    is_directory INTEGER DEFAULT 0,
    permissions INTEGER DEFAULT 755,
    created_at TEXT DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (owner_id) REFERENCES users(id),
    FOREIGN KEY (parent_id) REFERENCES files(id)
);

CREATE INDEX idx_files_parent ON files(parent_id);
CREATE INDEX idx_files_owner ON files(owner_id);

CREATE TABLE IF NOT EXISTS activity_logs (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    action_type TEXT NOT NULL,
    description TEXT,
    timestamp TEXT DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(id)
);

-- Default admin user (password: "admin" hashed with SHA256)
INSERT INTO users (username, password_hash) VALUES
('admin', '8c6976e5b5410415bde908bd4dee15dfb167a9c873fc4bb8a81f6f2ab448a918');
```

**Schema Loading:**
```c
Database* db_init(const char* db_path) {
    Database* db = malloc(sizeof(Database));
    sqlite3_open(db_path, &db->conn);
    pthread_mutex_init(&db->mutex, NULL);

    // Execute db_init.sql
    FILE* f = fopen("src/database/db_init.sql", "r");
    // Read and exec SQL...

    return db;
}
```

#### 3.3 Password Hashing (`src/common/crypto.c`)
```c
#include <openssl/sha.h>

char* hash_password(const char* password) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char*)password, strlen(password), hash);

    char* hex = malloc(65);
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        sprintf(hex + (i * 2), "%02x", hash[i]);
    }
    hex[64] = '\0';
    return hex;
}
```

**Makefile addition:**
```makefile
LDFLAGS += -lcrypto
```

#### 3.4 Update LOGIN Handler
```c
void handle_login(ClientSession* session, Packet* pkt) {
    cJSON* json = cJSON_Parse(pkt->payload);
    char* username = json_get_string(json, "username");
    char* password = json_get_string(json, "password");

    char* hash = hash_password(password);
    int user_id;

    Packet response;
    if (db_verify_user(global_db, username, hash, &user_id) == 0) {
        session->user_id = user_id;
        session->authenticated = 1;

        response.payload = "{\"status\": \"OK\"}";
        db_log_activity(global_db, user_id, "LOGIN", "Successful login");
    } else {
        response.payload = "{\"status\": \"FAIL\", \"error\": \"Invalid credentials\"}";
    }

    response.command = CMD_LOGIN_RES;
    response.data_length = strlen(response.payload);

    uint8_t buf[1024];
    int len = encode_packet(&response, buf, sizeof(buf));
    send(session->client_socket, buf, len, 0);

    free(hash);
    cJSON_Delete(json);
}
```

### Files to Create
- `src/database/db_manager.c` + `.h`
- `src/database/db_init.sql`
- `src/common/crypto.c` + `.h`
- `tests/test_db.c`

### Testing Milestone
- Database created with schema: `sqlite3 test.db < src/database/db_init.sql`
- Test script inserts user, verifies password
- Login with correct credentials succeeds
- Login with wrong password fails
- Activity log entries appear in database
- Concurrent login attempts (10 threads) don't cause corruption

**Test Case:**
```c
// tests/test_db.c
void test_concurrent_writes() {
    Database* db = db_init("test.db");
    pthread_t threads[10];

    for (int i = 0; i < 10; i++) {
        pthread_create(&threads[i], NULL, write_log_entry, db);
    }

    for (int i = 0; i < 10; i++) {
        pthread_join(threads[i], NULL);
    }

    // Verify 10 entries in database
    assert(count_logs(db) == 10);
}
```

---

## Phase 4: File Operations (No Permissions)

**Duration:** 1.5 weeks
**Dependencies:** Phase 3
**Objective:** LIST_DIR, MKDIR, UPLOAD, DOWNLOAD commands (owner-only access)

### Implementation Tasks

#### 4.1 Virtual File System Utilities (`src/server/vfs.c`)
```c
typedef struct {
    int id;
    int parent_id;
    char name[256];
    char physical_path[37];  // UUID
    int owner_id;
    long size;
    int is_directory;
    int permissions;
} FileEntry;

int vfs_create_directory(Database* db, int parent_id, const char* name, int owner_id);
int vfs_list_files(Database* db, int parent_id, FileEntry** entries, int* count);
int vfs_get_file_by_id(Database* db, int file_id, FileEntry* entry);
int vfs_resolve_path(Database* db, const char* path, int current_dir, int* file_id);
```

**Path Resolution Logic:**
```c
// Example: "/home/alice/docs" from current_dir = 5
// 1. Split path by '/'
// 2. For each component, query: SELECT id FROM files WHERE parent_id=? AND name=?
// 3. Return final ID or -1 if not found
```

#### 4.2 Physical Storage Manager (`src/server/storage.c`)
```c
char* storage_get_path(const char* uuid);
int storage_write_file(const char* uuid, const uint8_t* data, size_t size);
int storage_read_file(const char* uuid, uint8_t** data, size_t* size);
int storage_delete_file(const char* uuid);
```

**Implementation:**
```c
char* storage_get_path(const char* uuid) {
    // Returns "storage/<first_2_chars>/<uuid>"
    // Example: "storage/a1/a1b2c3d4-..."
    char* path = malloc(256);
    snprintf(path, 256, "storage/%c%c/%s", uuid[0], uuid[1], uuid);
    return path;
}
```

**Directory Creation:**
- In `main()`, create `storage/` + subdirectories `00/` through `ff/`

#### 4.3 Command Handlers

##### LIST_DIR (CMD_LIST_DIR = 0x10)
**Client Payload:** `{"directory_id": 5}`
**Server Response:** Array of FileEntry structs as JSON

```c
void handle_list_dir(ClientSession* session, Packet* pkt) {
    if (!session->authenticated) {
        send_error(session, "Not authenticated");
        return;
    }

    cJSON* json = cJSON_Parse(pkt->payload);
    int dir_id = json_get_int(json, "directory_id");

    FileEntry* entries;
    int count;
    vfs_list_files(global_db, dir_id, &entries, &count);

    cJSON* response_json = cJSON_CreateArray();
    for (int i = 0; i < count; i++) {
        cJSON* item = cJSON_CreateObject();
        cJSON_AddNumberToObject(item, "id", entries[i].id);
        cJSON_AddStringToObject(item, "name", entries[i].name);
        cJSON_AddBoolToObject(item, "is_directory", entries[i].is_directory);
        cJSON_AddNumberToObject(item, "size", entries[i].size);
        cJSON_AddItemToArray(response_json, item);
    }

    char* payload = cJSON_PrintUnformatted(response_json);
    send_response(session, CMD_LIST_DIR, payload);

    free(entries);
    free(payload);
    cJSON_Delete(json);
    cJSON_Delete(response_json);
}
```

##### MKDIR (CMD_MAKE_DIR = 0x12)
**Client Payload:** `{"parent_id": 0, "name": "Documents"}`
**Server Response:** `{"status": "OK", "id": 123}`

```c
void handle_mkdir(ClientSession* session, Packet* pkt) {
    cJSON* json = cJSON_Parse(pkt->payload);
    int parent_id = json_get_int(json, "parent_id");
    char* name = json_get_string(json, "name");

    int new_id = vfs_create_directory(global_db, parent_id, name, session->user_id);

    cJSON* response = cJSON_CreateObject();
    if (new_id > 0) {
        cJSON_AddStringToObject(response, "status", "OK");
        cJSON_AddNumberToObject(response, "id", new_id);
        db_log_activity(global_db, session->user_id, "MKDIR", name);
    } else {
        cJSON_AddStringToObject(response, "status", "FAIL");
    }

    char* payload = cJSON_PrintUnformatted(response);
    send_response(session, CMD_MAKE_DIR, payload);

    free(payload);
    cJSON_Delete(json);
    cJSON_Delete(response);
}
```

##### UPLOAD (Two-phase: CMD_UPLOAD_REQ + CMD_UPLOAD_DATA)

**Phase 1: Metadata (CMD_UPLOAD_REQ = 0x20)**
```c
void handle_upload_request(ClientSession* session, Packet* pkt) {
    cJSON* json = cJSON_Parse(pkt->payload);
    char* filename = json_get_string(json, "name");
    long size = json_get_int(json, "size");
    int parent_id = json_get_int(json, "parent_id");

    // Generate UUID for physical storage
    char* uuid = generate_uuid();

    // Create database entry
    sqlite3_stmt* stmt;
    pthread_mutex_lock(&global_db->mutex);
    sqlite3_prepare_v2(global_db->conn,
        "INSERT INTO files (parent_id, name, physical_path, owner_id, size) VALUES (?, ?, ?, ?, ?)",
        -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, parent_id);
    sqlite3_bind_text(stmt, 2, filename, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, uuid, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 4, session->user_id);
    sqlite3_bind_int64(stmt, 5, size);
    sqlite3_step(stmt);
    int file_id = sqlite3_last_insert_rowid(global_db->conn);
    sqlite3_finalize(stmt);
    pthread_mutex_unlock(&global_db->mutex);

    // Store UUID in session for upcoming data transfer
    session->pending_upload_uuid = strdup(uuid);
    session->pending_upload_size = size;

    cJSON* response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "status", "READY");
    cJSON_AddNumberToObject(response, "file_id", file_id);

    char* payload = cJSON_PrintUnformatted(response);
    send_response(session, CMD_UPLOAD_REQ, payload);

    free(uuid);
    free(payload);
    cJSON_Delete(json);
    cJSON_Delete(response);
}
```

**Phase 2: Data Transfer (CMD_UPLOAD_DATA = 0x21)**
```c
void handle_upload_data(ClientSession* session, Packet* pkt) {
    if (!session->pending_upload_uuid) {
        send_error(session, "No upload in progress");
        return;
    }

    // Write binary payload to storage
    int result = storage_write_file(session->pending_upload_uuid,
                                   (uint8_t*)pkt->payload,
                                   pkt->data_length);

    cJSON* response = cJSON_CreateObject();
    if (result == 0) {
        cJSON_AddStringToObject(response, "status", "OK");
        db_log_activity(global_db, session->user_id, "UPLOAD", session->pending_upload_uuid);
    } else {
        cJSON_AddStringToObject(response, "status", "FAIL");
    }

    char* payload = cJSON_PrintUnformatted(response);
    send_response(session, CMD_UPLOAD_DATA, payload);

    free(session->pending_upload_uuid);
    session->pending_upload_uuid = NULL;
    free(payload);
    cJSON_Delete(response);
}
```

##### DOWNLOAD (CMD_DOWNLOAD_REQ = 0x30)
**Client Payload:** `{"file_id": 45}`
**Server Response:** Binary file data

```c
void handle_download(ClientSession* session, Packet* pkt) {
    cJSON* json = cJSON_Parse(pkt->payload);
    int file_id = json_get_int(json, "file_id");

    FileEntry entry;
    if (vfs_get_file_by_id(global_db, file_id, &entry) < 0) {
        send_error(session, "File not found");
        return;
    }

    if (entry.is_directory) {
        send_error(session, "Cannot download directory");
        return;
    }

    uint8_t* data;
    size_t size;
    storage_read_file(entry.physical_path, &data, &size);

    Packet response;
    response.command = CMD_DOWNLOAD_REQ;
    response.data_length = size;
    response.payload = (char*)data;

    uint8_t* buffer = malloc(7 + size);
    int len = encode_packet(&response, buffer, 7 + size);
    send(session->client_socket, buffer, len, 0);

    db_log_activity(global_db, session->user_id, "DOWNLOAD", entry.name);

    free(data);
    free(buffer);
    cJSON_Delete(json);
}
```

### Files to Create
- `src/server/vfs.c` + `.h`
- `src/server/storage.c` + `.h`
- `src/server/commands.c` (expand with new handlers)
- `tests/test_vfs.c`
- `tests/integration/test_upload.sh`

### Testing Milestone
- Create directory: Appears in database with correct parent_id
- List root directory: Returns empty array initially
- Upload 1KB file: Physical file exists in `storage/`, database entry created
- Download file: Binary content matches upload
- Upload 10MB file: Transfer completes without corruption (MD5 checksum)
- Concurrent uploads (5 threads): No database locks or file conflicts

**Integration Test Script:**
```bash
#!/bin/bash
# tests/integration/test_file_ops.sh

# Start server
./build/server &
SERVER_PID=$!
sleep 1

# Test sequence using custom client or curl-like tool
echo "Creating directory..."
./build/test_client login admin admin
./build/test_client mkdir 0 "TestFolder"

echo "Uploading file..."
./build/test_client upload ./tests/fixtures/sample.txt 1

echo "Listing directory..."
./build/test_client list 1

echo "Downloading file..."
./build/test_client download 1 ./downloaded.txt

# Verify
diff ./tests/fixtures/sample.txt ./downloaded.txt
if [ $? -eq 0 ]; then
    echo "SUCCESS: File upload/download cycle preserved content"
else
    echo "FAIL: Content mismatch"
fi

kill $SERVER_PID
```

---

## Phase 5: Permission System

**Duration:** 1 week
**Dependencies:** Phase 4
**Objective:** Enforce Linux-style RWX permissions on file operations

### Implementation Tasks

#### 5.1 Permission Logic (`src/server/permissions.c`)
```c
#define PERM_READ    4
#define PERM_WRITE   2
#define PERM_EXECUTE 1

typedef enum {
    ACCESS_READ,
    ACCESS_WRITE,
    ACCESS_EXECUTE
} AccessType;

int check_permission(Database* db, int user_id, int file_id, AccessType access) {
    FileEntry entry;
    vfs_get_file_by_id(db, file_id, &entry);

    // Owner check
    if (entry.owner_id == user_id) {
        int owner_perm = (entry.permissions >> 6) & 0x7;  // Extract owner bits
        return has_access(owner_perm, access);
    }

    // TODO: Group permissions (Phase 6 enhancement)
    // For now, check "others" permissions
    int other_perm = entry.permissions & 0x7;
    return has_access(other_perm, access);
}

int has_access(int perm_bits, AccessType access) {
    switch (access) {
        case ACCESS_READ:    return (perm_bits & PERM_READ) != 0;
        case ACCESS_WRITE:   return (perm_bits & PERM_WRITE) != 0;
        case ACCESS_EXECUTE: return (perm_bits & PERM_EXECUTE) != 0;
    }
    return 0;
}
```

#### 5.2 Update Command Handlers

**Modify `handle_list_dir`:**
```c
void handle_list_dir(ClientSession* session, Packet* pkt) {
    int dir_id = ...;

    // Check READ permission on directory
    if (!check_permission(global_db, session->user_id, dir_id, ACCESS_READ)) {
        send_error(session, "Permission denied");
        return;
    }

    // ... rest of listing logic
}
```

**Modify `handle_mkdir`:**
```c
void handle_mkdir(ClientSession* session, Packet* pkt) {
    int parent_id = ...;

    // Check WRITE permission on parent directory
    if (!check_permission(global_db, session->user_id, parent_id, ACCESS_WRITE)) {
        send_error(session, "Permission denied");
        return;
    }

    // Set default permissions for new directory (755)
    int new_id = vfs_create_directory(...);
}
```

**Modify `handle_upload_request`:**
```c
void handle_upload_request(ClientSession* session, Packet* pkt) {
    int parent_id = ...;

    if (!check_permission(global_db, session->user_id, parent_id, ACCESS_WRITE)) {
        send_error(session, "Permission denied");
        return;
    }

    // Default file permissions: 644
    // ... upload logic
}
```

**Modify `handle_download`:**
```c
void handle_download(ClientSession* session, Packet* pkt) {
    int file_id = ...;

    if (!check_permission(global_db, session->user_id, file_id, ACCESS_READ)) {
        send_error(session, "Permission denied");
        return;
    }

    // ... download logic
}
```

#### 5.3 CHMOD Command (CMD_CHMOD = 0x40)
**Client Payload:** `{"file_id": 10, "permissions": 755}`

```c
void handle_chmod(ClientSession* session, Packet* pkt) {
    cJSON* json = cJSON_Parse(pkt->payload);
    int file_id = json_get_int(json, "file_id");
    int new_perm = json_get_int(json, "permissions");

    FileEntry entry;
    vfs_get_file_by_id(global_db, file_id, &entry);

    // Only owner can change permissions
    if (entry.owner_id != session->user_id) {
        send_error(session, "Not owner");
        return;
    }

    // Update database
    pthread_mutex_lock(&global_db->mutex);
    sqlite3_exec(global_db->conn,
        "UPDATE files SET permissions = ? WHERE id = ?", ...);
    pthread_mutex_unlock(&global_db->mutex);

    send_success(session, CMD_CHMOD);
    db_log_activity(global_db, session->user_id, "CHMOD", entry.name);

    cJSON_Delete(json);
}
```

#### 5.4 Enhanced Logging
Add permission check failures to activity log:
```c
db_log_activity(global_db, session->user_id, "ACCESS_DENIED",
               "Attempted to read file ID 45");
```

### Files to Create
- `src/server/permissions.c` + `.h`
- `src/common/protocol.h` (add CMD_CHMOD)
- `tests/test_permissions.c`

### Testing Milestone
- User A uploads file (644 permissions)
- User B attempts to download: Success (read allowed)
- User B attempts to delete: Fail (write denied)
- User A changes permissions to 600
- User B attempts to download: Fail (read denied)
- Directory with 755: User B can list (execute) but not create files (write)
- Activity log shows all permission denial events

**Test Scenario:**
```c
// Create two user sessions
session_alice = login("alice", "pass1");
session_bob = login("bob", "pass2");

// Alice creates file with 644
file_id = upload(session_alice, "secret.txt", 644);

// Bob reads: Should succeed
assert(download(session_bob, file_id) == SUCCESS);

// Bob writes: Should fail
assert(chmod(session_bob, file_id, 777) == PERMISSION_DENIED);

// Alice changes to 600
chmod(session_alice, file_id, 600);

// Bob reads: Should fail
assert(download(session_bob, file_id) == PERMISSION_DENIED);
```

---

## Phase 6: Client Application (GUI)

**Duration:** 2 weeks
**Dependencies:** Phase 5 (server fully functional)
**Objective:** GTK-based GUI client with file browser, upload/download, recursive folder handling

### Implementation Tasks

#### 6.1 Network Layer (`src/client/net_handler.c`)
```c
typedef struct {
    int socket_fd;
    char* server_ip;
    int server_port;
    int authenticated;
    char session_token[256];
} Connection;

Connection* net_connect(const char* ip, int port);
int net_send_packet(Connection* conn, Packet* pkt);
int net_recv_packet(Connection* conn, Packet* pkt);
void net_disconnect(Connection* conn);

// High-level API
int net_login(Connection* conn, const char* username, const char* password);
FileEntry* net_list_directory(Connection* conn, int dir_id, int* count);
int net_mkdir(Connection* conn, int parent_id, const char* name);
int net_upload_file(Connection* conn, const char* local_path, int parent_id);
int net_download_file(Connection* conn, int file_id, const char* local_path);
```

**Upload Implementation:**
```c
int net_upload_file(Connection* conn, const char* local_path, int parent_id) {
    // Phase 1: Send metadata
    FILE* f = fopen(local_path, "rb");
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    cJSON* metadata = cJSON_CreateObject();
    cJSON_AddStringToObject(metadata, "name", basename(local_path));
    cJSON_AddNumberToObject(metadata, "size", size);
    cJSON_AddNumberToObject(metadata, "parent_id", parent_id);

    Packet req;
    req.command = CMD_UPLOAD_REQ;
    req.payload = cJSON_PrintUnformatted(metadata);
    req.data_length = strlen(req.payload);

    net_send_packet(conn, &req);

    Packet resp;
    net_recv_packet(conn, &resp);
    // Check for "READY" status

    // Phase 2: Send binary data
    uint8_t* file_data = malloc(size);
    fread(file_data, 1, size, f);
    fclose(f);

    Packet data;
    data.command = CMD_UPLOAD_DATA;
    data.payload = (char*)file_data;
    data.data_length = size;

    net_send_packet(conn, &data);

    free(file_data);
    free(req.payload);
    cJSON_Delete(metadata);

    return 0;
}
```

#### 6.2 Recursive Folder Upload
```c
int net_upload_folder(Connection* conn, const char* local_path, int parent_id) {
    DIR* dir = opendir(local_path);
    struct dirent* entry;

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char full_path[512];
        snprintf(full_path, sizeof(full_path), "%s/%s", local_path, entry->d_name);

        if (entry->d_type == DT_DIR) {
            // Create remote directory
            int new_dir_id = net_mkdir(conn, parent_id, entry->d_name);

            // Recurse
            net_upload_folder(conn, full_path, new_dir_id);
        } else {
            // Upload file
            net_upload_file(conn, full_path, parent_id);
        }
    }

    closedir(dir);
    return 0;
}
```

#### 6.3 GTK GUI (`src/client/gui.c`)

**Makefile addition:**
```makefile
CFLAGS += `pkg-config --cflags gtk+-3.0`
LDFLAGS += `pkg-config --libs gtk+-3.0`
```

**UI Layout:**
```
+--------------------------------------------------+
| File Sharing Client                         [X]  |
+--------------------------------------------------+
| Server: [localhost] Port: [8080] [Connect]       |
| User: [admin] Pass: [****] [Login]               |
+--------------------------------------------------+
| Current Path: /home/alice/documents              |
| [Up] [New Folder] [Upload File] [Upload Folder]  |
+--------------------------------------------------+
| Name          | Type    | Size   | Permissions   |
|----------------------------------------------------
| Documents     | Folder  | -      | rwxr-xr-x     |
| report.pdf    | File    | 2.5 MB | rw-r--r--     |
| photos        | Folder  | -      | rwxr-x---     |
+--------------------------------------------------+
| [Download] [Delete] [Properties] [Refresh]       |
+--------------------------------------------------+
| Status: Connected as admin                       |
+--------------------------------------------------+
```

**Main Window Code:**
```c
typedef struct {
    GtkWidget* window;
    GtkWidget* tree_view;
    GtkWidget* path_label;
    Connection* conn;
    int current_directory;
} AppState;

void on_login_clicked(GtkButton* button, AppState* state) {
    const char* username = gtk_entry_get_text(GTK_ENTRY(state->username_entry));
    const char* password = gtk_entry_get_text(GTK_ENTRY(state->password_entry));

    if (net_login(state->conn, username, password) == 0) {
        gtk_label_set_text(GTK_LABEL(state->status_label), "Connected");
        refresh_file_list(state);
    } else {
        show_error_dialog("Login failed");
    }
}

void refresh_file_list(AppState* state) {
    int count;
    FileEntry* entries = net_list_directory(state->conn, state->current_directory, &count);

    GtkListStore* store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(state->tree_view)));
    gtk_list_store_clear(store);

    for (int i = 0; i < count; i++) {
        GtkTreeIter iter;
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
                          0, entries[i].name,
                          1, entries[i].is_directory ? "Folder" : "File",
                          2, format_size(entries[i].size),
                          3, format_permissions(entries[i].permissions),
                          -1);
    }

    free(entries);
}

void on_upload_file_clicked(GtkButton* button, AppState* state) {
    GtkWidget* dialog = gtk_file_chooser_dialog_new("Select File",
                                                    GTK_WINDOW(state->window),
                                                    GTK_FILE_CHOOSER_ACTION_OPEN,
                                                    "_Cancel", GTK_RESPONSE_CANCEL,
                                                    "_Upload", GTK_RESPONSE_ACCEPT,
                                                    NULL);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

        // Show progress dialog
        GtkWidget* progress = show_progress_dialog("Uploading...");

        net_upload_file(state->conn, filename, state->current_directory);

        gtk_widget_destroy(progress);
        refresh_file_list(state);

        g_free(filename);
    }

    gtk_widget_destroy(dialog);
}
```

#### 6.4 Progress Bars for Large Transfers
Modify `net_upload_file` to accept callback:
```c
typedef void (*ProgressCallback)(int bytes_sent, int total_bytes);

int net_upload_file_ex(Connection* conn, const char* path, int parent_id, ProgressCallback cb) {
    // ... metadata phase ...

    // Send data in chunks
    size_t chunk_size = 8192;
    size_t total_sent = 0;

    while (total_sent < size) {
        size_t to_send = (size - total_sent < chunk_size) ? size - total_sent : chunk_size;
        send(conn->socket_fd, file_data + total_sent, to_send, 0);
        total_sent += to_send;

        if (cb) cb(total_sent, size);
    }
}
```

### Files to Create
- `src/client/net_handler.c` + `.h`
- `src/client/gui.c` + `.h`
- `src/client/main.c`
- `src/client/Makefile`

### Testing Milestone
- GUI launches and connects to server
- Login with credentials: Tree view populates with root directory
- Double-click folder: Navigates into directory
- Upload 50MB file: Progress bar updates smoothly
- Upload folder with 100 files: All files appear on server
- Download file: Saves to local disk with correct content
- Permission denied: Shows error dialog
- Server offline: Connection error handled gracefully

**User Acceptance Test:**
```
1. Launch client, connect to localhost:8080
2. Login as admin/admin
3. Create folder "TestFolder"
4. Upload file "document.pdf" to TestFolder
5. Change permissions to 600
6. Logout and login as different user
7. Attempt to download: Verify error message
8. Logout and login as admin
9. Download file: Verify content matches original
```

---

## Build System Implementation

### Root Makefile
```makefile
# Compiler and Flags
CC = gcc
CFLAGS = -Wall -Wextra -pthread -Isrc/common -Ilib/cJSON `pkg-config --cflags gtk+-3.0`
LDFLAGS = -pthread -lsqlite3 -lcrypto `pkg-config --libs gtk+-3.0`

# Directories
SRC_DIR = src
BUILD_DIR = build
TEST_DIR = tests

# Targets
all: server client tests

server:
	$(MAKE) -C $(SRC_DIR)/common
	$(MAKE) -C $(SRC_DIR)/database
	$(MAKE) -C $(SRC_DIR)/server
	mkdir -p $(BUILD_DIR)
	$(CC) -o $(BUILD_DIR)/server \
		$(SRC_DIR)/server/*.o \
		$(SRC_DIR)/common/*.o \
		$(SRC_DIR)/database/*.o \
		$(LDFLAGS)

client:
	$(MAKE) -C $(SRC_DIR)/common
	$(MAKE) -C $(SRC_DIR)/client
	mkdir -p $(BUILD_DIR)
	$(CC) -o $(BUILD_DIR)/client \
		$(SRC_DIR)/client/*.o \
		$(SRC_DIR)/common/*.o \
		$(LDFLAGS)

tests:
	$(MAKE) -C $(TEST_DIR)
	mkdir -p $(BUILD_DIR)/tests
	$(CC) -o $(BUILD_DIR)/tests/run_tests \
		$(TEST_DIR)/*.o \
		$(SRC_DIR)/common/*.o \
		$(SRC_DIR)/database/*.o \
		$(LDFLAGS)

clean:
	rm -rf $(BUILD_DIR)
	find $(SRC_DIR) -name "*.o" -delete
	find $(TEST_DIR) -name "*.o" -delete
	rm -f *.db *.log

run-server: server
	mkdir -p storage
	./$(BUILD_DIR)/server

run-client: client
	./$(BUILD_DIR)/client

test: tests
	./$(BUILD_DIR)/tests/run_tests

valgrind-server: server
	valgrind --leak-check=full --track-origins=yes ./$(BUILD_DIR)/server

.PHONY: all server client tests clean run-server run-client test valgrind-server
```

### Module Makefiles (Example: src/server/Makefile)
```makefile
CC = gcc
CFLAGS = -Wall -Wextra -pthread -I../common -I../../lib/cJSON

OBJS = main.o socket_mgr.o thread_pool.o commands.o vfs.o storage.o permissions.o

all: $(OBJS)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o

.PHONY: all clean
```

---

## Testing Strategy Summary

### Phase-Level Tests

| Phase | Test Type | Coverage |
|-------|-----------|----------|
| 0 | Unit | Protocol encode/decode, UUID generation |
| 1 | Integration | Multi-client connections, graceful shutdown |
| 2 | Unit | JSON parsing, command dispatching |
| 3 | Unit | Database CRUD, concurrent writes |
| 4 | Integration | File upload/download cycle, directory listing |
| 5 | Integration | Permission enforcement across all commands |
| 6 | UI/Acceptance | End-to-end user workflows |

### Continuous Integration (GitHub Actions)
```yaml
name: Build and Test

on: [push, pull_request]

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - name: Install dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y libsqlite3-dev libssl-dev libgtk-3-dev
      - name: Build
        run: make all
      - name: Run tests
        run: make test
      - name: Check for memory leaks
        run: |
          valgrind --leak-check=full --error-exitcode=1 ./build/tests/run_tests
```

---

## Risk Mitigation

### Identified Risks

1. **Race Conditions in SQLite**
   - **Mitigation:** Mutex wraps all database operations, test with Thread Sanitizer
   - **Test:** `tests/test_concurrent_db.c` spawns 50 threads writing simultaneously

2. **Large File Memory Exhaustion**
   - **Mitigation:** Chunked transfer (8KB buffers), streaming writes to disk
   - **Test:** Upload 1GB file, monitor RSS with `ps`

3. **Client Timeout on Slow Networks**
   - **Mitigation:** Implement `SO_RCVTIMEO` and `SO_SNDTIMEO` socket options
   - **Test:** Simulate slow network with `tc qdisc` (Linux traffic control)

4. **Incomplete Folder Uploads (Crash Mid-Transfer)**
   - **Mitigation:** Transaction-like batch inserts, rollback on error
   - **Test:** Kill client during upload, verify database consistency

5. **Path Traversal Attacks (e.g., `../../etc/passwd`)**
   - **Mitigation:** Validate all paths against database-resolved IDs, reject `..`
   - **Test:** Attempt to upload file with `../` in name, verify rejection

---

## Documentation Requirements

### Per-Phase Documentation

- **Phase 0:** README.md with build instructions
- **Phase 2:** `docs/protocol_spec.md` with command format and examples
- **Phase 4:** `docs/api_reference.md` with function signatures for VFS
- **Phase 6:** User manual with screenshots (in `docs/user_guide.pdf`)

### Code Comments
- Every public function: Doxygen-style header
  ```c
  /**
   * @brief Creates a new directory in the virtual file system
   * @param db Database connection
   * @param parent_id ID of parent directory (0 for root)
   * @param name Directory name (max 255 chars)
   * @param owner_id User ID of creator
   * @return ID of new directory, or -1 on error
   */
  int vfs_create_directory(Database* db, int parent_id, const char* name, int owner_id);
  ```

---

## Success Criteria

### Phase Completion Checklist

Each phase is considered complete when:
- [ ] All files listed in "Files to Create" exist and compile
- [ ] All tests in "Testing Milestone" pass
- [ ] `valgrind` reports 0 memory leaks
- [ ] Code reviewed (self-review against SOLID principles)
- [ ] Documentation updated

### Project Completion Criteria
- [ ] Client can upload/download files with progress indication
- [ ] Permissions correctly enforce read/write restrictions
- [ ] Server handles 10 concurrent clients without crashes
- [ ] Activity log records all user actions
- [ ] No compiler warnings with `-Wall -Wextra`
- [ ] User guide written and reviewed

---

## Timeline Estimate

| Phase | Duration | Dependencies | Parallel Possible |
|-------|----------|--------------|-------------------|
| Phase 0 | 3-5 days | None | - |
| Phase 1 | 1 week | Phase 0 | - |
| Phase 2 | 1 week | Phase 1 | - |
| Phase 3 | 1 week | Phase 1 | Yes (after Phase 1) |
| Phase 4 | 1.5 weeks | Phase 2, 3 | - |
| Phase 5 | 1 week | Phase 4 | - |
| Phase 6 | 2 weeks | Phase 5 | - |
| **Total** | **8-10 weeks** | | |

**Notes:**
- Phase 3 (Database) can begin immediately after Phase 1 (sockets working)
- Buffer 20% time for debugging and integration issues
- User testing adds 1 week post-Phase 6

---

## Next Steps

1. **Immediate Actions (Week 1):**
   - Set up directory structure
   - Write root Makefile
   - Implement `src/common/protocol.h` with all command definitions
   - Create `src/database/db_init.sql`

2. **Week 2-3:**
   - Complete Phase 1 (Core Server)
   - Run multi-client stress tests

3. **Week 4-5:**
   - Complete Phase 2 (Protocol)
   - Parallel: Begin Phase 3 (Database)

4. **Week 6-8:**
   - Complete Phase 4 (File Ops)
   - Complete Phase 5 (Permissions)

5. **Week 9-10:**
   - Complete Phase 6 (Client GUI)
   - User acceptance testing

---

## Unresolved Questions

1. **Group Permissions:** Should we implement group-based access (like Linux groups), or only owner/others?
   - **Recommendation:** Defer to Phase 7 (post-MVP)

2. **File Versioning:** Track multiple versions of same file (like Dropbox)?
   - **Recommendation:** Out of scope for current design

3. **Encryption:** Should file transfers be encrypted (TLS)?
   - **Recommendation:** Add in Phase 7 using OpenSSL (would require protocol update)

4. **Quota Management:** Limit storage per user?
   - **Recommendation:** Add `quota_bytes` column to `users` table in Phase 7

5. **Web Interface:** Should there be an HTTP-based client in addition to GTK?
   - **Recommendation:** Separate project; server protocol remains the same

---

**Plan Status:** Ready for Implementation
**Approval Required From:** Project Stakeholder
**Risk Level:** Medium (complexity in concurrency and permissions)
**Estimated Effort:** 320-400 developer hours
