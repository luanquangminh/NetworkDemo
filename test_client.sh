#!/bin/bash
# Client Testing Script for File Sharing System

set -e

PROJECT_DIR="/Users/minhbohung111/workspace/projects/networkFinal"
SERVER_BIN="$PROJECT_DIR/build/server"
CLIENT_BIN="$PROJECT_DIR/build/client"
TEST_DB="$PROJECT_DIR/test_fileshare.db"
SERVER_LOG="$PROJECT_DIR/server_test.log"
SERVER_PID=""

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

# Cleanup function
cleanup() {
    log_info "Cleaning up..."
    if [ ! -z "$SERVER_PID" ] && kill -0 $SERVER_PID 2>/dev/null; then
        log_info "Stopping server (PID: $SERVER_PID)..."
        kill $SERVER_PID
        wait $SERVER_PID 2>/dev/null || true
    fi
    log_info "Cleanup complete"
}

# Set trap to cleanup on exit
trap cleanup EXIT INT TERM

# Check if binaries exist
if [ ! -f "$SERVER_BIN" ]; then
    log_error "Server binary not found. Please run 'make server' first."
    exit 1
fi

if [ ! -f "$CLIENT_BIN" ]; then
    log_error "Client binary not found. Please run 'make client' first."
    exit 1
fi

log_info "=== File Sharing System Client Test ==="
echo ""

# Start server
log_info "Starting server on port 8080..."
$SERVER_BIN 8080 > $SERVER_LOG 2>&1 &
SERVER_PID=$!
log_info "Server started with PID: $SERVER_PID"

# Wait for server to be ready
sleep 2

# Check if server is running
if ! kill -0 $SERVER_PID 2>/dev/null; then
    log_error "Server failed to start. Check $SERVER_LOG for details."
    cat $SERVER_LOG
    exit 1
fi

log_info "Server is running successfully"
echo ""

# Print instructions for manual testing
log_info "=== Manual Testing Instructions ==="
echo ""
echo "The server is now running on localhost:8080"
echo "You can test the client manually with:"
echo ""
echo "  $CLIENT_BIN localhost 8080"
echo ""
echo "Default test credentials:"
echo "  Username: admin"
echo "  Password: admin123"
echo ""
echo "  Username: testuser"
echo "  Password: test123"
echo ""
echo "Available commands in the client:"
echo "  ls                    - List current directory"
echo "  cd <id>              - Change to directory by ID"
echo "  mkdir <name>         - Create new directory"
echo "  upload <file>        - Upload local file"
echo "  download <id> <file> - Download file to local path"
echo "  chmod <id> <perm>    - Change permissions (e.g., 755)"
echo "  pwd                  - Print current directory"
echo "  help                 - Show help"
echo "  quit                 - Exit"
echo ""
log_warn "Press Ctrl+C to stop the server and exit"
echo ""

# Keep server running
log_info "Server is running. Logs: $SERVER_LOG"
wait $SERVER_PID
