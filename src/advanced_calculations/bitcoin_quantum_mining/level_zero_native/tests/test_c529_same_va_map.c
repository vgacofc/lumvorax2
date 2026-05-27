// TEST C529 - MÊME VA MAP QUE OPENCL
// Stratégie: Utiliser EXACTEMENT les mêmes GPU VA qu'OpenCL
// Soft-pinning avec les offsets du log OpenCL

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

// Offsets GPU EXACTS du log OpenCL
static const uint64_t opencl_offsets[10] = {
    0x000078fa3814c000,  // buffer_00
    0xffff8001fffed000,  // buffer_01
    0x000078fa3801e000,  // buffer_02
    0x000078fa37d06000,  // buffer_03 (Surface State)
    0xffff8001fffdd000,  // buffer_04
    0x000078fa38152000,  // buffer_05
    0x000078fa35000000,  // buffer_06 (KERNEL!)
    0xffff8001fffee000,  // buffer_07
    0x000078fa3804f000,  // buffer_08 (IDRT)
    0x000078fa37cf5000   // buffer_09 (Batch)
};

int main(void) {
    printf("\n╔════════════════════════════════════════════════════════════════╗\n");
    printf("║  TEST C529 - MÊME VA MAP QUE OPENCL                         ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n\n");
    printf("Stratégie: Utiliser EXACTEMENT les mêmes GPU VA qu'OpenCL\n");
    printf("Soft-pinning avec offsets du log OpenCL\n\n");

    // [1/8] Ouvrir DRM
    printf("[1/8] Ouverture /dev/dri/renderD128...\n");
    int fd = open("/dev/dri/renderD128", O_RDWR);
    if (fd < 0) {
        printf("  ❌ Échec ouverture DRM\n");
        return 1;
    }
    printf("  ✓ DRM ouvert (fd=%d)\n\n", fd);

    // [2/8] Init libdrm_intel
    printf("[2/8] Initialisation libdrm_intel...\n");
    drm_intel_bufmgr *bufmgr = drm_intel_bufmgr_gem_init(fd, 4096);
    if (!bufmgr) {
        printf("  ❌ Échec init bufmgr\n");
        close(fd);
        return 1;
    }
    printf("  ✓ Buffer manager initialisé\n\n");

    // [3/8] Allocation des 10 buffers
    printf("[3/8] Allocation 10 buffers...\n");
    drm_intel_bo *buffers[10];
    
    for (int i = 0; i < 10; i++) {
        char name[32];
        snprintf(name, sizeof(name), "buffer_%02d", i);
        
        if (i == 9) {
            buffers[i] = drm_intel_bo_alloc(bufmgr, name, BATCH_SIZE, 4096);
        } else {
            buffers[i] = drm_intel_bo_alloc(bufmgr, name, BUFFER_SIZE, 4096);
        }
        
        if (!buffers[i]) {
            printf("  ❌ Échec allocation buffer %d\n", i);
            return 1;
        }
    }
    printf("  ✓ 10 buffers alloués\n\n");

    // [4/8] Charger les dumps OpenCL
    printf("[4/8] Chargement dumps OpenCL...\n");
    
    const char *dump_files[] = {
        "/tmp/opencl_batch_dump_buffer_00_0.bin",
        NULL,
        "/tmp/opencl_batch_dump_buffer_02_0.bin",
        "/tmp/opencl_batch_dump_buffer_03_0.bin",
        NULL,
        "/tmp/opencl_batch_dump_buffer_05_0.bin",
        "/tmp/opencl_batch_dump_buffer_06_0.bin",
        NULL,
        "/tmp/opencl_batch_dump_buffer_08_0.bin",
        "/tmp/opencl_batch_dump_batch_0.bin"
    };
    
    for (int i = 0; i < 10; i++) {
        if (dump_files[i] == NULL) {
            drm_intel_bo_map(buffers[i], 1);
            memset(buffers[i]->virtual, 0, i == 9 ? BATCH_SIZE : BUFFER_SIZE);
            drm_intel_bo_unmap(buffers[i]);
        } else {
            FILE *f = fopen(dump_files[i], "rb");
            if (!f) {
                printf("  ❌ Échec ouverture %s\n", dump_files[i]);
                return 1;
            }
            
            drm_intel_bo_map(buffers[i], 1);
            size_t size = i == 9 ? BATCH_SIZE : BUFFER_SIZE;
            fread(buffers[i]->virtual, 1, size, f);
            drm_intel_bo_unmap(buffers[i]);
            fclose(f);
        }
    }
    printf("  ✓ Buffers chargés\n\n");

    // [5/8] Créer output buffer
    printf("[5/8] Création output buffer...\n");
    drm_intel_bo *output_bo = drm_intel_bo_alloc(bufmgr, "output", BUFFER_SIZE, 4096);
    if (!output_bo) {
        printf("  ❌ Échec allocation output\n");
        return 1;
    }
    
    drm_intel_bo_map(output_bo, 1);
    uint32_t *output = (uint32_t*)output_bo->virtual;
    for (int i = 0; i < 1024; i++) output[i] = 0xFFFFFFFF;
    drm_intel_bo_unmap(output_bo);
    printf("  ✓ Output buffer créé\n\n");

    // [6/8] Créer context
    printf("[6/8] Création context...\n");
    drm_intel_context *ctx = drm_intel_gem_context_create(bufmgr);
    if (!ctx) {
        printf("  ❌ Échec création context\n");
        return 1;
    }
    uint32_t ctx_id;
    drm_intel_gem_context_get_id(ctx, &ctx_id);
    printf("  ✓ Context créé (id=%u)\n\n", ctx_id);

    // [7/8] Soumission avec MÊMES VA qu'OpenCL
    printf("[7/8] Soumission avec MÊMES VA qu'OpenCL...\n");
    
    struct drm_i915_gem_exec_object2 exec_objects[11];
    memset(exec_objects, 0, sizeof(exec_objects));
    
    uint64_t pinned_flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS | EXEC_OBJECT_PINNED;
    
    // Utiliser les offsets EXACTS d'OpenCL
    for (int i = 0; i < 10; i++) {
        exec_objects[i].handle = buffers[i]->handle;
        exec_objects[i].offset = opencl_offsets[i];
        exec_objects[i].flags = pinned_flags;
        printf("  buffer_%02d: 0x%016lx\n", i, opencl_offsets[i]);
    }
    
    // Output à une adresse libre
    exec_objects[10].handle = output_bo->handle;
    exec_objects[10].offset = 0x000078fa40000000;  // Adresse libre
    exec_objects[10].flags = pinned_flags | EXEC_OBJECT_WRITE;
    printf("  output:     0x%016lx\n\n", exec_objects[10].offset);
    
    // Patcher output address dans Surface State
    printf("  Patch output dans Surface State...\n");
    drm_intel_bo_map(buffers[3], 1);
    uint32_t *surface_heap = (uint32_t*)buffers[3]->virtual;
    uint32_t *surface_state = &surface_heap[16];
    surface_state[6] = (uint32_t)(exec_objects[10].offset & 0xFFFFFFFF);
    surface_state[7] = (uint32_t)(exec_objects[10].offset >> 32);
    drm_intel_bo_unmap(buffers[3]);
    printf("  ✓ Output patché\n\n");
    
    struct drm_i915_gem_execbuffer2 execbuf;
    memset(&execbuf, 0, sizeof(execbuf));
    execbuf.buffers_ptr = (uintptr_t)exec_objects;
    execbuf.buffer_count = 11;
    execbuf.batch_start_offset = 0;
    execbuf.batch_len = BATCH_SIZE;
    execbuf.flags = I915_EXEC_NO_RELOC;
    execbuf.rsvd1 = ctx_id;
    
    printf("  🔧 Soumission avec VA map OpenCL exacte...\n");
    int ret = drmIoctl(fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf);
    if (ret != 0) {
        printf("  ❌ EXECBUFFER2 failed (ret=%d, errno=%d: %s)\n", ret, errno, strerror(errno));
        return 1;
    }
    printf("  ✅ EXECBUFFER2 RÉUSSI!\n\n");

    // [8/8] Vérification
    printf("[8/8] Vérification résultat...\n");
    drm_intel_bo_wait_rendering(output_bo);
    
    drm_intel_bo_map(output_bo, 0);
    output = (uint32_t*)output_bo->virtual;
    
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("output[0] = 0x%08X (attendu: 0x12345678)\n", output[0]);
    printf("output[1] = 0x%08X\n", output[1]);
    printf("output[2] = 0x%08X\n", output[2]);
    printf("output[3] = 0x%08X\n", output[3]);
    
    int success = (output[0] == 0x12345678);
    drm_intel_bo_unmap(output_bo);
    
    printf("\n[RÉSULTAT FINAL]\n");
    if (success) {
        printf("  ✅✅✅ SUCCÈS! GPU NATIF FONCTIONNEL! ✅✅✅\n");
    } else {
        printf("  ⚠️  Échec - Le problème est dans le dataport write\n");
        printf("  💡 Suspects: BTI index, Surface State, ou kernel ISA\n");
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
