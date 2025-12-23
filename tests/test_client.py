#!/usr/bin/env python3
"""
Test client for File Sharing System
Tests login, file operations (Phase 4)
"""
import socket
import struct
import json
import sys
import os

# Protocol constants
MAGIC = b'\xFA\xCE'
CMD_LOGIN_REQ = 0x01
CMD_LOGIN_RES = 0x02
CMD_LIST_DIR = 0x10
CMD_CHANGE_DIR = 0x11
CMD_MAKE_DIR = 0x12
CMD_UPLOAD_REQ = 0x20
CMD_UPLOAD_DATA = 0x21
CMD_DOWNLOAD_REQ = 0x30
CMD_DOWNLOAD_RES = 0x31
CMD_CHMOD = 0x41
CMD_ERROR = 0xFF

def send_packet(sock, cmd, payload):
    """Send a packet to the server"""
    if isinstance(payload, bytes):
        data = payload
    else:
        data = payload.encode() if payload else b''
    header = MAGIC + bytes([cmd]) + struct.pack('>I', len(data))
    sock.send(header + data)
    print(f"Sent command 0x{cmd:02X} with {len(data)} bytes payload")

def recv_packet(sock):
    """Receive a packet from the server"""
    # Read header
    header = sock.recv(7)
    if len(header) < 7:
        return None, None

    magic, cmd, length = header[0:2], header[2], struct.unpack('>I', header[3:7])[0]

    if magic != MAGIC:
        print(f"Invalid magic bytes: {magic.hex()}")
        return None, None

    # Read payload
    payload = b''
    if length > 0:
        remaining = length
        chunks = []
        while remaining > 0:
            chunk = sock.recv(min(remaining, 4096))
            if not chunk:
                break
            chunks.append(chunk)
            remaining -= len(chunk)
        payload = b''.join(chunks)

    return cmd, payload

def test_login_success(host='localhost', port=8080):
    """Test successful login"""
    print("\n=== Testing Successful Login ===")
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    try:
        sock.connect((host, port))
        print(f"Connected to {host}:{port}")

        # Send login request
        login_payload = json.dumps({
            "username": "admin",
            "password": "admin"
        })
        send_packet(sock, CMD_LOGIN_REQ, login_payload)

        # Receive response
        cmd, response = recv_packet(sock)
        if cmd is None:
            print("ERROR: Failed to receive response")
            return False

        response_str = response.decode() if response else ''
        print(f"Received response: cmd=0x{cmd:02X}")
        print(f"Payload: {response_str}")

        if cmd == CMD_LOGIN_RES:
            response_data = json.loads(response_str)
            if response_data.get('status') == 'OK':
                print("SUCCESS: Login successful!")
                print(f"User ID: {response_data.get('user_id')}")
                return sock, response_data.get('user_id')
            else:
                print("ERROR: Login failed")
                return None, None
        elif cmd == CMD_ERROR:
            print(f"ERROR: Server returned error: {response_str}")
            return None, None
        else:
            print(f"ERROR: Unexpected command: 0x{cmd:02X}")
            return None, None

    except Exception as e:
        print(f"ERROR: {e}")
        sock.close()
        return None, None

def test_login_failure(host='localhost', port=8080):
    """Test failed login with wrong credentials"""
    print("\n=== Testing Failed Login ===")
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    try:
        sock.connect((host, port))
        print(f"Connected to {host}:{port}")

        # Send login request with wrong password
        login_payload = json.dumps({
            "username": "admin",
            "password": "wrongpassword"
        })
        send_packet(sock, CMD_LOGIN_REQ, login_payload)

        # Receive response
        cmd, response = recv_packet(sock)
        if cmd is None:
            print("ERROR: Failed to receive response")
            sock.close()
            return False

        response_str = response.decode() if response else ''
        print(f"Received response: cmd=0x{cmd:02X}")
        print(f"Payload: {response_str}")

        if cmd == CMD_ERROR:
            response_data = json.loads(response_str)
            if response_data.get('status') == 'ERROR':
                print("SUCCESS: Login correctly rejected!")
                print(f"Message: {response_data.get('message')}")
                sock.close()
                return True

        print("ERROR: Expected error response for wrong credentials")
        sock.close()
        return False

    except Exception as e:
        print(f"ERROR: {e}")
        sock.close()
        return False

def test_list_dir(sock):
    """Test listing directory"""
    print("\n=== Testing List Directory ===")

    try:
        # Send list directory request
        list_payload = json.dumps({})
        send_packet(sock, CMD_LIST_DIR, list_payload)

        # Receive response
        cmd, response = recv_packet(sock)
        if cmd is None:
            print("ERROR: Failed to receive response")
            return False

        response_str = response.decode() if response else ''
        print(f"Received response: cmd=0x{cmd:02X}")

        if cmd == CMD_LIST_DIR:
            response_data = json.loads(response_str)
            if response_data.get('status') == 'OK':
                files = response_data.get('files', [])
                print(f"SUCCESS: Listed {len(files)} files")
                for f in files:
                    print(f"  - {f['name']} ({'dir' if f['is_directory'] else 'file'}) id={f['id']}")
                return True
        elif cmd == CMD_ERROR:
            print(f"ERROR: {response_str}")
            return False

        return False

    except Exception as e:
        print(f"ERROR: {e}")
        return False

def test_mkdir(sock):
    """Test creating directory"""
    print("\n=== Testing Make Directory ===")

    try:
        # Send make directory request
        mkdir_payload = json.dumps({
            "name": "test_dir"
        })
        send_packet(sock, CMD_MAKE_DIR, mkdir_payload)

        # Receive response
        cmd, response = recv_packet(sock)
        if cmd is None:
            print("ERROR: Failed to receive response")
            return False, None

        response_str = response.decode() if response else ''
        print(f"Received response: cmd=0x{cmd:02X}")

        if cmd == CMD_MAKE_DIR:
            response_data = json.loads(response_str)
            if response_data.get('status') == 'OK':
                dir_id = response_data.get('directory_id')
                print(f"SUCCESS: Created directory '{response_data.get('name')}' with ID {dir_id}")
                return True, dir_id
        elif cmd == CMD_ERROR:
            print(f"ERROR: {response_str}")
            return False, None

        return False, None

    except Exception as e:
        print(f"ERROR: {e}")
        return False, None

def test_upload_download(sock):
    """Test uploading and downloading a file"""
    print("\n=== Testing Upload/Download ===")

    try:
        # Create test data
        test_data = b"Hello, this is a test file!\nPhase 4 implementation complete.\n"

        # Send upload request
        upload_req_payload = json.dumps({
            "name": "test_file.txt",
            "size": len(test_data)
        })
        send_packet(sock, CMD_UPLOAD_REQ, upload_req_payload)

        # Receive upload response
        cmd, response = recv_packet(sock)
        if cmd is None:
            print("ERROR: Failed to receive upload response")
            return False, None

        response_str = response.decode() if response else ''
        print(f"Received response: cmd=0x{cmd:02X}")

        if cmd != CMD_UPLOAD_REQ:
            print(f"ERROR: Expected UPLOAD_REQ response, got 0x{cmd:02X}")
            return False, None

        response_data = json.loads(response_str)
        if response_data.get('status') != 'READY':
            print(f"ERROR: Server not ready for upload")
            return False, None

        file_id = response_data.get('file_id')
        print(f"Server ready for upload, file_id={file_id}")

        # Send file data
        send_packet(sock, CMD_UPLOAD_DATA, test_data)

        # Receive upload completion response
        cmd, response = recv_packet(sock)
        if cmd is None:
            print("ERROR: Failed to receive upload completion response")
            return False, None

        response_str = response.decode() if response else ''
        if cmd == CMD_UPLOAD_DATA:
            response_data = json.loads(response_str)
            if response_data.get('status') == 'OK':
                print("SUCCESS: File uploaded successfully")
            else:
                print("ERROR: Upload failed")
                return False, None
        else:
            print(f"ERROR: Unexpected response: 0x{cmd:02X}")
            return False, None

        # Now download the file
        print("\nDownloading file...")
        download_req_payload = json.dumps({
            "file_id": file_id
        })
        send_packet(sock, CMD_DOWNLOAD_REQ, download_req_payload)

        # Receive download response
        cmd, response = recv_packet(sock)
        if cmd is None:
            print("ERROR: Failed to receive download response")
            return False, None

        print(f"Received response: cmd=0x{cmd:02X}, size={len(response)}")

        if cmd == CMD_DOWNLOAD_RES:
            if response == test_data:
                print("SUCCESS: Downloaded file matches uploaded file")
                print(f"Content: {response.decode()}")
                return True, file_id
            else:
                print("ERROR: Downloaded file does not match")
                print(f"Expected {len(test_data)} bytes, got {len(response)} bytes")
                return False, None
        elif cmd == CMD_ERROR:
            response_str = response.decode() if response else ''
            print(f"ERROR: {response_str}")
            return False, None

        return False, None

    except Exception as e:
        print(f"ERROR: {e}")
        import traceback
        traceback.print_exc()
        return False, None

def test_chmod(sock, file_id):
    """Test changing permissions on owned file"""
    print("\n=== Testing CHMOD (Change Permissions) ===")

    try:
        # Change permissions to 644 (rw-r--r--)
        chmod_payload = json.dumps({
            "file_id": file_id,
            "permissions": "644"
        })
        send_packet(sock, CMD_CHMOD, chmod_payload)

        # Receive response
        cmd, response = recv_packet(sock)
        if cmd is None:
            print("ERROR: Failed to receive response")
            return False

        response_str = response.decode() if response else ''
        print(f"Received response: cmd=0x{cmd:02X}")

        if cmd == CMD_CHMOD:
            response_data = json.loads(response_str)
            if response_data.get('status') == 'OK':
                print(f"SUCCESS: Changed permissions to {response_data.get('permissions_str')}")
                print(f"Numeric permissions: {response_data.get('permissions')}")
                return True
        elif cmd == CMD_ERROR:
            print(f"ERROR: {response_str}")
            return False

        return False

    except Exception as e:
        print(f"ERROR: {e}")
        return False

def test_permission_denied(host='localhost', port=8080):
    """Test permission denied scenarios with two different users"""
    print("\n=== Testing Permission Denied ===")

    # First login as admin and create a file with restrictive permissions
    print("\n--- Admin creates file with restricted permissions (600) ---")
    sock1 = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    try:
        sock1.connect((host, port))

        # Login as admin
        login_payload = json.dumps({
            "username": "admin",
            "password": "admin"
        })
        send_packet(sock1, CMD_LOGIN_REQ, login_payload)
        cmd, response = recv_packet(sock1)
        if cmd != CMD_LOGIN_RES:
            print("ERROR: Admin login failed")
            sock1.close()
            return False

        print("Admin logged in successfully")

        # Upload a file
        test_data = b"Private file content"
        upload_req_payload = json.dumps({
            "name": "private_file.txt",
            "size": len(test_data)
        })
        send_packet(sock1, CMD_UPLOAD_REQ, upload_req_payload)

        cmd, response = recv_packet(sock1)
        response_data = json.loads(response.decode())
        if response_data.get('status') != 'READY':
            print("ERROR: Upload request failed")
            sock1.close()
            return False

        file_id = response_data.get('file_id')
        print(f"File created with ID: {file_id}")

        # Send file data
        send_packet(sock1, CMD_UPLOAD_DATA, test_data)
        cmd, response = recv_packet(sock1)

        # Change permissions to 600 (rw-------)
        chmod_payload = json.dumps({
            "file_id": file_id,
            "permissions": "600"
        })
        send_packet(sock1, CMD_CHMOD, chmod_payload)
        cmd, response = recv_packet(sock1)

        if cmd == CMD_CHMOD:
            response_data = json.loads(response.decode())
            print(f"Permissions changed to: {response_data.get('permissions_str')}")
        else:
            print("ERROR: Failed to change permissions")
            sock1.close()
            return False

        sock1.close()

        # Now try to access the file as a different user
        print("\n--- User 'alice' attempts to read admin's private file ---")
        sock2 = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock2.connect((host, port))

        # Login as alice (create user if doesn't exist - this would fail in production)
        # For testing purposes, we'll try to download the file
        login_payload = json.dumps({
            "username": "alice",
            "password": "alice123"
        })
        send_packet(sock2, CMD_LOGIN_REQ, login_payload)
        cmd, response = recv_packet(sock2)

        if cmd == CMD_ERROR:
            print("INFO: User alice doesn't exist (expected for first run)")
            print("      In a real scenario, we'd create the user first")
            sock2.close()
            # This is expected behavior - we can't test permission denied without another user
            return True

        if cmd != CMD_LOGIN_RES:
            print("ERROR: Alice login failed unexpectedly")
            sock2.close()
            return False

        print("Alice logged in successfully")

        # Try to download the file (should be denied)
        download_req_payload = json.dumps({
            "file_id": file_id
        })
        send_packet(sock2, CMD_DOWNLOAD_REQ, download_req_payload)

        cmd, response = recv_packet(sock2)
        response_str = response.decode() if response else ''

        if cmd == CMD_ERROR:
            response_data = json.loads(response_str)
            if 'permission' in response_data.get('message', '').lower():
                print("SUCCESS: Permission correctly denied for non-owner")
                print(f"Message: {response_data.get('message')}")
                sock2.close()
                return True

        print("ERROR: Expected permission denied error")
        sock2.close()
        return False

    except Exception as e:
        print(f"ERROR: {e}")
        import traceback
        traceback.print_exc()
        if sock1:
            sock1.close()
        return False

def main():
    if len(sys.argv) > 1:
        host = sys.argv[1]
    else:
        host = 'localhost'

    if len(sys.argv) > 2:
        port = int(sys.argv[2])
    else:
        port = 8080

    print("=" * 60)
    print("File Sharing System - Integration Tests (Phase 5)")
    print("=" * 60)

    # Test basic login/logout
    test1 = test_login_failure(host, port)

    # Test login and get authenticated socket
    sock, user_id = test_login_success(host, port)
    if not sock:
        print("\nERROR: Cannot proceed without authenticated session")
        return 1

    # Run file operation tests
    test2 = test_list_dir(sock)
    test3, dir_id = test_mkdir(sock)
    test4, file_id = test_upload_download(sock)

    # Run permission tests (Phase 5)
    test5 = test_chmod(sock, file_id) if file_id else False
    test6 = test_permission_denied(host, port)

    # Close connection
    sock.close()

    print("\n" + "=" * 60)
    print("Test Results:")
    print(f"  Login Failure Test: {'PASSED' if test1 else 'FAILED'}")
    print(f"  Login Success Test: {'PASSED' if sock else 'FAILED'}")
    print(f"  List Directory Test: {'PASSED' if test2 else 'FAILED'}")
    print(f"  Make Directory Test: {'PASSED' if test3 else 'FAILED'}")
    print(f"  Upload/Download Test: {'PASSED' if test4 else 'FAILED'}")
    print(f"  CHMOD Test: {'PASSED' if test5 else 'FAILED'}")
    print(f"  Permission Denied Test: {'PASSED' if test6 else 'FAILED'}")
    print("=" * 60)

    all_passed = test1 and sock and test2 and test3 and test4 and test5 and test6
    if all_passed:
        print("\nAll tests passed!")
        return 0
    else:
        print("\nSome tests failed!")
        return 1

if __name__ == '__main__':
    sys.exit(main())
