// TEST C527 - GPGPU_WALKER AVEC DIMENSIONS (ROOT CAUSE #36)
// ROOT CAUSE #36: Dump OpenCL contient walker avec dimensions nulles
// Solution: Remplir manuellement Thread Width/Height/Depth dans le walker

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

// Batch OpenCL capturé EXACT (320 bytes)
static const uint8_t opencl_batch_exact[320] = {
    0x12, 0x13, 0x04, 0x69, 0x04, 0x00, 0x00, 0x7a, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x11,
    0x04, 0xe4, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x11, 0x34, 0x70, 0x00, 0x00,
    0x21, 0x03, 0x00, 0x60, 0x01, 0x00, 0x04, 0x61, 0x00, 0x00, 0x00, 0x35, 0xfa, 0x78, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x7a, 0x21, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x70, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xa7, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x82, 0x07,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x11,
    0x80, 0x25, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x04, 0x00, 0x00, 0x7a, 0x20, 0x04, 0x10, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x11, 0x00, 0x01, 0x61, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00,
    0x01, 0x60, 0xd0, 0x37, 0xfa, 0x78, 0x00, 0x00, 0x01, 0xe0, 0x01, 0x38, 0xfa, 0x78, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x01, 0x80, 0xff, 0xff, 0x41, 0x00, 0x00, 0x00, 0x01, 0x80, 0x00, 0x00,
    0x01, 0xf0, 0xff, 0xff, 0x01, 0x00, 0x01, 0x00, 0x01, 0xf0, 0xff, 0xff, 0x01, 0xf0, 0xff, 0xff,
    0x01, 0x60, 0xd0, 0x37, 0xfa, 0x78, 0x00, 0x00, 0x00, 0xf0, 0x3b, 0x00, 0x01, 0x00, 0x02, 0x61,
    0x00, 0xe0, 0xfe, 0xff, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x7a, 0x00, 0x00, 0x10, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x01, 0x80, 0x18, 0x00, 0xf0, 0x04, 0x38, 0xfa, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

int main(void) {
    printf("\n╔════════════════════════════════════════════════════════════════╗\n");
    printf("║  TEST C527 - WALKER DIMENSIONS (ROOT CAUSE #36)             ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n\n");
    printf("ROOT CAUSE #36: Dump OpenCL contient walker avec dimensions nulles\n");
    printf("Solution: Remplir manuellement Thread Width/Height/Depth\n\n");

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

    // [3/15] Allocation buffers
    printf("[3/15] Allocation buffers...\n");
    drm_intel_bo *surface_state_bo = drm_intel_bo_alloc(bufmgr, "surface_state", 4096, 4096);
    drm_intel_bo *dynamic_state_bo = drm_intel_bo_alloc(bufmgr, "dynamic_state", 4096, 4096);
    drm_intel_bo *idrt_bo = drm_intel_bo_alloc(bufmgr, "idrt", 4096, 4096);
    drm_intel_bo *kernel_bo = drm_intel_bo_alloc(bufmgr, "kernel", KERNEL_SIZE, 4096);
    drm_intel_bo *output_bo = drm_intel_bo_alloc(bufmgr, "output", OUTPUT_SIZE, 4096);
    drm_intel_bo *batch_bo = drm_intel_bo_alloc(bufmgr, "batch", BATCH_SIZE, 4096);
    
    if (!surface_state_bo || !dynamic_state_bo || !idrt_bo || 
        !kernel_bo || !output_bo || !batch_bo) {
        printf("  ❌ Échec allocation buffers\n");
        return 1;
    }
    printf("  ✓ 6 buffers alloués\n\n");

    // [4/15] Init output + kernel
    printf("[4/15] Initialisation output + kernel...\n");
    drm_intel_bo_map(output_bo, 1);
    uint32_t *output = (uint32_t*)output_bo->virtual;
    for (int i = 0; i < 1024; i++) output[i] = 0xFFFFFFFF;
    drm_intel_bo_unmap(output_bo);
    
    drm_intel_bo_map(kernel_bo, 1);
    memcpy(kernel_bo->virtual, kernel_isa_gen9, sizeof(kernel_isa_gen9));
    drm_intel_bo_unmap(kernel_bo);
    printf("  ✓ Output et kernel initialisés\n\n");

    // [5/15] PASS 1: Résolution offsets i915
    printf("[5/15] PASS 1: Résolution offsets i915...\n");
    
    drm_intel_context *ctx = drm_intel_gem_context_create(bufmgr);
    if (!ctx) {
        printf("  ❌ Échec création context\n");
        return 1;
    }
    
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
    
    int ret = drmIoctl(fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf_pass1);
    if (ret != 0) {
        printf("  ❌ PASS 1 failed (ret=%d, errno=%d: %s)\n", ret, errno, strerror(errno));
        return 1;
    }
    printf("  ✅ PASS 1 RÉUSSI\n\n");

    // [6/15] Lecture offsets i915
    printf("[6/15] Lecture offsets i915...\n");
    uint64_t surface_state_addr = exec_objects_pass1[0].offset;
    uint64_t dynamic_state_addr = exec_objects_pass1[1].offset;
    uint64_t idrt_addr = exec_objects_pass1[2].offset;
    uint64_t kernel_addr = exec_objects_pass1[3].offset;
    uint64_t output_addr = exec_objects_pass1[4].offset;
    uint64_t batch_addr = exec_objects_pass1[5].offset;
    
    printf("  ✓ OFFSETS i915:\n");
    printf("    surface_state: 0x%016lx\n", surface_state_addr);
    printf("    dynamic_state: 0x%016lx\n", dynamic_state_addr);
    printf("    idrt:          0x%016lx\n", idrt_addr);
    printf("    kernel:        0x%016lx\n", kernel_addr);
    printf("    output:        0x%016lx\n", output_addr);
    printf("    batch:         0x%016lx\n\n", batch_addr);

    // [7/15] Charger buffers OpenCL depuis dumps
    printf("[7/15] Chargement buffers OpenCL depuis dumps...\n");
    
    FILE *f_buf03 = fopen("/tmp/opencl_batch_dump_buffer_03_0.bin", "rb");
    FILE *f_buf08 = fopen("/tmp/opencl_batch_dump_buffer_08_0.bin", "rb");
    
    if (!f_buf03 || !f_buf08) {
        printf("  ❌ Échec ouverture dumps OpenCL\n");
        return 1;
    }
    
    drm_intel_bo_map(surface_state_bo, 1);
    fread(surface_state_bo->virtual, 1, 4096, f_buf03);
    drm_intel_bo_unmap(surface_state_bo);
    fclose(f_buf03);
    
    drm_intel_bo_map(idrt_bo, 1);
    fread(idrt_bo->virtual, 1, 4096, f_buf08);
    drm_intel_bo_unmap(idrt_bo);
    fclose(f_buf08);
    
    printf("  ✓ Buffers OpenCL chargés\n\n");

    // [8/15] Patcher output address dans Surface State
    printf("[8/15] Patch output address dans Surface State...\n");
    drm_intel_bo_map(surface_state_bo, 1);
    uint32_t *surface_heap = (uint32_t*)surface_state_bo->virtual;
    uint32_t *surface_state = &surface_heap[16];  // offset 0x40
    surface_state[6] = (uint32_t)(output_addr & 0xFFFFFFFF);
    surface_state[7] = (uint32_t)(output_addr >> 32);
    drm_intel_bo_unmap(surface_state_bo);
    printf("  ✓ Output address patché: 0x%016lx\n\n", output_addr);

    // [9/15] Patcher kernel pointer dans IDRT
    printf("[9/15] Patch kernel pointer dans IDRT...\n");
    drm_intel_bo_map(idrt_bo, 1);
    uint32_t *idrt = (uint32_t*)idrt_bo->virtual;
    idrt[0] = (uint32_t)(kernel_addr & 0xFFFFFFFF);
    idrt[1] = (uint32_t)(kernel_addr >> 32);
    drm_intel_bo_unmap(idrt_bo);
    printf("  ✓ Kernel pointer patché: 0x%016lx\n\n", kernel_addr);

    // [10/15] Copier batch OpenCL EXACT
    printf("[10/15] Copie batch OpenCL EXACT (320 bytes)...\n");
    drm_intel_bo_map(batch_bo, 1);
    memcpy(batch_bo->virtual, opencl_batch_exact, 320);
    uint32_t *batch = (uint32_t*)batch_bo->virtual;
    
    // Patcher STATE_BASE_ADDRESS (offset 0xA0 = DW 40)
    batch[44] = (uint32_t)(surface_state_addr & 0xFFFFFFFF) | 1;
    batch[45] = (uint32_t)(surface_state_addr >> 32);
    batch[46] = (uint32_t)(dynamic_state_addr & 0xFFFFFFFF) | 1;
    batch[47] = (uint32_t)(dynamic_state_addr >> 32);
    batch[56] = (uint32_t)(surface_state_addr & 0xFFFFFFFF) | 1;
    batch[57] = (uint32_t)(surface_state_addr >> 32);
    
    // Patcher MEDIA_INTERFACE_DESCRIPTOR_LOAD (offset 0xEC = DW 59)
    uint64_t idrt_offset = idrt_addr - dynamic_state_addr;
    batch[60] = (uint32_t)(idrt_offset & 0xFFFFFFFF);
    
    // Patcher GPGPU_WALKER (offset 0x110 = DW 68)
    batch[69] = (uint32_t)(idrt_offset & 0xFFFFFFFF);
    batch[70] = (uint32_t)(idrt_offset >> 32);
    
    printf("  ✓ Batch OpenCL copié et patché\n\n");

    // [11/15] REMPLIR DIMENSIONS WALKER (ROOT CAUSE #36)
    printf("[11/15] REMPLIR DIMENSIONS WALKER (ROOT CAUSE #36)...\n");
    
    // GPGPU_WALKER @ offset 0x110 (DW 68)
    // DW0 (68): Opcode 0x18800101
    // DW1-2 (69-70): IDRT offset (déjà patché)
    // DW3 (71): Indirect Data Length = 0
    // DW4 (72): Thread Width Count X = 1
    // DW5 (73): Thread Height Count Y = 1  
    // DW6 (74): Thread Depth Count Z = 1
    // DW7 (75): Thread Group ID Starting X = 0
    // DW8 (76): Thread Group ID X Dimension = 1
    // DW9 (77): Thread Group ID Starting Y = 0
    // DW10 (78): Thread Group ID Y Dimension = 1
    // DW11 (79): Thread Group ID Starting Z = 0
    // DW12 (80): Thread Group ID Z Dimension = 1
    // DW13 (81): Right Execution Mask = 0xFFFFFFFF
    // DW14 (82): Bottom Execution Mask = 0xFFFFFFFF
    
    batch[72] = 1;  // Thread Width = 1
    batch[73] = 1;  // Thread Height = 1
    batch[74] = 1;  // Thread Depth = 1
    batch[76] = 1;  // Thread Group X Dimension = 1
    batch[78] = 1;  // Thread Group Y Dimension = 1
    batch[80] = 1;  // Thread Group Z Dimension = 1
    batch[81] = 0xFFFFFFFF;  // Right Execution Mask
    batch[82] = 0xFFFFFFFF;  // Bottom Execution Mask
    
    drm_intel_bo_unmap(batch_bo);
    
    printf("  ✓ Dimensions WALKER remplies:\n");
    printf("    Thread Width  = 1\n");
    printf("    Thread Height = 1\n");
    printf("    Thread Depth  = 1\n");
    printf("    Thread Groups = 1 x 1 x 1\n");
    printf("    Execution Masks = 0xFFFFFFFF\n\n");

    // [12/15] Construction exec_objects final
    printf("[12/15] Construction exec_objects final...\n");
    struct drm_i915_gem_exec_object2 exec_objects[10];
    memset(exec_objects, 0, sizeof(exec_objects));
    
    uint64_t pinned_flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS | EXEC_OBJECT_PINNED;
    
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
    
    for (int i = 5; i < 9; i++) {
        exec_objects[i].handle = batch_bo->handle;
        exec_objects[i].offset = batch_addr;
        exec_objects[i].flags = pinned_flags;
    }
    
    exec_objects[9].handle = batch_bo->handle;
    exec_objects[9].offset = batch_addr;
    exec_objects[9].flags = pinned_flags;
    
    printf("  ✓ 10 exec_objects configurés\n\n");

    // [13/15] PASS 2: Soumission finale
    printf("[13/15] PASS 2: Soumission batch AVEC DIMENSIONS...\n");
    
    struct drm_i915_gem_execbuffer2 execbuf_final;
    memset(&execbuf_final, 0, sizeof(execbuf_final));
    execbuf_final.buffers_ptr = (uintptr_t)exec_objects;
    execbuf_final.buffer_count = 10;
    execbuf_final.batch_start_offset = 0;
    execbuf_final.batch_len = 320;
    execbuf_final.flags = I915_EXEC_NO_RELOC;
    execbuf_final.rsvd1 = ctx_id;
    
    printf("  🔧 PASS 2 - WALKER AVEC DIMENSIONS:\n");
    printf("     Batch: Copie exacte OpenCL\n");
    printf("     Walker: Dimensions 1x1x1 remplies\n");
    printf("     Thread Groups: 1x1x1\n\n");
    
    ret = drmIoctl(fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf_final);
    if (ret != 0) {
        printf("  ❌ PASS 2 failed (ret=%d, errno=%d: %s)\n", ret, errno, strerror(errno));
        return 1;
    }
    printf("  ✅✅✅ PASS 2 RÉUSSI! ✅✅✅\n\n");

    // [14/15] Attente GPU
    printf("[14/15] Attente GPU...\n");
    drm_intel_bo_wait_rendering(output_bo);
    printf("  ✓ GPU terminé\n\n");

    // [15/15] VÉRIFICATION RÉSULTAT
    printf("[15/15] VÉRIFICATION RÉSULTAT GPU NATIF...\n");
    printf("═══════════════════════════════════════════════════════════════\n");
    
    drm_intel_bo_map(output_bo, 0);
    output = (uint32_t*)output_bo->virtual;
    
    printf("output[0] = 0x%08X (attendu: 0x12345678)\n", output[0]);
    printf("output[1] = 0x%08X\n", output[1]);
    printf("output[2] = 0x%08X\n", output[2]);
    printf("output[3] = 0x%08X\n", output[3]);
    
    int success = (output[0] == 0x12345678);
    drm_intel_bo_unmap(output_bo);
    
    printf("\n[RÉSULTAT FINAL]\n");
    if (success) {
        printf("  ✅✅✅ SUCCÈS! GPU NATIF i915 FONCTIONNEL! ✅✅✅\n");
        printf("  🎯 ROOT CAUSE #36 RÉSOLU!\n");
        printf("  🚀 ÉLIMINATION OPENCL: 100%% COMPLÈTE!\n");
    } else {
        printf("  ⚠️  Debug supplémentaire requis\n");
    }
    
    // Cleanup
    drm_intel_bo_unreference(surface_state_bo);
    drm_intel_bo_unreference(dynamic_state_bo);
    drm_intel_bo_unreference(idrt_bo);
    drm_intel_bo_unreference(kernel_bo);
    drm_intel_bo_unreference(output_bo);
    drm_intel_bo_unreference(batch_bo);
    drm_intel_gem_context_destroy(ctx);
    drm_intel_bufmgr_destroy(bufmgr);
    close(fd);
    
    return success ? 0 : 1;
}

// Made with Bob
