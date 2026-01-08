#!/bin/bash

echo "════════════════════════════════════════════════════════"
echo "  M1 Mac Air - NetworkFileManager Troubleshooting"
echo "════════════════════════════════════════════════════════"
echo ""

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 1. Check macOS version
echo "1️⃣  Checking macOS version..."
MACOS_VERSION=$(sw_vers -productVersion)
echo "   Current: macOS $MACOS_VERSION"
if [[ $(echo "$MACOS_VERSION >= 10.13" | bc -l) -eq 1 ]]; then
    echo -e "   ${GREEN}✅ Compatible (need 10.13+)${NC}"
else
    echo -e "   ${RED}❌ Too old (need 10.13+)${NC}"
fi
echo ""

# 2. Check if app exists
echo "2️⃣  Checking app installation..."
APP_PATH="/Applications/NetworkFileManager.app"
if [ -d "$APP_PATH" ]; then
    echo -e "   ${GREEN}✅ App found${NC}"
else
    echo -e "   ${RED}❌ App not in Applications folder${NC}"
    echo "   Did you copy it from the DMG?"
    exit 1
fi
echo ""

# 3. Check architecture
echo "3️⃣  Checking app architecture..."
ARCH=$(file "$APP_PATH/Contents/MacOS/NetworkFileManager" 2>/dev/null | grep -o "arm64\|x86_64")
echo "   Architecture: $ARCH"
MACHINE=$(uname -m)
echo "   Your Mac: $MACHINE"
if [ "$ARCH" = "arm64" ] && [ "$MACHINE" = "arm64" ]; then
    echo -e "   ${GREEN}✅ Perfect match - native ARM64${NC}"
elif [ "$ARCH" = "x86_64" ] && [ "$MACHINE" = "arm64" ]; then
    echo -e "   ${YELLOW}⚠️  Intel binary on M1 - needs Rosetta 2${NC}"
else
    echo -e "   ${GREEN}✅ Compatible${NC}"
fi
echo ""

# 4. Check GTK4
echo "4️⃣  Checking GTK4 installation..."
if command -v pkg-config &> /dev/null; then
    if pkg-config --exists gtk4; then
        GTK_VERSION=$(pkg-config --modversion gtk4)
        echo -e "   ${GREEN}✅ GTK4 version $GTK_VERSION${NC}"
        GTK_PATH=$(pkg-config --variable=prefix gtk4)
        echo "   Location: $GTK_PATH"
    else
        echo -e "   ${RED}❌ GTK4 not installed${NC}"
        echo "   Install with: brew install gtk4"
        exit 1
    fi
else
    echo -e "   ${RED}❌ pkg-config not found${NC}"
    echo "   Install with: brew install pkg-config gtk4"
    exit 1
fi
echo ""

# 5. Check quarantine attributes
echo "5️⃣  Checking quarantine attributes..."
QUARANTINE=$(xattr -l "$APP_PATH" 2>/dev/null | grep "com.apple.quarantine")
if [ -n "$QUARANTINE" ]; then
    echo -e "   ${YELLOW}⚠️  App is quarantined${NC}"
    echo "   Removing quarantine..."
    sudo xattr -cr "$APP_PATH"
    echo -e "   ${GREEN}✅ Quarantine removed${NC}"
else
    echo -e "   ${GREEN}✅ No quarantine${NC}"
fi
echo ""

# 6. Check code signing
echo "6️⃣  Checking code signing..."
SIGNATURE=$(codesign -dv "$APP_PATH" 2>&1)
if echo "$SIGNATURE" | grep -q "code object is not signed"; then
    echo -e "   ${YELLOW}⚠️  App is not code-signed${NC}"
    echo "   This is OK - you'll need to use 'Right-click → Open'"
else
    echo -e "   ${GREEN}✅ App has signature${NC}"
fi
echo ""

# 7. Check permissions
echo "7️⃣  Checking file permissions..."
EXEC_PERM=$(ls -l "$APP_PATH/Contents/MacOS/NetworkFileManager" 2>/dev/null | cut -d' ' -f1)
if [[ $EXEC_PERM == *"x"* ]]; then
    echo -e "   ${GREEN}✅ Executable permissions OK${NC}"
else
    echo -e "   ${RED}❌ Not executable${NC}"
    echo "   Fixing permissions..."
    chmod +x "$APP_PATH/Contents/MacOS/NetworkFileManager"
    echo -e "   ${GREEN}✅ Fixed${NC}"
fi
echo ""

# 8. Test launch
echo "8️⃣  Testing app launch..."
echo "   Attempting to launch in 3 seconds..."
sleep 3
open "$APP_PATH"

echo ""
echo "════════════════════════════════════════════════════════"
echo "  📋 NEXT STEPS IF APP DOESN'T OPEN:"
echo "════════════════════════════════════════════════════════"
echo ""
echo "Option 1: Right-Click Method"
echo "  1. Go to Applications folder"
echo "  2. RIGHT-CLICK NetworkFileManager.app"
echo "  3. Select 'Open' from menu"
echo "  4. Click 'Open' in security dialog"
echo ""
echo "Option 2: System Settings"
echo "  1. Go to System Settings → Privacy & Security"
echo "  2. Scroll to 'Security' section"
echo "  3. Look for NetworkFileManager message"
echo "  4. Click 'Open Anyway'"
echo ""
echo "Option 3: Terminal Launch (see errors)"
echo "  Run this command:"
echo "  $APP_PATH/Contents/MacOS/NetworkFileManager"
echo ""
echo "════════════════════════════════════════════════════════"
