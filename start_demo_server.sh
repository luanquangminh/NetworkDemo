#!/bin/bash

echo "═══════════════════════════════════════════════════════════════"
echo "  NETWORK FILE MANAGER - DEMO SERVER STARTUP"
echo "═══════════════════════════════════════════════════════════════"
echo ""

# Check if we're in the right directory
if [ ! -f "build/server" ]; then
    echo "❌ Error: build/server not found!"
    echo ""
    echo "Please run this script from the project root:"
    echo "  cd /path/to/networkFinal"
    echo "  ./start_demo_server.sh"
    exit 1
fi

# Find server IP
echo "🔍 Finding your server IP address..."
SERVER_IP=$(ifconfig | grep "inet " | grep -v 127.0.0.1 | awk '{print $2}' | head -1)

if [ -z "$SERVER_IP" ]; then
    echo "⚠️  Could not automatically detect IP"
    echo "   Please find it manually with: ifconfig"
    SERVER_IP="YOUR_IP_HERE"
else
    echo "   ✅ Found: $SERVER_IP"
fi

echo ""
echo "═══════════════════════════════════════════════════════════════"
echo "  📋 SHARE THIS WITH DEMO CLIENTS:"
echo "═══════════════════════════════════════════════════════════════"
echo ""
echo "  Server IP:  $SERVER_IP"
echo "  Port:       8080"
echo "  Username:   admin"
echo "  Password:   admin"
echo ""
echo "═══════════════════════════════════════════════════════════════"
echo ""

# Save connection info to file
cat > DEMO_CONNECTION_INFO.txt << EOF
═══════════════════════════════════════════════════════════════
  DEMO CONNECTION INFORMATION
═══════════════════════════════════════════════════════════════

Server IP:   $SERVER_IP
Port:        8080

Login:
Username:    admin
Password:    admin

═══════════════════════════════════════════════════════════════

Client Setup:
1. Launch NetworkFileManager app
2. Enter Server IP: $SERVER_IP
3. Enter Port: 8080
4. Click "Connect"
5. Login with admin/admin

═══════════════════════════════════════════════════════════════
EOF

echo "📄 Connection info saved to: DEMO_CONNECTION_INFO.txt"
echo ""

# Check for existing server
if lsof -Pi :8080 -sTCP:LISTEN -t >/dev/null 2>&1; then
    echo "⚠️  Warning: Something is already running on port 8080"
    echo ""
    echo "Options:"
    echo "  1. Kill existing process: lsof -ti:8080 | xargs kill"
    echo "  2. Use different port: ./build/server 8081"
    echo ""
    read -p "Kill existing process and continue? (y/n): " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        echo "Killing existing process..."
        lsof -ti:8080 | xargs kill 2>/dev/null
        sleep 2
    else
        echo "Exiting..."
        exit 1
    fi
fi

# Check database
if [ ! -f "fileshare.db" ]; then
    echo "ℹ️  No database found - will be created automatically"
else
    echo "✅ Database exists: fileshare.db"
fi

echo ""
echo "═══════════════════════════════════════════════════════════════"
echo "  🚀 STARTING SERVER..."
echo "═══════════════════════════════════════════════════════════════"
echo ""
echo "Press Ctrl+C to stop the server"
echo ""

# Start server
./build/server 8080
