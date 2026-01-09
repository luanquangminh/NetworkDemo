#!/usr/bin/env python3
"""
Test script to verify permission denial works correctly
Tests if test2 is blocked from downloading test1's private files (600 permissions)
"""
import socket
import struct
import json
import sys

# Protocol constants
MAGIC = b'\xFA\xCE'
CMD_LOGIN_REQ = 0x01
CMD_LOGIN_RES = 0x02
CMD_UPLOAD_REQ = 0x20
CMD_UPLOAD_DATA = 0x21
CMD_DOWNLOAD_REQ = 0x30
CMD_DOWNLOAD_RES = 0x31
CMD_CHMOD = 0x41
CMD_SUCCESS = 0xFE
CMD_ERROR = 0xFF

def send_packet(sock, cmd, payload):
    """Send a packet to the server"""
    if isinstance(payload, bytes):
        data = payload
    else:
        data = payload.encode() if payload else b''
    header = MAGIC + bytes([cmd]) + struct.pack('>I', len(data))
    sock.send(header + data)

def recv_packet(sock):
    """Receive a packet from the server"""
    header = sock.recv(7)
    if len(header) < 7:
        return None, None

    magic, cmd, length = header[0:2], header[2], struct.unpack('>I', header[3:7])[0]

    if magic != MAGIC:
        return None, None

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

def login(host, port, username, password):
    """Login and return socket"""
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((host, port))

    login_payload = json.dumps({
        "username": username,
        "password": password
    })
    send_packet(sock, CMD_LOGIN_REQ, login_payload)

    cmd, response = recv_packet(sock)
    if cmd == CMD_LOGIN_RES:
        response_data = json.loads(response.decode())
        if response_data.get('status') == 'OK':
            print(f"✓ {username} logged in successfully (user_id={response_data.get('user_id')})")
            return sock, response_data.get('user_id')

    print(f"✗ {username} login failed")
    sock.close()
    return None, None

def upload_file(sock, filename, content):
    """Upload a file and return file_id"""
    upload_req_payload = json.dumps({
        "name": filename,
        "size": len(content)
    })
    send_packet(sock, CMD_UPLOAD_REQ, upload_req_payload)

    cmd, response = recv_packet(sock)
    if cmd != CMD_UPLOAD_REQ and cmd != CMD_SUCCESS:
        print(f"✗ Upload request failed: cmd=0x{cmd:02X}")
        return None

    response_data = json.loads(response.decode())
    if response_data.get('status') != 'READY':
        print(f"✗ Server not ready: {response_data}")
        return None

    file_id = response_data.get('file_id')

    send_packet(sock, CMD_UPLOAD_DATA, content)

    cmd, response = recv_packet(sock)
    if cmd == CMD_UPLOAD_DATA or cmd == CMD_SUCCESS:
        response_data = json.loads(response.decode())
        if response_data.get('status') == 'OK' or response_data.get('message'):
            print(f"✓ File uploaded successfully (file_id={file_id})")
            return file_id

    print(f"✗ Upload failed")
    return None

def chmod_file(sock, file_id, permissions):
    """Change file permissions"""
    chmod_payload = json.dumps({
        "file_id": file_id,
        "permissions": permissions
    })
    send_packet(sock, CMD_CHMOD, chmod_payload)

    cmd, response = recv_packet(sock)
    if cmd == CMD_CHMOD or cmd == CMD_SUCCESS:
        response_data = json.loads(response.decode())
        if response_data.get('status') == 'OK':
            print(f"✓ Permissions changed to {permissions} ({response_data.get('permissions_str')})")
            return True

    print(f"✗ CHMOD failed")
    return False

def download_file(sock, file_id):
    """Download a file"""
    download_req_payload = json.dumps({
        "file_id": file_id
    })
    send_packet(sock, CMD_DOWNLOAD_REQ, download_req_payload)

    cmd, response = recv_packet(sock)

    if cmd == CMD_ERROR:
        response_data = json.loads(response.decode())
        return None, response_data.get('message')

    if cmd == CMD_DOWNLOAD_RES:
        metadata = json.loads(response.decode())
        cmd, data = recv_packet(sock)
        if cmd == CMD_SUCCESS:
            return data, None

    return None, f"Unexpected response: cmd=0x{cmd:02X}"

def main():
    host = sys.argv[1] if len(sys.argv) > 1 else 'localhost'
    port = int(sys.argv[2]) if len(sys.argv) > 2 else 8080

    print("=" * 70)
    print("Permission Denial Test")
    print("=" * 70)

    # Step 1: Login as test1 and upload private file
    print("\n[Step 1] Login as test1 and upload private file")
    sock1, user1_id = login(host, port, "test1", "123456")
    if not sock1:
        print("ERROR: test1 login failed")
        return 1

    test_data = b"This is a PRIVATE file with 600 permissions"
    file_id = upload_file(sock1, "private_file.txt", test_data)
    if not file_id:
        print("ERROR: Failed to upload file")
        sock1.close()
        return 1

    # Step 2: Change permissions to 600 (owner read/write only)
    print(f"\n[Step 2] Change file {file_id} permissions to 600 (owner-only)")
    if not chmod_file(sock1, file_id, "600"):
        print("ERROR: Failed to change permissions")
        sock1.close()
        return 1

    sock1.close()

    # Step 3: Login as test2 and try to download (should be denied)
    print(f"\n[Step 3] Login as test2 and try to download private file {file_id}")
    sock2, user2_id = login(host, port, "test2", "123456")
    if not sock2:
        print("ERROR: test2 login failed")
        return 1

    print(f"\n[Step 4] Attempt download (expecting permission denied)")
    data, error = download_file(sock2, file_id)

    sock2.close()

    # Results
    print("\n" + "=" * 70)
    print("Test Results:")
    print("=" * 70)
    if data:
        print("✗ FAIL: test2 was able to download test1's private file!")
        print("  This is a security bug - permission check not working")
        return 1
    elif error and "permission" in error.lower():
        print("✓ SUCCESS: Permission correctly denied")
        print(f"  Error message: {error}")
        return 0
    else:
        print(f"? UNCLEAR: Download failed but reason unclear: {error}")
        return 1

if __name__ == '__main__':
    sys.exit(main())
