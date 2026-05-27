/**
 * CYCLE C605 - BRUTEFORCE GPGPU_WALKER CONFIGURATION
 * 
 * PROBLÈME IDENTIFIÉ:
 * - Nous testons avec des valeurs FIXES (Thread Group = 1x1x1)
 * - GPU accepte batch mais résultats = 0xDEADBEEF (inchangé)
 * - EU dispatch JAMAIS prouvé expérimentalement
 * 
 * SOLUTION:
 * - BRUTEFORCE systématique de TOUTES les configurations GPGPU_WALKER
 * - Tester Thread Group X, Y, Z dimensions
 * - Tester SIMD Size (SIMD8, SIMD16, SIMD32)
 * - Tester Interface Descriptor Offset
 * - Trouver LA configuration qui produit output[0] = 0x12345678
 * 
 * ROOT CAUSE #155: Configuration GPGPU_WALKER incorrecte
 * 
 * Référence: PROTOCOLE_TEST_CUMULATIF_DRM.md
 * Référence: Intel Gen9 Command Reference - GPGPU_WALKER
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>
#include <time.h>

#include "../src/batch_generator_auto.h"

// Kernel manuel ultra-simple (64 bytes)
// Écriture directe 0x12345678 sans dépendances CURBE
static const unsigned char kernel_manual_simple[] = {
    // mov (1) r1.0<1>:ud 0x12345678:ud
    0x00, 0x00, 0x20, 0x01,  // Opcode + control
    0x78, 0x56, 0x34, 0x12,  // Immediate value
    0x00, 0x00, 0x00, 0x00,  // Padding
    0x00, 0x00, 0x00, 0x00,
    
    // send (1) null r1 0x0 0x040A8000
    0x00, 0x00, 0x00, 0x31,  // Send opcode
    0x00, 0x80, 0x0A, 0x04,  // Message descriptor
    0x00, 0x00, 0x00, 0x00,  // Extended descriptor
    0x00, 0x00, 0x00, 0x00,
    
    // Padding to 64 bytes
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

/**
 * Générer GPGPU_WALKER avec configuration spécifique
 */
void generate_gpgpu_walker_custom(uint32_t *batch, uint32_t *idx,
                                   uint32_t thread_x, uint32_t thread_y, uint32_t thread_z,
                                   uint32_t simd_size, uint32_t idrt_offset) {
    // DW0: Opcode GPGPU_WALKER
    // Bits 31-29: Command Type (3 = 3D)
    // Bits 28-27: Command SubType (3 = 3D)
    // Bits 26-24: 3D Command Opcode (1 = GPGPU)
    // Bits 23-16: 3D Command Sub-Opcode (0x05 = GPGPU_WALKER)
    // Bits 15-8: DWord Length (15-2 = 13)
    // Bits 7-0: SIMD Size (0=SIMD8, 1=SIMD16, 2=SIMD32)
    uint32_t dw0 = 0x18800000 | ((13 & 0xFF) << 8) | (simd_size & 0x3);
    batch[(*idx)++] = dw0;
    
    // DW1: Interface Descriptor Offset
    batch[(*idx)++] = idrt_offset & 0x3F;  // 6-bit offset (DWords)
    
    // DW2: Indirect Data Length
    batch[(*idx)++] = 0x00000000;  // Pas de données indirectes
    
    // DW3: Indirect Data Start Address
    batch[(*idx)++] = 0x00000000;
    
    // DW4: Thread Group ID Starting X
    batch[(*idx)++] = 0x00000000;
    
    // DW5: Thread Group ID X Dimension
    batch[(*idx)++] = thread_x;
    
    // DW6: Thread Group ID Starting Y
    batch[(*idx)++] = 0x00000000;
    
    // DW7: Thread Group ID Y Dimension
    batch[(*idx)++] = thread_y;
    
    // DW8: Thread Group ID Starting Z
    batch[(*idx)++] = 0x00000000;
    
    // DW9: Thread Group ID Z Dimension
    batch[(*idx)++] = thread_z;
    
    // DW10: Right Execution Mask
    batch[(*idx)++] = 0xFFFFFFFF;
    
    // DW11: Bottom Execution Mask
    batch[(*idx)++] = 0xFFFFFFFF;
    
    // DW12-14: Reserved
    batch[(*idx)++] = 0x00000000;
    batch[(*idx)++] = 0x00000000;
    batch[(*idx)++] = 0x00000000;
}

/**
 * Tester une configuration GPGPU_WALKER spécifique
 */
int test_configuration(batch_generator_t *gen,
                       uint32_t thread_x, uint32_t thread_y, uint32_t thread_z,
                       uint32_t simd_size, uint32_t idrt_offset,
                       int *success_count) {
    // Réinitialiser output buffer
    uint32_t *output = (uint32_t *)gen->buffers[BUFFER_TYPE_OUTPUT].cpu_addr;
    for (int i = 0; i < 16; i++) {
        output[i] = 0xDEADBEEF;
    }
    
    // Générer batch avec configuration custom
    uint32_t *batch = gen->batch_data;
    uint32_t idx = 0;
    
    // 1. PIPELINE_SELECT
    batch[idx++] = 0x69041312;
    
    // 2. STATE_BASE_ADDRESS (19 DWords) - utiliser fonction existante
    uint32_t sba_start = idx;
    generate_state_base_address(gen, batch, &idx);
    
    // 3. MEDIA_VFE_STATE (9 DWords)
    generate_media_vfe_state(gen, batch, &idx);
    
    // 4. MEDIA_INTERFACE_DESCRIPTOR_LOAD (3 DWords)
    generate_media_interface_descriptor_load(gen, batch, &idx);
    
    // 5. GPGPU_WALKER (15 DWords) - CUSTOM
    generate_gpgpu_walker_custom(batch, &idx, thread_x, thread_y, thread_z,
                                  simd_size, idrt_offset);
    
    // 6. PIPE_CONTROL (6 DWords)
    generate_pipe_control(gen, batch, &idx);
    
    // 7. MI_BATCH_BUFFER_END
    batch[idx++] = 0x05000000;
    
    // Copier batch dans buffer GPU
    memcpy(gen->buffers[BUFFER_TYPE_BATCH].cpu_addr, batch, idx * 4);
    
    // Configurer EXECBUFFER2
    struct drm_i915_gem_execbuffer2 execbuf = {0};
    struct drm_i915_gem_exec_object2 exec_objects[12];
    for (int i = 0; i < 12; i++) {
        exec_objects[i].handle = gen->buffers[i].handle;
        exec_objects[i].relocation_count = 0;
        exec_objects[i].relocs_ptr = 0;
        exec_objects[i].alignment = 0;
        exec_objects[i].offset = gen->buffers[i].offset;
        exec_objects[i].flags = gen->buffers[i].flags;
        exec_objects[i].rsvd1 = 0;
        exec_objects[i].rsvd2 = 0;
    }
    execbuf.buffers_ptr = (uint64_t)(uintptr_t)exec_objects;
    execbuf.buffer_count = 12;
    execbuf.batch_len = idx * 4;
    execbuf.flags = gen->exec_flags;
    execbuf.rsvd1 = gen->ctx_id;
    
    // Exécuter
    if (ioctl(gen->drm_fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf) < 0) {
        return -1;  // EXECBUFFER2 échoué
    }
    
    // Attendre GPU
    usleep(1000);  // 1ms
    
    // Flush cache
    struct drm_i915_gem_set_domain set_domain = {
        .handle = gen->buffers[BUFFER_TYPE_OUTPUT].handle,
        .read_domains = I915_GEM_DOMAIN_CPU,
        .write_domain = I915_GEM_DOMAIN_CPU
    };
    ioctl(gen->drm_fd, DRM_IOCTL_I915_GEM_SET_DOMAIN, &set_domain);
    
    // Vérifier résultat
    if (output[0] == 0x12345678) {
        printf("✅ SUCCÈS! Configuration trouvée:\n");
        printf("   Thread Group: %ux%ux%u\n", thread_x, thread_y, thread_z);
        printf("   SIMD Size: %u\n", simd_size);
        printf("   IDRT Offset: %u\n", idrt_offset);
        printf("   Output[0]: 0x%08x\n", output[0]);
        (*success_count)++;
        return 1;
    }
    
    return 0;  // Pas de succès
}

int main(void) {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║                                                            ║\n");
    printf("║  TEST C605 - BRUTEFORCE GPGPU_WALKER                      ║\n");
    printf("║  Mini Compute Runtime i915 DRM Natif                      ║\n");
    printf("║                                                            ║\n");
    printf("║  Objectif: Trouver configuration GPGPU_WALKER valide      ║\n");
    printf("║  ROOT CAUSE #155: Configuration incorrecte                ║\n");
    printf("║                                                            ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");
    
    // Ouvrir DRM
    int drm_fd = open("/dev/dri/card1", O_RDWR);
    if (drm_fd < 0) {
        fprintf(stderr, "❌ Échec ouverture DRM\n");
        return 1;
    }
    printf("✓ DRM ouvert: /dev/dri/card1\n");
    
    // Initialiser générateur
    batch_generator_t gen;
    if (batch_generator_init(&gen, drm_fd) < 0) {
        fprintf(stderr, "❌ Échec initialisation générateur\n");
        close(drm_fd);
        return 1;
    }
    printf("✓ Générateur initialisé\n");
    
    // Copier kernel
    memcpy(gen.buffers[BUFFER_TYPE_KERNEL].cpu_addr, kernel_manual_simple,
           sizeof(kernel_manual_simple));
    printf("✓ Kernel copié (64 bytes)\n");
    
    // Configurer IDRT, Surface State, Binding Table
    configure_idrt(&gen);
    configure_surface_state(&gen, 0);
    configure_binding_table(&gen);
    printf("✓ Structures GPU configurées\n\n");
    
    // BRUTEFORCE SYSTÉMATIQUE
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  BRUTEFORCE GPGPU_WALKER CONFIGURATION                    ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");
    
    int total_tests = 0;
    int success_count = 0;
    time_t start_time = time(NULL);
    
    // Paramètres à tester
    uint32_t thread_x_values[] = {1, 2, 4, 8, 16, 32, 64, 128, 256};
    uint32_t thread_y_values[] = {1, 2, 4, 8, 16};
    uint32_t thread_z_values[] = {1, 2, 4};
    uint32_t simd_sizes[] = {0, 1, 2};  // SIMD8, SIMD16, SIMD32
    uint32_t idrt_offsets[] = {0, 1, 2, 4, 8, 16};
    
    int n_x = sizeof(thread_x_values) / sizeof(thread_x_values[0]);
    int n_y = sizeof(thread_y_values) / sizeof(thread_y_values[0]);
    int n_z = sizeof(thread_z_values) / sizeof(thread_z_values[0]);
    int n_simd = sizeof(simd_sizes) / sizeof(simd_sizes[0]);
    int n_idrt = sizeof(idrt_offsets) / sizeof(idrt_offsets[0]);
    
    printf("Espace de recherche:\n");
    printf("  Thread X: %d valeurs\n", n_x);
    printf("  Thread Y: %d valeurs\n", n_y);
    printf("  Thread Z: %d valeurs\n", n_z);
    printf("  SIMD Size: %d valeurs\n", n_simd);
    printf("  IDRT Offset: %d valeurs\n", n_idrt);
    printf("  Total combinaisons: %d\n\n", n_x * n_y * n_z * n_simd * n_idrt);
    
    printf("Début bruteforce...\n\n");
    
    // Boucles imbriquées pour tester toutes les combinaisons
    for (int ix = 0; ix < n_x; ix++) {
        for (int iy = 0; iy < n_y; iy++) {
            for (int iz = 0; iz < n_z; iz++) {
                for (int is = 0; is < n_simd; is++) {
                    for (int io = 0; io < n_idrt; io++) {
                        total_tests++;
                        
                        uint32_t tx = thread_x_values[ix];
                        uint32_t ty = thread_y_values[iy];
                        uint32_t tz = thread_z_values[iz];
                        uint32_t simd = simd_sizes[is];
                        uint32_t idrt = idrt_offsets[io];
                        
                        // Afficher progression tous les 100 tests
                        if (total_tests % 100 == 0) {
                            printf("[%d/%d] Testing: %ux%ux%u SIMD%d IDRT=%u\r",
                                   total_tests, n_x * n_y * n_z * n_simd * n_idrt,
                                   tx, ty, tz, (simd == 0 ? 8 : (simd == 1 ? 16 : 32)), idrt);
                            fflush(stdout);
                        }
                        
                        // Tester configuration
                        int result = test_configuration(&gen, tx, ty, tz, simd, idrt,
                                                        &success_count);
                        
                        if (result == 1) {
                            // Succès trouvé - continuer pour trouver toutes les configs valides
                            printf("\n");
                        }
                    }
                }
            }
        }
    }
    
    time_t end_time = time(NULL);
    
    printf("\n\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  RÉSULTATS BRUTEFORCE                                      ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");
    
    printf("Tests effectués: %d\n", total_tests);
    printf("Configurations réussies: %d\n", success_count);
    printf("Temps écoulé: %ld secondes\n", end_time - start_time);
    printf("Taux de réussite: %.2f%%\n", (success_count * 100.0) / total_tests);
    
    if (success_count == 0) {
        printf("\n❌ AUCUNE CONFIGURATION VALIDE TROUVÉE\n");
        printf("   → Problème plus profond que GPGPU_WALKER\n");
        printf("   → Vérifier IDRT, Surface State, Kernel ISA\n");
    } else {
        printf("\n✅ %d CONFIGURATION(S) VALIDE(S) TROUVÉE(S)\n", success_count);
        printf("   → Utiliser ces valeurs pour génération batch\n");
    }
    
    // Nettoyage
    batch_generator_cleanup(&gen);
    close(drm_fd);
    
    printf("\n✅ TEST C605 TERMINÉ\n\n");
    
    return (success_count > 0) ? 0 : 1;
}

// Made with Bob
