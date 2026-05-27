#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <intel_bufmgr.h>
#include <i915_drm.h>

#include "beignet_i915/beignet_i915_minimal.h"
#include "beignet_i915/beignet_batch_builder_simd32.h"
#include "tests/test_c491_opencl_isa_gen9.h"

int main(void) {
    printf("=== C500: DUMP BATCH SIMD32 ===\n\n");
    
    int fd = open("/dev/dri/renderD128", O_RDWR);
    drm_intel_bufmgr *bufmgr = drm_intel_bufmgr_gem_init(fd, 4096);
    drm_intel_bufmgr_gem_enable_reuse(bufmgr);
    
    drm_intel_bo *batch_bo = drm_intel_bo_alloc(bufmgr, "batch", 65536, 4096);
    drm_intel_bo *output_bo = drm_intel_bo_alloc(bufmgr, "output", 4096, 4096);
    drm_intel_bo *aux_buf_bo = drm_intel_bo_alloc(bufmgr, "aux_buf", 65536, 4096);
    drm_intel_bo *kernel_bo = drm_intel_bo_alloc(bufmgr, "kernel", 4096, 64);
    
    drm_intel_bo_map(kernel_bo, 1);
    memcpy(kernel_bo->virtual, kernel_isa_opencl_gen9, sizeof(kernel_isa_opencl_gen9));
    drm_intel_bo_unmap(kernel_bo);
    
    beignet_i915_setup_bti_gen7(aux_buf_bo, 0, output_bo, 0, 4096, 0, I965_SURFACEFORMAT_RAW);
    
    drm_intel_bo_map(aux_buf_bo, 1);
    uint32_t idrt_offset = 8192;
    gen8_interface_descriptor_t *idrt = (gen8_interface_descriptor_t*)
        ((char*)aux_buf_bo->virtual + idrt_offset);
    memset(idrt, 0, sizeof(*idrt));
    idrt->dw0 = 0;
    idrt->dw3 = (12288 >> 5) << 5;
    idrt->dw4 = (0 >> 5) | (1 << 0);
    idrt->dw5 = 0;
    idrt->dw6 = (256U << 24);
    idrt->dw7 = 0;
    drm_intel_bo_unmap(aux_buf_bo);
    
    drm_intel_bo_emit_reloc(aux_buf_bo, idrt_offset + offsetof(gen8_interface_descriptor_t, dw0),
                            kernel_bo, 0, I915_GEM_DOMAIN_INSTRUCTION, 0);
    
    beignet_batch_builder_t builder;
    beignet_batch_init(&builder, batch_bo, aux_buf_bo, kernel_bo, output_bo);
    builder.thread_width = 8;
    builder.thread_height = 1;
    builder.thread_depth = 1;
    builder.max_threads = 256;
    builder.curbe_size = 0;
    builder.idrt_offset = idrt_offset;
    
    printf("Construction batch SIMD32...\n");
    uint32_t batch_len = beignet_build_gpgpu_batch_gen9_simd32(&builder, 32, 256);
    
    printf("Batch size: %u bytes (%u DWORDs)\n\n", batch_len, batch_len/4);
    
    drm_intel_bo_map(batch_bo, 0);
    uint32_t *batch = (uint32_t*)batch_bo->virtual;
    
    printf("=== BATCH DUMP (first 100 DWORDs) ===\n");
    for (int i = 0; i < 100 && i < (int)(batch_len/4); i++) {
        printf("%04x: 0x%08x", i*4, batch[i]);
        if ((i % 4) == 3) printf("\n");
        else printf("  ");
    }
    printf("\n");
    
    drm_intel_bo_unmap(batch_bo);
    
    drm_intel_bo_unreference(batch_bo);
    drm_intel_bo_unreference(output_bo);
    drm_intel_bo_unreference(aux_buf_bo);
    drm_intel_bo_unreference(kernel_bo);
    drm_intel_bufmgr_destroy(bufmgr);
    close(fd);
    
    return 0;
}
