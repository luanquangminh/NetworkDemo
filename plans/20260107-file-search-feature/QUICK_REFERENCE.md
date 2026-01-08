# Quick Reference Guide

## Command Summary

### CLI Usage
```bash
# Search in current directory
search *.txt

# Recursive search
search document -r
search *.pdf --recursive

# Pattern examples
search test*           # Prefix match
search *.log          # Suffix match
search *report*       # Contains match
search file?.txt      # Single character wildcard
```

### GUI Usage
1. Enter pattern in search box
2. Check "Recursive" if needed
3. Click "Search" or press Enter
4. Results appear in file list
5. Click "Clear" to return to directory view

## Code Snippets

### Database Search
```c
FileEntry* entries = NULL;
int count = 0;
int result = db_search_files(db, dir_id, "*.txt", recursive, user_id, 100, &entries, &count);
if (result == 0) {
    // Process entries
    free(entries);
}
```

### Client Search
```c
cJSON* results = client_search(conn, "*.pdf", 1);  // recursive=1
if (results) {
    cJSON* results_array = cJSON_GetObjectItem(results, "results");
    // Process results
    cJSON_Delete(results);
}
```

### Server Handler
```c
case CMD_SEARCH_REQ:
    handle_search(session, pkt);
    break;
```

## SQL Query Examples

### Non-recursive
```sql
SELECT id, parent_id, name, physical_path, owner_id, size,
       is_directory, permissions, created_at
FROM files
WHERE parent_id = ? AND name LIKE ? COLLATE NOCASE
ORDER BY is_directory DESC, name ASC
LIMIT ?;
```

### Recursive (CTE)
```sql
WITH RECURSIVE file_tree(...) AS (
    SELECT ... FROM files WHERE id = ?
    UNION ALL
    SELECT ... FROM files f
    INNER JOIN file_tree ft ON f.parent_id = ft.id
    WHERE ft.level < 20
)
SELECT * FROM file_tree
WHERE name LIKE ? COLLATE NOCASE
LIMIT ?;
```

## Protocol Packets

### Request Example
```
Magic: FA CE
Command: 54 (CMD_SEARCH_REQ)
Length: 00 00 00 3A (58 bytes)
Payload: {"pattern":"*.txt","directory_id":0,"recursive":true,"limit":100}
```

### Response Example
```
Magic: FA CE
Command: 55 (CMD_SEARCH_RES)
Length: 00 00 01 2F (303 bytes)
Payload: {"status":"OK","count":2,"results":[...]}
```

## Common Patterns

### Wildcard Conversion
```
Shell Pattern → SQL Pattern
*             → %
?             → _
test*         → test%
*.txt         → %.txt
file?.log     → file_.log
*report*      → %report%
```

### Permission Octal to String
```c
char perm_str[16];
snprintf(perm_str, sizeof(perm_str), "%03o", permissions);
// 644 → "644"
// 755 → "755"
```

### Size Formatting
```c
void format_size(long bytes, char* output, size_t size) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit = 0;
    double value = bytes;

    while (value >= 1024 && unit < 4) {
        value /= 1024;
        unit++;
    }

    snprintf(output, size, "%.1f %s", value, units[unit]);
}
```

## Error Handling

### Common Errors
```json
{"status":"ERROR","message":"Invalid search pattern"}
{"status":"ERROR","message":"Permission denied"}
{"status":"ERROR","message":"Search failed"}
{"status":"ERROR","message":"Missing 'pattern' field"}
```

### Error Codes
- `STATUS_OK` (0): Success
- `STATUS_ERROR` (1): Generic error
- `STATUS_PERM_DENIED` (3): No permission
- `STATUS_NOT_FOUND` (4): Not found

## Debugging Tips

### Enable Debug Logging
```c
log_debug("Search: pattern='%s', recursive=%d, limit=%d", pattern, recursive, limit);
```

### Check Query Plan
```bash
sqlite3 fileshare.db "EXPLAIN QUERY PLAN SELECT * FROM files WHERE name LIKE '%test%' COLLATE NOCASE;"
```

### Memory Leak Check
```bash
valgrind --leak-check=full ./client
```

### Network Debugging
```bash
tcpdump -i lo0 -X port 8080
```

## Performance Tuning

### Index Verification
```sql
SELECT * FROM sqlite_master WHERE type='index' AND name='idx_files_name';
```

### Query Timing
```sql
.timer on
SELECT * FROM files WHERE name LIKE '%test%' COLLATE NOCASE;
```

### Result Count Check
```sql
SELECT COUNT(*) FROM files WHERE name LIKE '%test%' COLLATE NOCASE;
```

## Testing Commands

### Unit Tests
```bash
make test_db_search
./test_db_search
```

### Integration Tests
```bash
make test_search_integration
./test_search_integration
```

### Manual Testing
```bash
# Start server
./server

# In another terminal
./client
> login admin admin
> search *.txt
> search document -r
> exit
```

## Security Checklist

- [ ] Pattern validated (length, characters)
- [ ] SQL parameterized (no string concatenation)
- [ ] Permissions checked before search
- [ ] Result limit enforced
- [ ] Activity logged
- [ ] No physical paths exposed

## Common Issues

### Issue: "Search failed"
**Cause:** Database error, invalid parameters
**Fix:** Check logs, validate pattern, check DB connection

### Issue: "Permission denied"
**Cause:** User lacks READ permission on directory
**Fix:** Verify permissions with `ls -l` or chmod

### Issue: No results found
**Cause:** Pattern doesn't match, wrong directory
**Fix:** Try broader pattern (*), check recursive flag

### Issue: Slow searches
**Cause:** Missing index, large DB, recursive search
**Fix:** Verify index, reduce result limit, optimize query

## File Locations

```
src/
├── common/
│   └── protocol.h          # CMD_SEARCH_REQ, CMD_SEARCH_RES
├── database/
│   ├── db_manager.h        # db_search_files()
│   ├── db_manager.c        # Implementation
│   └── db_init.sql         # Index
├── server/
│   ├── commands.h          # handle_search()
│   └── commands.c          # Implementation
└── client/
    ├── client.h            # client_search()
    ├── client.c            # Implementation
    ├── main.c              # CLI command
    └── gui/
        ├── main_window.c   # Search widget
        └── gui.h           # AppState
```

## Next Steps

1. Read full plan: `plan.md`
2. Start Phase 1: `phase-01-database.md`
3. Implement in order (Phase 1 → 5)
4. Test after each phase
5. Review security checklist
6. Update documentation

---

**Quick Reference Complete** | For detailed info see main plan
