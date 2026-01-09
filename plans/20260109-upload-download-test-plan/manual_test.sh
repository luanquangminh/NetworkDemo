#!/bin/bash

# Manual test for upload and download functionality

PROJECT_ROOT="/Users/minhbohung111/workspace/projects/networkFinal"
TEST_DIR="${PROJECT_ROOT}/plans/20260109-upload-download-test-plan/test_data"
RESULTS_DIR="${PROJECT_ROOT}/plans/20260109-upload-download-test-plan/results"

mkdir -p "${RESULTS_DIR}"

echo "=== Manual Upload/Download Test ==="
echo ""

# Test 1: Upload small file
echo "Test 1: Uploading small text file..."
echo "admin
admin
upload ${TEST_DIR}/upload/test_small.txt
ls
quit" | ${PROJECT_ROOT}/build/client localhost 8080 > "${RESULTS_DIR}/test1_upload.log" 2>&1

if [ $? -eq 0 ]; then
    echo "✓ Upload command completed"
    echo "Output:"
    tail -20 "${RESULTS_DIR}/test1_upload.log"
else
    echo "✗ Upload command failed"
    cat "${RESULTS_DIR}/test1_upload.log"
fi

echo ""
echo "=========================================="
echo ""

# Test 2: List files to get ID
echo "Test 2: Listing files to get uploaded file ID..."
echo "admin
admin
ls
quit" | ${PROJECT_ROOT}/build/client localhost 8080 > "${RESULTS_DIR}/test2_list.log" 2>&1

echo "Output:"
cat "${RESULTS_DIR}/test2_list.log"

echo ""
echo "=========================================="
echo ""

# Test 3: Download file (assuming ID 2 for first uploaded file after root)
echo "Test 3: Downloading file with ID 2..."
echo "admin
admin
download 2 ${TEST_DIR}/download/downloaded_small.txt
quit" | ${PROJECT_ROOT}/build/client localhost 8080 > "${RESULTS_DIR}/test3_download.log" 2>&1

if [ $? -eq 0 ]; then
    echo "✓ Download command completed"
    echo "Output:"
    tail -20 "${RESULTS_DIR}/test3_download.log"
else
    echo "✗ Download command failed"
    cat "${RESULTS_DIR}/test3_download.log"
fi

echo ""
echo "=========================================="
echo ""

# Test 4: Verify integrity
echo "Test 4: Verifying file integrity..."
if [ -f "${TEST_DIR}/download/downloaded_small.txt" ]; then
    echo "Downloaded file exists"

    ORIGINAL_SHA=$(shasum -a 256 "${TEST_DIR}/upload/test_small.txt" | awk '{print $1}')
    DOWNLOAD_SHA=$(shasum -a 256 "${TEST_DIR}/download/downloaded_small.txt" | awk '{print $1}')

    echo "Original SHA256:   ${ORIGINAL_SHA}"
    echo "Downloaded SHA256: ${DOWNLOAD_SHA}"

    if [ "${ORIGINAL_SHA}" = "${DOWNLOAD_SHA}" ]; then
        echo "✓ File integrity verified - checksums match!"
    else
        echo "✗ File integrity check failed - checksums do not match!"
    fi

    echo ""
    echo "File content comparison:"
    echo "--- Original ---"
    cat "${TEST_DIR}/upload/test_small.txt"
    echo ""
    echo "--- Downloaded ---"
    cat "${TEST_DIR}/download/downloaded_small.txt"
else
    echo "✗ Downloaded file not found"
fi

echo ""
echo "=========================================="
echo ""

# Test 5: Check database
echo "Test 5: Checking database entries..."
if [ -f "${PROJECT_ROOT}/data/fileshare.db" ]; then
    echo "Database query - All files:"
    sqlite3 "${PROJECT_ROOT}/data/fileshare.db" "SELECT id, name, size, is_directory, owner_id, created_at FROM files WHERE is_directory = 0 ORDER BY id;"
else
    echo "Database not found at ${PROJECT_ROOT}/data/fileshare.db"
fi

echo ""
echo "=========================================="
echo ""

# Test 6: Check storage
echo "Test 6: Checking storage directory..."
if [ -d "${PROJECT_ROOT}/storage" ]; then
    echo "Files in storage:"
    ls -lh "${PROJECT_ROOT}/storage"
else
    echo "Storage directory not found"
fi

echo ""
echo "=== Test Complete ==="
