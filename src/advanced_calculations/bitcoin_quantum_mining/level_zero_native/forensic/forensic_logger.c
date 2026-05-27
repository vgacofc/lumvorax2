#include "forensic_logger.h"
#include "execbuffer2_forensic.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <pthread.h>

/**
 * FORENSIC LOGGER - IMPLÉMENTATION
 * 
 * Logger thread-safe avec timestamps nanoseconde
 */

static logger_config_t g_logger_config = {0};
static pthread_mutex_t g_logger_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool g_logger_initialized = false;

bool init_forensic_logger(const char *log_path) {
    pthread_mutex_lock(&g_logger_mutex);
    
    if (g_logger_initialized) {
        pthread_mutex_unlock(&g_logger_mutex);
        return true;
    }
    
    // Ouvrir fichier log
    if (log_path) {
        g_logger_config.log_file = fopen(log_path, "w");
        if (!g_logger_config.log_file) {
            pthread_mutex_unlock(&g_logger_mutex);
            return false;
        }
        g_logger_config.file_output = true;
    } else {
        g_logger_config.log_file = NULL;
        g_logger_config.file_output = false;
    }
    
    // Configuration par défaut
    g_logger_config.min_level = LOG_LEVEL_DEBUG;
    g_logger_config.console_output = true;
    g_logger_config.timestamps = true;
    
    g_logger_initialized = true;
    
    pthread_mutex_unlock(&g_logger_mutex);
    return true;
}

void close_forensic_logger(void) {
    pthread_mutex_lock(&g_logger_mutex);
    
    if (g_logger_config.log_file) {
        fclose(g_logger_config.log_file);
        g_logger_config.log_file = NULL;
    }
    
    g_logger_initialized = false;
    
    pthread_mutex_unlock(&g_logger_mutex);
}

void log_event(const char *level, const char *format, ...) {
    if (!g_logger_initialized && !init_forensic_logger(NULL)) {
        return;
    }
    
    pthread_mutex_lock(&g_logger_mutex);
    
    char buffer[4096];
    va_list args;
    
    // Timestamp
    if (g_logger_config.timestamps) {
        uint64_t ts_ns = get_timestamp_ns();
        uint64_t sec = ts_ns / 1000000000ULL;
        uint64_t nsec = ts_ns % 1000000000ULL;
        snprintf(buffer, sizeof(buffer), "[%lu.%09lu] [%s] ", sec, nsec, level);
    } else {
        snprintf(buffer, sizeof(buffer), "[%s] ", level);
    }
    
    // Message
    size_t prefix_len = strlen(buffer);
    va_start(args, format);
    vsnprintf(buffer + prefix_len, sizeof(buffer) - prefix_len, format, args);
    va_end(args);
    
    // Ajouter newline si manquant
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len-1] != '\n') {
        if (len < sizeof(buffer) - 1) {
            buffer[len] = '\n';
            buffer[len+1] = '\0';
        }
    }
    
    // Sortie console
    if (g_logger_config.console_output) {
        fprintf(stdout, "%s", buffer);
        fflush(stdout);
    }
    
    // Sortie fichier
    if (g_logger_config.file_output && g_logger_config.log_file) {
        fprintf(g_logger_config.log_file, "%s", buffer);
        fflush(g_logger_config.log_file);
    }
    
    pthread_mutex_unlock(&g_logger_mutex);
}

// Made with Bob
