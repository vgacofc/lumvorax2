/**
 * TEST C596 - VALIDATION GÉNÉRATEUR AUTOMATIQUE i915
 * 
 * Premier test du système de génération automatique de batch buffers
 * basé sur le reverse engineering d'OpenCL (C545, C585).
 * 
 * Objectif: Valider l'infrastructure de base du générateur automatique
 * - Création automatique des 12 buffers avec soft-pinning
 * - Configuration automatique flags EXECBUFFER2
 * - Context i915 automatique
 * 
 * Date: 2026-05-26
 * Cycle: C596
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "../src/batch_generator_auto.h"

/**
 * Kernel minimal pour test (sera remplacé par kernel Bitcoin réel)
 */
static const uint32_t test_kernel_isa[] = {
    // Kernel ultra-minimal Gen9: write magic value
    0x00000000,  // Placeholder - sera généré par ocloc
    0x00000000,
    0x00000000,
    0x00000000
};

int main(int argc, char **argv) {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  TEST C596 - GÉNÉRATEUR AUTOMATIQUE i915 BATCH BUFFERS    ║\n");
    printf("║  Validation Infrastructure Automatique                     ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    
    int ret = 0;
    batch_generator_t gen = {0};
    
    // ========================================
    // PHASE 1: Ouverture DRM
    // ========================================
    printf("[TEST] Phase 1: Ouverture device DRM...\n");
    
    int drm_fd = open("/dev/dri/renderD128", O_RDWR);
    if (drm_fd < 0) {
        drm_fd = open("/dev/dri/card0", O_RDWR);
    }
    
    if (drm_fd < 0) {
        fprintf(stderr, "[TEST-ERROR] Impossible d'ouvrir DRM device: %s\n", strerror(errno));
        return 1;
    }
    
    printf("[TEST] ✓ DRM device ouvert: fd=%d\n\n", drm_fd);
    
    // ========================================
    // PHASE 2: Initialisation Générateur Automatique
    // ========================================
    printf("[TEST] Phase 2: Initialisation générateur automatique...\n");
    
    if (batch_generator_init(&gen, drm_fd) < 0) {
        fprintf(stderr, "[TEST-ERROR] Échec initialisation générateur\n");
        ret = 1;
        goto cleanup;
    }
    
    printf("[TEST] ✓ Générateur automatique initialisé\n\n");
    
    // ========================================
    // PHASE 3: Validation Configuration Automatique
    // ========================================
    printf("[TEST] Phase 3: Validation configuration automatique...\n");
    
    // Vérifier 12 buffers créés
    if (gen.buffer_count != OPENCL_BUFFER_COUNT) {
        fprintf(stderr, "[TEST-ERROR] Nombre de buffers incorrect: %u (attendu: %u)\n",
                gen.buffer_count, OPENCL_BUFFER_COUNT);
        ret = 1;
        goto cleanup;
    }
    printf("[TEST] ✓ 12 buffers créés (comme OpenCL)\n");
    
    // Vérifier flags automatiques
    if (gen.exec_flags != OPENCL_FLAGS) {
        fprintf(stderr, "[TEST-ERROR] Flags EXEC incorrects: 0x%lx (attendu: 0x%x)\n",
                gen.exec_flags, OPENCL_FLAGS);
        ret = 1;
        goto cleanup;
    }
    printf("[TEST] ✓ Flags EXEC: 0x%lx (I915_EXEC_NO_RELOC)\n", gen.exec_flags);
    
    // Vérifier object flags
    if (gen.object_flags != OPENCL_OBJECT_FLAGS) {
        fprintf(stderr, "[TEST-ERROR] Flags Object incorrects: 0x%x (attendu: 0x%x)\n",
                gen.object_flags, OPENCL_OBJECT_FLAGS);
        ret = 1;
        goto cleanup;
    }
    printf("[TEST] ✓ Flags Object: 0x%x (PINNED + 48B)\n", gen.object_flags);
    
    // Vérifier context i915
    if (gen.ctx_id == 0) {
        fprintf(stderr, "[TEST-ERROR] Context i915 non créé\n");
        ret = 1;
        goto cleanup;
    }
    printf("[TEST] ✓ Context i915: %u\n", gen.ctx_id);
    
    // Vérifier soft-pinning des buffers
    printf("[TEST] Validation soft-pinning des buffers:\n");
    for (uint32_t i = 0; i < gen.buffer_count; i++) {
        auto_buffer_t *buf = &gen.buffers[i];
        
        // Vérifier offset GTT non nul
        if (buf->offset == 0 && i != 0) {  // Batch peut être à 0
            fprintf(stderr, "[TEST-ERROR] Buffer %u (%s) offset GTT = 0\n",
                    i, buf->name);
            ret = 1;
            goto cleanup;
        }
        
        // Vérifier flags PINNED
        if ((buf->flags & 0x10) == 0) {
            fprintf(stderr, "[TEST-ERROR] Buffer %u (%s) pas PINNED (flags=0x%x)\n",
                    i, buf->name, buf->flags);
            ret = 1;
            goto cleanup;
        }
        
        // Vérifier mapping CPU
        if (buf->cpu_addr == NULL) {
            fprintf(stderr, "[TEST-ERROR] Buffer %u (%s) pas mappé CPU\n",
                    i, buf->name);
            ret = 1;
            goto cleanup;
        }
        
        printf("[TEST]   ✓ Buffer %2u: %-15s offset=0x%016lx flags=0x%x mapped=%p\n",
               i, buf->name, buf->offset, buf->flags, buf->cpu_addr);
    }
    
    printf("\n[TEST] ✓ Tous les buffers validés\n\n");
    
    // ========================================
    // PHASE 4: Configuration Kernel (Test)
    // ========================================
    printf("[TEST] Phase 4: Configuration kernel test...\n");
    
    if (batch_generator_set_kernel(&gen, 
                                    (void*)test_kernel_isa,
                                    sizeof(test_kernel_isa),
                                    256,  // global_work_size
                                    16) < 0) {  // local_work_size
        fprintf(stderr, "[TEST-ERROR] Échec configuration kernel\n");
        ret = 1;
        goto cleanup;
    }
    
    printf("[TEST] ✓ Kernel configuré\n\n");
    
    // ========================================
    // PHASE 5: Dump Configuration Finale
    // ========================================
    printf("[TEST] Phase 5: Dump configuration finale...\n");
    dump_execbuffer2_config(&gen);
    
    // ========================================
    // RÉSULTAT FINAL
    // ========================================
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  RÉSULTAT TEST C596                                        ║\n");
    printf("╠════════════════════════════════════════════════════════════╣\n");
    printf("║  ✓ Infrastructure générateur automatique: OK               ║\n");
    printf("║  ✓ 12 buffers avec soft-pinning: OK                       ║\n");
    printf("║  ✓ Flags automatiques (0x800, 0x18): OK                   ║\n");
    printf("║  ✓ Context i915 automatique: OK                           ║\n");
    printf("║                                                            ║\n");
    printf("║  Prochaines étapes (C597-C600):                           ║\n");
    printf("║  - Implémenter génération STATE_BASE_ADDRESS              ║\n");
    printf("║  - Implémenter génération MEDIA_VFE_STATE                 ║\n");
    printf("║  - Implémenter génération MEDIA_INTERFACE_DESCRIPTOR_LOAD ║\n");
    printf("║  - Implémenter génération GPGPU_WALKER                    ║\n");
    printf("║  - Implémenter génération PIPE_CONTROL                    ║\n");
    printf("║  - Implémenter batch_generator_generate() complet         ║\n");
    printf("║  - Implémenter batch_generator_submit()                   ║\n");
    printf("║  - Tests validation GPU                                   ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    
cleanup:
    batch_generator_cleanup(&gen);
    close(drm_fd);
    
    return ret;
}

// Made with Bob
