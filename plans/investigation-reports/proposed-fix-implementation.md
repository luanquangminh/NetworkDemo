# Proposed Fix Implementation for Upload Orphan Bug

**Date:** 2026-01-09
**Issue:** Orphaned database entries when UPLOAD_DATA never received after UPLOAD_REQ

---

## Fix 1: Validate Physical File on Download (IMMEDIATE)

**Priority:** P0 - Critical
**Effort:** Low (15 minutes)
**Risk:** Low (defensive check only)

### Implementation

**File:** `src/server/commands.c`
**Function:** `handle_download()` (line 454)

```c
void handle_download(ClientSession* session, Packet* pkt) {
    // ... existing code until line 491 ...

    // Check it's not a directory
    if (entry.is_directory) {
        send_error(session, "Cannot download a directory");
        cJSON_Delete(json);
        return;
    }

    // NEW: Validate physical file exists before attempting read
    if (!storage_file_exists(entry.physical_path)) {
        log_error("Physical file missing for file_id=%d, path=%s (orphaned upload)",
                  file_id, entry.physical_path);
        send_error(session, "File data unavailable (incomplete upload)");
        db_log_activity(global_db, session->user_id, "DOWNLOAD_FAILED", "MISSING_FILE");
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
    // ... rest of function ...
}
```

**Testing:**
```bash
# Attempt to download orphaned file
# Expected: Meaningful error instead of "No such file or directory"
```

---

## Fix 2: Add Upload Timeout and Cleanup (SHORT-TERM)

**Priority:** P1 - High
**Effort:** Medium (1-2 hours)
**Risk:** Medium (requires testing timeout logic)

### Schema Changes

**File:** Database schema
```sql
-- Add fields to track upload state
ALTER TABLE files ADD COLUMN upload_status TEXT DEFAULT 'COMPLETE';
-- Values: PENDING, COMPLETE, FAILED

-- Track upload initiation time
ALTER TABLE files ADD COLUMN upload_started_at TEXT;
```

### Session Structure Update

**File:** `src/server/server.h`
```c
typedef struct {
    // ... existing fields ...

    char* pending_upload_uuid;
    long pending_upload_size;
    int pending_file_id;        // NEW: Track file_id for cleanup
    time_t upload_req_time;     // NEW: Track when request was made

    // ... rest of struct ...
} ClientSession;
```

### Upload Request Modification

**File:** `src/server/commands.c`
**Function:** `handle_upload_req()` (line 313)

```c
void handle_upload_req(ClientSession* session, Packet* pkt) {
    // ... existing validation code ...

    // Generate UUID for file storage
    char* uuid = generate_uuid();
    if (!uuid) {
        send_error(session, "Failed to generate UUID");
        cJSON_Delete(json);
        return;
    }

    // Create file entry in database with PENDING status
    int file_id = db_create_file_with_status(global_db, parent_id, name, uuid,
                                              session->user_id, size, 0, 0644,
                                              "PENDING", time(NULL));

    if (file_id < 0) {
        send_error(session, "Failed to create file entry");
        free(uuid);
        cJSON_Delete(json);
        return;
    }

    // Store upload metadata in session
    if (session->pending_upload_uuid) {
        free(session->pending_upload_uuid);
    }
    session->pending_upload_uuid = uuid;
    session->pending_upload_size = size;
    session->pending_file_id = file_id;      // NEW
    session->upload_req_time = time(NULL);   // NEW
    session->state = STATE_TRANSFERRING;

    // ... send READY response ...
}
```

### Upload Data Completion

**File:** `src/server/commands.c`
**Function:** `handle_upload_data()` (line 390)

```c
void handle_upload_data(ClientSession* session, Packet* pkt) {
    // ... existing validation and write code ...

    // Write file to storage
    if (storage_write_file(session->pending_upload_uuid,
                          (uint8_t*)pkt->payload,
                          pkt->data_length) < 0) {
        send_error(session, "Failed to write file to storage");

        // NEW: Mark upload as FAILED and cleanup
        db_update_file_status(global_db, session->pending_file_id, "FAILED");
        db_delete_file(global_db, session->pending_file_id);

        free(session->pending_upload_uuid);
        session->pending_upload_uuid = NULL;
        session->pending_file_id = 0;
        session->state = STATE_AUTHENTICATED;
        return;
    }

    // NEW: Mark upload as COMPLETE
    db_update_file_status(global_db, session->pending_file_id, "COMPLETE");

    // Log activity
    db_log_activity(global_db, session->user_id, "UPLOAD",
                   session->pending_upload_uuid);

    send_success(session, CMD_SUCCESS, "Upload successful");

    // Clear pending upload state
    free(session->pending_upload_uuid);
    session->pending_upload_uuid = NULL;
    session->pending_file_id = 0;
    session->state = STATE_AUTHENTICATED;
}
```

### Session Cleanup with Timeout

**File:** `src/server/server.c`
**Function:** New function `cleanup_orphaned_uploads()`

```c
#define UPLOAD_TIMEOUT 300  // 5 minutes

void cleanup_orphaned_uploads(ClientSession* session) {
    if (!session->pending_upload_uuid) {
        return;
    }

    time_t now = time(NULL);
    if (now - session->upload_req_time > UPLOAD_TIMEOUT) {
        log_warning("Upload timeout for file_id=%d, UUID=%s (waited %ld seconds)",
                    session->pending_file_id,
                    session->pending_upload_uuid,
                    now - session->upload_req_time);

        // Delete orphaned database entry
        db_delete_file(global_db, session->pending_file_id);

        // Clear session state
        free(session->pending_upload_uuid);
        session->pending_upload_uuid = NULL;
        session->pending_file_id = 0;
        session->state = STATE_AUTHENTICATED;
    }
}

// Call from client_handler or periodic cleanup thread
void* client_handler(void* arg) {
    ClientSession* session = (ClientSession*)arg;

    while (session->running) {
        // ... existing packet handling ...

        // NEW: Check for upload timeout
        cleanup_orphaned_uploads(session);

        // ... rest of handler ...
    }
}
```

---

## Fix 3: Database Cleanup Command (SHORT-TERM)

**Priority:** P2 - Medium
**Effort:** Low (30 minutes)
**Risk:** Low (admin-only command)

### Admin Command Implementation

**File:** `src/server/commands.c`
**New function:** `handle_admin_cleanup_orphaned()`

```c
void handle_admin_cleanup_orphaned(ClientSession* session, Packet* pkt) {
    // Verify admin privileges
    if (!session->is_admin) {
        send_error(session, "Admin privileges required");
        return;
    }

    // Get all file entries
    // For each: check if physical file exists
    // Report and optionally delete orphaned entries

    cJSON* response = cJSON_CreateObject();
    cJSON* orphaned_list = cJSON_CreateArray();

    int orphaned_count = 0;

    // Query database for all non-directory files
    sqlite3_stmt* stmt;
    const char* sql = "SELECT id, name, physical_path FROM files WHERE is_directory = 0";

    if (sqlite3_prepare_v2(global_db->conn, sql, -1, &stmt, NULL) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int file_id = sqlite3_column_int(stmt, 0);
            const char* name = (const char*)sqlite3_column_text(stmt, 1);
            const char* physical_path = (const char*)sqlite3_column_text(stmt, 2);

            if (physical_path && !storage_file_exists(physical_path)) {
                cJSON* orphan = cJSON_CreateObject();
                cJSON_AddNumberToObject(orphan, "file_id", file_id);
                cJSON_AddStringToObject(orphan, "name", name);
                cJSON_AddStringToObject(orphan, "uuid", physical_path);
                cJSON_AddItemToArray(orphaned_list, orphan);

                orphaned_count++;

                // Optional: Auto-delete
                cJSON* auto_delete = cJSON_GetObjectItem(cJSON_Parse(pkt->payload), "auto_delete");
                if (auto_delete && cJSON_IsTrue(auto_delete)) {
                    db_delete_file(global_db, file_id);
                    log_info("Deleted orphaned entry: file_id=%d, name=%s", file_id, name);
                }
            }
        }
        sqlite3_finalize(stmt);
    }

    cJSON_AddNumberToObject(response, "orphaned_count", orphaned_count);
    cJSON_AddItemToObject(response, "orphaned_files", orphaned_list);

    char* payload = cJSON_PrintUnformatted(response);
    send_success(session, CMD_SUCCESS, payload);

    free(payload);
    cJSON_Delete(response);

    log_info("Storage integrity check completed: found %d orphaned entries", orphaned_count);
}
```

---

## Fix 4: Periodic Integrity Check (LONG-TERM)

**Priority:** P3 - Low
**Effort:** Medium (2-3 hours)
**Risk:** Low (background task)

### Implementation

**File:** `src/server/main.c`
**New thread:** `integrity_check_thread()`

```c
#define INTEGRITY_CHECK_INTERVAL 86400  // 24 hours

void* integrity_check_thread(void* arg) {
    while (server_running) {
        sleep(INTEGRITY_CHECK_INTERVAL);

        log_info("Starting scheduled storage integrity check...");

        int orphaned_count = 0;
        int total_files = 0;

        // Similar logic to admin command but automated
        // Log findings, optionally send alerts

        log_info("Integrity check complete: %d/%d files orphaned",
                 orphaned_count, total_files);
    }

    return NULL;
}

// In main():
pthread_t integrity_thread;
pthread_create(&integrity_thread, NULL, integrity_check_thread, NULL);
```

---

## Testing Strategy

### Unit Tests
```c
// Test upload timeout
test_upload_timeout() {
    // 1. Send UPLOAD_REQ
    // 2. Wait > UPLOAD_TIMEOUT
    // 3. Verify DB entry deleted
}

// Test orphan detection
test_orphan_detection() {
    // 1. Create DB entry without physical file
    // 2. Run integrity check
    // 3. Verify orphan detected
}

// Test download validation
test_download_orphaned_file() {
    // 1. Create DB entry without physical file
    // 2. Attempt download
    // 3. Verify meaningful error message
}
```

### Integration Tests
```bash
# Test normal upload/download
# Test interrupted upload
# Test cleanup command
# Test integrity check
```

---

## Rollout Plan

### Phase 1: Immediate (Today)
1. Apply Fix 1 (download validation)
2. Clean up existing orphaned entries manually
3. Deploy to server

### Phase 2: Short-term (This week)
1. Implement Fix 2 (upload timeout)
2. Add schema changes
3. Test timeout logic
4. Deploy with monitoring

### Phase 3: Medium-term (Next week)
1. Implement Fix 3 (admin cleanup command)
2. Add to GUI admin dashboard
3. Deploy

### Phase 4: Long-term (Next month)
1. Implement Fix 4 (periodic checks)
2. Add monitoring alerts
3. Build admin reporting

---

## Success Metrics

**After Fix 1:**
- Zero "No such file or directory" errors in logs
- Users get clear "incomplete upload" message

**After Fix 2:**
- Zero new orphaned entries after 24 hours
- Upload timeout triggers logged

**After Fix 3:**
- Admin can detect and clean orphans on-demand
- Storage integrity reports available

**After Fix 4:**
- Automated detection of any new orphans
- Alerting for integrity issues
