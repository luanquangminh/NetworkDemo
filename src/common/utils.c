#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>

static FILE* log_file_handle = NULL;
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

void log_init(const char* log_file) {
    pthread_mutex_lock(&log_mutex);
    if (log_file_handle) {
        fclose(log_file_handle);
    }
    log_file_handle = fopen(log_file, "a");
    if (!log_file_handle) {
        fprintf(stderr, "Warning: Failed to open log file %s\n", log_file);
    }
    pthread_mutex_unlock(&log_mutex);
}

void log_close(void) {
    pthread_mutex_lock(&log_mutex);
    if (log_file_handle) {
        fclose(log_file_handle);
        log_file_handle = NULL;
    }
    pthread_mutex_unlock(&log_mutex);
}

void log_info(const char* format, ...) {
    pthread_mutex_lock(&log_mutex);
    char* ts = get_timestamp();

    va_list args;
    va_start(args, format);

    if (log_file_handle) {
        fprintf(log_file_handle, "[%s] [INFO] ", ts);
        vfprintf(log_file_handle, format, args);
        fprintf(log_file_handle, "\n");
        fflush(log_file_handle);
    }

    va_end(args);
    free(ts);
    pthread_mutex_unlock(&log_mutex);
}

void log_error(const char* format, ...) {
    pthread_mutex_lock(&log_mutex);
    char* ts = get_timestamp();

    va_list args;
    va_start(args, format);

    if (log_file_handle) {
        fprintf(log_file_handle, "[%s] [ERROR] ", ts);
        vfprintf(log_file_handle, format, args);
        fprintf(log_file_handle, "\n");
        fflush(log_file_handle);
    }

    // Also print to stderr
    fprintf(stderr, "[ERROR] ");
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");

    va_end(args);
    free(ts);
    pthread_mutex_unlock(&log_mutex);
}

void log_debug(const char* format, ...) {
    #ifdef DEBUG
    pthread_mutex_lock(&log_mutex);
    char* ts = get_timestamp();

    va_list args;
    va_start(args, format);

    if (log_file_handle) {
        fprintf(log_file_handle, "[%s] [DEBUG] ", ts);
        vfprintf(log_file_handle, format, args);
        fprintf(log_file_handle, "\n");
        fflush(log_file_handle);
    }

    va_end(args);
    free(ts);
    pthread_mutex_unlock(&log_mutex);
    #else
    (void)format;
    #endif
}

char* generate_uuid(void) {
    char* uuid_str = malloc(37);
    if (!uuid_str) return NULL;

    // Simple UUID generation using random
    srand(time(NULL) ^ getpid());
    snprintf(uuid_str, 37, "%08x-%04x-%04x-%04x-%012llx",
             rand(), rand() & 0xffff, (rand() & 0x0fff) | 0x4000,
             (rand() & 0x3fff) | 0x8000,
             ((long long)rand() << 32) | rand());
    return uuid_str;
}

char* get_timestamp(void) {
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    char* buf = malloc(32);
    if (!buf) return NULL;

    strftime(buf, 32, "%Y-%m-%d %H:%M:%S", t);
    return buf;
}

char* str_duplicate(const char* str) {
    if (!str) return NULL;

    size_t len = strlen(str) + 1;
    char* dup = malloc(len);
    if (dup) {
        memcpy(dup, str, len);
    }
    return dup;
}
