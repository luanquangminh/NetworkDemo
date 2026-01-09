# Docker Testing and Validation Guide

This guide provides comprehensive testing steps to validate the Docker deployment of the File Sharing Server.

## Pre-Testing Checklist

### 1. Verify Docker Installation

```bash
# Check Docker is installed and running
docker --version
docker-compose --version

# Test Docker is working
docker run hello-world
```

Expected output:
```
Docker version 20.10+ or higher
Docker Compose version 2.0+ or higher
Hello from Docker! (from test container)
```

### 2. Verify Files Exist

```bash
# From project root
ls -la Dockerfile docker-compose.yml docker-entrypoint.sh .dockerignore

# Verify entrypoint is executable
test -x docker-entrypoint.sh && echo "Executable: OK" || echo "Executable: FAILED"
```

Expected: All files present, entrypoint is executable

---

## Phase 1: Build Testing

### Test 1.1: Docker Image Build

```bash
# Clean build from scratch
make docker-clean
make docker-build
```

**Expected Results:**
- No build errors
- Image created successfully
- Output shows "Server binary built successfully"
- Image size should be reasonable (< 200MB for runtime)

**Verify:**
```bash
docker images | grep fileshare-server
```

Should show:
```
fileshare-server   latest   <image-id>   <timestamp>   <size>
```

### Test 1.2: Build Logs Review

Review build output for:
- All dependencies installed correctly
- Compilation succeeds without errors
- Multi-stage build completes both stages
- Final image contains server binary

### Test 1.3: Multi-Architecture Support

**For M1/M2 Macs (ARM64):**
```bash
docker build --platform linux/arm64 -t fileshare-server:arm64 .
```

**For Intel Macs/x86_64:**
```bash
docker build --platform linux/amd64 -t fileshare-server:amd64 .
```

**Expected:** Both builds succeed without architecture-specific errors

---

## Phase 2: Container Startup Testing

### Test 2.1: First-Time Startup

```bash
# Start container for the first time
make docker-run
```

**Expected Results:**
- Container starts successfully
- Database is initialized automatically
- Default admin user created
- Server listens on port 8080
- No error messages in logs

**Verify:**
```bash
make docker-status
```

Should show:
```
NAME               STATUS          PORTS
fileshare-server   Up X seconds    0.0.0.0:8080->8080/tcp
```

### Test 2.2: Check Initialization Logs

```bash
make docker-logs
```

**Expected Output:**
```
[INFO] Starting File Sharing Server initialization...
[INFO] Setting up directory structure...
[SUCCESS] Directory structure created
[INFO] Checking database at: /app/data/fileshare.db
[WARN] Database not found, initializing...
[INFO] Initializing database schema...
[SUCCESS] Database initialized successfully
[SUCCESS] Database is accessible
[SUCCESS] Storage directory exists
═══════════════════════════════════════════════════════════════
  File Sharing Server - Configuration
═══════════════════════════════════════════════════════════════
  Server Port:      8080
  Database:         /app/data/fileshare.db
  Storage:          /app/storage
  Log Level:        INFO
═══════════════════════════════════════════════════════════════
[INFO] Starting server on port 8080...
Server started on port 8080...
```

### Test 2.3: Verify Persistent Volumes

```bash
# Check volumes are created
ls -la data/ storage/ logs/

# Verify database exists
test -f data/fileshare.db && echo "Database: OK" || echo "Database: MISSING"

# Check database contents
sqlite3 data/fileshare.db "SELECT username, is_admin FROM users WHERE username='admin';"
```

**Expected:**
```
data/fileshare.db exists
admin|1
```

---

## Phase 3: Network and Connectivity Testing

### Test 3.1: Port Accessibility

```bash
# Test from host machine
nc -zv localhost 8080
```

**Expected:** `Connection to localhost port 8080 [tcp/*] succeeded!`

### Test 3.2: Server Response Test

```bash
# Create a simple test script
cat > test_docker_connection.sh << 'EOF'
#!/bin/bash
echo "Testing Docker server connectivity..."

# Try to connect
timeout 5 bash -c 'echo > /dev/tcp/localhost/8080' 2>/dev/null
if [ $? -eq 0 ]; then
    echo "✓ Server is accepting connections"
else
    echo "✗ Server is not responding"
    exit 1
fi
EOF

chmod +x test_docker_connection.sh
./test_docker_connection.sh
```

**Expected:** Server is accepting connections

### Test 3.3: Client Connection Test

```bash
# Build GUI client if not built
make gui

# Run client and connect to Docker server
./build/gui_client
```

**Connection Settings:**
- Server IP: `localhost`
- Port: `8080`
- Username: `admin`
- Password: `admin`

**Expected:** Client connects successfully, can login, and view files

---

## Phase 4: Data Persistence Testing

### Test 4.1: Database Persistence

```bash
# Connect to server and create a test user (via GUI or CLI)
# Then stop container
make docker-stop

# Restart container
make docker-run

# Verify data persists
sqlite3 data/fileshare.db "SELECT COUNT(*) FROM users;"
```

**Expected:** User count > 1 (includes test user)

### Test 4.2: File Storage Persistence

```bash
# Upload a file via client
# Create a test file
echo "Test file for Docker persistence" > test_file.txt

# Upload via GUI client
# Then stop and restart container
make docker-stop
make docker-run

# Verify file exists in storage
find storage/ -name "*test_file*"
```

**Expected:** Uploaded file persists after restart

### Test 4.3: Container Restart Without Data Loss

```bash
# Before restart - count files
FILE_COUNT_BEFORE=$(sqlite3 data/fileshare.db "SELECT COUNT(*) FROM files;")

# Restart container
make docker-restart

# After restart - count files
FILE_COUNT_AFTER=$(sqlite3 data/fileshare.db "SELECT COUNT(*) FROM files;")

# Compare
if [ "$FILE_COUNT_BEFORE" -eq "$FILE_COUNT_AFTER" ]; then
    echo "✓ Data persistence: OK"
else
    echo "✗ Data persistence: FAILED"
fi
```

**Expected:** File counts match

---

## Phase 5: Health Check Testing

### Test 5.1: Container Health Status

```bash
# Check health status
docker inspect fileshare-server | grep -A 10 "Health"
```

**Expected:** Status should be "healthy" after 30 seconds

### Test 5.2: Manual Health Check

```bash
# Run health check manually
docker exec fileshare-server timeout 5 sh -c 'echo "" | nc -z localhost 8080'
echo "Health check exit code: $?"
```

**Expected:** Exit code 0 (success)

---

## Phase 6: Resource Usage Testing

### Test 6.1: CPU and Memory Usage

```bash
# Monitor resource usage
docker stats fileshare-server --no-stream
```

**Expected Results:**
- CPU usage < 5% when idle
- Memory usage < 100MB when idle
- Memory usage < 1GB under load

### Test 6.2: Resource Limits

```bash
# Verify resource limits are applied
docker inspect fileshare-server | grep -A 5 "Memory"
```

**Expected:** Memory limit: 1GB, CPU limit: 2 cores

---

## Phase 7: Error Handling Testing

### Test 7.1: Database Corruption Recovery

```bash
# Stop container
make docker-stop

# Corrupt database
echo "corrupted" > data/fileshare.db

# Try to start (should fail gracefully)
make docker-run

# Check logs for error message
make docker-logs
```

**Expected:** Container starts but shows database error, doesn't crash

**Recovery:**
```bash
make docker-stop
rm data/fileshare.db
make docker-run
```

### Test 7.2: Port Conflict Handling

```bash
# Start container
make docker-run

# Try to start another instance
make docker-run
```

**Expected:** Error about port 8080 already in use

### Test 7.3: Graceful Shutdown

```bash
# Start container with active connections
make docker-run

# Connect client
./build/gui_client

# While connected, stop container
make docker-stop

# Check logs for graceful shutdown
make docker-logs | tail -20
```

**Expected:** "[WARN] Received shutdown signal, stopping server gracefully..."

---

## Phase 8: Multi-Client Testing

### Test 8.1: Multiple Concurrent Connections

```bash
# Start server
make docker-run

# Launch multiple clients
./build/gui_client &
./build/gui_client &
./build/gui_client &

# Monitor logs
make docker-logs
```

**Expected:** All clients connect successfully, server handles multiple connections

### Test 8.2: Cross-Network Connection

**On Server Machine:**
```bash
# Find IP address
ifconfig | grep "inet " | grep -v 127.0.0.1 | awk '{print $2}'

# Start server
make docker-run
```

**On Another Machine:**
```bash
# Connect client to server IP
./build/gui_client
# Enter server IP and port 8080
```

**Expected:** Client from different machine connects successfully

---

## Phase 9: Performance Testing

### Test 9.1: File Upload Performance

```bash
# Create test files of various sizes
dd if=/dev/zero of=test_1mb.bin bs=1M count=1
dd if=/dev/zero of=test_10mb.bin bs=1M count=10
dd if=/dev/zero of=test_100mb.bin bs=1M count=100

# Upload via client and measure time
time ./build/client localhost 8080 upload test_1mb.bin
time ./build/client localhost 8080 upload test_10mb.bin
time ./build/client localhost 8080 upload test_100mb.bin
```

**Expected:** Upload speeds reasonable for network conditions

### Test 9.2: Concurrent Operations

```bash
# Multiple simultaneous uploads
for i in {1..5}; do
    ./build/client localhost 8080 upload test_1mb.bin &
done
wait
```

**Expected:** All uploads complete successfully without errors

---

## Phase 10: Production Readiness Testing

### Test 10.1: Log Rotation and Management

```bash
# Generate logs
# Check log file sizes
du -h logs/*

# Verify logs are written
test -f logs/server.log && echo "Logging: OK" || echo "Logging: FAILED"
```

### Test 10.2: Backup and Restore

```bash
# Create backup
tar -czf backup-test.tar.gz data/ storage/

# Simulate disaster
make docker-stop
rm -rf data/ storage/

# Restore from backup
tar -xzf backup-test.tar.gz

# Restart
make docker-run

# Verify data restored
sqlite3 data/fileshare.db "SELECT COUNT(*) FROM users;"
```

**Expected:** Data restored successfully

### Test 10.3: Update/Upgrade Path

```bash
# Make code change in src/
# Rebuild
make docker-build

# Stop old container
make docker-stop

# Start new container (data should persist)
make docker-run

# Verify data intact
sqlite3 data/fileshare.db "SELECT COUNT(*) FROM users;"
```

**Expected:** Upgrade successful, data persists

---

## Testing Checklist Summary

Use this checklist to track testing progress:

- [ ] Docker installation verified
- [ ] Image builds successfully
- [ ] Multi-architecture support works
- [ ] Container starts on first run
- [ ] Database initializes correctly
- [ ] Persistent volumes created
- [ ] Server responds to connections
- [ ] Client connects successfully
- [ ] Data persists across restarts
- [ ] Health checks pass
- [ ] Resource usage within limits
- [ ] Error handling works correctly
- [ ] Multiple clients connect
- [ ] Cross-network connection works
- [ ] File operations perform well
- [ ] Backup and restore works
- [ ] Upgrade path tested

---

## Troubleshooting Commands

```bash
# View all logs
make docker-logs

# Check container status
make docker-status

# Inspect container details
docker inspect fileshare-server

# Enter container for debugging
make docker-shell

# Check database directly
docker exec fileshare-server sqlite3 /app/data/fileshare.db ".tables"

# Check network connectivity
docker exec fileshare-server netstat -tulpn

# View environment variables
docker exec fileshare-server env

# Check file permissions
docker exec fileshare-server ls -la /app/data /app/storage
```

---

## Success Criteria

The Docker deployment is considered successful when:

1. ✅ Image builds without errors
2. ✅ Container starts and initializes database
3. ✅ Server accepts client connections
4. ✅ Data persists across container restarts
5. ✅ Health checks pass consistently
6. ✅ Resource usage is reasonable
7. ✅ Multiple clients can connect simultaneously
8. ✅ Error handling is graceful
9. ✅ Backup and restore works
10. ✅ Documentation is clear and accurate

---

## Reporting Issues

If tests fail, gather this information:

```bash
# System info
uname -a
docker version
docker-compose version

# Container logs
make docker-logs > docker-error-logs.txt

# Container inspect
docker inspect fileshare-server > docker-inspect.txt

# Database status
sqlite3 data/fileshare.db ".schema" > database-schema.txt

# Create issue report
tar -czf docker-issue-report.tar.gz \
    docker-error-logs.txt \
    docker-inspect.txt \
    database-schema.txt \
    Dockerfile \
    docker-compose.yml
```

Include this archive when reporting issues.
