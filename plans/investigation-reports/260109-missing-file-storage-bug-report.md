# Root Cause Analysis Report: Missing File Storage Bug

**Date:** 2026-01-09
**Issue ID:** File UUID 602a2c5f-424b-43c5-8ff5-5245dc77688d
**Severity:** Critical - Data Loss
**Status:** Root Cause Identified

---

## 1. Problem Summary

**Symptoms Observed:**
- User test1 attempts to download file `automated_client_test.sh` uploaded by test2
- Database contains file entry (ID: 23) with physical_path `602a2c5f-424b-43c5-8ff5-5245dc77688d`
- Physical file missing from `storage/60/602a2c5f-424b-43c5-8ff5-5245dc77688d`
- Download fails with: `Failed to open file 'storage/60/602a2c5f-424b-43c5-8ff5-5245dc77688d' for reading: No such file or directory`

**Business Impact:**
- Data loss - users cannot retrieve uploaded files
- Database/storage inconsistency
- Broken cross-user file sharing functionality

---

## 2. Evidence Chain

### Database State
```sql
-- File entry exists in database
id: 23
name: automated_client_test.sh
physical_path: 602a2c5f-424b-43c5-8ff5-5245dc77688d
owner_id: 3 (test2)
size: 4409 bytes
created_at: 2026-01-09 04:34:44
```

### Storage State
```bash
# Directory exists but file is missing
$ ls -la storage/60/
-rw-r--r-- 60ac73c3-38ca-43e1-b436-24ee037e0627  # Different file
# 602a2c5f-424b-43c5-8ff5-5245dc77688d NOT PRESENT
```

### Server Logs
```
[2026-01-09 11:34:44] [INFO] db_create_file: Successfully created file/dir 'automated_client_test.sh' with id=23
[2026-01-09 11:34:44] [INFO] Upload request accepted: file_id=23, uuid=602a2c5f-424b-43c5-8ff5-5245dc77688d, size=4409
# NO subsequent log for storage_write_file
# NO activity log for UPLOAD completion
[hours later]
[2026-01-09 18:23:26] [ERROR] Failed to open file 'storage/60/602a2c5f-424b-43c5-8ff5-5245dc77688d' for reading: No such file or directory
```

### Code Flow Analysis

**Upload Flow (commands.c):**
```c
// Line 313-388: handle_upload_req
1. Generates UUID: 602a2c5f-424b-43c5-8ff5-5245dc77688d
2. Creates database entry with UUID as physical_path
3. Stores UUID in session->pending_upload_uuid
4. Sends READY response to client
5. EXPECTS client to send CMD_UPLOAD_DATA next

// Line 390-432: handle_upload_data
1. Validates pending_upload_uuid exists
2. Verifies payload size matches
3. Calls storage_write_file(uuid, data, size)
4. Logs activity on success
```

**Storage Logic (storage.c):**
```c
// Line 36-53: storage_get_path
- Constructs: storage_base/subdir/uuid
- Example: "storage/60/602a2c5f-424b-43c5-8ff5-5245dc77688d"

// Line 55-104: storage_write_file
- Creates subdirectory if needed
- Writes file to constructed path
- Logs: "Wrote file to storage: <path> (<size> bytes)"
```

---

## 3. Hypotheses Tested

### Hypothesis 1: Path Construction Error ❌
**Theory:** storage_get_path() constructs wrong path
**Test:** Manual path construction simulation
**Result:** Path correctly constructed as `storage/60/602a2c5f...`
**Conclusion:** Path logic is correct

### Hypothesis 2: Storage Directory Missing ❌
**Theory:** Subdirectory storage/60/ doesn't exist
**Test:** `ls -la storage/60/`
**Result:** Directory exists, contains different file
**Conclusion:** Directory structure is functional

### Hypothesis 3: Upload Data Never Received ✅ **ROOT CAUSE**
**Theory:** Client sent UPLOAD_REQ but never sent UPLOAD_DATA
**Evidence:**
- Database entry created (UPLOAD_REQ processed)
- No storage_write_file log (UPLOAD_DATA never executed)
- No activity log for UPLOAD (logged only after storage write)
- File physically missing from storage

**Timeline:**
```
11:34:44 - UPLOAD_REQ received → DB entry created
          UPLOAD_DATA expected but NEVER received
          Client disconnected/failed before sending data
Hours later - test1 tries to download → File not found error
```

**Conclusion:** **Upload request succeeded but data transfer failed**

### Hypothesis 4: Race Condition/Cleanup ❌
**Theory:** File written then deleted
**Test:** Log analysis for deletion events
**Result:** No deletion logs, no storage_write_file success log
**Conclusion:** File was never written

---

## 4. Root Cause Identified

**Primary Cause:** **Incomplete Upload Protocol - Orphaned Database Entry**

The file upload follows a two-phase protocol:
1. **Phase 1 (UPLOAD_REQ):** Client requests upload → Server creates DB entry → Responds READY
2. **Phase 2 (UPLOAD_DATA):** Client sends file data → Server writes to storage → Logs completion

**What Went Wrong:**
- Phase 1 completed successfully (DB entry created)
- Phase 2 **never executed** (no data received, file never written)
- System left in inconsistent state: DB entry exists, physical file missing

**Why This Happened:**
Possible client-side failures:
- Network disconnection after UPLOAD_REQ
- Client crash before sending UPLOAD_DATA
- Client timeout waiting for READY response
- Client bug in upload protocol implementation
- Manual test script termination

**Why System Accepted This:**
- Server has **no cleanup mechanism** for abandoned uploads
- No timeout on pending_upload_uuid
- No validation that physical file exists before responding to downloads
- Database entry persists even if UPLOAD_DATA never arrives

---

## 5. Resolution Plan

### Immediate Fix (Cleanup Orphaned Entry)

**Option A: Delete Orphaned Database Entry**
```sql
DELETE FROM files WHERE id = 23;
```
**Pros:** Clean state
**Cons:** Data permanently lost

**Option B: Mark as Incomplete**
```sql
-- Add status column to files table in future
-- For now, manual deletion recommended
```

### Permanent Solution (Prevent Future Occurrences)

**Fix 1: Add Upload Timeout**
```c
// In session cleanup or periodic check
if (session->pending_upload_uuid &&
    time(NULL) - session->upload_req_time > UPLOAD_TIMEOUT) {
    // Delete orphaned database entry
    db_delete_file(global_db, session->pending_file_id);
    free(session->pending_upload_uuid);
    session->pending_upload_uuid = NULL;
}
```

**Fix 2: Validate Physical File on Download**
```c
// In handle_download before storage_read_file
if (!storage_file_exists(entry.physical_path)) {
    send_error(session, "File data missing (incomplete upload)");
    // Optionally: Delete orphaned DB entry
    return;
}
```

**Fix 3: Add Transaction-like Semantics**
```c
// Create DB entry AFTER successful storage write
// Or add status field: PENDING → COMPLETE
int file_id = storage_write_file(...);
if (file_id >= 0) {
    db_create_file(...);  // Only create DB entry if storage succeeds
}
```

**Fix 4: Add Integrity Check Command**
```c
// Admin command to find and clean orphaned entries
void check_storage_integrity() {
    foreach file in db:
        if (!storage_file_exists(file.physical_path)):
            log_warning("Orphaned entry: file_id=%d", file.id);
            // Optionally auto-delete or mark invalid
}
```

---

## 6. Prevention Strategy

### Monitoring
- **Alert on orphaned uploads:** Periodic check for pending_upload_uuid older than threshold
- **Storage integrity checks:** Daily cron job to compare DB entries with physical files
- **Upload success rate:** Track UPLOAD_REQ vs successful UPLOAD_DATA completion

### Logging Enhancements
- Log pending upload UUID with timestamp
- Log when pending upload is cleared (success or timeout)
- Add correlation ID to track upload request → data → completion

### Client-Side Improvements
- Add retry logic for UPLOAD_DATA
- Implement upload resume capability
- Send explicit ABORT command on cancellation

### Database Schema Enhancement
```sql
ALTER TABLE files ADD COLUMN status TEXT DEFAULT 'COMPLETE';
-- Values: PENDING, COMPLETE, CORRUPT
-- PENDING = UPLOAD_REQ received, waiting for data
-- COMPLETE = Data written to storage
-- CORRUPT = Physical file missing
```

---

## 7. Recommended Actions

**Priority 1 (Immediate):**
1. Delete orphaned database entry (file_id=23)
2. Implement physical file validation in handle_download
3. Add session cleanup for abandoned uploads

**Priority 2 (Short-term):**
1. Add upload timeout mechanism
2. Implement storage integrity check command
3. Add monitoring for orphaned entries

**Priority 3 (Long-term):**
1. Add status field to files table
2. Implement transaction-like upload semantics
3. Add upload resume capability
4. Build admin dashboard showing storage health

---

## 8. Testing Validation

**Test Cases to Verify Fix:**
1. **Normal upload/download:** Should work as before
2. **Client disconnects after UPLOAD_REQ:** DB entry should be cleaned up
3. **Download orphaned file:** Should return meaningful error
4. **Integrity check command:** Should detect and report orphaned entries
5. **Upload timeout:** Should trigger cleanup after configured delay

**Regression Prevention:**
- Add integration test: Upload request → simulate disconnect → verify cleanup
- Add validation test: Attempt download of missing physical file
- Monitor upload completion rate in production

---

## Conclusion

**Root Cause:** Two-phase upload protocol allows database entry creation before physical file storage. When clients fail to send UPLOAD_DATA after UPLOAD_REQ, system is left with orphaned database entries pointing to non-existent files.

**Impact:** High - Causes download failures and data loss appearance

**Solution Complexity:** Medium - Requires timeout mechanism, validation, and cleanup logic

**Priority:** High - Affects data integrity and user experience

**Prevention:** Implement validation, monitoring, and atomic upload semantics

---

**Unresolved Questions:**
1. Was this a client bug or network issue that caused incomplete upload?
2. How many other orphaned entries exist in the database?
3. Should we preserve metadata for incomplete uploads for recovery/debugging?
