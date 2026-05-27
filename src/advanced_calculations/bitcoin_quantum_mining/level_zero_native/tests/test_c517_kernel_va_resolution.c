// TEST C517 - RÉSOLUTION VA GPU PAR i915 (SANS SOFT-PINNING ARBITRAIRE)
// ROOT CAUSE #21: Adresses GTT arbitraires invalides
// Solution: Laisser i915 choisir les VA, puis reconstruire batch avec vraies adresses

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

int main(void) {
    printf("\n╔════════════════════════════════════════════════════════════════╗\n");
    printf("║  TEST C517 - RÉSOLUTION VA GPU PAR i915 (APPROCHE CORRECTE)  ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n\n");
    printf("ROOT CAUSE #21: Adresses GTT arbitraires INVALIDES!\n");
    printf("Solution: Laisser i915 choisir les VA GPU, puis reconstruire batch\n\n");

    // [1/12] Ouvrir DRM
    printf("[1/12] Ouverture /dev/dri/renderD128...\n");
    int fd = open("/dev/dri/renderD128", O_RDWR);
    if (fd < 0) {
        printf("  ❌ Échec ouverture DRM\n");
        return 1;
    }
    printf("  ✓ DRM ouvert (fd=%d)\n\n", fd);

    // [2/12] Init libdrm_intel
    printf("[2/12] Initialisation libdrm_intel...\n");
    drm_intel_bufmgr *bufmgr = drm_intel_bufmgr_gem_init(fd, 4096);
    if (!bufmgr) {
        printf("  ❌ Échec init bufmgr\n");
        close(fd);
        return 1;
    }
    printf("  ✓ Buffer manager initialisé\n\n");

    // [3/12] Allocation buffers (SANS pré-remplir offset64)
    printf("[3/12] Allocation buffers (VA GPU non spécifiées)...\n");
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
    printf("  ✓ 7 buffers alloués (VA GPU = 0, i915 choisira)\n\n");

    // [4/12] Map et init buffers
    printf("[4/12] Mapping et initialisation buffers...\n");
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

    // [5/12] Construction batch TEMPORAIRE (adresses nulles)
    printf("[5/12] Construction batch TEMPORAIRE (adresses = 0)...\n");
    uint32_t *batch = (uint32_t*)batch_bo->virtual;
    memset(batch, 0, BATCH_SIZE);
    int offset = 0;
    
    // PIPELINE_SELECT
    batch[offset++] = 0x69040000 | (20 - 2);
    for (int i = 1; i < 20; i++) batch[offset++] = 0;
    
    // MEDIA_VFE_STATE
    batch[offset++] = 0x70000000 | (9 - 2);
    batch[offset++] = 0;
    batch[offset++] = (24 << 16) | (24 << 8) | 24;
    for (int i = 3; i < 9; i++) batch[offset++] = 0;
    
    // PIPE_CONTROL
    batch[offset++] = 0x7A000004;
    for (int i = 1; i < 6; i++) batch[offset++] = 0;
    
    // STATE_BASE_ADDRESS (adresses temporaires = 0)
    batch[offset++] = 0x61010000 | (19 - 2);
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0 | 1;  // surface_state (sera mis à jour)
    batch[offset++] = 0;
    batch[offset++] = 0 | 1;  // dynamic_state (sera mis à jour)
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0 | 1;  // instruction (sera mis à jour)
    batch[offset++] = 0;
    batch[offset++] = 0xFFFFF001;
    batch[offset++] = 0xFFFFF001;
    batch[offset++] = 0;
    batch[offset++] = 0xFFFFF001;
    batch[offset++] = 0;
    batch[offset++] = 0xFFFFF001;
    batch[offset++] = 0;
    
    // PIPE_CONTROL
    batch[offset++] = 0x7A000004;
    for (int i = 1; i < 6; i++) batch[offset++] = 0;
    
    // MEDIA_INTERFACE_DESCRIPTOR_LOAD
    batch[offset++] = 0x70020000 | (3 - 2);
    batch[offset++] = 0;
    batch[offset++] = 0;  // idrt_addr (sera mis à jour)
    
    // GPGPU_WALKER
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
    
    // MEDIA_STATE_FLUSH
    batch[offset++] = 0x70040000;
    batch[offset++] = 0;
    
    // PIPE_CONTROL
    batch[offset++] = 0x7A000004;
    for (int i = 1; i < 6; i++) batch[offset++] = 0;
    
    // MI_BATCH_BUFFER_END
    batch[offset++] = 0x0A000000;
    
    int batch_len = offset * 4;
    printf("  ✓ Batch temporaire: %d bytes (adresses = 0)\n\n", batch_len);

    // [6/12] Unmap buffers
    printf("[6/12] Unmap buffers...\n");
    drm_intel_bo_unmap(batch_bo);
    drm_intel_bo_unmap(output_bo);
    drm_intel_bo_unmap(kernel_bo);
    drm_intel_bo_unmap(idrt_bo);
    drm_intel_bo_unmap(surface_state_bo);
    drm_intel_bo_unmap(dynamic_state_bo);
    drm_intel_bo_unmap(instruction_bo);
    printf("  ✓ Buffers unmappés\n\n");

    // [7/12] PASS 1: Soumission pour obtenir VA GPU réelles
    printf("[7/12] PASS 1: Soumission pour résolution VA GPU par i915...\n");
    
    drm_intel_context *ctx = drm_intel_gem_context_create(bufmgr);
    if (!ctx) {
        printf("  ❌ Échec création context\n");
        return 1;
    }
    
    struct drm_i915_gem_exec_object2 exec_objects[10];
    memset(exec_objects, 0, sizeof(exec_objects));
    
    // SANS EXEC_OBJECT_PINNED - laisser i915 choisir
    uint64_t base_flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS;
    
    exec_objects[0].handle = surface_state_bo->handle;
    exec_objects[0].flags = base_flags;
    
    exec_objects[1].handle = dynamic_state_bo->handle;
    exec_objects[1].flags = base_flags;
    
    exec_objects[2].handle = instruction_bo->handle;
    exec_objects[2].flags = base_flags;
    
    exec_objects[3].handle = idrt_bo->handle;
    exec_objects[3].flags = base_flags;
    
    exec_objects[4].handle = kernel_bo->handle;
    exec_objects[4].flags = base_flags;
    
    exec_objects[5].handle = output_bo->handle;
    exec_objects[5].flags = base_flags | EXEC_OBJECT_WRITE;
    
    exec_objects[6].handle = batch_bo->handle;
    exec_objects[6].flags = base_flags;
    
    // Padding
    for (int i = 7; i < 10; i++) {
        exec_objects[i].handle = batch_bo->handle;
        exec_objects[i].flags = base_flags;
    }
    
    struct drm_i915_gem_execbuffer2 execbuf;
    memset(&execbuf, 0, sizeof(execbuf));
    execbuf.buffers_ptr = (uintptr_t)exec_objects;
    execbuf.buffer_count = 10;
    execbuf.batch_start_offset = 0;
    execbuf.batch_len = batch_len;
    execbuf.flags = I915_EXEC_RENDER;  // SANS NO_RELOC pour PASS 1
    
    uint32_t ctx_id;
    drm_intel_gem_context_get_id(ctx, &ctx_id);
    execbuf.rsvd1 = ctx_id;
    
    printf("  🔧 PASS 1 - Flags: 0x%llx (RENDER uniquement)\n", (unsigned long long)execbuf.flags);
    
    int ret = drmIoctl(fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf);
    if (ret != 0) {
        printf("  ❌ PASS 1 failed (ret=%d, errno=%d: %s)\n", ret, errno, strerror(errno));
        return 1;
    }
    printf("  ✅ PASS 1 RÉUSSI - i915 a résolu les VA GPU!\n\n");

    // [8/12] Lecture VA GPU réelles retournées par i915
    printf("[8/12] Lecture VA GPU réelles assignées par i915...\n");
    uint64_t surface_addr = exec_objects[0].offset;
    uint64_t dynamic_addr = exec_objects[1].offset;
    uint64_t instruction_addr = exec_objects[2].offset;
    uint64_t idrt_addr = exec_objects[3].offset;
    uint64_t kernel_addr = exec_objects[4].offset;
    uint64_t output_addr = exec_objects[5].offset;
    uint64_t batch_addr = exec_objects[6].offset;
    
    printf("  ✓ VA GPU RÉELLES:\n");
    printf("    surface_state: 0x%016lx\n", surface_addr);
    printf("    dynamic_state: 0x%016lx\n", dynamic_addr);
    printf("    instruction:   0x%016lx\n", instruction_addr);
    printf("    idrt:          0x%016lx\n", idrt_addr);
    printf("    kernel:        0x%016lx\n", kernel_addr);
    printf("    output:        0x%016lx\n", output_addr);
    printf("    batch:         0x%016lx\n\n", batch_addr);

    // [9/12] Reconstruction IDRT avec vraie VA kernel
    printf("[9/12] Reconstruction IDRT avec VA kernel réelle...\n");
    drm_intel_bo_map(idrt_bo, 1);
    uint32_t *idrt = (uint32_t*)idrt_bo->virtual;
    memset(idrt, 0, 64);
    idrt[0] = (uint32_t)(kernel_addr & 0xFFFFFFFF);
    idrt[1] = (uint32_t)(kernel_addr >> 32);
    idrt[3] = 0;
    idrt[4] = 0;
    idrt[5] = (24 << 0) | (1 << 31);
    drm_intel_bo_unmap(idrt_bo);
    printf("  ✓ IDRT mis à jour: kernel @ 0x%016lx\n\n", kernel_addr);

    // [10/12] Reconstruction BTI avec vraie VA output
    printf("[10/12] Reconstruction BTI avec VA output réelle...\n");
    drm_intel_bo_map(surface_state_bo, 1);
    uint32_t *bti = (uint32_t*)surface_state_bo->virtual;
    memset(bti, 0, 256);
    bti[0] = (uint32_t)(output_addr & 0xFFFFFFFF);
    bti[1] = (uint32_t)(output_addr >> 32) | (0x7F << 18);
    bti[2] = OUTPUT_SIZE - 1;
    bti[3] = 0;
    drm_intel_bo_unmap(surface_state_bo);
    printf("  ✓ BTI mis à jour: output @ 0x%016lx\n\n", output_addr);

    // [11/12] Reconstruction batch avec VA réelles
    printf("[11/12] Reconstruction batch avec VA GPU réelles...\n");
    drm_intel_bo_map(batch_bo, 1);
    batch = (uint32_t*)batch_bo->virtual;
    
    // Mettre à jour STATE_BASE_ADDRESS (offset 35-50)
    batch[35] = (uint32_t)(surface_addr & 0xFFFFFFFF) | 1;
    batch[36] = (uint32_t)(surface_addr >> 32);
    batch[37] = (uint32_t)(dynamic_addr & 0xFFFFFFFF) | 1;
    batch[38] = (uint32_t)(dynamic_addr >> 32);
    batch[41] = (uint32_t)(instruction_addr & 0xFFFFFFFF) | 1;
    batch[42] = (uint32_t)(instruction_addr >> 32);
    
    // Mettre à jour MEDIA_INTERFACE_DESCRIPTOR_LOAD (offset 59)
    batch[59] = (uint32_t)(idrt_addr & 0xFFFFFFFF);
    
    drm_intel_bo_unmap(batch_bo);
    printf("  ✓ Batch reconstruit avec VA réelles\n\n");

    // [12/12] PASS 2: Soumission finale avec NO_RELOC + PINNED
    printf("[12/12] PASS 2: Soumission finale avec NO_RELOC + PINNED...\n");
    
    // Mettre à jour exec_objects avec PINNED + offsets réels
    for (int i = 0; i < 7; i++) {
        exec_objects[i].flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS | EXEC_OBJECT_PINNED;
        // offset déjà rempli par PASS 1
    }
    exec_objects[5].flags |= EXEC_OBJECT_WRITE;
    
    execbuf.flags = I915_EXEC_RENDER | I915_EXEC_NO_RELOC;
    
    printf("  🔧 PASS 2 - Flags: 0x%llx (RENDER | NO_RELOC)\n", (unsigned long long)execbuf.flags);
    printf("  🔧 Buffer flags: 0x%llx (PINNED | 48B_ADDRESS)\n\n", (unsigned long long)exec_objects[0].flags);
    
    ret = drmIoctl(fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf);
    if (ret != 0) {
        printf("  ❌ PASS 2 failed (ret=%d, errno=%d: %s)\n", ret, errno, strerror(errno));
        return 1;
    }
    printf("  ✅✅✅ PASS 2 RÉUSSI! GPU NATIF EXÉCUTÉ AVEC VA RÉELLES! ✅✅✅\n\n");

    // Attendre GPU
    printf("Attente GPU (100ms)...\n");
    usleep(100000);
    drm_intel_bo_wait_rendering(output_bo);
    printf("✓ GPU terminé\n\n");

    // Vérification résultat
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("VÉRIFICATION RÉSULTAT GPU NATIF\n");
    printf("═══════════════════════════════════════════════════════════════\n");
    
    drm_intel_bo_map(output_bo, 0);
    output = (uint32_t*)output_bo->virtual;
    
    printf("output[0] = 0x%08X (attendu: 0x12345678)\n", output[0]);
    printf("output[1] = 0x%08X\n", output[1]);
    printf("output[2] = 0x%08X\n", output[2]);
    printf("output[3] = 0x%08X\n\n", output[3]);
    
    if (output[0] == 0x12345678) {
        printf("✅✅✅ SUCCÈS TOTAL! GPU NATIF FONCTIONNE! ✅✅✅\n");
        printf("✅ ÉLIMINATION OPENCL: 100%% COMPLÈTE!\n");
    } else {
        printf("⚠️  Valeur incorrecte - debug nécessaire\n");
    }
    
    drm_intel_bo_unmap(output_bo);
    
    // Cleanup
    drm_intel_bo_unreference(batch_bo);
    drm_intel_bo_unreference(output_bo);
    drm_intel_bo_unreference(kernel_bo);
    drm_intel_bo_unreference(idrt_bo);
    drm_intel_bo_unreference(surface_state_bo);
    drm_intel_bo_unreference(dynamic_state_bo);
    drm_intel_bo_unreference(instruction_bo);
    drm_intel_gem_context_destroy(ctx);
    drm_intel_bufmgr_destroy(bufmgr);
    close(fd);
    
    return 0;
}

// Made with Bob
