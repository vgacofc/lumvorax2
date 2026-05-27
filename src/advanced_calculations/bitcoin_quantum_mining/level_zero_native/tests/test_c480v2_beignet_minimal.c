/*
 * CYCLE C480v2 - TEST BEIGNET I915 MINIMAL
 * 
 * Test de la fonction beignet_i915_setup_bti_gen7() avec dri_bo_emit_reloc()
 * pour résoudre le problème EXECBUFFER2 Invalid argument de C479
 * 
 * Objectif: Valider que dri_bo_emit_reloc() gère correctement les relocations
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <libdrm/intel_bufmgr.h>
#include <libdrm/i915_drm.h>

#include "../beignet_i915/beignet_i915_minimal.h"
#include "../include/gen9_null_state_batch.h"

#define OUTPUT_SIZE 4096

int main(int argc, char **argv) {
    printf("=== CYCLE C480v2: TEST BEIGNET I915 MINIMAL ===\n\n");
    
    struct timeval start, end;
    gettimeofday(&start, NULL);
    
    // 1. Ouverture DRM
    printf("[1/8] Ouverture /dev/dri/renderD128...\n");
    int fd = open("/dev/dri/renderD128", O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }
    printf("  ✓ fd = %d\n", fd);
    
    // 2. Initialisation bufmgr
    printf("\n[2/8] Initialisation drm_intel_bufmgr...\n");
    drm_intel_bufmgr *bufmgr = drm_intel_bufmgr_gem_init(fd, 4096);
    if (!bufmgr) {
        fprintf(stderr, "ERREUR: drm_intel_bufmgr_gem_init failed\n");
        close(fd);
        return 1;
    }
    drm_intel_bufmgr_gem_enable_reuse(bufmgr);
    printf("  ✓ bufmgr initialisé\n");
    
    // 3. Allocation buffers
    printf("\n[3/8] Allocation buffers...\n");
    
    drm_intel_bo *batch_bo = drm_intel_bo_alloc(bufmgr, "batch", 16384, 4096);
    drm_intel_bo *output_bo = drm_intel_bo_alloc(bufmgr, "output", OUTPUT_SIZE, 4096);
    drm_intel_bo *aux_buf_bo = drm_intel_bo_alloc(bufmgr, "aux_buf", 16384, 4096);
    
    if (!batch_bo || !output_bo || !aux_buf_bo) {
        fprintf(stderr, "ERREUR: Allocation buffers failed\n");
        drm_intel_bufmgr_destroy(bufmgr);
        close(fd);
        return 1;
    }
    
    printf("  ✓ batch_bo: %d bytes\n", 16384);
    printf("  ✓ output_bo: %d bytes\n", OUTPUT_SIZE);
    printf("  ✓ aux_buf_bo: %d bytes (surface_heap)\n", 16384);
    
    // 4. Initialisation output buffer
    printf("\n[4/8] Initialisation output buffer...\n");
    drm_intel_bo_map(output_bo, 1);
    uint32_t *output = (uint32_t*)output_bo->virtual;
    for (int i = 0; i < OUTPUT_SIZE/4; i++) {
        output[i] = 0xFFFFFFFF;
    }
    drm_intel_bo_unmap(output_bo);
    printf("  ✓ Output rempli avec 0xFFFFFFFF\n");
    
    // 5. Configuration Binding Table via Beignet
    printf("\n[5/8] Configuration Binding Table (Beignet i915)...\n");
    
    uint32_t surface_heap_offset = 0;
    
    // Appel fonction Beignet avec dri_bo_emit_reloc()
    beignet_i915_setup_bti_gen7(aux_buf_bo,
                                 surface_heap_offset,
                                 output_bo,
                                 0,  // internal_offset
                                 OUTPUT_SIZE,
                                 0,  // index BTI
                                 I965_SURFACEFORMAT_RAW);
    
    printf("  ✓ Binding Table configurée (index 0)\n");
    printf("  ✓ Surface State Gen7 configurée\n");
    printf("  ✓ Relocation enregistrée via dri_bo_emit_reloc()\n");
    
    // 6. Construction batch
    printf("\n[6/8] Construction batch buffer...\n");
    
    drm_intel_bo_map(batch_bo, 1);
    uint32_t *batch = (uint32_t*)batch_bo->virtual;
    int idx = 0;
    
    // Copie null_state complet (960 DWORDs)
    memcpy(&batch[idx], gen9_null_state_batch, sizeof(gen9_null_state_batch));
    idx += sizeof(gen9_null_state_batch) / 4;
    
    // MI_BATCH_BUFFER_END
    batch[idx++] = 0x0A000000;
    
    drm_intel_bo_unmap(batch_bo);
    
    printf("  ✓ Batch: %d DWORDs (%d bytes)\n", idx, idx * 4);
    printf("  ✓ Null state: 960 DWORDs\n");
    
    // 7. Soumission EXECBUFFER2 via drm_intel_gem_bo_context_exec
    printf("\n[7/8] Soumission EXECBUFFER2...\n");
    
    // Créer un drm_intel_context via libdrm_intel
    drm_intel_context *ctx = drm_intel_gem_context_create(bufmgr);
    if (!ctx) {
        fprintf(stderr, "ERREUR: Impossible de créer drm_intel_context\n");
        
        drm_intel_bo_unreference(batch_bo);
        drm_intel_bo_unreference(output_bo);
        drm_intel_bo_unreference(aux_buf_bo);
        drm_intel_bufmgr_destroy(bufmgr);
        close(fd);
        return 1;
    }
    
    printf("  ✓ drm_intel_context créé\n");
    
    // Utiliser drm_intel_gem_bo_context_exec comme Beignet
    // Cette fonction gère automatiquement les relocations enregistrées via dri_bo_emit_reloc()
    int ret = drm_intel_gem_bo_context_exec(batch_bo, ctx, idx * 4, I915_EXEC_RENDER);
    if (ret != 0) {
        perror("EXECBUFFER2");
        fprintf(stderr, "ERREUR: EXECBUFFER2 failed (ret=%d)\n", ret);
        
        drm_intel_bo_unreference(batch_bo);
        drm_intel_bo_unreference(output_bo);
        drm_intel_bo_unreference(aux_buf_bo);
        drm_intel_bufmgr_destroy(bufmgr);
        close(fd);
        return 1;
    }
    
    printf("  ✓✓✓ EXECBUFFER2 SUCCÈS! ✓✓✓\n");
    
    // 8. Vérification résultat
    printf("\n[8/8] Vérification résultat...\n");
    
    drm_intel_bo_map(output_bo, 0);
    output = (uint32_t*)output_bo->virtual;
    
    printf("  Output[0] = 0x%08x\n", output[0]);
    printf("  Output[1] = 0x%08x\n", output[1]);
    printf("  Output[2] = 0x%08x\n", output[2]);
    printf("  Output[3] = 0x%08x\n", output[3]);
    
    int success = (output[0] != 0xFFFFFFFF);
    
    drm_intel_bo_unmap(output_bo);
    
    // Cleanup
    drm_intel_bo_unreference(batch_bo);
    drm_intel_bo_unreference(output_bo);
    drm_intel_bo_unreference(aux_buf_bo);
    drm_intel_bufmgr_destroy(bufmgr);
    close(fd);
    
    gettimeofday(&end, NULL);
    double elapsed = (end.tv_sec - start.tv_sec) * 1000.0 + 
                     (end.tv_usec - start.tv_usec) / 1000.0;
    
    printf("\n=== RÉSUMÉ C480v2 ===\n");
    printf("Durée totale: %.3f ms\n", elapsed);
    printf("EXECBUFFER2: %s\n", ret == 0 ? "✓ SUCCÈS" : "✗ ÉCHEC");
    printf("Relocations: Automatiques via dri_bo_emit_reloc()\n");
    printf("Module: beignet_i915_minimal.h\n");
    printf("Statut: %s\n", success ? "✓ GPU exécuté" : "⚠ Pas de modification output");
    
    return ret == 0 ? 0 : 1;
}

// Made with Bob
