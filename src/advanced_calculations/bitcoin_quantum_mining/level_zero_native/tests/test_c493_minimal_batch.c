/*
 * LUMVORAX C493 - BATCH MINIMAL Style OpenCL
 * 
 * Objectif: Créer un batch aussi court que possible, similaire à OpenCL
 * 
 * Découvertes C492:
 * - Batch OpenCL: 320 bytes (80 DWORDs)
 * - 10 buffers séparés
 * - Soft-pinning complet (flags 0x18)
 * - Flags EXECBUFFER2 = 0x800
 * - Aucune relocation
 * 
 * Stratégie:
 * - Batch minimal: GPGPU_WALKER + MEDIA_STATE_FLUSH + PIPE_CONTROL + END
 * - Buffers séparés pour chaque structure
 * - Soft-pinning pur
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <assert.h>

#include <libdrm/drm.h>
#include <libdrm/i915_drm.h>
#include <xf86drm.h>
#include <libdrm/intel_bufmgr.h>

// Kernel ISA Gen9 OpenCL extrait (320 bytes)
#include "test_c491_opencl_isa_gen9.h"

#define OUTPUT_SIZE 4096

// Commandes GPU Gen9
#define CMD_GPGPU_WALKER            0x71050000
#define CMD_MEDIA_STATE_FLUSH       0x70040000
#define CMD_PIPE_CONTROL            0x7a000000
#define MI_BATCH_BUFFER_END         0x0A000000

// Flags buffers (soft-pinning)
#define EXEC_OBJECT_SUPPORTS_48B_ADDRESS  (1<<3)
#define EXEC_OBJECT_PINNED                (1<<4)
#define BUFFER_FLAGS (EXEC_OBJECT_SUPPORTS_48B_ADDRESS | EXEC_OBJECT_PINNED)

int main(void) {
    printf("=== CYCLE C493: BATCH MINIMAL STYLE OPENCL ===\n\n");
    
    // 1. Ouverture DRM
    printf("[1/12] Ouverture /dev/dri/renderD128...\n");
    int fd = open("/dev/dri/renderD128", O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }
    printf("  ✓ fd = %d\n", fd);
    
    // 2. Initialisation bufmgr
    printf("\n[2/12] Initialisation drm_intel_bufmgr...\n");
    drm_intel_bufmgr *bufmgr = drm_intel_bufmgr_gem_init(fd, 4096);
    if (!bufmgr) {
        fprintf(stderr, "❌ drm_intel_bufmgr_gem_init failed\n");
        close(fd);
        return 1;
    }
    drm_intel_bufmgr_gem_enable_reuse(bufmgr);
    printf("  ✓ bufmgr initialisé\n");
    
    // 3. Allocation buffers SÉPARÉS (style OpenCL)
    printf("\n[3/12] Allocation 10 buffers séparés...\n");
    
    drm_intel_bo *batch_bo = drm_intel_bo_alloc(bufmgr, "batch", 4096, 4096);
    drm_intel_bo *output_bo = drm_intel_bo_alloc(bufmgr, "output", OUTPUT_SIZE, 4096);
    drm_intel_bo *kernel_bo = drm_intel_bo_alloc(bufmgr, "kernel", 4096, 64);
    drm_intel_bo *idrt_bo = drm_intel_bo_alloc(bufmgr, "idrt", 4096, 64);
    drm_intel_bo *surface_bo = drm_intel_bo_alloc(bufmgr, "surface", 4096, 64);
    drm_intel_bo *binding_bo = drm_intel_bo_alloc(bufmgr, "binding", 4096, 64);
    drm_intel_bo *curbe_bo = drm_intel_bo_alloc(bufmgr, "curbe", 4096, 64);
    drm_intel_bo *scratch_bo = drm_intel_bo_alloc(bufmgr, "scratch", 4096, 4096);
    drm_intel_bo *state_bo = drm_intel_bo_alloc(bufmgr, "state", 4096, 4096);
    drm_intel_bo *aux_bo = drm_intel_bo_alloc(bufmgr, "aux", 4096, 4096);
    
    if (!batch_bo || !output_bo || !kernel_bo || !idrt_bo || !surface_bo ||
        !binding_bo || !curbe_bo || !scratch_bo || !state_bo || !aux_bo) {
        fprintf(stderr, "❌ Allocation buffers failed\n");
        drm_intel_bufmgr_destroy(bufmgr);
        close(fd);
        return 1;
    }
    
    printf("  ✓ 10 buffers alloués\n");
    
    // 4. Initialisation output buffer
    printf("\n[4/12] Initialisation output buffer...\n");
    drm_intel_bo_map(output_bo, 1);
    uint32_t *output = (uint32_t *)output_bo->virtual;
    for (int i = 0; i < OUTPUT_SIZE/4; i++) {
        output[i] = 0xFFFFFFFF;
    }
    drm_intel_bo_unmap(output_bo);
    printf("  ✓ Output initialisé avec 0xFFFFFFFF\n");
    
    // 5. Copie kernel ISA
    printf("\n[5/12] Copie kernel ISA Gen9 (%zu bytes)...\n", kernel_isa_opencl_gen9_size);
    drm_intel_bo_map(kernel_bo, 1);
    memcpy(kernel_bo->virtual, kernel_isa_opencl_gen9, kernel_isa_opencl_gen9_size);
    drm_intel_bo_unmap(kernel_bo);
    printf("  ✓ Kernel copié\n");
    
    // 6. Construction Surface State
    printf("\n[6/12] Construction Surface State...\n");
    drm_intel_bo_map(surface_bo, 1);
    uint32_t *surface_state = (uint32_t *)surface_bo->virtual;
    memset(surface_state, 0, 64);
    
    // Surface State pour output buffer (16 DWORDs = 64 bytes)
    surface_state[0] = (0 << 29) |      // Surface Type: BUFFER
                       (5 << 18) |      // Surface Format: RAW
                       (0 << 0);        // Render Cache
    surface_state[1] = 0x100000;        // Base Address = output_bo offset
    surface_state[2] = (OUTPUT_SIZE - 1);  // Width-1
    surface_state[3] = (1023 << 21) |   // Depth
                       (0 << 0);        // Pitch-1
    surface_state[4] = 0;
    surface_state[5] = 0;
    
    drm_intel_bo_unmap(surface_bo);
    printf("  ✓ Surface State créé\n");
    
    // 7. Construction Binding Table
    printf("\n[7/12] Construction Binding Table...\n");
    drm_intel_bo_map(binding_bo, 1);
    uint32_t *binding_table = (uint32_t *)binding_bo->virtual;
    binding_table[0] = 0x400000;  // Adresse absolue de surface_bo
    drm_intel_bo_unmap(binding_bo);
    printf("  ✓ Binding Table créé\n");
    
    // 8. Construction Interface Descriptor
    printf("\n[8/12] Construction Interface Descriptor...\n");
    drm_intel_bo_map(idrt_bo, 1);
    uint32_t *idrt = (uint32_t *)idrt_bo->virtual;
    memset(idrt, 0, 32);
    
    // Interface Descriptor (8 DWORDs = 32 bytes)
    idrt[0] = 0x200000;  // Kernel Start Pointer = kernel_bo offset
    idrt[1] = 0;
    idrt[2] = 0;
    idrt[3] = (1 << 16);  // Sampler Count = 0, Binding Table Entry Count = 1
    idrt[4] = 0x500000;  // Binding Table Pointer = binding_bo offset
    idrt[5] = (1 << 16) | (64 << 0);  // CURBE Read Length, CURBE Read Offset
    idrt[6] = 0x00000000;  // Barrier Enable, Number of Threads
    idrt[7] = 0;
    
    drm_intel_bo_unmap(idrt_bo);
    printf("  ✓ Interface Descriptor créé\n");
    
    // 9. Construction BATCH MINIMAL
    printf("\n[9/12] Construction batch minimal...\n");
    drm_intel_bo_map(batch_bo, 1);
    uint32_t *batch = (uint32_t *)batch_bo->virtual;
    int batch_offset = 0;
    
    // GPGPU_WALKER (15 DWORDs)
    batch[batch_offset++] = CMD_GPGPU_WALKER | 13;
    batch[batch_offset++] = 0x300000;  // DW1: Interface Descriptor Offset = idrt_bo
    batch[batch_offset++] = 0;  // DW2
    batch[batch_offset++] = 0;  // DW3
    batch[batch_offset++] = (0 << 30) | (63);  // DW4: SIMD8, 63 threads
    batch[batch_offset++] = 1;  // DW5: Thread Width X
    batch[batch_offset++] = 0;  // DW6
    batch[batch_offset++] = 1;  // DW7: Thread Height Y
    batch[batch_offset++] = 0;  // DW8
    batch[batch_offset++] = 1;  // DW9: Thread Depth Z
    batch[batch_offset++] = 0;  // DW10
    batch[batch_offset++] = 0;  // DW11: Thread Group ID X
    batch[batch_offset++] = 0;  // DW12: Thread Group ID Y
    batch[batch_offset++] = 0;  // DW13: Thread Group ID Z
    batch[batch_offset++] = 0;  // DW14: Right Execution Mask
    
    // MEDIA_STATE_FLUSH
    batch[batch_offset++] = CMD_MEDIA_STATE_FLUSH | 0;
    
    // PIPE_CONTROL
    batch[batch_offset++] = CMD_PIPE_CONTROL | 4;
    batch[batch_offset++] = (1 << 20) |  // CS Stall
                            (1 << 18);   // TLB Invalidate
    batch[batch_offset++] = 0;
    batch[batch_offset++] = 0;
    batch[batch_offset++] = 0;
    batch[batch_offset++] = 0;
    
    // MI_BATCH_BUFFER_END
    batch[batch_offset++] = MI_BATCH_BUFFER_END;
    
    int batch_len = batch_offset * 4;
    drm_intel_bo_unmap(batch_bo);
    printf("  ✓ Batch créé: %d bytes (%d DWORDs)\n", batch_len, batch_offset);
    
    // 10. Création contexte GPU
    printf("\n[10/12] Création contexte GPU...\n");
    drm_intel_context *ctx = drm_intel_gem_context_create(bufmgr);
    if (!ctx) {
        fprintf(stderr, "❌ drm_intel_gem_context_create failed\n");
        return 1;
    }
    printf("  ✓ Contexte créé\n");
    
    // 11. EXÉCUTION GPU avec API libdrm_intel!
    printf("\n[11/12] EXÉCUTION GPU...\n");
    int ret = drm_intel_gem_bo_context_exec(batch_bo, ctx, batch_len, I915_EXEC_RENDER);
    if (ret != 0) {
        fprintf(stderr, "❌ EXECBUFFER2 failed: %d (%s)\n", ret, strerror(errno));
        printf("\nVérifier dmesg pour GPU HANG:\n");
        system("dmesg | tail -20");
        drm_intel_gem_context_destroy(ctx);
        return 1;
    }
    printf("  ✓✓✓ EXECBUFFER2 RÉUSSI!\n");
    
    // 12. VÉRIFICATION RÉSULTATS
    printf("\n[12/12] Vérification résultats...\n");
    drm_intel_bo_map(output_bo, 0);
    output = (uint32_t *)output_bo->virtual;
    
    printf("\n=== RÉSULTATS GPU NATIF i915 ===\n");
    printf("Output[0] = 0x%08x %s\n", output[0], 
           output[0] == 0xDEADBEEF ? "✓✓✓ SUCCÈS!" : "❌ ÉCHEC");
    printf("Output[1] = 0x%08x %s\n", output[1],
           output[1] == 0xCAFEBABE ? "✓✓✓ SUCCÈS!" : "❌ ÉCHEC");
    printf("Output[2] = 0x%08x %s\n", output[2],
           output[2] == 0x12345678 ? "✓✓✓ SUCCÈS!" : "❌ ÉCHEC");
    printf("Output[3] = 0x%08x %s\n", output[3],
           output[3] == 0xABCDEF00 ? "✓✓✓ SUCCÈS!" : "❌ ÉCHEC");
    
    int success = (output[0] == 0xDEADBEEF && 
                   output[1] == 0xCAFEBABE &&
                   output[2] == 0x12345678 &&
                   output[3] == 0xABCDEF00);
    
    drm_intel_bo_unmap(output_bo);
    
    // Cleanup
    drm_intel_gem_context_destroy(ctx);
    drm_intel_bo_unreference(batch_bo);
    drm_intel_bo_unreference(output_bo);
    drm_intel_bo_unreference(kernel_bo);
    drm_intel_bo_unreference(idrt_bo);
    drm_intel_bo_unreference(surface_bo);
    drm_intel_bo_unreference(binding_bo);
    drm_intel_bo_unreference(curbe_bo);
    drm_intel_bo_unreference(scratch_bo);
    drm_intel_bo_unreference(state_bo);
    drm_intel_bo_unreference(aux_bo);
    drm_intel_bufmgr_destroy(bufmgr);
    close(fd);
    
    printf("\n=== CONCLUSION ===\n");
    if (success) {
        printf("✓✓✓ SUCCÈS TOTAL - GPU NATIF i915 DRM FONCTIONNE!\n");
        printf("✓✓✓ ÉLIMINATION OPENCL: 100%%!\n");
        return 0;
    } else {
        printf("❌ Échec - Vérifier dmesg pour diagnostics\n");
        return 1;
    }
}

// Made with Bob
