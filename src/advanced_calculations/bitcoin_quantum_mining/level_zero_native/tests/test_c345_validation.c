/*
 * LumVorax C345 — Test Validation Relocations
 * test_c345_validation.c — Valide que GPU écrit correctement via relocations
 */

#include "../include/btc_gen9_native_runner.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  LumVorax C345 — Test Validation Relocations              ║\n");
    printf("║  Kernel: write_magic (écrit 0xC345BEEF)                   ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    
    /* Configuration avec kernel de validation */
    btc_gen9_config_t config = BTC_GEN9_DEFAULT_CONFIG;
    config.kernel_path = "kernels/test_write_magic_gen9.bin";
    config.log_path = "logs/forensic/test_c345_validation.log";
    config.batch_size = 256;  /* Petit batch pour test rapide */
    
    printf("[TEST] Configuration:\n");
    printf("  - kernel: %s\n", config.kernel_path);
    printf("  - batch_size: %u\n", config.batch_size);
    printf("\n");
    
    /* Initialisation */
    printf("[1/3] Initialisation...\n");
    btc_gen9_context_t* ctx = NULL;
    int ret = btc_gen9_init(&ctx, &config);
    if (ret < 0) {
        printf("❌ ÉCHEC Initialisation\n");
        return 1;
    }
    printf("✅ Initialisation OK\n\n");
    
    /* Exécution */
    printf("[2/3] Exécution GPU...\n");
    ret = btc_gen9_execute(ctx);
    if (ret < 0) {
        printf("❌ ÉCHEC Exécution\n");
        btc_gen9_cleanup(ctx);
        return 1;
    }
    printf("✅ Exécution OK\n\n");
    
    /* Validation résultats */
    printf("[3/3] Validation résultats...\n");
    uint32_t* output = (uint32_t*)btc_gen9_get_output_map(ctx);
    
    printf("  output[0] = 0x%08X (attendu: 0xC345BEEF)\n", output[0]);
    printf("  output[1] = 0x%08X (attendu: 0x12345678)\n", output[1]);
    printf("  output[2] = 0x%08X (attendu: 0x00000000)\n", output[2]);
    printf("\n");
    
    int success = 1;
    if (output[0] != 0xC345BEEF) {
        printf("❌ ÉCHEC: output[0] incorrect\n");
        printf("   Cause probable: Relocations ne fonctionnent pas\n");
        printf("   GPU n'écrit pas à la bonne adresse\n");
        success = 0;
    }
    
    if (output[1] != 0x12345678) {
        printf("❌ ÉCHEC: output[1] incorrect\n");
        success = 0;
    }
    
    if (output[2] != 0x00000000) {
        printf("❌ ÉCHEC: output[2] incorrect (thread ID devrait être 0)\n");
        success = 0;
    }
    
    if (success) {
        printf("╔════════════════════════════════════════════════════════════╗\n");
        printf("║  ✅ SUCCÈS TOTAL                                           ║\n");
        printf("║  Relocations C345 fonctionnent correctement !              ║\n");
        printf("║  GPU écrit aux bonnes adresses via GTT mapping            ║\n");
        printf("╚════════════════════════════════════════════════════════════╝\n");
    } else {
        printf("╔════════════════════════════════════════════════════════════╗\n");
        printf("║  ❌ ÉCHEC                                                   ║\n");
        printf("║  Relocations ne fonctionnent pas correctement              ║\n");
        printf("╚════════════════════════════════════════════════════════════╝\n");
    }
    printf("\n");
    
    /* Cleanup */
    btc_gen9_cleanup(ctx);
    
    return success ? 0 : 1;
}

// Made with Bob
