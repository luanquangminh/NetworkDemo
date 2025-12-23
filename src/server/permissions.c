#include "permissions.h"
#include "../common/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int get_permission_bits(int permissions, int shift) {
    return (permissions >> shift) & 0x7;
}

int has_access(int perm_bits, AccessType access) {
    switch (access) {
        case ACCESS_READ:    return (perm_bits & PERM_READ) != 0;
        case ACCESS_WRITE:   return (perm_bits & PERM_WRITE) != 0;
        case ACCESS_EXECUTE: return (perm_bits & PERM_EXECUTE) != 0;
    }
    return 0;
}

int check_permission(Database* db, int user_id, int file_id, AccessType access) {
    // Root directory (id=0) is accessible to all authenticated users
    if (file_id == 0) {
        return 1;
    }

    FileEntry entry;
    if (db_get_file_by_id(db, file_id, &entry) < 0) {
        log_error("Permission check: file %d not found", file_id);
        return 0;
    }

    int perm_bits;

    // Check if user is owner
    if (entry.owner_id == user_id) {
        perm_bits = get_permission_bits(entry.permissions, PERM_OWNER_SHIFT);
    } else {
        // For others (group not implemented)
        perm_bits = get_permission_bits(entry.permissions, PERM_OTHER_SHIFT);
    }

    int allowed = has_access(perm_bits, access);

    if (!allowed) {
        log_info("Permission denied: user %d access %d on file %d (perms=%03o)",
                 user_id, access, file_id, entry.permissions);
    }

    return allowed;
}

char* format_permissions(int permissions) {
    char* str = malloc(10);
    if (!str) return NULL;

    int owner = get_permission_bits(permissions, PERM_OWNER_SHIFT);
    int group = get_permission_bits(permissions, PERM_GROUP_SHIFT);
    int other = get_permission_bits(permissions, PERM_OTHER_SHIFT);

    str[0] = (owner & PERM_READ) ? 'r' : '-';
    str[1] = (owner & PERM_WRITE) ? 'w' : '-';
    str[2] = (owner & PERM_EXECUTE) ? 'x' : '-';
    str[3] = (group & PERM_READ) ? 'r' : '-';
    str[4] = (group & PERM_WRITE) ? 'w' : '-';
    str[5] = (group & PERM_EXECUTE) ? 'x' : '-';
    str[6] = (other & PERM_READ) ? 'r' : '-';
    str[7] = (other & PERM_WRITE) ? 'w' : '-';
    str[8] = (other & PERM_EXECUTE) ? 'x' : '-';
    str[9] = '\0';

    return str;
}

int parse_permissions(const char* str) {
    if (!str) return -1;

    // Handle octal string like "755"
    if (strlen(str) == 3) {
        int owner = str[0] - '0';
        int group = str[1] - '0';
        int other = str[2] - '0';

        if (owner < 0 || owner > 7 || group < 0 || group > 7 || other < 0 || other > 7) {
            return -1;
        }

        return (owner << PERM_OWNER_SHIFT) | (group << PERM_GROUP_SHIFT) | (other << PERM_OTHER_SHIFT);
    }

    // Handle numeric
    return atoi(str);
}
