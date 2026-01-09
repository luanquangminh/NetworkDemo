#include <stdio.h>
#include <string.h>

// Simulate storage_get_path
char* storage_get_path(const char* uuid, const char* storage_base) {
    static char full_path[512];
    char subdir[3] = {uuid[0], uuid[1], '\0'};
    snprintf(full_path, 512, "%s/%s/%s", storage_base, subdir, uuid);
    return full_path;
}

int main() {
    // Case 1: UUID from database (just the UUID)
    const char* uuid_from_db = "602a2c5f-424b-43c5-8ff5-5245dc77688d";
    const char* storage_base = "storage";
    
    printf("UUID from DB: %s\n", uuid_from_db);
    printf("Constructed path: %s\n", storage_get_path(uuid_from_db, storage_base));
    
    return 0;
}
