# Phase 4: Pre-Demo Verification
**Comprehensive Testing Before Live Demonstration**

**Timeline:** T-45 minutes (45 minutes before demo)
**Duration:** 15-20 minutes
**Prerequisites:** Phase 3 complete, all systems deployed

---

## Verification Overview

### Testing Approach

**Systematic Validation:**
1. Network connectivity
2. Authentication flow
3. Core features (one by one)
4. Concurrent operations
5. Admin functions
6. Performance metrics

**Critical Rule:** Test EVERYTHING that will be demonstrated

---

## Test Plan

### Test 1: Network Connectivity (2 min)

**Objective:** Verify all machines can communicate with server

**Test Steps:**
```bash
# On each client machine (Machines 2, 3, 4):

# Ping test
ping -c 5 192.168.1.100 | grep "packet loss"
# Expected: 0% packet loss

# Port test
nc -zv 192.168.1.100 8080
# Expected: Connection succeeded

# Latency test
ping -c 10 192.168.1.100 | grep avg
# Expected: avg < 10ms
```

**Success Criteria:**
- [ ] All clients ping server successfully
- [ ] 0% packet loss from all machines
- [ ] Port 8080 accessible from all clients
- [ ] Latency under 10ms (LAN)

**If Failed:**
- Check network configuration (Phase 2)
- Verify firewall settings
- Restart network interfaces
- Check server is running

---

### Test 2: Authentication Flow (3 min)

**Objective:** Verify all user accounts can login and logout

**Machine 2 (test1):**
```
1. Launch: ./gui_client
2. Login dialog appears automatically
3. Enter: Server IP: 192.168.1.100
4. Enter: Username: test1
5. Enter: Password: test123
6. Click: Login
7. Main window appears
8. Verify: Status bar shows "Connected as test1"
9. Click: Logout button (or File → Logout)
10. Confirm: Returns to login dialog
```

**Machine 3 (test2):**
```
Repeat above steps with:
- Username: test2
- Password: test123
```

**Machine 4 (admin):**
```
Repeat above steps with:
- Username: admin
- Password: admin
- Verify: Admin menu/dashboard visible
```

**Check Server Logs (Machine 1):**
```bash
# Should see login events:
# [INFO] Client connected from 192.168.1.101
# [INFO] User 'test1' logged in successfully
# [INFO] Client connected from 192.168.1.102
# [INFO] User 'test2' logged in successfully
# [INFO] Client connected from 192.168.1.103
# [INFO] User 'admin' logged in successfully
```

**Success Criteria:**
- [ ] All users login successfully
- [ ] Main windows display correctly
- [ ] Status bar shows correct username
- [ ] Logout returns to login dialog
- [ ] Server logs authentication events
- [ ] No error messages

---

### Test 3: Directory Navigation (3 min)

**Machine 2 (test1):**
```
1. Login as test1
2. Main window shows "/" (root directory)
3. Directory tree visible in left pane
4. File list empty in right pane (new system)
5. Click: "New Folder" or right-click → "Create Directory"
6. Enter: "VerifyTest"
7. Confirm: New directory appears in tree and list
8. Double-click directory: Should navigate into it
9. Click: Back button (or up arrow)
10. Return to root directory
```

**Success Criteria:**
- [ ] Root directory displays
- [ ] Directory tree visible and functional
- [ ] Can create new directory
- [ ] Can navigate into directory
- [ ] Back button returns to parent
- [ ] Current path shown in UI

---

### Test 4: File Upload (3 min)

**Machine 2 (test1):**
```
1. Navigate to root directory
2. Click: "Upload" button
3. File chooser appears
4. Select: ~/Desktop/demo-upload/readme.txt
5. Confirm upload
6. Progress indicator shows (if implemented)
7. File appears in file list
8. Verify: File size shown correctly
9. Verify: Permissions shown (e.g., 644)
```

**Machine 3 (test2 - concurrent):**
```
1. Login as test2
2. Upload different file: image.jpg
3. Should see own uploaded file
4. Refresh or check if test1's file visible
```

**Server Logs Check:**
```bash
# Expected logs:
# [INFO] Command: UPLOAD_REQ, User: test1, File: readme.txt
# [INFO] Command: UPLOAD_DATA, User: test1, Size: 247 bytes
# [INFO] File upload completed: readme.txt
# [INFO] Command: UPLOAD_REQ, User: test2, File: image.jpg
```

**Success Criteria:**
- [ ] Upload button functional
- [ ] File chooser appears
- [ ] Upload completes successfully
- [ ] File visible in directory listing
- [ ] Correct file size and metadata
- [ ] Concurrent uploads work (test1 and test2)

---

### Test 5: File Download (2 min)

**Machine 2 (test1):**
```
1. In file list, select uploaded readme.txt
2. Click: "Download" button
3. Save dialog appears
4. Choose: ~/Desktop/download-test/
5. Confirm download
6. Download completes
7. Verify: File exists at destination
8. Open file: Content matches original
```

**Verification:**
```bash
# On Machine 2:
diff ~/Desktop/demo-upload/readme.txt ~/Desktop/download-test/readme.txt
# Expected: No differences
```

**Success Criteria:**
- [ ] Download button functional
- [ ] Save dialog appears
- [ ] Download completes without errors
- [ ] Downloaded file matches original (checksum)

---

### Test 6: File Operations (3 min)

**Machine 2 (test1):**

**6.1 Rename:**
```
1. Right-click readme.txt
2. Select: Rename
3. Enter: instructions.txt
4. Confirm: File renamed in list
```

**6.2 Copy:**
```
1. Right-click instructions.txt
2. Select: Copy
3. Navigate to different directory (VerifyTest)
4. Right-click empty space
5. Select: Paste
6. Confirm: File copied to new location
```

**6.3 Delete:**
```
1. Navigate back to root
2. Right-click instructions.txt
3. Select: Delete
4. Confirm deletion dialog
5. File removed from list
```

**Success Criteria:**
- [ ] Rename operation works
- [ ] Copy to clipboard successful
- [ ] Paste in different directory works
- [ ] Delete removes file
- [ ] All operations reflected in UI immediately

---

### Test 7: Directory Operations (2 min)

**Machine 3 (test2):**
```
1. Create directory: "ImagesTest"
2. Upload image.jpg into ImagesTest
3. Create subdirectory: "ImagesTest/Subfolder"
4. Navigate into subdirectories
5. Navigate back to root
6. Delete empty Subfolder
7. Verify: ImagesTest still contains image.jpg
```

**Success Criteria:**
- [ ] Create nested directories
- [ ] Upload files into subdirectories
- [ ] Navigate directory tree
- [ ] Delete operations work correctly

---

### Test 8: File Permissions (2 min)

**Machine 2 (test1):**
```
1. Upload or select existing file
2. Right-click file
3. Select: "Change Permissions" or "Properties"
4. Chmod dialog appears
5. Current permissions shown (e.g., 644)
6. Change to: 755
7. Confirm
8. Verify: Permissions updated in file list
```

**Success Criteria:**
- [ ] Chmod dialog accessible
- [ ] Current permissions displayed
- [ ] Can modify permissions
- [ ] Changes reflected immediately
- [ ] Server logs permission change

---

### Test 9: File Search (3 min)

**Machine 2 (test1):**
```
1. Ensure multiple files uploaded
2. Locate search bar at top of window
3. Enter search pattern: "*.txt"
4. Uncheck "Recursive" (current directory only)
5. Click: Search
6. Results dialog shows matching files
7. Close results
8. Enter pattern: "test"
9. Check "Recursive"
10. Click: Search
11. Results show all matches in subdirectories
```

**Success Criteria:**
- [ ] Search bar functional
- [ ] Non-recursive search works
- [ ] Recursive search finds files in subdirectories
- [ ] Results dialog displays correctly
- [ ] Can navigate to found files

---

### Test 10: Concurrent Operations (3 min)

**Objective:** Verify multiple clients operate simultaneously

**Simultaneous Actions:**

**Machine 2 (test1):**
```
Upload large file (video.mp4 - 5MB)
```

**Machine 3 (test2):**
```
While test1 uploading:
- Create new directory
- Upload small file
- List directory
```

**Machine 4 (admin):**
```
While both uploading:
- Open admin dashboard
- View user list
- Check activity logs
```

**Server Logs Check:**
```bash
# Should show interleaved operations:
# [INFO] Command: UPLOAD_REQ, User: test1, File: video.mp4
# [INFO] Command: MAKE_DIR, User: test2, Dir: NewFolder
# [INFO] Command: UPLOAD_DATA, User: test1, Chunk: 1/5
# [INFO] Command: LIST_DIR, User: test2
# [INFO] Command: ADMIN_LIST_USERS, User: admin
```

**Success Criteria:**
- [ ] Multiple clients connected simultaneously
- [ ] Operations do not block each other
- [ ] No deadlocks or hangs
- [ ] All operations complete successfully
- [ ] UI remains responsive on all clients

---

### Test 11: Admin Dashboard (3 min)

**Machine 4 (admin):**

**11.1 View Users:**
```
1. Click: Admin → Manage Users
2. User list displays
3. Verify: Shows test1, test2, admin
4. Check: Active status shown
```

**11.2 Create User:**
```
1. Click: "Create User" button
2. Dialog appears
3. Enter: Username: demo_user
4. Enter: Password: demo123
5. Confirm
6. New user appears in list
```

**11.3 Update User:**
```
1. Select demo_user
2. Click: "Edit" or "Deactivate"
3. Set: Active = No
4. Confirm
5. Status changes in list
```

**11.4 View Activity Logs:**
```
1. Click: Admin → Activity Logs
2. Recent operations displayed
3. Verify: Shows logins, uploads, deletions
4. Check: Timestamps and usernames correct
```

**Success Criteria:**
- [ ] Admin dashboard accessible (admin only)
- [ ] User list displays correctly
- [ ] Can create new users
- [ ] Can modify user status
- [ ] Activity logs show operations
- [ ] Regular users (test1, test2) cannot access admin features

---

### Test 12: Context Menus (2 min)

**Machine 2 (test1):**

**On File:**
```
1. Right-click file in list
2. Verify menu shows:
   - Download
   - Rename
   - Copy
   - Delete
   - Change Permissions
   - Properties
3. Test one operation from menu
```

**On Empty Space:**
```
1. Right-click empty area in file list
2. Verify menu shows:
   - Upload File
   - Create Directory
   - Paste (if clipboard has data)
   - Refresh
3. Test one operation from menu
```

**On Directory Tree:**
```
1. Right-click directory in tree
2. Verify menu shows appropriate options
```

**Success Criteria:**
- [ ] Right-click triggers context menu
- [ ] File context menu has all operations
- [ ] Empty space context menu functional
- [ ] Menu items trigger correct actions

---

## Performance Verification

### Metric Collection (Throughout Testing)

**Latency:**
```bash
# On each client machine:
ping -c 20 192.168.1.100 | tail -1
# Record: min/avg/max/stddev
```

**File Transfer Speed:**
```bash
# Upload 5MB file, record time
# Calculate: 5MB / seconds = MB/s
# Expected: > 1 MB/s (LAN)
```

**Operation Response Time:**
```
- Directory listing: < 100ms
- File upload (small): < 1 second
- File download (small): < 1 second
- Create directory: < 50ms
- Search operation: < 500ms
```

**Concurrent Capacity:**
```
- 3 clients connected: OK
- All clients performing operations: OK
- No timeouts or disconnections: OK
```

---

## Verification Checklist

### Core Functionality
- [ ] Authentication (login/logout) - All users
- [ ] Directory navigation (tree view)
- [ ] File upload (single file)
- [ ] File download
- [ ] File rename
- [ ] File copy/paste
- [ ] File delete
- [ ] Directory create
- [ ] Directory delete

### Advanced Features
- [ ] File permissions (chmod)
- [ ] File search (pattern matching)
- [ ] Recursive search
- [ ] Concurrent client operations
- [ ] Admin user management
- [ ] Activity logging
- [ ] Context menus (file and empty space)

### Performance
- [ ] Network latency < 10ms
- [ ] File transfers > 1 MB/s
- [ ] UI responsive (no freezes)
- [ ] Concurrent operations work smoothly
- [ ] No memory leaks (check Activity Monitor)

### Stability
- [ ] No crashes or errors
- [ ] Clean server logs (no warnings)
- [ ] Connections stable for 10+ minutes
- [ ] Can logout and re-login
- [ ] Database remains consistent

---

## Issue Resolution

### If Any Test Fails

**Priority 1 (Critical - Demo Blockers):**
- Authentication failure
- Cannot connect to server
- Client crashes
- Database corruption

**Action:** STOP. Fix immediately. Must work for demo.

**Priority 2 (Important - Demo Impact):**
- File upload/download fails
- Directory operations not working
- Admin dashboard inaccessible

**Action:** Attempt quick fix. If cannot resolve in 5 minutes, plan workaround in demo.

**Priority 3 (Minor - Demo Adjustment):**
- Search not working
- Copy/paste issues
- Performance slower than expected

**Action:** Note for demo. Mention "known issue" or skip that feature.

---

## Verification Timeline

| Test # | Feature | Duration | Status |
|--------|---------|----------|--------|
| 1 | Network connectivity | 2 min | [ ] |
| 2 | Authentication | 3 min | [ ] |
| 3 | Directory navigation | 3 min | [ ] |
| 4 | File upload | 3 min | [ ] |
| 5 | File download | 2 min | [ ] |
| 6 | File operations | 3 min | [ ] |
| 7 | Directory operations | 2 min | [ ] |
| 8 | Permissions (chmod) | 2 min | [ ] |
| 9 | Search | 3 min | [ ] |
| 10 | Concurrent ops | 3 min | [ ] |
| 11 | Admin dashboard | 3 min | [ ] |
| 12 | Context menus | 2 min | [ ] |
| **Total** | **All Features** | **31 min** | |

**Note:** Tests can overlap. Target completion: 20 minutes

---

## Pre-Demo Readiness Check

### 10 Minutes Before Demo

**Final Verification:**
```
[ ] All systems running and connected
[ ] No error messages visible
[ ] Server logs clean
[ ] Demo files ready for upload
[ ] Team members at stations
[ ] Presentation equipment working
[ ] Backup plans reviewed
[ ] Timing rehearsed
```

**Reset for Demo:**
```bash
# On server machine:
# Clean database (if needed for fresh demo)
sqlite3 fileshare.db << 'EOF'
DELETE FROM files WHERE id > 0;  -- Keep root
DELETE FROM activity_logs;       -- Clear logs
VACUUM;                           -- Compact database
EOF

# Clear storage directory
rm -rf storage/*

# Restart server for clean logs
killall server
./server 8080
```

**All Clients Logout:**
```
Close all GUI clients
Ready to launch fresh for demo
```

---

## Verification Success Criteria

### Must Achieve (Go/No-Go)
- [x] 100% of Priority 1 tests passing
- [x] 90%+ of Priority 2 tests passing
- [x] Network stable for 10+ minutes
- [x] All team members confident in their roles

### Demo Ready When
- All core features verified working
- Team rehearsed and comfortable
- Timing confirmed (10-15 minutes)
- Contingency plans prepared
- Equipment double-checked

---

## Next Phase

If verification complete and successful:
→ **Phase 5: Demo Script** (for reference during demo)

If issues remain:
→ Return to **Phase 3** for redeployment
→ Or proceed with **Phase 8: Contingency Plans** for workarounds

**Go/No-Go Decision:** T-25 minutes
- **GO:** All critical tests passed → Proceed to demo
- **NO-GO:** Major failures → Implement contingency plan
