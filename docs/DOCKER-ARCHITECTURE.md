# Docker Architecture Documentation

Visual architecture documentation for the File Sharing Server Docker deployment.

## System Overview

```
┌─────────────────────────────────────────────────────────────────────┐
│                    FILE SHARING SYSTEM                              │
│                     Docker Deployment                               │
└─────────────────────────────────────────────────────────────────────┘

┌──────────────────────┐        ┌──────────────────────────────────┐
│   Developer/Admin    │        │       Docker Host Machine        │
│                      │        │                                  │
│  - Makefile Commands │───────▶│  ┌────────────────────────────┐  │
│  - docker-compose    │        │  │   Docker Engine/Daemon     │  │
│  - Configuration     │        │  └────────────────────────────┘  │
└──────────────────────┘        └──────────────────────────────────┘
                                              │
                                              ▼
                                ┌─────────────────────────────┐
                                │    Docker Container         │
                                │  fileshare-server:latest    │
                                │                             │
                                │  ┌───────────────────────┐  │
                                │  │  File Sharing Server  │  │
                                │  │  (C Binary)           │  │
                                │  │  Port: 8080           │  │
                                │  └───────────────────────┘  │
                                │                             │
                                │  Volumes (Host ← Container) │
                                │  ./data ← /app/data         │
                                │  ./storage ← /app/storage   │
                                │  ./logs ← /app/logs         │
                                └─────────────────────────────┘
                                              │
                                              │ Port 8080
                                              ▼
        ┌───────────────────────────────────────────────────────────┐
        │                     Network Layer                         │
        │            Bridge Network (172.28.0.0/16)                │
        └───────────────────────────────────────────────────────────┘
                                              │
                        ┌─────────────────────┼─────────────────────┐
                        ▼                     ▼                     ▼
              ┌──────────────┐      ┌──────────────┐      ┌──────────────┐
              │ GUI Client 1 │      │ GUI Client 2 │      │ CLI Client   │
              │ (Host)       │      │ (Remote Mac) │      │ (Host)       │
              │              │      │              │      │              │
              │ localhost    │      │ <server-ip>  │      │ localhost    │
              │ :8080        │      │ :8080        │      │ :8080        │
              └──────────────┘      └──────────────┘      └──────────────┘
```

## Multi-Stage Build Process

```
┌─────────────────────────────────────────────────────────────────────┐
│                     DOCKER BUILD STAGES                             │
└─────────────────────────────────────────────────────────────────────┘

STAGE 1: BUILDER
┌─────────────────────────────────────────────────────────────────────┐
│  Base: debian:bookworm-slim                                         │
│  ┌───────────────────────────────────────────────────────────────┐  │
│  │ Install Build Dependencies                                    │  │
│  │  - gcc                                                        │  │
│  │  - make                                                       │  │
│  │  - libsqlite3-dev                                            │  │
│  │  - libssl-dev                                                │  │
│  └───────────────────────────────────────────────────────────────┘  │
│                              ▼                                      │
│  ┌───────────────────────────────────────────────────────────────┐  │
│  │ Copy Source Code                                              │  │
│  │  - src/                                                       │  │
│  │  - lib/                                                       │  │
│  │  - Makefile                                                   │  │
│  └───────────────────────────────────────────────────────────────┘  │
│                              ▼                                      │
│  ┌───────────────────────────────────────────────────────────────┐  │
│  │ Compile Server                                                │  │
│  │  $ make server                                                │  │
│  │                                                               │  │
│  │  Output: /build/build/server                                  │  │
│  └───────────────────────────────────────────────────────────────┘  │
│                                                                     │
│  Size: ~500MB (includes build tools)                               │
└─────────────────────────────────────────────────────────────────────┘
                              ▼
                    (Copy Binary Only)
                              ▼
STAGE 2: RUNTIME
┌─────────────────────────────────────────────────────────────────────┐
│  Base: debian:bookworm-slim                                         │
│  ┌───────────────────────────────────────────────────────────────┐  │
│  │ Install Runtime Dependencies ONLY                             │  │
│  │  - libsqlite3-0 (runtime lib)                                │  │
│  │  - libssl3 (runtime lib)                                     │  │
│  │  - ca-certificates                                           │  │
│  └───────────────────────────────────────────────────────────────┘  │
│                              ▼                                      │
│  ┌───────────────────────────────────────────────────────────────┐  │
│  │ Copy From Builder                                             │  │
│  │  - /build/build/server → /app/server                          │  │
│  │  - db_init.sql → /app/db_init.sql                             │  │
│  │  - docker-entrypoint.sh → /app/docker-entrypoint.sh           │  │
│  └───────────────────────────────────────────────────────────────┘  │
│                              ▼                                      │
│  ┌───────────────────────────────────────────────────────────────┐  │
│  │ Configuration                                                 │  │
│  │  - EXPOSE 8080                                                │  │
│  │  - HEALTHCHECK script                                         │  │
│  │  - ENTRYPOINT /app/docker-entrypoint.sh                       │  │
│  │  - CMD ["/app/server"]                                        │  │
│  └───────────────────────────────────────────────────────────────┘  │
│                                                                     │
│  Size: ~150MB (minimal runtime)                                    │
└─────────────────────────────────────────────────────────────────────┘
                              ▼
                    Final Docker Image
                   fileshare-server:latest
```

## Container Runtime Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│                DOCKER CONTAINER: fileshare-server                   │
└─────────────────────────────────────────────────────────────────────┘

Container Filesystem:
┌─────────────────────────────────────────────────────────────────────┐
│  /app/                          (Application Root)                  │
│  ├── server                     (C binary - 50KB)                   │
│  ├── db_init.sql                (Database schema)                   │
│  ├── docker-entrypoint.sh       (Initialization script)             │
│  │                                                                   │
│  ├── data/                      (Volume Mount)                      │
│  │   └── fileshare.db           (SQLite database)                   │
│  │                                                                   │
│  ├── storage/                   (Volume Mount)                      │
│  │   ├── 06/                    (File storage by hash)             │
│  │   ├── 0e/                                                        │
│  │   └── ...                                                        │
│  │                                                                   │
│  └── logs/                      (Volume Mount)                      │
│      └── server.log             (Application logs)                  │
└─────────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────────┐
│  Host Filesystem:                                                   │
│  /Users/minhbohung111/workspace/projects/networkFinal/              │
│  ├── data/                      ← Mapped to /app/data/              │
│  ├── storage/                   ← Mapped to /app/storage/           │
│  └── logs/                      ← Mapped to /app/logs/              │
└─────────────────────────────────────────────────────────────────────┘

Running Processes Inside Container:
┌─────────────────────────────────────────────────────────────────────┐
│  PID 1: /app/docker-entrypoint.sh                                   │
│         │                                                            │
│         ├─ Initialize directories                                   │
│         ├─ Check/create database                                    │
│         ├─ Setup signal handlers                                    │
│         └─ Execute: /app/server 8080                                │
│                     │                                                │
│                     ├─ Socket Manager (listen on 0.0.0.0:8080)     │
│                     ├─ Thread Pool (worker threads)                 │
│                     ├─ Database Manager (SQLite connection)         │
│                     ├─ Storage Manager (file operations)            │
│                     └─ Command Handler (protocol processing)        │
└─────────────────────────────────────────────────────────────────────┘
```

## Network Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│                        NETWORK TOPOLOGY                             │
└─────────────────────────────────────────────────────────────────────┘

Docker Host Network (macOS/Linux):
┌─────────────────────────────────────────────────────────────────────┐
│  Host Interface: en0 (WiFi/Ethernet)                                │
│  Host IP: 192.168.1.100                                             │
│  │                                                                   │
│  ├── Port Binding: 0.0.0.0:8080 → Container:8080                   │
│  │                                                                   │
│  └── Docker Bridge Network: fileshare-network (172.28.0.0/16)      │
│      │                                                               │
│      └── Container IP: 172.28.0.2                                   │
│          └── fileshare-server                                       │
│              └── Listening: 0.0.0.0:8080                            │
└─────────────────────────────────────────────────────────────────────┘
                              │
        ┌─────────────────────┼─────────────────────┐
        │                     │                     │
        ▼                     ▼                     ▼
┌────────────────┐   ┌────────────────┐   ┌────────────────┐
│ localhost:8080 │   │ 192.168.1.100  │   │ 172.28.0.2     │
│ (same machine) │   │ :8080          │   │ :8080          │
│                │   │ (LAN clients)  │   │ (internal)     │
└────────────────┘   └────────────────┘   └────────────────┘

Connection Paths:
1. Local clients    → localhost:8080 → 172.28.0.2:8080
2. LAN clients      → 192.168.1.100:8080 → 172.28.0.2:8080
3. Health checks    → 127.0.0.1:8080 (inside container)
```

## Data Flow Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│                          DATA FLOW                                  │
└─────────────────────────────────────────────────────────────────────┘

1. CLIENT REQUEST FLOW:
   Client                Network              Container             Server
     │                     │                     │                   │
     │─── Connect ────────▶│                     │                   │
     │                     │─── Port 8080 ──────▶│                   │
     │                     │                     │─── Accept ───────▶│
     │                     │                     │                   │
     │                     │                     │◀── Thread Pool ───┤
     │                     │                     │                   │
     │─── LOGIN ──────────▶│────────────────────▶│──────────────────▶│
     │                     │                     │         │         │
     │                     │                     │         └────┐    │
     │                     │                     │              │    │
     │                     │                     │         DB Query  │
     │                     │                     │         (SQLite)  │
     │                     │                     │              │    │
     │◀── Response ────────│◀────────────────────│◀─────────────┘    │
     │                     │                     │                   │

2. FILE UPLOAD FLOW:
   Client                Network              Container             Storage
     │                     │                     │                   │
     │─── UPLOAD ─────────▶│────────────────────▶│                   │
     │     + File Data     │                     │                   │
     │                     │                     │─── Hash ─────┐    │
     │                     │                     │              │    │
     │                     │                     │◀─────────────┘    │
     │                     │                     │                   │
     │                     │                     │─── Write ────────▶│
     │                     │                     │     /app/storage/ │
     │                     │                     │                   │
     │                     │                     │◀─────────────────┤
     │                     │                     │                   │
     │                     │                     │─── DB Insert ─┐   │
     │                     │                     │   (metadata)  │   │
     │                     │                     │◀──────────────┘   │
     │                     │                     │                   │
     │◀── Success ─────────│◀────────────────────│                   │
     │                     │                     │                   │

3. PERSISTENT DATA FLOW:
   Container              Volume Mount          Host Filesystem
     │                     │                     │
     │─── Write DB ───────▶│────────────────────▶│
     │   /app/data/        │   (bind mount)      │ ./data/
     │   fileshare.db      │                     │ fileshare.db
     │                     │                     │
     │─── Write File ─────▶│────────────────────▶│
     │   /app/storage/     │                     │ ./storage/
     │                     │                     │
     │                     │                     │
     │   [Container Restart]                     │
     │                     │                     │
     │◀── Read DB ─────────│◀────────────────────│
     │   (Data Persists)   │                     │ (Files Persist)
     │                     │                     │
```

## Initialization Sequence

```
┌─────────────────────────────────────────────────────────────────────┐
│                    CONTAINER STARTUP SEQUENCE                       │
└─────────────────────────────────────────────────────────────────────┘

docker-compose up -d
         │
         ▼
┌────────────────────┐
│ 1. Docker Engine   │  - Pull image if not exists
│    Initialization  │  - Create container from image
└────────────────────┘
         │
         ▼
┌────────────────────┐
│ 2. Volume Mounts   │  - Mount ./data to /app/data
│    Binding         │  - Mount ./storage to /app/storage
│                    │  - Mount ./logs to /app/logs
└────────────────────┘
         │
         ▼
┌────────────────────┐
│ 3. Network Setup   │  - Create bridge network
│                    │  - Assign container IP
│                    │  - Bind port 8080
└────────────────────┘
         │
         ▼
┌────────────────────┐
│ 4. ENTRYPOINT      │  - Execute: /app/docker-entrypoint.sh
│    Execution       │
└────────────────────┘
         │
         ▼
┌────────────────────┐
│ 5. Environment     │  - Set SERVER_PORT=8080
│    Setup           │  - Set DB_PATH=/app/data/fileshare.db
│                    │  - Set STORAGE_PATH=/app/storage
└────────────────────┘
         │
         ▼
┌────────────────────┐
│ 6. Directory       │  - mkdir -p /app/data
│    Creation        │  - mkdir -p /app/storage
│                    │  - mkdir -p /app/logs
│                    │  - Set permissions (755)
└────────────────────┘
         │
         ▼
┌────────────────────┐
│ 7. Database        │  - Check if /app/data/fileshare.db exists
│    Initialization  │  - If not: sqlite3 < /app/db_init.sql
│                    │  - Create tables, indexes
│                    │  - Insert default admin user
└────────────────────┘
         │
         ▼
┌────────────────────┐
│ 8. Signal Handlers │  - Trap SIGTERM, SIGINT, SIGQUIT
│    Setup           │  - Enable graceful shutdown
└────────────────────┘
         │
         ▼
┌────────────────────┐
│ 9. Server Start    │  - Execute: /app/server 8080
│                    │  - Initialize socket manager
│                    │  - Create thread pool
│                    │  - Start listening on 0.0.0.0:8080
└────────────────────┘
         │
         ▼
┌────────────────────┐
│ 10. Health Check   │  - Wait 10 seconds (start_period)
│     Monitoring     │  - Begin health checks every 30s
│                    │  - Test: nc -z localhost 8080
└────────────────────┘
         │
         ▼
    [READY TO ACCEPT CONNECTIONS]
```

## Shutdown Sequence

```
┌─────────────────────────────────────────────────────────────────────┐
│                   GRACEFUL SHUTDOWN SEQUENCE                        │
└─────────────────────────────────────────────────────────────────────┘

docker-compose down / make docker-stop
         │
         ▼
┌────────────────────┐
│ 1. Docker Sends    │  - SIGTERM to container PID 1
│    SIGTERM         │    (docker-entrypoint.sh)
└────────────────────┘
         │
         ▼
┌────────────────────┐
│ 2. Entrypoint      │  - Trap catches SIGTERM
│    Trap Handler    │  - Execute shutdown_handler()
└────────────────────┘
         │
         ▼
┌────────────────────┐
│ 3. Server Signal   │  - Send SIGTERM to server process
│    Propagation     │  - Wait for server to exit
└────────────────────┘
         │
         ▼
┌────────────────────┐
│ 4. Server Cleanup  │  - Close active connections
│                    │  - Flush database writes
│                    │  - Close file handles
│                    │  - Release resources
└────────────────────┘
         │
         ▼
┌────────────────────┐
│ 5. Entrypoint Exit │  - Return exit code 0
│                    │  - Container stops
└────────────────────┘
         │
         ▼
┌────────────────────┐
│ 6. Volume Unmount  │  - Sync filesystem changes
│                    │  - Unmount volumes
│                    │  - Data persists on host
└────────────────────┘
         │
         ▼
┌────────────────────┐
│ 7. Network Cleanup │  - Remove container from network
│                    │  - Release IP address
│                    │  - Close port binding
└────────────────────┘
         │
         ▼
┌────────────────────┐
│ 8. Container       │  - Container status: Exited
│    Removal         │  - Remove container (if using --rm)
│                    │  - Keep volumes
└────────────────────┘
         │
         ▼
    [SHUTDOWN COMPLETE]
    [Data persists: ./data, ./storage, ./logs]
```

## Health Check Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│                      HEALTH CHECK SYSTEM                            │
└─────────────────────────────────────────────────────────────────────┘

Docker Engine Health Check Loop:
┌─────────────────────────────────────────────────────────────────────┐
│  Every 30 seconds:                                                  │
│  ┌────────────────────────────────────────────────────────────────┐ │
│  │ 1. Execute health check command in container                   │ │
│  │    $ timeout 5 sh -c 'echo "" | nc -z localhost 8080'         │ │
│  └────────────────────────────────────────────────────────────────┘ │
│                              │                                      │
│                    ┌─────────┴─────────┐                           │
│                    │                   │                           │
│                 Success             Failure                         │
│                    │                   │                           │
│                    ▼                   ▼                           │
│  ┌─────────────────────┐   ┌───────────────────────┐              │
│  │ Status: healthy     │   │ Retry Counter++       │              │
│  │ Reset retry counter │   │ (max 3 retries)       │              │
│  └─────────────────────┘   └───────────────────────┘              │
│                                        │                           │
│                              3 consecutive failures                │
│                                        │                           │
│                                        ▼                           │
│                          ┌───────────────────────┐                 │
│                          │ Status: unhealthy     │                 │
│                          │ Alert/take action     │                 │
│                          └───────────────────────┘                 │
└─────────────────────────────────────────────────────────────────────┘

Health States:
- starting   : First 10 seconds (start_period)
- healthy    : Health checks passing
- unhealthy  : 3 consecutive failures
```

---

## Component Interaction Diagram

```
┌─────────────────────────────────────────────────────────────────────┐
│                    COMPONENT INTERACTIONS                           │
└─────────────────────────────────────────────────────────────────────┘

Makefile ──┬──▶ Docker Build ─────▶ Image Registry (local)
           │                              │
           ├──▶ Docker Compose ───────────┼──▶ Container Creation
           │                              │
           └──▶ Volume Management ────────┼──▶ Data Persistence

Container ──┬──▶ Entrypoint Script ──┬──▶ Environment Setup
            │                         ├──▶ Database Init
            │                         └──▶ Server Launch
            │
            ├──▶ Server Process ────┬──▶ Socket Manager
            │                       ├──▶ Thread Pool
            │                       ├──▶ Command Handler
            │                       └──▶ Storage Manager
            │
            ├──▶ Database (SQLite) ─────▶ Volume: ./data
            │
            ├──▶ File Storage ───────────▶ Volume: ./storage
            │
            └──▶ Logs ───────────────────▶ Volume: ./logs

Clients ────┬──▶ Network (Port 8080) ──▶ Container
            │
            └──▶ Protocol Handler ───────▶ Server Process
```

---

**Architecture Version**: 1.0
**Last Updated**: 2026-01-09
**Platforms**: macOS (ARM64/x86_64), Linux
