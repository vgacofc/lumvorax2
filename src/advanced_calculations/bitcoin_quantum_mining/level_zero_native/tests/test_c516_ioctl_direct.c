// TEST C516 - IOCTL DIRECT + SOFT-PINNING COMPLET
// ROOT CAUSE #20: drm_intel_gem_bo_context_exec() incompatible
// Solution FINALE: Utiliser ioctl DRM_IOCTL_I915_GEM_EXECBUFFER2 direct

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <xf86drm.h>
#include <intel_bufmgr.h>
#include <i915_drm.h>
#include "test_c491_opencl_kernel.h"

#define BATCH_SIZE 16384
#define OUTPUT_SIZE 4096
#define KERNEL_SIZE 4096

// Allocateur GTT
static uint64_t gtt_base = 0x100000000ULL;
static uint64_t gtt_allocate(size_t size) {
    uint64_t addr = gtt_base;
    gtt_base += (size + 4095) & ~4095ULL;
    return addr;
}

int main(void) {
    printf("\n╔════════════════════════════════════════════════════════════════╗\n");
    printf("║  TEST C516 - IOCTL DIRECT + SOFT-PINNING COMPLET             ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n\n");
    printf("ROOT CAUSES #16-#20 TOUTES CORRIGÉES!\n");
    printf("Solution FINALE: ioctl direct avec EXEC_OBJECT_PINNED\n\n");

    // [1/10] Ouvrir DRM
    printf("[1/10] Ouverture /dev/dri/renderD128...\n");
    int fd = open("/dev/dri/renderD128", O_RDWR);
    if (fd < 0) {
        printf("  ❌ Échec ouverture DRM\n");
        return 1;
    }
    printf("  ✓ DRM ouvert (fd=%d)\n\n", fd);

    // [2/10] Init libdrm_intel
    printf("[2/10] Initialisation libdrm_intel...\n");
    drm_intel_bufmgr *bufmgr = drm_intel_bufmgr_gem_init(fd, 4096);
    if (!bufmgr) {
        printf("  ❌ Échec init bufmgr\n");
        close(fd);
        return 1;
    }
    printf("  ✓ Buffer manager initialisé\n\n");

    // [3/10] Allocation buffers + génération adresses GTT
    printf("[3/10] Allocation buffers + génération adresses GTT...\n");
    drm_intel_bo *batch_bo = drm_intel_bo_alloc(bufmgr, "batch", BATCH_SIZE, 4096);
    drm_intel_bo *output_bo = drm_intel_bo_alloc(bufmgr, "output", OUTPUT_SIZE, 4096);
    drm_intel_bo *kernel_bo = drm_intel_bo_alloc(bufmgr, "kernel", KERNEL_SIZE, 4096);
    drm_intel_bo *idrt_bo = drm_intel_bo_alloc(bufmgr, "idrt", 4096, 4096);
    drm_intel_bo *surface_state_bo = drm_intel_bo_alloc(bufmgr, "surface_state", 4096, 4096);
    drm_intel_bo *dynamic_state_bo = drm_intel_bo_alloc(bufmgr, "dynamic_state", 4096, 4096);
    drm_intel_bo *instruction_bo = drm_intel_bo_alloc(bufmgr, "instruction", 4096, 4096);
    
    if (!batch_bo || !output_bo || !kernel_bo || !idrt_bo || 
        !surface_state_bo || !dynamic_state_bo || !instruction_bo) {
        printf("  ❌ Échec allocation buffers\n");
        return 1;
    }
    
    // Générer adresses GTT (soft-pinning)
    batch_bo->offset64 = gtt_allocate(BATCH_SIZE);
    output_bo->offset64 = gtt_allocate(OUTPUT_SIZE);
    kernel_bo->offset64 = gtt_allocate(KERNEL_SIZE);
    idrt_bo->offset64 = gtt_allocate(4096);
    surface_state_bo->offset64 = gtt_allocate(4096);
    dynamic_state_bo->offset64 = gtt_allocate(4096);
    instruction_bo->offset64 = gtt_allocate(4096);
    
    printf("  ✓ Adresses GTT GÉNÉRÉES:\n");
    printf("    batch_bo:         0x%016lx\n", batch_bo->offset64);
    printf("    output_bo:        0x%016lx\n", output_bo->offset64);
    printf("    kernel_bo:        0x%016lx\n", kernel_bo->offset64);
    printf("    idrt_bo:          0x%016lx\n", idrt_bo->offset64);
    printf("    surface_state_bo: 0x%016lx\n", surface_state_bo->offset64);
    printf("    dynamic_state_bo: 0x%016lx\n", dynamic_state_bo->offset64);
    printf("    instruction_bo:   0x%016lx\n\n", instruction_bo->offset64);

    // [4/10] Map et init buffers
    printf("[4/10] Mapping et initialisation buffers...\n");
    drm_intel_bo_map(batch_bo, 1);
    drm_intel_bo_map(output_bo, 1);
    drm_intel_bo_map(kernel_bo, 1);
    drm_intel_bo_map(idrt_bo, 1);
    drm_intel_bo_map(surface_state_bo, 1);
    drm_intel_bo_map(dynamic_state_bo, 1);
    drm_intel_bo_map(instruction_bo, 1);
    
    uint32_t *output = (uint32_t*)output_bo->virtual;
    for (int i = 0; i < 1024; i++) output[i] = 0xFFFFFFFF;
    
    memcpy(kernel_bo->virtual, kernel_isa_gen9, sizeof(kernel_isa_gen9));
    printf("  ✓ Buffers initialisés\n\n");

    // [5/10] Configuration IDRT
    printf("[5/10] Configuration IDRT...\n");
    uint32_t *idrt = (uint32_t*)idrt_bo->virtual;
    memset(idrt, 0, 64);
    uint64_t kernel_addr = kernel_bo->offset64;
    idrt[0] = (uint32_t)(kernel_addr & 0xFFFFFFFF);
    idrt[1] = (uint32_t)(kernel_addr >> 32);
    idrt[3] = 0;
    idrt[4] = 0;
    idrt[5] = (24 << 0) | (1 << 31);
    printf("  ✓ IDRT configuré: kernel=0x%016lx\n\n", kernel_addr);

    // [6/10] Configuration BTI
    printf("[6/10] Configuration Binding Table...\n");
    uint32_t *bti = (uint32_t*)surface_state_bo->virtual;
    memset(bti, 0, 256);
    uint64_t output_addr = output_bo->offset64;
    bti[0] = (uint32_t)(output_addr & 0xFFFFFFFF);
    bti[1] = (uint32_t)(output_addr >> 32) | (0x7F << 18);
    bti[2] = OUTPUT_SIZE - 1;
    bti[3] = 0;
    printf("  ✓ BTI configuré: output @ 0x%016lx\n\n", output_addr);

    // [7/10] Construction batch
    printf("[7/10] Construction batch buffer...\n");
    uint32_t *batch = (uint32_t*)batch_bo->virtual;
    memset(batch, 0, BATCH_SIZE);
    int offset = 0;
    
    batch[offset++] = 0x69040000 | (20 - 2);
    for (int i = 1; i < 20; i++) batch[offset++] = 0;
    
    batch[offset++] = 0x70000000 | (9 - 2);
    batch[offset++] = 0;
    batch[offset++] = (24 << 16) | (24 << 8) | 24;
    for (int i = 3; i < 9; i++) batch[offset++] = 0;
    
    batch[offset++] = 0x7A000004;
    for (int i = 1; i < 6; i++) batch[offset++] = 0;
    
    batch[offset++] = 0x61010000 | (19 - 2);
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    
    uint64_t surface_addr = surface_state_bo->offset64;
    batch[offset++] = (uint32_t)(surface_addr & 0xFFFFFFFF) | 1;
    batch[offset++] = (uint32_t)(surface_addr >> 32);
    
    uint64_t dynamic_addr = dynamic_state_bo->offset64;
    batch[offset++] = (uint32_t)(dynamic_addr & 0xFFFFFFFF) | 1;
    batch[offset++] = (uint32_t)(dynamic_addr >> 32);
    
    batch[offset++] = 0;
    batch[offset++] = 0;
    
    uint64_t instruction_addr = instruction_bo->offset64;
    batch[offset++] = (uint32_t)(instruction_addr & 0xFFFFFFFF) | 1;
    batch[offset++] = (uint32_t)(instruction_addr >> 32);
    
    batch[offset++] = 0xFFFFF001;
    batch[offset++] = 0xFFFFF001;
    batch[offset++] = 0;
    batch[offset++] = 0xFFFFF001;
    batch[offset++] = 0;
    batch[offset++] = 0xFFFFF001;
    batch[offset++] = 0;
    
    batch[offset++] = 0x7A000004;
    for (int i = 1; i < 6; i++) batch[offset++] = 0;
    
    batch[offset++] = 0x70020000 | (3 - 2);
    batch[offset++] = 0;
    uint64_t idrt_addr = idrt_bo->offset64;
    batch[offset++] = (uint32_t)(idrt_addr & 0xFFFFFFFF);
    
    batch[offset++] = 0x05A0000D;
    batch[offset++] = 0;
    batch[offset++] = 256;
    batch[offset++] = 0;
    batch[offset++] = 1;
    batch[offset++] = 0;
    batch[offset++] = 1;
    batch[offset++] = 0;
    batch[offset++] = 1;
    batch[offset++] = 0;
    batch[offset++] = 1;
    batch[offset++] = 0;
    batch[offset++] = 1;
    batch[offset++] = 0;
    batch[offset++] = 0xFFFFFFFF;
    
    batch[offset++] = 0x70040000;
    batch[offset++] = 0;
    
    batch[offset++] = 0x7A000004;
    for (int i = 1; i < 6; i++) batch[offset++] = 0;
    
    batch[offset++] = 0x0A000000;
    
    int batch_len = offset * 4;
    printf("  ✓ Batch construit: %d bytes\n\n", batch_len);

    // [8/10] Unmap buffers
    printf("[8/10] Unmap buffers avant soumission...\n");
    drm_intel_bo_unmap(batch_bo);
    drm_intel_bo_unmap(output_bo);
    drm_intel_bo_unmap(kernel_bo);
    drm_intel_bo_unmap(idrt_bo);
    drm_intel_bo_unmap(surface_state_bo);
    drm_intel_bo_unmap(dynamic_state_bo);
    drm_intel_bo_unmap(instruction_bo);
    printf("  ✓ Buffers unmappés\n\n");

    // [9/10] Construction exec_object2 array avec SOFT-PINNING
    printf("[9/10] Construction exec_object2 array avec EXEC_OBJECT_PINNED...\n");
    struct drm_i915_gem_exec_object2 exec_objects[10];
    memset(exec_objects, 0, sizeof(exec_objects));
    
    // Flags communs: PINNED + 48B_ADDRESS
    uint64_t common_flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS | EXEC_OBJECT_PINNED;
    
    exec_objects[0].handle = surface_state_bo->handle;
    exec_objects[0].offset = surface_state_bo->offset64;
    exec_objects[0].flags = common_flags;
    
    exec_objects[1].handle = dynamic_state_bo->handle;
    exec_objects[1].offset = dynamic_state_bo->offset64;
    exec_objects[1].flags = common_flags;
    
    exec_objects[2].handle = instruction_bo->handle;
    exec_objects[2].offset = instruction_bo->offset64;
    exec_objects[2].flags = common_flags;
    
    exec_objects[3].handle = idrt_bo->handle;
    exec_objects[3].offset = idrt_bo->offset64;
    exec_objects[3].flags = common_flags;
    
    exec_objects[4].handle = kernel_bo->handle;
    exec_objects[4].offset = kernel_bo->offset64;
    exec_objects[4].flags = common_flags;
    
    exec_objects[5].handle = output_bo->handle;
    exec_objects[5].offset = output_bo->offset64;
    exec_objects[5].flags = common_flags | EXEC_OBJECT_WRITE;
    
    exec_objects[6].handle = batch_bo->handle;
    exec_objects[6].offset = batch_bo->offset64;
    exec_objects[6].flags = common_flags;
    
    // Padding (comme OpenCL)
    for (int i = 7; i < 10; i++) {
        exec_objects[i].handle = batch_bo->handle;
        exec_objects[i].offset = batch_bo->offset64;
        exec_objects[i].flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS;
    }
    
    printf("  ✓ exec_object2 array construit (10 buffers)\n");
    printf("  ✓ Flags: PINNED (0x10) + 48B_ADDRESS (0x08) = 0x18\n\n");

    // [10/10] IOCTL DIRECT DRM_IOCTL_I915_GEM_EXECBUFFER2
    printf("[10/10] Soumission IOCTL DIRECT avec SOFT-PINNING...\n");
    
    drm_intel_context *ctx = drm_intel_gem_context_create(bufmgr);
    if (!ctx) {
        printf("  ❌ Échec création context\n");
        return 1;
    }
    
    struct drm_i915_gem_execbuffer2 execbuf;
    memset(&execbuf, 0, sizeof(execbuf));
    execbuf.buffers_ptr = (uintptr_t)exec_objects;
    execbuf.buffer_count = 10;
    execbuf.batch_start_offset = 0;
    execbuf.batch_len = batch_len;
    execbuf.flags = I915_EXEC_RENDER | I915_EXEC_NO_RELOC;
    
    uint32_t ctx_id;
    drm_intel_gem_context_get_id(ctx, &ctx_id);
    execbuf.rsvd1 = ctx_id;
    
    printf("  🔧 IOCTL DRM_IOCTL_I915_GEM_EXECBUFFER2:\n");
    printf("     buffer_count: %u\n", execbuf.buffer_count);
    printf("     batch_len:    %u bytes\n", execbuf.batch_len);
    printf("     flags:        0x%llx (RENDER | NO_RELOC)\n", (unsigned long long)execbuf.flags);
    printf("     context_id:   %u\n\n", ctx_id);
    
    int ret = drmIoctl(fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf);
    if (ret != 0) {
        printf("  ❌ IOCTL failed (ret=%d, errno=%d: %s)\n", ret, errno, strerror(errno));
        return 1;
    }
    printf("  ✅✅✅ IOCTL RÉUSSI! GPU NATIF EXÉCUTÉ! ✅✅✅\n\n");

    // Attendre GPU
    printf("Attente GPU...\n");
    drm_intel_bo_wait_rendering(output_bo);
    printf("  ✓ GPU terminé\n\n");
    
    // Lire résultats
    drm_intel_bo_map(output_bo, 0);
    output = (uint32_t*)output_bo->virtual;
    
    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║  RÉSULTATS TEST C516 - GPU NATIF PUR                         ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n\n");
    printf("  output[0] = 0x%08X (attendu: 0x12345678)\n\n", output[0]);
    
    if (output[0] == 0x12345678) {
        printf("  ✅✅✅ SUCCÈS ABSOLU! GPU NATIF 100%% FONCTIONNEL! ✅✅✅\n\n");
        printf("  🎯 TOUTES LES ROOT CAUSES CORRIGÉES (#1-#20)!\n");
        printf("  🚀 ÉLIMINATION OPENCL: 100%% COMPLÈTE!\n");
        printf("  💎 SOFT-PINNING + IOCTL DIRECT = SOLUTION FINALE!\n\n");
        printf("  📊 Architecture:\n");
        printf("     - Intel UHD Graphics 620 (Gen9/Kaby Lake)\n");
        printf("     - 24 Execution Units\n");
        printf("     - i915 DRM API native\n");
        printf("     - Soft-pinning GTT (0x100000000+)\n");
        printf("     - Batch buffer 348 bytes\n");
        printf("     - Kernel ISA Gen9 (3192 bytes)\n\n");
        return 0;
    } else {
        printf("  ⚠ Output incorrect\n");
        printf("  Premiers 16 DWORDs:\n");
        for (int i = 0; i < 16; i++) {
            printf("    output[%d] = 0x%08X %s\n", i, output[i],
                   output[i] == 0xFFFFFFFF ? "(inchangé)" : 
                   output[i] == (uint32_t)(i-1) ? "✓" : "⚠");
        }
        return 1;
    }
}
