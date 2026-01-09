# Phase 5: Demo Script
**Detailed Live Demonstration Flow**

**Duration:** 10-15 minutes
**Timeline:** T+0 (Demo start time)
**Audience:** Professors, students, evaluators

---

## Demo Overview

### Presentation Flow

```
1. Introduction (1 min)
2. Architecture Overview (1 min)
3. Authentication Demo (1 min)
4. File Operations Demo (4 min)
5. Concurrent Operations (2 min)
6. Admin Dashboard (2 min)
7. Advanced Features (2 min)
8. Conclusion (1 min)
9. Q&A (3-5 min)
```

**Total:** 12 minutes core + 3-5 minutes Q&A

---

## Pre-Demo Setup (T-5 min)

### Team Positions
- **Presenter:** Machine 2 (projected) - Primary speaker
- **Team Lead:** Machine 1 (server) - Monitor logs
- **Operations:** Machine 3 - Concurrent operations
- **Technical:** Machine 4 (admin) - Admin functions

### System State
```
Server: Running, clean logs
All Clients: Logged out, at login screens
Demo Files: Ready in ~/Desktop/demo-upload/
Projection: Machine 2 screen visible to audience
Notes: Each team member has script copy
```

---

## Demo Script

### Segment 1: Introduction (1 minute)

**Presenter (Machine 2):**

> "Good [morning/afternoon]. Today we're demonstrating our file sharing system - a distributed client-server application built in C.
>
> This system enables multiple users to store, share, and manage files across a network with features similar to FTP or cloud storage services.
>
> Our implementation includes:
> - Multi-threaded TCP server
> - GTK4 graphical clients
> - SQLite database backend
> - Unix-style permissions
> - Real-time concurrent operations
>
> We have 4 machines networked together: one server and three client machines operating independently to show true distributed functionality."

**[Gesture to physical machines]**

**Action Items:**
- [ ] Introduce team members and roles
- [ ] Show physical setup (4 machines)
- [ ] Explain demo structure

---

### Segment 2: Architecture Overview (1 minute)

**Team Lead (Machine 1 - briefly):**

> "Our server is running on this machine [indicate Machine 1], listening on port 8080. It manages:
> - Client connections via TCP sockets
> - Thread pool for concurrent client handling
> - SQLite database for user authentication and file metadata
> - Physical file storage with UUID-based naming
>
> All communication uses our custom binary protocol with JSON payloads."

**[Show server terminal with logs]**

**Presenter:**
> "Now let's see it in action."

**Action Items:**
- [ ] Point to server machine
- [ ] Briefly show server logs (running, waiting for connections)
- [ ] Mention key technologies: C, TCP, SQLite, GTK4

---

### Segment 3: Authentication (1 minute)

**Presenter (Machine 2):**

> "First, user authentication. I'll log in as a regular user named 'test1'."

**Actions:**
```
1. GUI client already open at login dialog
2. [Ensure audience can read text]
3. Type server IP: 192.168.1.100
4. Type username: test1
5. Type password: test123
6. Click "Login"
```

**[Main window appears]**

> "Successfully authenticated. The main window shows our file browser with a directory tree on the left and file list on the right. Currently empty as this is a fresh system."

**Team Lead (Machine 1):**

**[Show server log]:**
> "The server logs show the client connection and successful authentication."

**Expected Log:**
```
[INFO] Client connected from 192.168.1.101
[INFO] User 'test1' logged in successfully
```

**Action Items:**
- [ ] Login completes smoothly
- [ ] Main window displays correctly (projected)
- [ ] Server logs confirm connection
- [ ] Point out UI elements (tree, list, toolbar)

---

### Segment 4: File Operations (4 minutes)

#### 4.1: Create Directory (30 seconds)

**Presenter:**

> "Let's start by creating a directory structure."

**Actions:**
```
1. Right-click empty space in file list
2. Select "Create Directory"
3. Enter name: "Documents"
4. Click OK
```

**[Directory appears in tree and list]**

> "The directory appears immediately in both the tree view and file list."

#### 4.2: File Upload (1 minute)

**Presenter:**

> "Now I'll upload a file."

**Actions:**
```
1. Click "Upload" button in toolbar
2. File chooser appears
3. Navigate to ~/Desktop/demo-upload/
4. Select: readme.txt
5. Click "Open"
```

**[Upload completes]**

> "File uploaded successfully. Notice it shows the file size and permissions."

**[Point to file list showing: readme.txt, 247 bytes, 644]**

**Actions:**
```
Upload second file:
1. Click "Upload"
2. Select: document.pdf
3. Confirm upload
```

> "Let me upload one more file to demonstrate multiple items."

#### 4.3: Navigate Directory (30 seconds)

**Presenter:**

> "I can navigate into the directory."

**Actions:**
```
1. Double-click "Documents" folder
2. [Directory opens, shows empty]
3. Upload file into Documents:
   - Click Upload
   - Select report.docx
4. Click "Back" button or navigate in tree
5. Return to root
```

> "Navigation works like any file browser - double-click to enter, back button to return."

#### 4.4: File Download (45 seconds)

**Presenter:**

> "Let's download one of our files."

**Actions:**
```
1. Select readme.txt in file list
2. Click "Download" button
3. Save dialog appears
4. Choose: ~/Desktop/downloaded/
5. Click "Save"
```

**[Download completes]**

**Show downloaded file:**
```
CMD+Tab to Finder
Navigate to Desktop/downloaded/
Show readme.txt present
```

> "File downloaded successfully to my local machine."

**[Return to GUI client]**

#### 4.5: File Operations - Rename, Copy, Delete (1 minute)

**Presenter:**

> "We support standard file operations via context menus."

**Rename:**
```
1. Right-click document.pdf
2. Select "Rename"
3. Enter: presentation.pdf
4. Confirm
```

> "File renamed."

**Copy/Paste:**
```
1. Right-click presentation.pdf
2. Select "Copy"
3. Navigate into Documents folder
4. Right-click empty space
5. Select "Paste"
```

**[File appears in Documents]**

> "Copy and paste works between directories."

**Delete:**
```
1. Navigate back to root
2. Right-click readme.txt
3. Select "Delete"
4. Confirm deletion
```

> "File removed."

**Action Items:**
- [ ] All operations complete smoothly
- [ ] UI updates immediately
- [ ] Context menus visible on projection
- [ ] Narrate actions clearly

---

### Segment 5: Concurrent Operations (2 minutes)

**Presenter:**

> "Now let's demonstrate what makes this a true distributed system - multiple clients operating simultaneously."

**Operations (Machine 3):**

**[Start logging in]**

> [To teammate] "Go ahead and login as test2."

**Actions on Machine 3:**
```
1. Login as test2/test123
2. Main window appears
3. See files created by test1
```

**Operations (verbally):**
> "My colleague on Machine 3 has just logged in as a different user, test2."

**Presenter (Machine 2):**

**[Create directory]**
```
1. Create folder: "SharedFiles"
2. Upload a file
```

**Operations (Machine 3) - simultaneously:**
```
1. Create folder: "Images"
2. Upload image file
```

**Presenter:**

> "We're both creating folders and uploading files at the same time. Let me refresh..."

**Actions:**
```
Click "Refresh" or press F5
```

**[SharedFiles and Images both appear]**

> "Both operations completed successfully. The server handles multiple concurrent clients using a thread pool architecture."

**Team Lead (Machine 1):**

**[Show server logs]:**

> "The server logs show interleaved operations from both users."

**Expected Logs:**
```
[INFO] Command: MAKE_DIR, User: test1, Dir: SharedFiles
[INFO] Command: UPLOAD_REQ, User: test1, File: data.csv
[INFO] Command: MAKE_DIR, User: test2, Dir: Images
[INFO] Command: UPLOAD_REQ, User: test2, File: photo.jpg
```

**Action Items:**
- [ ] Machine 3 login successful
- [ ] Both users create content simultaneously
- [ ] No conflicts or errors
- [ ] Server logs show concurrent operations
- [ ] Emphasize "real distributed system"

---

### Segment 6: Admin Dashboard (2 minutes)

**Technical (Machine 4):**

**[Login as admin]**

> "I'll demonstrate the administrative features. Logging in as admin..."

**Actions:**
```
1. Login as admin/admin
2. Main window appears
3. Click "Admin" menu (or button)
4. Select "Manage Users"
```

**[Admin dashboard appears]**

> "The admin dashboard shows all users in the system."

**[Point to user list showing: admin, test1, test2]**

#### 6.1: Create User

**Actions:**
```
1. Click "Create User"
2. Enter username: demo_user
3. Enter password: demo123
4. Click "Create"
```

**[New user appears in list]**

> "New user created successfully."

#### 6.2: View Activity Logs

**Actions:**
```
1. Click "Activity Logs" button
2. Logs window appears
```

> "The system logs all user activities - logins, file uploads, deletions, everything. This provides full audit trail for security and compliance."

**[Scroll through logs showing recent operations]**

**Expected Log Entries:**
```
[2026-01-09 10:05:32] User 'test1' logged in
[2026-01-09 10:06:15] User 'test1' uploaded file 'readme.txt'
[2026-01-09 10:07:20] User 'test2' logged in
[2026-01-09 10:07:45] User 'test1' created directory 'SharedFiles'
[2026-01-09 10:07:46] User 'test2' created directory 'Images'
```

**Action Items:**
- [ ] Admin login successful
- [ ] User management interface clear
- [ ] Create user works
- [ ] Activity logs visible and detailed
- [ ] Explain security/audit purpose

---

### Segment 7: Advanced Features (2 minutes)

#### 7.1: File Permissions (45 seconds)

**Presenter (Machine 2):**

> "We implement Unix-style file permissions."

**Actions:**
```
1. Right-click a file (e.g., presentation.pdf)
2. Select "Change Permissions" or "Properties"
3. Chmod dialog appears showing current: 644
```

> "Current permissions are 644 - owner read/write, others read-only."

**Actions:**
```
1. Change to: 755 (all execute)
2. Click OK
```

**[Permissions update in file list]**

> "Permissions updated. The server enforces these for access control."

#### 7.2: File Search (1 minute 15 seconds)

**Presenter:**

> "Finally, our search functionality."

**Actions:**
```
1. In search bar at top, enter: "*.pdf"
2. Uncheck "Recursive"
3. Click "Search" button
```

**[Search results dialog appears]**

> "Search found all PDF files in the current directory."

**[Results show: presentation.pdf]**

**Actions:**
```
1. Close results
2. Enter search: "test"
3. Check "Recursive"
4. Click "Search"
```

**[Results show all files with "test" in name or in subdirectories]**

> "Recursive search finds matches throughout the entire directory tree, not just the current folder."

**Action Items:**
- [ ] Chmod dialog functions correctly
- [ ] Permissions update visible
- [ ] Search (non-recursive) works
- [ ] Recursive search finds files in subdirectories
- [ ] Results dialog clear and readable

---

### Segment 8: Conclusion (1 minute)

**Presenter:**

> "To summarize what we've demonstrated:
>
> **Core Features:**
> - Secure user authentication with password hashing
> - Full file management: upload, download, rename, delete
> - Directory operations and navigation
> - Unix-style permission system
>
> **Advanced Capabilities:**
> - True concurrent multi-client operations
> - Administrative user management
> - Comprehensive activity logging
> - Pattern-based file search
>
> **Technical Implementation:**
> - Multi-threaded C server with thread pool
> - Custom binary protocol with JSON payloads
> - SQLite database for metadata
> - GTK4 graphical interface
> - TCP socket networking
>
> The system successfully demonstrates distributed file sharing with security, concurrency, and scalability.
>
> We're ready for questions."

**All Team Members:**

**[Logout all clients]**

---

## Demo Timing Breakdown

| Segment | Duration | Cumulative | Presenter |
|---------|----------|------------|-----------|
| Introduction | 1:00 | 0:00-1:00 | Presenter |
| Architecture | 1:00 | 1:00-2:00 | Team Lead + Presenter |
| Authentication | 1:00 | 2:00-3:00 | Presenter |
| File Operations | 4:00 | 3:00-7:00 | Presenter |
| Concurrent Ops | 2:00 | 7:00-9:00 | Presenter + Operations |
| Admin Dashboard | 2:00 | 9:00-11:00 | Technical |
| Advanced Features | 2:00 | 11:00-13:00 | Presenter |
| Conclusion | 1:00 | 13:00-14:00 | Presenter |
| **TOTAL** | **14:00** | | |

**Buffer:** 1 minute for adjustments
**Target:** 12-14 minutes allows Q&A time

---

## Q&A Preparation

### Expected Questions & Answers

**Q: "How does the server handle concurrent file writes to the same file?"**

**A:** "The database uses SQLite's locking mechanism. For physical file writes, we use file-level locking with fcntl() to prevent corruption. If two clients attempt simultaneous write, one waits or receives an error."

**Q: "What happens if the server crashes?"**

**A:** "Clients receive connection error and can reconnect when server restarts. Database transactions ensure consistency - partial operations are rolled back. Physical files are atomic writes."

**Q: "How secure is the password storage?"**

**A:** "Passwords are hashed using SHA-256 before storage. We never store plaintext passwords. In production, we'd use bcrypt or Argon2 for additional security with salting."

**Q: "Can you scale this to hundreds of clients?"**

**A:** "Current implementation uses thread-per-client which scales to ~100 clients. For larger scale, we'd use epoll/kqueue for event-driven I/O or implement a connection pool with worker threads."

**Q: "How do you handle large file transfers?"**

**A:** "Files are transferred in chunks (configurable size). The protocol includes a two-phase upload: metadata first, then data in chunks. This allows progress tracking and resume capability."

**Q: "What about file synchronization between clients?"**

**A:** "Currently clients see changes on refresh. For real-time sync, we could implement file system watching on the server and push notifications to clients via a pub/sub mechanism."

**Q: "How does the permission system work?"**

**A:** "Unix-style numeric permissions (755, 644, etc.). Server checks owner_id against requesting user and evaluates read/write/execute bits before allowing operations."

**Q: "What testing did you perform?"**

**A:** "Unit tests for protocol encoding/decoding and database operations. Integration tests for client-server communication. Manual testing for UI and concurrent scenarios. We achieved 85% test pass rate."

---

## Demo Success Criteria

### Must Demonstrate Successfully
- [x] User login/authentication
- [x] File upload and download
- [x] Directory creation and navigation
- [x] Concurrent multi-client operations
- [x] Admin user management
- [x] One advanced feature (search OR permissions)

### Bonus Points
- [ ] Smooth, professional presentation
- [ ] No errors or crashes
- [ ] Clear explanation of technical concepts
- [ ] Good audience engagement
- [ ] Confident Q&A responses
- [ ] Team coordination

---

## Presentation Tips

### Do
- ✓ Speak clearly and loudly
- ✓ Face the audience, not screen
- ✓ Explain what you're doing before clicking
- ✓ Wait for operations to complete
- ✓ Point out successful results
- ✓ Emphasize "distributed" and "concurrent"
- ✓ Show enthusiasm for the project
- ✓ Handle errors gracefully (if occur)

### Don't
- ✗ Rush through steps
- ✗ Assume audience sees small details
- ✗ Use jargon without explanation
- ✗ Apologize excessively for minor issues
- ✗ Read directly from script
- ✗ Talk over teammates
- ✗ Ignore questions

---

## Visual Cues for Team

### Hand Signals (Pre-arranged)

**Thumbs up:** Ready to proceed
**Open hand (stop):** Wait, issue occurring
**Point up:** Speed up, running over time
**Point down:** Slow down, too fast
**Circle finger:** Wrap up segment

### Communication During Demo

- Minimal verbal between team members
- Use pre-arranged signals
- Stay in character/role
- Support each other if issues arise

---

## Emergency Procedures During Demo

### If Client Crashes

**Immediate Action:**
```
1. Stay calm: "Let me reconnect..."
2. Relaunch: ./gui_client
3. Login again
4. Continue from last successful step
```

**Narration:**
> "This demonstrates the robustness of client-server architecture - one client can restart without affecting others."

### If Server Crashes

**Critical - Use Contingency:**
```
1. Team Lead restarts server immediately
2. Presenter talks through architecture
3. ~30 seconds to restart
4. All clients reconnect
```

**Narration:**
> "We're experiencing a server issue. This gives me a moment to explain that in production, we'd have failover and redundancy..."

### If Network Fails

**Use Backup Plan:**
```
1. Switch to video recording of demo (if prepared)
2. Or explain with slides/diagrams
3. Show code on screen
```

### If Projection Fails

**Alternative:**
```
1. Pass laptop around (if small audience)
2. Or describe actions verbally
3. Show server logs as evidence
```

---

## Post-Demo Actions

**Immediately After:**
```
1. Thank audience
2. Remain at stations for individual questions
3. Don't shutdown yet - show code if requested
4. Distribute contact information if appropriate
```

**After Audience Leaves:**
```
1. Logout all clients
2. Stop server gracefully
3. Save logs (for reference)
4. Proceed to Phase 9: Cleanup
```

---

## Rehearsal Checklist

### Practice Session (Day Before)

**Run through entire demo:**
- [ ] Time each segment
- [ ] Practice transitions between presenters
- [ ] Test all features to be demonstrated
- [ ] Rehearse Q&A responses
- [ ] Verify timing (stay under 15 min)
- [ ] Practice with projection setup
- [ ] Test hand signals and communication

**Adjustments:**
- [ ] Cut content if running over
- [ ] Add detail if running under
- [ ] Smooth awkward transitions
- [ ] Clarify unclear explanations

---

## Next Phase

After demo completion:
→ **Phase 9: Post-Demo Cleanup** for system shutdown and equipment return

During demo if issues:
→ **Phase 7: Troubleshooting Guide** for quick fixes
→ **Phase 8: Contingency Plans** for major failures
