# Live Demo Deployment Plan
**File Sharing System - Multi-Machine Demonstration**

**Created:** 2026-01-09
**Duration:** 10-15 minutes live demo
**Environment:** macOS computer lab (4+ machines required)

---

## Overview

Comprehensive deployment and demonstration plan for file sharing system across multiple separate machines. Each system component runs on dedicated hardware to showcase distributed architecture and concurrent operations.

---

## Plan Structure

### Phase 1: Pre-Demo Preparation
**File:** `phase-01-equipment-and-setup.md`
**Duration:** 1-2 hours before demo
- Equipment checklist
- Machine assignments
- Network configuration
- Build and deployment

### Phase 2: Network Architecture
**File:** `phase-02-network-architecture.md`
**Duration:** Reference document
- Network topology
- IP addressing scheme
- Port configurations
- Firewall rules

### Phase 3: System Deployment
**File:** `phase-03-system-deployment.md`
**Duration:** 30-45 minutes setup
- Server deployment
- Client deployments
- Database setup
- Test data preparation

### Phase 4: Pre-Demo Verification
**File:** `phase-04-verification-testing.md`
**Duration:** 15-20 minutes
- Connectivity tests
- Authentication verification
- Feature validation
- Performance checks

### Phase 5: Demo Script
**File:** `phase-05-demo-script.md`
**Duration:** 10-15 minutes presentation
- Detailed demo flow
- Feature demonstrations
- Talking points
- Timing breakdown

### Phase 6: Team Roles
**File:** `phase-06-team-roles.md`
**Duration:** Reference document
- Role assignments
- Responsibilities
- Communication protocols
- Backup assignments

### Phase 7: Troubleshooting
**File:** `phase-07-troubleshooting-guide.md`
**Duration:** Reference document
- Common issues
- Quick fixes
- Emergency procedures
- Fallback strategies

### Phase 8: Contingency Plans
**File:** `phase-08-contingency-plans.md`
**Duration:** Reference document
- Backup scenarios
- Failure recovery
- Alternative demos
- Risk mitigation

### Phase 9: Post-Demo
**File:** `phase-09-post-demo-cleanup.md`
**Duration:** 10 minutes
- System shutdown
- Data cleanup
- Equipment return
- Lessons learned

---

## Quick Reference

### Minimum Requirements
- **Machines:** 4 macOS computers
- **Network:** Same local network/VLAN
- **Time:** 2 hours total (setup + demo + cleanup)
- **Team:** 4 people minimum

### Critical Success Factors
1. All machines on same network segment
2. Firewall rules configured before demo
3. Test users created and verified
4. Backup demo materials ready
5. All team members rehearsed

### Key Risks
- Network connectivity issues
- Firewall blocking connections
- Build compatibility problems
- Database synchronization issues
- Display/projection failures

---

## Timeline Summary

| Phase | Duration | When |
|-------|----------|------|
| Equipment setup | 30 min | T-120 min |
| Build & deploy | 45 min | T-90 min |
| Verification testing | 20 min | T-45 min |
| Team rehearsal | 15 min | T-25 min |
| Final checks | 10 min | T-10 min |
| **LIVE DEMO** | **12 min** | **T+0** |
| Q&A | 5 min | T+12 min |
| Cleanup | 10 min | T+17 min |

**T = Demo start time**

---

## Machine Roles

### Machine 1: Server Host
- **Role:** File sharing server
- **Component:** `build/server`
- **Requirements:** Database file, storage directory
- **Presenter:** Team Lead

### Machine 2: GUI Client (User 1)
- **Role:** Primary demonstration client
- **Component:** `build/gui_client`
- **User:** test1
- **Presenter:** Demo Lead

### Machine 3: GUI Client (User 2)
- **Role:** Secondary concurrent client
- **Component:** `build/gui_client`
- **User:** test2
- **Presenter:** Operations

### Machine 4: Admin Dashboard
- **Role:** Admin management interface
- **Component:** `build/gui_client` (admin login)
- **User:** admin
- **Presenter:** Technical Lead

### Backup: CLI Client (Optional)
- **Role:** Fallback demonstration
- **Component:** `build/client`
- **Location:** Any machine with network access

---

## Success Criteria

### Must Demonstrate
- [x] User authentication (login/logout)
- [x] Directory tree navigation
- [x] File upload from multiple clients
- [x] File download
- [x] Concurrent operations (2+ clients simultaneously)
- [x] Admin user management
- [x] File permissions (chmod)
- [x] File search with results
- [x] Context menus and operations

### Nice to Have
- [ ] Drag-and-drop file operations
- [ ] Copy-paste between directories
- [ ] Recursive directory operations
- [ ] Large file transfer progress
- [ ] Real-time directory updates

---

## Emergency Contacts

**Network Administrator:** [Lab IT contact]
**Faculty Advisor:** [Professor contact]
**Team Lead:** [Mobile number]

---

## Next Steps

1. Read all phase documents in order
2. Assign team roles (Phase 6)
3. Reserve computer lab equipment
4. Schedule rehearsal time (1 day before)
5. Prepare backup USB drives with builds
6. Create presentation slides (optional)
7. Test all equipment day before demo

---

## Notes

- All file paths assume project root: `/Users/[user]/workspace/projects/networkFinal/`
- Server default port: 8080 (configurable)
- Database file must be copied to server machine
- Storage directory created automatically by server
- Test users: admin/admin, test1/test123, test2/test123
