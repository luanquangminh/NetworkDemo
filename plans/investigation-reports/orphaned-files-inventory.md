# Orphaned Database Entries Inventory

**Date:** 2026-01-09
**Investigation:** Storage integrity check following bug report

## Summary

**Total Orphaned Entries:** 9 files
**Impact:** Database entries exist but physical files missing from storage
**Cause:** Incomplete upload protocol (UPLOAD_REQ succeeded, UPLOAD_DATA never received)

## Orphaned Entries List

| File ID | File Name | Physical Path (UUID) | Status |
|---------|-----------|----------------------|--------|
| 1 | aba.txt | 1c8d1f3d-ec6f-476a-8673-41d7506e1ee1 | ORPHANED |
| 3 | Screenshot 2025-12-03 at 13.10.14.png | 1c961912-fb67-4930-b041-14a3674773d1 | ORPHANED |
| 15 | Screenshot 2025-12-03 at 13.10.14.png | copy_3_�ځv | ORPHANED (corrupt path) |
| 21 | Screenshot 2025-10-17 at 12.37.47.png | 60cd059c-766a-43e8-bd47-79980d3f6d8d | ORPHANED |
| 22 | ATTT_final_knowledge-đã-gộp.pdf | 5fdaac25-e24d-4415-948b-3e11a4667840 | ORPHANED |
| 23 | automated_client_test.sh | 602a2c5f-424b-43c5-8ff5-5245dc77688d | ORPHANED (reported) |
| 29 | test_image.png | 26f40f05-253d-498e-8808-272562710761 | ORPHANED |
| 30 | test1_file.txt | 2561f025-204f-43cd-83f9-76a162ed5d65 | ORPHANED |
| 43 | asihn.txt | 08e9f8c3-cec7-47f2-a668-201946935b67 | ORPHANED |

## Special Cases

**File ID 15:** Has corrupted physical_path `copy_3_�ځv` - not a valid UUID format
- Indicates additional bug in upload handling or database corruption

## Cleanup Options

### Option 1: Delete All Orphaned Entries (Recommended)
```sql
DELETE FROM files WHERE id IN (1, 3, 15, 21, 22, 23, 29, 30, 43);
```

### Option 2: Mark as Invalid (Requires Schema Change)
```sql
-- First add status column
ALTER TABLE files ADD COLUMN status TEXT DEFAULT 'COMPLETE';

-- Then mark orphaned
UPDATE files SET status = 'INVALID' WHERE id IN (1, 3, 15, 21, 22, 23, 29, 30, 43);
```

### Option 3: Export for Recovery Attempt
```bash
# Export metadata before deletion
sqlite3 fileshare.db "SELECT * FROM files WHERE id IN (1, 3, 15, 21, 22, 23, 29, 30, 43);" \
  > orphaned_files_backup_20260109.csv
```

## Impact Analysis

**Users Affected:**
- Depends on owner_id of these files (not checked yet)
- Users may attempt to download these files and receive errors

**System Health:**
- Database bloat with invalid entries
- User confusion when files appear in listings but fail to download
- Potential security issue: phantom files in permission checks

## Recommendation

1. **Export metadata** for forensic analysis
2. **Delete orphaned entries** to restore consistency
3. **Implement fixes** from root cause analysis report
4. **Schedule regular integrity checks** to detect future occurrences
5. **Investigate file ID 15** separately (corrupt UUID)
