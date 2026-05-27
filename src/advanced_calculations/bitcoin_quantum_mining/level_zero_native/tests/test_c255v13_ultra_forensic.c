/*
 * LumVorax C255V13 — ULTRA FORENSIC TEST
 * Réutilise le runner existant avec instrumentation bit-level maximale
 * 
 * OBJECTIF: Capturer l'état COMPLET du contexte GPU à chaque dispatch
 * pour identifier EXACTEMENT ce qui cause le crash au dispatch #28
 */

#define _GNU_SOURCE
#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>

// Déclarations externes du runner
extern int btc_gen9_init(uint32_t batch_size, uint32_t work_group_size);
extern int btc_gen9_mine(uint32_t start_nonce, uint32_t count, uint32_t* best_nonce, uint32_t* leading_zeros);
extern void btc_gen9_cleanup(void);
extern void btc_gen9_set_batch_mode(int enabled);

// Variables globales pour forensic
static FILE* forensic_log = NULL;
static uint64_t dispatch_count = 0;

static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static void log_forensic(const char* event, const char* details) {
    if (!forensic_log) return;
    
    uint64_t ts = get_timestamp_ns();
    fprintf(forensic_log, "[%lu] DISPATCH_%lu | %s | %s\n", 
            ts, dispatch_count, event, details);
    fflush(forensic_log);
}

static void dump_system_state(void) {
    if (!forensic_log) return;
    
    fprintf(forensic_log, "\n═══════════════════════════════════════════════════════════════\n");
    fprintf(forensic_log, "SYSTEM STATE DUMP — Dispatch #%lu\n", dispatch_count);
    fprintf(forensic_log, "═══════════════════════════════════════════════════════════════\n");
    fprintf(forensic_log, "Timestamp: %lu ns\n", get_timestamp_ns());
    
    // Lire /proc/meminfo
    FILE* meminfo = fopen("/proc/meminfo", "r");
    if (meminfo) {
        char line[256];
        fprintf(forensic_log, "\n--- /proc/meminfo (relevant lines) ---\n");
        while (fgets(line, sizeof(line), meminfo)) {
            if (strstr(line, "MemAvailable") || 
                strstr(line, "MemFree") ||
                strstr(line, "Shmem")) {
                fprintf(forensic_log, "%s", line);
            }
        }
        fclose(meminfo);
    }
    
    // Lire /sys/kernel/debug/dri/0/i915_gem_objects (si accessible)
    FILE* gem_objects = fopen("/sys/kernel/debug/dri/0/i915_gem_objects", "r");
    if (gem_objects) {
        char line[256];
        fprintf(forensic_log, "\n--- i915 GEM Objects ---\n");
        int line_count = 0;
        while (fgets(line, sizeof(line), gem_objects) && line_count++ < 20) {
            fprintf(forensic_log, "%s", line);
        }
        fclose(gem_objects);
    }
    
    // Lire /sys/class/drm/card0/error (si accessible)
    FILE* drm_error = fopen("/sys/class/drm/card0/error", "r");
    if (drm_error) {
        char line[256];
        fprintf(forensic_log, "\n--- DRM Error State ---\n");
        while (fgets(line, sizeof(line), drm_error)) {
            fprintf(forensic_log, "%s", line);
        }
        fclose(drm_error);
    }
    
    fprintf(forensic_log, "═══════════════════════════════════════════════════════════════\n\n");
    fflush(forensic_log);
}

int main(void) {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  LumVorax C255V13 — ULTRA FORENSIC TEST                   ║\n");
    printf("║  Instrumentation Bit-Level Maximale                       ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");
    
    // Ouvrir log forensique
    forensic_log = fopen("logs/forensic/c255v13_ultra_forensic.log", "w");
    if (!forensic_log) {
        fprintf(stderr, "❌ Impossible d'ouvrir log forensique\n");
        return 1;
    }
    
    fprintf(forensic_log, "# LumVorax C255V13 — ULTRA FORENSIC LOG\n");
    fprintf(forensic_log, "# Timestamp Start: %lu\n", get_timestamp_ns());
    fprintf(forensic_log, "# Objective: Capture exact state at crash point\n\n");
    
    printf("Configuration:\n");
    printf("  - Batch size: 262144 nonces (256K)\n");
    printf("  - Max dispatches: 30\n");
    printf("  - Forensic log: logs/forensic/c255v13_ultra_forensic.log\n\n");
    
    // Initialiser
    log_forensic("INIT_START", "batch_size=262144");
    
    if (btc_gen9_init(262144, 256) != 0) {
        fprintf(stderr, "❌ Échec initialisation\n");
        log_forensic("INIT_FAILED", "");
        fclose(forensic_log);
        return 1;
    }
    
    log_forensic("INIT_SUCCESS", "");
    printf("✅ Initialisation réussie\n\n");
    
    // Activer batch mode (pas de reopen automatique)
    btc_gen9_set_batch_mode(1);
    log_forensic("BATCH_MODE_ENABLED", "");
    
    // Dump état initial
    dump_system_state();
    
    // Boucle de test: 30 dispatches
    printf("Démarrage test 30 dispatches...\n\n");
    
    int success_count = 0;
    uint32_t best_nonce = 0;
    uint32_t leading_zeros = 0;
    
    for (int i = 1; i <= 30; i++) {
        dispatch_count = i;
        
        printf("[Dispatch %d/30] ", i);
        
        char details[256];
        snprintf(details, sizeof(details), "start_nonce=0 count=262144");
        log_forensic("DISPATCH_START", details);
        
        // Dump état système tous les 5 dispatches
        if (i % 5 == 0) {
            dump_system_state();
        }
        
        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);
        
        int ret = btc_gen9_mine(0, 262144, &best_nonce, &leading_zeros);
        
        clock_gettime(CLOCK_MONOTONIC, &end);
        double elapsed = (end.tv_sec - start.tv_sec) + 
                        (end.tv_nsec - start.tv_nsec) / 1e9;
        
        if (ret == 0) {
            success_count++;
            double hashrate = 262144.0 / elapsed / 1e6;
            printf("✅ Succès (%.3f sec, %.2f MH/s)\n", elapsed, hashrate);
            
            snprintf(details, sizeof(details), 
                    "SUCCESS time=%.6f hashrate=%.2f", elapsed, hashrate);
            log_forensic("DISPATCH_SUCCESS", details);
        } else {
            printf("❌ ÉCHEC\n");
            
            snprintf(details, sizeof(details), "FAILED ret=%d", ret);
            log_forensic("DISPATCH_FAILED", details);
            
            // Dump état complet au moment du crash
            fprintf(forensic_log, "\n");
            fprintf(forensic_log, "╔═══════════════════════════════════════════════════════════════╗\n");
            fprintf(forensic_log, "║  CRASH DETECTED AT DISPATCH #%lu                              ║\n", dispatch_count);
            fprintf(forensic_log, "╚═══════════════════════════════════════════════════════════════╝\n");
            dump_system_state();
            
            printf("\n⚠️  Arrêt après échec au dispatch #%d\n", i);
            break;
        }
        
        usleep(10000); // Pause 10ms
    }
    
    printf("\n");
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("RÉSULTATS FINAUX\n");
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("Dispatches réussis: %d/30\n", success_count);
    printf("Log forensique: logs/forensic/c255v13_ultra_forensic.log\n");
    printf("═══════════════════════════════════════════════════════════════\n\n");
    
    log_forensic("CLEANUP_START", "");
    btc_gen9_cleanup();
    log_forensic("CLEANUP_COMPLETE", "");
    
    fprintf(forensic_log, "\n# Test completed at: %lu\n", get_timestamp_ns());
    fprintf(forensic_log, "# Total dispatches: %lu\n", dispatch_count);
    fprintf(forensic_log, "# Success count: %d\n", success_count);
    
    fclose(forensic_log);
    
    return (success_count == 30) ? 0 : 1;
}

// Made with Bob
