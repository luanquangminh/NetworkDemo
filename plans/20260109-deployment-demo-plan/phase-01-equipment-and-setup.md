# Phase 1: Equipment and Setup
**Pre-Demo Preparation**

**Timeline:** T-120 minutes (2 hours before demo)
**Duration:** 30-45 minutes
**Responsible:** All team members

---

## Equipment Checklist

### Hardware Requirements

#### Computer Systems (4 machines minimum)
- [ ] **Machine 1 (Server):** MacBook/iMac with wired network preferred
  - Memory: 4GB+ RAM
  - Storage: 2GB free space minimum
  - Display: Not critical (can be headless after setup)

- [ ] **Machine 2 (GUI Client 1):** MacBook/iMac with good display
  - Memory: 2GB+ RAM
  - Display: 1920x1080 or better (for audience visibility)
  - Mouse/trackpad: Required for demo operations

- [ ] **Machine 3 (GUI Client 2):** MacBook/iMac
  - Memory: 2GB+ RAM
  - Display: Standard resolution acceptable
  - Mouse/trackpad: Required

- [ ] **Machine 4 (Admin Dashboard):** MacBook/iMac
  - Memory: 2GB+ RAM
  - Display: Standard resolution acceptable
  - Mouse/trackpad: Required

#### Network Equipment
- [ ] **Ethernet cables:** 4x Cat5e/Cat6 cables (if using wired)
- [ ] **Network switch/hub:** If wired connections needed
- [ ] **WiFi access point:** Ensure all machines on same SSID
- [ ] **Router/gateway:** For DHCP or static IP assignment

#### Presentation Equipment
- [ ] **Projector/large display:** Connected to primary demo machine (Machine 2)
- [ ] **HDMI/DisplayPort cables:** For projection
- [ ] **Video adapters:** USB-C to HDMI (for newer Macs)
- [ ] **Extension cords/power strips:** 4 outlets minimum
- [ ] **Presentation clicker:** Optional for slides

#### Backup Equipment
- [ ] **USB flash drives:** 2x drives with complete project builds
- [ ] **External display:** Spare monitor if projection fails
- [ ] **Backup laptop:** 5th machine with complete setup (emergency)
- [ ] **Personal hotspot:** Phone with tethering capability (network backup)

### Software Requirements

#### Pre-Installed on All Machines
- [ ] **macOS:** Version 10.14+ (Mojave or later)
- [ ] **Xcode Command Line Tools:** `xcode-select --install`
- [ ] **GTK4:** `brew install gtk4` (for GUI clients)
- [ ] **SQLite3:** Usually pre-installed on macOS
- [ ] **OpenSSL:** `brew install openssl` (if needed)
- [ ] **Git:** For pulling latest code (optional)

#### Build Verification
- [ ] **GCC compiler:** `gcc --version`
- [ ] **pkg-config:** `pkg-config --version`
- [ ] **GTK4 libraries:** `pkg-config --exists gtk4`
- [ ] **SQLite3:** `sqlite3 --version`

---

## Machine Preparation

### Step 1: Reserve Lab Equipment (T-7 days)
```bash
# Contact lab administrator
# Reserve 4 adjacent machines in computer lab
# Request network access configuration
# Schedule 3-hour time block (setup + demo + cleanup)
```

**Lab Requirements:**
- Same room/adjacent tables for team coordination
- Machines on same network segment (same VLAN)
- Network ports unblocked (port 8080)
- Whiteboard/projector access for presentation

### Step 2: Download and Prepare Project (T-1 day)

On **development machine** (can be personal laptop):

```bash
# Clone or update repository
cd /Users/[username]/workspace/projects/
git clone [repository-url] networkFinal
cd networkFinal

# Build all components
make clean
make all

# Verify builds
ls -lh build/
# Should see: server, client, gui_client

# Test local execution
make status  # Should show no processes running
./build/server 8080 &
sleep 2
./build/client localhost 8080
# Test login with admin/admin
# Exit client
killall server
```

### Step 3: Create Deployment Package (T-1 day)

Create self-contained deployment archive:

```bash
# Create deployment directory
cd /Users/[username]/workspace/projects/networkFinal
mkdir -p deployment-package

# Copy binaries
cp build/server deployment-package/
cp build/gui_client deployment-package/
cp build/client deployment-package/

# Copy database
cp fileshare.db deployment-package/

# Create storage directory
mkdir -p deployment-package/storage

# Create deployment scripts
cat > deployment-package/start-server.sh << 'EOF'
#!/bin/bash
SERVER_IP=$(ifconfig | grep "inet " | grep -v 127.0.0.1 | head -1 | awk '{print $2}')
echo "Server IP: $SERVER_IP"
echo "Server starting on port 8080..."
./server 8080
EOF

cat > deployment-package/start-gui-client.sh << 'EOF'
#!/bin/bash
read -p "Enter server IP: " SERVER_IP
export SERVER_HOST=$SERVER_IP
export SERVER_PORT=8080
./gui_client
EOF

chmod +x deployment-package/*.sh

# Create README
cat > deployment-package/README.txt << 'EOF'
File Sharing System - Demo Deployment Package

DEPLOYMENT INSTRUCTIONS:

Server Machine:
1. Copy entire folder to server machine
2. Run: ./start-server.sh
3. Note the displayed IP address
4. Keep terminal open during demo

Client Machines:
1. Copy entire folder to client machine
2. Run: ./start-gui-client.sh
3. Enter server IP when prompted
4. Login with provided credentials

Test Users:
- admin / admin (Administrator)
- test1 / test123 (Regular user)
- test2 / test123 (Regular user)

Default Port: 8080
Database: fileshare.db
Storage: ./storage/

Troubleshooting:
- Check firewall: sudo /usr/libexec/ApplicationFirewall/socketfilterfw --add [path-to-server]
- Check connectivity: nc -zv [server-ip] 8080
- Check processes: ps aux | grep server
EOF

# Create archive
tar czf fileshare-demo.tar.gz deployment-package/
```

### Step 4: Prepare Backup Media (T-1 day)

```bash
# Copy to 2 USB drives
cp fileshare-demo.tar.gz /Volumes/USB1/
cp fileshare-demo.tar.gz /Volumes/USB2/

# Also include project source (for emergency rebuild)
tar czf fileshare-source.tar.gz --exclude build --exclude .git .
cp fileshare-source.tar.gz /Volumes/USB1/
```

---

## Lab Setup Day (T-120 min)

### Step 1: Physical Setup (15 min)

```
Suggested Lab Layout:

[Projector/Screen]
        |
    [Machine 2]  <-- Primary demo (projected)
    GUI Client 1
        |
[Machine 1]   [Machine 3]   [Machine 4]
  Server      GUI Client 2   Admin Dashboard

[Team Members positioned behind respective machines]
```

**Actions:**
1. Position machines according to layout
2. Connect all power cables
3. Connect Machine 2 to projector
4. Verify projector displays correctly
5. Test audio (if presentation has sound)
6. Connect network cables (if wired) or join WiFi

### Step 2: Network Configuration (10 min)

**WiFi Setup (Recommended):**
```bash
# On each machine, join same WiFi network
# Network preferences → WiFi → Connect to [Lab SSID]

# Verify connectivity
ping google.com
# Should get responses

# Note IP addresses of all machines
ifconfig | grep "inet " | grep -v 127.0.0.1
# Record IP for server machine

# Example IPs:
# Machine 1 (Server): 192.168.1.100
# Machine 2 (Client 1): 192.168.1.101
# Machine 3 (Client 2): 192.168.1.102
# Machine 4 (Admin): 192.168.1.103
```

**Wired Setup (Alternative):**
```bash
# Connect Ethernet cables to all machines
# Verify physical link (lights on port)
ifconfig en0  # Check wired interface
# Note IP addresses assigned by DHCP
```

### Step 3: Firewall Configuration (10 min)

**On Server Machine (Machine 1):**
```bash
# Check current firewall status
sudo /usr/libexec/ApplicationFirewall/socketfilterfw --getglobalstate

# If enabled, add server binary to allowed list
sudo /usr/libexec/ApplicationFirewall/socketfilterfw --add /path/to/server
sudo /usr/libexec/ApplicationFirewall/socketfilterfw --unblock /path/to/server

# Alternatively, temporarily disable firewall (for demo only)
sudo /usr/libexec/ApplicationFirewall/socketfilterfw --setglobalstate off

# WARNING: Re-enable after demo
```

**On All Client Machines:**
```bash
# Outbound connections usually allowed
# No configuration typically needed

# Verify no VPN or proxy active
scutil --proxy
# Should show no proxy settings
```

### Step 4: Deploy Software (15 min)

**On Server Machine (Machine 1):**
```bash
# Extract deployment package
cd ~/Desktop
tar xzf fileshare-demo.tar.gz
cd deployment-package

# Verify contents
ls -lh
# Should see: server, gui_client, client, fileshare.db, storage/, *.sh

# Test database
sqlite3 fileshare.db "SELECT username FROM users;"
# Should show: admin, test1, test2

# Set permissions
chmod +x server gui_client client *.sh
```

**On Client Machines (Machines 2, 3, 4):**
```bash
# Extract deployment package
cd ~/Desktop
tar xzf fileshare-demo.tar.gz
cd deployment-package

# Only need GUI client binary
ls -lh gui_client
chmod +x gui_client start-gui-client.sh

# Test GTK4 installation
pkg-config --modversion gtk4
# Should show version number (4.x.x)
```

---

## Pre-Demo Checklist

### Network Verification
- [ ] All machines on same subnet
- [ ] Server IP address documented and shared
- [ ] Port 8080 accessible (test with nc -zv [server-ip] 8080)
- [ ] No firewalls blocking connections
- [ ] Internet access available (for fallback research)

### Software Verification
- [ ] Server binary executes without errors
- [ ] GUI clients launch and show login window
- [ ] Database accessible with test users
- [ ] Storage directory writable

### Team Verification
- [ ] All team members present
- [ ] Each person at assigned machine
- [ ] Roles understood (Phase 6)
- [ ] Communication method established (hand signals, Slack, etc.)
- [ ] Demo script reviewed by all

### Presentation Verification
- [ ] Projector displays Machine 2 correctly
- [ ] Font sizes readable from back of room
- [ ] Mouse cursor visible on projection
- [ ] No screen saver interruptions (disable)
- [ ] No notification popups (enable Do Not Disturb)

### Backup Verification
- [ ] USB drives accessible
- [ ] Backup machine ready (if available)
- [ ] Mobile hotspot tested (if network backup)
- [ ] Emergency contact numbers saved

---

## Common Setup Issues

### Issue: Machines on different subnets
**Symptom:** Client cannot connect to server
**Solution:**
```bash
# Check subnet masks
ifconfig | grep netmask
# All should have same subnet (e.g., 255.255.255.0)
# Request network admin to place on same VLAN
```

### Issue: GTK4 not found
**Symptom:** GUI client fails to launch
**Solution:**
```bash
# Install GTK4
brew install gtk4
# May take 10-15 minutes

# Verify
pkg-config --libs gtk4
```

### Issue: Port 8080 already in use
**Symptom:** Server fails to start
**Solution:**
```bash
# Find process using port
lsof -i :8080
# Kill process
kill [PID]

# Or use different port
./server 8081
# Update clients to connect to 8081
```

### Issue: Database locked
**Symptom:** Server reports database errors
**Solution:**
```bash
# Check for existing connections
lsof fileshare.db
# Kill any hanging processes

# Rebuild database
rm fileshare.db*
sqlite3 fileshare.db < ../src/database/db_init.sql
# Re-create test users (see Phase 3)
```

---

## Time Management

| Task | Estimated | Actual | Notes |
|------|-----------|--------|-------|
| Physical setup | 15 min | ___ | Tables, power, projection |
| Network config | 10 min | ___ | IPs, connectivity |
| Firewall setup | 10 min | ___ | Allow server port |
| Software deploy | 15 min | ___ | Extract, test binaries |
| **Total Setup** | **50 min** | ___ | Buffer: 10 min |

**Target:** Complete by T-70 minutes to allow verification testing (Phase 4)

---

## Next Phase

Proceed to **Phase 2: Network Architecture** for detailed network topology and configuration reference.

Then move to **Phase 3: System Deployment** for server and client initialization.
