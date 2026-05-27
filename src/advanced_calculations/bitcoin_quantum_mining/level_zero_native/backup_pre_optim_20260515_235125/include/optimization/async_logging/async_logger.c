#include "async_logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <errno.h>

static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

static const char* level_to_string(async_log_level_t level) {
    switch (level) {
        case ASYNC_LOG_DEBUG: return "DEBUG";
        case ASYNC_LOG_INFO:  return "INFO";
        case ASYNC_LOG_WARN:  return "WARN";
        case ASYNC_LOG_ERROR: return "ERROR";
        case ASYNC_LOG_FATAL: return "FATAL";
        default: return "UNKNOWN";
    }
}

static void* flush_thread_func(void* arg) {
    async_logger_t* logger = (async_logger_t*)arg;
    
    while (logger->running) {
        pthread_mutex_lock(&logger->mutex);
        
        while (logger->running && logger->head == logger->tail) {
            struct timespec timeout;
            clock_gettime(CLOCK_REALTIME, &timeout);
            timeout.tv_nsec += ASYNC_LOG_FLUSH_INTERVAL_MS * 1000000;
            if (timeout.tv_nsec >= 1000000000) {
                timeout.tv_sec += 1;
                timeout.tv_nsec -= 1000000000;
            }
            pthread_cond_timedwait(&logger->cond, &logger->mutex, &timeout);
        }
        
        while (logger->head != logger->tail) {
            async_log_entry_t* entry = &logger->buffer[logger->tail];
            
            if (logger->output_file) {
                fprintf(logger->output_file, "[%lu][%s][%s:%d:%s] %s\n",
                        entry->timestamp_ns,
                        level_to_string(entry->level),
                        entry->file ? entry->file : "?",
                        entry->line,
                        entry->func ? entry->func : "?",
                        entry->message);
            }
            
            logger->tail = (logger->tail + 1) % logger->capacity;
        }
        
        if (logger->output_file) {
            fflush(logger->output_file);
        }
        
        pthread_mutex_unlock(&logger->mutex);
    }
    
    return NULL;
}

async_logger_t* async_logger_create(const char* filepath, size_t buffer_size) {
    async_logger_t* logger = (async_logger_t*)calloc(1, sizeof(async_logger_t));
    if (!logger) return NULL;
    
    if (buffer_size == 0) buffer_size = ASYNC_LOG_BUFFER_SIZE;
    
    logger->buffer = (async_log_entry_t*)calloc(buffer_size, sizeof(async_log_entry_t));
    if (!logger->buffer) {
        free(logger);
        return NULL;
    }
    
    logger->capacity = buffer_size;
    logger->head = 0;
    logger->tail = 0;
    logger->total_entries = 0;
    logger->dropped_entries = 0;
    logger->min_level = ASYNC_LOG_DEBUG;
    
    if (filepath) {
        logger->output_file = fopen(filepath, "a");
        if (!logger->output_file) {
            free(logger->buffer);
            free(logger);
            return NULL;
        }
    }
    
    pthread_mutex_init(&logger->mutex, NULL);
    pthread_cond_init(&logger->cond, NULL);
    
    logger->running = true;
    if (pthread_create(&logger->flush_thread, NULL, flush_thread_func, logger) != 0) {
        if (logger->output_file) fclose(logger->output_file);
        pthread_mutex_destroy(&logger->mutex);
        pthread_cond_destroy(&logger->cond);
        free(logger->buffer);
        free(logger);
        return NULL;
    }
    
    return logger;
}

void async_logger_destroy(async_logger_t* logger) {
    if (!logger) return;
    
    logger->running = false;
    pthread_cond_signal(&logger->cond);
    pthread_join(logger->flush_thread, NULL);
    
    async_logger_flush(logger);
    
    if (logger->output_file) {
        fclose(logger->output_file);
    }
    
    pthread_mutex_destroy(&logger->mutex);
    pthread_cond_destroy(&logger->cond);
    free(logger->buffer);
    free(logger);
}

bool async_logger_log(async_logger_t* logger, async_log_level_t level,
                      const char* file, int line, const char* func,
                      const char* format, ...) {
    if (!logger || level < logger->min_level) return false;
    
    pthread_mutex_lock(&logger->mutex);
    
    size_t next_head = (logger->head + 1) % logger->capacity;
    if (next_head == logger->tail) {
        logger->dropped_entries++;
        pthread_mutex_unlock(&logger->mutex);
        return false;
    }
    
    async_log_entry_t* entry = &logger->buffer[logger->head];
    entry->timestamp_ns = get_timestamp_ns();
    entry->level = level;
    entry->file = file;
    entry->line = line;
    entry->func = func;
    
    va_list args;
    va_start(args, format);
    vsnprintf(entry->message, ASYNC_LOG_MAX_MESSAGE, format, args);
    va_end(args);
    
    logger->head = next_head;
    logger->total_entries++;
    
    pthread_cond_signal(&logger->cond);
    pthread_mutex_unlock(&logger->mutex);
    
    return true;
}

void async_logger_flush(async_logger_t* logger) {
    if (!logger) return;
    
    pthread_mutex_lock(&logger->mutex);
    
    while (logger->head != logger->tail) {
        async_log_entry_t* entry = &logger->buffer[logger->tail];
        
        if (logger->output_file) {
            fprintf(logger->output_file, "[%lu][%s][%s:%d:%s] %s\n",
                    entry->timestamp_ns,
                    level_to_string(entry->level),
                    entry->file ? entry->file : "?",
                    entry->line,
                    entry->func ? entry->func : "?",
                    entry->message);
        }
        
        logger->tail = (logger->tail + 1) % logger->capacity;
    }
    
    if (logger->output_file) {
        fflush(logger->output_file);
    }
    
    pthread_mutex_unlock(&logger->mutex);
}

void async_logger_set_level(async_logger_t* logger, async_log_level_t level) {
    if (logger) logger->min_level = level;
}

size_t async_logger_pending_count(async_logger_t* logger) {
    if (!logger) return 0;
    size_t head = logger->head;
    size_t tail = logger->tail;
    if (head >= tail) return head - tail;
    return logger->capacity - tail + head;
}

uint64_t async_logger_get_total(async_logger_t* logger) {
    return logger ? logger->total_entries : 0;
}

uint64_t async_logger_get_dropped(async_logger_t* logger) {
    return logger ? logger->dropped_entries : 0;
}
