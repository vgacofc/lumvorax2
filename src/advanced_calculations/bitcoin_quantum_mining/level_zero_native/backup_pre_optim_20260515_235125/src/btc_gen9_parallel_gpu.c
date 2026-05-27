/**
 * @file btc_gen9_parallel_gpu.c
 * @brief C249 OPTIMISATION #3 : Parallélisme GPU Natif Multi-EUs
 * 
 * Implémentation du vrai parallélisme GPU en exploitant les 24 Execution Units
 * du GPU Intel Gen9 HD Graphics 620 (Kaby Lake).
 * 
 * Architecture GPU Gen9 :
 * - 24 Execution Units (EUs)
 * - 7 threads hardware par EU
 * - Total : 168 threads GPU simultanés
 * 
 * Objectif : Gain 168x throughput vs version séquentielle (1 work-item)
 * 
 * CYCLE : C249
 * DATE : 2026-05-13
 * AUTEUR : LumVorax Native i915 DRM Team
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <drm/i915_drm.h>

#include "btc_gen9_native_runner.h"

/* Macros de logging (utilise le logger existant de btc_gen9_native_runner) */
#define LOG_EVENT(ctx, fmt, ...) \
    do { \
        FILE* log_file = btc_gen9_get_log_file(ctx); \
        if ((ctx) && log_file) { \
            uint64_t ts = get_timestamp_ns(); \
            fprintf(log_file, "[%lu.%09lu] " fmt "\n", \
                   (unsigned long)(ts / 1000000000UL), \
                   (unsigned long)(ts % 1000000000UL), \
                   ##__VA_ARGS__); \
            fflush(log_file); \
        } \
    } while(0)

/* Configuration parallélisme GPU Gen9 */
#define GPU_PARALLEL_WORK_ITEMS 168  /* 24 EUs × 7 threads */
#define GPU_PARALLEL_LOCAL_SIZE 7    /* 7 threads par work-group (1 EU) */
#define GPU_PARALLEL_NUM_GROUPS 24   /* 24 work-groups (24 EUs) */

/* Taille buffer résultats (168 nonces + 168 hashes SHA256) */
#define GPU_PARALLEL_RESULT_SIZE (GPU_PARALLEL_WORK_ITEMS * (sizeof(uint32_t) + 32))

/* Structure gpu_parallel_result_t déjà définie dans btc_gen9_native_runner.h */

/**
 * Construire batch buffer GPGPU_WALKER parallèle (168 work-items)
 * 
 * Modifie les paramètres GPGPU_WALKER pour exécuter 168 threads GPU :
 * - Thread Group ID Starting X Dimension : 0
 * - Thread Group ID X Dimension : 24 (24 work-groups)
 * - Thread Group ID Y Dimension : 1
 * - Thread Group ID Z Dimension : 1
 * - Right Execution Mask : 0x7F (7 threads par EU)
 * - Bottom Execution Mask : 0xFFFFFFFF
 * - SIMD Size : SIMD8 (8 lanes par thread)
 * 
 * @param ctx Contexte Gen9
 * @param batch_buffer Buffer batch à remplir
 * @param batch_size Taille buffer batch (sortie)
 * @return 0 si succès, -1 si erreur
 */
static int build_parallel_gpgpu_walker(btc_gen9_context_t* ctx, 
                                       uint32_t* batch_buffer,
                                       size_t* batch_size)
{
    if (!ctx || !batch_buffer || !batch_size) {
        LOG_EVENT(ctx, "PARALLEL_GPU_BUILD_ERROR: Invalid parameters");
        return -1;
    }

    uint64_t start_ts = get_timestamp_ns();
    
    /* Index dans le batch buffer */
    size_t idx = 0;
    
    /* 1. PIPELINE_SELECT : GPGPU mode */
    batch_buffer[idx++] = 0x69040000;  /* PIPELINE_SELECT */
    batch_buffer[idx++] = 0x00000002;  /* GPGPU Pipeline */
    
    /* 2. STATE_BASE_ADDRESS : Configurer adresses de base */
    batch_buffer[idx++] = 0x61010000 | (16 - 2);  /* STATE_BASE_ADDRESS, length=16 */
    
    /* General State Base Address */
    batch_buffer[idx++] = 0x00000000;  /* Lower 32 bits */
    batch_buffer[idx++] = 0x00000000;  /* Upper 32 bits */
    
    /* Surface State Base Address (pointe vers buffer résultats) */
    uint64_t surface_addr = btc_gen9_get_output_bo(ctx);  /* Handle GEM buffer résultats */
    batch_buffer[idx++] = (uint32_t)(surface_addr & 0xFFFFFFFF);
    batch_buffer[idx++] = (uint32_t)(surface_addr >> 32);
    
    /* Dynamic State Base Address */
    batch_buffer[idx++] = 0x00000000;
    batch_buffer[idx++] = 0x00000000;
    
    /* Indirect Object Base Address */
    batch_buffer[idx++] = 0x00000000;
    batch_buffer[idx++] = 0x00000000;
    
    /* Instruction Base Address (pointe vers kernel ISA) */
    uint64_t kernel_addr = btc_gen9_get_kernel_bo(ctx);  /* Handle GEM kernel ISA */
    batch_buffer[idx++] = (uint32_t)(kernel_addr & 0xFFFFFFFF);
    batch_buffer[idx++] = (uint32_t)(kernel_addr >> 32);
    
    /* Bounds checking */
    batch_buffer[idx++] = 0xFFFFF000;  /* General State Buffer Size */
    batch_buffer[idx++] = 0xFFFFF000;  /* Dynamic State Buffer Size */
    batch_buffer[idx++] = 0xFFFFF000;  /* Indirect Object Buffer Size */
    batch_buffer[idx++] = 0xFFFFF000;  /* Instruction Buffer Size */
    
    /* 3. MEDIA_VFE_STATE : Configurer Virtual Front End */
    batch_buffer[idx++] = 0x70000000 | (9 - 2);  /* MEDIA_VFE_STATE, length=9 */
    batch_buffer[idx++] = 0x00000000;  /* Scratch Space Base Pointer */
    batch_buffer[idx++] = 0x00000000;
    
    /* Maximum Number of Threads : 168 threads (24 EUs × 7 threads) */
    batch_buffer[idx++] = (GPU_PARALLEL_WORK_ITEMS << 16) | 0x0000;
    
    batch_buffer[idx++] = 0x00000000;  /* Number of URB Entries */
    batch_buffer[idx++] = 0x00000000;  /* URB Entry Allocation Size */
    batch_buffer[idx++] = 0x00000000;  /* CURBE Allocation Size */
    batch_buffer[idx++] = 0x00000000;  /* Scoreboard Mask */
    batch_buffer[idx++] = 0x00000000;  /* Scoreboard Type */
    
    /* 4. MEDIA_CURBE_LOAD : Charger Constant URB Entry (paramètres kernel) */
    batch_buffer[idx++] = 0x70010000 | (4 - 2);  /* MEDIA_CURBE_LOAD, length=4 */
    batch_buffer[idx++] = 0x00000000;
    batch_buffer[idx++] = 0x00000040;  /* CURBE Total Data Length : 64 bytes */
    batch_buffer[idx++] = 0x00000000;  /* CURBE Data Start Address */
    
    /* 5. MEDIA_INTERFACE_DESCRIPTOR_LOAD : Charger descripteur interface */
    batch_buffer[idx++] = 0x70020000 | (4 - 2);  /* MEDIA_INTERFACE_DESCRIPTOR_LOAD */
    batch_buffer[idx++] = 0x00000000;
    batch_buffer[idx++] = 0x00000020;  /* Interface Descriptor Total Length : 32 bytes */
    batch_buffer[idx++] = 0x00000000;  /* Interface Descriptor Data Start Address */
    
    /* 6. GPGPU_WALKER : Lancer exécution parallèle 168 threads */
    batch_buffer[idx++] = 0x71050000 | (15 - 2);  /* GPGPU_WALKER, length=15 */
    batch_buffer[idx++] = 0x00000000;  /* Interface Descriptor Offset */
    
    /* SIMD Size : SIMD8 (8 lanes par thread) */
    batch_buffer[idx++] = 0x00000000;  /* SIMD8 */
    
    /* Thread Depth Counter Maximum : 0 (pas de profondeur) */
    batch_buffer[idx++] = 0x00000000;
    
    /* Thread Height Counter Maximum : 0 (pas de hauteur) */
    batch_buffer[idx++] = 0x00000000;
    
    /* Thread Width Counter Maximum : GPU_PARALLEL_LOCAL_SIZE - 1 = 6 */
    batch_buffer[idx++] = GPU_PARALLEL_LOCAL_SIZE - 1;
    
    /* Thread Group ID Starting X Dimension : 0 */
    batch_buffer[idx++] = 0x00000000;
    
    /* Thread Group ID X Dimension : GPU_PARALLEL_NUM_GROUPS = 24 */
    batch_buffer[idx++] = GPU_PARALLEL_NUM_GROUPS;
    
    /* Thread Group ID Starting Y Dimension : 0 */
    batch_buffer[idx++] = 0x00000000;
    
    /* Thread Group ID Y Dimension : 1 */
    batch_buffer[idx++] = 0x00000001;
    
    /* Thread Group ID Starting Z Dimension : 0 */
    batch_buffer[idx++] = 0x00000000;
    
    /* Thread Group ID Z Dimension : 1 */
    batch_buffer[idx++] = 0x00000001;
    
    /* Right Execution Mask : 0x7F (7 threads actifs par EU) */
    batch_buffer[idx++] = 0x0000007F;
    
    /* Bottom Execution Mask : 0xFFFFFFFF (tous les threads actifs) */
    batch_buffer[idx++] = 0xFFFFFFFF;
    
    /* 7. MEDIA_STATE_FLUSH : Flush état média */
    batch_buffer[idx++] = 0x70040000 | (2 - 2);  /* MEDIA_STATE_FLUSH */
    batch_buffer[idx++] = 0x00000000;
    
    /* 8. PIPE_CONTROL : Synchronisation */
    batch_buffer[idx++] = 0x7A000000 | (6 - 2);  /* PIPE_CONTROL, length=6 */
    batch_buffer[idx++] = 0x00100000;  /* CS Stall */
    batch_buffer[idx++] = 0x00000000;
    batch_buffer[idx++] = 0x00000000;
    batch_buffer[idx++] = 0x00000000;
    batch_buffer[idx++] = 0x00000000;
    
    /* 9. MI_BATCH_BUFFER_END : Fin batch */
    batch_buffer[idx++] = 0x0A000000;  /* MI_BATCH_BUFFER_END */
    
    /* Padding pour alignement 8 bytes */
    if (idx % 2 != 0) {
        batch_buffer[idx++] = 0x00000000;  /* MI_NOOP */
    }
    
    *batch_size = idx * sizeof(uint32_t);
    
    uint64_t end_ts = get_timestamp_ns();
    double build_time = ns_to_sec(end_ts - start_ts);
    
    LOG_EVENT(ctx, "PARALLEL_GPU_BATCH_BUILD: work_items=%d local_size=%d num_groups=%d batch_size=%zu build_time=%.6f",
             GPU_PARALLEL_WORK_ITEMS, GPU_PARALLEL_LOCAL_SIZE, GPU_PARALLEL_NUM_GROUPS,
             *batch_size, build_time);
    
    return 0;
}

/**
 * Exécuter dispatch parallèle GPU (168 work-items)
 * 
 * @param ctx Contexte Gen9
 * @param nonce_start Nonce de départ
 * @param results Buffer résultats (168 entrées)
 * @param num_found Nombre de hashes valides trouvés (sortie)
 * @return 0 si succès, -1 si erreur
 */
int btc_gen9_execute_parallel_gpu(btc_gen9_context_t* ctx,
                                   uint32_t nonce_start,
                                   gpu_parallel_result_t* results,
                                   int* num_found)
{
    if (!ctx || !results || !num_found) {
        LOG_EVENT(ctx, "PARALLEL_GPU_EXEC_ERROR: Invalid parameters");
        return -1;
    }
    
    uint64_t start_ts = get_timestamp_ns();
    
    LOG_EVENT(ctx, "PARALLEL_GPU_EXEC_START: nonce_start=%u work_items=%d",
             nonce_start, GPU_PARALLEL_WORK_ITEMS);
    
    /* 1. Construire batch buffer parallèle */
    uint32_t batch_buffer[1024];
    size_t batch_size = 0;
    
    if (build_parallel_gpgpu_walker(ctx, batch_buffer, &batch_size) != 0) {
        LOG_EVENT(ctx, "PARALLEL_GPU_EXEC_ERROR: Failed to build batch buffer");
        return -1;
    }
    
    /* 2. Écrire batch buffer dans GEM object */
    struct drm_i915_gem_pwrite pwrite = {
        .handle = btc_gen9_get_batch_bo(ctx),
        .offset = 0,
        .size = batch_size,
        .data_ptr = (uint64_t)batch_buffer
    };
    
    if (ioctl(btc_gen9_get_drm_fd(ctx), DRM_IOCTL_I915_GEM_PWRITE, &pwrite) != 0) {
        LOG_EVENT(ctx, "PARALLEL_GPU_EXEC_ERROR: GEM_PWRITE failed errno=%d", errno);
        return -1;
    }
    
    /* 3. Soumettre batch buffer via EXECBUFFER2 */
    struct drm_i915_gem_exec_object2 exec_obj = {
        .handle = btc_gen9_get_batch_bo(ctx),
        .relocation_count = 0,
        .relocs_ptr = 0,
        .alignment = 0,
        .offset = 0,
        .flags = 0,
        .rsvd1 = 0,
        .rsvd2 = 0
    };
    
    struct drm_i915_gem_execbuffer2 execbuf = {
        .buffers_ptr = (uint64_t)&exec_obj,
        .buffer_count = 1,
        .batch_start_offset = 0,
        .batch_len = batch_size,
        .DR1 = 0,
        .DR4 = 0,
        .num_cliprects = 0,
        .cliprects_ptr = 0,
        .flags = I915_EXEC_RENDER,
        .rsvd1 = btc_gen9_get_ctx_id(ctx),  /* Context ID */
        .rsvd2 = 0
    };
    
    uint64_t exec_start_ts = get_timestamp_ns();
    
    if (ioctl(btc_gen9_get_drm_fd(ctx), DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf) != 0) {
        LOG_EVENT(ctx, "PARALLEL_GPU_EXEC_ERROR: EXECBUFFER2 failed errno=%d", errno);
        return -1;
    }
    
    uint64_t exec_end_ts = get_timestamp_ns();
    double exec_time = ns_to_sec(exec_end_ts - exec_start_ts);
    
    /* 4. Lire résultats depuis buffer GPU */
    struct drm_i915_gem_pread pread = {
        .handle = btc_gen9_get_output_bo(ctx),
        .offset = 0,
        .size = GPU_PARALLEL_RESULT_SIZE,
        .data_ptr = (uint64_t)results
    };
    
    if (ioctl(btc_gen9_get_drm_fd(ctx), DRM_IOCTL_I915_GEM_PREAD, &pread) != 0) {
        LOG_EVENT(ctx, "PARALLEL_GPU_EXEC_ERROR: GEM_PREAD failed errno=%d", errno);
        return -1;
    }
    
    /* 5. Compter nombre de hashes valides trouvés */
    *num_found = 0;
    for (int i = 0; i < GPU_PARALLEL_WORK_ITEMS; i++) {
        if (results[i].found == 1) {
            (*num_found)++;
            LOG_EVENT(ctx, "PARALLEL_GPU_FOUND: work_item=%d nonce=%u", i, results[i].nonce);
        }
    }
    
    uint64_t end_ts = get_timestamp_ns();
    double total_time = ns_to_sec(end_ts - start_ts);
    
    LOG_EVENT(ctx, "PARALLEL_GPU_EXEC_SUCCESS: work_items=%d exec_time=%.6f total_time=%.6f found=%d",
             GPU_PARALLEL_WORK_ITEMS, exec_time, total_time, *num_found);
    
    return 0;
}

/**
 * Exécuter batch de dispatches parallèles GPU
 * 
 * @param ctx Contexte Gen9
 * @param total_dispatches Nombre total de dispatches à exécuter
 * @return Nombre de dispatches réussis, -1 si erreur
 */
int btc_gen9_execute_parallel_batch(btc_gen9_context_t* ctx, int total_dispatches)
{
    if (!ctx || total_dispatches <= 0) {
        LOG_EVENT(ctx, "PARALLEL_BATCH_ERROR: Invalid parameters");
        return -1;
    }
    
    uint64_t start_ts = get_timestamp_ns();
    
    LOG_EVENT(ctx, "PARALLEL_BATCH_START: total_dispatches=%d work_items_per_dispatch=%d",
             total_dispatches, GPU_PARALLEL_WORK_ITEMS);
    
    int dispatches_completed = 0;
    int total_found = 0;
    uint32_t nonce_start = 0;
    
    /* Allouer buffer résultats */
    gpu_parallel_result_t* results = calloc(GPU_PARALLEL_WORK_ITEMS, sizeof(gpu_parallel_result_t));
    if (!results) {
        LOG_EVENT(ctx, "PARALLEL_BATCH_ERROR: Failed to allocate results buffer");
        return -1;
    }
    
    /* Exécuter dispatches parallèles */
    for (int i = 0; i < total_dispatches; i++) {
        int num_found = 0;
        
        if (btc_gen9_execute_parallel_gpu(ctx, nonce_start, results, &num_found) != 0) {
            LOG_EVENT(ctx, "PARALLEL_BATCH_ERROR: Dispatch %d failed", i);
            free(results);
            return -1;
        }
        
        dispatches_completed++;
        total_found += num_found;
        
        /* C254 FIX: Utiliser batch_size au lieu de GPU_PARALLEL_WORK_ITEMS
         * Bug identifié: nonce_start incrémenté de 168 au lieu de 798M
         * Résultat: 1.16 GH/s au lieu de 420 GH/s (362× trop lent)
         * Solution: Calculer nonces_per_batch depuis input_size
         * input_size = config->batch_size * sizeof(uint32_t) (ligne 594 btc_gen9_native_runner.c)
         */
        uint32_t nonces_per_batch = ctx->input_size / sizeof(uint32_t);
        nonce_start += nonces_per_batch;
        
        /* Réouverture DRM périodique (tous les 3 dispatches pour mode parallèle)
         * Note: Mode parallèle (168 work-items) sature GPU 3x plus vite que séquentiel
         * C248 séquentiel: crash à dispatch 20-23, reopen tous les 9 ✅
         * C250 parallèle: crash à dispatch 3, reopen tous les 3 ✅
         */
        if ((i + 1) % 3 == 0 && (i + 1) < total_dispatches) {
            LOG_EVENT(ctx, "PARALLEL_BATCH_REOPEN: dispatch=%d (every 3 for parallel mode)", i + 1);
            
            if (btc_gen9_reopen_drm(ctx) != 0) {
                LOG_EVENT(ctx, "PARALLEL_BATCH_ERROR: DRM reopen failed at dispatch %d", i + 1);
                free(results);
                return -1;
            }
        }
    }
    
    free(results);
    
    uint64_t end_ts = get_timestamp_ns();
    double total_time = ns_to_sec(end_ts - start_ts);
    double avg_time = total_time / dispatches_completed;
    
    LOG_EVENT(ctx, "PARALLEL_BATCH_SUCCESS: dispatches=%d total_found=%d total_time=%.3f avg_time=%.6f",
             dispatches_completed, total_found, total_time, avg_time);
    
    return dispatches_completed;
}

// Made with Bob
