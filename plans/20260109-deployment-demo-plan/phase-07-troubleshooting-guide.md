# Phase 7: Troubleshooting Guide
**Quick Fixes for Common Demo Issues**

**Purpose:** Rapid problem resolution during live demonstration
**Format:** Symptom → Diagnosis → Solution (under 30 seconds)

---

## Quick Reference Matrix

| Issue | Severity | Quick Fix | Time |
|-------|----------|-----------|------|
| Client won't connect | **CRITICAL** | Check server IP, restart client | 15s |
| Server crashed | **CRITICAL** | Restart server, clients reconnect | 30s |
| Login fails | **HIGH** | Verify credentials, check database | 20s |
| GUI won't launch | **HIGH** | Check GTK4, use CLI client backup | 30s |
| Network timeout | **MEDIUM** | Check ping, restart network | 20s |
| Upload fails | **MEDIUM** | Check permissions, disk space | 15s |
| Slow performance | **LOW** | Continue demo, explain load | 5s |
| Projection fails | **MEDIUM** | Use backup laptop, describe actions | 30s |

---

## Network Issues

### Issue 1: Client Cannot Connect to Server

**Symptom:**
```
GUI client shows: "Connection refused" or "Cannot connect to server"
```

**Quick Diagnosis (5 seconds):**
```bash
# On client machine:
ping 192.168.1.100
nc -zv 192.168.1.100 8080
```

**Solutions:**

**If ping fails:**
```bash
# Wrong IP address
✓ Verify server IP: ifconfig on server machine
✓ Update client connection string
✓ Time: 15 seconds
```

**If ping works but port fails:**
```bash
# Server not running or firewall blocking
✓ Team Lead: Check server process (ps aux | grep server)
✓ If not running: ./server 8080
✓ If running: Check firewall
✓ Time: 20 seconds
```

**Demo Narration (While Fixing):**
> "We're experiencing a network connectivity issue - a common challenge in distributed systems. This demonstrates the importance of robust error handling. Let me reconnect..."

---

### Issue 2: Network Timeouts / Slow Connectivity

**Symptom:**
```
Operations taking > 5 seconds
Frequent "Connection timeout" messages
```

**Quick Diagnosis:**
```bash
ping -c 5 192.168.1.100 | grep time
# Look for high latency (>50ms) or packet loss
```

**Solutions:**

**If high latency:**
```bash
✓ Move closer to WiFi access point
✓ Switch to wired connection (if available)
✓ Continue demo with explanation
✓ Time: 5 seconds (narration only)
```

**Demo Narration:**
> "You'll notice some latency - we're on a WiFi network. In production, we'd use wired connections or optimize for high-latency scenarios with asynchronous operations."

---

### Issue 3: Intermittent Disconnections

**Symptom:**
```
Client randomly loses connection
Server logs show "Client disconnected unexpectedly"
```

**Quick Solution:**
```bash
# Keep alive pings in background
# On client machine:
ping 192.168.1.100 > /dev/null &

# Reconnect client
✓ Logout and login again
✓ Continue demo
✓ Time: 15 seconds
```

---

## Server Issues

### Issue 4: Server Won't Start

**Symptom:**
```
./server 8080
Error: Address already in use
```

**Quick Diagnosis:**
```bash
lsof -i :8080
# Shows process using port
```

**Solution:**
```bash
# Kill process and restart
lsof -i :8080 | grep LISTEN | awk '{print $2}' | xargs kill -9
./server 8080
✓ Time: 20 seconds
```

**Alternative:**
```bash
# Use different port
./server 8081
# Inform team: "Server on port 8081"
# Clients connect to 8081
✓ Time: 25 seconds
```

---

### Issue 5: Server Crashes During Demo

**Symptom:**
```
Server terminal shows: Segmentation fault
or Server becomes unresponsive
```

**CRITICAL - Immediate Action:**

**Team Lead (Machine 1):**
```bash
# 1. Restart server immediately
./server 8080
# Should be back in 2-3 seconds

# 2. Announce to team
"Server restarted"
✓ Time: 10 seconds
```

**All Clients:**
```
# Automatically try to reconnect
# Or logout and login again
✓ Time: 15 seconds per client
```

**Demo Narration:**
> "The server just restarted - demonstrating the resilience of our client-server architecture. Clients can reconnect automatically, and no data is lost due to database transactions."

**Prevention:**
```bash
# Before demo, test server stability
# Run for 30+ minutes with operations
# Check for memory leaks
valgrind --leak-check=full ./server 8080
```

---

### Issue 6: Server Logs Showing Errors

**Symptom:**
```
[ERROR] Database locked
[ERROR] File operation failed
[WARN] Thread pool exhausted
```

**Quick Actions:**

**Database Locked:**
```bash
# Check for competing connections
lsof fileshare.db
# Kill unnecessary sqlite3 processes
pkill sqlite3
✓ Time: 10 seconds
```

**File Operation Failed:**
```bash
# Check storage permissions
ls -ld storage/
chmod 755 storage/
✓ Time: 5 seconds
```

**Thread Pool Exhausted:**
```bash
# Continue demo - informative
# Explain: "This shows our connection limit working correctly"
✓ Time: 0 seconds (narration only)
```

---

## Client Issues

### Issue 7: GUI Client Won't Launch

**Symptom:**
```
./gui_client
Error: GTK initialization failed
or Segmentation fault
```

**Quick Solutions:**

**GTK4 Missing:**
```bash
# Check GTK installation
pkg-config --modversion gtk4

# If missing (unlikely if pre-verified):
# Use backup: CLI client
./client 192.168.1.100 8080
✓ Time: 30 seconds
```

**Client Binary Corrupted:**
```bash
# Use backup from USB
cp /Volumes/USB/deployment-package/gui_client .
chmod +x gui_client
./gui_client
✓ Time: 40 seconds
```

**Demo Narration:**
> "We'll switch to the command-line client for this demonstration - showing the flexibility of our system with multiple client interfaces."

---

### Issue 8: GUI Client Crashes

**Symptom:**
```
Client window disappears
Terminal shows segmentation fault
```

**Quick Solution:**
```bash
# Relaunch immediately
./gui_client
# Login again
✓ Time: 20 seconds
```

**If Persistent Crashes:**
```bash
# Fall back to CLI client
./client 192.168.1.100 8080
# Continue demo with command-line
✓ Time: 30 seconds
```

---

### Issue 9: GUI Client Frozen/Unresponsive

**Symptom:**
```
Window not responding to clicks
Beachball/spinner cursor (macOS)
```

**Quick Actions:**

**Force Quit:**
```bash
# Cmd+Option+Esc → Force Quit
# Or terminal:
pkill gui_client

# Relaunch
./gui_client
✓ Time: 25 seconds
```

**Demo Narration:**
> "The client is processing - in production we'd implement progress indicators for long operations. Let me restart the client."

---

## Authentication Issues

### Issue 10: Login Fails - Invalid Credentials

**Symptom:**
```
GUI shows: "Invalid username or password"
Server logs: [WARN] Login failed for user 'test1'
```

**Quick Diagnosis:**
```bash
# On server machine, check user exists
sqlite3 fileshare.db "SELECT username FROM users WHERE username='test1';"
```

**Solutions:**

**User Exists:**
```bash
# Typo in password - try again
# Verify credentials with team
# Default: test1/test123
✓ Time: 10 seconds
```

**User Missing:**
```bash
# Recreate user quickly
sqlite3 fileshare.db << 'EOF'
INSERT INTO users (id, username, password_hash, is_active)
VALUES (2, 'test1',
    'ecd71870d1963316a97e3ac3408c9835ad8cf0f3c1bc703527c30265534f75ae',
    1);
EOF
# Retry login
✓ Time: 30 seconds
```

---

### Issue 11: Admin Dashboard Not Accessible

**Symptom:**
```
Admin menu not visible after login
or "Permission denied" error
```

**Quick Diagnosis:**
```bash
# Check admin user
sqlite3 fileshare.db "SELECT username, id FROM users WHERE username='admin';"
```

**Solution:**
```bash
# Verify logged in as 'admin' not 'test1'
# Logout and login with correct credentials
# admin/admin
✓ Time: 15 seconds
```

---

## Database Issues

### Issue 12: Database Locked

**Symptom:**
```
Server logs: [ERROR] Database is locked
Operations fail with "database busy"
```

**Quick Solution:**
```bash
# Check what's locking database
lsof fileshare.db

# Close CLI connections
pkill sqlite3

# If persistent, restart server
killall server
./server 8080
✓ Time: 30 seconds
```

---

### Issue 13: Database Corrupted

**Symptom:**
```
Server logs: [ERROR] Database disk image is malformed
Server won't start
```

**Critical - Use Backup:**
```bash
# Copy backup database
cp fileshare.db.backup fileshare.db

# Or reinitialize (loses data)
rm fileshare.db
sqlite3 fileshare.db < ../src/database/db_init.sql
# Recreate test users (see Phase 3)
✓ Time: 60 seconds (if no backup)
✓ Time: 20 seconds (with backup)
```

---

## File Operation Issues

### Issue 14: Upload Fails

**Symptom:**
```
Upload button clicked, no progress
Error: "Upload failed"
```

**Quick Diagnosis:**
```bash
# On server, check storage directory
ls -ld storage/
df -h .  # Check disk space
```

**Solutions:**

**Permission Issue:**
```bash
chmod 755 storage/
✓ Time: 5 seconds
```

**Disk Full:**
```bash
# Free space
rm storage/old-file-*
# Or continue with smaller files
✓ Time: 10 seconds
```

**File Too Large:**
```bash
# Use smaller demo file
# Explain: "Protocol has 16MB limit for demo"
✓ Time: 5 seconds (narration)
```

---

### Issue 15: Download Fails

**Symptom:**
```
Download initiated, then error
"File not found" or "Permission denied"
```

**Quick Solution:**
```bash
# Verify file exists in database
sqlite3 fileshare.db "SELECT name, physical_path FROM files WHERE name='readme.txt';"

# Check physical file exists
ls -lh storage/[UUID-from-db]

# If missing, upload different file
✓ Time: 20 seconds
```

---

## Display / Presentation Issues

### Issue 16: Projection Not Working

**Symptom:**
```
Screen not displaying on projector
or Display mirroring not working
```

**Quick Solutions:**

**macOS Display Settings:**
```
# Press: Cmd+F1 (detect displays)
# Or: System Preferences → Displays → Detect Displays

# Use extended display mode:
System Preferences → Displays → Arrangement
Drag displays to mirror
✓ Time: 30 seconds
```

**Cable Issue:**
```
# Reseat HDMI/DisplayPort cable
# Try alternate cable
# Use adapter if needed (USB-C)
✓ Time: 45 seconds
```

**Backup Plan:**
```
# Pass laptop around (small audience)
# Or use pre-recorded video
# See Phase 8: Contingency Plans
✓ Time: Immediate
```

---

### Issue 17: Text Too Small on Projection

**Symptom:**
```
Audience cannot read text
Font sizes appear tiny
```

**Quick Solution:**
```
# Increase terminal font
Terminal → Preferences → Profiles → Font → 18pt+

# Increase GUI font (if possible)
# Or zoom in: Cmd+Plus

# Increase system UI scale
System Preferences → Displays → Scaled → Larger Text
✓ Time: 20 seconds
```

---

## Performance Issues

### Issue 18: Slow File Transfers

**Symptom:**
```
Large file upload taking too long
Progress bar stuck
```

**Quick Actions:**

**Use Smaller Files:**
```bash
# Switch to small demo files (< 1MB)
# Explain: "For time, using smaller file"
✓ Time: 5 seconds
```

**Continue in Background:**
```
# Let upload continue
# Demonstrate other features
# Return when complete
✓ Time: 0 seconds (continue demo)
```

---

### Issue 19: UI Lagging / Freezing

**Symptom:**
```
GUI responding slowly
Clicks delayed
```

**Quick Actions:**

**Continue Demo:**
```
# Usually resolves itself
# Narrate: "Processing operation..."
# Wait 5 seconds
✓ Time: 5 seconds
```

**If Persistent:**
```
# Restart client
pkill gui_client
./gui_client
# Login and continue
✓ Time: 25 seconds
```

---

## Coordination Issues

### Issue 20: Concurrent Operations Not Working

**Symptom:**
```
Machine 3 operations not visible to Machine 2
or vice versa
```

**Quick Diagnosis:**
```bash
# Check both clients connected to same server
# Verify server IP on both machines
```

**Solution:**
```bash
# Refresh file list
# Press F5 or click Refresh button

# If still not visible:
# Logout and login again on affected client
✓ Time: 15 seconds
```

---

## Emergency Contact Tree

### During Demo Issues

**Issue Detected By:**
→ **Team Lead** (primary responder)
   → Attempts quick fix (15 seconds)
   → If unsuccessful → **Contingency Plan** (Phase 8)

**Network Issues:**
→ **Team Lead** diagnoses
→ **Presenter** continues with narration

**Client Issues:**
→ **Affected Role** restarts client
→ **Presenter** covers with explanation

**Server Issues:**
→ **Team Lead** handles immediately
→ **All Roles** pause and wait for "ready" signal

---

## Troubleshooting Toolkit

### Pre-Staged Commands

**On Each Machine, Have Terminal Ready With:**

**Server (Machine 1):**
```bash
# Quick server restart
alias restart-server='killall server && ./server 8080'

# Check connections
alias check-conn='lsof -i :8080'

# Database check
alias check-db='sqlite3 fileshare.db "SELECT * FROM users;"'
```

**Clients (Machines 2, 3, 4):**
```bash
# Quick reconnect
alias reconnect='pkill gui_client && ./gui_client &'

# Network check
alias check-net='ping -c 3 192.168.1.100 && nc -zv 192.168.1.100 8080'
```

---

## Issue Escalation Matrix

### Severity Levels

**Level 1: Critical (Demo Blocker)**
- Server completely down > 30 seconds
- All clients cannot connect
- Database completely corrupted
- **Action:** Implement Contingency Plan (Phase 8)

**Level 2: High (Major Feature Broken)**
- One client cannot connect
- Authentication failing
- File uploads not working
- **Action:** Quick fix (< 30 seconds) or skip feature

**Level 3: Medium (Minor Feature Broken)**
- Search not working
- Permissions dialog issues
- Cosmetic UI problems
- **Action:** Note and continue, mention as "known issue"

**Level 4: Low (Acceptable)**
- Slight performance lag
- Minor UI glitches
- Timing slightly off
- **Action:** Continue demo normally

---

## Success Metrics

### Troubleshooting Effectiveness

**Excellent:**
- Issues resolved in < 15 seconds
- Minimal disruption to demo flow
- Professional narration during fix

**Good:**
- Issues resolved in < 30 seconds
- Some demo disruption but recovered
- Adequate explanation to audience

**Poor:**
- Issues taking > 60 seconds
- Multiple failed fix attempts
- Awkward silences
- **Switch to Contingency Plan**

---

## Prevention Checklist

### Before Demo (Minimize Issues)

**24 Hours Before:**
```
[ ] Full system test (Phase 4)
[ ] Identify potential issues
[ ] Prepare backup systems
[ ] Test all features twice
[ ] Verify network stability
```

**1 Hour Before:**
```
[ ] Final system check
[ ] Restart all machines (fresh state)
[ ] Clear caches and logs
[ ] Test critical path
[ ] Stage backup equipment
```

**10 Minutes Before:**
```
[ ] Quick connectivity test
[ ] Verify all processes running
[ ] Team sync on hand signals
[ ] Mental preparation
```

---

## Next Phase

If issues cannot be quickly resolved:
→ **Phase 8: Contingency Plans** for backup demonstration strategies

For post-demo analysis of issues:
→ **Phase 9: Post-Demo Cleanup** includes lessons learned documentation
