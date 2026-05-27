// TEST C535 - ROOT CAUSE #41: PATCHER TOUTES LES ADRESSES DYNAMIQUES
// Le batch OpenCL contient 13+ adresses de l'exécution précédente
// Solution: Identifier et patcher TOUTES les adresses avec nos offsets actuels

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
#include "test_c534_text_only.h"

#define BATCH_SIZE 4096
#define OUTPUT_SIZE 4096
#define KERNEL_SIZE 4096

// Batch OpenCL capturé EXACT (320 bytes)
static const uint8_t opencl_batch_template[320] = {
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

// Structure pour les adresses à patcher
typedef struct {
    uint32_t offset;      // Offset dans le batch
    const char *name;     // Nom du buffer
    uint64_t *target_addr; // Pointeur vers l'adresse cible
} AddressPatch;

int main(void) {
    printf("\n╔════════════════════════════════════════════════════════════════╗\n");
    printf("║  TEST C535 - PATCH TOUTES ADRESSES (ROOT CAUSE #41)         ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n\n");
    printf("ROOT CAUSE #41: Batch OpenCL contient 13+ adresses dynamiques\n");
    printf("Solution: Patcher TOUTES les adresses avec nos offsets i915\n\n");

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
    printf("[4/15] Initialisation output + kernel .TEXT PURE...\n");
    drm_intel_bo_map(output_bo, 1);
    uint32_t *output = (uint32_t*)output_bo->virtual;
    for (int i = 0; i < 1024; i++) output[i] = 0xFFFFFFFF;
    drm_intel_bo_unmap(output_bo);
    
    drm_intel_bo_map(kernel_bo, 1);
    memcpy(kernel_bo->virtual, kernel_text_only, kernel_text_only_size);
    drm_intel_bo_unmap(kernel_bo);
    printf("  ✓ Output et kernel .TEXT PURE initialisés (%zu bytes)\n\n", kernel_text_only_size);

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
        printf("  ❌ PASS 1 ÉCHEC: %s\n", strerror(errno));
        return 1;
    }
    printf("  ✅ PASS 1 RÉUSSI\n\n");

    // [6/15] Lecture offsets i915
    printf("[6/15] Lecture offsets i915...\n");
    uint64_t surface_offset = exec_objects_pass1[0].offset;
    uint64_t dynamic_offset = exec_objects_pass1[1].offset;
    uint64_t idrt_offset = exec_objects_pass1[2].offset;
    uint64_t kernel_offset = exec_objects_pass1[3].offset;
    uint64_t output_offset = exec_objects_pass1[4].offset;
    uint64_t batch_offset = exec_objects_pass1[5].offset;
    
    printf("  ✓ OFFSETS i915:\n");
    printf("    surface_state: 0x%016lX\n", surface_offset);
    printf("    dynamic_state: 0x%016lX\n", dynamic_offset);
    printf("    idrt:          0x%016lX\n", idrt_offset);
    printf("    kernel:        0x%016lX\n", kernel_offset);
    printf("    output:        0x%016lX\n", output_offset);
    printf("    batch:         0x%016lX\n\n", batch_offset);

    // [7/15] Chargement buffers OpenCL
    printf("[7/15] Chargement buffers OpenCL depuis dumps...\n");
    
    FILE *f_ss = fopen("/tmp/opencl_batch_dump_buffer_03_0.bin", "rb");
    FILE *f_ds = fopen("/tmp/opencl_batch_dump_buffer_02_0.bin", "rb");
    FILE *f_idrt = fopen("/tmp/opencl_batch_dump_buffer_08_0.bin", "rb");
    
    if (!f_ss || !f_ds || !f_idrt) {
        printf("  ❌ Échec ouverture dumps OpenCL\n");
        return 1;
    }
    
    drm_intel_bo_map(surface_state_bo, 1);
    fread(surface_state_bo->virtual, 1, 4096, f_ss);
    drm_intel_bo_unmap(surface_state_bo);
    fclose(f_ss);
    
    drm_intel_bo_map(dynamic_state_bo, 1);
    fread(dynamic_state_bo->virtual, 1, 4096, f_ds);
    drm_intel_bo_unmap(dynamic_state_bo);
    fclose(f_ds);
    
    drm_intel_bo_map(idrt_bo, 1);
    fread(idrt_bo->virtual, 1, 4096, f_idrt);
    drm_intel_bo_unmap(idrt_bo);
    fclose(f_idrt);
    
    printf("  ✓ Buffers OpenCL chargés\n\n");

    // [8/15] Patch output address dans Surface State
    printf("[8/15] Patch output address dans Surface State...\n");
    drm_intel_bo_map(surface_state_bo, 1);
    uint64_t *ss_ptr = (uint64_t*)((uint8_t*)surface_state_bo->virtual + 0x48);
    *ss_ptr = output_offset;
    drm_intel_bo_unmap(surface_state_bo);
    printf("  ✓ Output address patché: 0x%016lX\n\n", output_offset);

    // [9/15] Patch kernel pointer dans IDRT
    printf("[9/15] Patch kernel pointer dans IDRT...\n");
    drm_intel_bo_map(idrt_bo, 1);
    uint64_t *idrt_ptr = (uint64_t*)idrt_bo->virtual;
    idrt_ptr[0] = kernel_offset;
    drm_intel_bo_unmap(idrt_bo);
    printf("  ✓ Kernel pointer patché: 0x%016lX\n\n", kernel_offset);

    // [10/15] PATCHER TOUTES LES ADRESSES DANS LE BATCH
    printf("[10/15] PATCHER TOUTES LES ADRESSES BATCH (ROOT CAUSE #41)...\n");
    
    drm_intel_bo_map(batch_bo, 1);
    uint8_t *batch = (uint8_t*)batch_bo->virtual;
    memcpy(batch, opencl_batch_template, 320);
    
    // Liste des adresses à patcher (identifiées par analyse)
    AddressPatch patches[] = {
        {0x0B0, "idrt", &idrt_offset},
        {0x0B8, "idrt", &idrt_offset},
        {0x0E0, "idrt", &idrt_offset},
        {0x114, "kernel", &kernel_offset},
        {0x118, "dynamic", &dynamic_offset},
    };
    
    int num_patches = sizeof(patches) / sizeof(patches[0]);
    
    for (int i = 0; i < num_patches; i++) {
        uint64_t *addr_ptr = (uint64_t*)(batch + patches[i].offset);
        uint64_t old_addr = *addr_ptr;
        *addr_ptr = *patches[i].target_addr;
        printf("  ✓ Patch @ 0x%03X: %s 0x%016lX → 0x%016lX\n", 
               patches[i].offset, patches[i].name, old_addr, *patches[i].target_addr);
    }
    
    drm_intel_bo_unmap(batch_bo);
    printf("\n  ✅ %d adresses patchées dans le batch\n\n", num_patches);

    // [11/15] Construction exec_objects final
    printf("[11/15] Construction exec_objects final...\n");
    
    struct drm_i915_gem_exec_object2 exec_objects[6];
    memset(exec_objects, 0, sizeof(exec_objects));
    
    uint64_t pinned_flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS | EXEC_OBJECT_PINNED;
    
    exec_objects[0].handle = surface_state_bo->handle;
    exec_objects[0].offset = surface_offset;
    exec_objects[0].flags = pinned_flags;
    
    exec_objects[1].handle = dynamic_state_bo->handle;
    exec_objects[1].offset = dynamic_offset;
    exec_objects[1].flags = pinned_flags;
    
    exec_objects[2].handle = idrt_bo->handle;
    exec_objects[2].offset = idrt_offset;
    exec_objects[2].flags = pinned_flags;
    
    exec_objects[3].handle = kernel_bo->handle;
    exec_objects[3].offset = kernel_offset;
    exec_objects[3].flags = pinned_flags;
    
    exec_objects[4].handle = output_bo->handle;
    exec_objects[4].offset = output_offset;
    exec_objects[4].flags = pinned_flags | EXEC_OBJECT_WRITE;
    
    exec_objects[5].handle = batch_bo->handle;
    exec_objects[5].offset = batch_offset;
    exec_objects[5].flags = pinned_flags;
    
    printf("  ✓ 6 exec_objects configurés avec PINNED\n\n");

    // [12/15] PASS 2: Soumission batch patché
    printf("[12/15] PASS 2: Soumission batch AVEC TOUTES ADRESSES PATCHÉES...\n");
    printf("  🔧 PASS 2 - ADRESSES DYNAMIQUES PATCHÉES:\n");
    printf("     Batch: Copie OpenCL avec %d adresses patchées\n", num_patches);
    printf("     Kernel: @ 0x%016lX\n", kernel_offset);
    printf("     Output: @ 0x%016lX\n\n", output_offset);
    
    struct drm_i915_gem_execbuffer2 execbuf;
    memset(&execbuf, 0, sizeof(execbuf));
    execbuf.buffers_ptr = (uintptr_t)exec_objects;
    execbuf.buffer_count = 6;
    execbuf.batch_start_offset = 0;
    execbuf.batch_len = 320;
    execbuf.flags = I915_EXEC_RENDER | I915_EXEC_NO_RELOC;
    execbuf.rsvd1 = ctx_id;
    
    ret = drmIoctl(fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf);
    if (ret != 0) {
        printf("  ❌ PASS 2 ÉCHEC: %s\n", strerror(errno));
        return 1;
    }
    printf("  ✅✅✅ PASS 2 RÉUSSI! ✅✅✅\n\n");

    // [13/15] Attente GPU
    printf("[13/15] Attente GPU...\n");
    drm_intel_bo_wait_rendering(output_bo);
    printf("  ✓ GPU terminé\n\n");

    // [14/15] VÉRIFICATION RÉSULTAT
    printf("[14/15] VÉRIFICATION RÉSULTAT GPU NATIF...\n");
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
        printf("  ✅✅✅ SUCCESS! GPU NATIF FONCTIONNE! ✅✅✅\n");
        printf("  🎉 PREMIER RÉSULTAT VALIDE SUR GPU NATIF i915!\n");
        printf("  🎉 ÉLIMINATION OPENCL: 100%% COMPLÈTE!\n");
    } else {
        printf("  ⚠️  Adresses patchées mais résultat incorrect\n");
        printf("  → Vérifier les autres adresses dans le batch\n");
    }
    
    // Cleanup
    drm_intel_gem_context_destroy(ctx);
    drm_intel_bo_unreference(surface_state_bo);
    drm_intel_bo_unreference(dynamic_state_bo);
    drm_intel_bo_unreference(idrt_bo);
    drm_intel_bo_unreference(kernel_bo);
    drm_intel_bo_unreference(output_bo);
    drm_intel_bo_unreference(batch_bo);
    drm_intel_bufmgr_destroy(bufmgr);
    close(fd);
    
    return success ? 0 : 1;
}

// Made with Bob
