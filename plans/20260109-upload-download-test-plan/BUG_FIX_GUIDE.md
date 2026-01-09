# Download Bug Fix Guide

**Bug ID:** CRITICAL-001
**Component:** Server Download Handler
**Impact:** ALL downloads fail (0% success rate)
**Priority:** P0 - CRITICAL BLOCKER

---

## Quick Fix

### File to Edit
`src/server/commands.c`

### Function to Fix
`handle_download()` (lines 454-512)

### Current Code (BROKEN)
```c
// Line 502-505
// Send binary data with CMD_DOWNLOAD_RES
Packet* response = packet_create(CMD_DOWNLOAD_RES, (char*)data, size);
packet_send(session->client_socket, response);
packet_free(response);
```

### Fixed Code
```c
// STEP 1: Send metadata JSON first
cJSON* metadata = cJSON_CreateObject();
cJSON_AddNumberToObject(metadata, "size", size);
cJSON_AddStringToObject(metadata, "name", entry.name);

char* json_str = cJSON_PrintUnformatted(metadata);
Packet* meta_pkt = packet_create(CMD_DOWNLOAD_RES, json_str, strlen(json_str));
packet_send(session->client_socket, meta_pkt);

free(json_str);
packet_free(meta_pkt);
cJSON_Delete(metadata);

// STEP 2: Send file data
Packet* data_pkt = packet_create(CMD_DOWNLOAD_RES, (char*)data, size);
packet_send(session->client_socket, data_pkt);
packet_free(data_pkt);
```

---

## Complete Fixed Function

Replace the entire `handle_download()` function with this:

```c
void handle_download(ClientSession* session, Packet* pkt) {
    cJSON* json = cJSON_Parse(pkt->payload);
    if (!json) {
        send_error(session, "Invalid JSON");
        return;
    }

    cJSON* file_id_item = cJSON_GetObjectItem(json, "file_id");
    if (!file_id_item) {
        send_error(session, "Missing 'file_id' parameter");
        cJSON_Delete(json);
        return;
    }

    int file_id = file_id_item->valueint;

    // Check READ permission on file
    if (!check_permission(global_db, session->user_id, file_id, ACCESS_READ)) {
        send_error(session, "Permission denied");
        db_log_activity(global_db, session->user_id, "ACCESS_DENIED", "DOWNLOAD");
        cJSON_Delete(json);
        return;
    }

    // Get file entry from database
    FileEntry entry;
    if (db_get_file_by_id(global_db, file_id, &entry) < 0) {
        send_error(session, "File not found");
        cJSON_Delete(json);
        return;
    }

    // Check it's not a directory
    if (entry.is_directory) {
        send_error(session, "Cannot download a directory");
        cJSON_Delete(json);
        return;
    }

    // Read file from storage
    uint8_t* data = NULL;
    size_t size = 0;
    if (storage_read_file(entry.physical_path, &data, &size) < 0) {
        send_error(session, "Failed to read file from storage");
        cJSON_Delete(json);
        return;
    }

    // FIXED: Send metadata JSON first
    cJSON* metadata = cJSON_CreateObject();
    cJSON_AddNumberToObject(metadata, "size", (double)size);
    cJSON_AddStringToObject(metadata, "name", entry.name);

    char* json_str = cJSON_PrintUnformatted(metadata);
    Packet* meta_pkt = packet_create(CMD_DOWNLOAD_RES, json_str, strlen(json_str));
    packet_send(session->client_socket, meta_pkt);

    free(json_str);
    packet_free(meta_pkt);
    cJSON_Delete(metadata);

    // FIXED: Then send file data
    Packet* data_pkt = packet_create(CMD_DOWNLOAD_RES, (char*)data, size);
    packet_send(session->client_socket, data_pkt);
    packet_free(data_pkt);

    free(data);
    cJSON_Delete(json);

    db_log_activity(global_db, session->user_id, "DOWNLOAD", entry.name);
    log_info("Download completed: file_id=%d, name=%s, size=%zu", file_id, entry.name, size);
}
```

---

## Why This Fix Works

### Problem
Client expects 2-step protocol:
1. Metadata packet: `{"size": 14, "name": "file.txt"}`
2. Data packets: Binary file content

Server was sending data directly, client tried to parse binary as JSON → failed.

### Solution
Match client expectations:
1. Create JSON metadata
2. Send as CMD_DOWNLOAD_RES packet
3. Send file data as separate CMD_DOWNLOAD_RES packet

Client code (`src/client/client.c:335-392`) parses JSON, extracts size, then calls `net_recv_file()` to receive data.

---

## Testing After Fix

### 1. Rebuild
```bash
cd /Users/minhbohung111/workspace/projects/networkFinal
make clean
make all
```

### 2. Restart Server
```bash
# Stop old server
pkill -f "./build/server"

# Start new server
./build/server 8080
```

### 3. Test Download
```bash
# Upload test file
echo "admin
admin
upload plans/20260109-upload-download-test-plan/test_data/upload/test_small.txt
ls
quit" | ./build/client localhost 8080

# Note the file ID from ls output (e.g., 26)

# Download test file
echo "admin
admin
download 26 /tmp/downloaded_test.txt
quit" | ./build/client localhost 8080

# Verify file downloaded
cat /tmp/downloaded_test.txt
# Should output: "Hello, World!"
```

### 4. Verify Integrity
```bash
# Compare checksums
sha256sum plans/20260109-upload-download-test-plan/test_data/upload/test_small.txt
sha256sum /tmp/downloaded_test.txt
# Checksums should match
```

---

## Verification Checklist

After applying fix, verify:

- [ ] Server compiles without errors
- [ ] Small file (14B) downloads successfully
- [ ] Medium file (1KB) downloads successfully
- [ ] Large file (10MB) downloads successfully
- [ ] Binary file (PNG) downloads successfully
- [ ] Downloaded files match original (checksum)
- [ ] Error message for non-existent file ID
- [ ] Error message for permission denied
- [ ] Error message for directory download attempt
- [ ] GUI client downloads work (if applicable)

---

## Related Files

**Modified:**
- `src/server/commands.c` - handle_download() function

**No Changes Needed:**
- `src/client/client.c` - client_download() is correct
- `src/client/net_handler.c` - net_recv_file() is correct
- `src/common/protocol.h` - CMD_DOWNLOAD_RES defined correctly

---

## Risk Assessment

**Risk Level:** LOW
- Small, isolated change (single function)
- No API changes needed
- No database changes needed
- No client changes needed
- Only fixes protocol implementation

**Rollback Plan:**
- Revert `src/server/commands.c` to previous version
- Rebuild and restart server

---

## Additional Improvements (Optional)

### 1. Add Download Progress
```c
// In handle_download(), after sending metadata:
for (size_t offset = 0; offset < size; ) {
    size_t chunk = (size - offset > 8192) ? 8192 : (size - offset);
    Packet* chunk_pkt = packet_create(CMD_DOWNLOAD_RES, (char*)(data + offset), chunk);
    packet_send(session->client_socket, chunk_pkt);
    packet_free(chunk_pkt);
    offset += chunk;
}
```

### 2. Better Error Messages
```c
// Replace generic errors with specific ones:
if (entry.owner_id != session->user_id && !(entry.permissions & 0044)) {
    send_error(session, "Permission denied: file not readable");
    return;
}

if (db_get_file_by_id(global_db, file_id, &entry) < 0) {
    char error_msg[128];
    snprintf(error_msg, sizeof(error_msg), "File not found: ID %d does not exist", file_id);
    send_error(session, error_msg);
    return;
}
```

---

## Estimated Time

- **Code Change:** 5 minutes
- **Build:** 2 minutes
- **Testing:** 15 minutes
- **Verification:** 10 minutes
- **Total:** ~30 minutes

---

## Success Criteria

✓ Client downloads file successfully
✓ Downloaded file matches original (checksum verified)
✓ No error messages in server log
✓ Client displays "Download successful!"
✓ File appears in destination path with correct size

---

**Author:** QA Tester Agent
**Date:** 2026-01-09
**Status:** Ready to implement
**Reviewed:** Pending developer review
