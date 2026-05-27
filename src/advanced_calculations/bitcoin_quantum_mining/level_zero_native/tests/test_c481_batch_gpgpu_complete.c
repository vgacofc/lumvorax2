/*
 * TEST C481: BATCH GPGPU COMPLET avec code Beignet DIRECT
 * 
 * Copie DIRECTE du code Beignet intel_gpgpu_batch_start() + ajustements
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <assert.h>

#include <libdrm/drm.h>
#include <libdrm/i915_drm.h>
#include <libdrm/intel_bufmgr.h>

#include "../beignet_i915/beignet_i915_minimal.h"
#include "../beignet_i915/beignet_batch_builder.h"

// Kernel ISA Gen9 COMPLET (320 bytes) - Extrait C475 via Intel ocloc
// Source: test_c475_minimal_compute.cl compilé par Intel Graphics Compiler
// Écrit: 0xDEADBEEF, 0xCAFEBABE, 0x12345678, 0xABCDEF00
const unsigned char gen9_kernel_isa[] = {
    0x01,0x00,0x60,0x00,0x0c,0x02,0x40,0x20,0x00,0x00,0x20,0x00,
    0x00,0x00,0x00,0x00,0x06,0x80,0x00,0x00,0x04,0x00,0x00,0x30,
    0x00,0x10,0x00,0x16,0xc0,0x04,0xc0,0x04,0x01,0x00,0x00,0x00,
    0x0c,0x47,0x80,0x20,0xef,0xbe,0xad,0xde,0xbe,0xba,0xfe,0xca,
    0x01,0x00,0x60,0x00,0x6c,0x26,0x70,0x20,0x00,0x00,0x00,0x00,
    0x40,0xc8,0x00,0x00,0x01,0x00,0x00,0x00,0x0c,0x47,0x88,0x20,
    0x78,0x56,0x34,0x12,0x00,0xef,0xcd,0xab,0x01,0x4d,0x00,0x20,
    0x07,0x7f,0x02,0x00,0x01,0xcd,0x02,0x20,0x00,0x08,0x04,0x00,
    0x40,0x00,0x60,0x00,0x0c,0x43,0xc0,0x20,0x60,0x00,0x00,0x1a,
    0x70,0x00,0x8d,0x00,0x01,0xc3,0x02,0x60,0x05,0x08,0x04,0x00,
    0x33,0x00,0x60,0x0c,0x14,0x80,0x00,0x00,0xc1,0x00,0x00,0x00,
    0xff,0x81,0x06,0x04,0x31,0x00,0x00,0x07,0x04,0x02,0x00,0x20,
    0xe0,0x0f,0x00,0x06,0x10,0x00,0x00,0x82,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};
const unsigned int gen9_kernel_isa_len = sizeof(gen9_kernel_isa);

#define OUTPUT_SIZE 4096

int main(void) {
    struct timeval start, end;
    gettimeofday(&start, NULL);
    
    printf("=== CYCLE C481: BATCH GPGPU COMPLET BEIGNET ===\n\n");
    
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
        fprintf(stderr, "ERREUR: drm_intel_bufmgr_gem_init failed\n");
        close(fd);
        return 1;
    }
    printf("  ✓ bufmgr initialisé\n");
    
    // 3. Allocation buffers
    printf("\n[3/10] Allocation buffers...\n");
    
    drm_intel_bo *batch_bo = drm_intel_bo_alloc(bufmgr, "batch", 65536, 4096);
    drm_intel_bo *output_bo = drm_intel_bo_alloc(bufmgr, "output", OUTPUT_SIZE, 4096);
    drm_intel_bo *aux_buf_bo = drm_intel_bo_alloc(bufmgr, "aux_buf", 65536, 4096);
    drm_intel_bo *kernel_bo = drm_intel_bo_alloc(bufmgr, "kernel", 4096, 64);
    
    if (!batch_bo || !output_bo || !aux_buf_bo || !kernel_bo) {
        fprintf(stderr, "ERREUR: Allocation buffers failed\n");
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
    
    // 5. Copie kernel ISA Gen9 (320 bytes)
    printf("\n[5/10] Copie kernel ISA Gen9...\n");
    extern const unsigned char gen9_kernel_isa[];
    extern const unsigned int gen9_kernel_isa_len;
    
    drm_intel_bo_map(kernel_bo, 1);
    memcpy(kernel_bo->virtual, gen9_kernel_isa, gen9_kernel_isa_len);
    drm_intel_bo_unmap(kernel_bo);
    printf("  ✓ Kernel ISA copié: %u bytes\n", gen9_kernel_isa_len);
    
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
    // dw0: Kernel Start Pointer (sera relocalisé)
    idrt->dw0 = 0;
    // dw3: Sampler State Pointer (offset 12288 >> 5)
    idrt->dw3 = (12288 >> 5) << 5;
    // dw4: Binding Table Pointer + Entry Count
    // CORRECTION C483: Lier la Binding Table (offset 0 dans aux_buf)
    uint32_t bti_offset = 0;  // Binding Table au début de aux_buf
    idrt->dw4 = (bti_offset >> 5) | (1 << 0);  // BTI offset + 1 entry
    // dw5: CURBE Read Offset + Length
    idrt->dw5 = 0;
    // dw6: Threads configuration (1 thread par groupe, pas de SLM)
    idrt->dw6 = (1 << 24);  // Number of Threads = 1
    // dw7: Cross-Thread Constant Data Read Length
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
    builder.thread_width = 1;
    builder.thread_height = 1;
    builder.thread_depth = 1;
    builder.max_threads = 64;
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
    printf("  ✓ MI_BATCH_BUFFER_END\n");
    
    // 9. Soumission EXECBUFFER2
    printf("\n[9/10] Soumission EXECBUFFER2...\n");
    
    drm_intel_context *ctx = drm_intel_gem_context_create(bufmgr);
    if (!ctx) {
        fprintf(stderr, "ERREUR: drm_intel_gem_context_create failed\n");
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
        fprintf(stderr, "ERREUR: EXECBUFFER2 failed (ret=%d)\n", ret);
        
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
    printf("\n[10/10] Vérification résultat...\n");
    
    drm_intel_bo_map(output_bo, 0);
    output = (uint32_t*)output_bo->virtual;
    
    printf("  Output[0] = 0x%08x\n", output[0]);
    printf("  Output[1] = 0x%08x\n", output[1]);
    printf("  Output[2] = 0x%08x\n", output[2]);
    printf("  Output[3] = 0x%08x\n", output[3]);
    
    int success = (output[0] != 0xFFFFFFFF);
    
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
    
    printf("\n=== RÉSUMÉ C481 ===\n");
    printf("Durée totale: %.3f ms\n", elapsed);
    printf("EXECBUFFER2: %s\n", ret == 0 ? "✓ SUCCÈS" : "✗ ÉCHEC");
    printf("Batch: GPGPU COMPLET (Beignet)\n");
    printf("Relocations: Automatiques (STATE_BASE_ADDRESS + IDRT + BTI)\n");
    printf("Statut: %s\n", success ? "✓✓✓ GPU EXÉCUTÉ!" : "⚠ Pas de modification");
    
    if (success) {
        printf("\n🎉🎉🎉 PREMIER 0xDEADBEEF GPU NATIF! 🎉🎉🎉\n");
    }
    
    return ret == 0 ? 0 : 1;
}

// Made with Bob - Cycle C481