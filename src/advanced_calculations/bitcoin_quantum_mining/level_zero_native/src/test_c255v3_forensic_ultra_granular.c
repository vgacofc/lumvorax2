/**
 * LumVorax C255v3 — Test Forensique Ultra-Granulaire
 * 
 * OBJECTIF: Comprendre i915 DRM à 100% bit par bit
 * - Traçabilité TOTALE de tous les ioctl
 * - Dump hexadécimal de TOUS les buffers
 * - Timeline nanoseconde de TOUS les événements
 * - Analyse COMPLÈTE des dépendances GEM
 * 
 * AUCUNE LIMITE:
 * - Taille logs: illimitée (plusieurs GB acceptés)
 * - Latence: acceptée (priorité = traçabilité)
 * - Verbosité: maximale
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>

/* Activer tracking forensique AVANT includes DRM */
#define ENABLE_I915_FORENSIC_TRACKING
#include "btc_i915_drm_forensic_tracker.h"

/* Includes DRM (ioctl sera remplacé par version forensique) */
#include <drm/i915_drm.h>
#include <xf86drm.h>

#include "btc_gen9_native_runner.h"
#include "btc_gen9_validation_tests.h"

/**
 * Test forensique: 1 dispatch avec traçabilité maximale
 */
int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    
    printf("# LumVorax C255v3 — Test Forensique Ultra-Granulaire\n");
    printf("# Objectif: Comprendre i915 DRM à 100%% bit par bit\n");
    printf("# Traçabilité: MAXIMALE (aucune limite taille/latence)\n\n");
    
    /* Initialiser tracker forensique */
    printf("[FORENSIC] Initialisation tracker...\n");
    if (i915_forensic_tracker_init("logs/forensic") < 0) {
        fprintf(stderr, "ERREUR: Impossible initialiser tracker forensique\n");
        return 1;
    }
    printf("[FORENSIC] Tracker initialisé avec succès\n");
    printf("[FORENSIC] Logs: logs/forensic/i915_forensic_*.log\n\n");
    
    /* Initialiser contexte Gen9 */
    printf("[TEST] Initialisation contexte Gen9...\n");
    btc_gen9_config_t config = BTC_GEN9_DEFAULT_CONFIG;
    config.batch_size = 262144;  /* 256K nonces (éviter GPU hang) */
    config.work_group_size = 256;
    config.kernel_path = "kernels/btc_sha256_gen9.bin";  /* Kernel existant */
    
    btc_gen9_context_t* ctx = NULL;
    if (btc_gen9_init(&ctx, &config) < 0 || !ctx) {
        fprintf(stderr, "ERREUR: Impossible initialiser contexte Gen9\n");
        i915_forensic_tracker_cleanup();
        return 1;
    }
    printf("[TEST] Contexte Gen9 initialisé\n\n");
    
    /* C369: Exécuter suite de validation AVANT tout dispatch */
    printf("=== C369 VALIDATION SUITE ===\n");
    printf("[VALIDATION] Exécution tests préliminaires...\n");
    printf("[VALIDATION] Ces tests valident la pile GPU Gen9 complète\n\n");
    
    int validation_result = btc_gen9_run_validation_suite(ctx);
    if (validation_result < 0) {
        fprintf(stderr, "\n[VALIDATION] ❌ ÉCHEC: Un ou plusieurs tests ont échoué\n");
        fprintf(stderr, "[VALIDATION] Le minage ne peut pas démarrer avec une pile GPU défectueuse\n");
        fprintf(stderr, "[VALIDATION] Consultez les logs pour identifier le problème\n");
        btc_gen9_cleanup(ctx);
        i915_forensic_tracker_cleanup();
        return 1;
    }
    
    printf("\n[VALIDATION] ✅ SUCCÈS: Tous les tests de validation ont réussi\n");
    printf("[VALIDATION] La pile GPU Gen9 est opérationnelle\n");
    printf("[VALIDATION] Passage au minage production...\n\n");
    
    /* Dump buffer kernel ISA (TAILLE COMPLÈTE) */
    printf("[FORENSIC] Dump kernel ISA...\n");
    uint32_t kernel_bo = btc_gen9_get_kernel_bo(ctx);
    void* kernel_map = btc_gen9_get_kernel_map(ctx);
    size_t kernel_size = btc_gen9_get_kernel_size(ctx);
    if (kernel_map) {
        i915_forensic_dump_buffer(kernel_bo, kernel_map, kernel_size);
        printf("[FORENSIC] Kernel ISA dumpé (%zu bytes)\n", kernel_size);
    }
    
    /* Dump batch buffer (déjà construit lors de l'init) */
    printf("[FORENSIC] Dump batch buffer...\n");
    uint32_t batch_bo = btc_gen9_get_batch_bo(ctx);
    void* batch_map = btc_gen9_get_batch_map(ctx);
    size_t batch_size = btc_gen9_get_batch_size(ctx);
    if (batch_map) {
        i915_forensic_dump_buffer(batch_bo, batch_map, batch_size);
        printf("[FORENSIC] Batch buffer dumpé (%zu bytes)\n", batch_size);
    }
    
    /* Exécuter 1 dispatch avec traçabilité maximale */
    printf("\n[TEST] Exécution dispatch #1 (traçabilité maximale)...\n");
    printf("[TEST] TOUS les ioctl seront tracés bit par bit\n");
    printf("[TEST] Cela peut prendre plusieurs secondes...\n\n");
    
    struct timespec ts_start, ts_end;
    clock_gettime(CLOCK_MONOTONIC, &ts_start);
    
    int result = btc_gen9_execute(ctx);
    
    clock_gettime(CLOCK_MONOTONIC, &ts_end);
    double exec_time = (ts_end.tv_sec - ts_start.tv_sec) + 
                       (ts_end.tv_nsec - ts_start.tv_nsec) / 1e9;
    
    if (result < 0) {
        fprintf(stderr, "ERREUR: Dispatch échoué\n");
    } else {
        printf("[TEST] Dispatch réussi (temps: %.6f sec)\n", exec_time);
    }
    
    /* Dump output buffer (résultats) */
    printf("\n[FORENSIC] Dump output buffer (résultats)...\n");
    uint32_t output_bo = btc_gen9_get_output_bo(ctx);
    void* output_map = btc_gen9_get_output_map(ctx);
    size_t output_size = btc_gen9_get_output_size(ctx);
    if (output_map) {
        /* Dumper premiers 4096 bytes seulement (output = 1 MB) */
        i915_forensic_dump_buffer(output_bo, output_map, 4096);
        printf("[FORENSIC] Output buffer dumpé (4096 bytes sur %zu total)\n", output_size);
    }
    
    /* Cleanup */
    printf("\n[TEST] Cleanup...\n");
    btc_gen9_cleanup(ctx);
    
    printf("[FORENSIC] Génération statistiques finales...\n");
    i915_forensic_tracker_cleanup();
    
    printf("\n=== TEST FORENSIQUE TERMINÉ ===\n");
    printf("Résultat: %s\n", result == 0 ? "SUCCESS" : "FAILED");
    printf("Temps exécution: %.6f sec\n", exec_time);
    printf("\nLogs forensiques générés:\n");
    printf("  - logs/forensic/i915_forensic_main.log (événements principaux)\n");
    printf("  - logs/forensic/i915_forensic_hexdump.log (dumps hexadécimaux)\n");
    printf("  - logs/forensic/i915_forensic_timeline.log (timeline CSV)\n");
    printf("\nAnalyse recommandée:\n");
    printf("  1. Lire i915_forensic_main.log ligne par ligne\n");
    printf("  2. Identifier TOUS les ioctl appelés\n");
    printf("  3. Analyser structures complètes (hexdump)\n");
    printf("  4. Comprendre dépendances GEM (implicit sync)\n");
    printf("  5. Identifier cause sérialisation dispatches\n");
    
    return result == 0 ? 0 : 1;
}

// Made with Bob