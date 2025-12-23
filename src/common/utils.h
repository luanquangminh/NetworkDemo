#ifndef UTILS_H
#define UTILS_H

#include <pthread.h>

// Logging
void log_init(const char* log_file);
void log_close(void);
void log_info(const char* format, ...);
void log_error(const char* format, ...);
void log_debug(const char* format, ...);

// UUID
char* generate_uuid(void);

// Timestamp
char* get_timestamp(void);

// String utilities
char* str_duplicate(const char* str);

#endif // UTILS_H
