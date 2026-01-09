#!/bin/bash

# =============================================================================
# File Upload/Download Testing Script
# =============================================================================
# Test suite for validating file upload and download functionality
# Tests: CLI upload, CLI download, file integrity, error handling
# =============================================================================

set -e  # Exit on error

# Color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
PROJECT_ROOT="/Users/minhbohung111/workspace/projects/networkFinal"
TEST_DIR="${PROJECT_ROOT}/plans/20260109-upload-download-test-plan/test_data"
RESULTS_DIR="${PROJECT_ROOT}/plans/20260109-upload-download-test-plan/results"
SERVER_PORT=8080
SERVER_HOST="localhost"
TEST_USER="admin"
TEST_PASS="admin123"

# Counters
TESTS_PASSED=0
TESTS_FAILED=0
TESTS_TOTAL=0

# Functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[PASS]${NC} $1"
    ((TESTS_PASSED++))
    ((TESTS_TOTAL++))
}

log_error() {
    echo -e "${RED}[FAIL]${NC} $1"
    ((TESTS_FAILED++))
    ((TESTS_TOTAL++))
}

log_warning() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

# Setup test environment
setup_test_env() {
    log_info "Setting up test environment..."

    mkdir -p "${TEST_DIR}/upload"
    mkdir -p "${TEST_DIR}/download"
    mkdir -p "${RESULTS_DIR}"

    # Create test files
    log_info "Creating test files..."

    # Small text file
    echo "Hello, World!" > "${TEST_DIR}/upload/test_small.txt"

    # Medium text file (1KB)
    dd if=/dev/urandom bs=1024 count=1 2>/dev/null | base64 > "${TEST_DIR}/upload/test_medium.txt"

    # Large file (10MB)
    dd if=/dev/urandom of="${TEST_DIR}/upload/test_large.bin" bs=1024 count=10240 2>/dev/null

    # Binary file (image simulation)
    dd if=/dev/urandom of="${TEST_DIR}/upload/test_image.png" bs=1024 count=512 2>/dev/null

    # Calculate checksums
    log_info "Calculating checksums..."
    shasum -a 256 "${TEST_DIR}/upload/test_small.txt" > "${TEST_DIR}/upload/test_small.txt.sha256"
    shasum -a 256 "${TEST_DIR}/upload/test_medium.txt" > "${TEST_DIR}/upload/test_medium.txt.sha256"
    shasum -a 256 "${TEST_DIR}/upload/test_large.bin" > "${TEST_DIR}/upload/test_large.bin.sha256"
    shasum -a 256 "${TEST_DIR}/upload/test_image.png" > "${TEST_DIR}/upload/test_image.png.sha256"

    log_success "Test environment setup complete"
}

# Check if server is running
check_server() {
    log_info "Checking if server is running on port ${SERVER_PORT}..."

    if lsof -Pi :${SERVER_PORT} -sTCP:LISTEN -t >/dev/null 2>&1; then
        log_success "Server is running on port ${SERVER_PORT}"
        return 0
    else
        log_error "Server is not running on port ${SERVER_PORT}"
        return 1
    fi
}

# Start server
start_server() {
    log_info "Starting server on port ${SERVER_PORT}..."

    cd "${PROJECT_ROOT}"
    "${PROJECT_ROOT}/build/server" ${SERVER_PORT} > "${RESULTS_DIR}/server.log" 2>&1 &
    SERVER_PID=$!
    echo ${SERVER_PID} > "${RESULTS_DIR}/server.pid"

    sleep 3

    if check_server; then
        log_success "Server started successfully (PID: ${SERVER_PID})"
        return 0
    else
        log_error "Failed to start server"
        return 1
    fi
}

# Stop server
stop_server() {
    log_info "Stopping server..."

    if [ -f "${RESULTS_DIR}/server.pid" ]; then
        SERVER_PID=$(cat "${RESULTS_DIR}/server.pid")
        kill ${SERVER_PID} 2>/dev/null || true
        rm -f "${RESULTS_DIR}/server.pid"
        log_success "Server stopped"
    fi
}

# Test CLI upload
test_cli_upload() {
    local test_file="$1"
    local test_name="$2"

    log_info "Testing CLI upload: ${test_name}"

    # Create test script for client
    cat > "${RESULTS_DIR}/upload_test.txt" << EOF
${TEST_USER}
${TEST_PASS}
upload ${test_file}
ls
quit
EOF

    # Run client
    if "${PROJECT_ROOT}/build/client" ${SERVER_HOST} ${SERVER_PORT} < "${RESULTS_DIR}/upload_test.txt" > "${RESULTS_DIR}/upload_${test_name}.log" 2>&1; then
        log_success "CLI upload succeeded: ${test_name}"
        return 0
    else
        log_error "CLI upload failed: ${test_name}"
        cat "${RESULTS_DIR}/upload_${test_name}.log"
        return 1
    fi
}

# Test CLI download
test_cli_download() {
    local file_id="$1"
    local download_path="$2"
    local test_name="$3"

    log_info "Testing CLI download: ${test_name}"

    # Create test script for client
    cat > "${RESULTS_DIR}/download_test.txt" << EOF
${TEST_USER}
${TEST_PASS}
download ${file_id} ${download_path}
quit
EOF

    # Run client
    if "${PROJECT_ROOT}/build/client" ${SERVER_HOST} ${SERVER_PORT} < "${RESULTS_DIR}/download_test.txt" > "${RESULTS_DIR}/download_${test_name}.log" 2>&1; then
        log_success "CLI download succeeded: ${test_name}"
        return 0
    else
        log_error "CLI download failed: ${test_name}"
        cat "${RESULTS_DIR}/download_${test_name}.log"
        return 1
    fi
}

# Verify file integrity
verify_integrity() {
    local original="$1"
    local downloaded="$2"
    local test_name="$3"

    log_info "Verifying file integrity: ${test_name}"

    if [ ! -f "${downloaded}" ]; then
        log_error "Downloaded file not found: ${test_name}"
        return 1
    fi

    # Compare checksums
    local original_sum=$(shasum -a 256 "${original}" | awk '{print $1}')
    local downloaded_sum=$(shasum -a 256 "${downloaded}" | awk '{print $1}')

    if [ "${original_sum}" = "${downloaded_sum}" ]; then
        log_success "File integrity verified: ${test_name}"
        echo "  Original:   ${original_sum}"
        echo "  Downloaded: ${downloaded_sum}"
        return 0
    else
        log_error "File integrity check failed: ${test_name}"
        echo "  Original:   ${original_sum}"
        echo "  Downloaded: ${downloaded_sum}"
        return 1
    fi
}

# Test error scenarios
test_error_scenarios() {
    log_info "Testing error scenarios..."

    # Test 1: Upload non-existent file
    log_info "Test: Upload non-existent file"
    cat > "${RESULTS_DIR}/error_test.txt" << EOF
${TEST_USER}
${TEST_PASS}
upload /nonexistent/file.txt
quit
EOF

    if "${PROJECT_ROOT}/build/client" ${SERVER_HOST} ${SERVER_PORT} < "${RESULTS_DIR}/error_test.txt" > "${RESULTS_DIR}/error_nonexistent.log" 2>&1; then
        if grep -q -i "error\|fail\|not found" "${RESULTS_DIR}/error_nonexistent.log"; then
            log_success "Error handling: Non-existent file"
        else
            log_error "Error handling: Non-existent file (no error detected)"
        fi
    fi

    # Test 2: Download non-existent file ID
    log_info "Test: Download non-existent file ID"
    cat > "${RESULTS_DIR}/error_test2.txt" << EOF
${TEST_USER}
${TEST_PASS}
download 99999 /tmp/test.txt
quit
EOF

    if "${PROJECT_ROOT}/build/client" ${SERVER_HOST} ${SERVER_PORT} < "${RESULTS_DIR}/error_test2.txt" > "${RESULTS_DIR}/error_notfound.log" 2>&1; then
        if grep -q -i "error\|fail\|not found" "${RESULTS_DIR}/error_notfound.log"; then
            log_success "Error handling: Non-existent file ID"
        else
            log_error "Error handling: Non-existent file ID (no error detected)"
        fi
    fi
}

# Check database entries
check_database() {
    log_info "Checking database entries..."

    if [ -f "${PROJECT_ROOT}/data/fileshare.db" ]; then
        log_info "Database file found: ${PROJECT_ROOT}/data/fileshare.db"

        # Query files table
        sqlite3 "${PROJECT_ROOT}/data/fileshare.db" "SELECT COUNT(*) FROM files WHERE is_directory = 0;" > "${RESULTS_DIR}/db_file_count.txt" 2>&1

        local file_count=$(cat "${RESULTS_DIR}/db_file_count.txt")
        log_info "Total files in database: ${file_count}"

        if [ "${file_count}" -gt 0 ]; then
            log_success "Database entries verified"
        else
            log_warning "No files found in database"
        fi
    else
        log_warning "Database file not found"
    fi
}

# Check storage directory
check_storage() {
    log_info "Checking storage directory..."

    if [ -d "${PROJECT_ROOT}/storage" ]; then
        local file_count=$(find "${PROJECT_ROOT}/storage" -type f | wc -l)
        log_info "Total files in storage: ${file_count}"

        if [ "${file_count}" -gt 0 ]; then
            log_success "Storage directory verified"
        else
            log_warning "No files found in storage"
        fi
    else
        log_warning "Storage directory not found"
    fi
}

# Generate report
generate_report() {
    log_info "Generating test report..."

    local report_file="${RESULTS_DIR}/test_report_$(date +%Y%m%d_%H%M%S).md"

    cat > "${report_file}" << EOF
# File Upload/Download Test Report

**Date:** $(date)
**Test Duration:** ${TEST_DURATION} seconds

## Test Summary

- **Total Tests:** ${TESTS_TOTAL}
- **Passed:** ${TESTS_PASSED}
- **Failed:** ${TESTS_FAILED}
- **Success Rate:** $(echo "scale=2; ${TESTS_PASSED} * 100 / ${TESTS_TOTAL}" | bc)%

## Test Environment

- **Server Port:** ${SERVER_PORT}
- **Server Host:** ${SERVER_HOST}
- **Test User:** ${TEST_USER}
- **Project Root:** ${PROJECT_ROOT}

## Test Files

EOF

    # Add test file details
    for file in "${TEST_DIR}/upload"/*; do
        if [ -f "${file}" ] && [[ ! "${file}" =~ \.sha256$ ]]; then
            local filename=$(basename "${file}")
            local filesize=$(stat -f%z "${file}" 2>/dev/null || stat -c%s "${file}" 2>/dev/null)
            local checksum=$(shasum -a 256 "${file}" | awk '{print $1}')

            cat >> "${report_file}" << EOF
### ${filename}
- **Size:** ${filesize} bytes
- **SHA256:** ${checksum}

EOF
        fi
    done

    cat >> "${report_file}" << EOF

## Test Results

### Upload Tests

EOF

    # Add upload test results
    for log_file in "${RESULTS_DIR}"/upload_*.log; do
        if [ -f "${log_file}" ]; then
            local test_name=$(basename "${log_file}" .log | sed 's/upload_//')
            cat >> "${report_file}" << EOF
#### ${test_name}
\`\`\`
$(tail -n 20 "${log_file}")
\`\`\`

EOF
        fi
    done

    cat >> "${report_file}" << EOF

### Download Tests

EOF

    # Add download test results
    for log_file in "${RESULTS_DIR}"/download_*.log; do
        if [ -f "${log_file}" ]; then
            local test_name=$(basename "${log_file}" .log | sed 's/download_//')
            cat >> "${report_file}" << EOF
#### ${test_name}
\`\`\`
$(tail -n 20 "${log_file}")
\`\`\`

EOF
        fi
    done

    cat >> "${report_file}" << EOF

## Database Verification

\`\`\`
$(cat "${RESULTS_DIR}/db_file_count.txt" 2>/dev/null || echo "N/A")
\`\`\`

## Storage Verification

\`\`\`
$(find "${PROJECT_ROOT}/storage" -type f 2>/dev/null | wc -l) files in storage directory
\`\`\`

## Server Logs

\`\`\`
$(tail -n 50 "${RESULTS_DIR}/server.log" 2>/dev/null || echo "N/A")
\`\`\`

## Conclusion

EOF

    if [ ${TESTS_FAILED} -eq 0 ]; then
        cat >> "${report_file}" << EOF
All tests passed successfully. Upload and download functionality is working as expected.
EOF
    else
        cat >> "${report_file}" << EOF
${TESTS_FAILED} test(s) failed. Please review the logs for details.
EOF
    fi

    log_success "Test report generated: ${report_file}"
    echo "${report_file}"
}

# Main execution
main() {
    local START_TIME=$(date +%s)

    echo "=========================================="
    echo "  File Upload/Download Test Suite"
    echo "=========================================="
    echo ""

    # Setup
    setup_test_env

    # Build project
    log_info "Building project..."
    cd "${PROJECT_ROOT}"
    if make clean && make all; then
        log_success "Build successful"
    else
        log_error "Build failed"
        exit 1
    fi

    # Start server if not running
    if ! check_server; then
        start_server || exit 1
    fi

    # Run tests
    echo ""
    echo "=========================================="
    echo "  Running Upload Tests"
    echo "=========================================="
    echo ""

    test_cli_upload "${TEST_DIR}/upload/test_small.txt" "small_file"
    test_cli_upload "${TEST_DIR}/upload/test_medium.txt" "medium_file"
    test_cli_upload "${TEST_DIR}/upload/test_large.bin" "large_file"
    test_cli_upload "${TEST_DIR}/upload/test_image.png" "binary_file"

    # Verify database and storage
    check_database
    check_storage

    echo ""
    echo "=========================================="
    echo "  Running Download Tests"
    echo "=========================================="
    echo ""

    # Note: File IDs need to be determined after upload
    # For now, we'll test with assumed IDs
    # In production, we'd query the database or parse upload responses

    test_cli_download "2" "${TEST_DIR}/download/test_small_downloaded.txt" "small_file"
    test_cli_download "3" "${TEST_DIR}/download/test_medium_downloaded.txt" "medium_file"

    echo ""
    echo "=========================================="
    echo "  Running Integrity Checks"
    echo "=========================================="
    echo ""

    # Verify file integrity
    verify_integrity "${TEST_DIR}/upload/test_small.txt" "${TEST_DIR}/download/test_small_downloaded.txt" "small_file"
    verify_integrity "${TEST_DIR}/upload/test_medium.txt" "${TEST_DIR}/download/test_medium_downloaded.txt" "medium_file"

    echo ""
    echo "=========================================="
    echo "  Running Error Scenario Tests"
    echo "=========================================="
    echo ""

    test_error_scenarios

    # Calculate duration
    local END_TIME=$(date +%s)
    TEST_DURATION=$((END_TIME - START_TIME))

    # Generate report
    echo ""
    echo "=========================================="
    echo "  Test Summary"
    echo "=========================================="
    echo ""
    echo "Total Tests:  ${TESTS_TOTAL}"
    echo "Passed:       ${GREEN}${TESTS_PASSED}${NC}"
    echo "Failed:       ${RED}${TESTS_FAILED}${NC}"
    echo "Duration:     ${TEST_DURATION} seconds"
    echo ""

    local report_file=$(generate_report)

    echo ""
    echo "Full report: ${report_file}"
    echo ""

    # Cleanup
    if [ "${KEEP_SERVER_RUNNING}" != "1" ]; then
        stop_server
    fi

    # Exit with status
    if [ ${TESTS_FAILED} -eq 0 ]; then
        exit 0
    else
        exit 1
    fi
}

# Run main
main "$@"
