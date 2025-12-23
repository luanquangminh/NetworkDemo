#ifndef STORAGE_H
#define STORAGE_H

#include <stddef.h>
#include <stdint.h>

// Initialize storage directory
int storage_init(const char* base_path);

// Get full path for a UUID
char* storage_get_path(const char* uuid);

// Write file to storage
int storage_write_file(const char* uuid, const uint8_t* data, size_t size);

// Read file from storage
int storage_read_file(const char* uuid, uint8_t** data, size_t* size);

// Delete file from storage
int storage_delete_file(const char* uuid);

// Check if file exists
int storage_file_exists(const char* uuid);

#endif
