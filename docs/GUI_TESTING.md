# GTK GUI Client Testing Guide

## Prerequisites

- GTK+ 3 installed (via Homebrew on macOS: `brew install gtk+3`)
- XQuartz or native display server for macOS
- Server must be running

## Building the GUI Client

```bash
make gui
```

This creates `build/gui_client` (approx 100KB binary)

## Running the GUI Client

### Option 1: Using Make
```bash
make run-gui
```

### Option 2: Direct Execution
```bash
./build/gui_client
```

## Testing Workflow

1. **Start the Server**
   ```bash
   make run-server
   # Or manually:
   ./build/server 8080
   ```

2. **Launch GUI Client**
   ```bash
   ./build/gui_client
   ```

3. **Login Dialog**
   - Default credentials are pre-filled:
     - Server: `localhost`
     - Port: `8080`
     - Username: `admin`
     - Password: `admin`
   - Click "Login" button

4. **Main Window Features**
   - **File Browser**: Double-click directories to navigate
   - **Toolbar Buttons**:
     - Upload: Choose local file to upload
     - Download: Select file and save locally
     - New Folder: Create directory in current location
     - Delete: Remove selected file/folder (not yet implemented)
     - Permissions: Change file permissions (chmod)
   - **Status Bar**: Shows current directory path

## Known Issues

- Delete functionality shows "not yet implemented" message
- File list refresh requires proper display server

## Implementation Details

### File List Display
- Uses `client_list_dir_gui()` function which returns cJSON data
- Populates GTK TreeView with columns: Icon, Name, Type, Size, Permissions
- Icons: "folder" for directories, "text-x-generic" for files

### Network Layer
- Reuses existing `client.c` and `net_handler.c` code
- Same protocol as CLI client
- All operations use the same backend functions

## Troubleshooting

### "Cannot open display"
- On macOS, ensure XQuartz is installed and running
- Set DISPLAY environment variable if needed

### Login fails
- Verify server is running on specified port
- Check credentials in database: `sqlite3 fileshare.db "SELECT username FROM users;"`

### File list empty
- Check server logs for errors
- Verify user has files in root directory
- Test with CLI client first to ensure backend works
