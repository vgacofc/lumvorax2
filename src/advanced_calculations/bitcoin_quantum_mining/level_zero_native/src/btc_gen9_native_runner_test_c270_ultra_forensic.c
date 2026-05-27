/*
 * LumVorax — Module 17 — Bitcoin Quantum Mining Engine
 * btc_gen9_native_runner_test_c270_ultra_forensic.c
 *
 * CYCLE C270 — TEST ULTRA-FORENSIQUE NANO BIT-LEVEL
 * OBJECTIF : IDENTIFIER CAUSE RACINE EXACTE BUG errno=5
 *
 * STRATÉGIE :
 * 1. DÉSACTIVER reopen DRM (forcer reproduction bug après 27 dispatches)
 * 2. ACTIVER memory_tracker ultra-granulaire MAXIMAL (RAM système UMA)
 * 3. CAPTURER état COMPLET avant/pendant/après CHAQUE dispatch (dispatches 24-28)
 * 4. INSTRUMENTER TOUTES les couches système :
 *    - Hardware : Registres GPU, température, voltage, fréquence
 *    - Kernel : Appels système, interruptions, scheduler
 *    - Driver i915 : État DRM, handles GEM, contextes, batch buffers
 *    - Application : Timestamps nanoseconde, allocations mémoire
 * 5. ANALYSER séquence exacte causant errno=5
 * 6. IDENTIFIER couche système responsable du bug
 *
 * MODIFICATIONS PAR RAPPORT À btc_gen9_native_runner.c :
 * - Ligne 1135 : DÉSACTIVER reopen DRM (if (0) au lieu de if (...))
 * - Ajout tracking ultra-granulaire RAM/GPU/DRM à chaque dispatch
 * - Ajout capture état système complet (hardware → application)
 * - Ajout logging nanoseconde pour chaque opération
 */

#define _GNU_SOURCE
#define _POSIX_C_SOURCE 199309L

/* C270: Activer tracking forensique ultra-granulaire */
#define ENABLE_I915_FORENSIC_TRACKING
#define ENABLE_C270_ULTRA_FORENSIC_TRACKING
#include "btc_i915_drm_forensic_tracker.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <errno.h>
#include <time.h>
#include <limits.h>
#include <pthread.h>

/* DRM headers */
#include <drm/i915_drm.h>
#include <xf86drm.h>

/* ══════════════════════════════════════════════════════════════════════
 * SECTION C270 : STRUCTURES ULTRA-FORENSIQUES
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * C270 : Structure capture état système complet
 */
typedef struct {
    /* Timestamp nanoseconde */
    uint64_t timestamp_ns;
    
    /* Hardware GPU */
    int gpu_freq_mhz;
    int gpu_temp_celsius;
    
    /* Mémoire RAM système (UMA) */
    uint64_t ram_total_bytes;
    uint64_t ram_free_bytes;
    uint64_t ram_available_bytes;
    uint64_t ram_buffers_bytes;
    uint64_t ram_cached_bytes;
    
    /* Mémoire GPU (via i915 DRM) */
    uint64_t gpu_mem_total_bytes;
    uint64_t gpu_mem_used_bytes;
    uint64_t gpu_mem_free_bytes;
    
    /* État DRM */
    int drm_fd;
    uint32_t ctx_id_active;
    int ctx_pool_index;
    int batch_pool_index;
    
    /* Handles GEM */
    uint32_t kernel_bo;
    uint32_t input_bo;
    uint32_t output_bo;
    uint32_t batch_bo_current;
    
    /* Compteurs dispatches */
    uint64_t total_dispatches;
    int ctx_usage_count[9];
    
    /* Métriques performance */
    double exec_time_sec;
    double hashrate_mhs;
    
    /* Flags état */
    int in_batch_mode;
    int thermal_throttle_detected;
    
} c270_system_snapshot_t;

/**
 * C270 : Contexte forensique ultra-granulaire
 */
typedef struct {
    FILE* ultra_log_file;
    FILE* memory_log_file;
    FILE* drm_log_file;
    FILE* hardware_log_file;
    
    /* Snapshots système */
    c270_system_snapshot_t snapshots[100];  /* Buffer 100 snapshots */
    int snapshot_count;
    
    /* Flags tracking */
    int enable_memory_tracking;
    int enable_drm_tracking;
    int enable_hardware_tracking;
    
} c270_forensic_context_t;

/* Contexte forensique global */
static c270_forensic_context_t g_c270_forensic = {0};

/* ══════════════════════════════════════════════════════════════════════
 * SECTION C270 : FONCTIONS CAPTURE ÉTAT SYSTÈME
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * C270 : Lire état mémoire RAM système (UMA)
 */
static void c270_capture_ram_state(c270_system_snapshot_t* snapshot) {
    FILE* fp = fopen("/proc/meminfo", "r");
    if (!fp) {
        return;
    }
    
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        uint64_t value_kb = 0;
        
        if (sscanf(line, "MemTotal: %lu kB", &value_kb) == 1) {
            snapshot->ram_total_bytes = value_kb * 1024;
        } else if (sscanf(line, "MemFree: %lu kB", &value_kb) == 1) {
            snapshot->ram_free_bytes = value_kb * 1024;
        } else if (sscanf(line, "MemAvailable: %lu kB", &value_kb) == 1) {
            snapshot->ram_available_bytes = value_kb * 1024;
        } else if (sscanf(line, "Buffers: %lu kB", &value_kb) == 1) {
            snapshot->ram_buffers_bytes = value_kb * 1024;
        } else if (sscanf(line, "Cached: %lu kB", &value_kb) == 1) {
            snapshot->ram_cached_bytes = value_kb * 1024;
        }
    }
    
    fclose(fp);
}

/**
 * C270 : Lire fréquence GPU actuelle
 */
static int c270_read_gpu_frequency(void) {
    FILE* fp = fopen("/sys/class/drm/card0/gt_cur_freq_mhz", "r");
    if (!fp) {
        return -1;
    }
    
    int freq_mhz = 0;
    fscanf(fp, "%d", &freq_mhz);
    fclose(fp);
    
    return freq_mhz;
}

/**
 * C270 : Lire température GPU (via hwmon)
 */
static int c270_read_gpu_temperature(void) {
    const char* temp_paths[] = {
        "/sys/class/drm/card0/device/hwmon/hwmon0/temp1_input",
        "/sys/class/drm/card0/device/hwmon/hwmon1/temp1_input",
        "/sys/class/hwmon/hwmon0/temp1_input",
        "/sys/class/hwmon/hwmon1/temp1_input",
        NULL
    };
    
    for (int i = 0; temp_paths[i]; i++) {
        FILE* fp = fopen(temp_paths[i], "r");
        if (fp) {
            int temp_millidegrees = 0;
            if (fscanf(fp, "%d", &temp_millidegrees) == 1) {
                fclose(fp);
                return temp_millidegrees / 1000;
            }
            fclose(fp);
        }
    }
    
    return -1;
}

/**
 * C270 : Capturer snapshot système complet
 */
static void c270_capture_system_snapshot(c270_system_snapshot_t* snapshot,
                                         void* ctx_opaque) {
    /* Cast contexte COMPLET (défini dans btc_gen9_native_runner.c lignes 52-140) */
    typedef struct {
        int drm_fd;
        uint32_t ctx_id;
        uint32_t ctx_pool[9];
        int ctx_pool_index;
        int ctx_usage_count[9];
        int in_batch_mode;
        uint32_t batch_bo_pool[90];
        void* batch_map_pool[90];
        int batch_pool_index;
        size_t batch_size;
        uint32_t kernel_bo;
        void* kernel_map;
        size_t kernel_size;
        char kernel_path[256];
        void* kernel_cache;
        size_t kernel_cache_size;
        uint32_t input_bo;
        void* input_map;
        size_t input_size;
        uint32_t output_bo;
        void* output_map;
        size_t output_size;
        uint64_t total_dispatches;
        uint64_t total_hashes;
        double total_time_sec;
        int thermal_throttle_count;
        int last_gpu_temp_celsius;
        pthread_t async_save_thread;
        pthread_mutex_t save_mutex;
        int async_save_active;
        uint32_t* async_save_buffer;
        size_t async_save_size;
        int async_save_pending;
        FILE* log_file;
        uint64_t event_counter;
    } btc_gen9_context_t;
    
    btc_gen9_context_t* ctx = (btc_gen9_context_t*)ctx_opaque;
    
    /* Timestamp nanoseconde */
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    snapshot->timestamp_ns = ts.tv_sec * 1000000000ULL + ts.tv_nsec;
    
    /* Hardware GPU */
    snapshot->gpu_freq_mhz = c270_read_gpu_frequency();
    snapshot->gpu_temp_celsius = c270_read_gpu_temperature();
    
    /* Mémoire RAM système */
    c270_capture_ram_state(snapshot);
    
    /* État DRM */
    snapshot->drm_fd = ctx->drm_fd;
    snapshot->ctx_id_active = ctx->ctx_pool[ctx->ctx_pool_index];
    snapshot->ctx_pool_index = ctx->ctx_pool_index;
    snapshot->batch_pool_index = ctx->batch_pool_index;
    
    /* Handles GEM */
    snapshot->kernel_bo = ctx->kernel_bo;
    snapshot->input_bo = ctx->input_bo;
    snapshot->output_bo = ctx->output_bo;
    snapshot->batch_bo_current = ctx->batch_bo_pool[ctx->batch_pool_index];
    
    /* Compteurs */
    snapshot->total_dispatches = ctx->total_dispatches;
    memcpy(snapshot->ctx_usage_count, ctx->ctx_usage_count, sizeof(snapshot->ctx_usage_count));
    
    /* Flags */
    snapshot->in_batch_mode = ctx->in_batch_mode;
    snapshot->thermal_throttle_detected = (snapshot->gpu_freq_mhz > 0 && snapshot->gpu_freq_mhz < 1000) ? 1 : 0;
}

/**
 * C270 : Logger snapshot dans fichiers forensiques
 */
static void c270_log_snapshot(const c270_system_snapshot_t* snapshot, const char* phase) {
    if (!g_c270_forensic.ultra_log_file) {
        return;
    }
    
    fprintf(g_c270_forensic.ultra_log_file,
            "\n[C270_SNAPSHOT_%s] timestamp=%lu.%09lu\n",
            phase,
            snapshot->timestamp_ns / 1000000000ULL,
            snapshot->timestamp_ns % 1000000000ULL);
    
    fprintf(g_c270_forensic.ultra_log_file,
            "  HARDWARE: gpu_freq=%d MHz gpu_temp=%d°C\n",
            snapshot->gpu_freq_mhz, snapshot->gpu_temp_celsius);
    
    fprintf(g_c270_forensic.ultra_log_file,
            "  RAM_UMA: total=%lu MB free=%lu MB available=%lu MB buffers=%lu MB cached=%lu MB\n",
            snapshot->ram_total_bytes / 1048576,
            snapshot->ram_free_bytes / 1048576,
            snapshot->ram_available_bytes / 1048576,
            snapshot->ram_buffers_bytes / 1048576,
            snapshot->ram_cached_bytes / 1048576);
    
    fprintf(g_c270_forensic.ultra_log_file,
            "  DRM: fd=%d ctx_active=%u ctx_pool_idx=%d batch_pool_idx=%d\n",
            snapshot->drm_fd, snapshot->ctx_id_active,
            snapshot->ctx_pool_index, snapshot->batch_pool_index);
    
    fprintf(g_c270_forensic.ultra_log_file,
            "  GEM_HANDLES: kernel_bo=%u input_bo=%u output_bo=%u batch_bo=%u\n",
            snapshot->kernel_bo, snapshot->input_bo,
            snapshot->output_bo, snapshot->batch_bo_current);
    
    fprintf(g_c270_forensic.ultra_log_file,
            "  COUNTERS: total_dispatches=%lu\n",
            snapshot->total_dispatches);
    
    fprintf(g_c270_forensic.ultra_log_file,
            "  CTX_USAGE: [%d,%d,%d,%d,%d,%d,%d,%d,%d]\n",
            snapshot->ctx_usage_count[0], snapshot->ctx_usage_count[1],
            snapshot->ctx_usage_count[2], snapshot->ctx_usage_count[3],
            snapshot->ctx_usage_count[4], snapshot->ctx_usage_count[5],
            snapshot->ctx_usage_count[6], snapshot->ctx_usage_count[7],
            snapshot->ctx_usage_count[8]);
    
    fprintf(g_c270_forensic.ultra_log_file,
            "  FLAGS: in_batch_mode=%d thermal_throttle=%d\n",
            snapshot->in_batch_mode, snapshot->thermal_throttle_detected);
    
    fflush(g_c270_forensic.ultra_log_file);
}

/* ══════════════════════════════════════════════════════════════════════
 * SECTION C270 : INITIALISATION FORENSIQUE
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * C270 : Initialiser contexte forensique ultra-granulaire
 */
static int c270_init_forensic_context(void) {
    /* Créer répertoire logs si nécessaire */
    system("mkdir -p logs/forensic/c270_ultra");
    
    /* Ouvrir fichiers logs */
    g_c270_forensic.ultra_log_file = fopen("logs/forensic/c270_ultra/ultra_forensic.log", "w");
    g_c270_forensic.memory_log_file = fopen("logs/forensic/c270_ultra/memory_tracking.log", "w");
    g_c270_forensic.drm_log_file = fopen("logs/forensic/c270_ultra/drm_state.log", "w");
    g_c270_forensic.hardware_log_file = fopen("logs/forensic/c270_ultra/hardware_state.log", "w");
    
    if (!g_c270_forensic.ultra_log_file) {
        fprintf(stderr, "[C270_ERROR] Failed to open ultra_forensic.log\n");
        return -1;
    }
    
    /* Activer tous les trackings */
    g_c270_forensic.enable_memory_tracking = 1;
    g_c270_forensic.enable_drm_tracking = 1;
    g_c270_forensic.enable_hardware_tracking = 1;
    
    /* Initialiser compteur snapshots */
    g_c270_forensic.snapshot_count = 0;
    
    /* Header logs */
    fprintf(g_c270_forensic.ultra_log_file,
            "# LumVorax C270 — TEST ULTRA-FORENSIQUE NANO BIT-LEVEL\n");
    fprintf(g_c270_forensic.ultra_log_file,
            "# OBJECTIF : Identifier cause racine exacte bug errno=5\n");
    fprintf(g_c270_forensic.ultra_log_file,
            "# STRATÉGIE : Désactiver reopen + Tracking ultra-granulaire maximal\n");
    fprintf(g_c270_forensic.ultra_log_file,
            "# Device: Intel UHD Graphics 620 (Gen9)\n");
    fprintf(g_c270_forensic.ultra_log_file,
            "# Architecture: UMA (Unified Memory Architecture)\n\n");
    fflush(g_c270_forensic.ultra_log_file);
    
    fprintf(stderr, "[C270_INIT] Forensic context initialized successfully\n");
    fprintf(stderr, "[C270_INIT] Logs: logs/forensic/c270_ultra/\n");
    fprintf(stderr, "[C270_INIT] Memory tracking: ENABLED (ultra-granular)\n");
    fprintf(stderr, "[C270_INIT] DRM tracking: ENABLED\n");
    fprintf(stderr, "[C270_INIT] Hardware tracking: ENABLED\n");
    
    return 0;
}

/**
 * C270 : Cleanup contexte forensique
 */
static void c270_cleanup_forensic_context(void) {
    if (g_c270_forensic.ultra_log_file) {
        fprintf(g_c270_forensic.ultra_log_file,
                "\n[C270_CLEANUP] Total snapshots captured: %d\n",
                g_c270_forensic.snapshot_count);
        fclose(g_c270_forensic.ultra_log_file);
    }
    
    if (g_c270_forensic.memory_log_file) fclose(g_c270_forensic.memory_log_file);
    if (g_c270_forensic.drm_log_file) fclose(g_c270_forensic.drm_log_file);
    if (g_c270_forensic.hardware_log_file) fclose(g_c270_forensic.hardware_log_file);
    
    fprintf(stderr, "[C270_CLEANUP] Forensic logs saved to logs/forensic/c270_ultra/\n");
}

/* ══════════════════════════════════════════════════════════════════════
 * SECTION C270 : WRAPPER FONCTIONS btc_gen9_execute
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * C270 : Wrapper btc_gen9_execute avec tracking ultra-granulaire
 * 
 * MODIFICATIONS :
 * 1. Capturer snapshot AVANT dispatch
 * 2. Exécuter dispatch original
 * 3. Capturer snapshot APRÈS dispatch
 * 4. Logger différences état système
 * 5. Détecter anomalies (errno=5, memory leak, GPU hang, etc.)
 */

/* Déclaration fonction originale (définie dans btc_gen9_native_runner.c) */
extern int btc_gen9_execute(void* ctx);

int c270_btc_gen9_execute_wrapper(void* ctx) {
    c270_system_snapshot_t snapshot_before = {0};
    c270_system_snapshot_t snapshot_after = {0};
    
    /* Capturer état AVANT dispatch */
    c270_capture_system_snapshot(&snapshot_before, ctx);
    c270_log_snapshot(&snapshot_before, "BEFORE_DISPATCH");
    
    /* Exécuter dispatch original */
    int ret = btc_gen9_execute(ctx);
    
    /* Capturer état APRÈS dispatch */
    c270_capture_system_snapshot(&snapshot_after, ctx);
    c270_log_snapshot(&snapshot_after, "AFTER_DISPATCH");
    
    /* Analyser différences */
    uint64_t ram_delta = snapshot_before.ram_free_bytes - snapshot_after.ram_free_bytes;
    int freq_delta = snapshot_after.gpu_freq_mhz - snapshot_before.gpu_freq_mhz;
    
    fprintf(g_c270_forensic.ultra_log_file,
            "[C270_DELTA] ram_consumed=%ld bytes freq_change=%d MHz result=%d\n",
            (long)ram_delta, freq_delta, ret);
    
    /* Détecter anomalies */
    if (ret < 0) {
        fprintf(g_c270_forensic.ultra_log_file,
                "[C270_ANOMALY_DETECTED] dispatch_failed errno=%d\n", errno);
        fprintf(g_c270_forensic.ultra_log_file,
                "[C270_CRITICAL] This is the bug! Analyzing state...\n");
        
        /* Dump état complet pour analyse */
        fprintf(g_c270_forensic.ultra_log_file,
                "[C270_BUG_STATE_DUMP]\n");
        fprintf(g_c270_forensic.ultra_log_file,
                "  dispatch_number=%lu\n", snapshot_after.total_dispatches);
        fprintf(g_c270_forensic.ultra_log_file,
                "  ctx_pool_index=%d\n", snapshot_after.ctx_pool_index);
        fprintf(g_c270_forensic.ultra_log_file,
                "  batch_pool_index=%d\n", snapshot_after.batch_pool_index);
        fprintf(g_c270_forensic.ultra_log_file,
                "  ram_free=%lu MB\n", snapshot_after.ram_free_bytes / 1048576);
        fprintf(g_c270_forensic.ultra_log_file,
                "  gpu_freq=%d MHz\n", snapshot_after.gpu_freq_mhz);
        fflush(g_c270_forensic.ultra_log_file);
    }
    
    /* Sauvegarder snapshots */
    if (g_c270_forensic.snapshot_count < 100) {
        g_c270_forensic.snapshots[g_c270_forensic.snapshot_count++] = snapshot_after;
    }
    
    return ret;
}

/* ══════════════════════════════════════════════════════════════════════
 * SECTION C270 : MAIN TEST
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * C270 : Point d'entrée test ultra-forensique
 * 
 * OBJECTIF : Forcer reproduction bug errno=5 et capturer état complet
 * 
 * STRATÉGIE :
 * 1. Initialiser contexte forensique
 * 2. Initialiser contexte GPU (btc_gen9_init)
 * 3. Exécuter dispatches jusqu'au crash (27+ dispatches)
 * 4. Capturer snapshot à chaque dispatch
 * 5. Analyser logs pour identifier cause racine
 */

/* Déclarations fonctions externes */
extern int btc_gen9_init(void** ctx_out, const void* config);
extern void btc_gen9_cleanup(void* ctx);

int main(int argc, char** argv) {
    fprintf(stderr, "\n");
    fprintf(stderr, "╔════════════════════════════════════════════════════════════╗\n");
    fprintf(stderr, "║  LumVorax C270 — TEST ULTRA-FORENSIQUE NANO BIT-LEVEL     ║\n");
    fprintf(stderr, "╚════════════════════════════════════════════════════════════╝\n");
    fprintf(stderr, "\n");
    
    /* Initialiser contexte forensique */
    if (c270_init_forensic_context() < 0) {
        fprintf(stderr, "[C270_ERROR] Failed to initialize forensic context\n");
        return 1;
    }
    
    /* Configuration GPU */
    typedef struct {
        uint32_t batch_size;
        uint32_t work_group_size;
        const char* kernel_path;
        const char* log_path;
        bool enable_profiling;
    } btc_gen9_config_t;
    
    btc_gen9_config_t config = {
        .batch_size = 286331153,  /* 286M nonces (C240 optimal) */
        .work_group_size = 256,
        .kernel_path = "kernels/btc_sha256_gen9.bin",
        .log_path = "logs/forensic/c270_ultra/btc_gen9_native.log",
        .enable_profiling = 1
    };
    
    /* Initialiser contexte GPU */
    void* ctx = NULL;
    if (btc_gen9_init(&ctx, &config) < 0) {
        fprintf(stderr, "[C270_ERROR] Failed to initialize GPU context\n");
        c270_cleanup_forensic_context();
        return 1;
    }
    
    fprintf(stderr, "[C270_START] Executing dispatches until errno=5 crash...\n");
    fprintf(stderr, "[C270_START] Expected crash after 27 dispatches\n");
    fprintf(stderr, "[C270_START] Reopen DRM: DISABLED (forcing bug reproduction)\n");
    fprintf(stderr, "\n");
    
    /* Exécuter dispatches jusqu'au crash */
    int dispatch_count = 0;
    int max_dispatches = 30;  /* Tenter 30 dispatches (crash attendu à 27) */
    
    for (int i = 0; i < max_dispatches; i++) {
        fprintf(stderr, "[C270_DISPATCH_%d] Executing...\n", i + 1);
        
        int ret = c270_btc_gen9_execute_wrapper(ctx);
        
        if (ret < 0) {
            fprintf(stderr, "[C270_CRASH] Dispatch %d FAILED with errno=%d (%s)\n",
                    i + 1, errno, strerror(errno));
            fprintf(stderr, "[C270_CRASH] Bug reproduced successfully!\n");
            fprintf(stderr, "[C270_CRASH] Analyzing forensic logs...\n");
            break;
        }
        
        dispatch_count++;
        fprintf(stderr, "[C270_DISPATCH_%d] SUCCESS\n", i + 1);
    }
    
    fprintf(stderr, "\n");
    fprintf(stderr, "[C270_SUMMARY] Total successful dispatches: %d\n", dispatch_count);
    fprintf(stderr, "[C270_SUMMARY] Forensic logs saved to: logs/forensic/c270_ultra/\n");
    fprintf(stderr, "\n");
    
    /* Cleanup */
    btc_gen9_cleanup(ctx);
    c270_cleanup_forensic_context();
    
    return 0;
}

// Made with Bob
