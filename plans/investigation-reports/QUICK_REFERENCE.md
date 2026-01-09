# Quick Reference: File Storage Bug

## Problem
Downloaded files missing from storage despite DB entries existing.

## Root Cause
Upload protocol creates DB entry (UPLOAD_REQ) before physical file written (UPLOAD_DATA). If client disconnects between phases, orphaned DB entry remains.

## Current State
- **9 orphaned entries** found
- Files IDs: 1, 3, 15, 21, 22, 23, 29, 30, 43
- Reported case: file_id=23 (automated_client_test.sh)

## Immediate Actions

### 1. Cleanup Orphaned Entries
```bash
./cleanup_orphaned_entries.sh
```

### 2. Manual Verification
```bash
# Check for orphans
for uuid in $(sqlite3 fileshare.db "SELECT physical_path FROM files WHERE is_directory = 0 AND physical_path IS NOT NULL;"); do
  if [ ! -f "storage/${uuid:0:2}/$uuid" ]; then
    echo "ORPHANED: $uuid"
  fi
done
```

### 3. Quick Database Check
```sql
-- Count total files
SELECT COUNT(*) FROM files WHERE is_directory = 0;

-- List recent uploads
SELECT id, name, created_at FROM files
WHERE is_directory = 0
ORDER BY created_at DESC
LIMIT 10;
```

## Fix Priority

| Fix | Status | File | Function |
|-----|--------|------|----------|
| Download validation | TODO | commands.c | handle_download() |
| Upload timeout | TODO | commands.c | handle_upload_req/data() |
| Admin cleanup cmd | TODO | commands.c | handle_admin_cleanup_orphaned() |
| Periodic check | TODO | main.c | integrity_check_thread() |

## Testing Commands

```bash
# Test normal upload
./test_upload.sh

# Test interrupted upload (manual disconnect)
./test_upload.sh --disconnect-after-req

# Test download orphaned file
./test_download.sh --file-id=23

# Verify storage integrity
sqlite3 fileshare.db < verify_storage.sql
```

## Monitoring

```bash
# Check server logs for orphan errors
grep "No such file or directory" server.log

# Check for incomplete uploads
grep "Upload request accepted" server.log | grep -v "Wrote file to storage"

# Count orphaned entries
./count_orphans.sh
```

## Files Created

1. `260109-missing-file-storage-bug-report.md` - Full analysis
2. `orphaned-files-inventory.md` - List of orphans
3. `proposed-fix-implementation.md` - Fix details
4. `EXECUTIVE_SUMMARY.md` - Executive overview
5. `cleanup_orphaned_entries.sh` - Cleanup script
6. `QUICK_REFERENCE.md` - This file

## Next Steps

1. Review investigation reports
2. Approve orphan cleanup
3. Implement download validation (Fix 1)
4. Test and deploy
5. Monitor for new occurrences
6. Implement remaining fixes

## Contact

Questions? See full report: `260109-missing-file-storage-bug-report.md`
