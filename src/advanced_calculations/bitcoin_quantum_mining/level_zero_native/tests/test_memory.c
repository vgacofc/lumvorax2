/*
 * LumVorax C198 — Test Allocation Mémoire GPU Level Zero
 * test_memory.c — Validation complète allocation/écriture/lecture
 */

#include "../include/btc_levelzero_runner.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv) {
    (void)argc; (void)argv;
    
    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  LumVorax C198 — Test Allocation Mémoire GPU\n");
    printf("═══════════════════════════════════════════════════════════\n\n");
    
    /* Configuration */
    btc_l0_config_t config = {
        .batch_size = 262144,
        .work_group_size = 256,
        .num_work_groups = 0,
        .enable_profiling = true,
        .enable_forensic_log = true,
        .forensic_log_path = "logs/forensic/test_memory_c198.lum",
        .timeout_ms = 30000
    };
    
    printf("[TEST] Configuration:\n");
    printf("  - batch_size: %zu\n", config.batch_size);
    printf("  - forensic_log: %s\n\n", config.forensic_log_path);
    
    /* Test 1: Initialisation */
    printf("[TEST 1/5] Initialisation Level Zero...\n");
    btc_l0_context_t* ctx = btc_l0_init(&config);
    if (!ctx) {
        printf("❌ ÉCHEC : Initialisation échouée\n");
        return 1;
    }
    printf("✅ Initialisation réussie\n\n");
    
    /* Test 2: Allocation buffer 1 MB */
    printf("[TEST 2/5] Allocation buffer 1 MB...\n");
    size_t buffer_size = 1024 * 1024;  /* 1 MB */
    btc_l0_buffer_t* buffer = btc_l0_alloc_buffer(ctx, buffer_size, "test_buffer_1mb");
    if (!buffer) {
        printf("❌ ÉCHEC : Allocation échouée\n");
        btc_l0_cleanup(ctx);
        return 1;
    }
    printf("✅ Buffer alloué: %zu bytes (GPU: 0x%lx)\n\n", buffer->size, buffer->gpu_address);
    
    /* Test 3: Écriture données */
    printf("[TEST 3/5] Écriture 1 MB de données...\n");
    uint32_t* host_data = (uint32_t*)malloc(buffer_size);
    if (!host_data) {
        printf("❌ ÉCHEC : malloc() échoué\n");
        btc_l0_free_buffer(ctx, buffer);
        btc_l0_cleanup(ctx);
        return 1;
    }
    
    /* Remplissage pattern 0xDEADBEEF + index */
    for (size_t i = 0; i < buffer_size / sizeof(uint32_t); i++) {
        host_data[i] = 0xDEADBEEF + (uint32_t)i;
    }
    
    int ret = btc_l0_write_buffer(ctx, buffer, host_data, buffer_size);
    if (ret != BTC_L0_OK) {
        printf("❌ ÉCHEC : Écriture échouée (code=%d)\n", ret);
        free(host_data);
        btc_l0_free_buffer(ctx, buffer);
        btc_l0_cleanup(ctx);
        return 1;
    }
    printf("✅ Écriture réussie\n\n");
    
    /* Test 4: Lecture données */
    printf("[TEST 4/5] Lecture 1 MB depuis GPU...\n");
    uint32_t* read_data = (uint32_t*)calloc(1, buffer_size);
    if (!read_data) {
        printf("❌ ÉCHEC : calloc() échoué\n");
        free(host_data);
        btc_l0_free_buffer(ctx, buffer);
        btc_l0_cleanup(ctx);
        return 1;
    }
    
    ret = btc_l0_read_buffer(ctx, buffer, read_data, buffer_size);
    if (ret != BTC_L0_OK) {
        printf("❌ ÉCHEC : Lecture échouée (code=%d)\n", ret);
        free(read_data);
        free(host_data);
        btc_l0_free_buffer(ctx, buffer);
        btc_l0_cleanup(ctx);
        return 1;
    }
    printf("✅ Lecture réussie\n\n");
    
    /* Test 5: Vérification intégrité */
    printf("[TEST 5/5] Vérification intégrité données...\n");
    size_t errors = 0;
    size_t num_elements = buffer_size / sizeof(uint32_t);
    
    for (size_t i = 0; i < num_elements; i++) {
        uint32_t expected = 0xDEADBEEF + (uint32_t)i;
        if (read_data[i] != expected) {
            if (errors < 10) {  /* Affiche max 10 erreurs */
                printf("  ⚠️  Erreur index %zu: attendu=0x%08X, lu=0x%08X\n",
                       i, expected, read_data[i]);
            }
            errors++;
        }
    }
    
    if (errors > 0) {
        printf("❌ ÉCHEC : %zu erreurs sur %zu éléments (%.2f%%)\n",
               errors, num_elements, (errors * 100.0) / num_elements);
        free(read_data);
        free(host_data);
        btc_l0_free_buffer(ctx, buffer);
        btc_l0_cleanup(ctx);
        return 1;
    }
    
    printf("✅ Intégrité vérifiée: %zu éléments corrects\n\n", num_elements);
    
    /* Statistiques */
    printf("[STATISTIQUES]\n");
    printf("  - Opérations totales: %lu\n", ctx->total_operations);
    printf("  - Bytes transférés: %lu (%.2f MB)\n",
           ctx->total_bytes_transferred,
           ctx->total_bytes_transferred / (1024.0 * 1024.0));
    printf("  - Accès buffer: %lu\n\n", buffer->access_count);
    
    /* Cleanup */
    printf("[CLEANUP] Libération ressources...\n");
    free(read_data);
    free(host_data);
    btc_l0_free_buffer(ctx, buffer);
    btc_l0_cleanup(ctx);
    printf("✅ Cleanup terminé\n\n");
    
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  RÉSULTAT : ✅ TOUS LES TESTS RÉUSSIS\n");
    printf("═══════════════════════════════════════════════════════════\n\n");
    
    return 0;
}

// Made with Bob
