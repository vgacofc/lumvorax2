/*
 * LumVorax C198 — Test Initialisation Level Zero
 * test_init.c — Validation initialisation complète
 *
 * OBJECTIF : Tester l'initialisation Level Zero avec traçabilité bit-level
 */

#include "../include/btc_levelzero_runner.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv) {
    (void)argc; (void)argv;  /* Unused: no command-line args needed */
    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  LumVorax C198 — Test Initialisation Level Zero\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("\n");
    
    /* Configuration */
    btc_l0_config_t config = {
        .batch_size = BTC_L0_BATCH_DEFAULT,
        .work_group_size = 256,
        .num_work_groups = 0,
        .enable_profiling = true,
        .enable_forensic_log = true,
        .forensic_log_path = "logs/forensic/test_init_c198.lum",
        .timeout_ms = 30000
    };
    
    printf("[TEST] Configuration:\n");
    printf("  - batch_size: %zu\n", config.batch_size);
    printf("  - work_group_size: %u\n", config.work_group_size);
    printf("  - enable_profiling: %s\n", config.enable_profiling ? "OUI" : "NON");
    printf("  - enable_forensic_log: %s\n", config.enable_forensic_log ? "OUI" : "NON");
    printf("  - forensic_log_path: %s\n", config.forensic_log_path);
    printf("\n");
    
    /* Test 1 : Initialisation */
    printf("[TEST 1/3] Initialisation Level Zero...\n");
    btc_l0_context_t* ctx = btc_l0_init(&config);
    
    if (!ctx) {
        printf("❌ ÉCHEC : Initialisation échouée\n");
        printf("\n");
        printf("DIAGNOSTIC :\n");
        printf("  - Level Zero SDK installé ? (level-zero-dev)\n");
        printf("  - Driver Intel GPU avec support Level Zero ?\n");
        printf("  - GPU Intel détecté par le système ?\n");
        printf("\n");
        printf("Commandes de vérification :\n");
        printf("  $ sudo apt install level-zero-dev\n");
        printf("  $ ls /usr/include/level_zero/\n");
        printf("  $ ldconfig -p | grep libze_loader\n");
        printf("  $ lspci | grep -i vga\n");
        printf("\n");
        return 1;
    }
    
    printf("✅ Initialisation réussie\n");
    printf("\n");
    
    /* Test 2 : Validation contexte */
    printf("[TEST 2/3] Validation contexte...\n");
    
    if (!btc_l0_validate_magic(ctx->magic, BTC_L0_MAGIC_CONTEXT)) {
        printf("❌ ÉCHEC : Magic number invalide\n");
        btc_l0_cleanup(ctx);
        return 1;
    }
    printf("  ✅ Magic number valide: 0x%08X\n", ctx->magic);
    
    if (!ctx->initialized) {
        printf("❌ ÉCHEC : Contexte non initialisé\n");
        btc_l0_cleanup(ctx);
        return 1;
    }
    printf("  ✅ Contexte initialisé\n");
    
    if (!ctx->ze_driver) {
        printf("❌ ÉCHEC : Driver non initialisé\n");
        btc_l0_cleanup(ctx);
        return 1;
    }
    printf("  ✅ Driver Level Zero: %p\n", ctx->ze_driver);
    
    if (!ctx->ze_device) {
        printf("❌ ÉCHEC : Device non initialisé\n");
        btc_l0_cleanup(ctx);
        return 1;
    }
    printf("  ✅ Device Level Zero: %p\n", ctx->ze_device);
    
    if (!ctx->ze_context) {
        printf("❌ ÉCHEC : Contexte Level Zero non créé\n");
        btc_l0_cleanup(ctx);
        return 1;
    }
    printf("  ✅ Contexte Level Zero: %p\n", ctx->ze_context);
    
    if (!ctx->ze_command_queue) {
        printf("❌ ÉCHEC : Command queue non créée\n");
        btc_l0_cleanup(ctx);
        return 1;
    }
    printf("  ✅ Command queue: %p\n", ctx->ze_command_queue);
    
    if (!ctx->ze_command_list) {
        printf("❌ ÉCHEC : Command list non créée\n");
        btc_l0_cleanup(ctx);
        return 1;
    }
    printf("  ✅ Command list: %p\n", ctx->ze_command_list);
    
    printf("\n");
    
    /* Test 3 : Informations device */
    printf("[TEST 3/3] Informations device...\n");
    printf("  - Nom: %s\n", ctx->device_name);
    printf("  - Device ID: 0x%08X\n", ctx->device_id);
    printf("  - Compute Units: %u\n", ctx->max_compute_units);
    printf("  - Mémoire globale: %lu MB\n", ctx->global_mem_size / 1048576);
    printf("  - Max allocation: %lu MB\n", ctx->max_mem_alloc_size / 1048576);
    printf("\n");
    
    /* Métriques forensiques */
    if (ctx->forensic_log) {
        printf("[FORENSIQUE] Métriques logging:\n");
        printf("  - Événements totaux: %lu\n", ctx->forensic_log->total_events);
        printf("  - Événements perdus: %lu\n", ctx->forensic_log->dropped_events);
        printf("  - Fichier log: %s\n", ctx->forensic_log->log_file);
        printf("\n");
    }
    
    /* Cleanup */
    printf("[CLEANUP] Libération ressources...\n");
    btc_l0_cleanup(ctx);
    printf("✅ Cleanup terminé\n");
    printf("\n");
    
    /* Résumé */
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  RÉSULTAT : ✅ TOUS LES TESTS RÉUSSIS\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("\n");
    printf("Prochaines étapes:\n");
    printf("  1. Implémenter allocation mémoire (btc_l0_alloc_buffer)\n");
    printf("  2. Implémenter compilation kernel (btc_l0_compile_kernel)\n");
    printf("  3. Implémenter exécution (btc_l0_execute_kernel)\n");
    printf("  4. Implémenter mining Bitcoin (btc_l0_mine_batch)\n");
    printf("  5. Benchmark vs OpenCL\n");
    printf("\n");
    
    return 0;
}

/* Made with Bob - LumVorax C198 */

// Made with Bob
