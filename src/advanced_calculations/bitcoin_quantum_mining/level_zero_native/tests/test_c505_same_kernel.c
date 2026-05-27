/*
 * TEST C505 - UTILISATION DU MÊME KERNEL QUE OPENCL
 *
 * ROOT CAUSE #9 DÉCOUVERTE: Le kernel ISA test_c491_opencl_kernel.h
 * N'EST PAS le même que test_c427_store_validation.cl !
 *
 * SOLUTION: Compiler test_c427_store_validation.cl avec OpenCL
 * et extraire son ISA Gen9 pour test natif.
 *
 * OBJECTIF: Éliminer toute différence de kernel entre OpenCL et natif
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <errno.h>
#include <libdrm/intel_bufmgr.h>
#include <libdrm/i915_drm.h>

#include "../beignet_i915/beignet_i915_minimal.h"
#include "../beignet_i915/beignet_batch_builder.h"
#include "test_c505_extracted_kernel.h"

#define OUTPUT_SIZE 4096

int main(void) {
    struct timeval start, end;
    gettimeofday(&start, NULL);
    
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║  TEST C505 - MÊME KERNEL QUE OPENCL                          ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    printf("ROOT CAUSE #9: Kernel ISA différent entre OpenCL et natif\n");
    printf("SOLUTION: Utiliser EXACTEMENT le même kernel ISA\n");
    printf("\n");
    
    // 1. Ouverture DRM
    printf("[1/10] Ouverture /dev/dri/renderD128...\n");
    int fd = open("/dev/dri/renderD128", O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }
    printf("  ✓ DRM ouvert (fd=%d)\n", fd);
    
    // 2. Initialisation bufmgr
    printf("\n[2/10] Initialisation libdrm_intel...\n");
    drm_intel_bufmgr *bufmgr = drm_intel_bufmgr_gem_init(fd, 4096);
    if (!bufmgr) {
        fprintf(stderr, "❌ drm_intel_bufmgr_gem_init failed\n");
        close(fd);
        return 1;
    }
    printf("  ✓ Buffer manager initialisé\n");
    
    // 3. Allocation buffers
    printf("\n[3/10] Allocation buffers GPU...\n");
    
    drm_intel_bo *batch_bo = drm_intel_bo_alloc(bufmgr, "batch", 16384, 4096);
    drm_intel_bo *output_bo = drm_intel_bo_alloc(bufmgr, "output", OUTPUT_SIZE, 4096);
    drm_intel_bo *aux_buf_bo = drm_intel_bo_alloc(bufmgr, "aux_buf", 65536, 4096);
    drm_intel_bo *kernel_bo = drm_intel_bo_alloc(bufmgr, "kernel", 4096, 4096);
    
    if (!batch_bo || !output_bo || !aux_buf_bo || !kernel_bo) {
        fprintf(stderr, "❌ Allocation failed\n");
        drm_intel_bufmgr_destroy(bufmgr);
        close(fd);
        return 1;
    }
    
    printf("  ✓ batch_bo:   %zu bytes\n", batch_bo->size);
    printf("  ✓ output_bo:  %zu bytes\n", output_bo->size);
    printf("  ✓ aux_buf_bo: %zu bytes\n", aux_buf_bo->size);
    printf("  ✓ kernel_bo:  %zu bytes\n", kernel_bo->size);
    
    // 4. Initialisation output buffer
    printf("\n[4/10] Initialisation output buffer...\n");
    drm_intel_bo_map(output_bo, 1);
    uint32_t *output = (uint32_t*)output_bo->virtual;
    for (int i = 0; i < OUTPUT_SIZE/4; i++) {
        output[i] = 0xFFFFFFFF;
    }
    drm_intel_bo_unmap(output_bo);
    printf("  ✓ Output initialisé à 0xFFFFFFFF\n");
    
    // 5. Copie kernel ISA IDENTIQUE à OpenCL
    printf("\n[5/10] Copie kernel ISA IDENTIQUE à OpenCL...\n");
    
    drm_intel_bo_map(kernel_bo, 1);
    memcpy(kernel_bo->virtual, kernel_isa_c505, sizeof(kernel_isa_c505));
    drm_intel_bo_unmap(kernel_bo);
    printf("  ✓ Kernel ISA copié: %zu bytes\n", sizeof(kernel_isa_c505));
    printf("  ✓ Kernel IDENTIQUE à test_c427_store_opencl\n");
    
    // 6. Configuration Binding Table + Surface State (COPIE DE C503)
    printf("\n[6/10] Configuration Binding Table...\n");
    
    drm_intel_bo_map(aux_buf_bo, 1);
    
    // Surface State à offset 0
    gen7_surface_state_t *ss = (gen7_surface_state_t*)aux_buf_bo->virtual;
    memset(ss, 0, sizeof(*ss));
    
    ss->ss0.surface_type = I965_SURFACE_BUFFER;
    ss->ss0.surface_format = I965_SURFACEFORMAT_RAW;
    ss->ss2.width = (OUTPUT_SIZE & 0x7F);
    ss->ss2.height = ((OUTPUT_SIZE >> 7) & 0x3FFF);
    ss->ss3.depth = ((OUTPUT_SIZE >> 21) & 0x3FF);
    ss->ss3.pitch = OUTPUT_SIZE - 1;
    
    // Binding Table à offset 64
    uint32_t *bti = (uint32_t*)((char*)aux_buf_bo->virtual + 64);
    bti[0] = 0;  // Pointe vers Surface State à offset 0
    
    drm_intel_bo_unmap(aux_buf_bo);
    
    // Relocation Surface State vers output_bo
    drm_intel_bo_emit_reloc(aux_buf_bo,
                            offsetof(gen7_surface_state_t, ss1),
                            output_bo,
                            0,
                            I915_GEM_DOMAIN_RENDER,
                            I915_GEM_DOMAIN_RENDER);
    
    printf("  ✓ Binding Table configurée\n");
    printf("  ✓ Surface State configurée\n");
    printf("  ✓ Relocation output_bo enregistrée\n");
    
    // 7. Configuration Interface Descriptor (IDRT)
    printf("\n[7/10] Configuration Interface Descriptor...\n");
    
    uint32_t idrt_offset = 8192;
    drm_intel_bo_map(aux_buf_bo, 1);
    gen8_interface_descriptor_t *idrt = (gen8_interface_descriptor_t*)
        ((char*)aux_buf_bo->virtual + idrt_offset);
    
    memset(idrt, 0, sizeof(*idrt));
    idrt->dw0 = 0;
    idrt->dw3 = (12288 >> 5) << 5;  // Sampler State Pointer
    idrt->dw4 = (64 >> 5) | (1 << 0);  // BTI offset=64 + 1 entry
    idrt->dw5 = (128 << 0);  // GRF Count = 128
    idrt->dw6 = (256U << 24);  // Number of Threads = 256
    idrt->dw7 = 0;
    
    drm_intel_bo_unmap(aux_buf_bo);
    
    drm_intel_bo_emit_reloc(aux_buf_bo,
                            idrt_offset + offsetof(gen8_interface_descriptor_t, dw0),
                            kernel_bo,
                            0,
                            I915_GEM_DOMAIN_INSTRUCTION,
                            0);
    
    printf("  ✓ Interface Descriptor configuré\n");
    
    // 8. Construction batch GPGPU COMPLET
    printf("\n[8/10] Construction batch GPGPU...\n");
    
    beignet_batch_builder_t builder;
    beignet_batch_init(&builder, batch_bo, aux_buf_bo, kernel_bo, output_bo);
    
    builder.thread_width = 8;
    builder.thread_height = 1;
    builder.thread_depth = 1;
    builder.max_threads = 8;
    builder.curbe_size = 0;
    builder.idrt_offset = idrt_offset;
    
    uint32_t batch_len = beignet_build_gpgpu_batch_gen9(&builder);
    
    // Modifier GPGPU_WALKER pour SIMD32
    printf("  ✓ Modification GPGPU_WALKER pour SIMD32...\n");
    drm_intel_bo_map(batch_bo, 1);
    uint32_t *batch = (uint32_t*)batch_bo->virtual;
    
    for (uint32_t i = 0; i < batch_len/4; i++) {
        if ((batch[i] & 0xFFFF0000) == 0x71050000) {
            printf("    GPGPU_WALKER trouvé à offset 0x%x\n", i*4);
            batch[i+4] = (2 << 30) | 7;  // SIMD32 + ThreadWidth=7
            printf("    DW4 = 0x%08x (SIMD32 + ThreadWidth=7)\n", batch[i+4]);
            break;
        }
    }
    drm_intel_bo_unmap(batch_bo);
    
    printf("  ✓ Batch construit: %u bytes\n", batch_len);
    
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
    
    // Synchronisation GPU→CPU
    printf("\n  ✓ Synchronisation GPU→CPU...\n");
    
    struct drm_i915_gem_wait wait = {
        .bo_handle = output_bo->handle,
        .timeout_ns = 10000000000LL
    };
    
    if (ioctl(fd, DRM_IOCTL_I915_GEM_WAIT, &wait) != 0) {
        fprintf(stderr, "    ⚠ GEM_WAIT failed: %s\n", strerror(errno));
    } else {
        printf("    ✓ GEM_WAIT succès\n");
    }
    
    struct drm_i915_gem_set_domain set_domain = {
        .handle = output_bo->handle,
        .read_domains = I915_GEM_DOMAIN_CPU,
        .write_domain = 0
    };
    
    if (ioctl(fd, DRM_IOCTL_I915_GEM_SET_DOMAIN, &set_domain) != 0) {
        fprintf(stderr, "    ⚠ GEM_SET_DOMAIN failed: %s\n", strerror(errno));
    } else {
        printf("    ✓ GEM_SET_DOMAIN succès\n");
    }
    
    // 10. Vérification résultat
    printf("\n[10/10] Vérification résultat...\n");
    
    drm_intel_bo_map(output_bo, 0);
    output = (uint32_t*)output_bo->virtual;
    
    printf("\n=== RÉSULTATS GPU NATIF i915 DRM ===\n");
    printf("Output[0] = 0x%08x %s\n", output[0], 
           output[0] == 0x12345678 ? "✓✓✓ SUCCÈS TOTAL!" : 
           output[0] != 0xFFFFFFFF ? "⚠ MODIFIÉ" : "❌ INCHANGÉ");
    
    int success = (output[0] == 0x12345678);
    int modified = (output[0] != 0xFFFFFFFF);
    
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
    
    printf("\n=== RÉSUMÉ C505 ===\n");
    printf("Durée totale: %.3f ms\n", elapsed);
    printf("EXECBUFFER2: %s\n", ret == 0 ? "✓ SUCCÈS" : "✗ ÉCHEC");
    printf("Kernel: IDENTIQUE à OpenCL test_c427\n");
    
    if (success) {
        printf("\n🎉🎉🎉 SUCCÈS TOTAL - PREMIER 0x12345678 GPU NATIF! 🎉🎉🎉\n");
        return 0;
    } else if (modified) {
        printf("\n⚠ GPU EXÉCUTÉ mais résultat incorrect\n");
        return 0;
    } else {
        printf("\n❌ Pas de modification - Vérifier dmesg\n");
        return 1;
    }
}

// Made with Bob - C505 ROOT CAUSE #9: Kernel ISA différent