#ifndef PERMISSIONS_H
#define PERMISSIONS_H

#include "../database/db_manager.h"

// Permission bits (Linux-style)
#define PERM_READ    4
#define PERM_WRITE   2
#define PERM_EXECUTE 1

// Permission scopes
#define PERM_OWNER_SHIFT 6  // Bits 6-8: owner rwx
#define PERM_GROUP_SHIFT 3  // Bits 3-5: group rwx (not implemented)
#define PERM_OTHER_SHIFT 0  // Bits 0-2: others rwx

// Access types for permission checks
typedef enum {
    ACCESS_READ,     // Download, list files
    ACCESS_WRITE,    // Upload, mkdir, delete
    ACCESS_EXECUTE   // Enter directory (cd)
} AccessType;

// Check if user has permission on file/directory
// Returns 1 if allowed, 0 if denied
int check_permission(Database* db, int user_id, int file_id, AccessType access);

// Extract permission bits for a scope
int get_permission_bits(int permissions, int shift);

// Check if permission bits include access type
int has_access(int perm_bits, AccessType access);

// Format permissions as string (e.g., "rwxr-xr-x")
char* format_permissions(int permissions);

// Parse permission string to integer (e.g., "755" -> 493)
int parse_permissions(const char* str);

#endif
