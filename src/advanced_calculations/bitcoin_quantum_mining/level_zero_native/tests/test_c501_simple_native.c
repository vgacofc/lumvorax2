/*
 * CYCLE C501: TEST KERNEL ULTRA-SIMPLE SUR i915 NATIF
 * 
 * Objectif: Tester le kernel le plus simple possible (juste un store)
 * pour identifier la configuration minimale requise
 * 
 * Stratégie:
 * 1. Kernel ultra-simple: output[0] = 0xDEADBEEF
 * 2. Configuration minimale: 1 thread, SIMD8
 * 3. Logs forensiques détaillés de chaque étape
 * 4. Comparaison avec OpenCL
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <time.h>
#include <drm/i915_drm.h>
#include <intel_bufmgr.h>

// Batch builder Beignet
#include "../beignet_i915/beignet_batch_builder.h"

// Kernel ISA simple (compilé avec ocloc)
static const unsigned char kernel_isa_simple[] = {
    #include "test_c501_simple_kernel.isa"
};

int main(void)
{
    printf("\n=== CYCLE C501: KERNEL ULTRA-SIMPLE i915 NATIF ===\n\n");
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    // [1/10] Ouverture DRM
    printf("[1/10] Ouverture /dev/dri/renderD128...\n");
    int fd = open("/dev/dri/renderD128", O_RDWR);
    if (fd < 0) {
        perror("Erreur open");
        return 1;
    }
    printf("  ✓ fd = %d\n", fd);
    
    // [2/10] Init bufmgr
    printf("\n[2/10] Initialisation drm_intel_bufmgr...\n");
    drm_intel_bufmgr *bufmgr = drm_intel_bufmgr_gem_init(fd, 4096);
    if (!bufmgr) {
        fprintf(stderr, "Erreur drm_intel_bufmgr_gem_init\n");
        close(fd);
        return 1;
    }
    printf("  ✓ bufmgr initialisé\n");
    
    // [3/10] Allocation buffers
    printf("\n[3/10] Allocation buffers...\n");
    drm_intel_bo *batch_bo = drm_intel_bo_alloc(bufmgr, "batch", 65536, 4096);
    drm_intel_bo *output_bo = drm_intel_bo_alloc(bufmgr, "output", 4096, 4096);
    drm_intel_bo *aux_buf_bo = drm_intel_bo_alloc(bufmgr, "aux", 65536, 4096);
    drm_intel_bo *kernel_bo = drm_intel_bo_alloc(bufmgr, "kernel", 4096, 4096);
    
    if (!batch_bo || !output_bo || !aux_buf_bo || !kernel_bo) {
        fprintf(stderr, "Erreur allocation buffers\n");
        return 1;
    }
    printf("  ✓ batch_bo: %u bytes\n", batch_bo->size);
    printf("  ✓ output_bo: %u bytes\n", output_bo->size);
    printf("  ✓ aux_buf_bo: %u bytes\n", aux_buf_bo->size);
    printf("  ✓ kernel_bo: %u bytes\n", kernel_bo->size);
    
    // [4/10] Init output
    printf("\n[4/10] Initialisation output buffer...\n");
    drm_intel_bo_map(output_bo, 1);
    uint32_t *output = (uint32_t*)output_bo->virtual;
    for (int i = 0; i < 256; i++) {
        output[i] = 0xFFFFFFFF;
    }
    drm_intel_bo_unmap(output_bo);
    printf("  ✓ Output rempli avec 0xFFFFFFFF\n");
    
    // [5/10] Copie kernel ISA
    printf("\n[5/10] Copie kernel ISA simple...\n");
    drm_intel_bo_map(kernel_bo, 1);
    memcpy(kernel_bo->virtual, kernel_isa_simple, sizeof(kernel_isa_simple));
    drm_intel_bo_unmap(kernel_bo);
    printf("  ✓ Kernel ISA copié: %zu bytes\n", sizeof(kernel_isa_simple));
    
    // [6/10] Configuration Binding Table
    printf("\n[6/10] Configuration Binding Table...\n");
    drm_intel_bo_map(aux_buf_bo, 1);
    uint32_t *aux_buf = (uint32_t*)aux_buf_bo->virtual;
    
    // Binding Table à offset 0
    aux_buf[0] = 0x40;  // Offset vers Surface State
    
    // Surface State à offset 0x40 (Gen7 format)
    uint32_t *surface_state = &aux_buf[0x40/4];
    surface_state[0] = (0 << 29) |      // Surface Type: BUFFER
                       (5 << 18) |      // Surface Format: RAW
                       (1 << 10);       // Surface Array
    surface_state[1] = 0;               // Base Address (relocation)
    surface_state[2] = (4095 << 0);     // Width: 4096-1
    surface_state[3] = (0 << 21) |      // Depth
                       (0 << 10) |      // Pitch
                       (0 << 0);        // Height
    surface_state[4] = 0;
    surface_state[5] = 0;
    surface_state[6] = 0;
    surface_state[7] = 0;
    
    drm_intel_bo_emit_reloc(aux_buf_bo, 0x40 + 4, output_bo, 0,
                           I915_GEM_DOMAIN_RENDER, I915_GEM_DOMAIN_RENDER);
    drm_intel_bo_unmap(aux_buf_bo);
    printf("  ✓ Binding Table configurée\n");
    printf("  ✓ Surface State Gen7 configurée\n");
    printf("  ✓ Relocation enregistrée\n");
    
    // [7/10] Configuration Interface Descriptor
    printf("\n[7/10] Configuration Interface Descriptor...\n");
    drm_intel_bo_map(aux_buf_bo, 1);
    aux_buf = (uint32_t*)aux_buf_bo->virtual;
    
    // IDRT à offset 0x800
    uint32_t *idrt = &aux_buf[0x800/4];
    idrt[0] = 0;  // Kernel Start Pointer (relocation)
    idrt[1] = 0;
    idrt[2] = 0;
    idrt[3] = (1 << 16);  // Barrier Enable
    idrt[4] = 0;          // Binding Table Pointer: 0
    idrt[5] = 0;          // CURBE Read Length: 0
    idrt[6] = (1 << 24);  // Number of Threads: 1
    idrt[7] = 0;
    
    drm_intel_bo_emit_reloc(aux_buf_bo, 0x800, kernel_bo, 0,
                           I915_GEM_DOMAIN_INSTRUCTION, 0);
    drm_intel_bo_unmap(aux_buf_bo);
    printf("  ✓ Interface Descriptor configuré\n");
    printf("  ✓ Relocation kernel enregistrée\n");
    
    // [8/10] Construction batch GPGPU
    printf("\n[8/10] Construction batch GPGPU COMPLET...\n");
    
    beignet_batch_builder_t builder = {
        .batch_bo = batch_bo,
        .aux_buf_bo = aux_buf_bo,
        .kernel_bo = kernel_bo,
        .idrt_offset = 0x800,
        .binding_table_offset = 0,
        .thread_width = 1,      // 1 thread
        .thread_height = 1,
        .thread_depth = 1,
        .max_threads = 64,      // SIMD8
        .simd_size = 8
    };
    
    uint32_t batch_len = beignet_build_gpgpu_batch_gen9(&builder);
    printf("  ✓ Batch construit: %u bytes\n", batch_len);
    printf("  ✓ Configuration: 1 thread, SIMD8, 64 max threads\n");
    
    // [9/10] Soumission EXECBUFFER2
    printf("\n[9/10] Soumission EXECBUFFER2...\n");
    
    drm_intel_context *ctx = drm_intel_gem_context_create(bufmgr);
    if (!ctx) {
        fprintf(stderr, "Erreur création contexte\n");
        return 1;
    }
    printf("  ✓ Context créé\n");
    
    int ret = drm_intel_gem_bo_context_exec(batch_bo, batch_len, ctx, 0, 0);
    if (ret != 0) {
        fprintf(stderr, "  ❌ EXECBUFFER2 ÉCHEC: %d\n", ret);
        printf("\n⚠ Vérifier dmesg pour GPU HANG\n");
        return 1;
    }
    printf("  ✓✓✓ EXECBUFFER2 SUCCÈS! ✓✓✓\n");
    
    // [10/10] Vérification résultat
    printf("\n[10/10] Vérification résultat GPU NATIF...\n");
    
    drm_intel_bo_map(output_bo, 0);
    output = (uint32_t*)output_bo->virtual;
    
    printf("\n=== RÉSULTATS GPU NATIF i915 DRM ===\n");
    printf("Output[0] = 0x%08x ", output[0]);
    if (output[0] == 0xDEADBEEF) {
        printf("✅ SUCCÈS!\n");
    } else {
        printf("❌ ÉCHEC (attendu: 0xDEADBEEF)\n");
    }
    
    printf("Output[1] = 0x%08x\n", output[1]);
    printf("Output[2] = 0x%08x\n", output[2]);
    printf("Output[3] = 0x%08x\n", output[3]);
    
    drm_intel_bo_unmap(output_bo);
    
    // Calcul durée
    clock_gettime(CLOCK_MONOTONIC, &end);
    double duration = (end.tv_sec - start.tv_sec) * 1000.0 +
                     (end.tv_nsec - start.tv_nsec) / 1000000.0;
    
    printf("\n=== RÉSUMÉ C501 ===\n");
    printf("Durée totale: %.3f ms\n", duration);
    printf("EXECBUFFER2: %s\n", ret == 0 ? "✓ SUCCÈS" : "❌ ÉCHEC");
    printf("Batch: GPGPU COMPLET (Beignet)\n");
    printf("Kernel: ISA simple (1 store)\n");
    printf("Configuration: 1 thread, SIMD8\n");
    
    if (output[0] != 0xDEADBEEF) {
        printf("\n❌ Pas de modification - Vérifier dmesg\n");
    }
    
    // Cleanup
    drm_intel_gem_context_destroy(ctx);
    drm_intel_bo_unreference(batch_bo);
    drm_intel_bo_unreference(output_bo);
    drm_intel_bo_unreference(aux_buf_bo);
    drm_intel_bo_unreference(kernel_bo);
    drm_intel_bufmgr_destroy(bufmgr);
    close(fd);
    
    return (output[0] == 0xDEADBEEF) ? 0 : 1;
}

// Made with Bob
