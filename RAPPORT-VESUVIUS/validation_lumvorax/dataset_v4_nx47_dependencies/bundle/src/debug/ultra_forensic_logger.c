// Feature test macros for POSIX functions
// _GNU_SOURCE defined in Makefile
#define _POSIX_C_SOURCE 200809L

#include "ultra_forensic_logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>
#include <pthread.h>

#define MAX_MODULES 50
#define LOG_BUFFER_SIZE 4096

static module_forensic_tracker_t g_module_trackers[MAX_MODULES];
static int g_tracker_count = 0;
static pthread_mutex_t g_global_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool g_forensic_initialized = false;

// Obtenir timestamp nanoseconde précis
static uint64_t get_precise_timestamp_ns(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
        return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
    }
    return 0;
}

// Créer structure de répertoires forensiques
static bool create_forensic_directories(void) {
    const char* directories[] = {
        "logs/forensic",
        "logs/forensic/modules", 
        "logs/forensic/tests",
        "logs/forensic/metrics",
        "logs/forensic/sessions"
    };

    for (size_t i = 0; i < sizeof(directories)/sizeof(directories[0]); i++) {
        struct stat st = {0};
        if (stat(directories[i], &st) == -1) {
            if (mkdir(directories[i], 0755) != 0) {
                return false;
            }
        }
    }

    return true;
}

bool ultra_forensic_logger_init(void) {
    if (g_forensic_initialized) {
        return true;
    }

    if (!create_forensic_directories()) {
        fprintf(stderr, "Erreur: Impossible de créer les répertoires forensiques\n");
        return false;
    }

    // Initialisation du mutex
    if (pthread_mutex_init(&g_global_mutex, NULL) != 0) {
        fprintf(stderr, "Erreur: Impossible d'initialiser le mutex forensique\n");
        return false;
    }

    g_forensic_initialized = true;

    // Écriture du header initial
    // Note: La fonction ultra_forensic_log_header n'est pas définie dans le code fourni, 
    // elle sera supposée exister ou doit être ajoutée. Ici, on la remplace par un printf.
    // Si elle existe dans "ultra_forensic_logger.h", il faudrait l'inclure et l'appeler correctement.
    // Pour l'instant, on utilise un printf comme substitut pour démontrer l'intention.
    printf("[ULTRA_FORENSIC] Système de logging forensique ultra-strict initialisé\n");
    // ultra_forensic_log_header("SYSTEM", "INIT", "Système forensique initialisé");

    return true;
}

void ultra_forensic_logger_destroy(void) {
    if (!g_forensic_initialized) return;

    pthread_mutex_lock(&g_global_mutex);

    // Fermeture de tous les fichiers de logs modules
    for (int i = 0; i < g_tracker_count; i++) {
        module_forensic_tracker_t* tracker = &g_module_trackers[i];
        if (tracker->module_log_file) {
            fclose(tracker->module_log_file);
            tracker->module_log_file = NULL;
        }
        pthread_mutex_destroy(&tracker->mutex);
    }

    g_tracker_count = 0;
    g_forensic_initialized = false;

    pthread_mutex_unlock(&g_global_mutex);
    pthread_mutex_destroy(&g_global_mutex);

    printf("[ULTRA_FORENSIC] Système de logging forensique arrêté proprement\n");
}

// Trouver ou créer tracker pour module
static module_forensic_tracker_t* get_or_create_module_tracker(const char* module) {
    pthread_mutex_lock(&g_global_mutex);

    // Chercher tracker existant
    for (int i = 0; i < g_tracker_count; i++) {
        if (strcmp(g_module_trackers[i].module_name, module) == 0) {
            pthread_mutex_unlock(&g_global_mutex);
            return &g_module_trackers[i];
        }
    }

    // Créer nouveau tracker
    if (g_tracker_count >= MAX_MODULES) {
        pthread_mutex_unlock(&g_global_mutex);
        return NULL;
    }

    module_forensic_tracker_t* tracker = &g_module_trackers[g_tracker_count++];
    strncpy(tracker->module_name, module, sizeof(tracker->module_name) - 1);
    tracker->module_name[sizeof(tracker->module_name) - 1] = '\0';
    tracker->memory_used = 0; // Initialiser memory_used

    // Créer fichier de log pour ce module
    char log_filename[256];
    uint64_t timestamp = get_precise_timestamp_ns();
    snprintf(log_filename, sizeof(log_filename), 
             "logs/forensic/modules/%s_forensic_%lu.log", module, timestamp);

    tracker->module_log_file = fopen(log_filename, "w");
    if (!tracker->module_log_file) {
        fprintf(stderr, "[ULTRA_FORENSIC] ERREUR: Impossible de créer log pour module %s\n", module);
        g_tracker_count--;
        pthread_mutex_unlock(&g_global_mutex);
        return NULL;
    }

    pthread_mutex_init(&tracker->mutex, NULL);

    // Écrire header forensique
    fprintf(tracker->module_log_file, 
            "=== LOG FORENSIQUE ULTRA-STRICT MODULE %s ===\n", module);
    fprintf(tracker->module_log_file, 
            "Timestamp début: %lu ns\n", timestamp);
    fprintf(tracker->module_log_file, 
            "Processus: %d\n", getpid());
    fprintf(tracker->module_log_file, 
            "Standards: ISO/IEC 27037, NIST SP 800-86, IEEE 1012\n");
    fprintf(tracker->module_log_file, 
            "================================================\n");
    fflush(tracker->module_log_file);

    pthread_mutex_unlock(&g_global_mutex);
    return tracker;
}

void ultra_forensic_log_module_start(const char* file, int line, const char* func, 
                                   const char* module, const char* test) {
    if (!g_forensic_initialized) return;

    module_forensic_tracker_t* tracker = get_or_create_module_tracker(module);
    if (!tracker) return;

    pthread_mutex_lock(&tracker->mutex);

    tracker->start_timestamp_ns = get_precise_timestamp_ns();
    strncpy(tracker->test_name, test, sizeof(tracker->test_name) - 1);
    tracker->test_name[sizeof(tracker->test_name) - 1] = '\0';

    fprintf(tracker->module_log_file,
            "[%lu] TEST_START: %s\n", tracker->start_timestamp_ns, test);
    fprintf(tracker->module_log_file,
            "  Source: %s:%d in %s()\n", file, line, func);
    fprintf(tracker->module_log_file,
            "  Module: %s\n", module);
    fflush(tracker->module_log_file);

    pthread_mutex_unlock(&tracker->mutex);
}

void ultra_forensic_log_module_end(const char* file, int line, const char* func,
                                 const char* module, const char* test, bool success) {
    if (!g_forensic_initialized) return;

    module_forensic_tracker_t* tracker = get_or_create_module_tracker(module);
    if (!tracker) return;

    pthread_mutex_lock(&tracker->mutex);

    tracker->end_timestamp_ns = get_precise_timestamp_ns();
    uint64_t duration_ns = tracker->end_timestamp_ns - tracker->start_timestamp_ns;

    fprintf(tracker->module_log_file,
            "[%lu] TEST_END: %s\n", tracker->end_timestamp_ns, test);
    fprintf(tracker->module_log_file,
            "  Résultat: %s\n", success ? "SUCCÈS" : "ÉCHEC");
    fprintf(tracker->module_log_file,
            "  Durée: %lu ns (%.3f ms)\n", duration_ns, duration_ns / 1000000.0);
    fprintf(tracker->module_log_file,
            "  Source: %s:%d in %s()\n", file, line, func);
    fflush(tracker->module_log_file);

    pthread_mutex_unlock(&tracker->mutex);
}

void ultra_forensic_log_module_operation(const char* file, int line, const char* func,
                                        const char* module, const char* operation, const char* data) {
    if (!g_forensic_initialized) return;

    module_forensic_tracker_t* tracker = get_or_create_module_tracker(module);
    if (!tracker) return;

    pthread_mutex_lock(&tracker->mutex);

    uint64_t timestamp = get_precise_timestamp_ns();
    tracker->operations_count++;

    fprintf(tracker->module_log_file,
            "[%lu] OPERATION: %s\n", timestamp, operation);
    fprintf(tracker->module_log_file,
            "  Données: %s\n", data);
    fprintf(tracker->module_log_file,
            "  Opération #%lu\n", tracker->operations_count);
    fprintf(tracker->module_log_file,
            "  Source: %s:%d in %s()\n", file, line, func);
    fflush(tracker->module_log_file);

    // LOG TEMPS RÉEL AVEC AFFICHAGE CONSOLE
    char realtime_filename[512];
    snprintf(realtime_filename, sizeof(realtime_filename), 
             "logs/temps_reel/execution/%s_operation_%lu.log", module, timestamp);

    FILE* realtime_file = fopen(realtime_filename, "w");
    if (realtime_file) {
        fprintf(realtime_file, "[%lu] %s: %s\n", timestamp, module, operation);
        fprintf(realtime_file, "Data: %s\n", data);
        fprintf(realtime_file, "Source: %s:%d\n", file, line);
        fflush(realtime_file);
        fclose(realtime_file);

        printf("[%lu] LOG TEMPS REEL: %s\n", timestamp, realtime_filename);
        fflush(stdout);
    }

    pthread_mutex_unlock(&tracker->mutex);
}

void ultra_forensic_log_module_metric(const char* file, int line, const char* func,
                                     const char* module, const char* metric_name, double value) {
    if (!g_forensic_initialized) return;

    module_forensic_tracker_t* tracker = get_or_create_module_tracker(module);
    if (!tracker) return;

    pthread_mutex_lock(&tracker->mutex);

    uint64_t timestamp = get_precise_timestamp_ns();

    fprintf(tracker->module_log_file,
            "[%lu] MÉTRIQUE: %s = %.6f\n", timestamp, metric_name, value);
    fprintf(tracker->module_log_file,
            "  Source: %s:%d in %s()\n", file, line, func);
    fflush(tracker->module_log_file);

    // Log séparé pour métriques
    char metrics_filename[256];
    snprintf(metrics_filename, sizeof(metrics_filename), 
             "logs/forensic/metrics/%s_metrics.log", module);
    FILE* metrics_file = fopen(metrics_filename, "a");
    if (metrics_file) {
        fprintf(metrics_file, "%lu,%s,%.6f\n", timestamp, metric_name, value);
        fclose(metrics_file);
    }

    pthread_mutex_unlock(&tracker->mutex);
}

bool ultra_forensic_validate_all_logs_exist(void) {
    char validation_report[4096];
    snprintf(validation_report, sizeof(validation_report), 
             "logs/forensic/validation/logs_validation_%lu.txt", get_precise_timestamp_ns());

    FILE* validation_file = fopen(validation_report, "w");
    if (!validation_file) {
        fprintf(stderr, "Erreur: Impossible de créer le rapport de validation\n");
        return false;
    }

    fprintf(validation_file, "=== VALIDATION LOGS FORENSIQUES ULTRA-STRICTS ===\n");
    fprintf(validation_file, "Timestamp: %lu\n", get_precise_timestamp_ns());
    fprintf(validation_file, "Modules tracés: %d\n", g_tracker_count);

    bool all_logs_valid = true;
    for (int i = 0; i < g_tracker_count; i++) {
        module_forensic_tracker_t* tracker = &g_module_trackers[i];

        // Vérifier que le fichier de log existe et contient des données
        if (tracker->module_log_file) {
            fseek(tracker->module_log_file, 0, SEEK_END);
            long file_size = ftell(tracker->module_log_file);
            fseek(tracker->module_log_file, 0, SEEK_SET);

            fprintf(validation_file, "Module %s: ", tracker->module_name);
            if (file_size > 0) {
                fprintf(validation_file, "VALIDE (%ld bytes)\n", file_size);
            } else {
                fprintf(validation_file, "INVALIDE (fichier vide)\n");
                all_logs_valid = false;
            }
        } else {
            fprintf(validation_file, "Module %s: INVALIDE (fichier manquant)\n", tracker->module_name);
            all_logs_valid = false;
        }
    }

    fprintf(validation_file, "\nRésultat global: %s\n", all_logs_valid ? "TOUS LOGS VALIDES" : "ERREURS DÉTECTÉES");
    fclose(validation_file);

    return all_logs_valid;
}

void ultra_forensic_generate_summary_report(void) {
    char summary_path[512];
    snprintf(summary_path, sizeof(summary_path), 
             "logs/forensic/sessions/summary_%lu.txt", get_precise_timestamp_ns());

    FILE* summary_file = fopen(summary_path, "w");
    if (!summary_file) return;

    fprintf(summary_file, "=== RAPPORT RÉSUMÉ FORENSIQUE ULTRA-STRICT ===\n");
    fprintf(summary_file, "Timestamp génération: %lu\n", get_precise_timestamp_ns());
    fprintf(summary_file, "Modules totaux tracés: %d\n", g_tracker_count);
    fprintf(summary_file, "\nDétails par module:\n");

    for (int i = 0; i < g_tracker_count; i++) {
        module_forensic_tracker_t* tracker = &g_module_trackers[i];
        fprintf(summary_file, "- %s: %lu opérations, %zu bytes mémoire\n", 
                tracker->module_name, tracker->operations_count, tracker->memory_used);
    }

    fprintf(summary_file, "\n=== FIN RAPPORT RÉSUMÉ ===\n");
    fclose(summary_file);

    printf("📄 Rapport résumé généré: %s\n", summary_path);
}