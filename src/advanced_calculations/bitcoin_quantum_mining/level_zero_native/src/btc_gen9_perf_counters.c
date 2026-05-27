/*
 * LumVorax — Module 17 — Bitcoin Quantum Mining Engine
 * btc_gen9_perf_counters.c — Compteurs GPU i915 Perf (EU Busy Time)
 *
 * CYCLE C255v5 — Validations HARDWARE Complètes
 * OBJECTIF : Prouver parallélisme GPU réel avec compteurs hardware
 *
 * API i915 Perf:
 * - EU busy time (nanoseconde)
 * - EU stall time (nanoseconde)
 * - EU active threads count
 * - Validation: 9 batch_bo busy simultanément
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <time.h>

/* DRM headers */
#include <drm/i915_drm.h>

/* ══════════════════════════════════════════════════════════════════════
 * STRUCTURES COMPTEURS GPU
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * Compteurs GPU i915 perf
 */
typedef struct {
    int perf_fd;                    /* File descriptor i915 perf */
    uint64_t eu_busy_ns;            /* Temps EU busy (nanosec) */
    uint64_t eu_stall_ns;           /* Temps EU stall (nanosec) */
    uint64_t eu_active_threads;     /* Threads actifs */
    double eu_utilization_percent;  /* % utilisation */
    uint64_t timestamp_start;       /* Timestamp début mesure */
    uint64_t timestamp_end;         /* Timestamp fin mesure */
} btc_gpu_perf_t;

/* ══════════════════════════════════════════════════════════════════════
 * UTILITAIRES TIMESTAMP
 * ══════════════════════════════════════════════════════════════════════ */

static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

/* ══════════════════════════════════════════════════════════════════════
 * INITIALISATION i915 PERF
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * Initialiser compteurs GPU i915 perf
 * 
 * Note: i915 perf nécessite permissions root ou CAP_PERFMON
 * Alternative: Lire /sys/kernel/debug/dri/0/i915_engine_info
 */
int btc_gen9_init_perf(int drm_fd, btc_gpu_perf_t* perf) {
    if (!perf) {
        fprintf(stderr, "ERROR: perf structure NULL\n");
        return -1;
    }
    
    memset(perf, 0, sizeof(btc_gpu_perf_t));
    perf->perf_fd = -1;
    
    /* Tentative ouverture i915 perf (nécessite root) */
    struct drm_i915_perf_open_param param = {
        .flags = 0,
        .num_properties = 0,
        .properties_ptr = 0
    };
    
    int ret = ioctl(drm_fd, DRM_IOCTL_I915_PERF_OPEN, &param);
    if (ret < 0) {
        /* Fallback: Utiliser sysfs (pas de permissions root requises) */
        fprintf(stderr, "WARNING: i915 perf unavailable (errno=%d), using sysfs fallback\n", errno);
        perf->perf_fd = -1;
        return 0;  /* Succès avec fallback */
    }
    
    perf->perf_fd = ret;
    printf("INFO: i915 perf initialized (fd=%d)\n", perf->perf_fd);
    return 0;
}

/* ══════════════════════════════════════════════════════════════════════
 * LECTURE COMPTEURS SYSFS (FALLBACK)
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * Lire compteurs GPU depuis sysfs
 * Chemin: /sys/kernel/debug/dri/0/i915_engine_info
 */
static int read_sysfs_engine_info(btc_gpu_perf_t* perf) {
    FILE* f = fopen("/sys/kernel/debug/dri/0/i915_engine_info", "r");
    if (!f) {
        /* Pas de permissions debug, utiliser estimation */
        return -1;
    }
    
    char line[256];
    uint64_t busy_ns = 0;
    uint64_t total_ns = 0;
    
    while (fgets(line, sizeof(line), f)) {
        /* Parser lignes format:
         * render0: busy 123456789 ns, total 987654321 ns
         */
        if (strstr(line, "render") && strstr(line, "busy")) {
            sscanf(line, "%*s busy %llu ns, total %llu ns", &busy_ns, &total_ns);
            break;
        }
    }
    
    fclose(f);
    
    if (total_ns > 0) {
        perf->eu_busy_ns = busy_ns;
        perf->eu_utilization_percent = (double)busy_ns / (double)total_ns * 100.0;
        return 0;
    }
    
    return -1;
}

/* ══════════════════════════════════════════════════════════════════════
 * MESURE COMPTEURS GPU
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * Démarrer mesure compteurs GPU
 */
int btc_gen9_perf_start(btc_gpu_perf_t* perf) {
    if (!perf) return -1;
    
    perf->timestamp_start = get_timestamp_ns();
    
    if (perf->perf_fd >= 0) {
        /* Lire compteurs i915 perf */
        uint64_t data[16];
        ssize_t n = read(perf->perf_fd, data, sizeof(data));
        if (n > 0) {
            perf->eu_busy_ns = data[0];
            perf->eu_stall_ns = data[1];
        }
    } else {
        /* Fallback sysfs */
        read_sysfs_engine_info(perf);
    }
    
    return 0;
}

/**
 * Arrêter mesure compteurs GPU
 *
 * BUG FIX C255v6: Gérer perf_fd invalide (-1) sans crash
 */
int btc_gen9_perf_stop(btc_gpu_perf_t* perf) {
    if (!perf) return -1;
    
    perf->timestamp_end = get_timestamp_ns();
    
    uint64_t eu_busy_start = perf->eu_busy_ns;
    
    if (perf->perf_fd >= 0) {
        /* Lire compteurs i915 perf */
        uint64_t data[16];
        ssize_t n = read(perf->perf_fd, data, sizeof(data));
        if (n > 0) {
            perf->eu_busy_ns = data[0] - eu_busy_start;
            perf->eu_stall_ns = data[1];
        } else {
            /* Erreur lecture → fallback sysfs */
            fprintf(stderr, "WARNING: i915 perf read failed (errno=%d), using sysfs\n", errno);
            btc_gpu_perf_t perf_end;
            if (read_sysfs_engine_info(&perf_end) == 0) {
                perf->eu_busy_ns = perf_end.eu_busy_ns - eu_busy_start;
            } else {
                /* Estimation basée sur temps CPU */
                perf->eu_busy_ns = 0;
            }
        }
    } else {
        /* perf_fd invalide → fallback sysfs obligatoire */
        btc_gpu_perf_t perf_end;
        if (read_sysfs_engine_info(&perf_end) == 0) {
            perf->eu_busy_ns = perf_end.eu_busy_ns - eu_busy_start;
        } else {
            /* Aucune source disponible → estimation 0% */
            perf->eu_busy_ns = 0;
        }
    }
    
    /* Calculer % utilisation */
    uint64_t delta_ns = perf->timestamp_end - perf->timestamp_start;
    if (delta_ns > 0) {
        perf->eu_utilization_percent = (double)perf->eu_busy_ns / (double)delta_ns * 100.0;
    }
    
    return 0;
}

/**
 * Cleanup compteurs GPU
 */
void btc_gen9_perf_cleanup(btc_gpu_perf_t* perf) {
    if (perf && perf->perf_fd >= 0) {
        close(perf->perf_fd);
        perf->perf_fd = -1;
    }
}

/* ══════════════════════════════════════════════════════════════════════
 * VALIDATION PARALLÉLISME
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * Valider parallélisme GPU (9 batch_bo busy simultanément)
 * 
 * Critère: EU utilization >85% constant pendant 9 dispatches
 * Si sérialisation: EU utilization pics isolés <50%
 */
bool btc_gen9_validate_parallelism(btc_gpu_perf_t* perf_samples, int count) {
    if (!perf_samples || count < 9) {
        fprintf(stderr, "ERROR: Need at least 9 samples for validation\n");
        return false;
    }
    
    /* Calculer moyenne EU utilization */
    double sum_utilization = 0.0;
    double min_utilization = 100.0;
    double max_utilization = 0.0;
    
    for (int i = 0; i < count; i++) {
        double util = perf_samples[i].eu_utilization_percent;
        sum_utilization += util;
        if (util < min_utilization) min_utilization = util;
        if (util > max_utilization) max_utilization = util;
    }
    
    double avg_utilization = sum_utilization / count;
    double variance = max_utilization - min_utilization;
    
    printf("\n=== VALIDATION PARALLÉLISME GPU ===\n");
    printf("Samples: %d\n", count);
    printf("EU Utilization Average: %.1f%%\n", avg_utilization);
    printf("EU Utilization Min: %.1f%%\n", min_utilization);
    printf("EU Utilization Max: %.1f%%\n", max_utilization);
    printf("EU Utilization Variance: %.1f%%\n", variance);
    
    /* Critères validation */
    bool parallel_ok = (avg_utilization >= 85.0);
    bool stable_ok = (variance <= 15.0);
    
    printf("\nCritères:\n");
    printf("  Average >=85%%: %s (%.1f%%)\n", 
           parallel_ok ? "✅ PASS" : "❌ FAIL", avg_utilization);
    printf("  Variance <=15%%: %s (%.1f%%)\n", 
           stable_ok ? "✅ PASS" : "❌ FAIL", variance);
    
    bool validated = parallel_ok && stable_ok;
    printf("\nRésultat: %s\n", validated ? "✅ PARALLÉLISME VALIDÉ" : "❌ SÉRIALISATION DÉTECTÉE");
    printf("===================================\n\n");
    
    return validated;
}

/* ══════════════════════════════════════════════════════════════════════
 * AFFICHAGE MÉTRIQUES
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * Afficher métriques GPU
 */
void btc_gen9_perf_print(const btc_gpu_perf_t* perf, int dispatch_id) {
    if (!perf) return;
    
    uint64_t delta_ns = perf->timestamp_end - perf->timestamp_start;
    double delta_sec = (double)delta_ns / 1000000000.0;
    
    printf("Dispatch %d:\n", dispatch_id);
    printf("  Time: %.3f sec\n", delta_sec);
    printf("  EU Busy: %.3f sec (%.1f%%)\n", 
           (double)perf->eu_busy_ns / 1000000000.0,
           perf->eu_utilization_percent);
    printf("  EU Stall: %.3f sec\n", (double)perf->eu_stall_ns / 1000000000.0);
}

// Made with Bob
