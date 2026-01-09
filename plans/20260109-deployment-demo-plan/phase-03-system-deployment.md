# Phase 3: System Deployment
**Server and Client Initialization**

**Timeline:** T-90 minutes (after equipment setup)
**Duration:** 30-45 minutes
**Dependencies:** Phase 1 complete, network configured

---

## Deployment Overview

### Deployment Sequence

1. **Server Setup** (Machine 1) - 15 min
2. **Database Initialization** - 5 min
3. **Test Data Creation** - 10 min
4. **Client Deployment** (Machines 2, 3, 4) - 10 min
5. **Initial Connection Tests** - 5 min

**Critical Rule:** Start server BEFORE any clients

---

## Server Deployment (Machine 1)

### Step 1: Prepare Server Environment

```bash
# Navigate to deployment directory
cd ~/Desktop/deployment-package

# Verify files present
ls -lh
# Expected: server, fileshare.db, storage/, start-server.sh

# Check execute permissions
chmod +x server start-server.sh

# Verify database integrity
sqlite3 fileshare.db "PRAGMA integrity_check;"
# Should output: ok

# Check users table
sqlite3 fileshare.db "SELECT id, username, is_active FROM users;"
# Should show: admin, test1, test2
```

### Step 2: Configure Server

**Get Server IP Address:**
```bash
# Find active interface IP
ifconfig | grep "inet " | grep -v 127.0.0.1 | head -1 | awk '{print $2}'

# Example output: 192.168.1.100
# RECORD THIS IP - needed for all clients
```

**Set Environment Variables:**
```bash
# Optional: Set log level for debugging
export LOG_LEVEL=INFO  # Options: DEBUG, INFO, WARN, ERROR

# Verify storage directory
mkdir -p storage
ls -ld storage
# Should be writable: drwxr-xr-x
```

### Step 3: Start Server

**Method A: Using Start Script**
```bash
./start-server.sh
# Will display:
# Server IP: 192.168.1.100
# Server starting on port 8080...
# [2026-01-09 10:00:00] Server initialized on port 8080
# [2026-01-09 10:00:00] Thread pool created with 10 threads
# [2026-01-09 10:00:00] Waiting for connections...
```

**Method B: Direct Execution**
```bash
./server 8080
# Same output as Method A
```

**Verify Server Running:**
```bash
# In new terminal window (Cmd+T)
ps aux | grep server
# Should show process

lsof -i :8080
# Should show server listening

# Test local connection
nc -zv localhost 8080
# Connection succeeded!
```

### Step 4: Server Logging

**Monitor Server Logs:**
```bash
# Server logs to stdout
# Keep terminal visible during demo
# Watch for client connections and operations

# Expected log entries:
# [INFO] Client connected from 192.168.1.101
# [INFO] User 'test1' logged in successfully
# [INFO] Command: LIST_DIR, User: test1, Dir: /
# [INFO] Command: UPLOAD_REQ, User: test1, File: document.pdf
```

**Save Logs (Optional):**
```bash
# Redirect to file for post-demo analysis
./server 8080 2>&1 | tee server-demo.log
```

---

## Database Configuration

### Step 1: Verify Database Schema

```bash
cd ~/Desktop/deployment-package

# Check tables exist
sqlite3 fileshare.db ".tables"
# Expected: activity_logs, files, users

# Verify schema
sqlite3 fileshare.db ".schema users"
# Should show CREATE TABLE statement

sqlite3 fileshare.db ".schema files"
# Should show hierarchical file structure

sqlite3 fileshare.db ".schema activity_logs"
# Should show logging table
```

### Step 2: Verify Default Users

```bash
# List all users
sqlite3 fileshare.db << 'EOF'
SELECT
    id,
    username,
    CASE WHEN password_hash LIKE '%' THEN 'SET' ELSE 'EMPTY' END as password,
    is_active
FROM users;
EOF

# Expected output:
# 1|admin|SET|1
# 2|test1|SET|1
# 3|test2|SET|1
```

**User Credentials:**
- **admin** / admin - Administrator account
- **test1** / test123 - Regular user 1
- **test2** / test123 - Regular user 2

### Step 3: Create Test Users (If Needed)

If database is fresh or users missing:

```bash
sqlite3 fileshare.db << 'EOF'
-- Insert admin user
INSERT OR IGNORE INTO users (id, username, password_hash, is_active)
VALUES (1, 'admin',
    '8c6976e5b5410415bde908bd4dee15dfb167a9c873fc4bb8a81f6f2ab448a918',  -- SHA-256 of 'admin'
    1);

-- Insert test1 user
INSERT OR IGNORE INTO users (id, username, password_hash, is_active)
VALUES (2, 'test1',
    'ecd71870d1963316a97e3ac3408c9835ad8cf0f3c1bc703527c30265534f75ae',  -- SHA-256 of 'test123'
    1);

-- Insert test2 user
INSERT OR IGNORE INTO users (id, username, password_hash, is_active)
VALUES (3, 'test2',
    'ecd71870d1963316a97e3ac3408c9835ad8cf0f3c1bc703527c30265534f75ae',  -- SHA-256 of 'test123'
    1);

-- Verify
SELECT id, username FROM users;
EOF
```

### Step 4: Initialize Root Directory

```bash
# Ensure root directory exists (ID = 0)
sqlite3 fileshare.db << 'EOF'
INSERT OR IGNORE INTO files
    (id, parent_id, name, owner_id, is_directory, permissions, size, physical_path)
VALUES
    (0, NULL, '/', 1, 1, 755, 0, '/');

-- Verify
SELECT id, name, is_directory, permissions FROM files WHERE id = 0;
EOF

# Expected: 0|/|1|755
```

---

## Test Data Preparation

### Step 1: Create Demo Directories

```bash
cd ~/Desktop/deployment-package

# Create sample files for demo upload
mkdir -p demo-files

# Text document
cat > demo-files/readme.txt << 'EOF'
File Sharing System Demo
========================

This is a sample text file for demonstration.

Features demonstrated:
- File upload
- File download
- Directory navigation
- Permission management
- Search functionality

Created: 2026-01-09
EOF

# Create small PDF simulation (text file with .pdf extension)
cat > demo-files/document.pdf << 'EOF'
%PDF-1.4 Mock Document
This is a sample document file for upload testing.
Size: ~500 bytes
EOF

# Create image simulation
dd if=/dev/urandom of=demo-files/image.jpg bs=1024 count=100  # 100 KB
dd if=/dev/urandom of=demo-files/photo.png bs=1024 count=250  # 250 KB

# Create larger file
dd if=/dev/urandom of=demo-files/video.mp4 bs=1024 count=5000  # 5 MB

# Create additional files for search demo
echo "Configuration settings..." > demo-files/config.ini
echo "Meeting notes from Q4..." > demo-files/notes.txt
cat > demo-files/report.docx << 'EOF'
Annual Report 2025
Financial Summary
[Content for demonstration]
EOF
```

### Step 2: Organize Demo Materials

```bash
# Create directory structure for demo
mkdir -p demo-files/Documents
mkdir -p demo-files/Images
mkdir -p demo-files/Videos
mkdir -p demo-files/Shared

# Organize by type
mv demo-files/*.txt demo-files/Documents/ 2>/dev/null || true
mv demo-files/*.pdf demo-files/Documents/ 2>/dev/null || true
mv demo-files/*.docx demo-files/Documents/ 2>/dev/null || true
mv demo-files/*.ini demo-files/Documents/ 2>/dev/null || true

mv demo-files/*.jpg demo-files/Images/ 2>/dev/null || true
mv demo-files/*.png demo-files/Images/ 2>/dev/null || true

mv demo-files/*.mp4 demo-files/Videos/ 2>/dev/null || true

# List structure
tree demo-files/ || find demo-files/ -type f
```

### Step 3: Prepare for Upload

```bash
# Copy demo files to each client machine
# Machine 2 (Client 1): Documents folder
# Machine 3 (Client 2): Images folder
# Machine 4 (Admin): Configuration file

# Create distribution script
cat > prepare-client-files.sh << 'EOF'
#!/bin/bash
MACHINE=$1
case $MACHINE in
    client1)
        cp -r demo-files/Documents ~/Desktop/demo-upload/
        ;;
    client2)
        cp -r demo-files/Images ~/Desktop/demo-upload/
        ;;
    admin)
        cp demo-files/Documents/config.ini ~/Desktop/
        ;;
esac
echo "Demo files prepared for $MACHINE"
EOF

chmod +x prepare-client-files.sh
```

---

## Client Deployment

### Machine 2: GUI Client 1 (Primary Demo)

**Timeline:** After server started, T-75 min

```bash
# Navigate to deployment directory
cd ~/Desktop/deployment-package

# Verify GUI client executable
ls -lh gui_client
chmod +x gui_client

# Test GTK4 libraries
pkg-config --modversion gtk4
# Should show 4.x.x

# Prepare demo files
mkdir -p ~/Desktop/demo-upload
cp -r demo-files/Documents/* ~/Desktop/demo-upload/

# Create connection configuration
cat > client-config.txt << EOF
Server IP: 192.168.1.100
Server Port: 8080
Username: test1
Password: test123
EOF

# Test launch (don't login yet)
./gui_client &
# GUI should appear
# Close for now (Cmd+Q)
```

### Machine 3: GUI Client 2 (Concurrent Operations)

```bash
# Same setup as Machine 2
cd ~/Desktop/deployment-package
chmod +x gui_client

# Prepare different demo files
mkdir -p ~/Desktop/demo-upload
cp -r demo-files/Images/* ~/Desktop/demo-upload/

# Connection config
cat > client-config.txt << EOF
Server IP: 192.168.1.100
Server Port: 8080
Username: test2
Password: test123
EOF

# Test launch
./gui_client &
# Close after verification
```

### Machine 4: Admin Dashboard

```bash
cd ~/Desktop/deployment-package
chmod +x gui_client

# Admin credentials
cat > admin-config.txt << EOF
Server IP: 192.168.1.100
Server Port: 8080
Username: admin
Password: admin

Admin Features:
- View all users
- Create new users
- Delete users
- Modify user status
- View activity logs
EOF

# Test launch
./gui_client &
# Close after verification
```

---

## Initial Connection Tests

### Test 1: Server Accessibility

**From each client machine:**
```bash
# Test network connectivity
ping -c 3 192.168.1.100

# Test port connectivity
nc -zv 192.168.1.100 8080
# Should succeed

# Test with telnet
telnet 192.168.1.100 8080
# Should connect (Ctrl+] then 'quit' to exit)
```

### Test 2: Client Authentication

**On Machine 2 (Client 1):**
```bash
# Launch GUI client
./gui_client

# In GUI:
# 1. Login dialog should appear
# 2. Enter server IP: 192.168.1.100
# 3. Username: test1
# 4. Password: test123
# 5. Click "Login"
# 6. Should see main window with file browser
# 7. Logout immediately

# Check server logs on Machine 1
# Should show: [INFO] User 'test1' logged in successfully
```

**Repeat for Machine 3 and Machine 4 with respective credentials**

### Test 3: Basic Operations

**Quick functionality test:**
```bash
# Machine 2 (test1):
# 1. Login
# 2. Create directory "TestDir"
# 3. Upload one small file
# 4. List directory (should see new items)
# 5. Logout

# Machine 3 (test2):
# 1. Login
# 2. Should see TestDir created by test1
# 3. Create own directory "Test2Dir"
# 4. Logout

# Machine 4 (admin):
# 1. Login
# 2. Access admin dashboard
# 3. View user list (should see test1, test2 active)
# 4. Logout
```

---

## Deployment Verification Checklist

### Server Status
- [ ] Server process running (ps aux | grep server)
- [ ] Listening on port 8080 (lsof -i :8080)
- [ ] Database accessible (sqlite3 fileshare.db "SELECT * FROM users;")
- [ ] Storage directory writable
- [ ] Logs displaying in terminal

### Client Status (All Machines)
- [ ] GUI client launches without errors
- [ ] Login dialog appears
- [ ] Can connect to server
- [ ] Authentication succeeds
- [ ] Main window displays correctly
- [ ] Can logout cleanly

### Network Status
- [ ] All machines ping server successfully
- [ ] Port 8080 accessible from all clients
- [ ] No firewall blocks
- [ ] No connection timeouts

### Data Status
- [ ] Test users exist and active
- [ ] Root directory present (ID 0)
- [ ] Demo files prepared on client machines
- [ ] Storage directory empty (fresh demo)

---

## Common Deployment Issues

### Issue: Server won't start - Port already in use

**Symptom:**
```
[ERROR] Failed to bind socket on port 8080: Address already in use
```

**Solution:**
```bash
# Find process using port
lsof -i :8080
# Kill it
kill -9 [PID]

# Or use different port
./server 8081
# Update all clients to use 8081
```

### Issue: Client can't connect

**Symptom:** "Connection refused" error in GUI

**Diagnostics:**
```bash
# On client machine:
ping 192.168.1.100  # Server reachable?
nc -zv 192.168.1.100 8080  # Port open?

# On server machine:
lsof -i :8080  # Server listening?
tail -f server-logs  # Any errors?
```

**Solutions:**
- Verify server IP address
- Check firewall settings
- Restart server
- Verify client using correct port

### Issue: Authentication fails

**Symptom:** "Invalid credentials" message

**Diagnostics:**
```bash
# On server machine:
sqlite3 fileshare.db "SELECT username, length(password_hash) FROM users;"
# Should show usernames and hash length (64 for SHA-256)

# Check server logs for auth attempts
# [WARN] Login failed for user 'test1'
```

**Solutions:**
```bash
# Reset user password
sqlite3 fileshare.db << EOF
UPDATE users
SET password_hash = 'ecd71870d1963316a97e3ac3408c9835ad8cf0f3c1bc703527c30265534f75ae'
WHERE username = 'test1';
EOF
# Password is now: test123

# Verify credentials in login dialog
# Check for typos, caps lock
```

### Issue: Database locked

**Symptom:** "Database is locked" errors in server logs

**Solution:**
```bash
# Check for multiple server instances
ps aux | grep server
# Kill extras

# Check for sqlite3 CLI sessions
lsof fileshare.db
# Close unnecessary connections

# Last resort: copy database and restart
cp fileshare.db fileshare.db.backup
# Restart server
```

### Issue: GUI client won't launch

**Symptom:** Error about GTK libraries

**Solution:**
```bash
# Install GTK4
brew install gtk4

# Verify installation
pkg-config --libs gtk4

# Check environment
export PKG_CONFIG_PATH="/usr/local/lib/pkgconfig"

# Rebuild client if needed
cd [project-root]
make clean
make gui
```

---

## Deployment Timeline

| Task | Duration | Cumulative | Completed |
|------|----------|------------|-----------|
| Server setup | 5 min | T-90 | [ ] |
| Database verification | 5 min | T-85 | [ ] |
| Test data preparation | 10 min | T-80 | [ ] |
| Server startup | 2 min | T-78 | [ ] |
| Client 1 deployment | 5 min | T-75 | [ ] |
| Client 2 deployment | 5 min | T-70 | [ ] |
| Admin deployment | 5 min | T-65 | [ ] |
| Connection tests | 5 min | T-60 | [ ] |
| **Total Deployment** | **42 min** | | [ ] |

**Buffer:** 18 minutes for issues

**Target Completion:** T-60 (one hour before demo) to allow Phase 4 verification

---

## Deployment Success Criteria

### Must Achieve
- [x] Server running and accessible
- [x] All clients can connect and authenticate
- [x] Database operational with test users
- [x] Demo files prepared on all machines
- [x] Network stable and performant

### Ready for Verification Phase
- [x] All team members at stations
- [x] Server logs visible and clean
- [x] No error messages in any component
- [x] All connections tested successfully

---

## Next Phase

Proceed to **Phase 4: Pre-Demo Verification** for comprehensive testing of all features before live demonstration.

**Status Check:** All deployment tasks complete? Yes → Phase 4 | No → Troubleshoot issues
