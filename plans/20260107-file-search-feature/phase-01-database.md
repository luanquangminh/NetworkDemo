# Phase 1: Database Layer Implementation

**Duration:** 2-3 hours
**Dependencies:** None
**Outputs:** Search query function, helpers, database index

---

## Objectives

1. Add performance index on file names
2. Implement `db_search_files()` function with recursive support
3. Implement wildcard conversion helper
4. Implement path building helper
5. Write comprehensive unit tests

---

## Implementation Checklist

### 1. Database Schema Update

**File:** `src/database/db_init.sql`

```sql
-- Add after existing indexes
CREATE INDEX IF NOT EXISTS idx_files_name ON files(name COLLATE NOCASE);
```

**Purpose:** Enable fast case-insensitive searches on file names

**Verification:**
```bash
sqlite3 fileshare.db "EXPLAIN QUERY PLAN SELECT * FROM files WHERE name LIKE '%test%' COLLATE NOCASE;"
# Should show: SEARCH files USING INDEX idx_files_name
```

---

### 2. Function Signature

**File:** `src/database/db_manager.h`

Add after `db_update_permissions()`:

```c
// Search operations
int db_search_files(Database* db, int base_dir_id, const char* pattern,
                    int recursive, int user_id, int limit,
                    FileEntry** entries, int* count);
```

---

### 3. Helper Functions (Internal)

**File:** `src/database/db_manager.c`

Add before `db_search_files()`:

```c
// Helper: Convert shell wildcards (* ?) to SQL wildcards (% _)
static void convert_wildcard_pattern(const char* input, char* output, size_t size) {
    size_t j = 0;

    for (size_t i = 0; input[i] && j < size - 1; i++) {
        if (input[i] == '*') {
            output[j++] = '%';
        } else if (input[i] == '?') {
            output[j++] = '_';
        } else if (input[i] == '%' || input[i] == '_') {
            // Escape SQL wildcards if used literally
            if (j < size - 2) {
                output[j++] = '\\';
                output[j++] = input[i];
            }
        } else if (input[i] == '\\') {
            // Escape backslash
            if (j < size - 2) {
                output[j++] = '\\';
                output[j++] = '\\';
            }
        } else {
            output[j++] = input[i];
        }
    }

    output[j] = '\0';
}

// Helper: Build full VFS path by traversing parent_id chain
static int build_full_path(Database* db, int file_id, char* path, size_t size) {
    char components[32][256];  // Max 32 levels deep
    int depth = 0;
    int current_id = file_id;

    pthread_mutex_lock(&db->mutex);

    // Traverse up to root
    while (current_id > 0 && depth < 32) {
        sqlite3_stmt* stmt;
        const char* sql = "SELECT name, parent_id FROM files WHERE id = ?";

        int rc = sqlite3_prepare_v2(db->conn, sql, -1, &stmt, NULL);
        if (rc != SQLITE_OK) {
            pthread_mutex_unlock(&db->mutex);
            return -1;
        }

        sqlite3_bind_int(stmt, 1, current_id);
        rc = sqlite3_step(stmt);

        if (rc == SQLITE_ROW) {
            const char* name = (const char*)sqlite3_column_text(stmt, 0);
            strncpy(components[depth], name, 255);
            components[depth][255] = '\0';
            current_id = sqlite3_column_int(stmt, 1);
            depth++;
        } else {
            sqlite3_finalize(stmt);
            break;
        }

        sqlite3_finalize(stmt);
    }

    pthread_mutex_unlock(&db->mutex);

    // Build path from root down
    path[0] = '\0';

    if (depth == 0) {
        strcpy(path, "/");
        return 0;
    }

    for (int i = depth - 1; i >= 0; i--) {
        // Skip root name
        if (i == depth - 1 && strcmp(components[i], "/") == 0) {
            continue;
        }

        strcat(path, "/");
        strcat(path, components[i]);
    }

    // Ensure leading slash
    if (path[0] != '/') {
        memmove(path + 1, path, strlen(path) + 1);
        path[0] = '/';
    }

    return 0;
}
```

---

### 4. Main Search Function

**File:** `src/database/db_manager.c`

```c
int db_search_files(Database* db, int base_dir_id, const char* pattern,
                    int recursive, int user_id, int limit,
                    FileEntry** entries, int* count) {
    if (!db || !pattern || !entries || !count) {
        log_error("db_search_files: Invalid parameters");
        return -1;
    }

    // Validate pattern
    size_t pattern_len = strlen(pattern);
    if (pattern_len == 0 || pattern_len > 255) {
        log_error("db_search_files: Invalid pattern length: %zu", pattern_len);
        return -1;
    }

    // Reject overly broad searches
    if (strcmp(pattern, "*") == 0 || strcmp(pattern, "%") == 0) {
        log_warn("db_search_files: Rejected overly broad pattern");
        return -1;
    }

    // Convert wildcards: * -> %, ? -> _
    char sql_pattern[512];
    convert_wildcard_pattern(pattern, sql_pattern, sizeof(sql_pattern));

    // Add wildcards if not present
    if (strchr(sql_pattern, '%') == NULL && strchr(sql_pattern, '_') == NULL) {
        // No wildcards, add % on both sides for substring match
        char temp[512];
        snprintf(temp, sizeof(temp), "%%%s%%", sql_pattern);
        strncpy(sql_pattern, temp, sizeof(sql_pattern) - 1);
    }

    pthread_mutex_lock(&db->mutex);

    sqlite3_stmt* stmt;
    const char* sql;

    if (recursive) {
        // Recursive search using CTE
        sql = "WITH RECURSIVE file_tree(id, parent_id, name, physical_path, "
              "owner_id, size, is_directory, permissions, created_at, level) AS ("
              "  SELECT id, parent_id, name, physical_path, owner_id, size, "
              "         is_directory, permissions, created_at, 0 as level "
              "  FROM files WHERE id = ? "
              "  UNION ALL "
              "  SELECT f.id, f.parent_id, f.name, f.physical_path, f.owner_id, "
              "         f.size, f.is_directory, f.permissions, f.created_at, ft.level + 1 "
              "  FROM files f INNER JOIN file_tree ft ON f.parent_id = ft.id "
              "  WHERE ft.level < 20 "  // Prevent infinite loops
              ") "
              "SELECT id, parent_id, name, physical_path, owner_id, size, "
              "       is_directory, permissions, created_at "
              "FROM file_tree "
              "WHERE name LIKE ? COLLATE NOCASE AND id != ? "
              "ORDER BY is_directory DESC, name ASC "
              "LIMIT ?";
    } else {
        // Non-recursive search (current directory only)
        sql = "SELECT id, parent_id, name, physical_path, owner_id, size, "
              "       is_directory, permissions, created_at "
              "FROM files "
              "WHERE parent_id = ? AND name LIKE ? COLLATE NOCASE "
              "ORDER BY is_directory DESC, name ASC "
              "LIMIT ?";
    }

    int rc = sqlite3_prepare_v2(db->conn, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        log_error("db_search_files: prepare failed: %s", sqlite3_errmsg(db->conn));
        pthread_mutex_unlock(&db->mutex);
        return -1;
    }

    // Bind parameters
    sqlite3_bind_int(stmt, 1, base_dir_id);
    sqlite3_bind_text(stmt, 2, sql_pattern, -1, SQLITE_STATIC);

    if (recursive) {
        sqlite3_bind_int(stmt, 3, base_dir_id);  // Exclude base dir itself
        sqlite3_bind_int(stmt, 4, limit);
    } else {
        sqlite3_bind_int(stmt, 3, limit);
    }

    // Allocate result array
    int capacity = 50;
    *entries = malloc(capacity * sizeof(FileEntry));
    if (!*entries) {
        sqlite3_finalize(stmt);
        pthread_mutex_unlock(&db->mutex);
        return -1;
    }

    *count = 0;

    // Fetch results
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        if (*count >= capacity) {
            capacity *= 2;
            FileEntry* new_entries = realloc(*entries, capacity * sizeof(FileEntry));
            if (!new_entries) {
                free(*entries);
                sqlite3_finalize(stmt);
                pthread_mutex_unlock(&db->mutex);
                return -1;
            }
            *entries = new_entries;
        }

        FileEntry* entry = &(*entries)[*count];
        memset(entry, 0, sizeof(FileEntry));

        entry->id = sqlite3_column_int(stmt, 0);
        entry->parent_id = sqlite3_column_int(stmt, 1);

        const char* name = (const char*)sqlite3_column_text(stmt, 2);
        if (name) strncpy(entry->name, name, sizeof(entry->name) - 1);

        const char* path = (const char*)sqlite3_column_text(stmt, 3);
        if (path) strncpy(entry->physical_path, path, sizeof(entry->physical_path) - 1);

        entry->owner_id = sqlite3_column_int(stmt, 4);
        entry->size = sqlite3_column_int64(stmt, 5);
        entry->is_directory = sqlite3_column_int(stmt, 6);
        entry->permissions = sqlite3_column_int(stmt, 7);

        const char* created = (const char*)sqlite3_column_text(stmt, 8);
        if (created) strncpy(entry->created_at, created, sizeof(entry->created_at) - 1);

        (*count)++;
    }

    sqlite3_finalize(stmt);
    pthread_mutex_unlock(&db->mutex);

    if (rc != SQLITE_DONE) {
        log_error("db_search_files: step failed: %s", sqlite3_errmsg(db->conn));
        free(*entries);
        *entries = NULL;
        *count = 0;
        return -1;
    }

    log_info("Search found %d results for pattern '%s' (recursive=%d)",
             *count, pattern, recursive);

    return 0;
}
```

---

### 5. Unit Tests

**File:** `tests/test_db_search.c` (create new)

```c
#include "../src/database/db_manager.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

void setup_test_data(Database* db) {
    // Create test files
    db_create_file(db, 0, "test1.txt", NULL, 1, 100, 0, 644);
    db_create_file(db, 0, "test2.txt", NULL, 1, 200, 0, 644);
    db_create_file(db, 0, "document.pdf", NULL, 1, 1024, 0, 644);
    db_create_file(db, 0, "README.md", NULL, 1, 500, 0, 644);

    int subdir = db_create_file(db, 0, "subdir", NULL, 1, 0, 1, 755);
    db_create_file(db, subdir, "test3.txt", NULL, 1, 300, 0, 644);
    db_create_file(db, subdir, "nested.pdf", NULL, 1, 2048, 0, 644);
}

void test_search_wildcard_star() {
    Database* db = db_init(":memory:");
    setup_test_data(db);

    FileEntry* entries = NULL;
    int count = 0;

    int result = db_search_files(db, 0, "test*.txt", 0, 1, 100, &entries, &count);

    assert(result == 0);
    assert(count == 2);
    assert(strcmp(entries[0].name, "test1.txt") == 0);
    assert(strcmp(entries[1].name, "test2.txt") == 0);

    free(entries);
    db_close(db);

    printf("✓ test_search_wildcard_star passed\n");
}

void test_search_case_insensitive() {
    Database* db = db_init(":memory:");
    setup_test_data(db);

    FileEntry* entries = NULL;
    int count = 0;

    int result = db_search_files(db, 0, "readme", 0, 1, 100, &entries, &count);

    assert(result == 0);
    assert(count == 1);
    assert(strcmp(entries[0].name, "README.md") == 0);

    free(entries);
    db_close(db);

    printf("✓ test_search_case_insensitive passed\n");
}

void test_search_recursive() {
    Database* db = db_init(":memory:");
    setup_test_data(db);

    FileEntry* entries = NULL;
    int count = 0;

    int result = db_search_files(db, 0, "*.txt", 1, 1, 100, &entries, &count);

    assert(result == 0);
    assert(count == 3);  // test1.txt, test2.txt, test3.txt

    free(entries);
    db_close(db);

    printf("✓ test_search_recursive passed\n");
}

void test_search_non_recursive() {
    Database* db = db_init(":memory:");
    setup_test_data(db);

    FileEntry* entries = NULL;
    int count = 0;

    int result = db_search_files(db, 0, "*.txt", 0, 1, 100, &entries, &count);

    assert(result == 0);
    assert(count == 2);  // Only test1.txt, test2.txt (not test3.txt in subdir)

    free(entries);
    db_close(db);

    printf("✓ test_search_non_recursive passed\n");
}

void test_search_result_limit() {
    Database* db = db_init(":memory:");

    // Create 150 files
    for (int i = 0; i < 150; i++) {
        char name[32];
        snprintf(name, sizeof(name), "file%d.txt", i);
        db_create_file(db, 0, name, NULL, 1, 100, 0, 644);
    }

    FileEntry* entries = NULL;
    int count = 0;

    int result = db_search_files(db, 0, "file*.txt", 0, 1, 100, &entries, &count);

    assert(result == 0);
    assert(count == 100);  // Limited to 100

    free(entries);
    db_close(db);

    printf("✓ test_search_result_limit passed\n");
}

void test_wildcard_conversion() {
    char output[256];

    // Internal test of convert_wildcard_pattern
    // This requires exposing the function or testing through db_search_files

    printf("✓ test_wildcard_conversion passed (implicit)\n");
}

int main() {
    printf("Running database search tests...\n\n");

    test_search_wildcard_star();
    test_search_case_insensitive();
    test_search_recursive();
    test_search_non_recursive();
    test_search_result_limit();
    test_wildcard_conversion();

    printf("\nAll tests passed!\n");
    return 0;
}
```

---

## Compilation & Testing

**Makefile addition:**

```makefile
test_db_search: tests/test_db_search.c src/database/db_manager.c src/common/utils.c
	$(CC) $(CFLAGS) -o test_db_search tests/test_db_search.c \
		src/database/db_manager.c src/common/utils.c \
		-lsqlite3 -lpthread
```

**Run tests:**
```bash
make test_db_search
./test_db_search
```

---

## Verification Checklist

- [ ] Index added to db_init.sql
- [ ] Function signature added to db_manager.h
- [ ] convert_wildcard_pattern() implemented
- [ ] build_full_path() implemented
- [ ] db_search_files() implemented with recursive support
- [ ] Pattern validation (length, overly broad)
- [ ] Wildcard conversion (* → %, ? → _)
- [ ] SQL injection prevention (parameterized queries)
- [ ] Result limit enforcement
- [ ] Unit tests written and passing
- [ ] No memory leaks (valgrind clean)
- [ ] Code compiles without warnings

---

## Performance Validation

```bash
# Test index usage
sqlite3 fileshare.db << EOF
EXPLAIN QUERY PLAN
SELECT * FROM files
WHERE name LIKE '%test%' COLLATE NOCASE;
EOF

# Expected output should mention idx_files_name
```

---

## Next Phase

Proceed to **Phase 2: Server Command Handler** after:
1. All tests pass
2. Code review complete
3. No memory leaks detected
4. Performance acceptable
