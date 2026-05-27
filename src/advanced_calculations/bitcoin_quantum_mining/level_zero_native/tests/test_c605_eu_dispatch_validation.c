/**
 * CYCLE C605 - VALIDATION EU DISPATCH HARDWARE
 * 
 * Test pour prouver expérimentalement si les EUs sont dispatchés
 * 
 * MÉTHODE:
 * 1. Utiliser /sys/kernel/debug/dri/1/i915_engine_info
 * 2. Capturer état GPU avant/après exécution
 * 3. Comparer compteurs hardware (busy time, requests, etc.)
 * 4. Si compteurs identiques → EUs NON dispatchés
 * 5. Si compteurs différents → EUs dispatchés (problème kernel/mémoire)
 * 
 * ROOT CAUSE #155: Validation EU dispatch manquante
 * 
 * Référence: PROTOCOLE_TEST_CUMULATIF_DRM.md TEST_057
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

typedef struct {
    uint64_t busy_ns;
    uint64_t total_ns;
    uint32_t requests;
    uint32_t interrupts;
} gpu_metrics_t;

/**
 * Lire métriques GPU depuis sysfs/debugfs
 */
int read_gpu_metrics(gpu_metrics_t *metrics) {
    if (!metrics) return -1;
    
    memset(metrics, 0, sizeof(gpu_metrics_t));
    
    // Essayer /sys/kernel/debug/dri/1/i915_engine_info
    FILE *f = fopen("/sys/kernel/debug/dri/1/i915_engine_info", "r");
    if (!f) {
        // Fallback: /sys/class/drm/card1/engine/rcs0/busy_ns
        f = fopen("/sys/class/drm/card1/engine/rcs0/busy_ns", "r");
        if (!f) {
            fprintf(stderr, "⚠️  Impossible de lire métriques GPU (permissions?)\n");
            return -1;
        }
        
        if (fscanf(f, "%lu", &metrics->busy_ns) != 1) {
            fclose(f);
            return -1;
        }
        fclose(f);
        return 0;
    }
    
    // Parser i915_engine_info
    char line[256];
    int in_rcs0 = 0;
    
    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, "rcs0")) {
            in_rcs0 = 1;
        }
        
        if (in_rcs0) {
            if (sscanf(line, " busy: %lu", &metrics->busy_ns) == 1) {
                continue;
            }
            if (sscanf(line, " total: %lu", &metrics->total_ns) == 1) {
                continue;
            }
            if (sscanf(line, " requests: %u", &metrics->requests) == 1) {
                continue;
            }
            if (sscanf(line, " interrupts: %u", &metrics->interrupts) == 1) {
                break;  // Fin section rcs0
            }
        }
    }
    
    fclose(f);
    return 0;
}

/**
 * Afficher différence métriques
 */
void print_metrics_diff(const char *label,
                        const gpu_metrics_t *before,
                        const gpu_metrics_t *after) {
    printf("\n[MÉTRIQUES] %s\n", label);
    printf("  Busy time:   %lu ns → %lu ns (Δ = %ld ns)\n",
           before->busy_ns, after->busy_ns,
           (int64_t)(after->busy_ns - before->busy_ns));
    printf("  Total time:  %lu ns → %lu ns (Δ = %ld ns)\n",
           before->total_ns, after->total_ns,
           (int64_t)(after->total_ns - before->total_ns));
    printf("  Requests:    %u → %u (Δ = %d)\n",
           before->requests, after->requests,
           (int32_t)(after->requests - before->requests));
    printf("  Interrupts:  %u → %u (Δ = %d)\n",
           before->interrupts, after->interrupts,
           (int32_t)(after->interrupts - before->interrupts));
    
    // Analyse
    int64_t busy_delta = (int64_t)(after->busy_ns - before->busy_ns);
    int32_t req_delta = (int32_t)(after->requests - before->requests);
    
    printf("\n[ANALYSE]\n");
    if (busy_delta == 0 && req_delta == 0) {
        printf("  ❌ GPU INACTIF - EUs NON dispatchés\n");
        printf("  → Problème infrastructure batch/IDRT/dispatch\n");
    } else if (busy_delta > 0 && req_delta > 0) {
        printf("  ✅ GPU ACTIF - EUs dispatchés\n");
        printf("  → Problème dans kernel ISA ou accès mémoire\n");
    } else {
        printf("  ⚠️  État ambigu - Analyse manuelle requise\n");
    }
}

int main(void) {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║                                                            ║\n");
    printf("║  TEST C605 - VALIDATION EU DISPATCH HARDWARE              ║\n");
    printf("║  Mini Compute Runtime i915 DRM Natif                      ║\n");
    printf("║                                                            ║\n");
    printf("║  Objectif: Prouver si EUs sont réellement dispatchés      ║\n");
    printf("║  ROOT CAUSE #155: Validation EU dispatch manquante        ║\n");
    printf("║                                                            ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");
    
    // Ouvrir DRM
    int drm_fd = drm_open_device();
    if (drm_fd < 0) {
        fprintf(stderr, "❌ Échec ouverture DRM\n");
        return 1;
    }
    printf("✓ DRM ouvert: /dev/dri/card1\n");
    
    // Initialiser générateur automatique
    batch_generator_t gen;
    if (batch_generator_init(&gen, drm_fd) < 0) {
        fprintf(stderr, "❌ Échec initialisation générateur\n");
        close(drm_fd);
        return 1;
    }
    printf("✓ Générateur initialisé\n");
    
    // Initialiser output buffer
    uint32_t *output = (uint32_t *)gen.buffers[BUFFER_OUTPUT].map;
    for (int i = 0; i < 16; i++) {
        output[i] = 0xDEADBEEF;
    }
    printf("✓ Output buffer initialisé (pattern 0xDEADBEEF)\n");
    
    // Copier kernel
    memcpy(gen.buffers[BUFFER_KERNEL].map, kernel_manual_simple,
           sizeof(kernel_manual_simple));
    printf("✓ Kernel MANUEL SIMPLE copié (64 bytes)\n");
    
    // Configurer IDRT
    batch_generator_configure_idrt(&gen, 0, 1);
    printf("✓ IDRT configuré\n");
    
    // Configurer Surface State
    batch_generator_configure_surface_state(&gen, 0,
        gen.buffers[BUFFER_OUTPUT].offset, 4096);
    printf("✓ Surface State configuré\n");
    
    // Configurer Binding Table
    batch_generator_configure_binding_table(&gen, 0, 0);
    printf("✓ Binding Table configuré\n");
    
    // Générer batch
    if (batch_generator_generate(&gen) < 0) {
        fprintf(stderr, "❌ Échec génération batch\n");
        batch_generator_cleanup(&gen);
        close(drm_fd);
        return 1;
    }
    printf("✓ Batch généré\n");
    
    // MESURE 1: Métriques AVANT exécution
    gpu_metrics_t metrics_before;
    printf("\n[CAPTURE] Métriques GPU AVANT exécution...\n");
    if (read_gpu_metrics(&metrics_before) < 0) {
        printf("⚠️  Impossible de lire métriques (continuer sans validation hardware)\n");
        metrics_before.busy_ns = 0;
        metrics_before.total_ns = 0;
        metrics_before.requests = 0;
        metrics_before.interrupts = 0;
    } else {
        printf("✓ Métriques capturées\n");
    }
    
    // Exécuter sur GPU
    printf("\n[EXÉCUTION] Soumission batch au GPU...\n");
    if (batch_generator_execute(&gen) < 0) {
        fprintf(stderr, "❌ Échec exécution GPU\n");
        batch_generator_cleanup(&gen);
        close(drm_fd);
        return 1;
    }
    printf("✓ EXECBUFFER2 accepté\n");
    
    // Attendre GPU
    printf("[ATTENTE] Attente fin exécution GPU...\n");
    usleep(100000);  // 100ms pour être sûr
    printf("✓ GPU terminé\n");
    
    // MESURE 2: Métriques APRÈS exécution
    gpu_metrics_t metrics_after;
    printf("\n[CAPTURE] Métriques GPU APRÈS exécution...\n");
    if (read_gpu_metrics(&metrics_after) < 0) {
        printf("⚠️  Impossible de lire métriques\n");
        metrics_after = metrics_before;
    } else {
        printf("✓ Métriques capturées\n");
    }
    
    // Flush cache GPU→CPU
    if (drm_set_domain(drm_fd, gen.buffers[BUFFER_OUTPUT].handle,
                       I915_GEM_DOMAIN_CPU, I915_GEM_DOMAIN_CPU) < 0) {
        fprintf(stderr, "⚠️  Échec flush cache\n");
    }
    
    // Lire résultats
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  RÉSULTATS GPU                                             ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");
    
    printf("Output buffer (16 premiers DWords):\n");
    int all_deadbeef = 1;
    for (int i = 0; i < 16; i++) {
        printf("  output[%2d] = 0x%08x", i, output[i]);
        if (output[i] == 0x12345678) {
            printf(" ✅ SUCCÈS!\n");
            all_deadbeef = 0;
        } else if (output[i] == 0xDEADBEEF) {
            printf(" ⚠️  Inchangé\n");
        } else {
            printf(" ⚠️  Valeur inattendue\n");
            all_deadbeef = 0;
        }
    }
    
    // Afficher différence métriques
    print_metrics_diff("Différence AVANT/APRÈS", &metrics_before, &metrics_after);
    
    // Conclusion
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  CONCLUSION TEST C605                                      ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");
    
    int64_t busy_delta = (int64_t)(metrics_after.busy_ns - metrics_before.busy_ns);
    int32_t req_delta = (int32_t)(metrics_after.requests - metrics_before.requests);
    
    if (all_deadbeef && busy_delta == 0 && req_delta == 0) {
        printf("❌ ÉCHEC CRITIQUE: EUs NON dispatchés\n");
        printf("   → Résultats inchangés (0xDEADBEEF)\n");
        printf("   → Métriques GPU inchangées\n");
        printf("   → Problème: Infrastructure batch/IDRT/dispatch\n");
        printf("   → Prochaine étape: Analyser GPGPU_WALKER configuration\n");
    } else if (!all_deadbeef && busy_delta > 0) {
        printf("✅ SUCCÈS PARTIEL: EUs dispatchés\n");
        printf("   → Métriques GPU changées (GPU actif)\n");
        printf("   → Résultats modifiés\n");
        printf("   → Problème: Kernel ISA ou accès mémoire\n");
        printf("   → Prochaine étape: Débugger kernel ISA\n");
    } else if (all_deadbeef && busy_delta > 0) {
        printf("⚠️  ÉTAT AMBIGU: GPU actif mais résultats inchangés\n");
        printf("   → Métriques GPU changées\n");
        printf("   → Résultats inchangés (0xDEADBEEF)\n");
        printf("   → Problème: Cache coherency ou timing\n");
        printf("   → Prochaine étape: Analyser PIPE_CONTROL et cache\n");
    } else {
        printf("⚠️  RÉSULTAT INATTENDU\n");
        printf("   → Analyse manuelle requise\n");
    }
    
    // Nettoyage
    batch_generator_cleanup(&gen);
    close(drm_fd);
    
    printf("\n✅ TEST C605 TERMINÉ\n\n");
    
    return (all_deadbeef && busy_delta == 0) ? 1 : 0;
}

// Made with Bob
