# Phase 2: Network Architecture
**Network Topology and Configuration Reference**

**Purpose:** Define network infrastructure for multi-machine deployment
**Audience:** Technical team, network administrators

---

## Network Topology

### Logical Architecture

```
                    [Internet]
                        |
                   [Lab Router]
                  (192.168.1.1)
                        |
    +---------+---------+---------+---------+
    |         |         |         |         |
[Machine 1] [Machine 2] [Machine 3] [Machine 4]
  Server     Client 1    Client 2    Admin
192.168.1.100 .101       .102        .103
```

### Physical Connections

**Option A: WiFi (Recommended)**
- All machines join same SSID
- 802.11n/ac/ax (WiFi 4/5/6)
- 2.4GHz or 5GHz band
- No infrastructure changes needed
- Adequate bandwidth for demo

**Option B: Wired Ethernet**
- Cat5e/Cat6 cables
- Gigabit connections (1000BaseT)
- Reduced latency
- More reliable
- Requires physical infrastructure

**Option C: Hybrid**
- Server on wired (stability)
- Clients on WiFi (flexibility)
- Best of both approaches

---

## IP Addressing Scheme

### Static Assignment (Recommended for Demo)

Configure static IPs to prevent DHCP lease changes during demo:

| Machine | Role | IP Address | Hostname | MAC Address |
|---------|------|------------|----------|-------------|
| Machine 1 | Server | 192.168.1.100 | fileshare-srv | [Record] |
| Machine 2 | Client 1 | 192.168.1.101 | fileshare-c1 | [Record] |
| Machine 3 | Client 2 | 192.168.1.102 | fileshare-c2 | [Record] |
| Machine 4 | Admin | 192.168.1.103 | fileshare-adm | [Record] |

**Subnet Configuration:**
- Network: 192.168.1.0/24
- Subnet Mask: 255.255.255.0
- Gateway: 192.168.1.1
- DNS: 8.8.8.8, 8.8.4.4 (Google DNS)

### macOS Static IP Configuration

**GUI Method:**
```
1. System Preferences → Network
2. Select active interface (WiFi or Ethernet)
3. Click "Advanced..."
4. TCP/IP tab
5. Configure IPv4: Manually
6. IP Address: [Assigned IP]
7. Subnet Mask: 255.255.255.0
8. Router: 192.168.1.1
9. DNS tab → Add 8.8.8.8
10. Click OK → Apply
```

**CLI Method:**
```bash
# WiFi interface (typically en0 or en1)
sudo networksetup -setmanual "Wi-Fi" 192.168.1.100 255.255.255.0 192.168.1.1
sudo networksetup -setdnsservers "Wi-Fi" 8.8.8.8 8.8.4.4

# Verify
ipconfig getifaddr en0
networksetup -getinfo "Wi-Fi"
```

### DHCP Reservation (Alternative)

Coordinate with network administrator:

```bash
# Collect MAC addresses from all machines
ifconfig en0 | grep ether
# Example: ether a8:5e:45:xx:xx:xx

# Provide to admin for DHCP reservation
# Router assigns same IP on each boot
# No manual configuration on machines
```

---

## Port Configuration

### Application Ports

| Service | Protocol | Port | Direction | Purpose |
|---------|----------|------|-----------|---------|
| File Share Server | TCP | 8080 | Inbound | Client connections |
| SSH (backup) | TCP | 22 | Inbound | Remote admin |
| HTTP (docs) | TCP | 80 | Outbound | Documentation lookup |
| HTTPS (updates) | TCP | 443 | Outbound | Package updates |

### Server Port Configuration

**Default Configuration:**
```c
// In protocol.h
#define DEFAULT_PORT 8080
```

**Custom Port (if 8080 blocked):**
```bash
# Start server on alternative port
./server 8888

# Update clients
export SERVER_PORT=8888
./gui_client
```

**Port Range Requirements:**
- Primary: 8080
- Alternatives: 8081-8089
- Ensure not in restricted range (< 1024 requires root)

### Firewall Port Allowance

**macOS Application Firewall:**
```bash
# Allow server binary
sudo /usr/libexec/ApplicationFirewall/socketfilterfw --add /path/to/server
sudo /usr/libexec/ApplicationFirewall/socketfilterfw --unblockapp /path/to/server

# Allow specific port (alternate method)
sudo /usr/libexec/ApplicationFirewall/socketfilterfw --setstealthmode off

# Check status
sudo /usr/libexec/ApplicationFirewall/socketfilterfw --listapps
```

**pfctl (Packet Filter - Advanced):**
```bash
# Check if pf enabled
sudo pfctl -s info

# Add rule to /etc/pf.conf (if needed)
# pass in proto tcp from any to any port 8080

# Reload rules
sudo pfctl -f /etc/pf.conf
```

---

## Network Requirements

### Bandwidth

**Minimum Requirements:**
- Control traffic: 10 Kbps per client
- File transfers: 1 Mbps (small files)
- Concurrent operations: 5 Mbps total

**Recommended:**
- WiFi: 802.11n (54 Mbps+)
- Wired: 100 Mbps
- Actual: 10 Mbps sustained adequate

**Demo File Sizes:**
- Small files: 10-100 KB (documents)
- Medium files: 1-5 MB (images)
- Large files: 10-50 MB (videos)

### Latency

**Target Metrics:**
- Client → Server RTT: < 10ms (LAN)
- Max acceptable: 50ms
- File transfer initiation: < 100ms
- Directory listing: < 50ms

**Measurement:**
```bash
# Ping test
ping -c 10 192.168.1.100
# Look for avg RTT

# Port connectivity
nc -zv 192.168.1.100 8080
# Should connect immediately
```

### Reliability

**Requirements:**
- Packet loss: < 0.1%
- Connection stability: No drops during demo
- Network segmentation: No VLANs between machines

**Testing:**
```bash
# Extended ping test
ping -c 100 192.168.1.100 | grep loss
# 0% packet loss expected

# Sustained connectivity
while true; do nc -zv 192.168.1.100 8080; sleep 1; done
# Should succeed continuously
```

---

## Network Security

### Lab Network Isolation

**Considerations:**
- Lab network may be isolated from campus network
- Internet access may be restricted
- Port scanning may trigger alerts
- Coordinate with IT security

**Best Practices:**
```bash
# Do not:
- Run port scans (nmap)
- Attempt to access other subnets
- Disable system-level security
- Install unauthorized software

# Do:
- Use assigned IP ranges
- Follow lab network policies
- Report issues to administrators
- Request exceptions in advance
```

### Demonstration Security

**Server Hardening:**
```bash
# Use strong admin password (change from default)
# admin:admin → admin:DemoPass2024!

# Limit server to lab subnet only (code modification)
# bind(sockfd, INADDR_ANY, ...) → bind(sockfd, 192.168.1.100, ...)

# Enable verbose logging
export LOG_LEVEL=DEBUG
./server 8080
```

**Client Precautions:**
- Do not save passwords in GUI
- Clear session data after demo
- Remove test files containing sensitive data
- Disconnect from server after demo

---

## Network Troubleshooting

### Connectivity Issues

**Symptom: Client cannot connect to server**

**Diagnostic Steps:**
```bash
# 1. Verify network interface
ifconfig | grep "inet "
# Should show assigned IP

# 2. Ping gateway
ping 192.168.1.1
# Should succeed

# 3. Ping server
ping 192.168.1.100
# Should succeed

# 4. Test port
nc -zv 192.168.1.100 8080
# Connection to 192.168.1.100 port 8080 [tcp/*] succeeded!

# 5. Check server listening
# On server machine:
lsof -i :8080
# Should show server process
```

**Solutions:**
```bash
# Different subnet → Request same VLAN from admin
# Firewall blocking → Adjust firewall (Phase 1)
# Server not running → Start server
# Wrong IP address → Verify IP with ifconfig
```

### Performance Issues

**Symptom: Slow file transfers**

**Diagnostic Steps:**
```bash
# 1. Check WiFi signal
/System/Library/PrivateFrameworks/Apple80211.framework/Versions/Current/Resources/airport -I
# Look for RSSI (signal strength)

# 2. Check network utilization
nettop -m tcp
# Look for bandwidth usage

# 3. Test raw throughput
# On server:
nc -l 9999 > /dev/null
# On client:
dd if=/dev/zero bs=1M count=100 | nc 192.168.1.100 9999
# Calculate throughput
```

**Solutions:**
- Move closer to access point
- Switch to 5GHz band (less interference)
- Use wired connection
- Reduce concurrent operations

### Intermittent Disconnections

**Symptom: Clients randomly disconnect**

**Causes:**
- Power saving mode
- Network timeout settings
- Router/AP issues
- Interference

**Solutions:**
```bash
# Disable WiFi power management
sudo /System/Library/PrivateFrameworks/Apple80211.framework/Versions/Current/Resources/airport en0 prefs DisconnectOnLogout=NO

# Keep connection alive (ping in background)
ping 192.168.1.100 > /dev/null &

# Increase TCP keepalive (server code modification)
setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval));
```

---

## Network Verification Checklist

### Pre-Demo Network Tests

**Connectivity (All Machines):**
- [ ] Ping gateway (192.168.1.1)
- [ ] Ping server (192.168.1.100)
- [ ] Ping each client machine
- [ ] DNS resolution (ping google.com)

**Port Accessibility:**
- [ ] Server listening on port 8080 (lsof -i :8080)
- [ ] Clients can connect (nc -zv server-ip 8080)
- [ ] No firewall blocking
- [ ] No proxy interference

**Performance:**
- [ ] Latency < 10ms (ping -c 10)
- [ ] 0% packet loss
- [ ] File transfer test (1MB file in < 1 second)
- [ ] Concurrent connections (all clients connect simultaneously)

**Stability:**
- [ ] 5-minute continuous ping with no drops
- [ ] Server uptime > 10 minutes without issues
- [ ] No network interface resets
- [ ] No DHCP lease changes

---

## Network Diagram

### Demo Network Layout

```
┌─────────────────────────────────────────────────────────┐
│                    Lab Network                          │
│                  192.168.1.0/24                         │
│                                                         │
│  ┌───────────┐                                          │
│  │  Router   │  192.168.1.1 (Gateway)                   │
│  │  DHCP     │                                          │
│  └─────┬─────┘                                          │
│        │                                                │
│   ┌────┴─────────────────────────┐                      │
│   │                               │                      │
│   │  Switch/WiFi AP               │                      │
│   │                               │                      │
│   └────┬──────┬──────┬───────┬───┘                      │
│        │      │      │       │                          │
│   ┌────▼───┐ ┌▼─────┐ ┌─────▼┐ ┌────▼───┐              │
│   │ Server │ │Client│ │Client│ │ Admin  │              │
│   │  .100  │ │ .101 │ │ .102 │ │  .103  │              │
│   │        │ │      │ │      │ │        │              │
│   │ :8080  │ │      │ │      │ │        │              │
│   └────────┘ └──────┘ └──────┘ └────────┘              │
│                                                         │
│   TCP Connections:                                      │
│   Client1 → Server:8080 ━━━━━━━━━━━━━━━►               │
│   Client2 → Server:8080 ━━━━━━━━━━━━━━━►               │
│   Admin   → Server:8080 ━━━━━━━━━━━━━━━►               │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

---

## IP Address Quick Reference

**Record actual IPs during setup:**

| Machine | Planned IP | Actual IP | Interface | Notes |
|---------|------------|-----------|-----------|-------|
| Server | 192.168.1.100 | _________ | en0/en1 | ____ |
| Client 1 | 192.168.1.101 | _________ | en0/en1 | ____ |
| Client 2 | 192.168.1.102 | _________ | en0/en1 | ____ |
| Admin | 192.168.1.103 | _________ | en0/en1 | ____ |

**Server Connection String:**
```
Server IP: _______________
Port: 8080
Connection URL: tcp://_______________:8080
```

**Distribute to all team members before demo start.**

---

## Next Phase

Proceed to **Phase 3: System Deployment** for server startup, client configuration, and test data preparation.
