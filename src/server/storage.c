#include "storage.h"
#include "../common/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

static char storage_base[256] = {0};

int storage_init(const char* base_path) {
    if (!base_path || strlen(base_path) == 0) {
        log_error("Invalid storage base path");
        return -1;
    }

    strncpy(storage_base, base_path, sizeof(storage_base) - 1);
    storage_base[sizeof(storage_base) - 1] = '\0';

    // Create base storage directory
    struct stat st = {0};
    if (stat(storage_base, &st) == -1) {
        if (mkdir(storage_base, 0755) == -1) {
            log_error("Failed to create storage directory '%s': %s",
                     storage_base, strerror(errno));
            return -1;
        }
    }

    log_info("Storage initialized at: %s", storage_base);
    return 0;
}

char* storage_get_path(const char* uuid) {
    if (!uuid || strlen(uuid) < 2) {
        log_error("Invalid UUID");
        return NULL;
    }

    // Path format: storage/<first_2_chars>/<uuid>
    char* full_path = malloc(512);
    if (!full_path) {
        log_error("Memory allocation failed");
        return NULL;
    }

    char subdir[3] = {uuid[0], uuid[1], '\0'};
    snprintf(full_path, 512, "%s/%s/%s", storage_base, subdir, uuid);

    return full_path;
}

int storage_write_file(const char* uuid, const uint8_t* data, size_t size) {
    if (!uuid || !data || size == 0) {
        log_error("Invalid parameters for storage_write_file");
        return -1;
    }

    char* full_path = storage_get_path(uuid);
    if (!full_path) {
        return -1;
    }

    // Create subdirectory if needed
    char subdir_path[512];
    char subdir[3] = {uuid[0], uuid[1], '\0'};
    snprintf(subdir_path, sizeof(subdir_path), "%s/%s", storage_base, subdir);

    struct stat st = {0};
    if (stat(subdir_path, &st) == -1) {
        if (mkdir(subdir_path, 0755) == -1) {
            log_error("Failed to create subdirectory '%s': %s",
                     subdir_path, strerror(errno));
            free(full_path);
            return -1;
        }
    }

    // Write file
    FILE* fp = fopen(full_path, "wb");
    if (!fp) {
        log_error("Failed to open file '%s' for writing: %s",
                 full_path, strerror(errno));
        free(full_path);
        return -1;
    }

    size_t written = fwrite(data, 1, size, fp);
    fclose(fp);

    if (written != size) {
        log_error("Failed to write complete file. Expected %zu bytes, wrote %zu",
                 size, written);
        unlink(full_path);  // Clean up partial file
        free(full_path);
        return -1;
    }

    log_info("Wrote file to storage: %s (%zu bytes)", full_path, size);
    free(full_path);
    return 0;
}

int storage_read_file(const char* uuid, uint8_t** data, size_t* size) {
    if (!uuid || !data || !size) {
        log_error("Invalid parameters for storage_read_file");
        return -1;
    }

    char* full_path = storage_get_path(uuid);
    if (!full_path) {
        return -1;
    }

    FILE* fp = fopen(full_path, "rb");
    if (!fp) {
        log_error("Failed to open file '%s' for reading: %s",
                 full_path, strerror(errno));
        free(full_path);
        return -1;
    }

    // Get file size
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (file_size < 0) {
        log_error("Failed to get file size for '%s'", full_path);
        fclose(fp);
        free(full_path);
        return -1;
    }

    // Allocate buffer
    *data = malloc(file_size);
    if (!*data) {
        log_error("Memory allocation failed for file read");
        fclose(fp);
        free(full_path);
        return -1;
    }

    // Read file
    size_t read_bytes = fread(*data, 1, file_size, fp);
    fclose(fp);

    if (read_bytes != (size_t)file_size) {
        log_error("Failed to read complete file. Expected %ld bytes, read %zu",
                 file_size, read_bytes);
        free(*data);
        *data = NULL;
        free(full_path);
        return -1;
    }

    *size = file_size;
    log_info("Read file from storage: %s (%zu bytes)", full_path, *size);
    free(full_path);
    return 0;
}

int storage_delete_file(const char* uuid) {
    if (!uuid) {
        log_error("Invalid UUID for deletion");
        return -1;
    }

    char* full_path = storage_get_path(uuid);
    if (!full_path) {
        return -1;
    }

    if (unlink(full_path) == -1) {
        log_error("Failed to delete file '%s': %s", full_path, strerror(errno));
        free(full_path);
        return -1;
    }

    log_info("Deleted file from storage: %s", full_path);
    free(full_path);
    return 0;
}

int storage_file_exists(const char* uuid) {
    if (!uuid) {
        return 0;
    }

    char* full_path = storage_get_path(uuid);
    if (!full_path) {
        return 0;
    }

    struct stat st = {0};
    int exists = (stat(full_path, &st) == 0);

    free(full_path);
    return exists;
}
