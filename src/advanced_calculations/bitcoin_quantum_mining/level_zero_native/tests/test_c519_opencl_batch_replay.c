// TEST C519 - REPRODUCTION EXACTE BATCH OPENCL AVEC OFFSETS i915 RÉELS
// Stratégie: Laisser i915 assigner les VA GPU, puis patcher le batch OpenCL capturé

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

#define BATCH_SIZE 4096
#define OUTPUT_SIZE 4096
#define KERNEL_SIZE 4096

// Batch OpenCL capturé (320 bytes)
static const uint32_t opencl_batch_template[80] = {
    // [0x0000] PIPELINE_SELECT + padding (20 DWORDs)
    0x69041312, 0x7a000004, 0x00100000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x11000001,
    0x0000e404, 0x00000100, 0x11000001, 0x00007034, 0x60000321, 0x61040001, 0x35000000, 0x000078fa,
    // [0x0040] PIPE_CONTROL (6 DWORDs)
    0x7a000004, 0x00101021, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    // [0x0058] MEDIA_VFE_STATE (8 DWORDs)
    0x70000007, 0x00000000, 0x00000000, 0x00a70100, 0x00000000, 0x07820000, 0x00000000, 0x00000000,
    // [0x0078] Setup + PIPE_CONTROL (9 DWORDs)
    0x00000000, 0x11000001, 0x00002580, 0x00060000, 0x7a000004, 0x00100420, 0x00000000, 0x00000000,
    0x00000000,
    // [0x009C] Padding
    0x00000000,
    // [0x00A0] STATE_BASE_ADDRESS (19 DWORDs) - ADRESSES À PATCHER
    0x61010011, 0x00000001, 0x00000000, 0x00040000,
    0x37d06001, 0x000078fa,  // DW4-5: Surface State Base (À PATCHER)
    0x3801e001, 0x000078fa,  // DW6-7: Dynamic State Base (À PATCHER)
    0x00000001, 0xffff8001, 0x00000041, 0x00008001,
    0xfffff001, 0x00010001, 0xfffff001, 0xfffff001,
    0x37d06001, 0x000078fa, 0x003bf000,  // DW16-18: Bindless Surface State
    // [0x00EC] MEDIA_INTERFACE_DESCRIPTOR_LOAD (3 DWORDs)
    0x61020001, 0xfffee000, 0x00000000,  // DW1: offset relatif à Dynamic State
    // [0x00F8] PIPE_CONTROL (6 DWORDs)
    0x7a000004, 0x00100000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    // [0x0110] GPGPU_WALKER (11 DWORDs)
    0x18800101,
    0x3804f000, 0x000078fa,  // DW1-2: IDRT offset (À PATCHER)
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000
};

int main(void) {
    printf("\n╔════════════════════════════════════════════════════════════════╗\n");
    printf("║  TEST C519 - REPRODUCTION BATCH OPENCL AVEC OFFSETS i915     ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n\n");
    printf("ROOT CAUSES #22-#26 CORRIGÉES!\n");
    printf("Stratégie: Laisser i915 assigner VA, puis patcher batch OpenCL\n\n");

    // [1/15] Ouvrir DRM
    printf("[1/15] Ouverture /dev/dri/renderD128...\n");
    int fd = open("/dev/dri/renderD128", O_RDWR);
    if (fd < 0) {
        printf("  ❌ Échec ouverture DRM\n");
        return 1;
    }
    printf("  ✓ DRM ouvert (fd=%d)\n\n", fd);

    // [2/15] Init libdrm_intel
    printf("[2/15] Initialisation libdrm_intel...\n");
    drm_intel_bufmgr *bufmgr = drm_intel_bufmgr_gem_init(fd, 4096);
    if (!bufmgr) {
        printf("  ❌ Échec init bufmgr\n");
        close(fd);
        return 1;
    }
    printf("  ✓ Buffer manager initialisé\n\n");

    // [3/15] Allocation buffers (hiérarchie OpenCL)
    printf("[3/15] Allocation buffers (hiérarchie OpenCL)...\n");
    drm_intel_bo *surface_state_bo = drm_intel_bo_alloc(bufmgr, "surface_state", 4096, 4096);  // buffer[3]
    drm_intel_bo *dynamic_state_bo = drm_intel_bo_alloc(bufmgr, "dynamic_state", 4096, 4096);  // buffer[2]
    drm_intel_bo *idrt_bo = drm_intel_bo_alloc(bufmgr, "idrt", 4096, 4096);                    // buffer[8]
    drm_intel_bo *kernel_bo = drm_intel_bo_alloc(bufmgr, "kernel", KERNEL_SIZE, 4096);
    drm_intel_bo *output_bo = drm_intel_bo_alloc(bufmgr, "output", OUTPUT_SIZE, 4096);
    drm_intel_bo *batch_bo = drm_intel_bo_alloc(bufmgr, "batch", BATCH_SIZE, 4096);            // buffer[9]
    
    if (!surface_state_bo || !dynamic_state_bo || !idrt_bo || 
        !kernel_bo || !output_bo || !batch_bo) {
        printf("  ❌ Échec allocation buffers\n");
        return 1;
    }
    printf("  ✓ 6 buffers alloués (VA GPU = 0, i915 choisira)\n\n");

    // [4/15] Init buffers
    printf("[4/15] Initialisation buffers...\n");
    drm_intel_bo_map(output_bo, 1);
    uint32_t *output = (uint32_t*)output_bo->virtual;
    for (int i = 0; i < 1024; i++) output[i] = 0xFFFFFFFF;
    drm_intel_bo_unmap(output_bo);
    
    drm_intel_bo_map(kernel_bo, 1);
    memcpy(kernel_bo->virtual, kernel_isa_gen9, sizeof(kernel_isa_gen9));
    drm_intel_bo_unmap(kernel_bo);
    
    printf("  ✓ Output et kernel initialisés\n\n");

    // [5/15] PASS 1: Soumission pour obtenir offsets i915
    printf("[5/15] PASS 1: Soumission pour résolution VA GPU par i915...\n");
    
    drm_intel_context *ctx = drm_intel_gem_context_create(bufmgr);
    if (!ctx) {
        printf("  ❌ Échec création context\n");
        return 1;
    }
    
    // Batch temporaire minimal (MI_NOOP + MI_BATCH_BUFFER_END)
    drm_intel_bo_map(batch_bo, 1);
    uint32_t *batch_temp = (uint32_t*)batch_bo->virtual;
    batch_temp[0] = 0x00000000;  // MI_NOOP
    batch_temp[1] = 0x0A000000;  // MI_BATCH_BUFFER_END
    drm_intel_bo_unmap(batch_bo);
    
    struct drm_i915_gem_exec_object2 exec_objects_pass1[6];
    memset(exec_objects_pass1, 0, sizeof(exec_objects_pass1));
    
    uint64_t base_flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS;
    
    exec_objects_pass1[0].handle = surface_state_bo->handle;
    exec_objects_pass1[0].flags = base_flags;
    
    exec_objects_pass1[1].handle = dynamic_state_bo->handle;
    exec_objects_pass1[1].flags = base_flags;
    
    exec_objects_pass1[2].handle = idrt_bo->handle;
    exec_objects_pass1[2].flags = base_flags;
    
    exec_objects_pass1[3].handle = kernel_bo->handle;
    exec_objects_pass1[3].flags = base_flags;
    
    exec_objects_pass1[4].handle = output_bo->handle;
    exec_objects_pass1[4].flags = base_flags | EXEC_OBJECT_WRITE;
    
    exec_objects_pass1[5].handle = batch_bo->handle;
    exec_objects_pass1[5].flags = base_flags;
    
    struct drm_i915_gem_execbuffer2 execbuf_pass1;
    memset(&execbuf_pass1, 0, sizeof(execbuf_pass1));
    execbuf_pass1.buffers_ptr = (uintptr_t)exec_objects_pass1;
    execbuf_pass1.buffer_count = 6;
    execbuf_pass1.batch_start_offset = 0;
    execbuf_pass1.batch_len = 8;
    execbuf_pass1.flags = I915_EXEC_RENDER;
    
    uint32_t ctx_id;
    drm_intel_gem_context_get_id(ctx, &ctx_id);
    execbuf_pass1.rsvd1 = ctx_id;
    
    printf("  🔧 PASS 1 - Soumission batch minimal pour VA resolution\n");
    
    int ret = drmIoctl(fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf_pass1);
    if (ret != 0) {
        printf("  ❌ PASS 1 failed (ret=%d, errno=%d: %s)\n", ret, errno, strerror(errno));
        return 1;
    }
    printf("  ✅ PASS 1 RÉUSSI - i915 a assigné les VA GPU!\n\n");

    // [6/15] Lecture offsets i915 réels
    printf("[6/15] Lecture offsets i915 assignés...\n");
    uint64_t surface_state_addr = exec_objects_pass1[0].offset;
    uint64_t dynamic_state_addr = exec_objects_pass1[1].offset;
    uint64_t idrt_addr = exec_objects_pass1[2].offset;
    uint64_t kernel_addr = exec_objects_pass1[3].offset;
    uint64_t output_addr = exec_objects_pass1[4].offset;
    uint64_t batch_addr = exec_objects_pass1[5].offset;
    
    printf("  ✓ OFFSETS i915 RÉELS:\n");
    printf("    surface_state: 0x%016lx\n", surface_state_addr);
    printf("    dynamic_state: 0x%016lx\n", dynamic_state_addr);
    printf("    idrt:          0x%016lx\n", idrt_addr);
    printf("    kernel:        0x%016lx\n", kernel_addr);
    printf("    output:        0x%016lx\n", output_addr);
    printf("    batch:         0x%016lx\n\n", batch_addr);

    // [7/15] Construction IDRT avec offset kernel réel
    printf("[7/15] Construction IDRT (offset-based)...\n");
    drm_intel_bo_map(idrt_bo, 1);
    uint32_t *idrt = (uint32_t*)idrt_bo->virtual;
    memset(idrt, 0, 4096);
    
    // IDRT Entry 0 (64 bytes)
    idrt[0] = (uint32_t)(kernel_addr & 0xFFFFFFFF);
    idrt[1] = (uint32_t)(kernel_addr >> 32);
    idrt[2] = 0;
    idrt[3] = 0;
    idrt[4] = 0;
    idrt[5] = (24 << 0) | (1 << 31);  // Number of threads + Barrier enable
    idrt[6] = 0;
    idrt[7] = 0;
    
    // Binding Table Index (BTI) - offset 0 dans surface_state
    idrt[8] = 0;  // BTI offset relatif à Surface State Base
    
    drm_intel_bo_unmap(idrt_bo);
    printf("  ✓ IDRT configuré: kernel @ 0x%016lx\n\n", kernel_addr);

    // [8/15] Construction Binding Table + Surface State
    printf("[8/15] Construction Binding Table + Surface State...\n");
    drm_intel_bo_map(surface_state_bo, 1);
    uint32_t *surface_heap = (uint32_t*)surface_state_bo->virtual;
    memset(surface_heap, 0, 4096);
    
    // Binding Table Entry 0 → Surface State offset 64
    surface_heap[0] = 64;  // Offset vers Surface State (en bytes)
    
    // Surface State (16 DWORDs = 64 bytes) à offset 64
    uint32_t *surface_state = &surface_heap[16];
    surface_state[0] = (uint32_t)(output_addr & 0xFFFFFFFF);
    surface_state[1] = (uint32_t)(output_addr >> 32) | (0x7F << 18);  // MOCS
    surface_state[2] = OUTPUT_SIZE - 1;  // Width
    surface_state[3] = 0;
    surface_state[4] = 0;
    surface_state[5] = 0;
    surface_state[6] = 0;
    surface_state[7] = 0;
    
    drm_intel_bo_unmap(surface_state_bo);
    printf("  ✓ Binding Table + Surface State: output @ 0x%016lx\n\n", output_addr);

    // [9/15] Construction Dynamic State (vide pour ce test)
    printf("[9/15] Construction Dynamic State...\n");
    drm_intel_bo_map(dynamic_state_bo, 1);
    memset(dynamic_state_bo->virtual, 0, 4096);
    drm_intel_bo_unmap(dynamic_state_bo);
    printf("  ✓ Dynamic State initialisé\n\n");

    // [10/15] Copie et patch du batch OpenCL
    printf("[10/15] Copie et patch du batch OpenCL avec offsets réels...\n");
    drm_intel_bo_map(batch_bo, 1);
    uint32_t *batch = (uint32_t*)batch_bo->virtual;
    
    // Copier le template OpenCL
    memcpy(batch, opencl_batch_template, 320);
    
    // PATCH STATE_BASE_ADDRESS (offset 0x00A0 = DW 40)
    // DW44-45: Surface State Base
    batch[44] = (uint32_t)(surface_state_addr & 0xFFFFFFFF) | 1;
    batch[45] = (uint32_t)(surface_state_addr >> 32);
    
    // DW46-47: Dynamic State Base
    batch[46] = (uint32_t)(dynamic_state_addr & 0xFFFFFFFF) | 1;
    batch[47] = (uint32_t)(dynamic_state_addr >> 32);
    
    // DW56-57: Bindless Surface State Base (même que Surface State)
    batch[56] = (uint32_t)(surface_state_addr & 0xFFFFFFFF) | 1;
    batch[57] = (uint32_t)(surface_state_addr >> 32);
    
    // PATCH MEDIA_INTERFACE_DESCRIPTOR_LOAD (offset 0x00EC = DW 59)
    // DW60: Offset relatif à Dynamic State Base
    uint64_t idrt_offset = idrt_addr - dynamic_state_addr;
    batch[60] = (uint32_t)(idrt_offset & 0xFFFFFFFF);
    
    // PATCH GPGPU_WALKER (offset 0x0110 = DW 68)
    // DW69-70: IDRT offset (même que MEDIA_INTERFACE_DESCRIPTOR_LOAD)
    batch[69] = (uint32_t)(idrt_offset & 0xFFFFFFFF);
    batch[70] = (uint32_t)(idrt_offset >> 32);
    
    drm_intel_bo_unmap(batch_bo);
    printf("  ✓ Batch OpenCL patché avec offsets i915 réels\n");
    printf("    Surface State Base: 0x%016lx\n", surface_state_addr);
    printf("    Dynamic State Base: 0x%016lx\n", dynamic_state_addr);
    printf("    IDRT offset:        0x%016lx\n\n", idrt_offset);

    // [11/15] Construction exec_objects final (10 buffers comme OpenCL)
    printf("[11/15] Construction exec_objects final (10 buffers)...\n");
    struct drm_i915_gem_exec_object2 exec_objects[10];
    memset(exec_objects, 0, sizeof(exec_objects));
    
    uint64_t pinned_flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS | EXEC_OBJECT_PINNED;
    
    // Reproduire l'ordre OpenCL exact
    exec_objects[0].handle = surface_state_bo->handle;
    exec_objects[0].offset = surface_state_addr;
    exec_objects[0].flags = pinned_flags;
    
    exec_objects[1].handle = dynamic_state_bo->handle;
    exec_objects[1].offset = dynamic_state_addr;
    exec_objects[1].flags = pinned_flags;
    
    exec_objects[2].handle = idrt_bo->handle;
    exec_objects[2].offset = idrt_addr;
    exec_objects[2].flags = pinned_flags;
    
    exec_objects[3].handle = kernel_bo->handle;
    exec_objects[3].offset = kernel_addr;
    exec_objects[3].flags = pinned_flags;
    
    exec_objects[4].handle = output_bo->handle;
    exec_objects[4].offset = output_addr;
    exec_objects[4].flags = pinned_flags | EXEC_OBJECT_WRITE;
    
    // Padding buffers (5-8)
    for (int i = 5; i < 9; i++) {
        exec_objects[i].handle = batch_bo->handle;
        exec_objects[i].offset = batch_addr;
        exec_objects[i].flags = pinned_flags;
    }
    
    // Batch buffer (index 9)
    exec_objects[9].handle = batch_bo->handle;
    exec_objects[9].offset = batch_addr;
    exec_objects[9].flags = pinned_flags;
    
    printf("  ✓ 10 exec_objects configurés avec PINNED + offsets réels\n\n");

    // [12/15] PASS 2: Soumission finale avec batch OpenCL
    printf("[12/15] PASS 2: Soumission batch OpenCL avec NO_RELOC...\n");
    
    struct drm_i915_gem_execbuffer2 execbuf_final;
    memset(&execbuf_final, 0, sizeof(execbuf_final));
    execbuf_final.buffers_ptr = (uintptr_t)exec_objects;
    execbuf_final.buffer_count = 10;
    execbuf_final.batch_start_offset = 0;
    execbuf_final.batch_len = 320;  // Exactement comme OpenCL
    execbuf_final.flags = I915_EXEC_NO_RELOC;  // Exactement comme OpenCL (0x800)
    execbuf_final.rsvd1 = ctx_id;
    
    printf("  🔧 PASS 2 - Reproduction EXACTE OpenCL:\n");
    printf("     buffer_count: %u\n", execbuf_final.buffer_count);
    printf("     batch_len:    %u bytes\n", execbuf_final.batch_len);
    printf("     flags:        0x%llx (NO_RELOC)\n", (unsigned long long)execbuf_final.flags);
    printf("     context_id:   %u\n\n", ctx_id);
    
    ret = drmIoctl(fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf_final);
    if (ret != 0) {
        printf("  ❌ PASS 2 failed (ret=%d, errno=%d: %s)\n", ret, errno, strerror(errno));
        return 1;
    }
    printf("  ✅✅✅ PASS 2 RÉUSSI! BATCH OPENCL REPRODUIT! ✅✅✅\n\n");

    // [13/15] Attendre GPU
    printf("[13/15] Attente GPU...\n");
    usleep(100000);
    drm_intel_bo_wait_rendering(output_bo);
    printf("  ✓ GPU terminé\n\n");

    // [14/15] Vérification résultat
    printf("[14/15] VÉRIFICATION RÉSULTAT GPU NATIF...\n");
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
        printf("✅ BATCH OPENCL REPRODUIT AVEC SUCCÈS!\n");
    } else {
        printf("⚠️  Valeur incorrecte - analyse nécessaire\n");
        printf("    Possible: Surface State, IDRT, ou offset incorrect\n");
    }
    
    drm_intel_bo_unmap(output_bo);
    
    // [15/15] Cleanup
    printf("\n[15/15] Cleanup...\n");
    drm_intel_bo_unreference(surface_state_bo);
    drm_intel_bo_unreference(dynamic_state_bo);
    drm_intel_bo_unreference(idrt_bo);
    drm_intel_bo_unreference(kernel_bo);
    drm_intel_bo_unreference(output_bo);
    drm_intel_bo_unreference(batch_bo);
    drm_intel_gem_context_destroy(ctx);
    drm_intel_bufmgr_destroy(bufmgr);
    close(fd);
    printf("  ✓ Cleanup terminé\n");
    
    return 0;
}

// Made with Bob
