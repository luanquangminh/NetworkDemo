#!/usr/bin/env python3
"""
Comprehensive Download Functionality Test Suite
Tests all download scenarios for the Network File Manager system
"""
import socket
import struct
import json
import sys
import os
import hashlib
import time
from multiprocessing import Process, Queue

# Protocol constants
MAGIC = b'\xFA\xCE'
CMD_LOGIN_REQ = 0x01
CMD_LOGIN_RES = 0x02
CMD_LIST_DIR = 0x10
CMD_UPLOAD_REQ = 0x20
CMD_UPLOAD_DATA = 0x21
CMD_DOWNLOAD_REQ = 0x30
CMD_DOWNLOAD_RES = 0x31
CMD_CHMOD = 0x41
CMD_SUCCESS = 0xFE
CMD_ERROR = 0xFF

class TestResult:
    def __init__(self, scenario, status, message, details=None):
        self.scenario = scenario
        self.status = status  # "PASS", "FAIL", "ERROR"
        self.message = message
        self.details = details or {}
        self.timestamp = time.time()

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
            return sock, response_data.get('user_id')

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
        return None

    response_data = json.loads(response.decode())
    if response_data.get('status') != 'READY':
        return None

    file_id = response_data.get('file_id')

    send_packet(sock, CMD_UPLOAD_DATA, content)

    cmd, response = recv_packet(sock)
    if cmd == CMD_UPLOAD_DATA or cmd == CMD_SUCCESS:
        response_data = json.loads(response.decode())
        if response_data.get('status') == 'OK' or response_data.get('message'):
            return file_id

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
            return True

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

def scenario1_owner_downloads_own_file(host, port):
    """Scenario 1: Owner Downloads Own File"""
    print("\n" + "="*70)
    print("SCENARIO 1: Owner Downloads Own File")
    print("="*70)

    try:
        # Login as test1
        sock, user_id = login(host, port, "test1", "123456")
        if not sock:
            return TestResult("Scenario 1", "FAIL", "test1 login failed")

        # Upload file with unique name
        timestamp = int(time.time() * 1000)
        test_data = b"Test data for scenario 1 - owner download"
        file_id = upload_file(sock, f"scenario1_file_{timestamp}.txt", test_data)
        if not file_id:
            sock.close()
            return TestResult("Scenario 1", "FAIL", "File upload failed")

        print(f"  ✓ File uploaded (file_id={file_id})")

        # Download same file
        downloaded_data, error = download_file(sock, file_id)
        sock.close()

        if error:
            return TestResult("Scenario 1", "FAIL", f"Download failed: {error}")

        if downloaded_data == test_data:
            return TestResult("Scenario 1", "PASS", "Owner successfully downloaded own file", {
                "file_id": file_id,
                "size": len(downloaded_data),
                "integrity": "verified"
            })
        else:
            return TestResult("Scenario 1", "FAIL", "Downloaded content doesn't match", {
                "expected_size": len(test_data),
                "actual_size": len(downloaded_data)
            })

    except Exception as e:
        return TestResult("Scenario 1", "ERROR", f"Exception: {str(e)}")

def scenario2_cross_user_download(host, port):
    """Scenario 2: Cross-User Download (Standard Permissions)"""
    print("\n" + "="*70)
    print("SCENARIO 2: Cross-User Download (Standard Permissions)")
    print("="*70)

    try:
        # Login as test1 and upload
        sock1, user1_id = login(host, port, "test1", "123456")
        if not sock1:
            return TestResult("Scenario 2", "FAIL", "test1 login failed")

        timestamp = int(time.time() * 1000)
        test_data = b"Test data for scenario 2 - cross-user download with 644 permissions"
        file_id = upload_file(sock1, f"scenario2_file_{timestamp}.txt", test_data)
        if not file_id:
            sock1.close()
            return TestResult("Scenario 2", "FAIL", "File upload failed")

        print(f"  ✓ test1 uploaded file (file_id={file_id})")
        sock1.close()

        # Login as test2 and download
        sock2, user2_id = login(host, port, "test2", "123456")
        if not sock2:
            return TestResult("Scenario 2", "FAIL", "test2 login failed")

        downloaded_data, error = download_file(sock2, file_id)
        sock2.close()

        if error:
            return TestResult("Scenario 2", "FAIL", f"test2 download failed: {error}")

        if downloaded_data == test_data:
            return TestResult("Scenario 2", "PASS", "Cross-user download succeeded with 644 permissions", {
                "file_id": file_id,
                "uploader": "test1",
                "downloader": "test2",
                "size": len(downloaded_data),
                "integrity": "verified"
            })
        else:
            return TestResult("Scenario 2", "FAIL", "Downloaded content doesn't match")

    except Exception as e:
        return TestResult("Scenario 2", "ERROR", f"Exception: {str(e)}")

def scenario3_private_file_download_attempt(host, port):
    """Scenario 3: Private File Download Attempt"""
    print("\n" + "="*70)
    print("SCENARIO 3: Private File Download Attempt (600 permissions)")
    print("="*70)

    try:
        # Login as test1 and upload private file
        sock1, user1_id = login(host, port, "test1", "123456")
        if not sock1:
            return TestResult("Scenario 3", "FAIL", "test1 login failed")

        timestamp = int(time.time() * 1000)
        test_data = b"PRIVATE: This file should not be accessible by test2"
        file_id = upload_file(sock1, f"scenario3_private_{timestamp}.txt", test_data)
        if not file_id:
            sock1.close()
            return TestResult("Scenario 3", "FAIL", "File upload failed")

        # Change permissions to 600
        if not chmod_file(sock1, file_id, "600"):
            sock1.close()
            return TestResult("Scenario 3", "FAIL", "CHMOD to 600 failed")

        print(f"  ✓ test1 uploaded private file (file_id={file_id}, perms=600)")
        sock1.close()

        # Login as test2 and try to download (should fail)
        sock2, user2_id = login(host, port, "test2", "123456")
        if not sock2:
            return TestResult("Scenario 3", "FAIL", "test2 login failed")

        downloaded_data, error = download_file(sock2, file_id)
        sock2.close()

        if downloaded_data:
            return TestResult("Scenario 3", "FAIL", "Security bug: test2 accessed private file!", {
                "file_id": file_id,
                "permissions": "600",
                "security_issue": "permission check bypassed"
            })

        if error and "permission" in error.lower():
            return TestResult("Scenario 3", "PASS", "Permission correctly denied", {
                "file_id": file_id,
                "permissions": "600",
                "error_message": error
            })
        else:
            return TestResult("Scenario 3", "FAIL", f"Wrong error message: {error}")

    except Exception as e:
        return TestResult("Scenario 3", "ERROR", f"Exception: {str(e)}")

def scenario4_large_file_download(host, port):
    """Scenario 4: Large File Download"""
    print("\n" + "="*70)
    print("SCENARIO 4: Large File Download (150KB)")
    print("="*70)

    try:
        # Create 150KB test file
        large_data = os.urandom(150 * 1024)  # 150KB of random data
        expected_hash = hashlib.sha256(large_data).hexdigest()

        # Login and upload
        sock, user_id = login(host, port, "test1", "123456")
        if not sock:
            return TestResult("Scenario 4", "FAIL", "test1 login failed")

        timestamp = int(time.time() * 1000)
        start_upload = time.time()
        file_id = upload_file(sock, f"scenario4_large_{timestamp}.bin", large_data)
        upload_time = time.time() - start_upload

        if not file_id:
            sock.close()
            return TestResult("Scenario 4", "FAIL", "Large file upload failed")

        print(f"  ✓ Uploaded 150KB file in {upload_time:.2f}s (file_id={file_id})")

        # Download and verify
        start_download = time.time()
        downloaded_data, error = download_file(sock, file_id)
        download_time = time.time() - start_download
        sock.close()

        if error:
            return TestResult("Scenario 4", "FAIL", f"Download failed: {error}")

        downloaded_hash = hashlib.sha256(downloaded_data).hexdigest()

        if len(downloaded_data) != len(large_data):
            return TestResult("Scenario 4", "FAIL", "Size mismatch", {
                "expected_size": len(large_data),
                "actual_size": len(downloaded_data)
            })

        if downloaded_hash != expected_hash:
            return TestResult("Scenario 4", "FAIL", "Hash mismatch - data corruption", {
                "expected_hash": expected_hash,
                "actual_hash": downloaded_hash
            })

        return TestResult("Scenario 4", "PASS", "Large file downloaded successfully", {
            "file_id": file_id,
            "size": len(downloaded_data),
            "upload_time": f"{upload_time:.2f}s",
            "download_time": f"{download_time:.2f}s",
            "hash": expected_hash,
            "integrity": "verified"
        })

    except Exception as e:
        return TestResult("Scenario 4", "ERROR", f"Exception: {str(e)}")

def download_worker(host, port, username, password, file_id, queue):
    """Worker function for concurrent download testing"""
    try:
        sock, user_id = login(host, port, username, password)
        if not sock:
            queue.put({"user": username, "success": False, "error": "login failed"})
            return

        start_time = time.time()
        downloaded_data, error = download_file(sock, file_id)
        download_time = time.time() - start_time
        sock.close()

        if error:
            queue.put({"user": username, "success": False, "error": error})
        else:
            queue.put({
                "user": username,
                "success": True,
                "size": len(downloaded_data),
                "time": download_time,
                "hash": hashlib.sha256(downloaded_data).hexdigest()
            })

    except Exception as e:
        queue.put({"user": username, "success": False, "error": str(e)})

def scenario5_concurrent_downloads(host, port):
    """Scenario 5: Multiple Simultaneous Downloads"""
    print("\n" + "="*70)
    print("SCENARIO 5: Concurrent Downloads")
    print("="*70)

    try:
        # Login and upload test file
        sock, user_id = login(host, port, "test1", "123456")
        if not sock:
            return TestResult("Scenario 5", "FAIL", "test1 login failed")

        timestamp = int(time.time() * 1000)
        test_data = b"Concurrent download test data" * 1000  # ~30KB
        expected_hash = hashlib.sha256(test_data).hexdigest()

        file_id = upload_file(sock, f"scenario5_concurrent_{timestamp}.txt", test_data)
        sock.close()

        if not file_id:
            return TestResult("Scenario 5", "FAIL", "File upload failed")

        print(f"  ✓ Uploaded test file (file_id={file_id}, size={len(test_data)})")

        # Spawn concurrent download processes
        queue = Queue()
        p1 = Process(target=download_worker, args=(host, port, "test1", "123456", file_id, queue))
        p2 = Process(target=download_worker, args=(host, port, "test2", "123456", file_id, queue))

        print("  ⏳ Starting concurrent downloads...")
        start_time = time.time()
        p1.start()
        p2.start()

        p1.join(timeout=10)
        p2.join(timeout=10)
        total_time = time.time() - start_time

        # Check if processes are still alive (timeout)
        if p1.is_alive() or p2.is_alive():
            p1.terminate()
            p2.terminate()
            return TestResult("Scenario 5", "FAIL", "Concurrent download timeout")

        # Collect results
        results = []
        while not queue.empty():
            results.append(queue.get())

        if len(results) != 2:
            return TestResult("Scenario 5", "FAIL", f"Expected 2 results, got {len(results)}")

        # Verify both downloads succeeded
        failures = [r for r in results if not r.get("success")]
        if failures:
            return TestResult("Scenario 5", "FAIL", "Some downloads failed", {
                "failures": failures
            })

        # Verify data integrity
        for r in results:
            if r.get("hash") != expected_hash:
                return TestResult("Scenario 5", "FAIL", "Hash mismatch - concurrent access corruption", {
                    "expected_hash": expected_hash,
                    "results": results
                })

        return TestResult("Scenario 5", "PASS", "Concurrent downloads succeeded", {
            "file_id": file_id,
            "concurrent_users": 2,
            "total_time": f"{total_time:.2f}s",
            "results": results,
            "integrity": "verified"
        })

    except Exception as e:
        return TestResult("Scenario 5", "ERROR", f"Exception: {str(e)}")

def print_test_result(result):
    """Print formatted test result"""
    status_symbol = {
        "PASS": "✓",
        "FAIL": "✗",
        "ERROR": "⚠"
    }
    symbol = status_symbol.get(result.status, "?")
    print(f"\n{symbol} {result.scenario}: {result.status}")
    print(f"  Message: {result.message}")
    if result.details:
        print(f"  Details: {json.dumps(result.details, indent=4)}")

def generate_report(results, output_file):
    """Generate comprehensive test report"""
    report = []
    report.append("=" * 80)
    report.append("COMPREHENSIVE DOWNLOAD FUNCTIONALITY TEST REPORT")
    report.append("=" * 80)
    report.append(f"Generated: {time.strftime('%Y-%m-%d %H:%M:%S')}")
    report.append("")

    # Summary
    total = len(results)
    passed = sum(1 for r in results if r.status == "PASS")
    failed = sum(1 for r in results if r.status == "FAIL")
    errors = sum(1 for r in results if r.status == "ERROR")

    report.append("EXECUTIVE SUMMARY")
    report.append("-" * 80)
    report.append(f"Total Tests: {total}")
    report.append(f"Passed: {passed} ({passed*100//total if total else 0}%)")
    report.append(f"Failed: {failed}")
    report.append(f"Errors: {errors}")
    report.append("")

    # Detailed results
    report.append("DETAILED RESULTS")
    report.append("-" * 80)
    for result in results:
        report.append(f"\n{result.scenario}: {result.status}")
        report.append(f"  Message: {result.message}")
        if result.details:
            report.append(f"  Details: {json.dumps(result.details, indent=4)}")

    # Critical issues
    critical_issues = [r for r in results if r.status in ["FAIL", "ERROR"]]
    if critical_issues:
        report.append("\n" + "=" * 80)
        report.append("CRITICAL ISSUES")
        report.append("-" * 80)
        for issue in critical_issues:
            report.append(f"  - {issue.scenario}: {issue.message}")

    # Recommendations
    report.append("\n" + "=" * 80)
    report.append("RECOMMENDATIONS")
    report.append("-" * 80)
    if passed == total:
        report.append("  ✓ All tests passed. Download functionality is working correctly.")
    else:
        report.append("  ✗ Some tests failed. Review critical issues above.")
        if any("permission" in r.message.lower() for r in results if r.status == "FAIL"):
            report.append("  - Review permission checking logic in download handler")
        if any("hash" in r.message.lower() or "corruption" in r.message.lower() for r in results if r.status == "FAIL"):
            report.append("  - Investigate data integrity issues in file transfer")
        if any("concurrent" in r.scenario.lower() for r in results if r.status == "FAIL"):
            report.append("  - Review thread safety and locking mechanisms")

    report_text = "\n".join(report)
    print("\n" + report_text)

    # Write to file
    with open(output_file, 'w') as f:
        f.write(report_text)

    print(f"\n✓ Report saved to: {output_file}")

def main():
    host = sys.argv[1] if len(sys.argv) > 1 else 'localhost'
    port = int(sys.argv[2]) if len(sys.argv) > 2 else 8080

    print("=" * 80)
    print("NETWORK FILE MANAGER - COMPREHENSIVE DOWNLOAD TEST SUITE")
    print("=" * 80)
    print(f"Server: {host}:{port}")
    print(f"Test Users: test1, test2")
    print("")

    # Run all test scenarios
    results = []

    results.append(scenario1_owner_downloads_own_file(host, port))
    print_test_result(results[-1])
    time.sleep(1)  # 1 second delay to ensure unique UUID generation

    results.append(scenario2_cross_user_download(host, port))
    print_test_result(results[-1])
    time.sleep(1)

    results.append(scenario3_private_file_download_attempt(host, port))
    print_test_result(results[-1])
    time.sleep(1)

    results.append(scenario4_large_file_download(host, port))
    print_test_result(results[-1])
    time.sleep(1)

    results.append(scenario5_concurrent_downloads(host, port))
    print_test_result(results[-1])

    # Generate report
    output_file = os.path.join(
        os.path.dirname(__file__),
        "reports",
        f"comprehensive_download_test_{int(time.time())}.md"
    )
    os.makedirs(os.path.dirname(output_file), exist_ok=True)
    generate_report(results, output_file)

    # Exit code
    failed_count = sum(1 for r in results if r.status in ["FAIL", "ERROR"])
    sys.exit(0 if failed_count == 0 else 1)

if __name__ == '__main__':
    main()
