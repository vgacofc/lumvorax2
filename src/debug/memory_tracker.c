// NOTE: _GNU_SOURCE déjà défini dans Makefile
#include "memory_tracker.h"
#include <pthread.h>
#include <string.h> // Added for strncpy
#include <stdlib.h> // Added for abort() and free()
#include <time.h>   // Added for time()
#include <stdio.h>  // Added for printf()

#include "../logger/lum_logger.h" // Include for lum_log function

// Global tracking variables - removing unused duplicates
static size_t g_count = 0; // Current number of active allocations
static size_t g_total_allocated = 0; // Total bytes ever allocated
static size_t g_total_freed = 0; // Total bytes ever freed
static bool g_tracking_enabled = true; // Flag to enable/disable tracking
static bool g_release_mode = false; // Flag for release mode

void memory_tracker_enable(bool enable) {
    g_tracking_enabled = enable;
}

bool memory_tracker_is_enabled(void) {
    return g_tracking_enabled && !g_release_mode;
}

void memory_tracker_set_release_mode(bool mode) {
    g_release_mode = mode;
    if (mode) {
        g_tracking_enabled = false;  // Désactive en mode release
    }
}


void memory_tracker_export_json(const char* filename) {
    if (!memory_tracker_is_enabled()) return;

    FILE* fp = fopen(filename, "w");
    if (!fp) {
        fprintf(stderr, "[MEMORY_TRACKER] ERROR: Could not open file %s for writing.\n", filename);
        return;
    }

    fprintf(fp, "{\n");
    fprintf(fp, "  \"total_allocated\": %zu,\n", g_total_allocated);
    fprintf(fp, "  \"total_freed\": %zu,\n", g_total_freed);
    fprintf(fp, "  \"current_allocations\": %zu,\n", g_count);
    fprintf(fp, "  \"leak_detection\": %s\n", (g_total_allocated > g_total_freed) ? "true" : "false");
    fprintf(fp, "}\n");

    fclose(fp);
}

static memory_tracker_t g_tracker = {0};
/* MT-002 FIX: un seul mutex g_tracker_mutex protège l'intégralité de g_tracker.
 * allocation_mutex supprimé — il créait une race condition avec g_tracker_mutex. */
static pthread_mutex_t g_tracker_mutex = PTHREAD_MUTEX_INITIALIZER;
static int g_tracker_initialized = 0;
static uint64_t g_global_generation = 1;  /* Compteur génération global pour anti-ABA */

void memory_tracker_init(void) {
    pthread_mutex_lock(&g_tracker_mutex);
    if (!g_tracker_initialized) {
        memset(&g_tracker, 0, sizeof(memory_tracker_t));
        g_tracker_initialized = 1;
        printf("[MEMORY_TRACKER] Initialized - tracking enabled\n");
    }
    pthread_mutex_unlock(&g_tracker_mutex);
}

size_t memory_tracker_get_current_usage(void) {
    if (!g_tracker_initialized) return 0;
    pthread_mutex_lock(&g_tracker_mutex);
    size_t current = g_tracker.current_usage;
    pthread_mutex_unlock(&g_tracker_mutex);
    return current;
}

static int find_entry(void* ptr) {
    // CORRECTION: Chercher l'entrée active la plus récente pour ce pointeur
    int latest_index = -1;
    uint64_t latest_generation = 0;

    for (size_t i = 0; i < g_tracker.count; i++) {
        if (g_tracker.entries[i].ptr == ptr && !g_tracker.entries[i].is_freed) {
            if (g_tracker.entries[i].generation > latest_generation) {
                latest_generation = g_tracker.entries[i].generation;
                latest_index = (int)i;
            }
        }
    }
    return latest_index;
}

static void add_entry(void* ptr, size_t size, const char* file, int line, const char* func) {
    // CORRECTION: Chercher si on peut réutiliser une entrée libérée avec même adresse
    int reuse_index = -1;
    for (size_t i = 0; i < g_tracker.count; i++) {
        if (g_tracker.entries[i].ptr == ptr && g_tracker.entries[i].is_freed) {
            reuse_index = (int)i;
            break;
        }
    }

    memory_entry_t* entry;
    if (reuse_index >= 0) {
        // CORRECTION: Réutiliser entrée existante avec nouvelle génération
        entry = &g_tracker.entries[reuse_index];
    } else {
        // Créer nouvelle entrée
        if (g_tracker.count >= MAX_MEMORY_ENTRIES) {
            printf("[MEMORY_TRACKER] WARNING: Max entries reached!\n");
            return;
        }
        entry = &g_tracker.entries[g_tracker.count];
        g_tracker.count++;
    }

    // CORRECTION: Réinitialiser complètement l'entrée avec nouvelle génération
    entry->ptr = ptr;
    entry->size = size;
    entry->file = file;
    entry->line = line;
    entry->function = func;
    entry->allocated_time = time(NULL);
    entry->is_freed = 0;
    entry->freed_time = 0;
    entry->freed_file = NULL;
    entry->freed_line = 0;
    entry->freed_function = NULL;
    entry->generation = g_global_generation++;  // CORRECTION: Nouvelle génération

    g_tracker.total_allocated += size;
    g_tracker.current_usage += size;

    if (g_tracker.current_usage > g_tracker.peak_usage) {
        g_tracker.peak_usage = g_tracker.current_usage;
    }

    printf("[MEMORY_TRACKER] ALLOC: %p (%zu bytes) at %s:%d in %s()\n",
           ptr, size, file, line, func);
}

void* tracked_malloc(size_t size, const char* file, int line, const char* func) {
    if (!g_tracker_initialized) memory_tracker_init();
    if (!memory_tracker_is_enabled()) return malloc(size);

    /* MT-002 FIX: utiliser g_tracker_mutex (mutex unique) comme tracked_calloc
     * pour éviter la race condition entre les deux mutex sur le même état g_tracker. */
    pthread_mutex_lock(&g_tracker_mutex);

    void* ptr = malloc(size);
    if (!ptr) {
        pthread_mutex_unlock(&g_tracker_mutex);
        return NULL;
    }

    // CORRECTION CRITIQUE: Vérifier réutilisation d'adresse seulement si problématique
    bool address_reused = false;
    for (size_t i = 0; i < g_tracker.count; i++) {
        if (g_tracker.entries[i].ptr == ptr && !g_tracker.entries[i].is_freed) {
            // Vérifier si c'est une réallocation rapide suspecte (< 1 seconde)
            time_t current_time = time(NULL);
            if (current_time - g_tracker.entries[i].allocated_time < 1) {
                printf("[MEMORY_TRACKER] CRITICAL: Rapid address reuse detected %p\n", ptr);
                printf("[MEMORY_TRACKER] Previous allocation at %s:%d in %s() %ld seconds ago\n",
                       g_tracker.entries[i].file, g_tracker.entries[i].line, 
                       g_tracker.entries[i].function, current_time - g_tracker.entries[i].allocated_time);
                address_reused = true;
            }

            // Marquer l'ancienne entrée comme recyclée par le système
            g_tracker.entries[i].is_freed = 1;
            g_tracker.entries[i].freed_time = current_time;
            g_tracker.entries[i].freed_file = "SYSTEM_RECYCLED";
            g_tracker.entries[i].freed_line = 0;
            g_tracker.entries[i].freed_function = "auto_recycled";
            break;
        }
    }

    if (address_reused) {
        printf("[MEMORY_TRACKER] WARNING: System allocator reused address rapidly\n");
    }

    add_entry(ptr, size, file, line, func);
    pthread_mutex_unlock(&g_tracker_mutex);
    return ptr;
}

// Modified tracked_free function with double-free protection
void tracked_free(void* ptr, const char* file, int line, const char* func) {
    if (!ptr) {
        return;
    }
    if (!g_tracker_initialized) {
        printf("[MEMORY_TRACKER] WARNING: Auto-initializing tracker at %s:%d\n", file, line);
        memory_tracker_init();
    }
    if (!memory_tracker_is_enabled()) {
        free(ptr);
        return;
    }

    /* MT-002 FIX: même mutex unique g_tracker_mutex pour tracked_free */
    pthread_mutex_lock(&g_tracker_mutex);

    // CORRECTION CRITIQUE: Validation intégrité avant libération
    int found_entry_idx = -1;
    for (size_t i = 0; i < g_tracker.count; i++) {
        if (g_tracker.entries[i].ptr == ptr) {
            found_entry_idx = (int)i;
            break;
        }
    }

    if (found_entry_idx == -1) {
        /* MT-003 FIX: un pointeur non tracké peut venir d'un module alloué avant
         * l'initialisation du tracker (ex: module externe, init précoce).
         * Politique documentée : journaliser l'anomalie + libérer proprement
         * sans crasher. En build forensic strict, passer -DTRACKER_STRICT_ABORT=1. */
        fprintf(stderr, "[MEMORY_TRACKER] WARNING: Free of untracked pointer %p at %s:%d in %s()\n",
               ptr, file, line, func);
        fprintf(stderr, "[MEMORY_TRACKER] Pointer may have been allocated before tracker init.\n");
        fprintf(stderr, "[MEMORY_TRACKER] Freeing safely without abort (non-strict mode).\n");
#ifdef TRACKER_STRICT_ABORT
        pthread_mutex_unlock(&g_tracker_mutex);
        abort();
#else
        pthread_mutex_unlock(&g_tracker_mutex);
        free(ptr);  /* Libération sécurisée sans crash */
        return;
#endif
    }

    memory_entry_t* entry = &g_tracker.entries[found_entry_idx];

    // PROTECTION ABSOLUE: Vérifier double-free
    // CORRECTION: Validation renforcée avec checksums
    if (entry->is_freed) {
        printf("[MEMORY_TRACKER] CRITICAL ERROR: DOUBLE FREE DETECTED!\n");
        printf("[MEMORY_TRACKER] Pointer validation checksum: 0x%lx\n", 
               (unsigned long)((uintptr_t)ptr ^ entry->generation));
        // Validation supplémentaire avec pattern de corruption
        if (entry->generation > g_global_generation) {
            printf("[MEMORY_TRACKER] CORRUPTION: Invalid generation detected!\n");
        }
        abort(); // Arrêt immédiat sur double-free
    }

    // Marquer comme libéré avec validation
    entry->is_freed = 1;
    entry->freed_time = time(NULL);
    entry->freed_file = file;
    entry->freed_line = line;
    entry->freed_function = func;

    g_tracker.total_freed += entry->size;
    g_tracker.current_usage -= entry->size;

    printf("[MEMORY_TRACKER] FREE: %p (%zu bytes) at %s:%d in %s() - originally allocated at %s:%d\n",
           ptr, entry->size, file, line, func, entry->file, entry->line);

    pthread_mutex_unlock(&g_tracker_mutex);

    // LIBÉRATION SÉCURISÉE
    free(ptr);

    // INVALIDATION du pointeur (note: ne peut pas modifier ptr directement)
    // Le code appelant doit mettre ptr = NULL après cette fonction
}

void* tracked_calloc(size_t nmemb, size_t size, const char* file, int line, const char* func) {
    if (!g_tracker_initialized) memory_tracker_init();
    if (!memory_tracker_is_enabled()) return calloc(nmemb, size);

    void* ptr = calloc(nmemb, size);
    if (ptr) {
        pthread_mutex_lock(&g_tracker_mutex);
        add_entry(ptr, nmemb * size, file, line, func);
        pthread_mutex_unlock(&g_tracker_mutex);
    }
    return ptr;
}

void* tracked_realloc(void* ptr, size_t size, const char* file, int line, const char* func) {
    if (!g_tracker_initialized) memory_tracker_init();
    if (!memory_tracker_is_enabled()) return realloc(ptr, size);

    // If ptr is NULL, equivalent to malloc
    if (!ptr) {
        return tracked_malloc(size, file, line, func);
    }

    pthread_mutex_lock(&g_tracker_mutex);
    int entry_idx = find_entry(ptr); // Find active entry
    int found_any_entry_idx = -1; // Declare here for wider scope
    size_t old_size = 0;

    if (entry_idx != -1) {
        old_size = g_tracker.entries[entry_idx].size;
        // Mark the old entry as freed
        g_tracker.entries[entry_idx].is_freed = 1;
        g_tracker.entries[entry_idx].freed_time = time(NULL);
        g_tracker.entries[entry_idx].freed_file = file;
        g_tracker.entries[entry_idx].freed_line = line;
        g_tracker.entries[entry_idx].freed_function = func;
        g_tracker.current_usage -= old_size;
    } else {
        // If the pointer was not found as active, it might be a double realloc or an untracked pointer.
        // For realloc, we still need to proceed if it's a valid pointer, but we should log a warning.
        // We search for any entry to get the old size if available.
        for (size_t i = 0; i < g_tracker.count; i++) {
            if (g_tracker.entries[i].ptr == ptr) {
                found_any_entry_idx = (int)i;
                break;
            }
        }
        if (found_any_entry_idx != -1) {
            old_size = g_tracker.entries[found_any_entry_idx].size;
            // If it was already freed, this is problematic.
            if (g_tracker.entries[found_any_entry_idx].is_freed) {
                 printf("[MEMORY_TRACKER] WARNING: Realloc called on a freed pointer %p at %s:%d in %s()\n",
                       ptr, file, line, func);
                 // Depending on policy, might want to abort or just proceed. Proceeding for now.
            }
             // We don't update the freed status here again if it was already marked, to avoid confusion.
             // But we do adjust current_usage if it was active.
             if (!g_tracker.entries[found_any_entry_idx].is_freed) {
                 g_tracker.current_usage -= old_size;
             }
        } else {
             printf("[MEMORY_TRACKER] WARNING: Realloc called on an untracked pointer %p at %s:%d in %s()\n",
                   ptr, file, line, func);
        }
    }
    pthread_mutex_unlock(&g_tracker_mutex);

    void* new_ptr = realloc(ptr, size);
    if (new_ptr) {
        pthread_mutex_lock(&g_tracker_mutex);
        add_entry(new_ptr, size, file, line, func);
        pthread_mutex_unlock(&g_tracker_mutex);
    } else {
        // If realloc fails, the original pointer is still valid and should be considered active again.
        if (entry_idx != -1) { // If it was a tracked active pointer before realloc
            pthread_mutex_lock(&g_tracker_mutex);
            g_tracker.entries[entry_idx].is_freed = 0; // Revert the freed status
            g_tracker.entries[entry_idx].freed_time = 0;
            g_tracker.entries[entry_idx].freed_file = NULL;
            g_tracker.entries[entry_idx].freed_line = 0;
            g_tracker.entries[entry_idx].freed_function = NULL;
            g_tracker.current_usage += old_size; // Restore usage
            pthread_mutex_unlock(&g_tracker_mutex);
        } else if (found_any_entry_idx != -1 && !g_tracker.entries[found_any_entry_idx].is_freed) {
            // If it was an untracked but valid pointer and realloc failed, restore usage if it was active.
            pthread_mutex_lock(&g_tracker_mutex);
            g_tracker.current_usage += old_size;
            pthread_mutex_unlock(&g_tracker_mutex);
        }
    }

    return new_ptr;
}

// NOUVELLES FONCTIONS : tracked_strdup et tracked_strndup
char* tracked_strdup(const char* str, const char* file, int line, const char* func) {
    if (!str) return NULL;
    if (!memory_tracker_is_enabled()) return strdup(str);

    size_t len = strlen(str) + 1;
    char* copy = tracked_malloc(len, file, line, func);
    if (copy) {
        memcpy(copy, str, len);
    }
    return copy;
}

char* tracked_strndup(const char* str, size_t n, const char* file, int line, const char* func) {
    if (!str) return NULL;
    if (!memory_tracker_is_enabled()) return strndup(str, n);

    size_t len = strnlen(str, n);
    char* copy = tracked_malloc(len + 1, file, line, func);
    if (copy) {
        memcpy(copy, str, len);
        copy[len] = '\0';
    }
    return copy;
}

void memory_tracker_report(void) {
    if (!g_tracker_initialized) return;
    if (!memory_tracker_is_enabled()) {
        printf("[MEMORY_TRACKER] Tracking is disabled.\n");
        return;
    }

    pthread_mutex_lock(&g_tracker_mutex);

    printf("\n=== MEMORY TRACKER REPORT ===\n");
    printf("Total allocations: %zu bytes\n", g_tracker.total_allocated);
    printf("Total freed: %zu bytes\n", g_tracker.total_freed);
    printf("Current usage: %zu bytes\n", g_tracker.current_usage);
    printf("Peak usage: %zu bytes\n", g_tracker.peak_usage);
    printf("Active entries: ");

    size_t active_count = 0;
    for (size_t i = 0; i < g_tracker.count; i++) {
        if (!g_tracker.entries[i].is_freed) {
            active_count++;
        }
    }
    printf("%zu\n", active_count);

    if (active_count > 0) {
        printf("\nACTIVE ALLOCATIONS (potential leaks):\n");
        for (size_t i = 0; i < g_tracker.count; i++) {
            if (!g_tracker.entries[i].is_freed) {
                printf("  %p (%zu bytes) - allocated at %s:%d in %s()\n",
                       g_tracker.entries[i].ptr,
                       g_tracker.entries[i].size,
                       g_tracker.entries[i].file,
                       g_tracker.entries[i].line,
                       g_tracker.entries[i].function);
            }
        }
    }

    printf("==============================\n\n");

    pthread_mutex_unlock(&g_tracker_mutex);
}

void memory_tracker_check_leaks(void) {
    if (!g_tracker_initialized) return;
     if (!memory_tracker_is_enabled()) {
        printf("[MEMORY_TRACKER] Leak check skipped: tracking is disabled.\n");
        return;
    }

    pthread_mutex_lock(&g_tracker_mutex);

    size_t leak_count = 0;
    size_t leak_size = 0;

    for (size_t i = 0; i < g_tracker.count; i++) {
        if (!g_tracker.entries[i].is_freed) {
            leak_count++;
            leak_size += g_tracker.entries[i].size;
        }
    }

    if (leak_count > 0) {
        printf("[MEMORY_TRACKER] LEAK DETECTION: %zu leaks (%zu bytes total)\n",
               leak_count, leak_size);
    } else {
        printf("[MEMORY_TRACKER] No memory leaks detected\n");
    }

    pthread_mutex_unlock(&g_tracker_mutex);
}

void memory_tracker_destroy(void) {
    if (!g_tracker_initialized) return;

    printf("[MEMORY_TRACKER] Final report before shutdown:\n");
    memory_tracker_report();
    memory_tracker_check_leaks();

    pthread_mutex_lock(&g_tracker_mutex);
    g_tracker_initialized = 0;
    pthread_mutex_unlock(&g_tracker_mutex);
}

// lum_log is defined in the logger mod

void memory_tracker_cleanup(void) {
    if (!g_tracker_initialized) return;

    printf("[MEMORY_TRACKER] Cleanup initiated\n");
    memory_tracker_report();
    memory_tracker_check_leaks();

    pthread_mutex_lock(&g_tracker_mutex);

    // Reset all entries
    memset(&g_tracker, 0, sizeof(g_tracker));
    g_tracker_initialized = false;
    g_tracking_enabled = true; // Reset to default

    pthread_mutex_unlock(&g_tracker_mutex);
    printf("[MEMORY_TRACKER] Cleanup completed\n");
}

/* C-FIX-RAM-03 : compactage du tableau entries entre chaque module.
 * Supprime les entrées is_freed=true par compactage en place (déplace les actives
 * vers le début). Évite l'accumulation de 50 000 entrées mixtes actives/libérées
 * qui ne se réinitialisent jamais (g_tracker est statique).
 * Retourne le nombre d'entrées actives restantes après compactage. */
int memory_tracker_reset_freed(void) {
    if (!g_tracker_initialized) return 0;
    pthread_mutex_lock(&g_tracker_mutex);

    size_t write_idx = 0;
    size_t active_count = 0;
    for (size_t i = 0; i < g_tracker.count; i++) {
        if (!g_tracker.entries[i].is_freed) {
            /* Entrée active : la conserver en compactant vers le début */
            if (write_idx != i) {
                g_tracker.entries[write_idx] = g_tracker.entries[i];
            }
            write_idx++;
            active_count++;
        }
        /* Entrées libérées : ignorées (écrasées par la prochaine entrée active) */
    }
    /* Zeroise les entrées expulsées pour éviter les pointeurs fantômes */
    if (write_idx < g_tracker.count) {
        memset(&g_tracker.entries[write_idx], 0,
               (g_tracker.count - write_idx) * sizeof(memory_entry_t));
    }
    size_t freed_count = g_tracker.count - write_idx;
    g_tracker.count = write_idx;

    pthread_mutex_unlock(&g_tracker_mutex);
    fprintf(stderr,
        "[MEMORY_TRACKER] C-FIX-RAM-03: purge freed entries: %zu supprimées, %zu actives restantes\n",
        freed_count, active_count);
    return (int)active_count;
}
/* ======================================================
 * lv_* — Implémentations LumVorax Integration Bridge
 * MT-001 FIX: les fonctions journalisent réellement les événements
 * et délèguent les allocations au tracker instrumenté.
 * Requis par lumvorax_integration.h (flag -DLUMVORAX_ENABLED=1)
 * Compatibilité Replit + Ubuntu — C48 2026-04-16 / MT-001-FIX
 * ====================================================== */
#include <time.h>
#include <stdint.h>
#include <stdbool.h>

/* Fichier de log bridge LumVorax — ouvert une seule fois au lv_init() */
static FILE* lv_log_fp = NULL;

bool lv_init(const char* log_dir) {
    /* MT-001 FIX: ouvrir un fichier de log réel pour tracer les événements bridge */
    if (!log_dir) log_dir = "/tmp";
    char path[512];
    snprintf(path, sizeof(path), "%s/lv_bridge_%lu.log", log_dir,
             (unsigned long)time(NULL));
    lv_log_fp = fopen(path, "w");
    if (!lv_log_fp) {
        fprintf(stderr, "[LV_BRIDGE] WARNING: cannot open log file %s — events lost\n", path);
        return false;
    }
    fprintf(lv_log_fp, "[LV_BRIDGE] init log_dir=%s\n", log_dir);
    fflush(lv_log_fp);
    return true;
}

void lv_destroy(void) {
    /* MT-001 FIX: fermer proprement le fichier de log bridge */
    if (lv_log_fp) {
        fprintf(lv_log_fp, "[LV_BRIDGE] destroy\n");
        fclose(lv_log_fp);
        lv_log_fp = NULL;
    }
}

void lv_module_start(const char* file, int line, const char* func,
                     const char* module, const char* label) {
    /* MT-001 FIX: écriture réelle dans le log bridge */
    if (lv_log_fp) {
        fprintf(lv_log_fp, "[LV_BRIDGE] MODULE_START module=%s label=%s at %s:%d %s()\n",
                module ? module : "?", label ? label : "?", file, line, func);
        fflush(lv_log_fp);
    }
}

void lv_module_end(const char* file, int line, const char* func,
                   const char* module, const char* label, bool success) {
    /* MT-001 FIX: écriture réelle dans le log bridge */
    if (lv_log_fp) {
        fprintf(lv_log_fp, "[LV_BRIDGE] MODULE_END module=%s label=%s success=%s at %s:%d %s()\n",
                module ? module : "?", label ? label : "?",
                success ? "true" : "false", file, line, func);
        fflush(lv_log_fp);
    }
}

void lv_module_metric(const char* file, int line, const char* func,
                      const char* module, const char* metric, double value) {
    /* MT-001 FIX: enregistrement réel de la métrique dans le log bridge */
    if (lv_log_fp) {
        fprintf(lv_log_fp, "[LV_BRIDGE] METRIC module=%s metric=%s value=%.6f at %s:%d %s()\n",
                module ? module : "?", metric ? metric : "?", value, file, line, func);
        fflush(lv_log_fp);
    }
}

void lv_module_operation(const char* file, int line, const char* func,
                         const char* module, const char* op, const char* data) {
    /* MT-001 FIX: enregistrement réel de l'opération dans le log bridge */
    if (lv_log_fp) {
        fprintf(lv_log_fp, "[LV_BRIDGE] OPERATION module=%s op=%s data=%s at %s:%d %s()\n",
                module ? module : "?", op ? op : "?", data ? data : "null", file, line, func);
        fflush(lv_log_fp);
    }
}

void* lv_tracked_calloc(size_t nmemb, size_t size, const char* file, int line, const char* func) {
    /* MT-001 FIX: délégation au tracker instrumenté */
    return tracked_calloc(nmemb, size, file, line, func);
}

void* lv_tracked_malloc(size_t size, const char* file, int line, const char* func) {
    /* MT-001 FIX: délégation au tracker instrumenté */
    return tracked_malloc(size, file, line, func);
}

void lv_tracked_free(void* ptr, const char* file, int line, const char* func) {
    /* MT-001 FIX: délégation au tracker instrumenté */
    tracked_free(ptr, file, line, func);
}

void lv_report_leaks(void) {
    /* MT-001 FIX: rapport réel des leaks via tracker */
    memory_tracker_check_leaks();
    if (lv_log_fp) {
        fprintf(lv_log_fp, "[LV_BRIDGE] lv_report_leaks called — see MEMORY_TRACKER output\n");
        fflush(lv_log_fp);
    }
}

uint64_t lv_get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}
