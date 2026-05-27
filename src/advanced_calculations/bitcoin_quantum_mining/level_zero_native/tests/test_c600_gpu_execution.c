/**
 * TEST C600 - VALIDATION GPU HARDWARE RÉELLE
 * 
 * Premier test d'exécution GPU réelle avec:
 * - Infrastructure batch generator automatique (C596-C599)
 * - Configuration IDRT, Surface State, Binding Table (C600)
 * - Kernel ISA simple validé (C396, C491)
 * 
 * Objectif: Obtenir le PREMIER résultat valide du GPU natif i915!
 * 
 * Date: 2026-05-26
 * Cycle: C600
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include "../src/batch_generator_auto.h"
// ROOT CAUSE #147 FIX: Utiliser kernel ISA OpenCL validé
#include "test_c601_opencl_kernel_isa_pure.h"

/**
 * Test 1: Configuration complète GPU
 * Valide que toutes les structures sont correctement configurées
 */
int test_1_configuration_complete(void) {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  TEST 1 - CONFIGURATION COMPLÈTE GPU                      ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");
    
    // Ouvrir DRM
    int drm_fd = open("/dev/dri/card1", O_RDWR);
    if (drm_fd < 0) {
        fprintf(stderr, "❌ Échec ouverture /dev/dri/card1\n");
        return -1;
    }
    printf("✓ DRM ouvert: /dev/dri/card1\n");
    
    // Initialiser générateur
    batch_generator_t gen;
    if (batch_generator_init(&gen, drm_fd) < 0) {
        fprintf(stderr, "❌ Échec initialisation générateur\n");
        close(drm_fd);
        return -1;
    }
    printf("✓ Générateur initialisé (12 buffers)\n");
    
    // ROOT CAUSE #147 FIX: Copier kernel ISA OpenCL validé
    auto_buffer_t *kernel_buf = &gen.buffers[BUFFER_TYPE_KERNEL];
    memcpy(kernel_buf->cpu_addr, kernel_c601_opencl_isa_pure, kernel_c601_opencl_isa_pure_size);
    printf("✓ Kernel ISA OpenCL PURE copié (%zu bytes)\n", kernel_c601_opencl_isa_pure_size);
    
    // Configurer IDRT
    if (configure_idrt(&gen) < 0) {
        fprintf(stderr, "❌ Échec configuration IDRT\n");
        batch_generator_cleanup(&gen);
        close(drm_fd);
        return -1;
    }
    printf("✓ IDRT configuré\n");
    
    // Configurer Surface State
    if (configure_surface_state(&gen, 0) < 0) {
        fprintf(stderr, "❌ Échec configuration Surface State\n");
        batch_generator_cleanup(&gen);
        close(drm_fd);
        return -1;
    }
    printf("✓ Surface State configuré\n");
    
    // Configurer Binding Table
    if (configure_binding_table(&gen) < 0) {
        fprintf(stderr, "❌ Échec configuration Binding Table\n");
        batch_generator_cleanup(&gen);
        close(drm_fd);
        return -1;
    }
    printf("✓ Binding Table configuré\n");
    
    // Vérifications
    auto_buffer_t *idrt_buf = &gen.buffers[BUFFER_TYPE_IDRT];
    uint32_t *idrt = (uint32_t *)idrt_buf->cpu_addr;
    
    // Vérifier kernel pointer dans IDRT
    uint64_t kernel_gtt = gen.buffers[BUFFER_TYPE_KERNEL].offset;
    uint64_t idrt_kernel_ptr = ((uint64_t)idrt[1] << 32) | (idrt[0] & 0xFFFFFFC0);
    assert(idrt_kernel_ptr == (kernel_gtt & 0xFFFFFFFFFFFFFFC0ULL));
    printf("✓ IDRT kernel pointer cohérent: 0x%016lx\n", idrt_kernel_ptr);
    
    // Vérifier Surface State
    auto_buffer_t *ss_buf = &gen.buffers[BUFFER_TYPE_SURFACE_STATE];
    uint32_t *ss = (uint32_t *)ss_buf->cpu_addr;
    uint64_t output_gtt = gen.buffers[BUFFER_TYPE_OUTPUT].offset;
    uint64_t ss_base = ((uint64_t)ss[2] << 32) | ss[1];
    assert(ss_base == output_gtt);
    printf("✓ Surface State base address cohérent: 0x%016lx\n", ss_base);
    
    // Vérifier Binding Table
    auto_buffer_t *bt_buf = &gen.buffers[BUFFER_TYPE_BINDING_TABLE];
    uint32_t *bt = (uint32_t *)bt_buf->cpu_addr;
    assert(bt[0] == 0x00000000);  // Première surface à offset 0
    printf("✓ Binding Table entry 0: 0x%08x\n", bt[0]);
    
    // Cleanup
    batch_generator_cleanup(&gen);
    close(drm_fd);
    
    printf("\n✅ TEST 1 RÉUSSI - Configuration complète validée\n");
    return 0;
}

/**
 * Test 2: Génération batch avec kernel
 * Valide que le batch est correctement généré avec toutes les structures
 */
int test_2_generation_batch_kernel(void) {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  TEST 2 - GÉNÉRATION BATCH AVEC KERNEL                    ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");
    
    // Ouvrir DRM
    int drm_fd = open("/dev/dri/card1", O_RDWR);
    if (drm_fd < 0) {
        fprintf(stderr, "❌ Échec ouverture /dev/dri/card1\n");
        return -1;
    }
    
    // Initialiser générateur
    batch_generator_t gen;
    if (batch_generator_init(&gen, drm_fd) < 0) {
        fprintf(stderr, "❌ Échec initialisation générateur\n");
        close(drm_fd);
        return -1;
    }
    
    // Copier kernel + configurer structures
    auto_buffer_t *kernel_buf = &gen.buffers[BUFFER_TYPE_KERNEL];
    memcpy(kernel_buf->cpu_addr, kernel_c601_opencl_isa_pure, kernel_c601_opencl_isa_pure_size);
    configure_idrt(&gen);
    configure_surface_state(&gen, 0);
    configure_binding_table(&gen);
    
    // Générer batch
    if (batch_generator_generate(&gen) < 0) {
        fprintf(stderr, "❌ Échec génération batch\n");
        batch_generator_cleanup(&gen);
        close(drm_fd);
        return -1;
    }
    printf("✓ Batch généré: %u DWords (%u bytes)\n", gen.batch_dwords, gen.batch_dwords * 4);
    
    // Vérifications batch
    uint32_t *batch = gen.batch_data;
    
    // Vérifier PIPELINE_SELECT
    assert(batch[0] == 0x69041312);
    printf("✓ PIPELINE_SELECT: 0x%08x\n", batch[0]);
    
    // Vérifier STATE_BASE_ADDRESS opcode
    assert(batch[1] == 0x61010011);
    printf("✓ STATE_BASE_ADDRESS: 0x%08x\n", batch[1]);
    
    // Vérifier MEDIA_VFE_STATE opcode
    assert(batch[20] == 0x70000007);
    printf("✓ MEDIA_VFE_STATE: 0x%08x\n", batch[20]);
    
    // Vérifier MEDIA_IDL opcode
    assert(batch[29] == 0x61020001);
    printf("✓ MEDIA_IDL: 0x%08x\n", batch[29]);
    
    // Vérifier GPGPU_WALKER opcode
    assert((batch[32] & 0xFFFF0000) == 0x18800000);
    printf("✓ GPGPU_WALKER: 0x%08x\n", batch[32]);
    
    // Vérifier MI_BATCH_BUFFER_END
    assert(batch[47] == 0x05000000);
    printf("✓ MI_BATCH_BUFFER_END: 0x%08x\n", batch[47]);
    
    // Cleanup
    batch_generator_cleanup(&gen);
    close(drm_fd);
    
    printf("\n✅ TEST 2 RÉUSSI - Batch généré correctement\n");
    return 0;
}

/**
 * Test 3: EXÉCUTION GPU RÉELLE
 * LE TEST CRITIQUE - Premier résultat GPU natif i915!
 */
int test_3_execution_gpu_reelle(void) {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  TEST 3 - EXÉCUTION GPU RÉELLE                            ║\n");
    printf("║  🎯 OBJECTIF: PREMIER RÉSULTAT GPU NATIF i915!            ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");
    
    // Ouvrir DRM
    int drm_fd = open("/dev/dri/card1", O_RDWR);
    if (drm_fd < 0) {
        fprintf(stderr, "❌ Échec ouverture /dev/dri/card1\n");
        return -1;
    }
    printf("✓ DRM ouvert: /dev/dri/card1\n");
    
    // Initialiser générateur
    batch_generator_t gen;
    if (batch_generator_init(&gen, drm_fd) < 0) {
        fprintf(stderr, "❌ Échec initialisation générateur\n");
        close(drm_fd);
        return -1;
    }
    printf("✓ Générateur initialisé\n");
    
    // Initialiser output buffer avec pattern reconnaissable
    auto_buffer_t *output_buf = &gen.buffers[BUFFER_TYPE_OUTPUT];
    uint32_t *output = (uint32_t *)output_buf->cpu_addr;
    for (int i = 0; i < 16; i++) {
        output[i] = 0xDEADBEEF;  // Pattern initial
    }
    printf("✓ Output buffer initialisé (pattern 0xDEADBEEF)\n");
    
    // ROOT CAUSE #147 FIX: Copier kernel ISA OpenCL validé
    auto_buffer_t *kernel_buf = &gen.buffers[BUFFER_TYPE_KERNEL];
    memcpy(kernel_buf->cpu_addr, kernel_c601_opencl_isa_pure, kernel_c601_opencl_isa_pure_size);
    printf("✓ Kernel ISA OpenCL PURE copié (%zu bytes)\n", kernel_c601_opencl_isa_pure_size);
    
    // Configurer structures GPU
    printf("\n[CONFIGURATION GPU]\n");
    if (configure_idrt(&gen) < 0) {
        fprintf(stderr, "❌ Échec configuration IDRT\n");
        batch_generator_cleanup(&gen);
        close(drm_fd);
        return -1;
    }
    
    if (configure_surface_state(&gen, 0) < 0) {
        fprintf(stderr, "❌ Échec configuration Surface State\n");
        batch_generator_cleanup(&gen);
        close(drm_fd);
        return -1;
    }
    
    if (configure_binding_table(&gen) < 0) {
        fprintf(stderr, "❌ Échec configuration Binding Table\n");
        batch_generator_cleanup(&gen);
        close(drm_fd);
        return -1;
    }
    
    // Générer batch
    printf("\n[GÉNÉRATION BATCH]\n");
    if (batch_generator_generate(&gen) < 0) {
        fprintf(stderr, "❌ Échec génération batch\n");
        batch_generator_cleanup(&gen);
        close(drm_fd);
        return -1;
    }
    
    // Soumettre au GPU
    printf("\n[SOUMISSION GPU]\n");
    if (batch_generator_submit(&gen) < 0) {
        fprintf(stderr, "❌ Échec soumission GPU\n");
        batch_generator_cleanup(&gen);
        close(drm_fd);
        return -1;
    }
    
    // Attendre GPU
    printf("\n[ATTENTE GPU]\n");
    if (batch_generator_wait(&gen) < 0) {
        fprintf(stderr, "❌ Échec attente GPU\n");
        batch_generator_cleanup(&gen);
        close(drm_fd);
        return -1;
    }
    
    // Lire résultats
    printf("\n[LECTURE RÉSULTATS]\n");
    uint32_t results[16];
    if (batch_generator_read_results(&gen, results, sizeof(results)) < 0) {
        fprintf(stderr, "❌ Échec lecture résultats\n");
        batch_generator_cleanup(&gen);
        close(drm_fd);
        return -1;
    }
    
    // Afficher résultats
    printf("\n╔════════════════════════════════════════════════════════════╗\n");
    printf("║  RÉSULTATS GPU                                             ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");
    
    printf("Output buffer (16 premiers DWords):\n");
    for (int i = 0; i < 16; i++) {
        printf("  output[%2d] = 0x%08x", i, results[i]);
        if (results[i] == 0x12345678) {
            printf(" ✅ CORRECT!");
        } else if (results[i] == 0xDEADBEEF) {
            printf(" ⚠️  Inchangé (pattern initial)");
        } else if (results[i] == 0x00000000) {
            printf(" ❌ Zéro (GPU n'a pas écrit)");
        }
        printf("\n");
    }
    
    // Vérification finale
    printf("\n");
    if (results[0] == 0x12345678) {
        printf("╔════════════════════════════════════════════════════════════╗\n");
        printf("║  🎉🎉🎉 SUCCÈS TOTAL! 🎉🎉🎉                              ║\n");
        printf("║                                                            ║\n");
        printf("║  PREMIER RÉSULTAT VALIDE GPU NATIF i915!                  ║\n");
        printf("║  Valeur attendue: 0x%08x                              ║\n", 0x12345678);
        printf("║  Valeur obtenue:  0x%08x                              ║\n", results[0]);
        printf("║                                                            ║\n");
        printf("║  Le GPU a correctement exécuté le kernel!                 ║\n");
        printf("╚════════════════════════════════════════════════════════════╝\n");
    } else {
        printf("╔════════════════════════════════════════════════════════════╗\n");
        printf("║  ⚠️  RÉSULTAT INATTENDU                                   ║\n");
        printf("║                                                            ║\n");
        printf("║  Valeur attendue: 0x%08x                              ║\n", 0x12345678);
        printf("║  Valeur obtenue:  0x%08x                              ║\n", results[0]);
        printf("║                                                            ║\n");
        printf("║  Le GPU a exécuté mais résultat différent                 ║\n");
        printf("╚════════════════════════════════════════════════════════════╝\n");
    }
    
    // Cleanup
    batch_generator_cleanup(&gen);
    close(drm_fd);
    
    printf("\n✅ TEST 3 TERMINÉ - Exécution GPU complète\n");
    return (results[0] == 0x12345678) ? 0 : 1;
}

/**
 * Main - Exécuter tous les tests
 */
int main(void) {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║                                                            ║\n");
    printf("║  TEST C600 - VALIDATION GPU HARDWARE RÉELLE               ║\n");
    printf("║  Mini Compute Runtime i915 DRM Natif                      ║\n");
    printf("║                                                            ║\n");
    printf("║  Objectif: Premier résultat valide GPU natif i915!        ║\n");
    printf("║                                                            ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    
    int result = 0;
    
    // Test 1: Configuration complète
    if (test_1_configuration_complete() < 0) {
        fprintf(stderr, "\n❌ TEST 1 ÉCHOUÉ\n");
        result = -1;
    }
    
    // Test 2: Génération batch
    if (result == 0 && test_2_generation_batch_kernel() < 0) {
        fprintf(stderr, "\n❌ TEST 2 ÉCHOUÉ\n");
        result = -1;
    }
    
    // Test 3: Exécution GPU réelle
    if (result == 0) {
        int gpu_result = test_3_execution_gpu_reelle();
        if (gpu_result < 0) {
            fprintf(stderr, "\n❌ TEST 3 ÉCHOUÉ (erreur système)\n");
            result = -1;
        } else if (gpu_result > 0) {
            fprintf(stderr, "\n⚠️  TEST 3 PARTIEL (résultat inattendu)\n");
            result = 1;
        }
    }
    
    // Résumé final
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  RÉSUMÉ TESTS C600                                         ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    if (result == 0) {
        printf("✅ TOUS LES TESTS RÉUSSIS (3/3)\n");
        printf("🎉 PREMIER RÉSULTAT VALIDE GPU NATIF i915!\n");
    } else if (result == 1) {
        printf("⚠️  TESTS PARTIELS (GPU exécuté, résultat différent)\n");
    } else {
        printf("❌ ÉCHEC TESTS\n");
    }
    printf("\n");
    
    return result;
}

// Made with Bob