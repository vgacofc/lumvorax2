/**
 * C406: TEST KERNEL NATIF GEN9 ISA
 * 
 * OBJECTIF: Tester kernel natif pur (btc_sha256_native_c406.bin)
 * - SEND descriptor: 0x04025E00 (stateful, BTI=0)
 * - Format: Pure Gen9 ISA (pas de dump OpenCL)
 * - Runtime: 100% i915 DRM avec GEM_WAIT
 * 
 * ATTENDU: Résultats non-zéro (validation SEND descriptor)
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
#include <drm/i915_drm.h>
#include <xf86drm.h>

#define NUM_THREADS 256

typedef struct {
    int fd;
    uint32_t handle_kernel, handle_output, handle_ssh, handle_batch;
    void *map_kernel, *map_output, *map_ssh, *map_batch;
    uint64_t gtt_kernel, gtt_output, gtt_ssh, gtt_batch;
} gpu_context_t;

// Créer buffer GEM
static int create_bo(int fd, size_t size, uint32_t *handle_out, void **map_out) {
    struct drm_i915_gem_create create = { .size = size };
    
    if (drmIoctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create) < 0) {
        fprintf(stderr, "GEM_CREATE failed: %s\n", strerror(errno));
        return -1;
    }
    
    *handle_out = create.handle;
    
    struct drm_i915_gem_mmap mmap_arg = {
        .handle = create.handle,
        .offset = 0,
        .size = size,
        .flags = 0
    };
    
    if (drmIoctl(fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_arg) < 0) {
        fprintf(stderr, "GEM_MMAP failed: %s\n", strerror(errno));
        struct drm_gem_close close_arg = { .handle = create.handle };
        drmIoctl(fd, DRM_IOCTL_GEM_CLOSE, &close_arg);
        return -1;
    }
    
    *map_out = (void *)(uintptr_t)mmap_arg.addr_ptr;
    memset(*map_out, 0, size);
    
    return 0;
}

int main() {
    printf("=== C406 TEST KERNEL NATIF GEN9 ISA ===\n");
    printf("Kernel: btc_sha256_native_c406.bin\n");
    printf("SEND descriptor: 0x04025E00 (stateful, BTI=0)\n");
    printf("Runtime: 100%% i915 DRM + GEM_WAIT\n\n");
    
    // Ouvrir DRM
    int fd = open("/dev/dri/card1", O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "Failed to open DRM: %s\n", strerror(errno));
        return 1;
    }
    printf("✓ DRM opened: fd=%d\n", fd);
    
    // Créer contexte GPU
    struct drm_i915_gem_context_create ctx_create = {0};
    if (drmIoctl(fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &ctx_create) < 0) {
        fprintf(stderr, "Failed to create GPU context: %s\n", strerror(errno));
        close(fd);
        return 1;
    }
    uint32_t ctx_id = ctx_create.ctx_id;
    printf("✓ GPU context created: ctx_id=%u\n", ctx_id);
    
    // Charger kernel NATIF C406
    FILE *f = fopen("kernels/btc_sha256_native_c406.bin", "rb");
    if (!f) {
        fprintf(stderr, "Failed to open kernel binary\n");
        close(fd);
        return 1;
    }
    
    fseek(f, 0, SEEK_END);
    size_t kernel_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    printf("✓ Kernel size: %zu bytes\n", kernel_size);
    
    gpu_context_t ctx = {0};
    ctx.fd = fd;
    
    // Créer buffers
    if (create_bo(fd, kernel_size, &ctx.handle_kernel, &ctx.map_kernel) < 0) {
        close(fd);
        return 1;
    }
    
    // Charger kernel
    fread(ctx.map_kernel, 1, kernel_size, f);
    fclose(f);
    printf("✓ Kernel loaded\n");
    
    // Output buffer (256 threads × 4 bytes)
    if (create_bo(fd, NUM_THREADS * 4, &ctx.handle_output, &ctx.map_output) < 0) {
        close(fd);
        return 1;
    }
    printf("✓ Output buffer created: %d bytes\n", NUM_THREADS * 4);
    
    // Surface State Heap (1 surface × 64 bytes)
    if (create_bo(fd, 4096, &ctx.handle_ssh, &ctx.map_ssh) < 0) {
        close(fd);
        return 1;
    }
    
    // Batch buffer
    if (create_bo(fd, 4096, &ctx.handle_batch, &ctx.map_batch) < 0) {
        close(fd);
        return 1;
    }
    
    // Pin buffers
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
    
    printf("✓ GTT addresses:\n");
    printf("  Kernel: 0x%lx\n", ctx.gtt_kernel);
    printf("  Output: 0x%lx\n", ctx.gtt_output);
    printf("  SSH: 0x%lx\n", ctx.gtt_ssh);
    printf("  Batch: 0x%lx\n", ctx.gtt_batch);
    
    // Configurer Surface State (BTI=0)
    uint32_t *ssh = (uint32_t *)ctx.map_ssh;
    ssh[0] = 0x00000000;  // DW0: Surface Type = Buffer
    ssh[1] = ctx.gtt_output & 0xFFFFFFFF;  // DW1: Base Address Low
    ssh[2] = (ctx.gtt_output >> 32) & 0xFFFF;  // DW2: Base Address High
    ssh[3] = (NUM_THREADS * 4) - 1;  // DW3: Width (size - 1)
    ssh[4] = 0x00000000;  // DW4
    ssh[5] = 0x00000000;  // DW5
    ssh[6] = 0x00000000;  // DW6
    ssh[7] = 0x00000000;  // DW7
    
    printf("✓ Surface State configured (BTI=0)\n");
    
    // Construire batch buffer
    uint32_t *batch = (uint32_t *)ctx.map_batch;
    int idx = 0;
    
    // STATE_BASE_ADDRESS
    batch[idx++] = 0x61010009;  // STATE_BASE_ADDRESS, length=10
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
    
    // MEDIA_VFE_STATE
    batch[idx++] = 0x70000006;  // MEDIA_VFE_STATE, length=7
    batch[idx++] = 0x00000000;  // Scratch Space Base: disabled
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;  // Max threads = 0 (auto)
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    
    // MEDIA_INTERFACE_DESCRIPTOR_LOAD
    batch[idx++] = 0x70020002;  // MEDIA_INTERFACE_DESCRIPTOR_LOAD, length=3
    batch[idx++] = 0x00000000;
    batch[idx++] = 32;  // Interface Descriptor Length = 32 bytes
    batch[idx++] = ctx.gtt_ssh & 0xFFFFFFFF;  // Interface Descriptor Base
    
    // Interface Descriptor (dans SSH après surface state)
    uint32_t *id = (uint32_t *)((uint8_t *)ctx.map_ssh + 64);
    id[0] = ctx.gtt_kernel & 0xFFFFFFC0;  // DW0: Kernel Start Pointer (bits [63:6])
    id[1] = (ctx.gtt_kernel >> 32) & 0xFFFF;  // DW1: Kernel Start Pointer High
    id[2] = 0x00000000;  // DW2
    id[3] = 0x00040000;  // DW3: Barrier Enable
    id[4] = 0x00000000;  // DW4: Binding Table Pointer = 0
    id[5] = 0x00000000;  // DW5
    id[6] = 0x00000000;  // DW6
    id[7] = 0x00000000;  // DW7
    
    printf("✓ Interface Descriptor: KSP=0x%lx\n", ctx.gtt_kernel);
    
    // GPGPU_WALKER
    batch[idx++] = 0x70050009;  // GPGPU_WALKER, length=10
    batch[idx++] = 0x00000000;  // Interface Descriptor Offset = 0
    batch[idx++] = 0x00000000;  // Indirect Data Length = 0
    batch[idx++] = 0x00000000;  // Indirect Data Start Address = 0
    batch[idx++] = 0x00000001;  // Thread Width = 1
    batch[idx++] = 0x00000001;  // Thread Height = 1
    batch[idx++] = 0x00000001;  // Thread Depth = 1
    batch[idx++] = NUM_THREADS;  // Thread Group X = 256
    batch[idx++] = 0x00000001;  // Thread Group Y = 1
    batch[idx++] = 0x00000001;  // Thread Group Z = 1
    batch[idx++] = 0x00000000;  // Right Execution Mask = 0
    
    // PIPE_CONTROL (flush)
    batch[idx++] = 0x7A000004;  // PIPE_CONTROL, length=5
    batch[idx++] = 0x00100000;  // CS Stall
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    batch[idx++] = 0x00000000;
    
    // MI_BATCH_BUFFER_END
    batch[idx++] = 0x0A000000;
    
    printf("✓ Batch buffer built: %d dwords\n", idx);
    
    // Execbuffer
    struct drm_i915_gem_execbuffer2 execbuf = {
        .buffers_ptr = (uintptr_t)exec_objects,
        .buffer_count = 4,
        .batch_start_offset = 0,
        .batch_len = idx * 4,
        .flags = I915_EXEC_RENDER,
        .rsvd1 = ctx_id  // Context ID
    };
    
    printf("\n🚀 Executing GPU kernel...\n");
    if (drmIoctl(fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf) < 0) {
        fprintf(stderr, "EXECBUFFER2 failed: %s\n", strerror(errno));
        close(fd);
        return 1;
    }
    
    // GEM_WAIT (synchronisation GPU)
    printf("⏳ Waiting for GPU completion...\n");
    struct drm_i915_gem_wait wait = {
        .bo_handle = ctx.handle_output,
        .timeout_ns = 1000000000  // 1 seconde
    };
    
    if (drmIoctl(fd, DRM_IOCTL_I915_GEM_WAIT, &wait) < 0) {
        fprintf(stderr, "GEM_WAIT failed: %s\n", strerror(errno));
        close(fd);
        return 1;
    }
    
    printf("✓ GPU completed\n\n");
    
    // Lire résultats
    printf("=== RÉSULTATS ===\n");
    uint32_t *output = (uint32_t *)ctx.map_output;
    
    int non_zero = 0;
    for (int i = 0; i < NUM_THREADS; i++) {
        if (output[i] != 0) {
            non_zero++;
            if (non_zero <= 10) {
                printf("Thread %3d: 0x%08x\n", i, output[i]);
            }
        }
    }
    
    printf("\n📊 STATISTIQUES:\n");
    printf("Total threads: %d\n", NUM_THREADS);
    printf("Non-zero results: %d (%.1f%%)\n", non_zero, (non_zero * 100.0) / NUM_THREADS);
    
    if (non_zero > 0) {
        printf("\n✅ SUCCÈS: Kernel natif produit des résultats non-zéro!\n");
        printf("SEND descriptor 0x04025E00 fonctionne correctement.\n");
    } else {
        printf("\n❌ ÉCHEC: Tous les résultats sont zéro.\n");
        printf("Problème: SEND descriptor ou configuration Surface State.\n");
    }
    
    // Cleanup
    struct drm_gem_close close_arg;
    close_arg.handle = ctx.handle_kernel;
    drmIoctl(fd, DRM_IOCTL_GEM_CLOSE, &close_arg);
    close_arg.handle = ctx.handle_output;
    drmIoctl(fd, DRM_IOCTL_GEM_CLOSE, &close_arg);
    close_arg.handle = ctx.handle_ssh;
    drmIoctl(fd, DRM_IOCTL_GEM_CLOSE, &close_arg);
    close_arg.handle = ctx.handle_batch;
    drmIoctl(fd, DRM_IOCTL_GEM_CLOSE, &close_arg);
    
    // Détruire contexte
    struct drm_i915_gem_context_destroy ctx_destroy = { .ctx_id = ctx_id };
    drmIoctl(fd, DRM_IOCTL_I915_GEM_CONTEXT_DESTROY, &ctx_destroy);
    
    close(fd);
    
    return (non_zero > 0) ? 0 : 1;
}

// Made with Bob
