// TEST C528 - REPLAY EXACT EXECBUFFER2 OPENCL
// Stratégie: Copier TOUS les buffers OpenCL byte-for-byte
// Reproduire EXACTEMENT la même VA map GPU
// Modifier UNIQUEMENT l'adresse du output buffer

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

#define BUFFER_SIZE 4096
#define BATCH_SIZE 320

int main(void) {
    printf("\n╔════════════════════════════════════════════════════════════════╗\n");
    printf("║  TEST C528 - REPLAY EXACT EXECBUFFER2 OPENCL                ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n\n");
    printf("Stratégie: Copier TOUS les buffers OpenCL byte-for-byte\n");
    printf("Reproduire la même VA map GPU\n");
    printf("Modifier UNIQUEMENT l'adresse output\n\n");

    // [1/10] Ouvrir DRM
    printf("[1/10] Ouverture /dev/dri/renderD128...\n");
    int fd = open("/dev/dri/renderD128", O_RDWR);
    if (fd < 0) {
        printf("  ❌ Échec ouverture DRM\n");
        return 1;
    }
    printf("  ✓ DRM ouvert (fd=%d)\n\n", fd);

    // [2/10] Init libdrm_intel
    printf("[2/10] Initialisation libdrm_intel...\n");
    drm_intel_bufmgr *bufmgr = drm_intel_bufmgr_gem_init(fd, 4096);
    if (!bufmgr) {
        printf("  ❌ Échec init bufmgr\n");
        close(fd);
        return 1;
    }
    printf("  ✓ Buffer manager initialisé\n\n");

    // [3/10] Allocation des 10 buffers (comme OpenCL)
    printf("[3/10] Allocation 10 buffers (replay exact OpenCL)...\n");
    drm_intel_bo *buffers[10];
    
    for (int i = 0; i < 10; i++) {
        char name[32];
        snprintf(name, sizeof(name), "buffer_%02d", i);
        
        if (i == 9) {
            // Batch buffer: 320 bytes
            buffers[i] = drm_intel_bo_alloc(bufmgr, name, BATCH_SIZE, 4096);
        } else {
            // Autres buffers: 4096 bytes
            buffers[i] = drm_intel_bo_alloc(bufmgr, name, BUFFER_SIZE, 4096);
        }
        
        if (!buffers[i]) {
            printf("  ❌ Échec allocation buffer %d\n", i);
            return 1;
        }
    }
    printf("  ✓ 10 buffers alloués\n\n");

    // [4/10] Charger les dumps OpenCL
    printf("[4/10] Chargement dumps OpenCL...\n");
    
    const char *dump_files[] = {
        "/tmp/opencl_batch_dump_buffer_00_0.bin",
        NULL,  // buffer_01 non dumpé
        "/tmp/opencl_batch_dump_buffer_02_0.bin",
        "/tmp/opencl_batch_dump_buffer_03_0.bin",
        NULL,  // buffer_04 non dumpé
        "/tmp/opencl_batch_dump_buffer_05_0.bin",
        "/tmp/opencl_batch_dump_buffer_06_0.bin",
        NULL,  // buffer_07 non dumpé
        "/tmp/opencl_batch_dump_buffer_08_0.bin",
        "/tmp/opencl_batch_dump_batch_0.bin"
    };
    
    for (int i = 0; i < 10; i++) {
        if (dump_files[i] == NULL) {
            // Buffer non dumpé: initialiser à zéro
            drm_intel_bo_map(buffers[i], 1);
            memset(buffers[i]->virtual, 0, i == 9 ? BATCH_SIZE : BUFFER_SIZE);
            drm_intel_bo_unmap(buffers[i]);
            printf("  ⚠️  buffer_%02d: initialisé à zéro (non dumpé)\n", i);
        } else {
            // Charger le dump
            FILE *f = fopen(dump_files[i], "rb");
            if (!f) {
                printf("  ❌ Échec ouverture %s\n", dump_files[i]);
                return 1;
            }
            
            drm_intel_bo_map(buffers[i], 1);
            size_t size = i == 9 ? BATCH_SIZE : BUFFER_SIZE;
            size_t read_bytes = fread(buffers[i]->virtual, 1, size, f);
            drm_intel_bo_unmap(buffers[i]);
            fclose(f);
            
            printf("  ✓ buffer_%02d: %zu bytes chargés\n", i, read_bytes);
        }
    }
    printf("\n");

    // [5/10] Créer notre output buffer
    printf("[5/10] Création output buffer...\n");
    drm_intel_bo *output_bo = drm_intel_bo_alloc(bufmgr, "output", BUFFER_SIZE, 4096);
    if (!output_bo) {
        printf("  ❌ Échec allocation output\n");
        return 1;
    }
    
    // Initialiser output à 0xFFFFFFFF
    drm_intel_bo_map(output_bo, 1);
    uint32_t *output = (uint32_t*)output_bo->virtual;
    for (int i = 0; i < 1024; i++) output[i] = 0xFFFFFFFF;
    drm_intel_bo_unmap(output_bo);
    printf("  ✓ Output buffer créé et initialisé\n\n");

    // [6/10] PASS 1: Résolution offsets
    printf("[6/10] PASS 1: Résolution offsets i915...\n");
    
    drm_intel_context *ctx = drm_intel_gem_context_create(bufmgr);
    if (!ctx) {
        printf("  ❌ Échec création context\n");
        return 1;
    }
    
    // Batch temporaire pour PASS 1
    drm_intel_bo_map(buffers[9], 1);
    uint32_t *batch_temp = (uint32_t*)buffers[9]->virtual;
    batch_temp[0] = 0x00000000;  // MI_NOOP
    batch_temp[1] = 0x0A000000;  // MI_BATCH_BUFFER_END
    drm_intel_bo_unmap(buffers[9]);
    
    struct drm_i915_gem_exec_object2 exec_objects_pass1[11];  // 10 buffers + output
    memset(exec_objects_pass1, 0, sizeof(exec_objects_pass1));
    
    uint64_t base_flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS;
    
    for (int i = 0; i < 10; i++) {
        exec_objects_pass1[i].handle = buffers[i]->handle;
        exec_objects_pass1[i].flags = base_flags;
    }
    
    exec_objects_pass1[10].handle = output_bo->handle;
    exec_objects_pass1[10].flags = base_flags | EXEC_OBJECT_WRITE;
    
    struct drm_i915_gem_execbuffer2 execbuf_pass1;
    memset(&execbuf_pass1, 0, sizeof(execbuf_pass1));
    execbuf_pass1.buffers_ptr = (uintptr_t)exec_objects_pass1;
    execbuf_pass1.buffer_count = 11;
    execbuf_pass1.batch_start_offset = 0;
    execbuf_pass1.batch_len = 8;
    execbuf_pass1.flags = I915_EXEC_RENDER;
    execbuf_pass1.rsvd2 = 0;  // Pas de cliprects
    
    uint32_t ctx_id;
    drm_intel_gem_context_get_id(ctx, &ctx_id);
    execbuf_pass1.rsvd1 = ctx_id;
    
    int ret = drmIoctl(fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf_pass1);
    if (ret != 0) {
        printf("  ❌ PASS 1 failed (ret=%d, errno=%d: %s)\n", ret, errno, strerror(errno));
        return 1;
    }
    printf("  ✅ PASS 1 RÉUSSI\n\n");

    // [7/10] Lecture offsets i915
    printf("[7/10] Lecture offsets i915...\n");
    uint64_t buffer_offsets[10];
    for (int i = 0; i < 10; i++) {
        buffer_offsets[i] = exec_objects_pass1[i].offset;
        printf("  buffer_%02d: 0x%016lx\n", i, buffer_offsets[i]);
    }
    uint64_t output_offset = exec_objects_pass1[10].offset;
    printf("  output:     0x%016lx\n\n", output_offset);

    // [8/10] Patcher UNIQUEMENT l'adresse output dans buffer_03 (Surface State)
    printf("[8/10] Patch adresse output dans Surface State...\n");
    drm_intel_bo_map(buffers[3], 1);
    uint32_t *surface_heap = (uint32_t*)buffers[3]->virtual;
    uint32_t *surface_state = &surface_heap[16];  // offset 0x40
    
    printf("  Surface State avant patch:\n");
    printf("    DW6: 0x%08x\n", surface_state[6]);
    printf("    DW7: 0x%08x\n", surface_state[7]);
    
    surface_state[6] = (uint32_t)(output_offset & 0xFFFFFFFF);
    surface_state[7] = (uint32_t)(output_offset >> 32);
    
    printf("  Surface State après patch:\n");
    printf("    DW6: 0x%08x\n", surface_state[6]);
    printf("    DW7: 0x%08x\n", surface_state[7]);
    
    drm_intel_bo_unmap(buffers[3]);
    printf("  ✓ Output address patché\n\n");

    // [9/10] Recharger le batch OpenCL original
    printf("[9/10] Rechargement batch OpenCL original...\n");
    FILE *f_batch = fopen("/tmp/opencl_batch_dump_batch_0.bin", "rb");
    if (!f_batch) {
        printf("  ❌ Échec ouverture batch\n");
        return 1;
    }
    
    drm_intel_bo_map(buffers[9], 1);
    fread(buffers[9]->virtual, 1, BATCH_SIZE, f_batch);
    drm_intel_bo_unmap(buffers[9]);
    fclose(f_batch);
    printf("  ✓ Batch OpenCL rechargé (320 bytes)\n\n");

    // [10/10] PASS 2: Soumission EXACTE comme OpenCL
    printf("[10/10] PASS 2: Soumission EXACTE OpenCL...\n");
    
    struct drm_i915_gem_exec_object2 exec_objects_final[11];
    memset(exec_objects_final, 0, sizeof(exec_objects_final));
    
    uint64_t pinned_flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS | EXEC_OBJECT_PINNED;
    
    for (int i = 0; i < 10; i++) {
        exec_objects_final[i].handle = buffers[i]->handle;
        exec_objects_final[i].offset = buffer_offsets[i];
        exec_objects_final[i].flags = pinned_flags;
    }
    
    exec_objects_final[10].handle = output_bo->handle;
    exec_objects_final[10].offset = output_offset;
    exec_objects_final[10].flags = pinned_flags | EXEC_OBJECT_WRITE;
    
    struct drm_i915_gem_execbuffer2 execbuf_final;
    memset(&execbuf_final, 0, sizeof(execbuf_final));
    execbuf_final.buffers_ptr = (uintptr_t)exec_objects_final;
    execbuf_final.buffer_count = 11;
    execbuf_final.batch_start_offset = 0;
    execbuf_final.batch_len = BATCH_SIZE;
    execbuf_final.flags = I915_EXEC_NO_RELOC;
    execbuf_final.rsvd1 = ctx_id;
    
    printf("  🔧 REPLAY EXACT:\n");
    printf("     11 buffers (10 OpenCL + 1 output)\n");
    printf("     Batch: 320 bytes OpenCL exact\n");
    printf("     Patch: Output address uniquement\n");
    printf("     Flags: NO_RELOC (soft-pinning)\n\n");
    
    ret = drmIoctl(fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf_final);
    if (ret != 0) {
        printf("  ❌ PASS 2 failed (ret=%d, errno=%d: %s)\n", ret, errno, strerror(errno));
        return 1;
    }
    printf("  ✅✅✅ PASS 2 RÉUSSI! ✅✅✅\n\n");

    // Attente GPU
    printf("Attente GPU...\n");
    drm_intel_bo_wait_rendering(output_bo);
    printf("  ✓ GPU terminé\n\n");

    // VÉRIFICATION RÉSULTAT
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("VÉRIFICATION RÉSULTAT GPU NATIF\n");
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
        printf("  ✅✅✅ SUCCÈS! GPU NATIF i915 FONCTIONNEL! ✅✅✅\n");
        printf("  🎯 REPLAY EXACT OPENCL VALIDÉ!\n");
        printf("  🚀 ÉLIMINATION OPENCL: 100%% COMPLÈTE!\n");
    } else {
        printf("  ⚠️  Échec - Analyse supplémentaire requise\n");
        printf("  💡 Prochaine étape: Vérifier BTI index dans kernel\n");
    }
    
    // Cleanup
    for (int i = 0; i < 10; i++) {
        drm_intel_bo_unreference(buffers[i]);
    }
    drm_intel_bo_unreference(output_bo);
    drm_intel_gem_context_destroy(ctx);
    drm_intel_bufmgr_destroy(bufmgr);
    close(fd);
    
    return success ? 0 : 1;
}

// Made with Bob
