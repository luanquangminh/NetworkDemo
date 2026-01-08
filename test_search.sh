#!/bin/bash

# Test script for file search functionality

echo "=== File Search Functionality Test ==="
echo ""

# Test 1: Connect and login
echo "Test 1: Login and setup test files"
cat << EOF | timeout 10 ./build/client localhost 8080
admin
admin
mkdir testdir1
mkdir testdir2
quit
EOF

sleep 1

# Test 2: Search for exact filename (non-recursive)
echo ""
echo "Test 2: Search for 'testdir1' (non-recursive)"
cat << EOF | timeout 10 ./build/client localhost 8080
admin
admin
search testdir1
quit
EOF

sleep 1

# Test 3: Search with wildcard pattern (non-recursive)
echo ""
echo "Test 3: Search with wildcard 'test*' (non-recursive)"
cat << EOF | timeout 10 ./build/client localhost 8080
admin
admin
search test*
quit
EOF

sleep 1

# Test 4: Search with wildcard pattern (recursive)
echo ""
echo "Test 4: Search with wildcard 'test*' (recursive)"
cat << EOF | timeout 10 ./build/client localhost 8080
admin
admin
search test* -r
quit
EOF

sleep 1

# Test 5: Search with ? wildcard
echo ""
echo "Test 5: Search with '?' wildcard 'testdir?'"
cat << EOF | timeout 10 ./build/client localhost 8080
admin
admin
search testdir?
quit
EOF

sleep 1

# Test 6: Search that returns no results
echo ""
echo "Test 6: Search for non-existent file"
cat << EOF | timeout 10 ./build/client localhost 8080
admin
admin
search nonexistent.txt
quit
EOF

echo ""
echo "=== All tests completed ==="
