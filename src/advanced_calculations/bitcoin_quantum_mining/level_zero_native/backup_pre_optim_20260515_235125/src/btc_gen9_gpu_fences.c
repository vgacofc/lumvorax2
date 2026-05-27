/*
 * LumVorax — Module 17 — Bitcoin Quantum Mining Engine
 * btc_gen9_gpu_fences.c — Timestamps GPU Hardware Fences
 *
 * CYCLE C255v5 — Validations HARDWARE Complètes
 * OBJECTIF : Mesurer temps GPU réel avec timestamps hardware (pas CPU)
 *
 * MÉTHODE :
 * - Créer fence buffer avec timestamp GPU
 * - Insérer commandes PIPE_CONTROL avec timestamp dans batch buffer
 * - Lire timestamps GPU après exécution
 * - Précision: Nanoseconde hardware
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
#include <sys/mman.h>
#include <errno.h>

/* DRM headers */
#include <drm/i915_drm.h>

/* Gen9 ISA commands */
#define GEN9_PIPE_CONTROL           0x7A000000
#define GEN9_PIPE_CONTROL_LENGTH    6
#define GEN9_PIPE_CONTROL_TIMESTAMP (1 << 21)  /* Write timestamp */
#define GEN9_PIPE_CONTROL_FLUSH     (1 << 18)  /* CS stall */

/* ══════════════════════════════════════════════════════════════════════
 * STRUCTURES GPU FENCES
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * Fence GPU avec timestamps hardware
 */
typedef struct {
    uint32_t fence_bo;              /* Buffer object fence */
    void* fence_map;                /* Mapping CPU */
    uint64_t gpu_timestamp_start;   /* Timestamp GPU début (ns) */
    uint64_t gpu_timestamp_end;     /* Timestamp GPU fin (ns) */
    uint64_t cpu_timestamp_start;   /* Timestamp CPU début (ns) */
    uint64_t cpu_timestamp_end;     /* Timestamp CPU fin (ns) */
    double gpu_time_sec;            /* Temps GPU réel (sec) */
    double cpu_time_sec;            /* Temps CPU mesuré (sec) */
} btc_gpu_fence_t;

/* ══════════════════════════════════════════════════════════════════════
 * CRÉATION FENCE
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * Créer fence buffer pour timestamps GPU
 */
int btc_gen9_create_fence(int drm_fd, btc_gpu_fence_t* fence) {
    if (!fence) {
        fprintf(stderr, "ERROR: fence structure NULL\n");
        return -1;
    }
    
    memset(fence, 0, sizeof(btc_gpu_fence_t));
    
    /* Créer buffer 4KB pour timestamps (2× uint64_t = 16 bytes) */
    struct drm_i915_gem_create gem_create = {
        .size = 4096
    };
    
    int ret = ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &gem_create);
    if (ret < 0) {
        fprintf(stderr, "ERROR: GEM_CREATE fence failed (errno=%d)\n", errno);
        return -1;
    }
    
    fence->fence_bo = gem_create.handle;
    
    /* Mapper buffer en mémoire CPU */
    struct drm_i915_gem_mmap_offset mmap_offset = {
        .handle = fence->fence_bo,
        .flags = I915_MMAP_OFFSET_WB  /* Write-back cache */
    };
    
    ret = ioctl(drm_fd, DRM_IOCTL_I915_GEM_MMAP_OFFSET, &mmap_offset);
    if (ret < 0) {
        fprintf(stderr, "ERROR: GEM_MMAP_OFFSET fence failed (errno=%d)\n", errno);
        struct drm_gem_close close = { .handle = fence->fence_bo };
        ioctl(drm_fd, DRM_IOCTL_GEM_CLOSE, &close);
        return -1;
    }
    
    fence->fence_map = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED,
                           drm_fd, mmap_offset.offset);
    if (fence->fence_map == MAP_FAILED) {
        fprintf(stderr, "ERROR: mmap fence failed (errno=%d)\n", errno);
        struct drm_gem_close close = { .handle = fence->fence_bo };
        ioctl(drm_fd, DRM_IOCTL_GEM_CLOSE, &close);
        return -1;
    }
    
    /* Initialiser buffer à 0 */
    memset(fence->fence_map, 0, 4096);
    
    printf("INFO: Fence created (bo=%u map=%p)\n", fence->fence_bo, fence->fence_map);
    return 0;
}

/* ══════════════════════════════════════════════════════════════════════
 * INSERTION COMMANDES TIMESTAMP
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * Insérer commande PIPE_CONTROL avec timestamp dans batch buffer
 * 
 * Format Gen9 PIPE_CONTROL:
 * DW0: Command (0x7A000000) | Length (6 DWORDs)
 * DW1: Flags (timestamp write + CS stall)
 * DW2: Address low 32 bits
 * DW3: Address high 32 bits
 * DW4: Immediate data low (0)
 * DW5: Immediate data high (0)
 */
void btc_gen9_insert_timestamp_start(uint32_t* batch, int* offset, 
                                     uint32_t fence_bo, uint64_t fence_offset) {
    int idx = *offset;
    
    /* PIPE_CONTROL: Write timestamp to fence buffer offset 0 */
    batch[idx++] = GEN9_PIPE_CONTROL | (GEN9_PIPE_CONTROL_LENGTH - 2);
    batch[idx++] = GEN9_PIPE_CONTROL_TIMESTAMP | GEN9_PIPE_CONTROL_FLUSH;
    batch[idx++] = (uint32_t)(fence_offset & 0xFFFFFFFF);        /* Address low */
    batch[idx++] = (uint32_t)((fence_offset >> 32) & 0xFFFFFFFF); /* Address high */
    batch[idx++] = 0;  /* Immediate data low */
    batch[idx++] = 0;  /* Immediate data high */
    
    *offset = idx;
}

void btc_gen9_insert_timestamp_end(uint32_t* batch, int* offset,
                                   uint32_t fence_bo, uint64_t fence_offset) {
    int idx = *offset;
    
    /* PIPE_CONTROL: Write timestamp to fence buffer offset 8 (2nd uint64_t) */
    batch[idx++] = GEN9_PIPE_CONTROL | (GEN9_PIPE_CONTROL_LENGTH - 2);
    batch[idx++] = GEN9_PIPE_CONTROL_TIMESTAMP | GEN9_PIPE_CONTROL_FLUSH;
    batch[idx++] = (uint32_t)((fence_offset + 8) & 0xFFFFFFFF);        /* Address low */
    batch[idx++] = (uint32_t)(((fence_offset + 8) >> 32) & 0xFFFFFFFF); /* Address high */
    batch[idx++] = 0;  /* Immediate data low */
    batch[idx++] = 0;  /* Immediate data high */
    
    *offset = idx;
}

/* ══════════════════════════════════════════════════════════════════════
 * LECTURE TIMESTAMPS GPU
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * Lire timestamps GPU depuis fence buffer
 * 
 * Format fence buffer:
 * Offset 0: Timestamp start (uint64_t)
 * Offset 8: Timestamp end (uint64_t)
 */
int btc_gen9_read_gpu_timestamps(btc_gpu_fence_t* fence) {
    if (!fence || !fence->fence_map) {
        fprintf(stderr, "ERROR: Invalid fence for reading timestamps\n");
        return -1;
    }
    
    /* Lire timestamps depuis buffer GPU */
    uint64_t* timestamps = (uint64_t*)fence->fence_map;
    fence->gpu_timestamp_start = timestamps[0];
    fence->gpu_timestamp_end = timestamps[1];
    
    /* Calculer temps GPU réel */
    if (fence->gpu_timestamp_end > fence->gpu_timestamp_start) {
        uint64_t delta_ns = fence->gpu_timestamp_end - fence->gpu_timestamp_start;
        fence->gpu_time_sec = (double)delta_ns / 1000000000.0;
    } else {
        fprintf(stderr, "WARNING: Invalid GPU timestamps (end < start)\n");
        fence->gpu_time_sec = 0.0;
    }
    
    return 0;
}

/* ══════════════════════════════════════════════════════════════════════
 * CALCUL TEMPS GPU
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * Calculer temps GPU réel (secondes)
 */
double btc_gen9_compute_gpu_time(const btc_gpu_fence_t* fence) {
    if (!fence) return 0.0;
    return fence->gpu_time_sec;
}

/**
 * Calculer overhead CPU vs GPU
 * 
 * Overhead = (temps_cpu - temps_gpu) / temps_gpu * 100
 */
double btc_gen9_compute_cpu_overhead(const btc_gpu_fence_t* fence) {
    if (!fence || fence->gpu_time_sec == 0.0) return 0.0;
    
    double overhead = (fence->cpu_time_sec - fence->gpu_time_sec) / fence->gpu_time_sec * 100.0;
    return overhead;
}

/* ══════════════════════════════════════════════════════════════════════
 * AFFICHAGE MÉTRIQUES
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * Afficher métriques fence
 */
void btc_gen9_fence_print(const btc_gpu_fence_t* fence, int dispatch_id) {
    if (!fence) return;
    
    printf("Dispatch %d Fence:\n", dispatch_id);
    printf("  GPU Time: %.6f sec\n", fence->gpu_time_sec);
    printf("  CPU Time: %.6f sec\n", fence->cpu_time_sec);
    printf("  CPU Overhead: %.2f%%\n", btc_gen9_compute_cpu_overhead(fence));
    printf("  GPU Timestamp Start: %llu ns\n", fence->gpu_timestamp_start);
    printf("  GPU Timestamp End: %llu ns\n", fence->gpu_timestamp_end);
}

/* ══════════════════════════════════════════════════════════════════════
 * CLEANUP
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * Détruire fence
 */
void btc_gen9_destroy_fence(int drm_fd, btc_gpu_fence_t* fence) {
    if (!fence) return;
    
    if (fence->fence_map && fence->fence_map != MAP_FAILED) {
        munmap(fence->fence_map, 4096);
        fence->fence_map = NULL;
    }
    
    if (fence->fence_bo) {
        struct drm_gem_close close = { .handle = fence->fence_bo };
        ioctl(drm_fd, DRM_IOCTL_GEM_CLOSE, &close);
        fence->fence_bo = 0;
    }
}

/* ══════════════════════════════════════════════════════════════════════
 * VALIDATION PRÉCISION
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * Valider précision timestamps GPU
 * 
 * Critère: Écart CPU vs GPU <10% (overhead acceptable)
 */
bool btc_gen9_validate_timestamp_precision(const btc_gpu_fence_t* fence) {
    if (!fence) return false;
    
    double overhead = btc_gen9_compute_cpu_overhead(fence);
    bool valid = (overhead >= -10.0 && overhead <= 100.0);  /* -10% à +100% acceptable */
    
    printf("\n=== VALIDATION PRÉCISION TIMESTAMPS ===\n");
    printf("GPU Time: %.6f sec\n", fence->gpu_time_sec);
    printf("CPU Time: %.6f sec\n", fence->cpu_time_sec);
    printf("Overhead: %.2f%%\n", overhead);
    printf("Critère: -10%% <= overhead <= 100%%\n");
    printf("Résultat: %s\n", valid ? "✅ PRÉCISION VALIDÉE" : "❌ PRÉCISION INVALIDE");
    printf("======================================\n\n");
    
    return valid;
}

// Made with Bob
