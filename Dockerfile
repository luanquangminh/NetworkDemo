# ============================================================================
# Multi-Stage Dockerfile for File Sharing Server
# ============================================================================
# This Dockerfile creates an optimized container image for the C-based file
# sharing server with multi-architecture support (ARM64 and x86_64).
#
# Build stages:
#   1. builder: Compile the server from source
#   2. runtime: Minimal runtime environment with only necessary dependencies
# ============================================================================

# ============================================================================
# Stage 1: Builder - Compile the server
# ============================================================================
FROM --platform=$BUILDPLATFORM debian:bookworm-slim AS builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    gcc \
    make \
    libsqlite3-dev \
    libssl-dev \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /build

# Copy source files
COPY src/ ./src/
COPY lib/ ./lib/
COPY Makefile ./

# Build the server
RUN make server

# Verify the build
RUN test -f /build/build/server && \
    echo "✓ Server binary built successfully" || \
    (echo "✗ Server build failed" && exit 1)

# ============================================================================
# Stage 2: Runtime - Minimal production image
# ============================================================================
FROM debian:bookworm-slim AS runtime

# Install runtime dependencies only
RUN apt-get update && apt-get install -y \
    libsqlite3-0 \
    libssl3 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Create application directories
RUN mkdir -p /app/data /app/storage /app/logs

# Copy the compiled server binary from builder stage
COPY --from=builder /build/build/server /app/server

# Copy database schema
COPY src/database/db_init.sql /app/db_init.sql

# Copy entrypoint script
COPY docker-entrypoint.sh /app/docker-entrypoint.sh
RUN chmod +x /app/docker-entrypoint.sh

# Set working directory
WORKDIR /app

# Expose the server port
EXPOSE 3000

# Health check - verify server is responding
HEALTHCHECK --interval=30s --timeout=10s --start-period=5s --retries=3 \
    CMD timeout 5 sh -c 'echo "" | nc -z localhost 3000' || exit 1

# Environment variables with defaults
ENV SERVER_PORT=3000 \
    DB_PATH=/app/data/fileshare.db \
    STORAGE_PATH=/app/storage \
    LOG_LEVEL=INFO

# Use entrypoint script for initialization
ENTRYPOINT ["/app/docker-entrypoint.sh"]

# Default command (can be overridden)
CMD ["/app/server"]

# ============================================================================
# Build metadata
# ============================================================================
LABEL maintainer="File Sharing System"
LABEL description="C-based file sharing server with SQLite backend"
LABEL version="1.0.0"
