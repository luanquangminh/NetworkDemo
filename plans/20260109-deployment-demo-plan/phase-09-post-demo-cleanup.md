# Phase 9: Post-Demo Cleanup
**System Shutdown and Equipment Return**

**Timeline:** Immediately after demo completion
**Duration:** 10-15 minutes
**Purpose:** Clean shutdown, data preservation, equipment return

---

## Cleanup Overview

### Cleanup Phases

1. **Immediate Actions** (2 min) - Right after demo
2. **Data Preservation** (3 min) - Save logs and results
3. **System Shutdown** (3 min) - Clean process termination
4. **Equipment Cleanup** (5 min) - Disconnect and organize
5. **Documentation** (5 min) - Record observations

**Total Time:** ~20 minutes including team debrief

---

## Phase 1: Immediate Actions (2 min)

### After Last Question

**All Team Members:**
```
1. Thank the audience
2. Stay at stations for individual questions (1-2 min)
3. Do NOT shutdown yet - offer to show details if requested
4. Be available for follow-up discussions
```

**Presenter:**
> "Thank you for your attention. We're happy to answer additional questions or show you any specific features in more detail."

---

### Individual Q&A Period

**If Someone Approaches:**
```
✓ Show specific code sections
✓ Demonstrate features in detail
✓ Explain technical decisions
✓ Discuss implementation challenges
✓ Share repository/documentation links
```

**Keep Systems Running:**
- Server continues running
- Clients remain logged in
- Can demonstrate any feature on request
- Show logs, database, code

---

## Phase 2: Data Preservation (3 min)

### After Audience Leaves

**Machine 1 (Server) - Team Lead:**

**Save Server Logs:**
```bash
# Copy server logs
# If logs in terminal:
script /dev/null  # Stop if recording

# Save visible log output
# Take screenshot (Cmd+Shift+4)

# Or redirect logs to file
# If running: ./server 8080 | tee server-log.txt
cp server-log.txt ~/Desktop/demo-logs/server-$(date +%Y%m%d-%H%M).txt

# Check server statistics
lsof -i :8080  # Active connections
ps aux | grep server  # Process info
```

**Save Database State:**
```bash
# Backup post-demo database
cp fileshare.db ~/Desktop/demo-logs/fileshare-post-demo-$(date +%Y%m%d-%H%M).db

# Export activity logs
sqlite3 fileshare.db << 'EOF' > ~/Desktop/demo-logs/activity-log.txt
SELECT
    datetime(timestamp, 'unixepoch') as time,
    username,
    action_type,
    description
FROM activity_logs al
JOIN users u ON al.user_id = u.id
ORDER BY timestamp DESC
LIMIT 50;
EOF

# Export file listing
sqlite3 fileshare.db << 'EOF' > ~/Desktop/demo-logs/files-list.txt
SELECT
    f.id,
    f.name,
    f.is_directory,
    f.size,
    f.permissions,
    u.username as owner
FROM files f
JOIN users u ON f.owner_id = u.id
WHERE f.id > 0
ORDER BY f.name;
EOF
```

---

**Machines 2, 3, 4 (Clients) - All Team:**

**Take Screenshots:**
```bash
# Capture final client state
# Cmd+Shift+3 (full screen)
# Or Cmd+Shift+4 (selection)

# Save to ~/Desktop/demo-logs/
# Rename: client1-final-state.png
```

**Save Client Logs (if any):**
```bash
# If client logging enabled
cp gui-client.log ~/Desktop/demo-logs/
```

---

**Create Demo Summary Document:**

```bash
# On any machine
cat > ~/Desktop/demo-logs/demo-summary-$(date +%Y%m%d).txt << 'EOF'
File Sharing System Demo - Summary
==================================

Date: $(date)
Team Members: [List names]
Duration: [Actual demo time]

Demo Segments Completed:
[ ] Introduction and Architecture
[ ] Authentication
[ ] File Operations
[ ] Concurrent Operations
[ ] Admin Dashboard
[ ] Advanced Features
[ ] Conclusion and Q&A

Issues Encountered:
[Describe any issues]

Contingency Plans Used:
[List if any were activated]

Audience Questions:
[Key questions asked]

Overall Assessment:
[Team's self-evaluation]

Notes for Future Demos:
[Lessons learned]
EOF
```

---

## Phase 3: System Shutdown (3 min)

### Graceful Shutdown Sequence

**Step 1: Logout All Clients (30 sec)**

**Machines 2, 3, 4:**
```
1. In GUI: Click "Logout" button or File → Logout
2. Wait for logout confirmation
3. Close GUI client windows
4. Verify client processes stopped: ps aux | grep gui_client
```

**If Clients Won't Logout:**
```bash
# Force quit
pkill gui_client

# Or force kill
killall -9 gui_client
```

---

**Step 2: Stop Server (30 sec)**

**Machine 1 (Server) - Team Lead:**

**Graceful Shutdown:**
```bash
# If server supports signal handling (SIGINT)
# Press Ctrl+C in server terminal

# Server should log:
# [INFO] Received shutdown signal
# [INFO] Closing client connections...
# [INFO] Shutting down thread pool...
# [INFO] Closing database connection...
# [INFO] Server shutdown complete
```

**If Server Doesn't Stop:**
```bash
# Get PID
ps aux | grep server | grep -v grep

# Send SIGTERM
kill [PID]

# Wait 5 seconds
sleep 5

# If still running, force kill
kill -9 [PID]

# Verify stopped
lsof -i :8080  # Should show nothing
```

---

**Step 3: Verify Clean Shutdown (30 sec)**

**Check All Machines:**
```bash
# No server processes
ps aux | grep server | grep -v grep  # Should be empty

# No client processes
ps aux | grep gui_client | grep -v grep  # Should be empty

# No orphaned processes
ps aux | grep fileshare  # Should be empty

# Port released
lsof -i :8080  # Should be empty
```

---

**Step 4: Database Cleanup (30 sec)**

**Machine 1 (Server):**

**Option A: Preserve Demo Data**
```bash
# Keep database as-is for review
# Copy to safe location
cp fileshare.db ~/Documents/demo-backups/fileshare-$(date +%Y%m%d-%H%M).db

# Create backup copy
cp fileshare.db fileshare-demo-backup.db
```

**Option B: Reset for Next Demo**
```bash
# Clear all demo data
sqlite3 fileshare.db << 'EOF'
DELETE FROM files WHERE id > 0;  -- Remove all user files
DELETE FROM activity_logs;       -- Clear activity logs
VACUUM;                           -- Compact database
EOF

# Verify reset
sqlite3 fileshare.db << 'EOF'
SELECT COUNT(*) as file_count FROM files WHERE id > 0;
SELECT COUNT(*) as log_count FROM activity_logs;
EOF
# Both should be 0

# Keep users for next demo
sqlite3 fileshare.db "SELECT username FROM users;"
# Should still show: admin, test1, test2
```

---

**Step 5: Clean Storage Directory (30 sec)**

**Machine 1 (Server):**

**Option A: Archive Storage**
```bash
# Backup uploaded files
cd ~/Desktop/deployment-package
tar czf storage-backup-$(date +%Y%m%d-%H%M).tar.gz storage/

# Move to safe location
mv storage-backup-*.tar.gz ~/Documents/demo-backups/
```

**Option B: Clear Storage**
```bash
# Remove all uploaded files
rm -rf storage/*

# Or remove entire directory
rm -rf storage/

# Verify empty
ls -la storage/  # Should be empty or not exist
```

---

## Phase 4: Equipment Cleanup (5 min)

### Disconnect and Organize

**All Machines:**

**Step 1: Close Applications (1 min)**
```
1. Close all terminal windows
2. Close any text editors with code
3. Close Finder windows
4. Quit unnecessary applications
```

**Step 2: Network Cleanup (1 min)**

**If Using Static IPs:**
```
System Preferences → Network → Select Interface → Advanced
TCP/IP Tab → Configure IPv4: Using DHCP
Click OK → Apply

# Restore to automatic configuration for next users
```

**If Using WiFi:**
```
# Disconnect from lab network (if required)
# Leave connected if lab policy allows
```

---

**Step 3: File Cleanup (2 min)**

**All Machines:**
```bash
# Remove deployment package from Desktop
# (Keep original on USB/network)

# Option A: Delete
rm -rf ~/Desktop/deployment-package

# Option B: Archive
tar czf deployment-package.tar.gz ~/Desktop/deployment-package
mv deployment-package.tar.gz ~/Documents/
rm -rf ~/Desktop/deployment-package

# Clean demo files
rm -rf ~/Desktop/demo-upload
rm -rf ~/Desktop/demo-logs
rm -rf ~/Desktop/downloaded

# Remove temporary files
rm -f ~/Desktop/*.txt
rm -f ~/Desktop/demo-*
```

---

**Step 4: Restore System Settings (1 min)**

**Machine 2 (Presenter - Projected):**
```
# Restore display settings
System Preferences → Displays → Resolution: Default

# Disable Do Not Disturb
# Restore notification settings

# Restore font sizes to normal
# Terminal, browser, etc.

# Disconnect projector
# Unplug HDMI/DisplayPort cable
```

---

**Step 5: Physical Cleanup (1 min)**

**All Team Members:**
```
[ ] Disconnect power cables
[ ] Coil network cables neatly
[ ] Disconnect video cables
[ ] Organize cables by type
[ ] Return any borrowed equipment:
    - HDMI cables
    - Adapters
    - Extension cords
    - Power strips
[ ] Return USB drives to team lead
[ ] Collect any printed materials
[ ] Tidy workspace
[ ] Leave machines in original configuration
```

---

## Phase 5: Documentation (5 min)

### Immediate Documentation

**Team Lead Creates:**

**Demo Results Document:**
```markdown
# Demo Results - [Date]

## Execution Summary
- Start Time: [Time]
- End Time: [Time]
- Actual Duration: [X minutes]
- Audience Size: [Number]
- Evaluators: [Names if applicable]

## Technical Performance
- Server Uptime: [X minutes without restart]
- Client Connections: [Number of successful connections]
- Operations Performed: [Estimate]
- Concurrent Clients Peak: [Number]

## Features Demonstrated
- [x] User Authentication
- [x] Directory Navigation
- [x] File Upload/Download
- [x] File Operations (rename, copy, delete)
- [x] Directory Operations
- [x] File Permissions
- [x] File Search
- [x] Concurrent Multi-client Operations
- [x] Admin Dashboard
- [x] User Management
- [x] Activity Logging

## Issues Encountered
[List any technical issues, how resolved]

## Contingency Plans Used
[None / List which plans activated]

## Audience Questions
1. [Question] - [Answer summary]
2. [Question] - [Answer summary]
...

## Team Performance
- Coordination: [Excellent/Good/Fair]
- Timing: [On schedule/Slightly over/Under]
- Professional Demeanor: [Assessment]
- Technical Knowledge: [Assessment]

## Lessons Learned
### What Went Well
- [Item 1]
- [Item 2]

### What Could Improve
- [Item 1]
- [Item 2]

### For Next Time
- [Recommendation 1]
- [Recommendation 2]

## Media Captured
- Server logs: [Location]
- Database backup: [Location]
- Screenshots: [Location]
- Video (if recorded): [Location]

## Follow-up Actions
- [ ] Share code repository link with interested parties
- [ ] Send thank-you email to evaluators/audience
- [ ] Update documentation based on questions
- [ ] Fix any discovered bugs
- [ ] Incorporate feedback

---
Created by: [Team Lead Name]
Review Date: [Date for team review]
```

---

### Team Debrief (5 min)

**Gather team immediately after cleanup:**

**Discussion Points:**
```
1. How did we do? (Quick round-robin)
2. What went well? (Celebrate successes)
3. What issues occurred? (No blame, just facts)
4. How did we handle issues? (Effective response?)
5. Timing - too long/short? (Adjust for next time)
6. Anything to improve? (Constructive feedback)
7. Questions we couldn't answer? (Research needed)
8. Overall satisfaction? (1-10 rating from each member)
```

**Action Items:**
```
[ ] Compile feedback into document
[ ] Schedule follow-up meeting (if needed)
[ ] Distribute demo materials to team
[ ] Archive all demo assets
[ ] Update planning documents based on experience
```

---

## Cleanup Checklist

### System Cleanup
```
[ ] All clients logged out and closed
[ ] Server stopped gracefully
[ ] Database backed up
[ ] Activity logs saved
[ ] Storage archived or cleared
[ ] Processes verified stopped
[ ] Ports released (8080 available)
```

### File Cleanup
```
[ ] Demo files removed from Desktop
[ ] Temporary files deleted
[ ] Logs saved to permanent location
[ ] USB drives retrieved
[ ] Source code organized
[ ] Documentation updated
```

### Equipment Cleanup
```
[ ] Cables disconnected and organized
[ ] Projector/display disconnected
[ ] Power cables returned
[ ] Network restored to default
[ ] Display settings restored
[ ] Machines left clean and organized
```

### Documentation
```
[ ] Demo summary created
[ ] Issues documented
[ ] Lessons learned recorded
[ ] Screenshots saved
[ ] Team debrief completed
[ ] Follow-up actions identified
```

---

## Data Retention Policy

### What to Keep

**Permanent:**
```
✓ Source code (in version control)
✓ Documentation
✓ Planning documents
✓ Demo summary report
✓ Lessons learned
```

**Temporary (30 days):**
```
✓ Server logs
✓ Database backups
✓ Screenshots
✓ Activity logs
✓ Storage archives
```

**Delete After Demo:**
```
✗ Test user data
✗ Demo upload files
✗ Temporary configurations
✗ Network scripts
```

---

## Follow-Up Actions

### Within 24 Hours

**Team Lead:**
```
[ ] Email thank-you to evaluators/faculty
[ ] Share demo recording (if made)
[ ] Distribute demo summary to team
[ ] Archive all materials to shared drive
```

**All Team Members:**
```
[ ] Review demo summary
[ ] Add individual observations
[ ] Research any unanswered questions
[ ] Update personal portfolios (if applicable)
```

---

### Within 1 Week

**Team:**
```
[ ] Post-mortem meeting (if needed for major project)
[ ] Update project README with demo notes
[ ] Fix any bugs discovered during demo
[ ] Incorporate Q&A insights into documentation
[ ] Submit project (if academic requirement)
```

---

## Long-Term Preservation

### Project Archive

**Create Final Archive:**
```bash
# Complete project snapshot
cd /Users/[user]/workspace/projects/
tar czf networkFinal-demo-archive-$(date +%Y%m%d).tar.gz \
    networkFinal/ \
    --exclude=build \
    --exclude=.git \
    --exclude=storage

# Include demo materials
mkdir networkFinal-demo-package/
cp -r networkFinal/ networkFinal-demo-package/project/
cp -r demo-logs/ networkFinal-demo-package/logs/
cp demo-summary-*.txt networkFinal-demo-package/
cp demo-video.mp4 networkFinal-demo-package/  # If exists

tar czf networkFinal-complete-$(date +%Y%m%d).tar.gz \
    networkFinal-demo-package/

# Burn to USB or upload to cloud storage
```

---

### Repository Maintenance

**If Using Git:**
```bash
# Tag demo version
git tag -a demo-v1.0 -m "Version demonstrated on [date]"
git push origin demo-v1.0

# Create demo branch
git checkout -b demo-[date]
git push origin demo-[date]

# Document demo results in repository
echo "Demo conducted: [date]" >> CHANGELOG.md
echo "Results: [summary]" >> CHANGELOG.md
git add CHANGELOG.md
git commit -m "docs: add demo results for [date]"
git push
```

---

## Cleanup Success Criteria

### Complete When

**All Checkboxes Marked:**
```
[x] Systems properly shutdown
[x] Data preserved or deleted as appropriate
[x] Equipment returned/organized
[x] Documentation completed
[x] Team debriefed
[x] Follow-up actions identified
[x] Workspace clean
[x] Future improvements noted
```

**Ready to Leave:**
```
✓ No running processes
✓ No personal files on lab machines
✓ Equipment organized
✓ Team satisfied with cleanup
✓ All materials archived
✓ Lessons documented
```

---

## Post-Demo Self-Assessment

### Rate Your Demo (Each Team Member)

**Scale: 1-5 (1=Poor, 5=Excellent)**

```
Technical Execution: ___/5
Team Coordination: ___/5
Problem Solving: ___/5
Presentation Quality: ___/5
Audience Engagement: ___/5
Time Management: ___/5

Overall Demo: ___/5

Most Proud Of: ________________
Biggest Challenge: ________________
Key Learning: ________________
```

---

## Celebration

**After all cleanup complete:**

```
🎉 Congratulations on completing your demo! 🎉

Whether perfect or with minor hiccups,
you demonstrated:
- Technical competence
- Teamwork
- Problem-solving
- Professional presentation skills

Take pride in your work!

Now: Team dinner/coffee to celebrate! ☕🍕
```

---

## Final Notes

**Remember:**
- Clean shutdown preserves data integrity
- Documentation helps future demos
- Lessons learned improve next presentations
- Professional cleanup shows respect for shared resources

**Demo is complete when:**
1. ✓ Systems properly terminated
2. ✓ Equipment returned
3. ✓ Data preserved
4. ✓ Team debriefed
5. ✓ Workspace clean

---

## Appendix: Emergency Shutdown

### If Must Leave Immediately

**Minimum Actions (2 minutes):**
```
1. Stop server: killall server
2. Close all clients: killall gui_client
3. Grab USB drives
4. Lock machines (if lab policy)
5. Note what remains for later cleanup
```

**Return Later To:**
```
- Properly clean files
- Archive important data
- Complete documentation
- Thorough equipment cleanup
```

---

**End of Phase 9: Post-Demo Cleanup**

**End of Deployment Plan**

**Success!** 🚀
