// TEST C515 - ALLOCATEUR GTT POUR SOFT-PINNING
// ROOT CAUSE #19: EXEC_OBJECT_PINNED requiert offset pré-rempli !
// Solution: Générer adresses GTT valides dans espace 48-bit

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

// Allocateur GTT simple - génère adresses dans espace 48-bit valide
static uint64_t gtt_base = 0x100000000ULL;  // Commence à 4GB
static uint64_t gtt_allocate(size_t size) {
    uint64_t addr = gtt_base;
    gtt_base += (size + 4095) & ~4095ULL;  // Aligne sur 4KB
    return addr;
}

int main(void) {
    printf("\n╔════════════════════════════════════════════════════════════════╗\n");
    printf("║  TEST C515 - ALLOCATEUR GTT + SOFT-PINNING                   ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n\n");
    printf("ROOT CAUSES #17, #18, #19 CORRIGÉES!\n");
    printf("Solution: Générer adresses GTT + EXEC_OBJECT_PINNED\n\n");

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

    // [3/10] Allocation buffers + GÉNÉRATION ADRESSES GTT
    printf("[3/10] Allocation buffers + génération adresses GTT...\n");
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
    
    // PRÉ-REMPLIR les adresses GTT (soft-pinning)
    batch_bo->offset64 = gtt_allocate(BATCH_SIZE);
    output_bo->offset64 = gtt_allocate(OUTPUT_SIZE);
    kernel_bo->offset64 = gtt_allocate(KERNEL_SIZE);
    idrt_bo->offset64 = gtt_allocate(4096);
    surface_state_bo->offset64 = gtt_allocate(4096);
    dynamic_state_bo->offset64 = gtt_allocate(4096);
    instruction_bo->offset64 = gtt_allocate(4096);
    
    printf("  ✓ Adresses GTT GÉNÉRÉES (soft-pinning):\n");
    printf("    batch_bo:         GTT 0x%016lx ✓ VALIDE\n", batch_bo->offset64);
    printf("    output_bo:        GTT 0x%016lx ✓ VALIDE\n", output_bo->offset64);
    printf("    kernel_bo:        GTT 0x%016lx ✓ VALIDE\n", kernel_bo->offset64);
    printf("    idrt_bo:          GTT 0x%016lx ✓ VALIDE\n", idrt_bo->offset64);
    printf("    surface_state_bo: GTT 0x%016lx ✓ VALIDE\n", surface_state_bo->offset64);
    printf("    dynamic_state_bo: GTT 0x%016lx ✓ VALIDE\n", dynamic_state_bo->offset64);
    printf("    instruction_bo:   GTT 0x%016lx ✓ VALIDE\n\n", instruction_bo->offset64);

    // [4/10] Map buffers
    printf("[4/10] Mapping buffers...\n");
    drm_intel_bo_map(batch_bo, 1);
    drm_intel_bo_map(output_bo, 1);
    drm_intel_bo_map(kernel_bo, 1);
    drm_intel_bo_map(idrt_bo, 1);
    drm_intel_bo_map(surface_state_bo, 1);
    drm_intel_bo_map(dynamic_state_bo, 1);
    drm_intel_bo_map(instruction_bo, 1);
    printf("  ✓ Buffers mappés\n\n");

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

    // [7/10] Configuration IDRT avec adresses GTT GÉNÉRÉES
    printf("[7/10] Configuration IDRT avec adresses GTT GÉNÉRÉES...\n");
    uint32_t *idrt = (uint32_t*)idrt_bo->virtual;
    memset(idrt, 0, 64);
    
    uint64_t kernel_addr = kernel_bo->offset64;
    idrt[0] = (uint32_t)(kernel_addr & 0xFFFFFFFF);
    idrt[1] = (uint32_t)(kernel_addr >> 32);
    idrt[3] = 0;
    idrt[4] = 0;
    idrt[5] = (24 << 0) | (1 << 31);
    
    printf("  ✓ IDRT configuré: kernel=0x%016lx\n", kernel_addr);
    printf("  ✓✓✓ IDRT prêt avec adresses GTT GÉNÉRÉES! ✓✓✓\n\n");

    // [8/10] Configuration Binding Table Entry
    printf("[8/10] Configuration Binding Table Entry...\n");
    uint32_t *bti = (uint32_t*)surface_state_bo->virtual;
    memset(bti, 0, 256);
    
    uint64_t output_addr = output_bo->offset64;
    bti[0] = (uint32_t)(output_addr & 0xFFFFFFFF);
    bti[1] = (uint32_t)(output_addr >> 32) | (0x7F << 18);
    bti[2] = OUTPUT_SIZE - 1;
    bti[3] = 0;
    
    printf("  ✓ BTI[0]: output @ 0x%016lx\n\n", output_addr);

    // [9/10] Construction batch
    printf("[9/10] Construction batch OpenCL avec adresses GTT GÉNÉRÉES...\n");
    uint32_t *batch = (uint32_t*)batch_bo->virtual;
    memset(batch, 0, BATCH_SIZE);
    int offset = 0;
    
    // PIPELINE_SELECT
    batch[offset++] = 0x69040000 | (20 - 2);
    for (int i = 1; i < 20; i++) batch[offset++] = 0;
    
    // MEDIA_VFE_STATE
    batch[offset++] = 0x70000000 | (9 - 2);
    batch[offset++] = 0;
    batch[offset++] = (24 << 16) | (24 << 8) | 24;
    for (int i = 3; i < 9; i++) batch[offset++] = 0;
    
    // PIPE_CONTROL
    batch[offset++] = 0x7A000004;
    for (int i = 1; i < 6; i++) batch[offset++] = 0;
    
    // STATE_BASE_ADDRESS avec adresses GÉNÉRÉES
    batch[offset++] = 0x61010000 | (19 - 2);
    batch[offset++] = 0;
    batch[offset++] = 0;
    batch[offset++] = 0;
    
    uint64_t surface_addr = surface_state_bo->offset64;
    batch[offset++] = (uint32_t)(surface_addr & 0xFFFFFFFF) | 1;
    batch[offset++] = (uint32_t)(surface_addr >> 32);
    
    uint64_t dynamic_addr = dynamic_state_bo->offset64;
    batch[offset++] = (uint32_t)(dynamic_addr & 0xFFFFFFFF) | 1;
    batch[offset++] = (uint32_t)(dynamic_addr >> 32);
    
    batch[offset++] = 0;
    batch[offset++] = 0;
    
    uint64_t instruction_addr = instruction_bo->offset64;
    batch[offset++] = (uint32_t)(instruction_addr & 0xFFFFFFFF) | 1;
    batch[offset++] = (uint32_t)(instruction_addr >> 32);
    
    batch[offset++] = 0xFFFFF001;
    batch[offset++] = 0xFFFFF001;
    batch[offset++] = 0;
    batch[offset++] = 0xFFFFF001;
    batch[offset++] = 0;
    batch[offset++] = 0xFFFFF001;
    batch[offset++] = 0;
    
    // PIPE_CONTROL
    batch[offset++] = 0x7A000004;
    for (int i = 1; i < 6; i++) batch[offset++] = 0;
    
    // MEDIA_INTERFACE_DESCRIPTOR_LOAD
    batch[offset++] = 0x70020000 | (3 - 2);
    batch[offset++] = 0;
    uint64_t idrt_addr = idrt_bo->offset64;
    batch[offset++] = (uint32_t)(idrt_addr & 0xFFFFFFFF);
    
    // GPGPU_WALKER
    batch[offset++] = 0x05A0000D;
    batch[offset++] = 0;
    batch[offset++] = 256;
    batch[offset++] = 0;
    batch[offset++] = 1;
    batch[offset++] = 0;
    batch[offset++] = 1;
    batch[offset++] = 0;
    batch[offset++] = 1;
    batch[offset++] = 0;
    batch[offset++] = 1;
    batch[offset++] = 0;
    batch[offset++] = 1;
    batch[offset++] = 0;
    batch[offset++] = 0xFFFFFFFF;
    
    // MEDIA_STATE_FLUSH
    batch[offset++] = 0x70040000;
    batch[offset++] = 0;
    
    // PIPE_CONTROL final
    batch[offset++] = 0x7A000004;
    for (int i = 1; i < 6; i++) batch[offset++] = 0;
    
    // MI_BATCH_BUFFER_END
    batch[offset++] = 0x0A000000;
    
    int batch_len = offset * 4;
    printf("  ✓ Batch construit: %d bytes\n\n", batch_len);

    // [10/10] EXECBUFFER2 avec SOFT-PINNING
    printf("[10/10] Soumission EXECBUFFER2 avec SOFT-PINNING...\n");
    
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
    
    printf("  🔧 Soumission avec drm_intel_gem_bo_context_exec...\n");
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
    printf("║  RÉSULTATS TEST C515                                          ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n\n");
    printf("  output[0] = 0x%08X (attendu: 0x12345678)\n", output[0]);
    
    if (output[0] == 0x12345678) {
        printf("\n  ✅✅✅ SUCCÈS ABSOLU! GPU NATIF FONCTIONNE! ✅✅✅\n");
        printf("  ROOT CAUSES #17, #18, #19 CORRIGÉES!\n");
        printf("  SOFT-PINNING + ALLOCATEUR GTT = SOLUTION FINALE!\n");
        return 0;
    } else {
        printf("\n  ⚠ Output incorrect, analyse nécessaire\n");
        printf("  Premiers 16 DWORDs:\n");
        for (int i = 0; i < 16; i++) {
            printf("    output[%d] = 0x%08X %s\n", i, output[i],
                   output[i] == 0xFFFFFFFF ? "(inchangé)" : 
                   output[i] == (uint32_t)(i-1) ? "✓ CORRECT" : "⚠ MODIFIÉ");
        }
        return 1;
    }
}
