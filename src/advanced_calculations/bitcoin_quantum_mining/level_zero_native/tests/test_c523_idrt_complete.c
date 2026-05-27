// TEST C523 - IDRT STRUCTURE COMPLÈTE + CURBE DATA
// ROOT CAUSE #33: IDRT structure incomplète (flags manquants)
// Solution: Reproduire IDRT exact d'OpenCL buffer[8] avec tous les flags

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
    0x69041312, 0x7a000004, 0x00100000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x11000001,
    0x0000e404, 0x00000100, 0x11000001, 0x00007034, 0x60000321, 0x61040001, 0x35000000, 0x000078fa,
    0x7a000004, 0x00101021, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x70000007, 0x00000000, 0x00000000, 0x00a70100, 0x00000000, 0x07820000, 0x00000000, 0x00000000,
    0x00000000, 0x11000001, 0x00002580, 0x00060000, 0x7a000004, 0x00100420, 0x00000000, 0x00000000,
    0x00000000, 0x00000000,
    0x61010011, 0x00000001, 0x00000000, 0x00040000,
    0x37d06001, 0x000078fa,  // Surface State Base (À PATCHER)
    0x3801e001, 0x000078fa,  // Dynamic State Base (À PATCHER)
    0x00000001, 0xffff8001, 0x00000041, 0x00008001,
    0xfffff001, 0x00010001, 0xfffff001, 0xfffff001,
    0x37d06001, 0x000078fa, 0x003bf000,
    0x61020001, 0xfffee000, 0x00000000,
    0x7a000004, 0x00100000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x18800101,
    0x3804f000, 0x000078fa,  // IDRT offset (À PATCHER)
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000
};

int main(void) {
    printf("\n╔════════════════════════════════════════════════════════════════╗\n");
    printf("║  TEST C523 - IDRT STRUCTURE COMPLÈTE + CURBE DATA           ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n\n");
    printf("ROOT CAUSE #33: IDRT structure incomplète (flags manquants)\n");
    printf("Solution: IDRT exact d'OpenCL buffer[8] avec tous les flags\n\n");

    // [1/16] Ouvrir DRM
    printf("[1/16] Ouverture /dev/dri/renderD128...\n");
    int fd = open("/dev/dri/renderD128", O_RDWR);
    if (fd < 0) {
        printf("  ❌ Échec ouverture DRM\n");
        return 1;
    }
    printf("  ✓ DRM ouvert (fd=%d)\n\n", fd);

    // [2/16] Init libdrm_intel
    printf("[2/16] Initialisation libdrm_intel...\n");
    drm_intel_bufmgr *bufmgr = drm_intel_bufmgr_gem_init(fd, 4096);
    if (!bufmgr) {
        printf("  ❌ Échec init bufmgr\n");
        close(fd);
        return 1;
    }
    printf("  ✓ Buffer manager initialisé\n\n");

    // [3/16] Allocation buffers
    printf("[3/16] Allocation buffers...\n");
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

    // [4/16] Init output + kernel
    printf("[4/16] Initialisation output + kernel...\n");
    drm_intel_bo_map(output_bo, 1);
    uint32_t *output = (uint32_t*)output_bo->virtual;
    for (int i = 0; i < 1024; i++) output[i] = 0xFFFFFFFF;
    drm_intel_bo_unmap(output_bo);
    
    drm_intel_bo_map(kernel_bo, 1);
    memcpy(kernel_bo->virtual, kernel_isa_gen9, sizeof(kernel_isa_gen9));
    drm_intel_bo_unmap(kernel_bo);
    printf("  ✓ Output et kernel initialisés\n\n");

    // [5/16] PASS 1: Résolution offsets i915
    printf("[5/16] PASS 1: Résolution offsets i915...\n");
    
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

    // [6/16] Lecture offsets i915
    printf("[6/16] Lecture offsets i915...\n");
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

    // [7/16] Construction Surface State Gen9 EXACT (format OpenCL buffer[3])
    printf("[7/16] Construction Surface State Gen9 EXACT...\n");
    drm_intel_bo_map(surface_state_bo, 1);
    uint32_t *surface_heap = (uint32_t*)surface_state_bo->virtual;
    memset(surface_heap, 0, 4096);
    
    // Binding Table à offset 0x1A0 (416 bytes) - pointe vers Surface State à offset 0x40
    // OpenCL utilise 0x1A0, pas 0x80!
    surface_heap[104] = 0x00000040;  // BTI[0] = offset 0x40 (64 bytes) @ 0x1A0
    
    // Surface State à offset 0x40 (16 DWORDs = 64 bytes) - FORMAT GEN9 EXACT
    uint32_t *surface_state = &surface_heap[16];  // offset 0x40
    
    // DW0: Surface Type + Format + MOCS (copié d'OpenCL: 0x87fd4000)
    surface_state[0] = 0x87fd4000;
    //   Bits 29-27: Surface Type = 4 (SURFTYPE_BUFFER)
    //   Bits 26-18: Format = 0x1FF (RAW)
    //   Bits 17-12: MOCS = 0x3D (cache policy)
    
    // DW1: Width (copié d'OpenCL: 0x04000000)
    surface_state[1] = 0x04000000;  // Width = 0x400 (1024 DWORDs)
    
    // DW2: Height/Depth/Pitch (copié d'OpenCL: 0x00080003)
    surface_state[2] = 0x00080003;
    
    // DW3: Reserved
    surface_state[3] = 0x00000000;
    
    // DW4: Reserved
    surface_state[4] = 0x00000000;
    
    // DW5: Reserved
    surface_state[5] = 0x00000000;
    
    // DW6: Base Address LOW (output buffer)
    surface_state[6] = (uint32_t)(output_addr & 0xFFFFFFFF);
    
    // DW7: Base Address HIGH
    surface_state[7] = (uint32_t)(output_addr >> 32);
    
    // DW8-15: Reserved
    for (int i = 8; i < 16; i++) {
        surface_state[i] = 0x00000000;
    }
    
    drm_intel_bo_unmap(surface_state_bo);
    printf("  ✓ Surface State Gen9: output @ 0x%016lx\n", output_addr);
    printf("  ✓ Binding Table @ offset 0x1A0 (OpenCL format)\n");
    printf("  ✓ Binding Table[0] = offset 0x40\n\n");

    // [8/16] Construction IDRT COMPLET avec tous les flags OpenCL
    printf("[8/16] Construction IDRT COMPLET (format OpenCL buffer[8])...\n");
    drm_intel_bo_map(idrt_bo, 1);
    uint32_t *idrt = (uint32_t*)idrt_bo->virtual;
    memset(idrt, 0, 4096);
    
    // IDRT Entry 0 (8 DWORDs = 32 bytes) - FORMAT EXACT OpenCL
    // DW0-1: Kernel Start Pointer (64-bit)
    idrt[0] = (uint32_t)(kernel_addr & 0xFFFFFFFF);
    idrt[1] = (uint32_t)(kernel_addr >> 32);
    
    // DW2: Sampler State Pointer (0 = none)
    idrt[2] = 0x00000000;
    
    // DW3: Binding Table Pointer + FLAGS (format OpenCL: 0x7105000d)
    // Bits 0-4: BTI offset / 32 = 0x1A0 / 32 = 13 (0x0d)
    // Bits 5-31: Flags = 0x710500 (Surface State Base Address enable, etc.)
    uint32_t bti_offset_units = 0x1A0 / 32;  // 13 (0x0d)
    idrt[3] = bti_offset_units | (0x710500 << 5);
    
    // DW4: Binding Table Entry Count + Thread Group Config (OpenCL: 0x00000640)
    // Bits 0-4: Sampler Count = 0
    // Bits 5-9: Binding Table Entry Count = 1
    // Bits 10-31: Thread Group Config = 0x0640
    idrt[4] = (0 << 0) | (1 << 5) | (0x0640 << 10);
    
    // DW5: Thread Group Size + CURBE + Barrier (OpenCL: 0xfffdd000 + 0x80000007)
    // Bits 0-15: CURBE Read Length = 7 (7 * 32 bytes = 224 bytes)
    // Bits 16-31: Thread Group Size = 0xfffd
    // Bit 31: Barrier Enable = 1
    idrt[5] = 0xfffdd000 | (7 << 0) | (1 << 31);
    
    // DW6-7: Reserved
    idrt[6] = 0x00000000;
    idrt[7] = 0x00000000;
    
    drm_intel_bo_unmap(idrt_bo);
    printf("  ✓ IDRT: kernel @ 0x%016lx\n", kernel_addr);
    printf("  ✓ IDRT DW3: BTI pointer = 0x%08X (avec flags OpenCL)\n", idrt[3]);
    printf("  ✓ IDRT DW4: Entry Count + Thread Config = 0x%08X\n", idrt[4]);
    printf("  ✓ IDRT DW5: CURBE Length=7 + Thread Size + Barrier = 0x%08X\n\n", idrt[5]);

    // [9/16] Construction Dynamic State + CURBE Data
    printf("[9/16] Construction Dynamic State + CURBE Data...\n");
    drm_intel_bo_map(dynamic_state_bo, 1);
    uint32_t *dynamic_state = (uint32_t*)dynamic_state_bo->virtual;
    memset(dynamic_state, 0, 4096);
    
    // CURBE Data (Constant URB Entry) - Arguments kernel
    // Offset 0x100 (256 bytes) dans dynamic_state_bo
    // Total: 7 * 32 bytes = 224 bytes (comme spécifié dans IDRT DW5)
    uint32_t *curbe = &dynamic_state[64];  // Offset 0x100
    
    // CURBE Layout pour kernel test_store_validation:
    // [0-1]: Output buffer pointer (64-bit)
    // [2-4]: Global work size (X, Y, Z)
    // [5-7]: Local work size (X, Y, Z)
    // [8-10]: Global offset (X, Y, Z)
    // ... reste pour autres arguments
    
    curbe[0] = (uint32_t)(output_addr & 0xFFFFFFFF);  // Output pointer LOW
    curbe[1] = (uint32_t)(output_addr >> 32);          // Output pointer HIGH
    curbe[2] = 256;  // Global size X (256 threads)
    curbe[3] = 1;    // Global size Y
    curbe[4] = 1;    // Global size Z
    curbe[5] = 256;  // Local size X
    curbe[6] = 1;    // Local size Y
    curbe[7] = 1;    // Local size Z
    curbe[8] = 0;    // Global offset X
    curbe[9] = 0;    // Global offset Y
    curbe[10] = 0;   // Global offset Z
    
    drm_intel_bo_unmap(dynamic_state_bo);
    printf("  ✓ Dynamic State initialisé\n");
    printf("  ✓ CURBE Data: output @ 0x%016lx\n", output_addr);
    printf("  ✓ CURBE Data: global_size = (256, 1, 1)\n\n");

    // [10/16] Patch batch OpenCL
    printf("[10/16] Patch batch OpenCL avec offsets réels...\n");
    drm_intel_bo_map(batch_bo, 1);
    uint32_t *batch = (uint32_t*)batch_bo->virtual;
    memcpy(batch, opencl_batch_template, 320);
    
    // STATE_BASE_ADDRESS (offset 0x00A0 = DW 40)
    batch[44] = (uint32_t)(surface_state_addr & 0xFFFFFFFF) | 1;
    batch[45] = (uint32_t)(surface_state_addr >> 32);
    batch[46] = (uint32_t)(dynamic_state_addr & 0xFFFFFFFF) | 1;
    batch[47] = (uint32_t)(dynamic_state_addr >> 32);
    batch[56] = (uint32_t)(surface_state_addr & 0xFFFFFFFF) | 1;
    batch[57] = (uint32_t)(surface_state_addr >> 32);
    
    // MEDIA_INTERFACE_DESCRIPTOR_LOAD (offset 0x00EC = DW 59)
    uint64_t idrt_offset = idrt_addr - dynamic_state_addr;
    batch[60] = (uint32_t)(idrt_offset & 0xFFFFFFFF);
    
    // GPGPU_WALKER (offset 0x0110 = DW 68)
    batch[69] = (uint32_t)(idrt_offset & 0xFFFFFFFF);
    batch[70] = (uint32_t)(idrt_offset >> 32);
    
    drm_intel_bo_unmap(batch_bo);
    printf("  ✓ Batch patché\n\n");

    // [11/16] Construction exec_objects final
    printf("[11/16] Construction exec_objects final...\n");
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

    // [12/16] PASS 2: Soumission finale
    printf("[12/16] PASS 2: Soumission batch OpenCL...\n");
    
    struct drm_i915_gem_execbuffer2 execbuf_final;
    memset(&execbuf_final, 0, sizeof(execbuf_final));
    execbuf_final.buffers_ptr = (uintptr_t)exec_objects;
    execbuf_final.buffer_count = 10;
    execbuf_final.batch_start_offset = 0;
    execbuf_final.batch_len = 320;
    execbuf_final.flags = I915_EXEC_NO_RELOC;
    execbuf_final.rsvd1 = ctx_id;
    
    printf("  🔧 PASS 2 - IDRT COMPLET + CURBE:\n");
    printf("     IDRT DW3: BTI pointer avec flags OpenCL\n");
    printf("     IDRT DW4: Entry Count + Thread Config\n");
    printf("     IDRT DW5: CURBE Length=7 + Barrier\n");
    printf("     CURBE Data: Arguments kernel chargés\n");
    printf("     Surface State: Format Gen9 exact\n\n");
    
    ret = drmIoctl(fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf_final);
    if (ret != 0) {
        printf("  ❌ PASS 2 failed (ret=%d, errno=%d: %s)\n", ret, errno, strerror(errno));
        return 1;
    }
    printf("  ✅✅✅ PASS 2 RÉUSSI! ✅✅✅\n\n");

    // [13/16] Attendre GPU
    printf("[13/16] Attente GPU...\n");
    usleep(100000);
    drm_intel_bo_wait_rendering(output_bo);
    printf("  ✓ GPU terminé\n\n");

    // [14/16] VÉRIFICATION RÉSULTAT
    printf("[14/16] VÉRIFICATION RÉSULTAT GPU NATIF...\n");
    printf("═══════════════════════════════════════════════════════════════\n");
    
    drm_intel_bo_map(output_bo, 0);
    output = (uint32_t*)output_bo->virtual;
    
    printf("output[0] = 0x%08X (attendu: 0x12345678)\n", output[0]);
    printf("output[1] = 0x%08X\n", output[1]);
    printf("output[2] = 0x%08X\n", output[2]);
    printf("output[3] = 0x%08X\n\n", output[3]);
    
    int success = 0;
    if (output[0] == 0x12345678) {
        printf("✅✅✅ SUCCÈS TOTAL! GPU NATIF FONCTIONNE! ✅✅✅\n");
        printf("✅ ÉLIMINATION OPENCL: 100%% COMPLÈTE!\n");
        printf("✅ IDRT STRUCTURE COMPLÈTE VALIDÉE!\n");
        printf("✅ CURBE DATA FONCTIONNEL!\n");
        printf("✅ CHAÎNE COMPLÈTE: IDRT→CURBE→BTI→SurfaceState→Output!\n");
        success = 1;
    } else {
        printf("⚠️  Valeur incorrecte - analyse supplémentaire nécessaire\n");
    }
    
    drm_intel_bo_unmap(output_bo);
    
    // [15/16] Cleanup
    printf("\n[15/16] Cleanup...\n");
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
    
    // [16/16] Résultat final
    printf("\n[16/16] RÉSULTAT FINAL:\n");
    if (success) {
        printf("  ✅ GPU NATIF 100%% FONCTIONNEL!\n");
        printf("  ✅ OPENCL ÉLIMINÉ COMPLÈTEMENT!\n");
        return 0;
    } else {
        printf("  ⚠️  Debug supplémentaire requis\n");
        return 1;
    }
}

// Made with Bob
