#!/bin/bash

echo "═══════════════════════════════════════════════════════════════"
echo "  NETWORK FILE MANAGER - NEW MAC CLIENT SETUP"
echo "═══════════════════════════════════════════════════════════════"
echo ""
echo "This script will set up NetworkFileManager on a brand new Mac."
echo ""

# Check if Homebrew is installed
if ! command -v brew &> /dev/null; then
    echo "📦 Homebrew not found. Installing Homebrew..."
    echo ""
    /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

    # Add Homebrew to PATH for M1/M2 Macs
    if [[ $(uname -m) == 'arm64' ]]; then
        echo ""
        echo "Adding Homebrew to PATH (M1/M2 Mac detected)..."
        echo 'eval "$(/opt/homebrew/bin/brew shellenv)"' >> ~/.zprofile
        eval "$(/opt/homebrew/bin/brew shellenv)"
    fi
else
    echo "✅ Homebrew already installed"
fi

echo ""
echo "📥 Installing GTK4..."
brew install gtk4

echo ""
echo "🔍 Checking for NetworkFileManager.app..."
if [ -d "/Applications/NetworkFileManager.app" ]; then
    echo "✅ App found in Applications"
else
    echo "❌ App not found!"
    echo ""
    echo "Please:"
    echo "  1. Open NetworkFileManager.dmg"
    echo "  2. Drag NetworkFileManager.app to Applications"
    echo "  3. Run this script again"
    exit 1
fi

echo ""
echo "🔓 Removing security quarantine..."
sudo xattr -cr /Applications/NetworkFileManager.app

echo ""
echo "✅ Setup complete!"
echo ""
echo "═══════════════════════════════════════════════════════════════"
echo "  TO LAUNCH:"
echo "═══════════════════════════════════════════════════════════════"
echo ""
echo "Option 1: Finder"
echo "  • Open Applications"
echo "  • RIGHT-CLICK NetworkFileManager.app"
echo "  • Select 'Open'"
echo "  • Click 'Open' in dialog"
echo ""
echo "Option 2: Terminal"
echo "  open /Applications/NetworkFileManager.app"
echo ""
echo "═══════════════════════════════════════════════════════════════"
echo "  CONNECTION INFO:"
echo "═══════════════════════════════════════════════════════════════"
echo ""
echo "  Server IP:  192.168.1.71"
echo "  Port:       8080"
echo "  Username:   admin"
echo "  Password:   admin"
echo ""
echo "═══════════════════════════════════════════════════════════════"
