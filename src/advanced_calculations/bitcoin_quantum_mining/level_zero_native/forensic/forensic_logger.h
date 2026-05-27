#ifndef FORENSIC_LOGGER_H
#define FORENSIC_LOGGER_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

/**
 * FORENSIC LOGGER - Système de logging nanoseconde
 * 
 * Fonctionnalités:
 * - Timestamps nanoseconde
 * - Niveaux de log (DEBUG, INFO, WARNING, ERROR)
 * - Sortie fichier et console
 * - Thread-safe
 */

// Niveaux de log
typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO = 1,
    LOG_LEVEL_WARNING = 2,
    LOG_LEVEL_ERROR = 3
} log_level_t;

// Configuration logger
typedef struct {
    FILE *log_file;
    log_level_t min_level;
    bool console_output;
    bool file_output;
    bool timestamps;
} logger_config_t;

/**
 * Initialiser logger avec fichier
 */
bool init_forensic_logger(const char *log_path);

/**
 * Fermer logger
 */
void close_forensic_logger(void);

/**
 * Logger événement avec niveau
 */
void log_event(const char *level, const char *format, ...);

#endif // FORENSIC_LOGGER_H

// Made with Bob
