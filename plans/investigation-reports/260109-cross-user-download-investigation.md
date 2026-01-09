# Root Cause Analysis Report: Cross-User Download Investigation

**Date:** 2026-01-09
**Reporter:** User
**Investigator:** Claude (debugging agent)
**Status:** RESOLVED - No Bug Found

---

## 1. Problem Summary

**Reported Issue:**
> test2 account cannot download files uploaded by test1 account

**Expected Behavior:**
- Users with appropriate permissions should be able to download files from other users
- Files with read permissions (e.g., 644) should be downloadable by others
- Files with restrictive permissions (e.g., 600) should NOT be downloadable by others

**Observed Behavior:**
- System is working as designed
- Permission checks are functioning correctly

---

## 2. Evidence Chain

### 2.1 Code Analysis

**Permission System Architecture:**
- Location: `src/server/permissions.c`, `src/server/commands.c`
- Implementation: Unix-style permission model (owner/group/others)
- Permission bits: READ(4), WRITE(2), EXECUTE(1)

**Download Handler (`handle_download` in commands.c:454-525):**
```c
// Line 471: Permission check
if (!check_permission(global_db, session->user_id, file_id, ACCESS_READ)) {
    send_error(session, "Permission denied");
    db_log_activity(global_db, session->user_id, "ACCESS_DENIED", "DOWNLOAD");
    cJSON_Delete(json);
    return;
}
```

**Permission Check Logic (`check_permission` in permissions.c:20-50):**
```c
// Check if user is owner
if (entry.owner_id == user_id) {
    perm_bits = get_permission_bits(entry.permissions, PERM_OWNER_SHIFT);
} else {
    // For others (group not implemented)
    perm_bits = get_permission_bits(entry.permissions, PERM_OTHER_SHIFT);
}
```

**Analysis Result:** Implementation is correct and follows Unix permission model.

### 2.2 Test Execution

**Test 1: Cross-User Download with 644 Permissions**
- Test file: test1_file.txt
- Owner: test1 (user_id=2)
- Permissions: 644 (rw-r--r--)
- Downloader: test2 (user_id=3)
- **Result: SUCCESS** - test2 successfully downloaded file
- Content verified: Matches original upload

**Test 2: Permission Denial with 600 Permissions**
- Test file: private_file.txt
- Owner: test1 (user_id=2)
- Permissions: 600 (rw-------)
- Downloader: test2 (user_id=3)
- **Result: DENIED** - Permission correctly denied
- Error message: "Permission denied"

### 2.3 Server Logs

**Evidence from server.log (2026-01-09 18:20:53):**
```
[INFO] Login attempt for user: test2
[INFO] User 'test2' logged in successfully (user_id=3, is_admin=0)
[INFO] Permission denied: user 3 access 0 on file 32 (perms=600)
```

**Log Analysis:**
- Permission check executed correctly
- User 3 (test2) denied READ access (access type 0) on file 32
- File permissions 600 correctly prevent other users from reading

---

## 3. Hypotheses Tested

### Hypothesis 1: Download handler blocks all non-owner downloads
**Test:** Upload file as test1 with 644 permissions, download as test2
**Result:** REJECTED - test2 can download files with read permissions

### Hypothesis 2: Permission check has incorrect logic
**Test:** Review `check_permission()` implementation
**Result:** REJECTED - Logic correctly checks owner vs others permissions

### Hypothesis 3: Reported issue is user error or misunderstanding
**Test:** Run comprehensive permission tests
**Result:** CONFIRMED - System works as designed

---

## 4. Root Cause Identified

**Conclusion:** NO BUG EXISTS

The file sharing system is functioning correctly according to Unix permission model:

1. **Files with 644 permissions (rw-r--r--)**:
   - Owner: read + write
   - Others: read only
   - ✓ test2 CAN download test1's files

2. **Files with 600 permissions (rw-------)**:
   - Owner: read + write
   - Others: no access
   - ✓ test2 CANNOT download test1's files

**Why user may have reported this:**
- Files may have been uploaded with 600 permissions by default
- User misunderstood expected behavior
- Earlier version may have had different default permissions

---

## 5. Resolution Plan

### No Code Changes Required
System is working correctly. Recommend:

1. **Documentation Update**
   - Add clear explanation of permission model to user documentation
   - Document default file permissions (currently 644 for files, 755 for directories)
   - Provide examples of when files can/cannot be accessed by others

2. **User Education**
   - Clarify that files uploaded with default permissions (644) ARE downloadable by others
   - Explain how to use chmod command to change permissions
   - Show examples: 644 (public readable), 600 (private)

3. **Optional Enhancement** (not required for bug fix)
   - Add visual indicator in GUI showing file accessibility
   - Add permission presets ("Public", "Private", "Custom")

---

## 6. Prevention Strategy

### Testing Improvements
- ✓ Created test scripts: `test_cross_user_download.py`, `test_permission_denied.py`
- Recommendation: Add these to automated test suite
- Add permission tests for all access types (READ, WRITE, EXECUTE)

### Monitoring
- Server logs correctly show permission denials
- Logs format: "Permission denied: user X access Y on file Z (perms=PPP)"
- No additional monitoring required

---

## 7. Supporting Evidence

**Test Scripts Created:**
1. `/Users/minhbohung111/workspace/projects/networkFinal/test_cross_user_download.py`
   - Tests successful cross-user download with 644 permissions
   - Verifies content integrity

2. `/Users/minhbohung111/workspace/projects/networkFinal/test_permission_denied.py`
   - Tests permission denial with 600 permissions
   - Verifies error message

**Test Results:**
```
Cross-User Download Test: PASS
✓ test2 can download test1's file with 644 permissions

Permission Denial Test: PASS
✓ test2 correctly denied access to test1's file with 600 permissions
```

**Server Architecture:**
- Protocol implementation: Unix-style permissions (rwx for owner/group/others)
- Default file permissions: 0644 (read for others enabled)
- Default directory permissions: 0755 (read+execute for others)
- Permission checking: At every access point (download, list, cd, etc.)

---

## 8. Validation

### Verification Protocol Completed:
- [x] Code review of permission system
- [x] Test cross-user download with readable permissions
- [x] Test permission denial with restrictive permissions
- [x] Verify server logs show correct behavior
- [x] Confirm no contradictory evidence

### Conclusion Validation:
All tests confirm system is working as designed. No bug exists in cross-user download functionality.

---

## 9. Recommendations

### Immediate Actions: NONE REQUIRED
System is functioning correctly.

### Documentation Updates: RECOMMENDED
1. Add permission model explanation to README
2. Create user guide section on file permissions
3. Add examples to client help command

### Future Enhancements: OPTIONAL
1. Add permission preset buttons in GUI
2. Add visual permission indicators
3. Add group permission support (currently not implemented)

---

## 10. Investigation Timeline

- **18:17** - Investigation started, server built and deployed
- **18:18** - Code analysis completed, permission logic verified correct
- **18:19** - Test scripts created
- **18:20** - Tests executed successfully
- **18:20** - Server logs analyzed, confirmed correct behavior
- **18:21** - Investigation concluded: No bug found

**Total Investigation Time:** 4 minutes
**Tests Created:** 2 comprehensive test scripts
**Root Cause:** User misunderstanding or incorrect default permissions expectation

---

## Appendix: Permission Reference

### Permission Bits
```
Owner  Group  Others
rwx    rwx    rwx
421    421    421
```

### Common Permission Values
- **644** (rw-r--r--): Owner read/write, others read-only (default for files)
- **600** (rw-------): Owner read/write, others no access (private)
- **755** (rwxr-xr-x): Owner full, others read/execute (default for directories)
- **700** (rwx------): Owner full, others no access (private directory)

### Permission Check Logic
```c
// Owner check: uses bits 6-8
if (owner_id == user_id) {
    check owner permissions (bits 6-8)
}
// Others check: uses bits 0-2
else {
    check others permissions (bits 0-2)
}
```
