#!/usr/bin/env python3
"""
Test script to reproduce cross-user download issue
Tests if test2 can download files uploaded by test1
"""
import socket
import struct
import json
import sys

# Protocol constants
MAGIC = b'\xFA\xCE'
CMD_LOGIN_REQ = 0x01
CMD_LOGIN_RES = 0x02
CMD_LIST_DIR = 0x10
CMD_UPLOAD_REQ = 0x20
CMD_UPLOAD_DATA = 0x21
CMD_DOWNLOAD_REQ = 0x30
CMD_DOWNLOAD_RES = 0x31
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
    # Send upload request
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

    # Send file data
    send_packet(sock, CMD_UPLOAD_DATA, content)

    cmd, response = recv_packet(sock)
    if cmd == CMD_UPLOAD_DATA or cmd == CMD_SUCCESS:
        response_data = json.loads(response.decode())
        if response_data.get('status') == 'OK' or response_data.get('message'):
            print(f"✓ File uploaded successfully (file_id={file_id})")
            return file_id

    print(f"✗ Upload failed")
    return None

def download_file(sock, file_id):
    """Download a file"""
    download_req_payload = json.dumps({
        "file_id": file_id
    })
    send_packet(sock, CMD_DOWNLOAD_REQ, download_req_payload)

    # Receive metadata
    cmd, response = recv_packet(sock)

    if cmd == CMD_ERROR:
        response_data = json.loads(response.decode())
        print(f"✗ Download failed: {response_data.get('message')}")
        return None

    if cmd == CMD_DOWNLOAD_RES:
        metadata = json.loads(response.decode())
        print(f"  Metadata: {metadata}")

        # Receive file data
        cmd, data = recv_packet(sock)
        if cmd == CMD_SUCCESS:
            print(f"✓ Downloaded {len(data)} bytes")
            return data

    print(f"✗ Unexpected response: cmd=0x{cmd:02X}")
    return None

def list_directory(sock):
    """List directory contents"""
    list_payload = json.dumps({})
    send_packet(sock, CMD_LIST_DIR, list_payload)

    cmd, response = recv_packet(sock)
    if cmd == CMD_LIST_DIR:
        response_data = json.loads(response.decode())
        if response_data.get('status') == 'OK':
            files = response_data.get('files', [])
            print(f"✓ Directory listing ({len(files)} items):")
            for f in files:
                perms = oct(f['permissions'])
                print(f"  [{f['id']}] {f['name']} (perms={perms}, owner={f.get('owner', f['owner_id'])})")
            return files
    return []

def main():
    host = sys.argv[1] if len(sys.argv) > 1 else 'localhost'
    port = int(sys.argv[2]) if len(sys.argv) > 2 else 8080

    print("=" * 70)
    print("Cross-User Download Test")
    print("=" * 70)

    # Step 1: Login as test1 and upload file
    print("\n[Step 1] Login as test1 and upload file")
    sock1, user1_id = login(host, port, "test1", "123456")
    if not sock1:
        print("ERROR: test1 login failed. Ensure test1 user exists.")
        return 1

    test_data = b"This file was uploaded by test1 with 644 permissions"
    file_id = upload_file(sock1, "test1_file.txt", test_data)
    if not file_id:
        print("ERROR: Failed to upload file")
        sock1.close()
        return 1

    print(f"\n[Step 2] Check file permissions in directory listing")
    files = list_directory(sock1)

    sock1.close()

    # Step 2: Login as test2 and try to download test1's file
    print(f"\n[Step 3] Login as test2 and try to download file {file_id}")
    sock2, user2_id = login(host, port, "test2", "123456")
    if not sock2:
        print("ERROR: test2 login failed. Ensure test2 user exists.")
        return 1

    print(f"\n[Step 4] List directory as test2")
    files2 = list_directory(sock2)

    print(f"\n[Step 5] Download file {file_id} as test2")
    downloaded_data = download_file(sock2, file_id)

    sock2.close()

    # Results
    print("\n" + "=" * 70)
    print("Test Results:")
    print("=" * 70)
    if downloaded_data:
        if downloaded_data == test_data:
            print("✓ SUCCESS: test2 can download test1's file")
            print(f"  Content matches: {downloaded_data.decode()}")
            return 0
        else:
            print("✗ FAIL: Downloaded content doesn't match")
            return 1
    else:
        print("✗ FAIL: test2 cannot download test1's file")
        print("  This confirms the reported bug!")
        return 1

if __name__ == '__main__':
    sys.exit(main())
