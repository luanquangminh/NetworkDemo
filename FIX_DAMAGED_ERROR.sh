#!/bin/bash

echo "════════════════════════════════════════════════════════════════"
echo "  FIX: 'NetworkFileManager is damaged and can't be opened'"
echo "════════════════════════════════════════════════════════════════"
echo ""

APP="/Applications/NetworkFileManager.app"

# Check if app exists
if [ ! -d "$APP" ]; then
    echo "❌ App not found at $APP"
    echo ""
    echo "Please:"
    echo "  1. Open the DMG file"
    echo "  2. Drag NetworkFileManager.app to /Applications"
    echo "  3. Run this script again"
    exit 1
fi

echo "✅ App found: $APP"
echo ""

# Method 1: Remove quarantine (most common fix)
echo "🔧 Method 1: Removing quarantine attributes..."
sudo xattr -cr "$APP"

# Check if successful
ATTRS=$(xattr "$APP" 2>/dev/null | grep -c "com.apple.quarantine")
if [ "$ATTRS" -eq 0 ]; then
    echo "   ✅ Quarantine removed"
else
    echo "   ⚠️  Still has quarantine, trying aggressive method..."
    sudo xattr -d com.apple.quarantine "$APP" 2>/dev/null
    sudo xattr -d com.apple.metadata:kMDItemWhereFroms "$APP" 2>/dev/null
fi

echo ""

# Method 2: Fix permissions
echo "🔧 Method 2: Fixing permissions..."
sudo chmod -R 755 "$APP"
sudo chmod +x "$APP/Contents/MacOS/NetworkFileManager"
echo "   ✅ Permissions fixed"
echo ""

# Method 3: Clear Launch Services
echo "🔧 Method 3: Clearing Launch Services cache..."
/System/Library/Frameworks/CoreServices.framework/Frameworks/LaunchServices.framework/Support/lsregister -kill -r -domain local -domain system -domain user
echo "   ✅ Cache cleared"
echo ""

# Method 4: Re-register app
echo "🔧 Method 4: Re-registering app with system..."
/System/Library/Frameworks/CoreServices.framework/Frameworks/LaunchServices.framework/Support/lsregister -v "$APP"
echo "   ✅ App re-registered"
echo ""

# Check GTK4
echo "🔧 Method 5: Checking GTK4..."
if command -v pkg-config &> /dev/null && pkg-config --exists gtk4; then
    GTK_VERSION=$(pkg-config --modversion gtk4)
    echo "   ✅ GTK4 $GTK_VERSION installed"
else
    echo "   ⚠️  GTK4 not found, installing..."
    brew install gtk4
fi

echo ""
echo "════════════════════════════════════════════════════════════════"
echo "  ✅ ALL FIXES APPLIED!"
echo "════════════════════════════════════════════════════════════════"
echo ""
echo "NOW TRY TO OPEN THE APP:"
echo ""
echo "Method A: Right-Click (Recommended)"
echo "  1. Open Finder → Applications"
echo "  2. RIGHT-CLICK NetworkFileManager.app"
echo "  3. Select 'Open'"
echo "  4. Click 'Open' in security dialog"
echo ""
echo "Method B: Command Line"
echo "  open $APP"
echo ""
echo "Method C: Direct Launch (shows errors)"
echo "  $APP/Contents/MacOS/NetworkFileManager"
echo ""
echo "════════════════════════════════════════════════════════════════"
echo ""
echo "If STILL showing 'damaged' error, try this nuclear option:"
echo ""
echo "  sudo spctl --master-disable"
echo "  open $APP"
echo "  sudo spctl --master-enable"
echo ""
echo "⚠️  WARNING: This temporarily disables Gatekeeper"
echo "    Remember to re-enable it after opening!"
echo ""
echo "════════════════════════════════════════════════════════════════"
