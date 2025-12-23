#!/bin/bash
# Automated Client Testing Script

set -e

PROJECT_DIR="/Users/minhbohung111/workspace/projects/networkFinal"
SERVER_BIN="$PROJECT_DIR/build/server"
CLIENT_BIN="$PROJECT_DIR/build/client"
SERVER_LOG="$PROJECT_DIR/server_test.log"
CLIENT_OUTPUT="$PROJECT_DIR/client_test_output.log"
TEST_FILE="$PROJECT_DIR/test_upload.txt"
DOWNLOAD_FILE="$PROJECT_DIR/test_download.txt"
SERVER_PID=""

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

log_info() { echo -e "${GREEN}[INFO]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; }
log_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
log_warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }

cleanup() {
    log_info "Cleaning up..."
    if [ ! -z "$SERVER_PID" ] && kill -0 $SERVER_PID 2>/dev/null; then
        kill $SERVER_PID 2>/dev/null || true
        wait $SERVER_PID 2>/dev/null || true
    fi
    rm -f $TEST_FILE $DOWNLOAD_FILE 2>/dev/null || true
}

trap cleanup EXIT INT TERM

# Check binaries
if [ ! -f "$SERVER_BIN" ] || [ ! -f "$CLIENT_BIN" ]; then
    log_error "Binaries not found. Please build first."
    exit 1
fi

log_info "=== Automated Client Test Suite ==="
echo ""

# Start server
log_info "Starting server..."
$SERVER_BIN 8080 > $SERVER_LOG 2>&1 &
SERVER_PID=$!
sleep 2

if ! kill -0 $SERVER_PID 2>/dev/null; then
    log_error "Server failed to start"
    cat $SERVER_LOG
    exit 1
fi
log_success "Server started (PID: $SERVER_PID)"

# Create test file
echo "This is a test file for upload testing." > $TEST_FILE
echo "Line 2: Testing file upload functionality." >> $TEST_FILE
log_info "Created test file: $TEST_FILE"

# Test 1: Connection test
log_info "Test 1: Testing connection..."
timeout 5 bash -c "echo -e 'quit' | $CLIENT_BIN localhost 8080" > $CLIENT_OUTPUT 2>&1 || true
if grep -q "Connected successfully" $CLIENT_OUTPUT; then
    log_success "Connection test PASSED"
else
    log_error "Connection test FAILED"
    cat $CLIENT_OUTPUT
fi
echo ""

# Test 2: Login test with valid credentials
log_info "Test 2: Testing login with valid credentials..."
timeout 5 bash -c "echo -e 'admin\nadmin\nquit' | $CLIENT_BIN localhost 8080" > $CLIENT_OUTPUT 2>&1 || true
if grep -q "Login successful" $CLIENT_OUTPUT; then
    log_success "Login test PASSED"
else
    log_error "Login test FAILED"
    cat $CLIENT_OUTPUT
fi
echo ""

# Test 3: Login test with invalid credentials
log_info "Test 3: Testing login with invalid credentials..."
timeout 5 bash -c "echo -e 'invaliduser\nwrongpassword\nquit' | $CLIENT_BIN localhost 8080" > $CLIENT_OUTPUT 2>&1 || true
if grep -q "Login failed" $CLIENT_OUTPUT; then
    log_success "Invalid login rejection test PASSED"
else
    log_error "Invalid login rejection test FAILED"
fi
echo ""

# Test 4: List directory command
log_info "Test 4: Testing ls command..."
timeout 10 bash -c "echo -e 'admin\nadmin\nls\nquit' | $CLIENT_BIN localhost 8080" > $CLIENT_OUTPUT 2>&1 || true
if grep -q "ID" $CLIENT_OUTPUT && grep -q "Type" $CLIENT_OUTPUT; then
    log_success "List directory test PASSED"
else
    log_error "List directory test FAILED"
    cat $CLIENT_OUTPUT
fi
echo ""

# Test 5: Create directory command
log_info "Test 5: Testing mkdir command..."
timeout 10 bash -c "echo -e 'admin\nadmin\nmkdir testdir\nls\nquit' | $CLIENT_BIN localhost 8080" > $CLIENT_OUTPUT 2>&1 || true
if grep -q "created successfully\|testdir" $CLIENT_OUTPUT; then
    log_success "Create directory test PASSED"
else
    log_error "Create directory test FAILED"
    cat $CLIENT_OUTPUT
fi
echo ""

# Test 6: Help command
log_info "Test 6: Testing help command..."
timeout 5 bash -c "echo -e 'admin\nadmin\nhelp\nquit' | $CLIENT_BIN localhost 8080" > $CLIENT_OUTPUT 2>&1 || true
if grep -q "Commands:" $CLIENT_OUTPUT; then
    log_success "Help command test PASSED"
else
    log_error "Help command test FAILED"
fi
echo ""

# Test 7: PWD command
log_info "Test 7: Testing pwd command..."
timeout 5 bash -c "echo -e 'admin\nadmin\npwd\nquit' | $CLIENT_BIN localhost 8080" > $CLIENT_OUTPUT 2>&1 || true
if grep -q "Current directory:" $CLIENT_OUTPUT; then
    log_success "PWD command test PASSED"
else
    log_error "PWD command test FAILED"
fi
echo ""

log_info "=== Test Summary ==="
log_success "Automated tests completed successfully!"
log_info "For manual testing, run: ./test_client.sh"
echo ""
log_info "Server log: $SERVER_LOG"
log_info "Client output: $CLIENT_OUTPUT"
