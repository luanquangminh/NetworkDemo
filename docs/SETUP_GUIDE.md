# File Sharing System - Complete Setup & Usage Guide

A cross-platform C-based client-server file sharing application with CLI and GTK GUI clients.

## Table of Contents

1. [Quick Start](#quick-start)
2. [System Requirements](#system-requirements)
3. [macOS Setup](#macos-setup)
4. [Windows Setup](#windows-setup)
5. [Building the Application](#building-the-application)
6. [Running the System](#running-the-system)
7. [Network Configuration](#network-configuration)
8. [CLI Client Usage](#cli-client-usage)
9. [GUI Client Usage](#gui-client-usage)
10. [Command Reference](#command-reference)
11. [Troubleshooting](#troubleshooting)
12. [Security Notes](#security-notes)
13. [FAQ](#faq)

---

## Quick Start

### For macOS (Already Compiled)

```bash
# Terminal 1: Start the server
cd /Users/minhbohung111/workspace/projects/networkFinal
./build/server 8080

# Terminal 2: Start the CLI client
./build/client localhost 8080
# Login with: admin / admin

# Or start the GUI client
./build/gui_client
```

### For Windows or Fresh Build

```bash
# See Windows Setup or Building sections below for initial setup
# Then follow the same commands above
```

---

## System Requirements

### Common Requirements
- **TCP Network**: Both systems must be on same network (LAN)
- **Port Availability**: Port 8080 (default) must be available
- **Database**: SQLite3 (embedded, no installation needed)
- **Memory**: Minimum 50MB RAM
- **Storage**: 100MB for application and dependencies

### macOS Requirements
- **OS Version**: macOS 10.12 (Sierra) or later
- **Compiler**: Xcode Command Line Tools
- **Package Manager**: Homebrew (optional but recommended)

### Windows Requirements
- **OS Version**: Windows 10 or later
- **Build Tools**: One of the following:
  - WSL2 (Windows Subsystem for Linux 2) - RECOMMENDED
  - MinGW/MSYS2
  - Cygwin
  - Visual C++ with Unix tools compatibility

### GUI Client Requirements (macOS + Linux)
- **GTK+3**: GTK development libraries
- **XQuartz** (macOS only): X11 server

---

## macOS Setup

### Option 1: Using Pre-Built Binaries (Fastest)

The build directory contains pre-compiled binaries for macOS:

```bash
cd /Users/minhbohung111/workspace/projects/networkFinal
ls -la build/
# Output: client, gui_client, server
```

**No setup needed. Jump to [Running the System](#running-the-system).**

### Option 2: Fresh Build from Source

#### 1. Install Xcode Command Line Tools

```bash
# Check if already installed
gcc --version

# If not installed:
xcode-select --install
```

A dialog will appear. Click "Install" and wait for completion.

#### 2. Install Dependencies via Homebrew

```bash
# Install Homebrew if not present
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install required packages
brew install sqlite3 openssl pkg-config

# For GUI client (optional)
brew install gtk+3
```

#### 3. Download/Navigate to Project

```bash
cd /Users/minhbohung111/workspace/projects/networkFinal
```

#### 4. Build Everything

```bash
# Build all components
make clean
make all

# Or specific components:
make server        # Build server only
make client        # Build CLI client only
make gui          # Build GUI client only
make tests        # Build test suite
```

**Output binaries** will be in `./build/`:
- `server` - Server executable
- `client` - CLI client executable
- `gui_client` - GTK GUI client executable

#### 5. Verify Build Success

```bash
# Test server
./build/server --help 2>&1 || echo "Server ready"

# Test client
./build/client --help 2>&1 || echo "Client ready"

# Test GUI
file ./build/gui_client
```

---

## Windows Setup

### Option 1: Using WSL2 (RECOMMENDED)

WSL2 allows you to run the native Linux C application on Windows.

#### Prerequisites
- Windows 10 version 2004 or later (or Windows 11)
- At least 4GB RAM available for WSL2

#### Installation Steps

**1. Enable WSL2**

Open PowerShell as Administrator and run:

```powershell
# Enable required Windows features
wsl --install

# If you already have WSL1, upgrade to WSL2:
wsl --set-default-version 2
```

Restart your computer.

**2. Install a Linux Distribution**

```powershell
# Install Ubuntu 22.04
wsl --install -d Ubuntu-22.04

# Set as default
wsl --setdefault Ubuntu-22.04
```

First launch will ask for username/password. Set these up.

**3. Inside WSL2 Ubuntu Terminal**

```bash
# Update package lists
sudo apt update
sudo apt upgrade -y

# Install build tools
sudo apt install -y build-essential gcc make

# Install required libraries
sudo apt install -y libsqlite3-dev libssl-dev

# Clone or copy project files
# Option A: Clone from git
git clone <your-repo-url> ~/networkFinal
cd ~/networkFinal

# Option B: Copy existing files (if already have them)
# Use Windows File Explorer to access \\wsl$ share
```

**4. Build the Application**

```bash
cd ~/networkFinal
make clean
make all

# Binaries will be in ./build/
ls -la build/
```

**5. Run the Application**

```bash
# Start server
./build/server 8080

# In another WSL terminal:
./build/client localhost 8080
```

---

### Option 2: MinGW/MSYS2

For native Windows compilation without WSL.

#### Prerequisites
- Download MSYS2 from https://www.msys2.org/
- Follow their installation instructions

#### Installation Steps

**1. Launch MSYS2 MinGW 64-bit**

Start menu → MSYS2 → MSYS2 MinGW 64-bit

**2. Install Dependencies**

```bash
pacman -Syu              # Update package database
pacman -S base-devel      # Build tools
pacman -S mingw-w64-x86_64-gcc
pacman -S mingw-w64-x86_64-make
pacman -S mingw-w64-x86_64-sqlite3
pacman -S mingw-w64-x86_64-openssl
```

**3. Navigate to Project**

```bash
cd /c/path/to/networkFinal
# Or use Windows drive letter syntax
```

**4. Build**

```bash
make clean
make all
```

**5. Run**

```bash
# On Windows, specify the full path or ensure ./build is in PATH
./build/server 8080
```

---

### Option 3: Cygwin

For compatibility with older Windows versions.

#### Installation Steps

**1. Download Cygwin Installer**

Visit https://cygwin.com/install.html and download `setup-x86_64.exe`

**2. Run Installer with Packages**

Run setup and select these packages during installation:
- `gcc-core`
- `make`
- `sqlite3`
- `libssl-devel`
- `pkg-config`

**3. Open Cygwin Terminal**

**4. Navigate and Build**

```bash
cd /cygdrive/c/path/to/networkFinal
make clean
make all
```

---

## Building the Application

### Prerequisites Check

Verify all dependencies are installed:

```bash
# macOS
brew list sqlite3 openssl

# WSL2/Linux
dpkg -l | grep sqlite3
dpkg -l | grep libssl

# MSYS2
pacman -Q mingw-w64-x86_64-sqlite3
```

### Build Targets

```bash
# Build all components (server, CLI client, GUI client)
make all

# Build server only
make server

# Build CLI client only
make client

# Build GUI client only
make gui

# Build tests
make tests

# Clean all build artifacts
make clean

# Rebuild everything from scratch
make clean && make all
```

### Build Output

```
networkFinal/build/
├── server           # Server executable (~115KB)
├── client           # CLI client executable (~90KB)
└── gui_client       # GTK GUI client executable (~115KB)
```

### Troubleshooting Build Issues

**Error: sqlite3.h not found**
```bash
# macOS
brew install sqlite3

# Linux/WSL2
sudo apt install libsqlite3-dev

# MSYS2
pacman -S mingw-w64-x86_64-sqlite3
```

**Error: openssl/crypto.h not found**
```bash
# macOS
brew install openssl

# Linux/WSL2
sudo apt install libssl-dev

# MSYS2
pacman -S mingw-w64-x86_64-openssl
```

**Error: gcc not found**
```bash
# macOS
xcode-select --install

# Linux/WSL2
sudo apt install build-essential

# MSYS2: Re-run installer and select gcc packages
```

---

## Running the System

### Single Computer Setup

**Terminal 1: Start Server**

```bash
cd /Users/minhbohung111/workspace/projects/networkFinal
./build/server 8080
```

Expected output:
```
[INFO] Server listening on port 8080...
[INFO] Database initialized
[INFO] Default admin user loaded
```

**Terminal 2: Start CLI Client**

```bash
cd /Users/minhbohung111/workspace/projects/networkFinal
./build/client localhost 8080
```

**Terminal 3: Start GUI Client (Optional)**

```bash
cd /Users/minhbohung111/workspace/projects/networkFinal
./build/gui_client
```

### Multi-Computer Setup

See [Network Configuration](#network-configuration) section below.

### Using Make Shortcuts

```bash
# Build and run server
make run-server

# Build and run CLI client (requires server running)
make run-client

# Build and run GUI client (requires server running)
make run-gui
```

---

## Network Configuration

### Two-Computer Setup (Local Network)

This guide assumes two computers on the same LAN (WiFi/Ethernet).

#### Step 1: Find Server Computer IP Address

**On macOS (Server Machine)**

```bash
# Method 1: Using ifconfig
ifconfig | grep "inet " | grep -v 127.0.0.1

# Output example:
# inet 192.168.101.140 netmask 0xffffff00 broadcast 192.168.101.255

# Method 2: System Preferences
# System Settings → Network → Status (shows IP like 192.168.101.140)
```

**On Windows (Server Machine using WSL2)**

```bash
# Inside WSL2, get the IP
ip addr show | grep "inet " | grep -v 127.0.0.1

# Or from Windows PowerShell
ipconfig
# Look for "IPv4 Address: 192.168.x.x"
```

**On Linux (Server Machine)**

```bash
hostname -I
# or
ip addr show | grep "inet " | grep -v 127.0.0.1
```

Common IP ranges:
- Home WiFi: `192.168.1.x`, `192.168.0.x`, `10.0.0.x`
- Example from context: `192.168.101.140`

#### Step 2: Verify Network Connectivity

**From Client Computer**

```bash
# Test ping to server
ping 192.168.101.140

# Output should show responses (press Ctrl+C to stop)
# If no response: check WiFi connection, firewall settings
```

#### Step 3: Start Server

**On Server Computer**

```bash
cd /Users/minhbohung111/workspace/projects/networkFinal
./build/server 8080
```

Keep this terminal window open.

#### Step 4: Connect Client

**On Client Computer**

```bash
# CLI Client
./build/client 192.168.101.140 8080

# GUI Client
./build/gui_client
# Then enter IP address: 192.168.101.140
# Port: 8080
```

### Network Troubleshooting

**"Connection refused"**
- Verify server is running on server machine
- Confirm port 8080 is not blocked by firewall
- Check IP address is correct (use ping)

**"No route to host"**
- Computers not on same network
- Check WiFi/Ethernet connection
- Verify IP address range matches (both on same subnet)

**Firewall Issues**

macOS:
```bash
# Check firewall status
sudo launchctl list | grep firewall

# Allow port 8080 through macOS firewall
# System Settings → Security & Privacy → Firewall Options
```

Windows:
```powershell
# Open Windows Defender Firewall with Advanced Security
# Inbound Rules → New Rule
# Select "Port" → TCP → Specific local ports: 8080
```

Linux/WSL2:
```bash
# Check UFW status
sudo ufw status

# Allow port 8080
sudo ufw allow 8080/tcp
```

---

## CLI Client Usage

### Starting the Client

```bash
# Local server
./build/client localhost 8080

# Remote server
./build/client 192.168.101.140 8080

# Custom port
./build/client 192.168.101.140 9000
```

### Login

After starting, you'll see:
```
Connected to server at 192.168.101.140:8080
Enter username: admin
Enter password: admin
```

**Default credentials:**
- Username: `admin`
- Password: `admin`

### Interactive Commands

Once logged in, type commands at the prompt:

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
  exit               - Disconnect and exit
  help               - Show this help

>
```

### Common Tasks

#### List Files

```
> ls
.
..
documents/
photos/
backup.zip

> ls documents/
file1.txt
file2.txt
report.pdf
```

#### Navigate Directories

```
> cd documents/
Current directory: /documents/

> cd ..
Current directory: /

> cd /
Current directory: /
```

#### Create Directory

```
> mkdir my_folder
Directory created: /my_folder

> cd my_folder
Current directory: /my_folder
```

#### Upload File

```
> upload /Users/username/local_file.txt remote_file.txt
Uploading: local_file.txt (1024 bytes)
Upload complete: 100%

> ls
remote_file.txt
```

#### Download File

```
> download remote_file.txt /Users/username/Downloaded_file.txt
Downloading: remote_file.txt (1024 bytes)
Download complete: 100%

> ls -la /Users/username/
-rw-r--r--  1 username  staff  1024 Dec 20 10:00 Downloaded_file.txt
```

#### Delete File

```
> delete remote_file.txt
File deleted: /remote_file.txt

> ls
(empty - file was deleted)
```

#### Change Permissions

```
> chmod myfile.txt 755
Permissions changed: /myfile.txt -> 755

> chmod myfile.txt 644
Permissions changed: /myfile.txt -> 644
```

#### Exit

```
> exit
Disconnecting...
Connection closed.
```

---

## GUI Client Usage

### Starting the GUI Client

```bash
./build/gui_client
```

### Initial Connection

1. **Server Address**: Enter `192.168.101.140` (or `localhost` for local)
2. **Port**: Enter `8080`
3. **Username**: Enter `admin`
4. **Password**: Enter `admin`
5. Click **Connect**

### GUI Interface

The GUI client provides a file manager-like interface:

```
┌─────────────────────────────────────┐
│ File Sharing System - GUI Client     │
├─────────────────────────────────────┤
│ Server: 192.168.101.140:8080 [●]    │
│                                     │
│ Current Path: /documents/           │
│ ┌─────────────────────────────────┐ │
│ │ Name          | Size  | Modified│ │
│ ├─────────────────────────────────┤ │
│ │ [📁] photos/  | --    | 20 Dec │ │
│ │ [📄] file.txt | 1.2KB | 19 Dec │ │
│ │ [📄] data.zip | 5.4MB | 18 Dec │ │
│ └─────────────────────────────────┘ │
│                                     │
│ [↑ Up] [🔄 Refresh] [➕ New Folder] │
│ [⬆ Upload] [⬇ Download] [🗑 Delete] │
└─────────────────────────────────────┘
```

### Navigation

- **Double-click** folders to open
- **Click "↑ Up"** to go to parent directory
- **Click "🔄 Refresh"** to reload current directory

### File Operations

#### Upload File

1. Click **"⬆ Upload"** button
2. Select file from your computer
3. Confirm destination name/path
4. Click **"Upload"**

#### Download File

1. Right-click on file
2. Select **"⬇ Download"** or use Download button
3. Choose save location on your computer
4. Click **"Save"**

#### Delete File

1. Select file
2. Click **"🗑 Delete"** button
3. Confirm deletion

#### Create Folder

1. Click **"➕ New Folder"** button
2. Enter folder name
3. Click **"Create"**

### Keyboard Shortcuts

- **Ctrl+U** - Upload
- **Ctrl+D** - Download
- **Delete** - Delete selected file
- **F5** - Refresh
- **Ctrl+Q** - Quit application

---

## Command Reference

### Server Commands

```bash
# Start server on default port 8080
./build/server 8080

# Start server on custom port
./build/server 9000

# Server options (if implemented)
./build/server --help
./build/server --version
```

### CLI Client Commands

#### Connection
```
connect [host] [port]    - Connect to server
disconnect               - Disconnect from server
status                   - Show connection status
```

#### Directory Operations
```
pwd                      - Print working directory
ls [path]               - List directory (default: current)
cd [path]               - Change directory
mkdir [name]            - Create directory
rmdir [path]            - Remove empty directory
```

#### File Operations
```
upload [src] [dst]      - Upload file from local to server
download [src] [dst]    - Download file from server to local
delete [path]           - Delete file
chmod [path] [mode]     - Change file permissions (octal: 755, 644, etc.)
cp [src] [dst]          - Copy file
mv [src] [dst]          - Move/rename file
cat [file]              - Display file contents
```

#### User & System
```
whoami                   - Show current user
pwd                      - Print working directory
clear                    - Clear screen
help                     - Show help
exit                     - Exit client
```

### Permission Modes

Common permission values (octal notation):

```
755  - rwxr-xr-x  (Read/write/execute for owner, read/execute for others)
644  - rw-r--r--  (Read/write for owner, read for others)
700  - rwx------  (Full permissions for owner only)
600  - rw-------  (Read/write for owner only)
777  - rwxrwxrwx  (Full permissions for everyone)
```

---

## Troubleshooting

### Server Issues

#### Server won't start: "Address already in use"

```bash
# Find process using port 8080
lsof -i :8080           # macOS/Linux
netstat -ano | findstr :8080  # Windows

# Kill the process
kill -9 <PID>           # macOS/Linux
taskkill /PID <PID> /F  # Windows

# Try different port
./build/server 9000
```

#### Server crashes on startup

```bash
# Check database corruption
ls -lah fileshare.db

# Backup and recreate database
mv fileshare.db fileshare.db.backup
./build/server 8080  # Will create new database
```

#### "Permission denied" running server

```bash
# Make executable
chmod +x ./build/server

# Run with proper path
./build/server 8080
```

### Client Issues

#### Client won't connect: "Connection refused"

1. Verify server is running:
   ```bash
   lsof -i :8080  # Should show server process
   ```

2. Check IP address:
   ```bash
   # From client computer
   ping 192.168.101.140
   ```

3. Check firewall (see Network Troubleshooting above)

#### "Authentication failed" error

- Verify username/password (default: `admin`/`admin`)
- Check server is running and accepting connections
- Try disconnecting and reconnecting

#### File upload/download hangs

- Check network connection
- Verify file is not being used by another application
- Try smaller file first
- Restart server and client

#### GUI client won't display

macOS:
```bash
# Ensure XQuartz is installed
brew install xquartz

# Restart computer
# Launch XQuartz first
# Then run GUI client
./build/gui_client
```

Linux/WSL2:
```bash
# Install GTK libraries
sudo apt install libgtk-3-0

# Check X11 display
echo $DISPLAY  # Should show something like :0 or :10
```

### File Transfer Issues

#### Upload fails with "File not found"

- Verify local file path is correct
- Use absolute path: `/Users/username/file.txt`
- Check file permissions (must be readable)

#### Download fails with "Permission denied"

- Verify file exists on server
- Check server-side permissions
- Ensure destination directory is writable

#### Large files timeout

- Try uploading smaller chunks
- Check network speed: `ping -c 10 <ip>`
- Restart server and client
- Check available disk space

### Database Issues

#### Database locked error

```bash
# Server may still be running
lsof | grep fileshare.db

# Kill any lingering processes
pkill -f "build/server"

# Remove lock files
rm -f fileshare.db-shm fileshare.db-wal
```

#### Database corruption

```bash
# Stop server first
pkill -f "build/server"

# Backup corrupted database
cp fileshare.db fileshare.db.corrupted

# Remove corrupted database
rm fileshare.db fileshare.db-shm fileshare.db-wal

# Start server to recreate database
./build/server 8080
```

---

## Security Notes

### Current Implementation

The current version uses plain-text password transmission. This is suitable for:
- **LOCAL networks only** (same LAN/office)
- **Testing and development**
- **Trusted networks without internet exposure**

### NOT Suitable For

- Internet-facing servers
- Untrusted networks
- Sensitive data
- Production environments

### Recommended Practices

1. **Network Isolation**
   - Keep server on internal network only
   - Don't expose port 8080 to internet
   - Use firewall to restrict access

2. **Local Network Only**
   - Use this only on private LANs
   - Don't run over WiFi to internet

3. **Credential Management**
   - Change default admin password after setup
   - Create individual user accounts
   - Don't share credentials

4. **File Permissions**
   - Use `chmod 700` for sensitive files
   - Set strict directory permissions
   - Regular backups of important files

5. **Monitoring**
   - Check server logs regularly
   - Review activity logs for suspicious access
   - Monitor file modifications

### Future Security Improvements

Planned for future versions:
- TLS/SSL encryption
- SHA-256 password hashing
- Session timeout
- Audit logging
- User role-based access control
- Two-factor authentication

---

## FAQ

### Q: Can I run both server and client on the same computer?

**A:** Yes! Use `localhost` as the address:
```bash
# Terminal 1
./build/server 8080

# Terminal 2
./build/client localhost 8080
```

### Q: What's the default password?

**A:** Username: `admin`, Password: `admin`

### Q: Can I change the port?

**A:** Yes, any unused port works:
```bash
# Server on custom port
./build/server 9000

# Client connects to custom port
./build/client 192.168.101.140 9000
```

### Q: How do I create new user accounts?

**A:** User management is through the database. Default admin account available. For now, only default admin credentials work.

### Q: What's the maximum file size?

**A:** Up to 16MB per file (protocol limit). Larger files will need to be split.

### Q: Can I use this over the internet?

**A:** Not recommended. Passwords are transmitted in plain text. Use only on local networks.

### Q: What if the server crashes?

**A:** The database persists. Restart the server:
```bash
./build/server 8080
```
All files and settings will be recovered from the database.

### Q: How do I backup my files?

**A:** Use the download command to pull files locally:
```
> download /path/to/file /local/backup/path
```

Or backup the entire database:
```bash
cp fileshare.db fileshare.db.backup
```

### Q: Why is my GUI client not showing icons?

**A:** This is a display issue with GTK themes. The functionality works even without icons. Try:
```bash
# macOS
defaults write com.apple.dt.Xcode IDESourceTreeEnumeratorShowHiddenExtensions -bool true
```

### Q: Can I run multiple servers?

**A:** Yes, on different ports:
```bash
# Terminal 1: Server on 8080
./build/server 8080

# Terminal 2: Server on 9000
./build/server 9000
```

Clients connect to whichever they choose.

### Q: How do I see server logs?

**A:** Server logs are printed to console:
```bash
./build/server 8080 | tee server.log
```

Or capture to file:
```bash
./build/server 8080 > server.log 2>&1 &
tail -f server.log
```

### Q: What if I forget the admin password?

**A:** Reset the database:
```bash
pkill -f "build/server"
rm fileshare.db fileshare.db-shm fileshare.db-wal
./build/server 8080  # Creates fresh database with default credentials
```

### Q: How do I uninstall the application?

**A:** Simply delete the directory:
```bash
rm -rf /Users/minhbohung111/workspace/projects/networkFinal
```

The application doesn't install anywhere else.

---

## Advanced Configuration

### Custom Server Configuration

To modify server behavior, edit the source code:

```bash
# Server main configuration
nano src/server/main.c

# Database settings
nano src/database/db_manager.c

# Protocol settings
nano src/common/protocol.h
```

Recompile with:
```bash
make clean && make all
```

### Environment Variables

None currently required. All configuration is in the code.

### Database Files

- **fileshare.db** - Main database
- **fileshare.db-shm** - Database memory mapping
- **fileshare.db-wal** - Write-ahead log

These are created automatically on first server start.

### Storage Directory

Files are stored in the `storage/` directory:
```bash
ls -la storage/
# Shows physical file storage
```

---

## Getting Help

### Useful Commands for Troubleshooting

```bash
# Check if server is running
ps aux | grep "build/server"

# Check port usage
lsof -i :8080

# Test network connectivity
ping 192.168.101.140

# Check process logs
dmesg | tail -20  # System logs
```

### Reporting Issues

Include the following when reporting problems:
1. Operating system and version
2. Complete error message
3. Steps to reproduce
4. Server and client logs
5. Output of `./build/server --version` (if available)

---

## Version History

- **v1.0** (Current)
  - Basic file upload/download
  - CLI and GUI clients
  - SQLite database persistence
  - Multi-platform support (macOS, Windows WSL, Linux)

---

## Project Structure Reference

```
networkFinal/
├── SETUP_GUIDE.md              ← This file
├── README.md                   ← Technical overview
├── Makefile                    ← Build system
├── build/                      ← Compiled binaries
│   ├── server
│   ├── client
│   └── gui_client
├── src/                        ← Source code
│   ├── server/                 ← Server implementation
│   ├── client/                 ← CLI and GUI clients
│   ├── common/                 ← Shared protocol code
│   └── database/               ← Database layer
├── docs/                       ← Technical documentation
│   ├── protocol_spec.md        ← Wire protocol details
│   └── api_reference.md        ← API documentation
├── tests/                      ← Test suite
├── storage/                    ← File storage
├── lib/                        ← Third-party libraries
│   └── cJSON/                  ← JSON parser
├── plans/                      ← Project planning
├── fileshare.db                ← Database file
└── GUI_TESTING.md              ← GUI testing notes
```

---

## Next Steps

1. **Complete Setup**: Follow the appropriate section for your OS
2. **Start Server**: `./build/server 8080`
3. **Connect Client**: CLI or GUI client
4. **Upload File**: Test with a small file first
5. **Explore Features**: Try all operations
6. **Set Up Network**: Follow network configuration for multi-computer use

---

## Support Resources

- **Technical Details**: See `docs/protocol_spec.md`
- **API Reference**: See `docs/api_reference.md`
- **Project Status**: See `docs/current_status.md`
- **Build Issues**: See Makefile in project root

---

## License

Educational project for network programming demonstration.

---

**Last Updated**: December 20, 2025
**Platform Support**: macOS 10.12+, Windows 10+ (WSL2, MinGW, Cygwin), Linux (Ubuntu, Debian, Fedora, etc.)
