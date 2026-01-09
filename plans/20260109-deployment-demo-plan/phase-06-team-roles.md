# Phase 6: Team Roles and Responsibilities
**Role Assignments for Live Demonstration**

**Team Size:** 4 members minimum (can scale to 5-6)
**Coordination:** Essential for smooth demo

---

## Role Definitions

### Role 1: Presenter / Demo Lead

**Machine:** Machine 2 (GUI Client 1) - PROJECTED
**Primary User:** test1

**Core Responsibilities:**
- Main speaker during demonstration
- Execute primary demo flow on projected screen
- Narrate all actions clearly
- Engage with audience
- Handle Q&A responses (with team support)
- Keep demo on schedule

**Skills Required:**
- Strong public speaking
- Clear, confident voice
- Ability to think on feet
- Technical knowledge of all features
- Comfortable with audience interaction

**Specific Tasks:**

**Pre-Demo (T-30 to T-0):**
```
[ ] Test projection from Machine 2
[ ] Verify GUI client launches
[ ] Test all features to demonstrate
[ ] Review demo script timing
[ ] Prepare speaking notes
[ ] Set up demo files in ~/Desktop/demo-upload/
[ ] Adjust display settings for visibility
[ ] Enable "Do Not Disturb" mode
[ ] Close unnecessary applications
```

**During Demo:**
```
[ ] Introduction and architecture overview
[ ] Login as test1
[ ] Execute file operations segment
[ ] Coordinate with Operations for concurrent demo
[ ] Execute advanced features (search, permissions)
[ ] Deliver conclusion
[ ] Lead Q&A (with team support)
```

**Communication:**
- Speaks to audience throughout
- Cues other team members verbally or with signals
- Monitors time using watch/timer

**Backup Responsibilities:**
If Technical Lead unavailable, cover admin dashboard demo

---

### Role 2: Team Lead / Server Monitor

**Machine:** Machine 1 (Server)
**Position:** Adjacent to Presenter, visible to audience

**Core Responsibilities:**
- Start and monitor server
- Watch server logs in real-time
- Verify operations executing correctly
- Provide technical narration for server-side
- First responder for server issues
- Monitor network status

**Skills Required:**
- Strong Linux/Unix command line skills
- Understanding of networking
- Debugging expertise
- Calm under pressure
- Quick problem resolution

**Specific Tasks:**

**Pre-Demo (T-120 to T-0):**
```
[ ] Set up server machine
[ ] Configure network settings
[ ] Verify database integrity
[ ] Create/verify test users
[ ] Start server on port 8080
[ ] Position terminal for audience visibility
[ ] Increase font size for readability
[ ] Verify all clients can connect
[ ] Keep server logs visible
[ ] Have backup terminal sessions ready
```

**During Demo:**
```
[ ] Monitor server logs continuously
[ ] Provide narration for server events:
    - "Client connected from..."
    - "User authenticated successfully..."
    - "File upload completed..."
[ ] Watch for errors or warnings
[ ] Ready to restart server if needed
[ ] Support Presenter with technical details
```

**Critical Monitoring:**
```bash
# Keep these visible:
# 1. Server process status
ps aux | grep server

# 2. Active connections
lsof -i :8080

# 3. Network interfaces
ifconfig | grep inet

# 4. Server logs (main terminal)
./server 8080  # Running in foreground
```

**Emergency Procedures:**
```bash
# If server hangs:
killall -9 server
./server 8080

# If port blocked:
lsof -i :8080 | grep LISTEN | awk '{print $2}' | xargs kill
./server 8080

# If database locked:
fuser -k fileshare.db
./server 8080
```

**Communication:**
- Speaks when showing server logs
- Alerts Presenter if server issues detected
- Provides technical backup during Q&A

**Backup Responsibilities:**
If Presenter unavailable, take over primary demo

---

### Role 3: Operations / Concurrent Client

**Machine:** Machine 3 (GUI Client 2)
**Primary User:** test2

**Core Responsibilities:**
- Demonstrate concurrent operations
- Execute actions simultaneously with Presenter
- Show multi-client capability
- Support file operations demo
- Be ready for backup demonstrations

**Skills Required:**
- Good timing and coordination
- Technical proficiency with GUI
- Ability to work in sync with Presenter
- Awareness of demo flow

**Specific Tasks:**

**Pre-Demo (T-60 to T-0):**
```
[ ] Set up Machine 3
[ ] Test connectivity to server
[ ] Verify GUI client launches
[ ] Prepare demo files (Images folder)
[ ] Test all operations
[ ] Review concurrent operations script
[ ] Position screen (not projected but visible)
[ ] Sync with Presenter on timing cues
```

**During Demo:**
```
[ ] Wait at login screen
[ ] On cue, login as test2
[ ] Execute concurrent operations:
    - Create directory while Presenter creates different directory
    - Upload file simultaneously with Presenter
    - Refresh to show both users' files
[ ] Stay logged in to show active concurrent session
[ ] Logout when demo concludes
```

**Concurrent Operations Segment (Key Role):**

**Cue from Presenter:**
> "Now let's demonstrate concurrent operations. Go ahead and login as test2."

**Actions:**
```
1. Login as test2/test123 (quickly)
2. Wait for next cue
3. On "let's both create folders":
   - Create directory: "Images"
   - Upload: photo.jpg
4. Observe Presenter creating "SharedFiles"
5. Both operations complete
6. Presenter explains success
```

**Timing Critical:**
- Must coordinate with Presenter
- Not too fast (audience loses track)
- Not too slow (delays demo)
- Practice timing in rehearsal

**Communication:**
- Minimal verbal during demo
- Use hand signals with Presenter
- Thumbs up when ready
- Nod when operation complete

**Backup Responsibilities:**
If Presenter's machine fails, can take over primary demo

---

### Role 4: Technical Lead / Admin

**Machine:** Machine 4 (GUI Client - Admin Dashboard)
**Primary User:** admin

**Core Responsibilities:**
- Demonstrate admin dashboard
- Show user management features
- Present activity logs
- Explain administrative capabilities
- Technical Q&A support

**Skills Required:**
- Understanding of admin features
- Clear explanation skills
- Database/backend knowledge
- Security awareness

**Specific Tasks:**

**Pre-Demo (T-60 to T-0):**
```
[ ] Set up Machine 4
[ ] Test admin login
[ ] Verify admin dashboard accessible
[ ] Review user management features
[ ] Check activity logs displaying correctly
[ ] Prepare talking points for admin segment
[ ] Practice user creation flow
```

**During Demo:**
```
[ ] Wait until Admin Dashboard segment (minute 9)
[ ] On cue, begin narration
[ ] Login as admin/admin
[ ] Open admin dashboard/menu
[ ] Show user list
[ ] Create demo user (demo_user/demo123)
[ ] Display activity logs
[ ] Explain security and audit features
[ ] Answer admin-related questions
```

**Admin Dashboard Segment (Key Role):**

**Script:**
> "I'll demonstrate the administrative features. The admin dashboard provides full control over the system."

**Actions:**
```
1. Login as admin
2. Navigate to Admin menu
3. Click "Manage Users"
4. Show user list: admin, test1, test2
5. Click "Create User"
6. Enter: demo_user / demo123
7. Confirm creation
8. Click "Activity Logs"
9. Show recent operations
10. Explain: "Full audit trail for compliance"
```

**Talking Points:**
- Administrative separation (RBAC)
- Security through activity logging
- User lifecycle management
- Compliance and audit capabilities

**Communication:**
- Primary speaker during admin segment
- Support Q&A about security and administration
- Defer complex technical questions to Team Lead

**Backup Responsibilities:**
Can assist with any client demonstrations if needed

---

## Team Coordination

### Pre-Demo Sync (T-10 minutes)

**All team members meet:**

```
1. Confirm all systems operational
2. Review hand signals
3. Verify timing: "Presenter starts at 0:00, Operations at 7:00, Technical at 9:00"
4. Address any last-minute concerns
5. Psychological prep: "We got this!"
6. Final positions check
```

### Communication Protocol

**Hand Signals:**

| Signal | Meaning | Response |
|--------|---------|----------|
| Thumbs up | Ready / Success | Acknowledge |
| Open palm (stop) | Wait / Issue | Hold action |
| Point at person | Your cue | Begin action |
| Circle finger in air | Speed up | Accelerate |
| Palm down motion | Slow down | Decelerate |
| Cut throat | Abort segment | Skip to next |

**Verbal Cues (During Demo):**

Presenter to Operations:
- "Go ahead and login as test2" → Operations logs in
- "Let's both create folders" → Concurrent action starts

Presenter to Technical:
- "Now for administrative features" → Technical begins segment

Presenter to Team Lead:
- "What does the server show?" → Team Lead narrates logs

### Timing Coordination

**Time Keeper:** Team Lead (has watch visible)

**Time Warnings (Non-verbal):**
- T+10 minutes: Hold up 5 fingers (5 min remaining)
- T+12 minutes: Wrap-up signal (circle finger)
- T+14 minutes: Cut signal if needed

---

## Role Assignment Guidelines

### Choosing Team Members

**Presenter:**
- Best speaker
- Most comfortable with public presentation
- Quick thinker
- Knows system thoroughly

**Team Lead:**
- Most technical member
- Best troubleshooter
- Command-line expert
- Calm under pressure

**Operations:**
- Good timing sense
- Team player
- Reliable
- Can work independently

**Technical Lead:**
- Explains concepts clearly
- Understanding of security/admin
- Professional demeanor
- Q&A capable

### Training Requirements

**All Team Members:**
```
[ ] Complete system walkthrough
[ ] Read all phase documents
[ ] Hands-on practice with assigned role
[ ] Full dress rehearsal (day before)
[ ] Backup role training (in case someone absent)
```

**Cross-Training:**
Each member should be capable of covering at least one other role

---

## Backup Assignments

### If 4-Person Team

**Primary Assignments:**
1. Presenter (Machine 2)
2. Team Lead (Machine 1)
3. Operations (Machine 3)
4. Technical (Machine 4)

**If Someone Unavailable:**

**Missing Presenter:**
→ Team Lead becomes Presenter
→ Technical monitors server

**Missing Team Lead:**
→ Technical becomes Team Lead
→ Presenter covers admin demo quickly

**Missing Operations:**
→ Skip concurrent demo or Presenter does both clients sequentially

**Missing Technical:**
→ Presenter covers admin demo
→ Shorten admin segment

### If 3-Person Team

**Consolidated Roles:**
1. **Presenter + Operations** (Machines 2 & 3): Switch between machines
2. **Team Lead** (Machine 1): Server monitoring
3. **Technical** (Machine 4): Admin demo

**Adjusted Demo:**
- Concurrent ops: Presenter quickly switches machines
- Slightly longer demo (16-17 min)
- Requires more rehearsal

### If 5-Person Team

**Additional Role:**

**5th Person: Backup / Floater**
- **Position:** Behind team with laptop
- **Responsibilities:**
  - Monitor time
  - Have backup builds ready on USB
  - Watch for issues
  - Ready to replace any role
  - Manage audience questions (write down)
  - Record demo (video)

**Advantages:**
- Redundancy
- Can handle issues without disrupting demo
- Better documentation

---

## Role-Specific Checklists

### Presenter Checklist

**Pre-Demo:**
```
[ ] Machine 2 set up and tested
[ ] Projection working and visible
[ ] Font sizes appropriate (18pt+)
[ ] Demo files ready
[ ] GUI client tested
[ ] Speaking notes available (but not read from)
[ ] Water available
[ ] Watch/timer visible
[ ] Deep breath, relax
```

**During Demo:**
```
[ ] Introduction delivered
[ ] Authentication demonstrated
[ ] File operations complete
[ ] Concurrent ops coordinated
[ ] Advanced features shown
[ ] Conclusion delivered
[ ] Q&A managed
```

### Team Lead Checklist

**Pre-Demo:**
```
[ ] Server running and stable
[ ] Logs visible and readable
[ ] Backup terminal sessions open
[ ] Network verified
[ ] Database checked
[ ] Emergency commands ready
[ ] Server IP documented and shared
```

**During Demo:**
```
[ ] Monitor logs continuously
[ ] Narrate server events when cued
[ ] Watch for errors
[ ] Support technical questions
[ ] Ready for emergency response
```

### Operations Checklist

**Pre-Demo:**
```
[ ] Machine 3 set up
[ ] test2 credentials ready
[ ] Demo files prepared
[ ] Timing reviewed with Presenter
[ ] Hand signals understood
[ ] Actions practiced
```

**During Demo:**
```
[ ] Login on cue
[ ] Execute concurrent operations
[ ] Stay synchronized with Presenter
[ ] Observe operations complete
[ ] Logout at conclusion
```

### Technical Lead Checklist

**Pre-Demo:**
```
[ ] Machine 4 set up
[ ] admin credentials ready
[ ] Admin dashboard tested
[ ] User creation practiced
[ ] Activity logs verified
[ ] Talking points reviewed
```

**During Demo:**
```
[ ] Admin segment delivered
[ ] User management demonstrated
[ ] Activity logs explained
[ ] Security concepts articulated
[ ] Support Q&A
```

---

## Team Meeting Schedule

### One Week Before (T-7 days)
```
- Read all documentation
- Assign roles based on strengths
- Schedule lab time for rehearsal
- Identify backups for each role
```

### Day Before (T-1 day)
```
- Full dress rehearsal in lab
- Time complete demo
- Practice with projection
- Test all features
- Refine based on timing
- Address any concerns
```

### Day of Demo (T-2 hours)
```
- Arrive early
- Set up equipment
- Run verification tests
- Team sync meeting (T-10 min)
- Final mental preparation
```

---

## Success Metrics

### Team Performance

**Excellent:**
- Smooth coordination
- No awkward pauses
- Clean hand-offs between speakers
- Professional demeanor
- Confident Q&A responses

**Good:**
- Minor coordination hiccups recovered quickly
- Most segments smooth
- Adequate Q&A handling

**Needs Improvement:**
- Poor coordination
- Long pauses
- Overlapping speakers
- Unprepared for questions

---

## Post-Demo Debrief

### Immediately After

**Quick Team Huddle (2 min):**
```
- High-fives / celebrate
- Note any issues encountered
- Stay available for individual questions
```

### Later (Same Day)

**Team Retrospective (15 min):**
```
What went well?
What could improve?
Any technical issues to fix?
Lessons learned?
Document for future demos
```

---

## Next Phase

With roles assigned and understood:
→ **Phase 5: Demo Script** (specific actions for each role)
→ **Phase 7: Troubleshooting Guide** (role-specific emergency procedures)

**Key Success Factor:** Team coordination through rehearsal and clear communication
