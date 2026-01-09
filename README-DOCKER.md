# Docker Deployment Guide for File Sharing Server

This guide explains how to deploy the File Sharing Server using Docker, providing a consistent and isolated environment across different platforms.

## Table of Contents

- [Overview](#overview)
- [Prerequisites](#prerequisites)
- [Quick Start](#quick-start)
- [Architecture](#architecture)
- [Configuration](#configuration)
- [Usage](#usage)
- [Client Connection](#client-connection)
- [Troubleshooting](#troubleshooting)
- [Advanced Usage](#advanced-usage)

---

## Overview

The Docker deployment containerizes **only the server component** while clients (GUI and CLI) run on host machines. This approach provides:

- **Isolation**: Server runs in its own environment with specific dependencies
- **Portability**: Works consistently across macOS (Intel and Apple Silicon), Linux, and Windows
- **Persistence**: Database and file storage persist across container restarts
- **Easy Updates**: Rebuild and redeploy without affecting client installations
- **Multi-Architecture**: Supports both ARM64 (M1/M2 Macs) and x86_64 platforms

---

## Prerequisites

### Required Software

1. **Docker Desktop** (v20.10 or higher)
   - macOS: [Download Docker Desktop for Mac](https://www.docker.com/products/docker-desktop)
   - Linux: Install via package manager
   - Windows: [Download Docker Desktop for Windows](https://www.docker.com/products/docker-desktop)

2. **Docker Compose** (v2.0 or higher)
   - Included with Docker Desktop
   - Linux: Install separately if needed

### Verify Installation

```bash
docker --version
docker-compose --version
```

---

## Quick Start

### 1. Build and Run

From the project root directory:

```bash
# Build Docker image
make docker-build

# Start server container
make docker-run
```

The server will be accessible at `localhost:8080`.

### 2. Connect Clients

On host machines:

```bash
# Build GUI client (if not already built)
make gui

# Run GUI client
./build/gui_client
```

In the connection dialog:
- **Server IP**: `localhost` (or Docker host IP if connecting from another machine)
- **Port**: `8080`
- **Username**: `admin`
- **Password**: `admin`

### 3. Stop Server

```bash
make docker-stop
```

---

## Architecture

### Container Structure

```
┌─────────────────────────────────────────┐
│         Docker Container                │
│  ┌───────────────────────────────────┐  │
│  │   File Sharing Server (C binary)  │  │
│  │   - Port: 8080                    │  │
│  │   - SQLite3 Database              │  │
│  │   - OpenSSL Crypto                │  │
│  └───────────────────────────────────┘  │
│                                         │
│  Mounted Volumes:                       │
│  - /app/data     ← ./data  (DB)        │
│  - /app/storage  ← ./storage (files)   │
│  - /app/logs     ← ./logs  (logs)      │
└─────────────────────────────────────────┘
           ↕ Port 8080
┌─────────────────────────────────────────┐
│         Host Machine                    │
│  - GUI Client (build/gui_client)        │
│  - CLI Client (build/client)            │
│  - Connects to: localhost:8080          │
└─────────────────────────────────────────┘
```

### Multi-Stage Build

The Dockerfile uses a multi-stage build for optimization:

1. **Builder Stage**: Compiles the server with all build dependencies
2. **Runtime Stage**: Creates minimal image with only runtime dependencies

This reduces the final image size significantly.

---

## Configuration

### Environment Variables

Configure the server by editing `docker-compose.yml`:

```yaml
environment:
  - SERVER_PORT=8080          # Server listening port
  - DB_PATH=/app/data/fileshare.db  # Database location
  - STORAGE_PATH=/app/storage       # File storage path
  - LOG_LEVEL=INFO                  # Logging level
  - ADMIN_USERNAME=admin            # Default admin username
  - ADMIN_PASSWORD=admin            # Default admin password
```

### Port Configuration

To use a different port:

1. Edit `docker-compose.yml`:
   ```yaml
   ports:
     - "9090:8080"  # Host port:Container port
   ```

2. Restart container:
   ```bash
   make docker-restart
   ```

3. Connect clients to port 9090

### Volume Mounts

Persistent data locations:

- **Database**: `./data/fileshare.db`
- **File Storage**: `./storage/`
- **Logs**: `./logs/`

These directories are created automatically on first run.

---

## Usage

### Makefile Commands

| Command | Description |
|---------|-------------|
| `make docker-build` | Build Docker image from source |
| `make docker-run` | Start server container in background |
| `make docker-stop` | Stop and remove container |
| `make docker-restart` | Restart running container |
| `make docker-logs` | View real-time container logs |
| `make docker-status` | Show container status |
| `make docker-shell` | Open shell inside container |
| `make docker-clean` | Remove container and image |

### Docker Compose Commands

Alternatively, use `docker-compose` directly:

```bash
# Start services
docker-compose up -d

# View logs
docker-compose logs -f

# Stop services
docker-compose down

# Restart services
docker-compose restart

# View status
docker-compose ps
```

### Manual Docker Commands

```bash
# Build image
docker build -t fileshare-server:latest .

# Run container
docker run -d \
  --name fileshare-server \
  -p 8080:8080 \
  -v $(pwd)/data:/app/data \
  -v $(pwd)/storage:/app/storage \
  -v $(pwd)/logs:/app/logs \
  fileshare-server:latest

# Stop container
docker stop fileshare-server

# Remove container
docker rm fileshare-server
```

---

## Client Connection

### Connecting from Host Machine

When clients run on the same machine as Docker:

```
Server IP: localhost
Port: 8080
```

### Connecting from Another Machine on Network

1. **Find Docker host IP**:
   ```bash
   # macOS/Linux
   ifconfig | grep "inet " | grep -v 127.0.0.1

   # Or use the demo script
   ./start_demo_server.sh
   ```

2. **Connect clients**:
   ```
   Server IP: <docker-host-ip>
   Port: 8080
   ```

### Default Credentials

```
Username: admin
Password: admin
```

**⚠️ Security Note**: Change default credentials in production deployments.

---

## Troubleshooting

### Port Already in Use

**Problem**: Error when starting container - port 8080 already in use

**Solution**:
```bash
# Check what's using the port
lsof -i :8080

# Kill the process
lsof -ti:8080 | xargs kill

# Or use a different port in docker-compose.yml
```

### Container Won't Start

**Check logs**:
```bash
make docker-logs
```

**Common issues**:
- Database corruption: Remove `./data/fileshare.db` and restart
- Permission issues: Check volume mount permissions
- Build errors: Rebuild with `make docker-build`

### Database Issues

**Reset database**:
```bash
# Stop container
make docker-stop

# Remove database
rm -rf data/

# Restart (will create fresh database)
make docker-run
```

### Client Can't Connect

**Verify server is running**:
```bash
make docker-status

# Should show container as "Up"
```

**Test connectivity**:
```bash
# From host machine
nc -zv localhost 8080

# Should show "Connection succeeded"
```

**Check firewall**:
- Ensure port 8080 is not blocked
- Docker Desktop on macOS may need network permissions

### View Container Internals

**Open shell inside container**:
```bash
make docker-shell

# Inside container:
ls /app              # Check files
ps aux               # Check processes
sqlite3 /app/data/fileshare.db ".tables"  # Check database
```

### Build Failures

**Clean rebuild**:
```bash
# Remove old images and build artifacts
make docker-clean

# Clean local build
make clean

# Rebuild from scratch
make docker-build
```

---

## Advanced Usage

### Custom Build Arguments

Build for specific architecture:

```bash
# For ARM64 (M1/M2 Macs)
docker build --platform linux/arm64 -t fileshare-server:latest .

# For x86_64 (Intel)
docker build --platform linux/amd64 -t fileshare-server:latest .
```

### Resource Limits

Edit `docker-compose.yml` to adjust:

```yaml
deploy:
  resources:
    limits:
      cpus: '2'      # Maximum CPU cores
      memory: 1G     # Maximum memory
    reservations:
      cpus: '0.5'    # Minimum CPU cores
      memory: 256M   # Minimum memory
```

### Development Mode

For active development, mount source code:

```yaml
volumes:
  - ./src:/app/src:ro  # Read-only source mount
```

Rebuild and restart after code changes:
```bash
make docker-build && make docker-restart
```

### Production Deployment

For production:

1. **Change default credentials**:
   - Modify `src/database/db_init.sql` before building
   - Or change via admin dashboard after deployment

2. **Enable HTTPS** (if using reverse proxy):
   ```yaml
   environment:
     - ENABLE_SSL=true
     - SSL_CERT_PATH=/app/certs/cert.pem
     - SSL_KEY_PATH=/app/certs/key.pem
   volumes:
     - ./certs:/app/certs:ro
   ```

3. **Set up backups**:
   ```bash
   # Backup database and storage
   tar -czf backup-$(date +%Y%m%d).tar.gz data/ storage/
   ```

4. **Monitor logs**:
   ```bash
   # Set up log rotation
   docker-compose logs --tail=100 --follow > server.log &
   ```

### Integration with Demo Scripts

Update existing demo script to use Docker:

```bash
#!/bin/bash
# Start Docker server
make docker-run

# Wait for server to be ready
sleep 3

# Launch GUI client
./build/gui_client
```

---

## Best Practices

1. **Regular Backups**: Back up `./data/` and `./storage/` directories regularly
2. **Log Monitoring**: Check logs periodically with `make docker-logs`
3. **Updates**: Rebuild image after code changes
4. **Security**: Change default credentials immediately
5. **Network**: Use Docker networks for inter-container communication
6. **Volumes**: Never delete volume directories while container is running

---

## Support

For issues and questions:

1. Check logs: `make docker-logs`
2. Verify status: `make docker-status`
3. Review [PROJECT_DOCUMENTATION.md](PROJECT_DOCUMENTATION.md)
4. Check server logs in `./logs/` directory

---

## Additional Resources

- [Docker Documentation](https://docs.docker.com/)
- [Docker Compose Reference](https://docs.docker.com/compose/compose-file/)
- [Main Project README](README.md)
- [Setup Guide](SETUP_GUIDE.md)
