/*
 * LumVorax C255V12 — BIT-LEVEL FORENSIC TEST
 * Test ultra-détaillé pour reverse engineering du bug dispatch #28
 * 
 * OBJECTIF: Capturer EXACTEMENT ce qui se passe bit par bit, nanoseconde par nanoseconde
 * - Dump mémoire GPU avant/après chaque dispatch
 * - Dump état contexte DRM complet
 * - Dump batch buffer content
 * - Dump registres GPU (si accessible)
 * - Timestamp nanoseconde précis
 * 
 * HYPOTHÈSE À TESTER: Le bug vient de quelque chose À L'INTÉRIEUR du contexte GPU
 */

#define _GNU_SOURCE
#define _POSIX_C_SOURCE 199309L

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
#include <time.h>
#include <pthread.h>

#include <drm/i915_drm.h>
#include <xf86drm.h>

/* ═══════════════════════════════════════════════════════════════════════
 * SECTION 1: STRUCTURES BIT-LEVEL FORENSIC
 * ═══════════════════════════════════════════════════════════════════════ */

typedef struct {
    uint64_t timestamp_ns;          // Timestamp nanoseconde
    uint32_t dispatch_id;           // Numéro dispatch
    uint32_t ctx_id;                // Context ID utilisé
    uint32_t batch_bo_handle;       // Batch buffer handle
    uint32_t input_bo_handle;       // Input buffer handle
    uint32_t output_bo_handle;      // Output buffer handle
    
    // État contexte DRM
    struct {
        uint32_t ctx_id;
        uint32_t flags;
        uint64_t param_value;       // I915_CONTEXT_PARAM_*
    } ctx_state;
    
    // État batch buffer (premiers 256 bytes)
    uint8_t batch_content[256];
    
    // État mémoire GPU (hash des premiers 1KB)
    uint32_t input_hash;
    uint32_t output_hash;
    
    // Résultat
    int success;                    // 1=succès, 0=échec
    int errno_value;                // errno si échec
    double execution_time_sec;      // Temps d'exécution
    
} forensic_snapshot_t;

/* ═══════════════════════════════════════════════════════════════════════
 * SECTION 2: FONCTIONS UTILITAIRES BIT-LEVEL
 * ═══════════════════════════════════════════════════════════════════════ */

static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static uint32_t hash_memory(const void* data, size_t size) {
    const uint8_t* bytes = (const uint8_t*)data;
    uint32_t hash = 0x811c9dc5; // FNV-1a offset basis
    for (size_t i = 0; i < size; i++) {
        hash ^= bytes[i];
        hash *= 0x01000193; // FNV-1a prime
    }
    return hash;
}

static void dump_hex(FILE* fp, const char* label, const void* data, size_t size) {
    const uint8_t* bytes = (const uint8_t*)data;
    fprintf(fp, "[HEX_DUMP] %s (%zu bytes):\n", label, size);
    for (size_t i = 0; i < size; i += 16) {
        fprintf(fp, "  %04zx: ", i);
        for (size_t j = 0; j < 16 && i + j < size; j++) {
            fprintf(fp, "%02x ", bytes[i + j]);
        }
        fprintf(fp, "\n");
    }
}

static void capture_context_state(int drm_fd, uint32_t ctx_id, forensic_snapshot_t* snap) {
    struct drm_i915_gem_context_param param = {0};
    param.ctx_id = ctx_id;
    
    // Capturer I915_CONTEXT_PARAM_GTT_SIZE
    param.param = I915_CONTEXT_PARAM_GTT_SIZE;
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_GETPARAM, &param) == 0) {
        snap->ctx_state.param_value = param.value;
    }
    
    snap->ctx_state.ctx_id = ctx_id;
    snap->ctx_state.flags = 0; // TODO: Capturer flags si accessible
}

static void capture_batch_content(void* batch_map, forensic_snapshot_t* snap) {
    if (batch_map) {
        memcpy(snap->batch_content, batch_map, 256);
    }
}

static void save_snapshot(FILE* fp, const forensic_snapshot_t* snap) {
    fprintf(fp, "\n═══════════════════════════════════════════════════════════════\n");
    fprintf(fp, "FORENSIC SNAPSHOT — Dispatch #%u\n", snap->dispatch_id);
    fprintf(fp, "═══════════════════════════════════════════════════════════════\n");
    fprintf(fp, "Timestamp: %lu ns\n", snap->timestamp_ns);
    fprintf(fp, "Context ID: %u\n", snap->ctx_id);
    fprintf(fp, "Batch BO Handle: %u\n", snap->batch_bo_handle);
    fprintf(fp, "Input BO Handle: %u\n", snap->input_bo_handle);
    fprintf(fp, "Output BO Handle: %u\n", snap->output_bo_handle);
    fprintf(fp, "\n--- Context State ---\n");
    fprintf(fp, "GTT Size: 0x%lx\n", snap->ctx_state.param_value);
    fprintf(fp, "\n--- Memory Hashes ---\n");
    fprintf(fp, "Input Hash: 0x%08x\n", snap->input_hash);
    fprintf(fp, "Output Hash: 0x%08x\n", snap->output_hash);
    fprintf(fp, "\n--- Result ---\n");
    fprintf(fp, "Success: %s\n", snap->success ? "YES" : "NO");
    if (!snap->success) {
        fprintf(fp, "Errno: %d (%s)\n", snap->errno_value, strerror(snap->errno_value));
    }
    fprintf(fp, "Execution Time: %.6f sec\n", snap->execution_time_sec);
    
    // Dump batch content
    dump_hex(fp, "Batch Buffer Content", snap->batch_content, 256);
    
    fprintf(fp, "═══════════════════════════════════════════════════════════════\n\n");
    fflush(fp);
}

/* ═══════════════════════════════════════════════════════════════════════
 * SECTION 3: TEST PRINCIPAL
 * ═══════════════════════════════════════════════════════════════════════ */

#define MAX_DISPATCHES 30
#define BATCH_SIZE 262144  // 256K nonces (petit pour test rapide)

int main(void) {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  LumVorax C255V12 — BIT-LEVEL FORENSIC TEST               ║\n");
    printf("║  Reverse Engineering du Bug Dispatch #28                  ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");
    
    // Ouvrir log forensique
    FILE* forensic_log = fopen("logs/forensic/c255v12_bit_level.log", "w");
    if (!forensic_log) {
        fprintf(stderr, "❌ Impossible d'ouvrir log forensique\n");
        return 1;
    }
    
    fprintf(forensic_log, "# LumVorax C255V12 — BIT-LEVEL FORENSIC LOG\n");
    fprintf(forensic_log, "# Timestamp: %lu\n", get_timestamp_ns());
    fprintf(forensic_log, "# Max Dispatches: %d\n", MAX_DISPATCHES);
    fprintf(forensic_log, "# Batch Size: %d nonces\n\n", BATCH_SIZE);
    
    // Ouvrir DRM
    int drm_fd = open("/dev/dri/renderD128", O_RDWR);
    if (drm_fd < 0) {
        fprintf(stderr, "❌ Impossible d'ouvrir /dev/dri/renderD128: %s\n", strerror(errno));
        fclose(forensic_log);
        return 1;
    }
    
    printf("✅ DRM ouvert: fd=%d\n", drm_fd);
    
    // Créer contexte principal
    struct drm_i915_gem_context_create ctx_create = {0};
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &ctx_create) < 0) {
        fprintf(stderr, "❌ Impossible de créer contexte: %s\n", strerror(errno));
        close(drm_fd);
        fclose(forensic_log);
        return 1;
    }
    
    uint32_t main_ctx_id = ctx_create.ctx_id;
    printf("✅ Contexte principal créé: ctx_id=%u\n", main_ctx_id);
    
    // Créer pool de 9 contextes
    #define CTX_POOL_SIZE 9
    uint32_t ctx_pool[CTX_POOL_SIZE];
    for (int i = 0; i < CTX_POOL_SIZE; i++) {
        struct drm_i915_gem_context_create ctx = {0};
        if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &ctx) < 0) {
            fprintf(stderr, "❌ Impossible de créer contexte pool[%d]: %s\n", i, strerror(errno));
            close(drm_fd);
            fclose(forensic_log);
            return 1;
        }
        ctx_pool[i] = ctx.ctx_id;
    }
    printf("✅ Pool de %d contextes créé\n", CTX_POOL_SIZE);
    
    // Allouer buffers GPU
    struct drm_i915_gem_create gem_create = {0};
    gem_create.size = BATCH_SIZE * 4; // 4 bytes par nonce
    
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &gem_create) < 0) {
        fprintf(stderr, "❌ Impossible de créer input buffer: %s\n", strerror(errno));
        close(drm_fd);
        fclose(forensic_log);
        return 1;
    }
    uint32_t input_bo = gem_create.handle;
    
    gem_create.size = BATCH_SIZE * 4;
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &gem_create) < 0) {
        fprintf(stderr, "❌ Impossible de créer output buffer: %s\n", strerror(errno));
        close(drm_fd);
        fclose(forensic_log);
        return 1;
    }
    uint32_t output_bo = gem_create.handle;
    
    printf("✅ Buffers GPU créés: input_bo=%u, output_bo=%u\n", input_bo, output_bo);
    
    // Mapper buffers
    struct drm_i915_gem_mmap mmap_arg = {0};
    mmap_arg.handle = input_bo;
    mmap_arg.size = BATCH_SIZE * 4;
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_arg) < 0) {
        fprintf(stderr, "❌ Impossible de mapper input buffer: %s\n", strerror(errno));
        close(drm_fd);
        fclose(forensic_log);
        return 1;
    }
    void* input_map = (void*)(uintptr_t)mmap_arg.addr_ptr;
    
    mmap_arg.handle = output_bo;
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_arg) < 0) {
        fprintf(stderr, "❌ Impossible de mapper output buffer: %s\n", strerror(errno));
        close(drm_fd);
        fclose(forensic_log);
        return 1;
    }
    void* output_map = (void*)(uintptr_t)mmap_arg.addr_ptr;
    
    printf("✅ Buffers mappés\n");
    
    // Créer batch buffer simple (NOP pour test)
    gem_create.size = 4096;
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &gem_create) < 0) {
        fprintf(stderr, "❌ Impossible de créer batch buffer: %s\n", strerror(errno));
        close(drm_fd);
        fclose(forensic_log);
        return 1;
    }
    uint32_t batch_bo = gem_create.handle;
    
    mmap_arg.handle = batch_bo;
    mmap_arg.size = 4096;
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_arg) < 0) {
        fprintf(stderr, "❌ Impossible de mapper batch buffer: %s\n", strerror(errno));
        close(drm_fd);
        fclose(forensic_log);
        return 1;
    }
    void* batch_map = (void*)(uintptr_t)mmap_arg.addr_ptr;
    
    // Remplir batch buffer avec NOP + MI_BATCH_BUFFER_END
    uint32_t* batch_cmds = (uint32_t*)batch_map;
    for (int i = 0; i < 16; i++) {
        batch_cmds[i] = 0x00000000; // MI_NOOP
    }
    batch_cmds[16] = 0x0A000000; // MI_BATCH_BUFFER_END
    
    printf("✅ Batch buffer créé et rempli\n\n");
    
    // BOUCLE DE TEST: 30 dispatches avec forensic complet
    printf("Démarrage test %d dispatches...\n\n", MAX_DISPATCHES);
    
    int success_count = 0;
    int ctx_index = 0;
    
    for (int dispatch = 1; dispatch <= MAX_DISPATCHES; dispatch++) {
        forensic_snapshot_t snap = {0};
        snap.timestamp_ns = get_timestamp_ns();
        snap.dispatch_id = dispatch;
        snap.ctx_id = ctx_pool[ctx_index];
        snap.batch_bo_handle = batch_bo;
        snap.input_bo_handle = input_bo;
        snap.output_bo_handle = output_bo;
        
        printf("[Dispatch %d/%d] ctx_id=%u batch_bo=%u\n", 
               dispatch, MAX_DISPATCHES, snap.ctx_id, snap.batch_bo_handle);
        
        // CAPTURE PRE-DISPATCH
        capture_context_state(drm_fd, snap.ctx_id, &snap);
        capture_batch_content(batch_map, &snap);
        snap.input_hash = hash_memory(input_map, 1024);
        snap.output_hash = hash_memory(output_map, 1024);
        
        // EXÉCUTION
        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);
        
        struct drm_i915_gem_execbuffer2 execbuf = {0};
        struct drm_i915_gem_exec_object2 exec_obj[3] = {0};
        
        exec_obj[0].handle = input_bo;
        exec_obj[1].handle = output_bo;
        exec_obj[2].handle = batch_bo;
        
        execbuf.buffers_ptr = (uintptr_t)exec_obj;
        execbuf.buffer_count = 3;
        execbuf.batch_len = 68; // 17 commandes × 4 bytes
        execbuf.rsvd1 = snap.ctx_id;
        
        int ret = ioctl(drm_fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf);
        
        clock_gettime(CLOCK_MONOTONIC, &end);
        snap.execution_time_sec = (end.tv_sec - start.tv_sec) + 
                                   (end.tv_nsec - start.tv_nsec) / 1e9;
        
        if (ret == 0) {
            snap.success = 1;
            success_count++;
            printf("  ✅ Succès (%.6f sec)\n", snap.execution_time_sec);
        } else {
            snap.success = 0;
            snap.errno_value = errno;
            printf("  ❌ ÉCHEC errno=%d (%s)\n", errno, strerror(errno));
        }
        
        // CAPTURE POST-DISPATCH
        snap.input_hash = hash_memory(input_map, 1024);
        snap.output_hash = hash_memory(output_map, 1024);
        
        // SAUVEGARDER SNAPSHOT
        save_snapshot(forensic_log, &snap);
        
        // Rotation contexte
        ctx_index = (ctx_index + 1) % CTX_POOL_SIZE;
        
        // Si échec, arrêter
        if (!snap.success) {
            printf("\n⚠️  Arrêt après échec au dispatch #%d\n", dispatch);
            break;
        }
        
        usleep(10000); // Pause 10ms entre dispatches
    }
    
    printf("\n");
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("RÉSULTATS FINAUX\n");
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("Dispatches réussis: %d/%d\n", success_count, MAX_DISPATCHES);
    printf("Log forensique: logs/forensic/c255v12_bit_level.log\n");
    printf("═══════════════════════════════════════════════════════════════\n\n");
    
    // Cleanup
    munmap(input_map, BATCH_SIZE * 4);
    munmap(output_map, BATCH_SIZE * 4);
    munmap(batch_map, 4096);
    
    struct drm_gem_close gem_close = {0};
    gem_close.handle = input_bo;
    ioctl(drm_fd, DRM_IOCTL_GEM_CLOSE, &gem_close);
    gem_close.handle = output_bo;
    ioctl(drm_fd, DRM_IOCTL_GEM_CLOSE, &gem_close);
    gem_close.handle = batch_bo;
    ioctl(drm_fd, DRM_IOCTL_GEM_CLOSE, &gem_close);
    
    for (int i = 0; i < CTX_POOL_SIZE; i++) {
        struct drm_i915_gem_context_destroy ctx_destroy = {0};
        ctx_destroy.ctx_id = ctx_pool[i];
        ioctl(drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_DESTROY, &ctx_destroy);
    }
    
    struct drm_i915_gem_context_destroy ctx_destroy = {0};
    ctx_destroy.ctx_id = main_ctx_id;
    ioctl(drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_DESTROY, &ctx_destroy);
    
    close(drm_fd);
    fclose(forensic_log);
    
    return (success_count == MAX_DISPATCHES) ? 0 : 1;
}

// Made with Bob
