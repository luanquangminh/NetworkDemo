# Windows Connection Guide - File Sharing System

A step-by-step guide for Windows users to connect to the cross-platform file sharing server running on macOS (192.168.0.101:8080).

---

## Table of Contents

1. [Introduction](#introduction)
2. [Prerequisites](#prerequisites)
3. [Quick Start](#quick-start)
4. [Connection Methods](#connection-methods)
5. [Downloading the Client](#downloading-the-client)
6. [Testing Connection](#testing-connection)
7. [User Credentials](#user-credentials)
8. [Features Available](#features-available)
9. [Troubleshooting](#troubleshooting)
10. [Advanced Topics](#advanced-topics)
11. [Support](#support)

---

## Introduction

The file sharing system is a C-based client-server application that allows multiple users to securely upload, download, and manage files across a network. The server is currently running on macOS at **192.168.0.101:8080** and is accessible to Windows users through multiple connection methods.

### Key Information
- **Server IP**: 192.168.0.101
- **Server Port**: 8080
- **Protocol**: TCP (custom binary protocol)
- **Supported Windows Versions**: Windows 10 or later
- **Available Clients**: CLI client (recommended), GUI client (experimental)

---

## Prerequisites

### Network Requirements

1. **Same Network Connection**: Your Windows computer must be on the same Local Area Network (LAN) as the macOS server
   - WiFi: Connected to same WiFi network
   - Ethernet: Connected to same network switch

2. **Network Verification**
   ```powershell
   # Open PowerShell and test connection to server
   ping 192.168.0.101

   # Expected output: Replies from 192.168.0.101
   ```

3. **Firewall Considerations**
   - Ensure port 8080 is accessible from your Windows machine
   - See [Firewall Issues](#firewall-issues) if you cannot ping the server

### System Requirements

- **Windows 10** (version 2004 or later) or **Windows 11**
- **RAM**: Minimum 2GB available for WSL2 or MinGW
- **Disk Space**: 500MB for toolchain and application
- **Administrator Access**: Required for WSL2 installation

---

## Quick Start

### Fastest Method: WSL2 (Recommended)

For the quickest setup, follow these steps (total time: 10-15 minutes):

1. **Open PowerShell as Administrator**
   - Right-click Start menu
   - Select "Windows PowerShell (Admin)" or "Terminal (Admin)"

2. **Enable WSL2** (if not already installed)
   ```powershell
   wsl --install
   # Or for fresh install with Ubuntu 22.04:
   wsl --install -d Ubuntu-22.04
   ```
   - Restart your computer when prompted

3. **Open WSL2 Ubuntu Terminal**
   - Search for "Ubuntu" in Start menu and open it

4. **Inside WSL2 Terminal, run:**
   ```bash
   sudo apt update && sudo apt install -y build-essential libsqlite3-dev libssl-dev
   ```

5. **Prepare the client** (copy or download binary)
   ```bash
   # If you have the binary file, copy it
   cp /mnt/c/Users/YourUsername/Downloads/client ~/client
   chmod +x ~/client

   # Or build from source
   cd ~/networkFinal
   make client
   ```

6. **Connect to server**
   ```bash
   ./client 192.168.0.101 8080
   ```

7. **Login when prompted**
   ```
   Enter username: test1
   Enter password: 123456
   ```

8. **Use interactive commands**
   ```
   > help
   > ls
   > cd documents/
   > upload /path/to/file.txt remote_file.txt
   ```

**Congratulations!** You are now connected to the file sharing server.

---

## Connection Methods

### Option 1: WSL2 (Windows Subsystem for Linux 2) - RECOMMENDED

**Best for**: Most Windows users, easiest setup, full compatibility

#### Why WSL2?
- Runs real Linux environment on Windows
- Native C compilation support
- Perfect Windows/Linux integration
- No additional software needed after installation

#### Installation Steps

**1. Install WSL2**

Open PowerShell as Administrator:
```powershell
# Install WSL2 with default Ubuntu distribution
wsl --install

# Or specify Ubuntu version
wsl --install -d Ubuntu-22.04
```

You will be prompted to restart. Do so.

**2. Launch Ubuntu Terminal**

After restart, open the Ubuntu app from Start menu or search for "Ubuntu".

**3. Update Package Manager**

```bash
sudo apt update
sudo apt upgrade -y
```

**4. Install Dependencies**

```bash
# Install C compiler and build tools
sudo apt install -y build-essential gcc make

# Install required libraries
sudo apt install -y libsqlite3-dev libssl-dev
```

**5. Get the Client Binary**

*Option A: Download Binary from HTTP Server*

If the macOS server hosts files:
```bash
cd ~
wget http://192.168.0.101:8080/client
chmod +x client
```

*Option B: Copy Binary from Windows*

```bash
# In WSL2 Ubuntu terminal
cp /mnt/c/Users/YourUsername/Downloads/client ~/
chmod +x ~/client
```

*Option C: Build from Source*

```bash
# Clone or copy project to WSL2
# Using Windows File Explorer: \\wsl$\Ubuntu-22.04
# Copy networkFinal folder

cd ~/networkFinal
make client
```

**6. Run the Client**

```bash
# From home directory
./client 192.168.0.101 8080

# Or with full path
~/client 192.168.0.101 8080
```

Expected output:
```
Connected to server at 192.168.0.101:8080
Enter username:
```

---

### Option 2: MinGW/MSYS2

**Best for**: Users who prefer native Windows tools, developers familiar with MSYS2

#### Why MinGW?
- Native Windows compilation (no Linux layer)
- Direct Windows integration
- Familiar to some developers
- No WSL2 required

#### Installation Steps

**1. Download and Install MSYS2**

Visit https://www.msys2.org/ and download the installer (usually `msys2-x86_64-...exe`)

Run the installer with default settings.

**2. Launch MSYS2 MinGW 64-bit**

Open Start menu and select: **MSYS2 → MSYS2 MinGW 64-bit**

**3. Update Package Database**

```bash
pacman -Syu
# May ask to restart MSYS2, do so
```

**4. Install Dependencies**

```bash
pacman -S base-devel
pacman -S mingw-w64-x86_64-gcc
pacman -S mingw-w64-x86_64-make
pacman -S mingw-w64-x86_64-sqlite3
pacman -S mingw-w64-x86_64-openssl
```

**5. Navigate to Project**

```bash
# If copied to Windows drive
cd /c/Users/YourUsername/Downloads/networkFinal

# Or mount any path
cd /d/Projects/networkFinal
```

**6. Build Client**

```bash
make client
```

Binary will be in `build/client`

**7. Run the Client**

```bash
./build/client 192.168.0.101 8080
```

---

### Option 3: Cygwin

**Best for**: Users with older Windows versions (pre-Windows 10), compatibility

#### Installation Steps

**1. Download Cygwin Installer**

Visit https://cygwin.com/install.html and download `setup-x86_64.exe`

**2. Run Installer**

- Choose "Install from Internet"
- Select root installation directory (default: `C:\cygwin64`)
- When prompted, select these packages:
  - `gcc-core`
  - `make`
  - `sqlite3`
  - `libssl-devel`
  - `pkg-config`

**3. Open Cygwin Terminal**

Search for "Cygwin64 Terminal" in Start menu and open it

**4. Navigate to Project**

```bash
# If on Windows C: drive
cd /cygdrive/c/Users/YourUsername/Downloads/networkFinal
```

**5. Build Client**

```bash
make client
```

**6. Run the Client**

```bash
./build/client 192.168.0.101 8080
```

---

## Downloading the Client

### Method 1: From HTTP File Server

If the macOS server is hosting files on port 8080:

```bash
# Windows PowerShell
curl -O http://192.168.0.101:8080/client

# Or using wget (if installed)
wget http://192.168.0.101:8080/client
```

### Method 2: Direct File Transfer

**Using Windows File Sharing:**

1. Locate the macOS server on the network
2. Open network file browser in Windows File Explorer
3. Navigate to the server's shared folder
4. Copy the `client` or `client.exe` file to your Windows machine

**Using USB Drive:**

1. Copy client binary to USB drive on macOS
2. Insert USB drive into Windows machine
3. Copy client to desired location (e.g., Desktop, Downloads)

### Method 3: Build from Source

See the [Building the Client](#option-1-wsl2-windows-subsystem-for-linux-2---recommended) section for instructions to compile the client yourself.

---

## Testing Connection

### Step 1: Verify Network Connectivity

Open PowerShell and test ping:

```powershell
ping 192.168.0.101
```

Expected output:
```
Pinging 192.168.0.101 with 32 bytes of data:
Reply from 192.168.0.101: bytes=32 time=5ms TTL=64
```

If ping fails, see [Network Troubleshooting](#network-troubleshooting).

### Step 2: Test Port Connectivity

```powershell
# Test if port 8080 is accessible
Test-NetConnection -ComputerName 192.168.0.101 -Port 8080
```

Expected output:
```
TcpTestSucceeded : True
```

### Step 3: Launch Client

Using WSL2:
```bash
./client 192.168.0.101 8080
```

Or MinGW:
```bash
./build/client 192.168.0.101 8080
```

### Step 4: Attempt Login

When prompted:
```
Enter username: test1
Enter password: 123456
```

Expected success output:
```
Connected to server at 192.168.0.101:8080
Login successful!
Current directory: /
>
```

---

## User Credentials

### Available Test Users

| Username | Password | Type | Purpose |
|----------|----------|------|---------|
| `admin` | `admin` | Administrator | Full system access, admin dashboard |
| `test1` | `123456` | Regular User | File operations (upload/download) |

### Using Different Users

**Login as Admin (For Administrative Tasks):**
```bash
./client 192.168.0.101 8080
# When prompted:
# Enter username: admin
# Enter password: admin
```

**Login as Test User (For Regular File Operations):**
```bash
./client 192.168.0.101 8080
# When prompted:
# Enter username: test1
# Enter password: 123456
```

### Logout and Re-login

```
> exit
Disconnecting...
Connection closed.
```

To login again, simply run the client again:
```bash
./client 192.168.0.101 8080
```

---

## Features Available

### File Operations

#### List Directory Contents
```
> ls
.
..
documents/
photos/
readme.txt
```

#### Navigate Directories
```
> cd documents/
Current directory: /documents/

> pwd
/documents/

> cd ..
Current directory: /
```

#### Create Directory
```
> mkdir my_folder
Directory created: /my_folder
```

#### Upload File
```
> upload C:\Users\YourName\file.txt remote_file.txt
Uploading: file.txt (5120 bytes)
Upload complete: 100%
```

#### Download File
```
> download remote_file.txt C:\Users\YourName\Downloads\file.txt
Downloading: remote_file.txt (5120 bytes)
Download complete: 100%
```

#### Delete File
```
> delete remote_file.txt
File deleted: /remote_file.txt
```

### Permissions Management

#### Change File Permissions
```
> chmod myfile.txt 755
Permissions changed: /myfile.txt -> 755
```

Common permission values:
- `755` - Read/write/execute for owner, read/execute for others
- `644` - Read/write for owner, read for others
- `700` - Full permissions for owner only
- `600` - Read/write for owner only

### User Information

```
> whoami
Current user: test1

> pwd
/documents/
```

### Get Help

```
> help
Available commands:
  ls [path]           - List directory contents
  cd [path]          - Change directory
  mkdir [path]       - Create directory
  upload [src] [dst] - Upload file
  download [src] [dst] - Download file
  delete [path]      - Delete file
  chmod [path] [mode] - Change permissions
  pwd                - Print working directory
  whoami             - Show current user
  exit               - Disconnect and exit
  help               - Show this help
```

---

## Troubleshooting

### Network Troubleshooting

#### Cannot Ping Server (192.168.0.101)

**Problem**: `ping 192.168.0.101` returns "Destination host unreachable" or times out

**Solutions**:

1. **Verify Server is Running**
   - Ask macOS user to confirm server is running: `./build/server 8080`
   - Check that server shows "Server listening on port 8080..."

2. **Verify Network Connection**
   - Ensure both computers are on same WiFi network
   - Check IP address on Windows: `ipconfig`
   - Verify WiFi name matches on both computers

3. **Check Server IP Address**
   - On macOS, run: `ifconfig | grep "inet "`
   - Verify IP address in ping command matches actual server IP
   - Example: if server shows `inet 192.168.1.50`, use that address

4. **Check Firewall**
   - See [Firewall Issues](#firewall-issues) section below

#### Firewall Issues

**Problem**: Can ping server but cannot connect to port 8080

**Solution 1: Windows Firewall (Windows 10/11)**

1. Open Windows Defender Firewall with Advanced Security
   - Start Menu → "Windows Defender Firewall with Advanced Security"

2. Click "Inbound Rules" on left sidebar

3. Click "New Rule" on right sidebar

4. Configure rule:
   - Rule Type: Select **Port**
   - Protocol: Select **TCP**
   - Specific local ports: Enter **8080**
   - Action: Select **Allow**
   - Apply to: Domain, Private, Public (check all)

5. Finish and test connection

**Solution 2: Disable Firewall (Temporary Testing)**

```powershell
# Run as Administrator
netsh advfirewall set allprofiles state off

# To re-enable
netsh advfirewall set allprofiles state on
```

**Solution 3: macOS Server Firewall**

Ask macOS user to:
1. System Settings → Security & Privacy → Firewall Options
2. Allow port 8080 through firewall
3. Or disable firewall temporarily for testing

### Connection Issues

#### "Connection refused" Error

**Problem**: Client shows "Connection refused" when attempting to connect

**Solutions**:

1. **Verify Server is Running**
   ```bash
   # On macOS
   ps aux | grep "build/server"

   # Should show process like:
   # .../build/server 8080
   ```

2. **Check Server Port**
   ```bash
   # On macOS
   lsof -i :8080

   # Should show:
   # COMMAND  PID  USER  FD  TYPE  DEVICE  SIZE/OFF  NODE  NAME
   # server   123  user  3   IPv4  ...     8080
   ```

3. **Verify Connection Settings**
   - Double-check server IP address (192.168.0.101)
   - Double-check port number (8080)
   - Ensure you're not using a different port

#### "No route to host" Error

**Problem**: Network connectivity issue between computers

**Solutions**:

1. **Verify Computers on Same Network**
   - Check WiFi SSID name is identical on both machines
   - Check both are not on VPN or separate networks

2. **Check Network Subnet**
   - Run `ipconfig` on Windows
   - Run `ifconfig` on macOS
   - Verify IP addresses are in same range (e.g., both 192.168.0.x)

3. **Restart Networking**
   ```powershell
   # Windows
   ipconfig /release
   ipconfig /renew
   ```

### Authentication Issues

#### "Login failed. Invalid credentials."

**Problem**: Incorrect username or password

**Solutions**:

1. **Verify Credentials**
   - Check username spelling (case-sensitive)
   - Check password spelling exactly
   - Default users: `admin`/`admin` or `test1`/`123456`

2. **Attempt Retry**
   - Most login failures allow retry
   - Enter correct credentials on next attempt

3. **Verify User Exists**
   - Ask macOS admin to verify user exists in database
   - Run query: `SELECT * FROM users;` in SQLite

#### "Authentication failed" Error

**Problem**: Connection succeeds but login fails

**Solutions**:

1. **Check Server Logs**
   - Ask macOS user to check server output for error messages
   - Server should show authentication attempt

2. **Verify Database**
   - Confirm user table is populated with test users
   - Database location: `fileshare.db`

### File Transfer Issues

#### Upload Fails with "File not found"

**Problem**: Upload command returns error about local file

**Solutions**:

1. **Verify File Path**
   - Use full path: `upload C:\Users\YourName\Documents\file.txt`
   - Not relative path: `upload file.txt` (may fail)

2. **Check File Permissions**
   - Ensure file is readable by your user account
   - Right-click file → Properties → Security

3. **Use Correct Path Format**
   - Windows paths use backslash: `C:\Users\...\file.txt`
   - Or forward slash: `C:/Users/.../file.txt`

#### Download Fails with "Permission denied"

**Problem**: Download command returns permission error

**Solutions**:

1. **Verify Destination Directory**
   - Ensure directory exists: `C:\Users\YourName\Downloads\`
   - Ensure you have write permissions

2. **Check File Permissions on Server**
   - File may be restricted for your user
   - Ask admin to check permissions: `> chmod filename 644`

3. **Free Disk Space**
   - Ensure Windows drive has enough free space
   - Check: File Explorer → C: drive properties

#### File Transfer Times Out

**Problem**: Upload/download hangs or times out

**Solutions**:

1. **Check Network Connection**
   - Test ping again: `ping 192.168.0.101`
   - If slow or unstable, restart WiFi

2. **Try Smaller File**
   - Large files may take longer
   - Start with 1MB file to verify it works

3. **Check Server Disk Space**
   - Ask macOS admin to check: `df -h`
   - May be out of disk space

4. **Restart Server and Client**
   ```bash
   # Server side (macOS)
   pkill -f "build/server"
   ./build/server 8080

   # Client side (Windows)
   # Exit client and reconnect
   ```

---

## Advanced Topics

### GUI Client on Windows

The file sharing system includes an experimental GTK-based GUI client (primarily for Linux/macOS). GUI support on Windows is limited.

**Current Status**:
- **Not recommended** for Windows production use
- CLI client is stable and feature-complete
- GUI client requires XQuartz or additional setup

### Building from Source on Windows

If you prefer to compile the entire project:

**Using WSL2:**
```bash
cd ~/networkFinal
make clean
make all
```

**Using MinGW:**
```bash
cd /c/Projects/networkFinal
make clean
make all
```

Binaries will be in `build/` directory:
- `build/client` - CLI client executable
- `build/server` - Server executable (for reference only)

### Cross-Platform Development

If contributing to the project from Windows:

1. Use WSL2 for development (closer to Linux environment)
2. Follow Unix line endings (LF, not CRLF)
3. Use forward slashes in paths within code
4. Test on actual Linux/macOS before submitting

### Performance Optimization

**For Large File Transfers**:

1. **Check Network Speed**
   ```powershell
   # Test download speed
   Invoke-WebRequest -Uri http://192.168.0.101:8080/largefile -OutFile test.tmp
   ```

2. **Optimize for Stability**
   - Use wired Ethernet instead of WiFi (if possible)
   - Close other network-intensive applications
   - Move close to WiFi router

3. **Monitor Server Load**
   - Ask macOS admin to check: `top` or Activity Monitor
   - Disconnect other clients if server is busy

---

## Support

### Getting Help

If you encounter issues not covered in this guide:

1. **Check Server Logs**
   - Ask macOS admin to run: `./build/server 8080 | tee server.log`
   - Share server.log output

2. **Provide Connection Details**
   - Your Windows IP: `ipconfig`
   - Server status: Can you ping? Port 8080 accessible?
   - Error message: Full text with context
   - Steps to reproduce: Exact commands run

3. **Gather System Information**
   ```powershell
   # Run in PowerShell
   systeminfo | findstr /C:"OS Name" /C:"OS Version" /C:"System Type"
   ```

### Common Questions

**Q: Can I use the server from outside the local network?**

A: Not recommended. The application is designed for local networks only. Communications are not encrypted. Use only on trusted LANs.

**Q: What's the maximum file size I can upload?**

A: Currently 16 MB per file. Larger files need to be compressed or split.

**Q: Can multiple Windows users connect simultaneously?**

A: Yes, the server supports multiple concurrent clients. Each user can be connected from different computers.

**Q: Do I need to keep the client running in the background?**

A: No. The client is interactive. Once you exit (`> exit`), the connection closes. You can reconnect anytime.

**Q: Where are my files stored?**

A: Files are stored on the macOS server in a `storage/` directory. Users only access them through the client interface.

**Q: Can I automate file transfers with scripts?**

A: The current client is interactive. Automation would require developing a programmatic interface or wrapping the client in a script with input piping.

---

## Next Steps

1. **Complete Setup**: Follow one of the connection methods above
2. **Test Connection**: Verify you can ping and connect to server
3. **Login**: Use provided credentials (test1/123456)
4. **Try File Operations**: Upload and download a test file
5. **Explore Features**: Try all commands using `help`
6. **Share with Others**: Provide them this guide to connect their Windows machines

---

## Additional Resources

- **Project Documentation**: See `SETUP_GUIDE.md` in project root
- **Protocol Details**: See `docs/protocol_spec.md`
- **API Reference**: See `docs/api_reference.md`
- **Troubleshooting Server**: Ask macOS admin to check `docs/current_status.md`

---

**Last Updated**: January 6, 2026
**Tested On**: Windows 10, Windows 11
**Client Compatibility**: Windows 10 or later (with WSL2, MinGW, or Cygwin)
**Server Status**: Active at 192.168.0.101:8080

---

## Version History

- **v1.0** (January 6, 2026)
  - Initial Windows connection guide
  - WSL2, MinGW, and Cygwin setup instructions
  - Comprehensive troubleshooting section
  - Features and usage examples
  - Testing procedures
