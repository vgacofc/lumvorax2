#ifndef ASYNC_LOGGER_H
#define ASYNC_LOGGER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <pthread.h>

#define ASYNC_LOG_VERSION_MAJOR 1
#define ASYNC_LOG_VERSION_MINOR 0
#define ASYNC_LOG_VERSION_PATCH 0

#define ASYNC_LOG_BUFFER_SIZE 65536
#define ASYNC_LOG_MAX_MESSAGE 1024
#define ASYNC_LOG_FLUSH_INTERVAL_MS 100

typedef enum {
    ASYNC_LOG_DEBUG = 0,
    ASYNC_LOG_INFO = 1,
    ASYNC_LOG_WARN = 2,
    ASYNC_LOG_ERROR = 3,
    ASYNC_LOG_FATAL = 4
} async_log_level_t;

typedef struct {
    uint64_t timestamp_ns;
    async_log_level_t level;
    char message[ASYNC_LOG_MAX_MESSAGE];
    const char* file;
    int line;
    const char* func;
} async_log_entry_t;

typedef struct {
    async_log_entry_t* buffer;
    size_t capacity;
    volatile size_t head;
    volatile size_t tail;
    volatile bool running;
    pthread_t flush_thread;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    FILE* output_file;
    uint64_t total_entries;
    uint64_t dropped_entries;
    async_log_level_t min_level;
} async_logger_t;

async_logger_t* async_logger_create(const char* filepath, size_t buffer_size);
void async_logger_destroy(async_logger_t* logger);
bool async_logger_log(async_logger_t* logger, async_log_level_t level, 
                      const char* file, int line, const char* func,
                      const char* format, ...);
void async_logger_flush(async_logger_t* logger);
void async_logger_set_level(async_logger_t* logger, async_log_level_t level);
size_t async_logger_pending_count(async_logger_t* logger);
uint64_t async_logger_get_total(async_logger_t* logger);
uint64_t async_logger_get_dropped(async_logger_t* logger);

#define ASYNC_LOG(logger, level, fmt, ...) \
    async_logger_log(logger, level, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

#define ASYNC_DEBUG(logger, fmt, ...) ASYNC_LOG(logger, ASYNC_LOG_DEBUG, fmt, ##__VA_ARGS__)
#define ASYNC_INFO(logger, fmt, ...)  ASYNC_LOG(logger, ASYNC_LOG_INFO, fmt, ##__VA_ARGS__)
#define ASYNC_WARN(logger, fmt, ...)  ASYNC_LOG(logger, ASYNC_LOG_WARN, fmt, ##__VA_ARGS__)
#define ASYNC_ERROR(logger, fmt, ...) ASYNC_LOG(logger, ASYNC_LOG_ERROR, fmt, ##__VA_ARGS__)
#define ASYNC_FATAL(logger, fmt, ...) ASYNC_LOG(logger, ASYNC_LOG_FATAL, fmt, ##__VA_ARGS__)

#endif
