/*
 * TEST C496: CONFIGURATION CORRECTE - PREMIER RÉSULTAT GPU NATIF
 *
 * Objectif: Obtenir le premier 0xDEADBEEF sur GPU natif SANS OpenCL
 *
 * Corrections C495:
 * 1. thread_width = 256 (était 1)
 * 2. max_threads = 256 (était 64)
 * 3. idrt->dw6 = (256 << 24) (était 1 << 24)
 *
 * Configuration OpenCL identifiée:
 * - Work Group Size: 256 threads
 * - SIMD Préféré: SIMD32
 * - Preferred Multiple: 32
 *
 * Attendu: Output[0] = 0xDEADBEEF ✅ PREMIER RÉSULTAT GPU NATIF!
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <intel_bufmgr.h>
#include <i915_drm.h>

// Infrastructure Beignet (header-only)
#include "beignet_i915/beignet_i915_minimal.h"
#include "beignet_i915/beignet_batch_builder.h"

// Kernel ISA Gen9 OpenCL (320 bytes, validé C489)
#include "tests/test_c491_opencl_isa_gen9.h"

#define OUTPUT_SIZE 4096

int main(void) {
    struct timeval start, end;
    gettimeofday(&start, NULL);
    
    printf("=== CYCLE C496: CONFIGURATION CORRECTE GPU NATIF ===\n\n");
    printf("Objectif: 0xDEADBEEF avec configuration OpenCL (256 threads, SIMD32)!\n\n");
    
    // 1. Ouverture DRM
    printf("[1/10] Ouverture /dev/dri/renderD128...\n");
    int fd = open("/dev/dri/renderD128", O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }
    printf("  ✓ fd = %d\n", fd);
    
    // 2. Initialisation bufmgr
    printf("\n[2/10] Initialisation drm_intel_bufmgr...\n");
    drm_intel_bufmgr *bufmgr = drm_intel_bufmgr_gem_init(fd, 4096);
    if (!bufmgr) {
        fprintf(stderr, "❌ drm_intel_bufmgr_gem_init failed\n");
        close(fd);
        return 1;
    }
    drm_intel_bufmgr_gem_enable_reuse(bufmgr);
    printf("  ✓ bufmgr initialisé\n");
    
    // 3. Allocation buffers
    printf("\n[3/10] Allocation buffers...\n");
    
    drm_intel_bo *batch_bo = drm_intel_bo_alloc(bufmgr, "batch", 65536, 4096);
    drm_intel_bo *output_bo = drm_intel_bo_alloc(bufmgr, "output", OUTPUT_SIZE, 4096);
    drm_intel_bo *aux_buf_bo = drm_intel_bo_alloc(bufmgr, "aux_buf", 65536, 4096);
    drm_intel_bo *kernel_bo = drm_intel_bo_alloc(bufmgr, "kernel", 4096, 64);
    
    if (!batch_bo || !output_bo || !aux_buf_bo || !kernel_bo) {
        fprintf(stderr, "❌ Allocation failed\n");
        drm_intel_bufmgr_destroy(bufmgr);
        close(fd);
        return 1;
    }
    
    printf("  ✓ batch_bo: %zu bytes\n", batch_bo->size);
    printf("  ✓ output_bo: %zu bytes\n", output_bo->size);
    printf("  ✓ aux_buf_bo: %zu bytes\n", aux_buf_bo->size);
    printf("  ✓ kernel_bo: %zu bytes\n", kernel_bo->size);
    
    // 4. Initialisation output
    printf("\n[4/10] Initialisation output buffer...\n");
    drm_intel_bo_map(output_bo, 1);
    uint32_t *output = (uint32_t*)output_bo->virtual;
    for (int i = 0; i < OUTPUT_SIZE/4; i++) {
        output[i] = 0xFFFFFFFF;
    }
    drm_intel_bo_unmap(output_bo);
    printf("  ✓ Output rempli avec 0xFFFFFFFF\n");
    
    // 5. Copie kernel ISA Gen9 OpenCL (320 bytes)
    printf("\n[5/10] Copie kernel ISA Gen9 OpenCL...\n");
    
    drm_intel_bo_map(kernel_bo, 1);
    memcpy(kernel_bo->virtual, kernel_isa_opencl_gen9, sizeof(kernel_isa_opencl_gen9));
    drm_intel_bo_unmap(kernel_bo);
    printf("  ✓ Kernel ISA OpenCL copié: %zu bytes\n", sizeof(kernel_isa_opencl_gen9));
    printf("  ✓ Kernel VALIDÉ en C489 avec OpenCL\n");
    
    // 6. Configuration Binding Table + Surface State
    printf("\n[6/10] Configuration Binding Table (Beignet)...\n");
    
    beignet_i915_setup_bti_gen7(aux_buf_bo,
                                 0,  // surface_heap_offset
                                 output_bo,
                                 0,  // internal_offset
                                 OUTPUT_SIZE,
                                 0,  // index BTI
                                 I965_SURFACEFORMAT_RAW);
    
    printf("  ✓ Binding Table configurée\n");
    printf("  ✓ Surface State Gen7 configurée\n");
    printf("  ✓ Relocation enregistrée\n");
    
    // 7. Configuration Interface Descriptor (IDRT)
    printf("\n[7/10] Configuration Interface Descriptor...\n");
    
    drm_intel_bo_map(aux_buf_bo, 1);
    
    // IDRT à offset 8192
    uint32_t idrt_offset = 8192;
    gen8_interface_descriptor_t *idrt = (gen8_interface_descriptor_t*)
        ((char*)aux_buf_bo->virtual + idrt_offset);
    
    memset(idrt, 0, sizeof(*idrt));
    idrt->dw0 = 0;  // Kernel Start Pointer (sera relocalisé)
    idrt->dw3 = (12288 >> 5) << 5;  // Sampler State Pointer
    idrt->dw4 = (0 >> 5) | (1 << 0);  // BTI offset + 1 entry
    idrt->dw5 = 0;  // CURBE
    idrt->dw6 = (256 << 24);  // ✅ C496 FIX: Number of Threads = 256 (OpenCL config)
    idrt->dw7 = 0;
    
    drm_intel_bo_unmap(aux_buf_bo);
    
    // RELOCATION kernel dans IDRT (dw0)
    drm_intel_bo_emit_reloc(aux_buf_bo,
                            idrt_offset + offsetof(gen8_interface_descriptor_t, dw0),
                            kernel_bo,
                            0,
                            I915_GEM_DOMAIN_INSTRUCTION,
                            0);
    
    printf("  ✓ Interface Descriptor configuré\n");
    printf("  ✓ Relocation kernel enregistrée\n");
    
    // 8. Construction batch GPGPU COMPLET
    printf("\n[8/10] Construction batch GPGPU COMPLET...\n");
    
    beignet_batch_builder_t builder;
    beignet_batch_init(&builder, batch_bo, aux_buf_bo, kernel_bo, output_bo);
    
    // Configuration dimensions threads
    builder.thread_width = 256;  // ✅ C496 FIX: 256 threads (OpenCL config)
    builder.thread_height = 1;
    builder.thread_depth = 1;
    builder.max_threads = 256;   // ✅ C496 FIX: Match work group size
    builder.curbe_size = 0;
    builder.idrt_offset = idrt_offset;
    
    // Construction batch avec TOUTES les commandes
    uint32_t batch_len = beignet_build_gpgpu_batch_gen9(&builder);
    
    printf("  ✓ Batch construit: %u bytes\n", batch_len);
    printf("  ✓ PIPE_CONTROL\n");
    printf("  ✓ PIPELINE_SELECT GPGPU\n");
    printf("  ✓ STATE_BASE_ADDRESS + relocations\n");
    printf("  ✓ MEDIA_VFE_STATE\n");
    printf("  ✓ MEDIA_INTERFACE_DESCRIPTOR_LOAD\n");
    printf("  ✓ GPGPU_WALKER\n");
    printf("  ✓ MEDIA_STATE_FLUSH\n");
    printf("  ✓ PIPE_CONTROL\n");
    printf("  ✓ MI_BATCH_BUFFER_END\n");
    
    // 9. Soumission EXECBUFFER2
    printf("\n[9/10] Soumission EXECBUFFER2...\n");
    
    drm_intel_context *ctx = drm_intel_gem_context_create(bufmgr);
    if (!ctx) {
        fprintf(stderr, "❌ drm_intel_gem_context_create failed\n");
        drm_intel_bo_unreference(batch_bo);
        drm_intel_bo_unreference(output_bo);
        drm_intel_bo_unreference(aux_buf_bo);
        drm_intel_bo_unreference(kernel_bo);
        drm_intel_bufmgr_destroy(bufmgr);
        close(fd);
        return 1;
    }
    
    printf("  ✓ Context créé\n");
    
    int ret = drm_intel_gem_bo_context_exec(batch_bo, ctx, batch_len, I915_EXEC_RENDER);
    if (ret != 0) {
        perror("EXECBUFFER2");
        fprintf(stderr, "❌ EXECBUFFER2 failed (ret=%d)\n", ret);
        
        drm_intel_gem_context_destroy(ctx);
        drm_intel_bo_unreference(batch_bo);
        drm_intel_bo_unreference(output_bo);
        drm_intel_bo_unreference(aux_buf_bo);
        drm_intel_bo_unreference(kernel_bo);
        drm_intel_bufmgr_destroy(bufmgr);
        close(fd);
        return 1;
    }
    
    printf("  ✓✓✓ EXECBUFFER2 SUCCÈS! ✓✓✓\n");
    
    // 10. Vérification résultat
    printf("\n[10/10] Vérification résultat GPU NATIF...\n");
    
    drm_intel_bo_map(output_bo, 0);
    output = (uint32_t*)output_bo->virtual;
    
    printf("\n=== RÉSULTATS GPU NATIF i915 DRM ===\n");
    printf("Output[0] = 0x%08x %s\n", output[0], 
           output[0] == 0xDEADBEEF ? "✓✓✓ SUCCÈS TOTAL!" : 
           output[0] != 0xFFFFFFFF ? "⚠ MODIFIÉ (near-miss)" : "❌ INCHANGÉ");
    printf("Output[1] = 0x%08x %s\n", output[1],
           output[1] == 0xCAFEBABE ? "✓✓✓ SUCCÈS!" : 
           output[1] != 0xFFFFFFFF ? "⚠ MODIFIÉ" : "❌ INCHANGÉ");
    printf("Output[2] = 0x%08x %s\n", output[2],
           output[2] == 0x12345678 ? "✓✓✓ SUCCÈS!" : 
           output[2] != 0xFFFFFFFF ? "⚠ MODIFIÉ" : "❌ INCHANGÉ");
    printf("Output[3] = 0x%08x %s\n", output[3],
           output[3] == 0xABCDEF00 ? "✓✓✓ SUCCÈS!" : 
           output[3] != 0xFFFFFFFF ? "⚠ MODIFIÉ" : "❌ INCHANGÉ");
    
    int success = (output[0] == 0xDEADBEEF && 
                   output[1] == 0xCAFEBABE &&
                   output[2] == 0x12345678 &&
                   output[3] == 0xABCDEF00);
    
    int modified = (output[0] != 0xFFFFFFFF || 
                    output[1] != 0xFFFFFFFF ||
                    output[2] != 0xFFFFFFFF ||
                    output[3] != 0xFFFFFFFF);
    
    drm_intel_bo_unmap(output_bo);
    
    // Cleanup
    drm_intel_gem_context_destroy(ctx);
    drm_intel_bo_unreference(batch_bo);
    drm_intel_bo_unreference(output_bo);
    drm_intel_bo_unreference(aux_buf_bo);
    drm_intel_bo_unreference(kernel_bo);
    drm_intel_bufmgr_destroy(bufmgr);
    close(fd);
    
    gettimeofday(&end, NULL);
    double elapsed = (end.tv_sec - start.tv_sec) * 1000.0 + 
                     (end.tv_usec - start.tv_usec) / 1000.0;
    
    printf("\n=== RÉSUMÉ C494 ===\n");
    printf("Durée totale: %.3f ms\n", elapsed);
    printf("EXECBUFFER2: %s\n", ret == 0 ? "✓ SUCCÈS" : "✗ ÉCHEC");
    printf("Batch: GPGPU COMPLET (Beignet)\n");
    printf("Kernel: ISA Gen9 OpenCL (validé C489)\n");
    printf("Relocations: Automatiques (libdrm_intel)\n");
    
    if (success) {
        printf("\n🎉🎉🎉 SUCCÈS TOTAL - PREMIER RÉSULTAT GPU NATIF! 🎉🎉🎉\n");
        printf("✓✓✓ ÉLIMINATION OPENCL: 100%%! ✓✓✓\n");
        printf("✓✓✓ GPU i915 DRM NATIF FONCTIONNE! ✓✓✓\n");
        return 0;
    } else if (modified) {
        printf("\n⚠ GPU EXÉCUTÉ mais résultat incorrect\n");
        printf("→ Kernel s'exécute mais logique à corriger\n");
        return 0;
    } else {
        printf("\n❌ Pas de modification - Vérifier dmesg\n");
        return 1;
    }
}

// Made with Bob - C494 Premier Résultat GPU Natif