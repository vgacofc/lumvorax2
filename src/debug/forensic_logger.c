
/* **************************************************************************
** forensic_logger.c — Logger forensic bit-level nanoseconde
**
** Projet : LumVorax (Autonomous Reflexive Temporal Cognitive Engine)
** Module : src/debug / forensic_logger
** Auteur : LumVorax Project
**
** FORENSIC-UNIF-003 — corrections appliquées :
**   BUG-1 FIX : lum_id uint32_t → uint64_t (corrige troncature 64→32)
**   BUG-3 FIX : horloge header = CLOCK_MONOTONIC (idem événements)
**              séparation explicite horloge monotone / horloge civile
**   PERF-1 FIX : batch flush toutes les FLUSH_BATCH_SIZE écritures
**               (au lieu d'un fflush() par événement)
**   NEW : compteur event_seq global monotone (uint64_t atomique sous mutex)
**         détection de pertes/duplications par analyse du log
**
** CERTIFIED_100=false | unique_human_proven=false
** Mode DEBUG actif
** ************************************************************************ */

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
#include <sys/stat.h>   /* Pour mkdir() */
#include <unistd.h>     /* Pour access() */
#include <errno.h>      /* Pour errno */
#include <pthread.h>    /* FL-001 FIX: mutex pour individual_log */
#include <inttypes.h>   /* FL-001 FIX: PRIu64 pour timestamp_ns (uint64_t) */

/* ── PERF-1 FIX : taille du batch flush ─────────────────────────────────────
 * fflush() est appelé une fois toutes les FLUSH_BATCH_SIZE écritures au lieu
 * d'une fois par événement. En cas d'arrêt brutal (kill/crash), au maximum
 * FLUSH_BATCH_SIZE-1 événements peuvent être perdus. Valeur choisie : 1024.
 */
#define FLUSH_BATCH_SIZE 1024U

static FILE* forensic_log_file = NULL;

/* ── FORENSIC-UNIF-003 : compteur séquentiel global ─────────────────────────
 * event_seq est incrémenté de façon atomique (sous fl001_log_file_mutex) à
 * chaque appel à forensic_log_individual_lum(). Il est écrit dans le log :
 *   [ts_ns] [seq=N] [lum_id=0xXXXX...] ...
 * Un programme d'analyse peut ainsi détecter des gaps (pertes) ou des
 * doublons (duplications) dans le fichier de log.
 */
static uint64_t g_event_seq = 0;

/* ── PERF-1 : compteur d'événements pour le batch flush ─────────────────── */
static uint64_t g_flush_counter = 0;

/* FL-001 FIX: mutex unique pour tout accès à individual_log */
static pthread_mutex_t fl001_individual_mutex = PTHREAD_MUTEX_INITIALIZER;

/* FL-001 FIX v2: mutex global pour forensic_log_file — protège toutes les
 * fonctions écrivant dans forensic_log_file (forensic_log_memory_operation,
 * forensic_log_lum_operation, forensic_log, unified_forensic_log,
 * forensic_logger_init, forensic_logger_destroy). Sans ce verrou, deux threads
 * peuvent entremêler leurs écritures ou accéder à forensic_log_file pendant
 * qu'un autre thread le ferme/réinitialise. */
static pthread_mutex_t fl001_log_file_mutex = PTHREAD_MUTEX_INITIALIZER;

/* ── BUG-3 FIX : fonctions d'horloge publiques ──────────────────────────── */

uint64_t forensic_get_monotonic_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

uint64_t forensic_get_event_seq(void)
{
    pthread_mutex_lock(&fl001_log_file_mutex);
    uint64_t seq = g_event_seq;
    pthread_mutex_unlock(&fl001_log_file_mutex);
    return seq;
}

bool forensic_logger_init(const char* filename) {
    if (!filename) {
        fprintf(stderr, "[FORENSIC] ERROR: filename is NULL\n");
        return false;
    }

    /* Créer répertoire si nécessaire avec vérification complète */
    char dir_path[256];
    strncpy(dir_path, filename, sizeof(dir_path) - 1);
    dir_path[sizeof(dir_path) - 1] = '\0';

    char *last_slash = strrchr(dir_path, '/');
    if (last_slash) {
        *last_slash = '\0';

        /* Créer récursivement tous les répertoires parents */
        char temp_path[256];
        char *token = strtok(dir_path, "/");
        temp_path[0] = '\0';

        while (token != NULL) {
            strncat(temp_path, token, sizeof(temp_path) - strlen(temp_path) - 1);
            strncat(temp_path, "/", sizeof(temp_path) - strlen(temp_path) - 1);
            mkdir(temp_path, 0755);
            token = strtok(NULL, "/");
        }
    }

    /* BUG-3 FIX : le header utilise CLOCK_MONOTONIC (même source que les événements).
     * L'horloge civile (CLOCK_REALTIME) est également journalisée à titre informatif
     * pour permettre la corrélation externe — les deux valeurs sont clairement étiquetées. */
    pthread_mutex_lock(&fl001_log_file_mutex);

    /* Réinitialiser les compteurs au démarrage d'une nouvelle session */
    g_event_seq    = 0;
    g_flush_counter = 0;

    forensic_log_file = fopen(filename, "w");
    if (!forensic_log_file) {
        /* Fallback vers répertoire courant */
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

    /* BUG-3 FIX : header utilise CLOCK_MONOTONIC pour cohérence avec les événements.
     * ts_monotonic_ns = horloge utilisée pour event_seq et les durées.
     * ts_realtime_ns  = horloge civile, pour corrélation externe uniquement. */
    uint64_t ts_mono = forensic_get_monotonic_ns();
    struct timespec tsr;
    clock_gettime(CLOCK_REALTIME, &tsr);
    uint64_t ts_real = (uint64_t)tsr.tv_sec * 1000000000ULL + (uint64_t)tsr.tv_nsec;

    fprintf(forensic_log_file,
            "=== FORENSIC LOG STARTED ===\n"
            "ts_monotonic_ns=%" PRIu64 " ts_realtime_ns=%" PRIu64 "\n"
            "flush_batch_size=%u event_seq_start=0\n"
            "format: [ts_ns] [seq=N] [lum_id=0xXXXX...] op_name\n",
            ts_mono, ts_real, FLUSH_BATCH_SIZE);
    fflush(forensic_log_file);
    pthread_mutex_unlock(&fl001_log_file_mutex);

    printf("[FORENSIC] Log initialized successfully: %s\n", filename);
    printf("[FORENSIC] BUG-3 FIX: header monotonic=%" PRIu64 " ns | realtime=%" PRIu64 " ns\n",
           ts_mono, ts_real);
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

/* FORENSIC-UNIF-003 : forensic_log_individual_lum corrigé
 *
 * BUG-1 FIX : lum_id est maintenant uint64_t — aucune troncature 64→32.
 * PERF-1 FIX : batch flush toutes les FLUSH_BATCH_SIZE écritures.
 * NEW      : event_seq global monotone incrémenté sous mutex — chaque entrée
 *             du log porte son numéro de séquence unique pour détection de
 *             pertes et duplications.
 *
 * FL-005 FIX v2 conservé : fl001_log_file_mutex maintenu pendant toute l'écriture
 * (pas de relâche entre vérification et fflush conditionnelle).
 * Ordre d'acquisition strict : fl001_log_file_mutex TOUJOURS avant
 * fl001_individual_mutex — jamais l'inverse. */
void forensic_log_individual_lum(uint64_t lum_id, const char* operation, uint64_t timestamp_ns) {
    pthread_mutex_lock(&fl001_log_file_mutex);
    if (!forensic_log_file) {
        pthread_mutex_unlock(&fl001_log_file_mutex);
        /* PERF : pas de printf par événement manqué en production — stderr seulement */
        fprintf(stderr, "[FORENSIC_ERROR] Log file not initialized for LUM_0x%016" PRIx64 "\n",
                lum_id);
        return;
    }

    /* BUG-1 FIX : lum_id est écrit en hexadécimal 64 bits complet.
     * NEW : event_seq incrémenté sous mutex, écrit dans le log.
     * Format : [ts_ns] [seq=N] [lum_id=0xXXXXXXXXXXXXXXXX] op_name */
    uint64_t seq = ++g_event_seq;
    fprintf(forensic_log_file,
            "[%" PRIu64 "] [seq=%" PRIu64 "] [lum_id=0x%016" PRIx64 "] %s\n",
            timestamp_ns, seq, lum_id, operation);

    /* PERF-1 FIX : flush conditionnel — une fois toutes les FLUSH_BATCH_SIZE écritures.
     * Le flush final est garanti par forensic_logger_destroy(). */
    g_flush_counter++;
    if (g_flush_counter % FLUSH_BATCH_SIZE == 0) {
        fflush(forensic_log_file);
    }

    pthread_mutex_unlock(&fl001_log_file_mutex);

    /* FL-001 FIX : accès à individual_log entièrement sous fl001_individual_mutex.
     * Ordre d'acquisition strict respecté : fl001_log_file_mutex déjà relâché. */
    pthread_mutex_lock(&fl001_individual_mutex);

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
            /* BUG-1 FIX : header individual_log avec même format lum_id 64 bits */
            fprintf(individual_log,
                    "=== LOG INDIVIDUEL LUMs - SESSION ts=%" PRIu64 " ===\n"
                    "format: [ts_ns] [seq=N] [lum_id=0xXXXX...] op\n",
                    timestamp_ns);
            fflush(individual_log);
        }
    }

    if (individual_log) {
        /* BUG-1 FIX : lum_id 64 bits complet dans le log individuel */
        fprintf(individual_log,
                "[%" PRIu64 "] [seq=%" PRIu64 "] [lum_id=0x%016" PRIx64 "] %s\n",
                timestamp_ns, seq, lum_id, operation);
        /* PERF-1 FIX : pas de fflush par événement dans individual_log non plus */
    }

    pthread_mutex_unlock(&fl001_individual_mutex);
}

void forensic_logger_destroy(void) {
    pthread_mutex_lock(&fl001_log_file_mutex);
    if (forensic_log_file) {
        /* BUG-3 FIX : footer utilise CLOCK_MONOTONIC (cohérent avec le header) */
        uint64_t ts_mono = forensic_get_monotonic_ns();
        struct timespec tsr;
        clock_gettime(CLOCK_REALTIME, &tsr);
        uint64_t ts_real = (uint64_t)tsr.tv_sec * 1000000000ULL + (uint64_t)tsr.tv_nsec;

        fprintf(forensic_log_file,
                "=== FORENSIC LOG ENDED ===\n"
                "ts_monotonic_ns=%" PRIu64 " ts_realtime_ns=%" PRIu64 "\n"
                "total_events=%" PRIu64 "\n",
                ts_mono, ts_real, g_event_seq);
        /* Flush final garanti (PERF-1 : flush des événements en attente dans le batch) */
        fflush(forensic_log_file);
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
