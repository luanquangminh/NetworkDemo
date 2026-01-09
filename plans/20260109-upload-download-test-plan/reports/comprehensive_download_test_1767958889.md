================================================================================
COMPREHENSIVE DOWNLOAD FUNCTIONALITY TEST REPORT
================================================================================
Generated: 2026-01-09 18:41:29

EXECUTIVE SUMMARY
--------------------------------------------------------------------------------
Total Tests: 5
Passed: 5 (100%)
Failed: 0
Errors: 0

DETAILED RESULTS
--------------------------------------------------------------------------------

Scenario 1: PASS
  Message: Owner successfully downloaded own file
  Details: {
    "file_id": 38,
    "size": 41,
    "integrity": "verified"
}

Scenario 2: PASS
  Message: Cross-user download succeeded with 644 permissions
  Details: {
    "file_id": 39,
    "uploader": "test1",
    "downloader": "test2",
    "size": 67,
    "integrity": "verified"
}

Scenario 3: PASS
  Message: Permission correctly denied
  Details: {
    "file_id": 40,
    "permissions": "600",
    "error_message": "Permission denied"
}

Scenario 4: PASS
  Message: Large file downloaded successfully
  Details: {
    "file_id": 41,
    "size": 153600,
    "upload_time": "0.00s",
    "download_time": "0.00s",
    "hash": "eefb7e8c2af50d2e69a4ec0a7f5abf3ff4ef5b858f255a3978c9a03f2c89f311",
    "integrity": "verified"
}

Scenario 5: PASS
  Message: Concurrent downloads succeeded
  Details: {
    "file_id": 42,
    "concurrent_users": 2,
    "total_time": "0.04s",
    "results": [
        {
            "user": "test2",
            "success": true,
            "size": 29000,
            "time": 0.0001270771026611328,
            "hash": "9357d1d052ff7d060880aaf972c6ed9b1ad5c9da7a6d54e13f9b0987f3e6b2ad"
        },
        {
            "user": "test1",
            "success": true,
            "size": 29000,
            "time": 0.00018286705017089844,
            "hash": "9357d1d052ff7d060880aaf972c6ed9b1ad5c9da7a6d54e13f9b0987f3e6b2ad"
        }
    ],
    "integrity": "verified"
}

================================================================================
RECOMMENDATIONS
--------------------------------------------------------------------------------
  ✓ All tests passed. Download functionality is working correctly.