#!/bin/bash
# Cleanup orphaned database entries (files without physical storage)

echo "=== Checking for orphaned file entries ==="
sqlite3 fileshare.db "SELECT id, name, physical_path FROM files WHERE is_directory=0" | while IFS='|' read id name uuid; do
    if [ ! -z "$uuid" ]; then
        # Check if physical file exists
        if ! find storage -name "$uuid" | grep -q .; then
            echo "Orphaned: ID $id - $name (UUID: $uuid)"
            sqlite3 fileshare.db "DELETE FROM files WHERE id=$id"
            echo "  ✓ Deleted"
        fi
    fi
done

echo ""
echo "✓ Cleanup complete"
