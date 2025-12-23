# File Sharing Protocol Specification

## Overview
Binary protocol for client-server file sharing system using TCP sockets.

## Packet Format

### Header Structure (7 bytes)
```
+----------------+----------------+------------------+
| Magic (2 bytes)| Command (1 byte)| Length (4 bytes) |
+----------------+----------------+------------------+
```

- **Magic Bytes**: 0xFA 0xCE (for protocol validation)
- **Command**: 1-byte command identifier
- **Length**: 4-byte unsigned integer (network byte order) - payload size

### Payload (Variable Length)
- Maximum size: 16 MB (16,777,216 bytes)
- Format: JSON-encoded data (using cJSON library)
- Empty for some commands

## Command Set

### Authentication Commands

#### LOGIN_REQ (0x01)
Client requests authentication.

**Payload:**
```json
{
  "username": "string",
  "password": "string"
}
```

#### LOGIN_RES (0x02)
Server responds with authentication result.

**Payload:**
```json
{
  "status": 0,           // 0=success, 2=auth_fail
  "session_token": "uuid",
  "user_id": 123,
  "message": "Login successful"
}
```

### Directory Operations

#### LIST_DIR (0x10)
List contents of a directory.

**Request Payload:**
```json
{
  "path": "/directory/path"
}
```

**Response Payload:**
```json
{
  "status": 0,
  "entries": [
    {
      "name": "file.txt",
      "is_directory": false,
      "size": 1024,
      "permissions": 644,
      "created_at": "2025-12-18 10:00:00"
    }
  ]
}
```

#### CHANGE_DIR (0x11)
Change current working directory.

**Payload:**
```json
{
  "path": "/new/directory"
}
```

#### MAKE_DIR (0x12)
Create new directory.

**Payload:**
```json
{
  "path": "/new/directory/name",
  "permissions": 755
}
```

### File Transfer Commands

#### UPLOAD_REQ (0x20)
Initiate file upload.

**Payload:**
```json
{
  "path": "/remote/path/file.txt",
  "size": 1048576,
  "permissions": 644
}
```

#### UPLOAD_DATA (0x21)
File data chunk during upload.

**Payload:** Raw binary data (not JSON)

#### DOWNLOAD_REQ (0x30)
Request file download.

**Payload:**
```json
{
  "path": "/remote/path/file.txt"
}
```

#### DOWNLOAD_RES (0x31)
Server sends file metadata before transfer.

**Payload:**
```json
{
  "status": 0,
  "size": 1048576,
  "permissions": 644,
  "name": "file.txt"
}
```

### File Management Commands

#### DELETE (0x40)
Delete file or directory.

**Payload:**
```json
{
  "path": "/path/to/delete"
}
```

#### CHMOD (0x41)
Change file permissions.

**Payload:**
```json
{
  "path": "/path/to/file",
  "permissions": 755
}
```

### Response Commands

#### SUCCESS (0xFE)
Generic success response.

**Payload:**
```json
{
  "message": "Operation completed successfully"
}
```

#### ERROR (0xFF)
Generic error response.

**Payload:**
```json
{
  "status": 1,           // Error code
  "message": "Error description"
}
```

## Status Codes

- `0` - STATUS_OK: Operation successful
- `1` - STATUS_ERROR: Generic error
- `2` - STATUS_AUTH_FAIL: Authentication failed
- `3` - STATUS_PERM_DENIED: Permission denied
- `4` - STATUS_NOT_FOUND: File/directory not found
- `5` - STATUS_EXISTS: File/directory already exists

## Connection Flow

1. Client connects to server TCP socket
2. Client sends LOGIN_REQ
3. Server validates credentials and returns LOGIN_RES with session token
4. Client uses session token in subsequent requests
5. Client sends operation commands
6. Server responds with appropriate response packets
7. Client disconnects when done

## Error Handling

- Invalid magic bytes: Connection terminated
- Payload exceeds MAX_PAYLOAD_SIZE: ERROR response
- Invalid command: ERROR response with STATUS_ERROR
- Network timeout: Connection terminated after 30 seconds idle

## Security Considerations

- Passwords transmitted in plain text (Phase 0)
- TODO Phase 1+: Implement TLS encryption
- TODO Phase 1+: Hash passwords using SHA-256
- Session tokens are UUID v4 format
