# Demo Quick Reference Card
**Print and carry during demonstration**

---

## System Configuration

**Server:** Machine 1 → 192.168.1.100:8080
**Client 1:** Machine 2 → test1/test123 (PROJECTED)
**Client 2:** Machine 3 → test2/test123
**Admin:** Machine 4 → admin/admin

---

## Demo Timeline (14 min)

```
0-1   Introduction
1-3   Authentication
3-7   File Operations
7-9   Concurrent Operations
9-11  Admin Dashboard
11-13 Advanced Features
13-14 Conclusion
```

---

## Hand Signals

👍 Ready / Success
✋ Wait / Issue
☝️ Speed up
👇 Slow down
👉 Your turn
⭕ Wrap up

---

## Emergency Fixes (30 seconds)

**Client won't connect:**
```
nc -zv 192.168.1.100 8080
Restart client
```

**Server port busy:**
```
lsof -i :8080 | awk 'NR>1 {print $2}' | xargs kill
./server 8080
```

**Login fails:**
```
Verify: test1/test123
Check server logs
```

**Crash → Restart:**
```
./gui_client (client)
./server 8080 (server)
```

---

## Contingency Activation

**Issue > 60 seconds:**
→ Team Lead announces: "Contingency Plan [X]"

**Plan A:** Single machine fails → Consolidate
**Plan B:** Server fails → Rapid recovery
**Plan C:** Network fails → Video/localhost
**Plan D:** Total fail → Code + slides

---

## Key Commands

**Server (Machine 1):**
```bash
./server 8080
lsof -i :8080
ps aux | grep server
```

**Client (All):**
```bash
./gui_client
ping 192.168.1.100
nc -zv 192.168.1.100 8080
```

---

## Role Reminders

**Presenter:** Lead demo, narrate, Q&A
**Team Lead:** Monitor logs, troubleshoot
**Operations:** Concurrent ops on cue
**Technical:** Admin dashboard segment

---

## Talking Points

**Architecture:**
"Multi-threaded TCP server, GTK4 clients, SQLite database, distributed system"

**Features:**
"Authentication, file management, concurrent operations, admin dashboard, Unix permissions"

**Tech Stack:**
"C language, sockets, threads, cJSON protocol, SHA-256 security"

---

## Q&A Quick Answers

**Concurrent writes?**
"SQLite locking + file-level fcntl() prevents corruption"

**Security?**
"SHA-256 hashed passwords, permission system, activity logging"

**Scalability?**
"Thread-per-client ~100 users. For more: epoll/kqueue or connection pool"

**Large files?**
"Chunked transfer with resume capability. Max 16MB for demo"

**Sync?**
"Refresh on demand. Real-time needs pub/sub with file watching"

---

## Pre-Demo Checks

```
[ ] Server running, logs visible
[ ] All clients at login screens
[ ] Projection readable
[ ] Demo files ready
[ ] Team at positions
[ ] Water available
[ ] Deep breath!
```

---

## Post-Demo

```
1. Thank audience
2. Individual questions
3. Save logs
4. Shutdown gracefully
5. Cleanup equipment
6. Team debrief
```

---

## Confidence Builders

✅ Comprehensive plan prepared
✅ All features tested
✅ Contingencies ready
✅ Team rehearsed
✅ Professional demeanor

**You got this!** 💪

---

**Emergency Contact:** Team Lead [Mobile]
**Backup:** [Name/Mobile]

---

**Fold on dotted line below to pocket size**

- - - - - - - - - - - - - - - - - - - - - - -

## DEMO SCRIPT CONDENSED

**INTRO (1 min):**
"File sharing system in C. Server + GUI clients + database. Distributed multi-client architecture."

**AUTH (1 min):**
Login test1/test123 → Main window → Server logs confirm

**FILE OPS (4 min):**
1. Create "Documents" dir
2. Upload readme.txt
3. Upload document.pdf
4. Navigate into Documents
5. Download readme.txt
6. Rename document.pdf
7. Copy file
8. Paste in different dir
9. Delete file

**CONCURRENT (2 min):**
Machine 3 logs in as test2 → Both create folders → Both upload → Refresh → Show both operations → Server logs interleaved

**ADMIN (2 min):**
Machine 4 login admin → Manage Users → Create demo_user → View Activity Logs → Explain audit trail

**ADVANCED (2 min):**
Chmod: 644→755 → Search: "*.pdf" → Recursive search → Results shown

**CONCLUDE (1 min):**
"Secure auth, file management, concurrent ops, admin features, permissions. C + TCP + SQLite + GTK4. Questions?"

---

## TIME WARNINGS

10 min → Team Lead: 🖐️ (5 min left)
12 min → Team Lead: ⭕ (wrap up)
14 min → MUST conclude

---

**Presenter Notes:**
- Speak loudly
- Narrate before clicking
- Face audience
- Professional error handling
- Watch time

**Remember:** Professional response to issues > perfect execution

---

**Good luck! 🚀**
