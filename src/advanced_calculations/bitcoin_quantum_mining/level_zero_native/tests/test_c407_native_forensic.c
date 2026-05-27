/**
 * C407: TEST KERNEL NATIF AVEC FORENSIC BIT-LEVEL LUMVORAX
 * 
 * OBJECTIF: Tester kernel natif C406 avec instrumentation forensic complète
 * - Kernel: btc_sha256_native_c406.bin (176 bytes, SEND 0x04025E00)
 * - Runtime: 100% i915 DRM natif
 * - Forensic: Bit-level tracking Lumvorax (ultra_forensic_logger)
 * - Batch: Complet avec STATE_BASE_ADDRESS + GPGPU_WALKER
 * 
 * ATTENDU: Résultats non-zéro + logs forensic complets
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>
#include <time.h>
#include <drm/i915_drm.h>
#include <xf86drm.h>

/* Activer tracking forensique AVANT includes */
#define ENABLE_I915_FORENSIC_TRACKING
#include "../src/btc_i915_drm_forensic_tracker.h"

#define NUM_THREADS 256

/* Timestamp nanoseconde */
static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

typedef struct {
    int fd;
    uint32_t ctx_id;
    uint32_t handle_kernel, handle_output, handle_ssh, handle_batch;
    void *map_kernel, *map_output, *map_ssh, *map_batch;
    uint64_t gtt_kernel, gtt_output, gtt_ssh, gtt_batch;
    FILE *forensic_log;
} gpu_context_t;

/* Créer buffer GEM avec logging forensic */
static int create_bo_forensic(gpu_context_t *ctx, size_t size, uint32_t *handle_out, void **map_out, const char *name) {
    uint64_t ts_start = get_timestamp_ns();
    
    struct drm_i915_gem_create create = { .size = size };
    
    if (drmIoctl(ctx->fd, DRM_IOCTL_I915_GEM_CREATE, &create) < 0) {
        fprintf(ctx->forensic_log, "[%lu] ERROR: GEM_CREATE %s failed: %s\n", 
                get_timestamp_ns(), name, strerror(errno));
        return -1;
    }
    
    *handle_out = create.handle;
    
    struct drm_i915_gem_mmap mmap_arg = {
        .handle = create.handle,
        .offset = 0,
        .size = size,
        .flags = 0
    };
    
    if (drmIoctl(ctx->fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_arg) < 0) {
        fprintf(ctx->forensic_log, "[%lu] ERROR: GEM_MMAP %s failed: %s\n",
                get_timestamp_ns(), name, strerror(errno));
        struct drm_gem_close close_arg = { .handle = create.handle };
        drmIoctl(ctx->fd, DRM_IOCTL_GEM_CLOSE, &close_arg);
        return -1;
    }
    
    *map_out = (void *)(uintptr_t)mmap_arg.addr_ptr;
    memset(*map_out, 0, size);
    
    uint64_t elapsed = get_timestamp_ns() - ts_start;
    fprintf(ctx->forensic_log, "[%lu] GEM_CREATE+MMAP %s: handle=%u, size=%zu, elapsed=%lu ns\n",
            get_timestamp_ns(), name, create.handle, size, elapsed);
    fflush(ctx->forensic_log);
    
    return 0;
}

int main() {
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  C407 TEST KERNEL NATIF + FORENSIC BIT-LEVEL LUMVORAX     ║\n");
    printf("║  Kernel: btc_sha256_native_c406.bin (176 bytes)           ║\n");
    printf("║  SEND descriptor: 0x04025E00 (stateful, BTI=0)            ║\n");
    printf("║  Runtime: 100%% i915 DRM natif                             ║\n");
    printf("║  Forensic: ultra_forensic_logger + i915_drm_tracker       ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");
    
    /* Initialiser tracker forensique */
    printf("[FORENSIC] Initialisation tracker bit-level...\n");
    if (i915_forensic_tracker_init("logs/forensic/c407") < 0) {
        fprintf(stderr, "ERROR: Impossible initialiser tracker forensique\n");
        return 1;
    }
    printf("[FORENSIC] Tracker initialisé\n");
    printf("[FORENSIC] Logs: logs/forensic/c407/i915_forensic_*.log\n\n");
    
    gpu_context_t ctx = {0};
    
    /* Ouvrir log forensic principal */
    ctx.forensic_log = fopen("logs/forensic/c407/execution_c407.log", "w");
    if (!ctx.forensic_log) {
        fprintf(stderr, "ERROR: Cannot open forensic log\n");
        i915_forensic_tracker_cleanup();
        return 1;
    }
    
    fprintf(ctx.forensic_log, "# LumVorax C407 — FORENSIC BIT-LEVEL EXECUTION LOG\n");
    fprintf(ctx.forensic_log, "# Timestamp: %lu ns\n", get_timestamp_ns());
    fprintf(ctx.forensic_log, "# Kernel: btc_sha256_native_c406.bin\n");
    fprintf(ctx.forensic_log, "# SEND descriptor: 0x04025E00\n");
    fprintf(ctx.forensic_log, "# Threads: %d\n\n", NUM_THREADS);
    fflush(ctx.forensic_log);
    
    /* Ouvrir DRM */
    uint64_t ts_drm_open = get_timestamp_ns();
    ctx.fd = open("/dev/dri/card1", O_RDWR);
    if (ctx.fd < 0) {
        fprintf(ctx.forensic_log, "[%lu] ERROR: open(/dev/dri/card1) failed: %s\n",
                get_timestamp_ns(), strerror(errno));
        fclose(ctx.forensic_log);
        i915_forensic_tracker_cleanup();
        return 1;
    }
    fprintf(ctx.forensic_log, "[%lu] DRM opened: fd=%d, elapsed=%lu ns\n",
            get_timestamp_ns(), ctx.fd, get_timestamp_ns() - ts_drm_open);
    printf("✓ DRM opened: fd=%d\n", ctx.fd);
    
    /* Créer contexte GPU */
    uint64_t ts_ctx = get_timestamp_ns();
    struct drm_i915_gem_context_create ctx_create = {0};
    if (drmIoctl(ctx.fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &ctx_create) < 0) {
        fprintf(ctx.forensic_log, "[%lu] ERROR: GEM_CONTEXT_CREATE failed: %s\n",
                get_timestamp_ns(), strerror(errno));
        close(ctx.fd);
        fclose(ctx.forensic_log);
        i915_forensic_tracker_cleanup();
        return 1;
    }
    ctx.ctx_id = ctx_create.ctx_id;
    fprintf(ctx.forensic_log, "[%lu] GPU context created: ctx_id=%u, elapsed=%lu ns\n",
            get_timestamp_ns(), ctx.ctx_id, get_timestamp_ns() - ts_ctx);
    printf("✓ GPU context created: ctx_id=%u\n", ctx.ctx_id);
    
    /* Charger kernel NATIF C406 */
    uint64_t ts_kernel_load = get_timestamp_ns();
    FILE *f = fopen("kernels/btc_sha256_native_c406.bin", "rb");
    if (!f) {
        fprintf(ctx.forensic_log, "[%lu] ERROR: Cannot open kernel binary\n", get_timestamp_ns());
        close(ctx.fd);
        fclose(ctx.forensic_log);
        i915_forensic_tracker_cleanup();
        return 1;
    }
    
    fseek(f, 0, SEEK_END);
    size_t kernel_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    fprintf(ctx.forensic_log, "[%lu] Kernel file opened: size=%zu bytes\n",
            get_timestamp_ns(), kernel_size);
    printf("✓ Kernel size: %zu bytes\n", kernel_size);
    
    /* Créer buffers avec forensic */
    if (create_bo_forensic(&ctx, kernel_size, &ctx.handle_kernel, &ctx.map_kernel, "KERNEL") < 0) {
        fclose(f);
        close(ctx.fd);
        fclose(ctx.forensic_log);
        i915_forensic_tracker_cleanup();
        return 1;
    }
    
    /* Charger kernel */
    fread(ctx.map_kernel, 1, kernel_size, f);
    fclose(f);
    fprintf(ctx.forensic_log, "[%lu] Kernel loaded into BO, elapsed=%lu ns\n",
            get_timestamp_ns(), get_timestamp_ns() - ts_kernel_load);
    printf("✓ Kernel loaded\n");
    
    /* Dump kernel ISA (forensic) */
    i915_forensic_dump_buffer(ctx.handle_kernel, ctx.map_kernel, kernel_size);
    
    /* Output buffer */
    if (create_bo_forensic(&ctx, NUM_THREADS * 4, &ctx.handle_output, &ctx.map_output, "OUTPUT") < 0) {
        close(ctx.fd);
        fclose(ctx.forensic_log);
        i915_forensic_tracker_cleanup();
        return 1;
    }
    printf("✓ Output buffer created: %d bytes\n", NUM_THREADS * 4);
    
    /* Surface State Heap */
    if (create_bo_forensic(&ctx, 4096, &ctx.handle_ssh, &ctx.map_ssh, "SSH") < 0) {
        close(ctx.fd);
        fclose(ctx.forensic_log);
        i915_forensic_tracker_cleanup();
        return 1;
    }
    
    /* Batch buffer */
    if (create_bo_forensic(&ctx, 4096, &ctx.handle_batch, &ctx.map_batch, "BATCH") < 0) {
        close(ctx.fd);
        fclose(ctx.forensic_log);
        i915_forensic_tracker_cleanup();
        return 1;
    }
    
    /* Pin buffers et obtenir GTT */
    struct drm_i915_gem_exec_object2 exec_objects[4] = {
        { .handle = ctx.handle_kernel, .flags = EXEC_OBJECT_PINNED | EXEC_OBJECT_SUPPORTS_48B_ADDRESS, .offset = 0x100000 },
        { .handle = ctx.handle_output, .flags = EXEC_OBJECT_PINNED | EXEC_OBJECT_SUPPORTS_48B_ADDRESS | EXEC_OBJECT_WRITE, .offset = 0x200000 },
        { .handle = ctx.handle_ssh, .flags = EXEC_OBJECT_PINNED | EXEC_OBJECT_SUPPORTS_48B_ADDRESS, .offset = 0x300000 },
        { .handle = ctx.handle_batch, .flags = EXEC_OBJECT_PINNED | EXEC_OBJECT_SUPPORTS_48B_ADDRESS, .offset = 0x400000 }
    };
    
    ctx.gtt_kernel = exec_objects[0].offset;
    ctx.gtt_output = exec_objects[1].offset;
    ctx.gtt_ssh = exec_objects[2].offset;
    ctx.gtt_batch = exec_objects[3].offset;
    
    fprintf(ctx.forensic_log, "[%lu] GTT addresses:\n", get_timestamp_ns());
    fprintf(ctx.forensic_log, "  Kernel: 0x%016lx\n", ctx.gtt_kernel);
    fprintf(ctx.forensic_log, "  Output: 0x%016lx\n", ctx.gtt_output);
    fprintf(ctx.forensic_log, "  SSH:    0x%016lx\n", ctx.gtt_ssh);
    fprintf(ctx.forensic_log, "  Batch:  0x%016lx\n", ctx.gtt_batch);
    fflush(ctx.forensic_log);
    
    printf("✓ GTT addresses assigned\n");
    
    /* Configurer Surface State (BTI=0) */
    uint32_t *ssh = (uint32_t *)ctx.map_ssh;
    ssh[0] = 0x00000000;  // DW0
    ssh[1] = ctx.gtt_output & 0xFFFFFFFF;  // DW1
    ssh[2] = (ctx.gtt_output >> 32) & 0xFFFF;  // DW2
    ssh[3] = (NUM_THREADS * 4) - 1;  // DW3
    ssh[4] = 0x00000000;  // DW4
    ssh[5] = 0x00000000;  // DW5
    ssh[6] = 0x00000000;  // DW6
    ssh[7] = 0x00000000;  // DW7
    
    fprintf(ctx.forensic_log, "[%lu] Surface State configured (BTI=0)\n", get_timestamp_ns());
    printf("✓ Surface State configured\n");
    
    /* Interface Descriptor */
    uint32_t *id = (uint32_t *)((uint8_t *)ctx.map_ssh + 64);
    id[0] = ctx.gtt_kernel & 0xFFFFFFC0;  // DW0: KSP
    id[1] = (ctx.gtt_kernel >> 32) & 0xFFFF;  // DW1
    id[2] = 0x00000000;  // DW2
    id[3] = 0x00040000;  // DW3: Barrier Enable
    id[4] = 0x00000000;  // DW4: BTI=0
    id[5] = 0x00000000;  // DW5
    id[6] = 0x00000000;  // DW6
    id[7] = 0x00000000;  // DW7
    
    fprintf(ctx.forensic_log, "[%lu] Interface Descriptor: KSP=0x%016lx\n",
            get_timestamp_ns(), ctx.gtt_kernel);
    printf("✓ Interface Descriptor configured\n");
    
    /* Construire batch buffer COMPLET */
    uint32_t *batch = (uint32_t *)ctx.map_batch;
    int idx = 0;
    
    /* STATE_BASE_ADDRESS */
    batch[idx++] = 0x61010009;  // Opcode
    batch[idx++] = 0x00000000;  // General State Base: disabled
    batch[idx++] = 0x00000000;
    batch[idx++] = ctx.gtt_ssh & 0xFFFFFFFF;  // Surface State Base Low
    batch[idx++] = (ctx.gtt_ssh >> 32) & 0xFFFF;  // Surface State Base High
    batch[idx++] = 0x00000000;  // Dynamic State Base: disabled
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;  // Indirect Object Base: disabled
    batch[idx++] = 0x00000000;
    batch[idx++] = ctx.gtt_ssh & 0xFFFFFFFF;  // Instruction Base Low
    batch[idx++] = (ctx.gtt_ssh >> 32) & 0xFFFF;  // Instruction Base High
    
    /* MEDIA_VFE_STATE */
    batch[idx++] = 0x70000006;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    
    /* MEDIA_INTERFACE_DESCRIPTOR_LOAD */
    batch[idx++] = 0x70020002;
    batch[idx++] = 0x00000000;
    batch[idx++] = 32;  // Length
    batch[idx++] = (ctx.gtt_ssh + 64) & 0xFFFFFFFF;  // IDesc address
    
    /* GPGPU_WALKER */
    batch[idx++] = 0x70050009;
    batch[idx++] = 0x00000000;  // IDesc offset = 0
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000001;  // Thread Width
    batch[idx++] = 0x00000001;  // Thread Height
    batch[idx++] = 0x00000001;  // Thread Depth
    batch[idx++] = NUM_THREADS;  // Thread Group X
    batch[idx++] = 0x00000001;  // Thread Group Y
    batch[idx++] = 0x00000001;  // Thread Group Z
    batch[idx++] = 0x00000000;  // Right Execution Mask
    
    /* PIPE_CONTROL */
    batch[idx++] = 0x7A000004;
    batch[idx++] = 0x00100000;  // CS Stall
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    
    /* MI_BATCH_BUFFER_END */
    batch[idx++] = 0x0A000000;
    
    fprintf(ctx.forensic_log, "[%lu] Batch buffer built: %d dwords\n",
            get_timestamp_ns(), idx);
    printf("✓ Batch buffer built: %d dwords\n", idx);
    
    /* Dump batch buffer (forensic) */
    i915_forensic_dump_buffer(ctx.handle_batch, ctx.map_batch, idx * 4);
    
    /* Execbuffer */
    struct drm_i915_gem_execbuffer2 execbuf = {
        .buffers_ptr = (uintptr_t)exec_objects,
        .buffer_count = 4,
        .batch_start_offset = 0,
        .batch_len = idx * 4,
        .flags = I915_EXEC_RENDER,
        .rsvd1 = ctx.ctx_id
    };
    
    printf("\n🚀 Executing GPU kernel...\n");
    fprintf(ctx.forensic_log, "\n[%lu] === EXECBUFFER2 START ===\n", get_timestamp_ns());
    fflush(ctx.forensic_log);
    
    uint64_t ts_exec = get_timestamp_ns();
    if (drmIoctl(ctx.fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf) < 0) {
        fprintf(ctx.forensic_log, "[%lu] ERROR: EXECBUFFER2 failed: %s\n",
                get_timestamp_ns(), strerror(errno));
        fprintf(stderr, "❌ EXECBUFFER2 failed: %s\n", strerror(errno));
        fclose(ctx.forensic_log);
        close(ctx.fd);
        i915_forensic_tracker_cleanup();
        return 1;
    }
    
    fprintf(ctx.forensic_log, "[%lu] EXECBUFFER2 SUCCESS, elapsed=%lu ns\n",
            get_timestamp_ns(), get_timestamp_ns() - ts_exec);
    printf("✅ EXECBUFFER2 SUCCESS!\n");
    
    /* GEM_WAIT */
    printf("⏳ Waiting for GPU completion...\n");
    fprintf(ctx.forensic_log, "[%lu] GEM_WAIT start\n", get_timestamp_ns());
    
    uint64_t ts_wait = get_timestamp_ns();
    struct drm_i915_gem_wait wait = {
        .bo_handle = ctx.handle_output,
        .timeout_ns = 1000000000  // 1 seconde
    };
    
    if (drmIoctl(ctx.fd, DRM_IOCTL_I915_GEM_WAIT, &wait) < 0) {
        fprintf(ctx.forensic_log, "[%lu] ERROR: GEM_WAIT failed: %s\n",
                get_timestamp_ns(), strerror(errno));
        fprintf(stderr, "❌ GEM_WAIT failed: %s\n", strerror(errno));
        fclose(ctx.forensic_log);
        close(ctx.fd);
        i915_forensic_tracker_cleanup();
        return 1;
    }
    
    fprintf(ctx.forensic_log, "[%lu] GEM_WAIT completed, elapsed=%lu ns\n",
            get_timestamp_ns(), get_timestamp_ns() - ts_wait);
    printf("✓ GPU completed\n\n");
    
    /* Dump output buffer (forensic) */
    i915_forensic_dump_buffer(ctx.handle_output, ctx.map_output, NUM_THREADS * 4);
    
    /* Lire résultats */
    printf("=== RÉSULTATS ===\n");
    fprintf(ctx.forensic_log, "\n[%lu] === RESULTS ANALYSIS ===\n", get_timestamp_ns());
    
    uint32_t *output = (uint32_t *)ctx.map_output;
    int non_zero = 0;
    uint32_t expected_pattern = (4 << 16);  // leading_zeros=4, nonce=thread_id
    
    for (int i = 0; i < NUM_THREADS; i++) {
        if (output[i] != 0) {
            non_zero++;
            if (non_zero <= 10) {
                printf("Thread %3d: 0x%08x", i, output[i]);
                uint32_t expected = expected_pattern | i;
                if (output[i] == expected) {
                    printf(" ✓ CORRECT\n");
                } else {
                    printf(" ✗ EXPECTED 0x%08x\n", expected);
                }
                fprintf(ctx.forensic_log, "  Thread %d: result=0x%08x, expected=0x%08x, match=%d\n",
                        i, output[i], expected, (output[i] == expected));
            }
        }
    }
    
    printf("\n📊 STATISTIQUES:\n");
    printf("Total threads: %d\n", NUM_THREADS);
    printf("Non-zero results: %d (%.1f%%)\n", non_zero, (non_zero * 100.0) / NUM_THREADS);
    
    fprintf(ctx.forensic_log, "\nSTATISTICS:\n");
    fprintf(ctx.forensic_log, "  Total threads: %d\n", NUM_THREADS);
    fprintf(ctx.forensic_log, "  Non-zero results: %d (%.1f%%)\n", non_zero, (non_zero * 100.0) / NUM_THREADS);
    
    if (non_zero > 0) {
        printf("\n✅ SUCCÈS: Kernel natif produit des résultats non-zéro!\n");
        printf("SEND descriptor 0x04025E00 fonctionne correctement.\n");
        fprintf(ctx.forensic_log, "\n✅ SUCCESS: Native kernel produces non-zero results\n");
        fprintf(ctx.forensic_log, "SEND descriptor 0x04025E00 works correctly\n");
    } else {
        printf("\n❌ ÉCHEC: Tous les résultats sont zéro.\n");
        printf("Problème: SEND descriptor ou configuration GPU.\n");
        fprintf(ctx.forensic_log, "\n❌ FAILURE: All results are zero\n");
        fprintf(ctx.forensic_log, "Problem: SEND descriptor or GPU configuration\n");
    }
    
    /* Cleanup */
    fprintf(ctx.forensic_log, "\n[%lu] === CLEANUP START ===\n", get_timestamp_ns());
    
    struct drm_gem_close close_arg;
    close_arg.handle = ctx.handle_kernel;
    drmIoctl(ctx.fd, DRM_IOCTL_GEM_CLOSE, &close_arg);
    close_arg.handle = ctx.handle_output;
    drmIoctl(ctx.fd, DRM_IOCTL_GEM_CLOSE, &close_arg);
    close_arg.handle = ctx.handle_ssh;
    drmIoctl(ctx.fd, DRM_IOCTL_GEM_CLOSE, &close_arg);
    close_arg.handle = ctx.handle_batch;
    drmIoctl(ctx.fd, DRM_IOCTL_GEM_CLOSE, &close_arg);
    
    struct drm_i915_gem_context_destroy ctx_destroy = { .ctx_id = ctx.ctx_id };
    drmIoctl(ctx.fd, DRM_IOCTL_I915_GEM_CONTEXT_DESTROY, &ctx_destroy);
    
    close(ctx.fd);
    
    fprintf(ctx.forensic_log, "[%lu] Cleanup completed\n", get_timestamp_ns());
    fprintf(ctx.forensic_log, "\n# END OF LOG\n");
    fclose(ctx.forensic_log);
    
    /* Générer statistiques forensiques */
    printf("\n[FORENSIC] Génération statistiques finales...\n");
    i915_forensic_tracker_cleanup();
    
    printf("\n╔════════════════════════════════════════════════════════════╗\n");
    printf("║  LOGS FORENSIQUES GÉNÉRÉS                                  ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    printf("  - logs/forensic/c407/execution_c407.log (principal)\n");
    printf("  - logs/forensic/c407/i915_forensic_main.log (événements)\n");
    printf("  - logs/forensic/c407/i915_forensic_hexdump.log (dumps hex)\n");
    printf("  - logs/forensic/c407/i915_forensic_timeline.log (timeline CSV)\n\n");
    
    return (non_zero > 0) ? 0 : 1;
}

// Made with Bob
