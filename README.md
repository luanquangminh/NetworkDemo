# Linux C File Sharing System

A client-server file sharing system implemented in C with TCP sockets, SQLite database, and thread pool architecture.

## Phase 0: Foundation & Build System

This is the initial foundation phase with:
- Complete directory structure
- Full protocol definitions
- Database schema
- Build system with Makefiles
- Stub implementations for all modules

## Prerequisites

- GCC compiler
- SQLite3 development libraries
- OpenSSL development libraries (libcrypto)
- POSIX threads (pthread)

### Installing Dependencies

**Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install build-essential libsqlite3-dev libssl-dev
```

**macOS:**
```bash
brew install sqlite3 openssl
```

**Fedora/RHEL:**
```bash
sudo dnf install gcc make sqlite-devel openssl-devel
```

## Project Structure

```
networkFinal/
├── Makefile                    # Root build orchestrator
├── README.md                   # This file
├── .gitignore                  # Build artifacts exclusions
├── docs/                       # Documentation
├── plans/                      # Project planning documents
├── src/
│   ├── server/                 # Server implementation
│   ├── client/                 # Client implementation
│   ├── common/                 # Shared protocol & utilities
│   └── database/               # Database management
├── tests/                      # Unit tests
├── storage/                    # Physical file storage
├── build/                      # Compiled binaries
└── lib/cJSON/                  # cJSON library
```

## Building

### Build Everything
```bash
make all
```

### Build Server Only
```bash
make server
```

### Build Client Only
```bash
make client
```

### Build Tests
```bash
make tests
```

### Clean Build Artifacts
```bash
make clean
```

## Running

### Start Server
```bash
make run-server
# Or manually:
./build/server 8080
```

### Start Client
```bash
make run-client
# Or manually:
./build/client localhost 8080
```

## Current Status

Phase 0 (Foundation) - COMPLETE
- [x] Directory structure
- [x] Protocol definitions (protocol.h)
- [x] Database schema (db_init.sql)
- [x] Build system (Makefiles)
- [x] Stub implementations
- [x] cJSON library integration

## Protocol Specification

### Packet Format
```
+----------------+----------------+------------------+
| Magic (2 bytes)| Command (1 byte)| Length (4 bytes) |
+----------------+----------------+------------------+
|              Payload (variable)                    |
+----------------------------------------------------+
```

- Magic Bytes: 0xFA 0xCE
- Max Payload: 16MB

### Commands
- `0x01` - Login Request
- `0x02` - Login Response
- `0x10` - List Directory
- `0x11` - Change Directory
- `0x12` - Make Directory
- `0x20` - Upload Request
- `0x21` - Upload Data
- `0x30` - Download Request
- `0x31` - Download Response
- `0x40` - Delete File
- `0x41` - Change Permissions
- `0xFE` - Success Response
- `0xFF` - Error Response

## Database Schema

### Tables
- **users**: User authentication and profiles
- **files**: Virtual file system (hierarchical structure)
- **activity_logs**: User activity tracking

Default admin credentials:
- Username: `admin`
- Password: `admin`

## Development

### Adding New Features
1. Update relevant header files in `src/*/`
2. Implement functions in corresponding `.c` files
3. Add tests in `tests/`
4. Update Makefiles if new files are added
5. Run `make clean && make all` to rebuild

### Code Organization
- **Common**: Protocol encoding/decoding, logging, utilities
- **Server**: Socket management, thread pool, request handling
- **Client**: Network I/O, user interface, file operations
- **Database**: SQLite wrapper, user/file/log management

## Testing

```bash
make tests
./tests/test_protocol
./tests/test_db
```

## Next Steps (Post Phase 0)

- Phase 1: Implement protocol encode/decode functions
- Phase 2: Implement basic server socket handling
- Phase 3: Implement client network operations
- Phase 4: Implement database operations
- Phase 5: Implement file transfer logic
- Phase 6: Add authentication and authorization
- Phase 7: Testing and optimization

## License

Educational project for network programming demonstration.
