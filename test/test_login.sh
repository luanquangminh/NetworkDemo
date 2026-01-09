#!/bin/bash

# Kill existing server
pkill -9 server 2>/dev/null

# Start server and capture output
./build/server > /tmp/server_direct.log 2>&1 &
SERVER_PID=$!
echo "Started server with PID: $SERVER_PID"
sleep 2

# Test login
echo "Testing login..."
printf "1\nnewadmin\nadmin123\nq\n" | ./build/client localhost 8080

# Show server output
echo ""
echo "=== Server output ==="
cat /tmp/server_direct.log

# Clean up
kill $SERVER_PID 2>/dev/null
