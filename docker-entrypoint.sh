#!/bin/bash
# ============================================================================
# Docker Entrypoint Script for File Sharing Server
# ============================================================================
# This script initializes the server environment and database before starting
# the server process. It handles:
#   - Database creation and initialization
#   - Directory structure setup
#   - Environment validation
#   - Graceful shutdown handling
# ============================================================================

set -e

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Logging functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# ============================================================================
# Environment Setup
# ============================================================================

log_info "Starting File Sharing Server initialization..."

# Set defaults if not provided
export SERVER_PORT=${SERVER_PORT:-3000}
export DB_PATH=${DB_PATH:-/app/data/fileshare.db}
export STORAGE_PATH=${STORAGE_PATH:-/app/storage}
export LOG_LEVEL=${LOG_LEVEL:-INFO}

# ============================================================================
# Directory Setup
# ============================================================================

log_info "Setting up directory structure..."

# Create necessary directories
mkdir -p /app/data
mkdir -p /app/storage
mkdir -p /app/logs

# Set proper permissions
chmod 755 /app/data
chmod 755 /app/storage
chmod 755 /app/logs

log_success "Directory structure created"

# ============================================================================
# Database Initialization
# ============================================================================

log_info "Checking database at: $DB_PATH"

if [ ! -f "$DB_PATH" ]; then
    log_warn "Database not found, initializing..."

    # Check if init SQL exists
    if [ -f "/app/db_init.sql" ]; then
        log_info "Initializing database schema..."
        sqlite3 "$DB_PATH" < /app/db_init.sql
        log_success "Database initialized successfully"
    else
        log_error "Database initialization SQL not found!"
        exit 1
    fi
else
    log_success "Database exists, skipping initialization"
fi

# Verify database is accessible
if sqlite3 "$DB_PATH" "SELECT 1;" > /dev/null 2>&1; then
    log_success "Database is accessible"
else
    log_error "Database is not accessible"
    exit 1
fi

# ============================================================================
# Storage Setup
# ============================================================================

log_info "Checking storage directory: $STORAGE_PATH"

if [ ! -d "$STORAGE_PATH" ]; then
    log_warn "Storage directory not found, creating..."
    mkdir -p "$STORAGE_PATH"
    chmod 755 "$STORAGE_PATH"
    log_success "Storage directory created"
else
    log_success "Storage directory exists"
fi

# ============================================================================
# Environment Information
# ============================================================================

echo ""
echo "═══════════════════════════════════════════════════════════════"
echo "  File Sharing Server - Configuration"
echo "═══════════════════════════════════════════════════════════════"
echo ""
echo "  Server Port:      $SERVER_PORT"
echo "  Database:         $DB_PATH"
echo "  Storage:          $STORAGE_PATH"
echo "  Log Level:        $LOG_LEVEL"
echo ""
echo "═══════════════════════════════════════════════════════════════"
echo ""

# ============================================================================
# Signal Handling for Graceful Shutdown
# ============================================================================

# Function to handle shutdown signals
shutdown_handler() {
    log_warn "Received shutdown signal, stopping server gracefully..."

    # If server PID is known, send SIGTERM
    if [ ! -z "$SERVER_PID" ]; then
        kill -TERM "$SERVER_PID" 2>/dev/null || true
        wait "$SERVER_PID" 2>/dev/null || true
    fi

    log_success "Server stopped"
    exit 0
}

# Trap signals for graceful shutdown
trap shutdown_handler SIGTERM SIGINT SIGQUIT

# ============================================================================
# Start Server
# ============================================================================

log_info "Starting server on port $SERVER_PORT..."
echo ""

# Start the server with the configured port
# If a command is passed to the entrypoint, execute it
if [ "$1" = "/app/server" ]; then
    exec /app/server "$SERVER_PORT" &
    SERVER_PID=$!

    # Wait for server process
    wait "$SERVER_PID"
else
    # Execute custom command if provided
    exec "$@"
fi
