# Executive Summary: File Storage Bug Investigation

**Date:** 2026-01-09
**Investigator:** Debugger Agent
**Status:** Root Cause Identified, Fixes Proposed

---

## Problem

Users cannot download files that appear in database but physical files missing from storage.

**Error:** `Failed to open file 'storage/60/602a2c5f-424b-43c5-8ff5-5245dc77688d' for reading: No such file or directory`

---

## Root Cause

**Two-phase upload protocol allows database entry creation before physical file storage.**

Upload Flow:
1. Client sends UPLOAD_REQ → Server creates DB entry → Server responds READY
2. Client sends UPLOAD_DATA → Server writes physical file → Complete

**Issue:** When clients fail/disconnect after step 1, database entry exists but physical file never written.

---

## Impact Assessment

**Severity:** Critical - Data Loss / System Inconsistency

**Scope:**
- **9 orphaned entries** found in current database
- Users experience download failures
- Database bloat with invalid entries
- Phantom files in permission/listing checks

**Files Affected:**
```
ID 1:  aba.txt
ID 3:  Screenshot 2025-12-03 at 13.10.14.png
ID 15: Screenshot 2025-12-03 at 13.10.14.png (CORRUPT UUID)
ID 21: Screenshot 2025-10-17 at 12.37.47.png
ID 22: ATTT_final_knowledge-đã-gộp.pdf
ID 23: automated_client_test.sh (reported case)
ID 29: test_image.png
ID 30: test1_file.txt
ID 43: asihn.txt
```

---

## Immediate Action Required

### 1. Clean Orphaned Entries (Manual - Now)
```sql
DELETE FROM files WHERE id IN (1, 3, 15, 21, 22, 23, 29, 30, 43);
```

### 2. Apply Download Validation (Code Fix - Today)
**File:** `src/server/commands.c:handle_download()`
**Add:** Physical file existence check before read attempt
**Result:** Users get meaningful error instead of "No such file"

---

## Proposed Fixes

| Fix | Priority | Effort | Impact |
|-----|----------|--------|--------|
| Download validation | P0 Critical | 15 min | Prevent error exposure |
| Upload timeout cleanup | P1 High | 1-2 hrs | Prevent new orphans |
| Admin cleanup command | P2 Medium | 30 min | Manual orphan removal |
| Periodic integrity check | P3 Low | 2-3 hrs | Automated detection |

---

## Technical Solution Summary

**Fix 1: Defensive Download** (Immediate)
- Check `storage_file_exists()` before `storage_read_file()`
- Return "File data unavailable (incomplete upload)" error

**Fix 2: Upload Timeout** (Short-term)
- Track upload request time in session
- Auto-delete DB entry if UPLOAD_DATA not received within 5 minutes
- Add upload_status field to database: PENDING → COMPLETE

**Fix 3: Admin Tools** (Short-term)
- Add command to scan DB vs storage and report/clean orphans
- Integrate into admin dashboard

**Fix 4: Monitoring** (Long-term)
- Background thread checks integrity every 24 hours
- Alert on orphaned entries detected
- Track upload success rate metrics

---

## Testing Requirements

**Must Test:**
1. Normal upload/download still works
2. Interrupted upload triggers cleanup after timeout
3. Download of orphaned file returns clear error
4. Admin cleanup command detects all orphans
5. No race conditions in timeout logic

**Regression Tests:**
- Upload request → client disconnect → verify cleanup
- Multiple concurrent uploads with timeouts
- Large file upload timeout handling

---

## Rollout Timeline

| Phase | Timeline | Actions |
|-------|----------|---------|
| Phase 1 | Today | Apply Fix 1, manual cleanup, deploy |
| Phase 2 | This week | Implement Fix 2, test timeout, deploy |
| Phase 3 | Next week | Add Fix 3, GUI integration, deploy |
| Phase 4 | Next month | Implement Fix 4, monitoring setup |

---

## Success Metrics

**Immediate (24 hours):**
- Zero "No such file" errors in logs
- All orphaned entries cleaned

**Short-term (1 week):**
- Zero new orphaned entries
- Upload timeout events logged and handled

**Long-term (1 month):**
- Automated integrity checks running
- Upload success rate > 99%
- Alert system operational

---

## Risk Assessment

**Implementation Risks:**
- **Low:** Fix 1 is defensive, no existing functionality impacted
- **Medium:** Fix 2 requires careful timeout testing to avoid false positives
- **Low:** Fix 3 is admin-only, limited blast radius
- **Low:** Fix 4 is background monitoring, no critical path

**Mitigation:**
- Thorough testing of timeout logic
- Configurable timeout value (not hardcoded)
- Logging before any automated deletion
- Manual verification before auto-cleanup enabled

---

## Recommendations

**Do Now:**
1. Backup database before orphan cleanup
2. Export orphaned entry metadata for forensics
3. Apply Fix 1 (download validation)
4. Delete orphaned entries manually

**Do This Week:**
1. Implement upload timeout with extensive testing
2. Add database schema for upload status tracking
3. Monitor upload patterns for timeout tuning

**Do Long-term:**
1. Build admin dashboard with storage health view
2. Implement upload resume capability
3. Add transaction-like semantics to uploads
4. Consider upload status webhooks for clients

---

## Open Questions

1. **Client-side issue?** Was incomplete upload due to client bug or network?
2. **Other orphans?** Should we scan older backups for lost data?
3. **Recovery possible?** Can we preserve metadata for incomplete uploads?
4. **Timeout value?** What's optimal timeout for various file sizes?

---

## Related Documentation

- `260109-missing-file-storage-bug-report.md` - Full root cause analysis
- `orphaned-files-inventory.md` - Complete list of orphaned entries
- `proposed-fix-implementation.md` - Detailed implementation guide

---

**Next Steps:** Await approval for manual cleanup and Fix 1 deployment.
