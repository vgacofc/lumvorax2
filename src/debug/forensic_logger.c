
/* FL-001 FIX: individual_log est une variable statique locale dans
 * forensic_log_individual_lum(). Sans protection mutex, deux threads
 * appelant simultanément cette fonction peuvent ouvrir le fichier deux
 * fois (double-init) ou écrire de façon entrelacée (données corrompues).
 * Solution : mutex statique fl001_individual_mutex protège l'ouverture
 * ET chaque écriture sur individual_log. */
#include "forensic_logger.h"
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>   // Pour mkdir()
#include <unistd.h>     // Pour access()
#include <errno.h>      // Pour errno
#include <pthread.h>    /* FL-001 FIX: mutex pour individual_log */
#include <inttypes.h>   /* FL-001 FIX: PRIu64 pour timestamp_ns (uint64_t) */

static FILE* forensic_log_file = NULL;

/* FL-001 FIX: mutex unique pour tout accès à individual_log */
static pthread_mutex_t fl001_individual_mutex = PTHREAD_MUTEX_INITIALIZER;

/* FL-001 FIX v2: mutex global pour forensic_log_file — protège toutes les
 * fonctions écrivant dans forensic_log_file (forensic_log_memory_operation,
 * forensic_log_lum_operation, forensic_log, unified_forensic_log,
 * forensic_logger_init, forensic_logger_destroy). Sans ce verrou, deux threads
 * peuvent entremêler leurs écritures ou accéder à forensic_log_file pendant
 * qu'un autre thread le ferme/réinitialise. */
static pthread_mutex_t fl001_log_file_mutex = PTHREAD_MUTEX_INITIALIZER;

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
    pthread_mutex_lock(&fl001_log_file_mutex);
    forensic_log_file = fopen(filename, "w");
    if (!forensic_log_file) {
        // Fallback vers répertoire courant
        char fallback_name[256];
        snprintf(fallback_name, sizeof(fallback_name), "forensic_fallback_%lu.log",
                 (unsigned long)time(NULL));
        
        forensic_log_file = fopen(fallback_name, "w");
        if (!forensic_log_file) {
            pthread_mutex_unlock(&fl001_log_file_mutex);
            fprintf(stderr, "[FORENSIC] CRITICAL: Cannot create any log file\n");
            return false;
        }
        
        fprintf(stderr, "[FORENSIC] WARNING: Using fallback log: %s\n", fallback_name);
    }
    
    uint64_t timestamp = lum_get_timestamp();
    fprintf(forensic_log_file, "=== FORENSIC LOG STARTED (timestamp: %llu ns) ===\n", timestamp);
    fprintf(forensic_log_file, "Forensic logging initialized successfully\n");
    fflush(forensic_log_file);
    pthread_mutex_unlock(&fl001_log_file_mutex);
    
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
    pthread_mutex_lock(&fl001_log_file_mutex);
    if (!forensic_log_file) { pthread_mutex_unlock(&fl001_log_file_mutex); return; }
    
    uint64_t timestamp = lum_get_timestamp();
    fprintf(forensic_log_file, "[%llu] MEMORY_%s: ptr=%p, size=%zu\n",
            timestamp, operation, ptr, size);
    fflush(forensic_log_file);
    pthread_mutex_unlock(&fl001_log_file_mutex);
}

void forensic_log_lum_operation(const char* operation, uint64_t lum_count, double duration_ns) {
    pthread_mutex_lock(&fl001_log_file_mutex);
    if (!forensic_log_file) { pthread_mutex_unlock(&fl001_log_file_mutex); return; }
    
    uint64_t timestamp = lum_get_timestamp();
    fprintf(forensic_log_file, "[%llu] LUM_%s: count=%llu, duration=%.3f ns\n",
            timestamp, operation, lum_count, duration_ns);
    fflush(forensic_log_file);
    pthread_mutex_unlock(&fl001_log_file_mutex);
    
    // NOUVEAU: Log détaillé pour chaque LUM individuel
    printf("[FORENSIC_REALTIME] LUM_%s: count=%llu at timestamp=%llu ns\n",
           operation, lum_count, timestamp);
}

// FONCTION RENFORCÉE: Log systématique pour chaque LUM avec double écriture
void forensic_log_individual_lum(uint32_t lum_id, const char* operation, uint64_t timestamp_ns) {
    /* FL-005 FIX v2 (rapport 149) : maintien du fl001_log_file_mutex pendant TOUTE
     * la durée de l'écriture dans forensic_log_file (Option A rapport 148 §4).
     * La correction session 147 copiait le FILE* puis relâchait le mutex AVANT
     * d'écrire — forensic_logger_destroy() pouvait appeler fclose() sur ce même
     * FILE* entre la copie et l'écriture (use-after-close).
     * Ici le mutex n'est jamais relâché entre la vérification et le fflush final,
     * donc forensic_logger_destroy() doit attendre la fin de l'écriture complète.
     * Ordre d'acquisition strict : fl001_log_file_mutex TOUJOURS avant
     * fl001_individual_mutex — jamais l'inverse. */
    pthread_mutex_lock(&fl001_log_file_mutex);
    if (!forensic_log_file) {
        pthread_mutex_unlock(&fl001_log_file_mutex);
        printf("[FORENSIC_ERROR] Log file not initialized for LUM_%u\n", lum_id);
        return;
    }

    /* FL-002 FIX: &lum_id était l'adresse d'une variable locale (stack), pas l'adresse
     * du LUM en mémoire — log forensique trompeur. On supprime ce champ sans valeur. */
    fprintf(forensic_log_file, "[%" PRIu64 "] [LUM_%u] %s: Individual LUM processing\n",
            timestamp_ns, lum_id, operation);
    fflush(forensic_log_file);
    pthread_mutex_unlock(&fl001_log_file_mutex);

    // ÉCRITURE CONSOLE: Affichage temps réel obligatoire (hors mutex — stdout ne dépend pas du FILE*)
    printf("[FORENSIC_LUM] [%" PRIu64 "] LUM_%u %s\n", timestamp_ns, lum_id, operation);
    fflush(stdout);

    /* FL-001 FIX: accès à individual_log entièrement sous fl001_individual_mutex.
     * Élimine la double-initialisation et les écritures entrelacées en cas
     * d'appels multi-thread simultanés.
     * Ordre d'acquisition strict respecté : fl001_log_file_mutex déjà relâché
     * ci-dessus avant d'acquérir fl001_individual_mutex. */
    pthread_mutex_lock(&fl001_individual_mutex);

    /* individual_log promu en variable statique de fichier (fl001_*) pour
     * que le mutex externe puisse la protéger. */
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
            fprintf(individual_log, "=== LOG INDIVIDUEL LUMs - SESSION %" PRIu64 " ===\n",
                    timestamp_ns);
            fflush(individual_log);
        }
    }

    if (individual_log) {
        fprintf(individual_log, "[%" PRIu64 "] LUM_%u: %s\n",
                timestamp_ns, lum_id, operation);
        fflush(individual_log);
    }

    pthread_mutex_unlock(&fl001_individual_mutex);
}

void forensic_logger_destroy(void) {
    pthread_mutex_lock(&fl001_log_file_mutex);
    if (forensic_log_file) {
        uint64_t timestamp = lum_get_timestamp();
        fprintf(forensic_log_file, "=== FORENSIC LOG ENDED (timestamp: %llu ns) ===\n", timestamp);
        fclose(forensic_log_file);
        forensic_log_file = NULL;
    }
    pthread_mutex_unlock(&fl001_log_file_mutex);
}

void forensic_log(forensic_level_e level, const char* function, const char* format, ...) {
    pthread_mutex_lock(&fl001_log_file_mutex);
    if (!forensic_log_file) { pthread_mutex_unlock(&fl001_log_file_mutex); return; }
    
    uint64_t timestamp = lum_get_timestamp();
    va_list args;
    va_start(args, format);
    
    fprintf(forensic_log_file, "[%llu] [%d] %s: ", timestamp, level, function);
    vfprintf(forensic_log_file, format, args);
    fprintf(forensic_log_file, "\n");
    fflush(forensic_log_file);
    
    va_end(args);
    pthread_mutex_unlock(&fl001_log_file_mutex);
}

// Implementation of unified_forensic_log for compatibility
void unified_forensic_log(unified_forensic_level_e level, const char* function, const char* format, ...) {
    pthread_mutex_lock(&fl001_log_file_mutex);
    if (!forensic_log_file) { pthread_mutex_unlock(&fl001_log_file_mutex); return; }
    
    uint64_t timestamp = lum_get_timestamp();
    va_list args;
    va_start(args, format);
    
    fprintf(forensic_log_file, "[%llu] [UNIFIED_%d] %s: ", timestamp, level, function);
    vfprintf(forensic_log_file, format, args);
    fprintf(forensic_log_file, "\n");
    fflush(forensic_log_file);
    
    va_end(args);
    pthread_mutex_unlock(&fl001_log_file_mutex);
}
