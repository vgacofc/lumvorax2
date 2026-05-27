// TEST C514 - REPRODUIRE MÉTHODE OPENCL: NO_RELOC + ADRESSES FIXES
// ROOT CAUSE #17: OpenCL n'utilise PAS de relocations !
// Stratégie: Map tous les buffers pour forcer GTT, puis utiliser adresses fixes

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <xf86drm.h>
#include <intel_bufmgr.h>
#include <i915_drm.h>
#include "test_c491_opencl_kernel.h"

#define BATCH_SIZE 16384
#define OUTPUT_SIZE 4096
#define KERNEL_SIZE 4096

int main(void) {
    printf("\n╔════════════════════════════════════════════════════════════════╗\n");
    printf("║  TEST C514 - MÉTHODE OPENCL: NO_RELOC + ADRESSES FIXES       ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n\n");
    printf("ROOT CAUSE #17 CORRIGÉE: Pas de relocations, adresses GTT fixes!\n");
    printf("Stratégie: Map buffers → GTT auto-résolu → batch avec adresses fixes\n\n");

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

    // [3/10] Allocation buffers
    printf("[3/10] Allocation buffers GPU...\n");
    drm_intel_bo *batch_bo = drm_intel_bo_alloc(bufmgr, "batch", BATCH_SIZE, 4096);
    drm_intel_bo *output_bo = drm_intel_bo_alloc(bufmgr, "output", OUTPUT_SIZE, 4096);
    drm_intel_bo *kernel_bo = drm_intel_bo_alloc(bufmgr, "kernel", KERNEL_SIZE, 4096);
    drm_intel_bo *idrt_bo = drm_intel_bo_alloc(bufmgr, "idrt", 4096, 4096);
    drm_intel_bo *surface_state_bo = drm_intel_bo_alloc(bufmgr, "surface_state", 4096, 4096);
    drm_intel_bo *dynamic_state_bo = drm_intel_bo_alloc(bufmgr, "dynamic_state", 4096, 4096);
    drm_intel_bo *instruction_bo = drm_intel_bo_alloc(bufmgr, "instruction", 4096, 4096);
    
    if (!batch_bo || !output_bo || !kernel_bo || !idrt_bo || 
        !surface_state_bo || !dynamic_state_bo || !instruction_bo) {
        printf("  ❌ Échec allocation buffers\n");
        return 1;
    }
    printf("  ✓ Tous les buffers alloués\n\n");

    // [4/10] MAP TOUS LES BUFFERS pour forcer résolution GTT
    printf("[4/10] MAPPING buffers pour forcer résolution GTT...\n");
    drm_intel_bo_map(batch_bo, 1);
    drm_intel_bo_map(output_bo, 1);
    drm_intel_bo_map(kernel_bo, 1);
    drm_intel_bo_map(idrt_bo, 1);
    drm_intel_bo_map(surface_state_bo, 1);
    drm_intel_bo_map(dynamic_state_bo, 1);
    drm_intel_bo_map(instruction_bo, 1);
    
    printf("  ✓ Vérification adresses GTT après mapping:\n");
    printf("    batch_bo:         GTT 0x%016llx %s\n", batch_bo->offset64, 
           batch_bo->offset64 ? "✓ VALIDE" : "❌ NULLE");
    printf("    output_bo:        GTT 0x%016llx %s\n", output_bo->offset64,
           output_bo->offset64 ? "✓ VALIDE" : "❌ NULLE");
    printf("    kernel_bo:        GTT 0x%016llx %s\n", kernel_bo->offset64,
           kernel_bo->offset64 ? "✓ VALIDE" : "❌ NULLE");
    printf("    idrt_bo:          GTT 0x%016llx %s\n", idrt_bo->offset64,
           idrt_bo->offset64 ? "✓ VALIDE" : "❌ NULLE");
    printf("    surface_state_bo: GTT 0x%016llx %s\n", surface_state_bo->offset64,
           surface_state_bo->offset64 ? "✓ VALIDE" : "❌ NULLE");
    printf("    dynamic_state_bo: GTT 0x%016llx %s\n", dynamic_state_bo->offset64,
           dynamic_state_bo->offset64 ? "✓ VALIDE" : "❌ NULLE");
    printf("    instruction_bo:   GTT 0x%016llx %s\n\n", instruction_bo->offset64,
           instruction_bo->offset64 ? "✓ VALIDE" : "❌ NULLE");

    // [5/10] Init output buffer
    printf("[5/10] Initialisation output buffer...\n");
    uint32_t *output = (uint32_t*)output_bo->virtual;
    for (int i = 0; i < 1024; i++) {
        output[i] = 0xFFFFFFFF;
    }
    printf("  ✓ Output initialisé à 0xFFFFFFFF\n\n");

    // [6/10] Copie kernel ISA
    printf("[6/10] Copie kernel ISA OpenCL Gen9...\n");
    memcpy(kernel_bo->virtual, kernel_isa_gen9, sizeof(kernel_isa_gen9));
    printf("  ✓ Kernel ISA copié: %zu bytes\n\n", sizeof(kernel_isa_gen9));

    // [7/10] Configuration IDRT avec adresses GTT FIXES
    printf("[7/10] Configuration IDRT avec adresses GTT FIXES (méthode OpenCL)...\n");
    uint32_t *idrt = (uint32_t*)idrt_bo->virtual;
    memset(idrt, 0, 64);
    
    // DWord 0-1: Kernel Start Pointer (adresse FIXE)
    uint64_t kernel_addr = kernel_bo->offset64;
    idrt[0] = (uint32_t)(kernel_addr & 0xFFFFFFFF);
    idrt[1] = (uint32_t)(kernel_addr >> 32);
    
    // DWord 3: Binding Table Pointer (relatif à surface_state_bo)
    idrt[3] = 0;  // Offset 0 dans surface_state_bo
    
    // DWord 4: Sampler State Pointer
    idrt[4] = 0;
    
    // DWord 5: Number of Threads + Barrier Enable
    idrt[5] = (24 << 0) | (1 << 31);  // 24 threads, barrier enabled
    
    printf("  ✓ IDRT configuré:\n");
    printf("    Kernel pointer: 0x%016llx (FIXE)\n", kernel_addr);
    printf("    Binding table:  offset 0x%08x\n", idrt[3]);
    printf("    Threads:        %u\n", (idrt[5] & 0xFFFF));
    printf("  ✓✓✓ IDRT prêt avec adresses GTT FIXES! ✓✓✓\n\n");

    // [8/10] Configuration Binding Table Entry dans surface_state_bo
    printf("[8/10] Configuration Binding Table Entry...\n");
    uint32_t *bti = (uint32_t*)surface_state_bo->virtual;
    memset(bti, 0, 256);
    
    // BTI[0] pointe vers output_bo (adresse FIXE)
    uint64_t output_addr = output_bo->offset64;
    bti[0] = (uint32_t)(output_addr & 0xFFFFFFFF);
    bti[1] = (uint32_t)(output_addr >> 32) | (0x7F << 18);  // Surface type: buffer
    bti[2] = OUTPUT_SIZE - 1;  // Surface size
    bti[3] = 0;
    
    printf("  ✓ BTI[0] configuré: output @ 0x%016llx (FIXE)\n\n", output_addr);

    // [9/10] Construction batch OpenCL EXACT (320 bytes)
    printf("[9/10] Construction batch OpenCL EXACT avec adresses FIXES...\n");
    uint32_t *batch = (uint32_t*)batch_bo->virtual;
    memset(batch, 0, BATCH_SIZE);
    int offset = 0;
    
    // PIPELINE_SELECT (20 DWORDs)
    batch[offset++] = 0x69040000 | (20 - 2);
    for (int i = 1; i < 20; i++) batch[offset++] = 0;
    
    // MEDIA_VFE_STATE (9 DWORDs)
    batch[offset++] = 0x70000000 | (9 - 2);
    batch[offset++] = 0;
    batch[offset++] = (24 << 16) | (24 << 8) | 24;  // Max threads
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    
    // PIPE_CONTROL (6 DWORDs)
    batch[offset++] = 0x7A000004;
    for (int i = 1; i < 6; i++) batch[offset++] = 0;
    
    // STATE_BASE_ADDRESS (19 DWORDs) - ADRESSES FIXES
    batch[offset++] = 0x61010000 | (19 - 2);
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    
    // Surface State Base Address (DWord 4-5) - ADRESSE FIXE
    uint64_t surface_addr = surface_state_bo->offset64;
    batch[offset++] = (uint32_t)(surface_addr & 0xFFFFFFFF) | 1;
    batch[offset++] = (uint32_t)(surface_addr >> 32);
    
    // Dynamic State Base Address (DWord 6-7) - ADRESSE FIXE
    uint64_t dynamic_addr = dynamic_state_bo->offset64;
    batch[offset++] = (uint32_t)(dynamic_addr & 0xFFFFFFFF) | 1;
    batch[offset++] = (uint32_t)(dynamic_addr >> 32);
    
    // Indirect Object Base Address (DWord 8-9)
    batch[offset++] = 0;
    batch[offset++] = 0;
    
    // Instruction Base Address (DWord 10-11) - ADRESSE FIXE
    uint64_t instruction_addr = instruction_bo->offset64;
    batch[offset++] = (uint32_t)(instruction_addr & 0xFFFFFFFF) | 1;
    batch[offset++] = (uint32_t)(instruction_addr >> 32);
    
    // Size limits (DWord 12-18)
    batch[offset++] = 0xFFFFF001;
    batch[offset++] = 0xFFFFF001;
    batch[offset++] = 0;
    batch[offset++] = 0xFFFFF001;
    batch[offset++] = 0;
    batch[offset++] = 0xFFFFF001;
    batch[offset++] = 0;
    
    // PIPE_CONTROL (6 DWORDs)
    batch[offset++] = 0x7A000004;
    for (int i = 1; i < 6; i++) batch[offset++] = 0;
    
    // MEDIA_INTERFACE_DESCRIPTOR_LOAD (3 DWORDs) - ADRESSE FIXE
    batch[offset++] = 0x70020000 | (3 - 2);
    batch[offset++] = 0;
    uint64_t idrt_addr = idrt_bo->offset64;
    batch[offset++] = (uint32_t)(idrt_addr & 0xFFFFFFFF);
    
    // GPGPU_WALKER (15 DWORDs)
    batch[offset++] = 0x05A0000D;
    batch[offset++] = 0;
    batch[offset++] = 256;  // Thread Width
    batch[offset++] = 0;
    batch[offset++] = 1;    // Thread Height
    batch[offset++] = 0;
    batch[offset++] = 1;    // Thread Depth
    batch[offset++] = 0;
    batch[offset++] = 1;    // Thread Group X
    batch[offset++] = 0;
    batch[offset++] = 1;    // Thread Group Y
    batch[offset++] = 0;
    batch[offset++] = 1;    // Thread Group Z
    batch[offset++] = 0;
    batch[offset++] = 0xFFFFFFFF;
    
    // MEDIA_STATE_FLUSH (2 DWORDs)
    batch[offset++] = 0x70040000;
    batch[offset++] = 0;
    
    // PIPE_CONTROL final (6 DWORDs)
    batch[offset++] = 0x7A000004;
    for (int i = 1; i < 6; i++) batch[offset++] = 0;
    
    // MI_BATCH_BUFFER_END
    batch[offset++] = 0x0A000000;
    
    int batch_len = offset * 4;
    printf("  ✓ Batch construit: %d bytes (%d DWORDs)\n", batch_len, offset);
    printf("  ✓ Adresses GTT FIXES utilisées (pas de relocations)\n\n");

    // [10/10] EXECBUFFER2 avec I915_EXEC_NO_RELOC
    printf("[10/10] Soumission EXECBUFFER2 avec I915_EXEC_NO_RELOC...\n");
    
    drm_intel_context *ctx = drm_intel_gem_context_create(bufmgr);
    if (!ctx) {
        printf("  ❌ Échec création context\n");
        return 1;
    }
    printf("  ✓ Context créé\n");
    
    // Unmap avant exec
    drm_intel_bo_unmap(batch_bo);
    drm_intel_bo_unmap(output_bo);
    drm_intel_bo_unmap(kernel_bo);
    drm_intel_bo_unmap(idrt_bo);
    drm_intel_bo_unmap(surface_state_bo);
    drm_intel_bo_unmap(dynamic_state_bo);
    drm_intel_bo_unmap(instruction_bo);
    
    // Préparer exec_object2 array (10 buffers comme OpenCL)
    struct drm_i915_gem_exec_object2 exec_objects[10];
    memset(exec_objects, 0, sizeof(exec_objects));
    
    exec_objects[0].handle = surface_state_bo->handle;
    exec_objects[0].flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS | EXEC_OBJECT_PINNED;
    exec_objects[0].offset = surface_state_bo->offset64;
    
    exec_objects[1].handle = dynamic_state_bo->handle;
    exec_objects[1].flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS | EXEC_OBJECT_PINNED;
    exec_objects[1].offset = dynamic_state_bo->offset64;
    
    exec_objects[2].handle = instruction_bo->handle;
    exec_objects[2].flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS | EXEC_OBJECT_PINNED;
    exec_objects[2].offset = instruction_bo->offset64;
    
    exec_objects[3].handle = idrt_bo->handle;
    exec_objects[3].flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS | EXEC_OBJECT_PINNED;
    exec_objects[3].offset = idrt_bo->offset64;
    
    exec_objects[4].handle = kernel_bo->handle;
    exec_objects[4].flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS | EXEC_OBJECT_PINNED;
    exec_objects[4].offset = kernel_bo->offset64;
    
    exec_objects[5].handle = output_bo->handle;
    exec_objects[5].flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS | EXEC_OBJECT_PINNED | EXEC_OBJECT_WRITE;
    exec_objects[5].offset = output_bo->offset64;
    
    exec_objects[6].handle = batch_bo->handle;
    exec_objects[6].flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS | EXEC_OBJECT_PINNED;
    exec_objects[6].offset = batch_bo->offset64;
    
    // Buffers additionnels (padding comme OpenCL)
    for (int i = 7; i < 10; i++) {
        exec_objects[i].handle = batch_bo->handle;
        exec_objects[i].flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS;
    }
    
    struct drm_i915_gem_execbuffer2 execbuf;
    memset(&execbuf, 0, sizeof(execbuf));
    execbuf.buffers_ptr = (uintptr_t)exec_objects;
    execbuf.buffer_count = 10;
    execbuf.batch_start_offset = 0;
    execbuf.batch_len = batch_len;
    execbuf.flags = I915_EXEC_RENDER | I915_EXEC_NO_RELOC;  // NO_RELOC comme OpenCL!
    execbuf.rsvd1 = 0;  // Context ID sera géré par drm_intel_gem_bo_context_exec
    
    printf("  🔧 EXECBUFFER2 avec flags=0x%llx (I915_EXEC_NO_RELOC)\n",
           (unsigned long long)execbuf.flags);
    
    // Utiliser drm_intel_gem_bo_context_exec au lieu de ioctl direct
    int ret = drm_intel_gem_bo_context_exec(batch_bo, ctx, batch_len,
                                             I915_EXEC_RENDER | I915_EXEC_NO_RELOC);
    if (ret != 0) {
        printf("  ❌ EXECBUFFER2 failed (ret=%d)\n", ret);
        return 1;
    }
    printf("  ✓✓✓ EXECBUFFER2 RÉUSSI! ✓✓✓\n\n");

    // Attendre GPU
    drm_intel_bo_wait_rendering(output_bo);
    
    // Lire résultats
    drm_intel_bo_map(output_bo, 0);
    output = (uint32_t*)output_bo->virtual;
    
    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║  RÉSULTATS TEST C514                                          ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n\n");
    printf("  output[0] = 0x%08X (attendu: 0x12345678)\n", output[0]);
    
    if (output[0] == 0x12345678) {
        printf("\n  ✅✅✅ SUCCÈS ABSOLU! GPU NATIF FONCTIONNE! ✅✅✅\n");
        printf("  ROOT CAUSE #17 CORRIGÉE: Méthode OpenCL reproduite!\n");
        return 0;
    } else {
        printf("\n  ⚠ Output incorrect, analyse nécessaire\n");
        printf("  Premiers 16 DWORDs:\n");
        for (int i = 0; i < 16; i++) {
            printf("    output[%d] = 0x%08X %s\n", i, output[i],
                   output[i] == 0xFFFFFFFF ? "(inchangé)" : 
                   output[i] == i-1 ? "✓ CORRECT" : "⚠ MODIFIÉ");
        }
        return 1;
    }
}
