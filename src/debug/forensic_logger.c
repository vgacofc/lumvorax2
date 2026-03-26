
#include "forensic_logger.h"
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>   // Pour mkdir()
#include <unistd.h>     // Pour access()
#include <errno.h>      // Pour errno

static FILE* forensic_log_file = NULL;

bool forensic_logger_init(const char* filename) {
    if (!filename) {
        fprintf(stderr, "[FORENSIC] ERROR: filename is NULL\n");
        return false;
    }
    
    // Créer répertoire si nécessaire avec vérification complète
    char dir_path[256];
    strncpy(dir_path, filename, sizeof(dir_path) - 1);
    dir_path[sizeof(dir_path) - 1] = '\0';
    
    char *last_slash = strrchr(dir_path, '/');
    if (last_slash) {
        *last_slash = '\0';
        
        // Créer récursivement tous les répertoires parents
        char temp_path[256];
        char *token = strtok(dir_path, "/");
        temp_path[0] = '\0';  // Secure initialization
        
        while (token != NULL) {
            strncat(temp_path, token, sizeof(temp_path) - strlen(temp_path) - 1);
            strncat(temp_path, "/", sizeof(temp_path) - strlen(temp_path) - 1);
            mkdir(temp_path, 0755);
            token = strtok(NULL, "/");
        }
    }
    
    // Tentative d'ouverture avec gestion d'erreur robuste
    forensic_log_file = fopen(filename, "w");
    if (!forensic_log_file) {
        // Fallback vers répertoire courant
        char fallback_name[256];
        snprintf(fallback_name, sizeof(fallback_name), "forensic_fallback_%lu.log", 
                 (unsigned long)time(NULL));
        
        forensic_log_file = fopen(fallback_name, "w");
        if (!forensic_log_file) {
            fprintf(stderr, "[FORENSIC] CRITICAL: Cannot create any log file\n");
            return false;
        }
        
        fprintf(stderr, "[FORENSIC] WARNING: Using fallback log: %s\n", fallback_name);
    }
    
    uint64_t timestamp = lum_get_timestamp();
    fprintf(forensic_log_file, "=== FORENSIC LOG STARTED (timestamp: %lu ns) ===\n", timestamp);
    fprintf(forensic_log_file, "Forensic logging initialized successfully\n");
    fflush(forensic_log_file);
    
    printf("[FORENSIC] Log initialized successfully: %s\n", filename);
    return true;
}

// CORRECTION CRITIQUE #001: Initialiser logging forensique AVANT création LUMs
bool forensic_logger_init_individual_files(void) {
    // Générer nom de fichier avec timestamp unique
    char filename[256];
    uint64_t timestamp = lum_get_timestamp();
    snprintf(filename, sizeof(filename), 
             "logs/forensic/forensic_session_%llu_%llu.log",
             timestamp / 1000000000ULL, timestamp % 1000000000ULL);
    
    return forensic_logger_init(filename);
}

void forensic_log_memory_operation(const char* operation, void* ptr, size_t size) {
    if (!forensic_log_file) return;
    
    uint64_t timestamp = lum_get_timestamp();
    fprintf(forensic_log_file, "[%lu] MEMORY_%s: ptr=%p, size=%zu\n", 
            timestamp, operation, ptr, size);
    fflush(forensic_log_file);
}

void forensic_log_lum_operation(const char* operation, uint64_t lum_count, double duration_ns) {
    if (!forensic_log_file) return;
    
    uint64_t timestamp = lum_get_timestamp();
    fprintf(forensic_log_file, "[%lu] LUM_%s: count=%lu, duration=%.3f ns\n",
            timestamp, operation, lum_count, duration_ns);
    fflush(forensic_log_file);
    
    // NOUVEAU: Log détaillé pour chaque LUM individuel
    printf("[FORENSIC_REALTIME] LUM_%s: count=%lu at timestamp=%lu ns\n", 
           operation, lum_count, timestamp);
}

// FONCTION RENFORCÉE: Log systématique pour chaque LUM avec double écriture
void forensic_log_individual_lum(uint32_t lum_id, const char* operation, uint64_t timestamp_ns) {
    if (!forensic_log_file) {
        printf("[FORENSIC_ERROR] Log file not initialized for LUM_%u\n", lum_id);
        return;
    }
    
    // ÉCRITURE FICHIER: Log détaillé avec flush immédiat
    fprintf(forensic_log_file, "[%lu] [LUM_%u] %s: Individual LUM processing (memory=%p)\n",
            timestamp_ns, lum_id, operation, (void*)&lum_id);
    fflush(forensic_log_file);
    
    // ÉCRITURE CONSOLE: Affichage temps réel obligatoire
    printf("[FORENSIC_LUM] [%lu] LUM_%u %s\n", timestamp_ns, lum_id, operation);
    fflush(stdout);
    
    // NOUVEAU: Log dans fichier séparé horodaté
    static FILE* individual_log = NULL;
    if (!individual_log) {
        char individual_filename[256];
        time_t now = time(NULL);
        struct tm* tm_info = localtime(&now);
        snprintf(individual_filename, sizeof(individual_filename), 
                 "logs/forensic/individual_lums_%04d%02d%02d_%02d%02d%02d.log",
                 tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
                 tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
        individual_log = fopen(individual_filename, "w");
        if (individual_log) {
            fprintf(individual_log, "=== LOG INDIVIDUEL LUMs - SESSION %lu ===\n", timestamp_ns);
            fflush(individual_log);
        }
    }
    
    if (individual_log) {
        fprintf(individual_log, "[%lu] LUM_%u: %s\n", timestamp_ns, lum_id, operation);
        fflush(individual_log);
    }
}

void forensic_logger_destroy(void) {
    if (forensic_log_file) {
        uint64_t timestamp = lum_get_timestamp();
        fprintf(forensic_log_file, "=== FORENSIC LOG ENDED (timestamp: %lu ns) ===\n", timestamp);
        fclose(forensic_log_file);
        forensic_log_file = NULL;
    }
}

void forensic_log(forensic_level_e level, const char* function, const char* format, ...) {
    if (!forensic_log_file) return;
    
    uint64_t timestamp = lum_get_timestamp();
    va_list args;
    va_start(args, format);
    
    fprintf(forensic_log_file, "[%lu] [%d] %s: ", timestamp, level, function);
    vfprintf(forensic_log_file, format, args);
    fprintf(forensic_log_file, "\n");
    fflush(forensic_log_file);
    
    va_end(args);
}

// Implementation of unified_forensic_log for compatibility
void unified_forensic_log(unified_forensic_level_e level, const char* function, const char* format, ...) {
    if (!forensic_log_file) return;
    
    uint64_t timestamp = lum_get_timestamp();
    va_list args;
    va_start(args, format);
    
    fprintf(forensic_log_file, "[%lu] [UNIFIED_%d] %s: ", timestamp, level, function);
    vfprintf(forensic_log_file, format, args);
    fprintf(forensic_log_file, "\n");
    fflush(forensic_log_file);
    
    va_end(args);
}
