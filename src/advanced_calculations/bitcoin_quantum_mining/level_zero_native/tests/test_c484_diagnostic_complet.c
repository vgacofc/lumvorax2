/*
 * TEST C484: DIAGNOSTIC COMPLET - Dump toutes les structures
 * 
 * Objectif: Identifier EXACTEMENT pourquoi le kernel n'est pas exécuté
 * en dumpant TOUTES les structures critiques
 * 
 * Base: Code test_c481 qui compile et s'exécute avec succès
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <time.h>
#include <libdrm/i915_drm.h>
#include <libdrm/intel_bufmgr.h>
#include "beignet_i915_minimal.h"
#include "intel_structs_minimal.h"
#include "beignet_batch_builder.h"
#include "test_c475_minimal_compute_text.h"

// Fonction dump hexadécimal
static void dump_hex(const char *label, const void *data, size_t size) {
    const uint8_t *bytes = (const uint8_t*)data;
    printf("\n=== %s (%zu bytes) ===\n", label, size);
    for (size_t i = 0; i < size; i += 16) {
        printf("%04zx: ", i);
        for (size_t j = 0; j < 16 && i + j < size; j++) {
            printf("%02x ", bytes[i + j]);
        }
        printf("\n");
    }
}

// Fonction dump DWORD
static void dump_dwords(const char *label, const uint32_t *data, size_t count) {
    printf("\n=== %s (%zu DWORDs) ===\n", label, count);
    for (size_t i = 0; i < count; i++) {
        printf("  [%2zu] 0x%08x", i, data[i]);
        if (i % 4 == 3) printf("\n");
    }
    if (count % 4 != 0) printf("\n");
}

int main(void) {
    printf("=== CYCLE C484: DIAGNOSTIC COMPLET ===\n\n");
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    // 1. Ouverture /dev/dri/renderD128
    printf("[1/12] Ouverture /dev/dri/renderD128...\n");
    int fd = open("/dev/dri/renderD128", O_RDWR);
    if (fd < 0) {
        perror("ERREUR: open /dev/dri/renderD128");
        return 1;
    }
    printf("  ✓ fd = %d\n", fd);
    
    // 2. Initialisation drm_intel_bufmgr
    printf("\n[2/12] Initialisation drm_intel_bufmgr...\n");
    drm_intel_bufmgr *bufmgr = drm_intel_bufmgr_gem_init(fd, 4096);
    if (!bufmgr) {
        fprintf(stderr, "ERREUR: drm_intel_bufmgr_gem_init failed\n");
        close(fd);
        return 1;
    }
    printf("  ✓ bufmgr initialisé\n");
    
    // 3. Allocation buffers
    printf("\n[3/12] Allocation buffers...\n");
    drm_intel_bo *batch_bo = drm_intel_bo_alloc(bufmgr, "batch", 65536, 4096);
    drm_intel_bo *output_bo = drm_intel_bo_alloc(bufmgr, "output", 4096, 4096);
    drm_intel_bo *aux_buf_bo = drm_intel_bo_alloc(bufmgr, "aux_buf", 65536, 4096);
    drm_intel_bo *kernel_bo = drm_intel_bo_alloc(bufmgr, "kernel", 4096, 4096);
    
    if (!batch_bo || !output_bo || !aux_buf_bo || !kernel_bo) {
        fprintf(stderr, "ERREUR: Allocation buffers failed\n");
        if (batch_bo) drm_intel_bo_unreference(batch_bo);
        if (output_bo) drm_intel_bo_unreference(output_bo);
        if (aux_buf_bo) drm_intel_bo_unreference(aux_buf_bo);
        if (kernel_bo) drm_intel_bo_unreference(kernel_bo);
        drm_intel_bufmgr_destroy(bufmgr);
        close(fd);
        return 1;
    }
    
    printf("  ✓ batch_bo: %zu bytes\n", batch_bo->size);
    printf("  ✓ output_bo: %zu bytes\n", output_bo->size);
    printf("  ✓ aux_buf_bo: %zu bytes\n", aux_buf_bo->size);
    printf("  ✓ kernel_bo: %zu bytes\n", kernel_bo->size);
    
    // 4. Initialisation output buffer
    printf("\n[4/12] Initialisation output buffer...\n");
    drm_intel_bo_map(output_bo, 1);
    uint32_t *output = (uint32_t*)output_bo->virtual;
    for (int i = 0; i < 4; i++) {
        output[i] = 0xFFFFFFFF;
    }
    drm_intel_bo_unmap(output_bo);
    printf("  ✓ Output rempli avec 0xFFFFFFFF\n");
    
    // 5. Copie kernel ISA Gen9
    printf("\n[5/12] Copie kernel ISA Gen9...\n");
    drm_intel_bo_map(kernel_bo, 1);
    memcpy(kernel_bo->virtual, kernel_text, KERNEL_TEXT_SIZE);
    drm_intel_bo_unmap(kernel_bo);
    printf("  ✓ Kernel ISA copié: %d bytes\n", KERNEL_TEXT_SIZE);
    
    // 6. Configuration Binding Table (CODE EXACT test_c481)
    printf("\n[6/12] Configuration Binding Table (Beignet)...\n");
    
    uint32_t surface_heap_offset = 0;
    
    beignet_i915_setup_bti_gen7(aux_buf_bo,
                                 surface_heap_offset,
                                 output_bo,
                                 0,
                                 output_bo->size,
                                 0,
                                 I965_SURFACEFORMAT_RAW);
    
    printf("  ✓ Binding Table configurée\n");
    printf("  ✓ Surface State Gen7 configurée\n");
    printf("  ✓ Relocation enregistrée\n");
    
    // 7. Configuration Interface Descriptor (CODE EXACT test_c481)
    printf("\n[7/12] Configuration Interface Descriptor...\n");
    
    drm_intel_bo_map(aux_buf_bo, 1);
    
    uint32_t idrt_offset = 8192;
    gen8_interface_descriptor_t *idrt = (gen8_interface_descriptor_t*)
        ((char*)aux_buf_bo->virtual + idrt_offset);
    
    memset(idrt, 0, sizeof(*idrt));
    idrt->dw0 = 0;
    idrt->dw3 = (12288 >> 5) << 5;
    
    // CORRECTION C483: Binding Table Pointer
    uint32_t bti_offset = 0;
    idrt->dw4 = (bti_offset >> 5) | (1 << 0);
    
    idrt->dw5 = 0;
    idrt->dw6 = (1 << 24);
    idrt->dw7 = 0;
    
    drm_intel_bo_unmap(aux_buf_bo);
    
    // RELOCATION kernel dans IDRT
    drm_intel_bo_emit_reloc(aux_buf_bo,
                            idrt_offset + offsetof(gen8_interface_descriptor_t, dw0),
                            kernel_bo,
                            0,
                            I915_GEM_DOMAIN_INSTRUCTION,
                            0);
    
    printf("  ✓ Interface Descriptor configuré\n");
    printf("  ✓ Relocation kernel enregistrée\n");
    
    // 8. Construction batch GPGPU (CODE EXACT test_c481)
    printf("\n[8/12] Construction batch GPGPU COMPLET...\n");
    
    beignet_batch_builder_t builder;
    beignet_batch_init(&builder, batch_bo, aux_buf_bo, kernel_bo, output_bo);
    
    builder.thread_width = 1;
    builder.thread_height = 1;
    builder.thread_depth = 1;
    builder.max_threads = 64;
    builder.curbe_size = 0;
    builder.idrt_offset = idrt_offset;
    
    uint32_t batch_len = beignet_build_gpgpu_batch_gen9(&builder);
    
    printf("  ✓ Batch construit: %u bytes\n", batch_len);
    printf("  ✓ PIPE_CONTROL\n");
    printf("  ✓ PIPELINE_SELECT GPGPU\n");
    printf("  ✓ STATE_BASE_ADDRESS + relocations\n");
    printf("  ✓ MEDIA_VFE_STATE\n");
    printf("  ✓ MEDIA_INTERFACE_DESCRIPTOR_LOAD\n");
    printf("  ✓ GPGPU_WALKER\n");
    printf("  ✓ MI_BATCH_BUFFER_END\n");
    
    // 9. DUMP COMPLET - Binding Table
    printf("\n[9/12] DUMP Binding Table...\n");
    drm_intel_bo_map(aux_buf_bo, 1);
    surface_heap_t *heap = (surface_heap_t*)((char*)aux_buf_bo->virtual + surface_heap_offset);
    dump_dwords("Binding Table", heap->binding_table, 4);
    drm_intel_bo_unmap(aux_buf_bo);
    
    // 10. DUMP COMPLET - Surface State
    printf("\n[10/12] DUMP Surface State...\n");
    drm_intel_bo_map(aux_buf_bo, 1);
    heap = (surface_heap_t*)((char*)aux_buf_bo->virtual + surface_heap_offset);
    dump_dwords("Surface State Gen7", (uint32_t*)heap->surface, 8);
    drm_intel_bo_unmap(aux_buf_bo);
    
    // 11. DUMP COMPLET - Interface Descriptor
    printf("\n[11/12] DUMP Interface Descriptor...\n");
    drm_intel_bo_map(aux_buf_bo, 1);
    dump_dwords("Interface Descriptor Gen8",
                (uint32_t*)((char*)aux_buf_bo->virtual + idrt_offset), 8);
    drm_intel_bo_unmap(aux_buf_bo);
    
    // 12. DUMP COMPLET - Batch Buffer
    printf("\n[12/12] DUMP Batch Buffer...\n");
    drm_intel_bo_map(batch_bo, 1);
    dump_hex("Batch Buffer", batch_bo->virtual, batch_len);
    drm_intel_bo_unmap(batch_bo);
    
    // Soumission EXECBUFFER2
    printf("\n[EXEC] Soumission EXECBUFFER2...\n");
    
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
    
    int ret = drm_intel_gem_bo_context_exec(batch_bo, ctx, batch_len, 0);
    if (ret != 0) {
        fprintf(stderr, "ERREUR: EXECBUFFER2 failed: %d\n", ret);
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
    
    // Vérification résultat
    printf("\n[RESULT] Vérification résultat...\n");
    drm_intel_bo_map(output_bo, 0);
    output = (uint32_t*)output_bo->virtual;
    
    printf("  Output[0] = 0x%08x", output[0]);
    if (output[0] == 0xDEADBEEF) printf(" ✓✓✓ SUCCÈS! ✓✓✓\n");
    else printf(" (attendu: 0xDEADBEEF)\n");
    
    printf("  Output[1] = 0x%08x", output[1]);
    if (output[1] == 0xCAFEBABE) printf(" ✓✓✓ SUCCÈS! ✓✓✓\n");
    else printf(" (attendu: 0xCAFEBABE)\n");
    
    printf("  Output[2] = 0x%08x", output[2]);
    if (output[2] == 0x12345678) printf(" ✓✓✓ SUCCÈS! ✓✓✓\n");
    else printf(" (attendu: 0x12345678)\n");
    
    printf("  Output[3] = 0x%08x", output[3]);
    if (output[3] == 0xABCDEF00) printf(" ✓✓✓ SUCCÈS! ✓✓✓\n");
    else printf(" (attendu: 0xABCDEF00)\n");
    
    // Sauvegarder les valeurs AVANT unmap
    uint32_t out0 = output[0];
    uint32_t out1 = output[1];
    uint32_t out2 = output[2];
    uint32_t out3 = output[3];
    
    drm_intel_bo_unmap(output_bo);
    
    // Cleanup
    drm_intel_gem_context_destroy(ctx);
    drm_intel_bo_unreference(batch_bo);
    drm_intel_bo_unreference(output_bo);
    drm_intel_bo_unreference(aux_buf_bo);
    drm_intel_bo_unreference(kernel_bo);
    drm_intel_bufmgr_destroy(bufmgr);
    close(fd);
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) * 1000.0 +
                     (end.tv_nsec - start.tv_nsec) / 1000000.0;
    
    printf("\n=== RÉSUMÉ C484 ===\n");
    printf("Durée totale: %.3f ms\n", elapsed);
    printf("EXECBUFFER2: ✓ SUCCÈS\n");
    printf("Batch: GPGPU COMPLET (Beignet)\n");
    printf("Relocations: Automatiques (STATE_BASE_ADDRESS + IDRT + BTI)\n");
    
    if (out0 == 0xDEADBEEF && out1 == 0xCAFEBABE &&
        out2 == 0x12345678 && out3 == 0xABCDEF00) {
        printf("Statut: ✓✓✓ KERNEL EXÉCUTÉ! PREMIER RÉSULTAT GPU NATIF! ✓✓✓\n");
        return 0;
    } else {
        printf("Statut: ⚠ Pas de modification\n");
        return 1;
    }
}

// Made with Bob
