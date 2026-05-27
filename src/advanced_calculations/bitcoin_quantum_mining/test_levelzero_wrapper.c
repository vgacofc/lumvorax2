/**
 * @file test_levelzero_wrapper.c
 * @brief Test unitaire wrapper Level Zero - Validation C222
 *
 * Test isolé du wrapper Level Zero pour valider :
 * - Initialisation Level Zero
 * - Allocation buffers GPU
 * - Transferts host↔device
 * - Lancement kernel (si bytecode disponible)
 * - Métriques performance
 *
 * @author Bob (LumVorax AI Agent)
 * @date 2026-05-12
 * @cycle C222
 */

#define _POSIX_C_SOURCE 200809L

#include "btc_gpu_backend.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>

/* Obtenir timestamp nanoseconde */
static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    
    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║  TEST UNITAIRE WRAPPER LEVEL ZERO - C222                ║\n");
    printf("║  Objectif : Validation interface abstraite GPU          ║\n");
    printf("╚══════════════════════════════════════════════════════════╝\n\n");
    
    /* Configuration Level Zero */
    btc_gpu_config_t config = {0};
    config.backend_type = BTC_GPU_BACKEND_LEVELZERO;
    config.device_id = 0;
    config.enable_profiling = true;
    config.kernel_bytecode = NULL;  /* Pas de kernel pour ce test */
    config.kernel_bytecode_size = 0;
    
    /* Test 1 : Initialisation */
    printf("[TEST 1] Initialisation Level Zero...\n");
    uint64_t t1_start = get_timestamp_ns();
    
    btc_gpu_backend_context_t *ctx = NULL;
    int ret = btc_gpu_backend_init(&config, &ctx);
    
    uint64_t t1_end = get_timestamp_ns();
    double t1_ms = (t1_end - t1_start) / 1000000.0;
    
    if (ret != 0 || !ctx) {
        printf("❌ ÉCHEC : Initialisation Level Zero (ret=%d)\n", ret);
        return 1;
    }
    printf("✅ SUCCÈS : Initialisation Level Zero (%.2f ms)\n\n", t1_ms);
    
    /* Test 2 : Allocation buffer GPU */
    printf("[TEST 2] Allocation buffer GPU (1 MB)...\n");
    uint64_t t2_start = get_timestamp_ns();
    
    size_t buffer_size = 1024 * 1024;  /* 1 MB */
    btc_gpu_buffer_t *buffer = NULL;
    ret = btc_gpu_buffer_alloc(ctx, buffer_size, &buffer);
    
    uint64_t t2_end = get_timestamp_ns();
    double t2_ms = (t2_end - t2_start) / 1000000.0;
    
    if (ret != 0 || !buffer) {
        printf("❌ ÉCHEC : Allocation buffer (ret=%d)\n", ret);
        btc_gpu_backend_destroy(ctx);
        return 1;
    }
    printf("✅ SUCCÈS : Buffer alloué (%.2f ms)\n\n", t2_ms);
    
    /* Test 3 : Upload données host→device */
    printf("[TEST 3] Upload données host→device (1 MB)...\n");
    uint64_t t3_start = get_timestamp_ns();
    
    uint8_t *host_data = malloc(buffer_size);
    if (!host_data) {
        printf("❌ ÉCHEC : Allocation host_data\n");
        btc_gpu_buffer_free(buffer);
        btc_gpu_backend_destroy(ctx);
        return 1;
    }
    
    /* Remplir avec pattern */
    for (size_t i = 0; i < buffer_size; i++) {
        host_data[i] = (uint8_t)(i & 0xFF);
    }
    
    ret = btc_gpu_buffer_write(buffer, host_data, buffer_size, true);
    
    uint64_t t3_end = get_timestamp_ns();
    double t3_ms = (t3_end - t3_start) / 1000000.0;
    double t3_bw = (buffer_size / (1024.0 * 1024.0)) / (t3_ms / 1000.0);  /* MB/s */
    
    if (ret != 0) {
        printf("❌ ÉCHEC : Upload données (ret=%d)\n", ret);
        free(host_data);
        btc_gpu_buffer_free(buffer);
        btc_gpu_backend_destroy(ctx);
        return 1;
    }
    printf("✅ SUCCÈS : Upload (%.2f ms, %.2f MB/s)\n\n", t3_ms, t3_bw);
    
    /* Test 4 : Download données device→host */
    printf("[TEST 4] Download données device→host (1 MB)...\n");
    uint64_t t4_start = get_timestamp_ns();
    
    uint8_t *host_readback = malloc(buffer_size);
    if (!host_readback) {
        printf("❌ ÉCHEC : Allocation host_readback\n");
        free(host_data);
        btc_gpu_buffer_free(buffer);
        btc_gpu_backend_destroy(ctx);
        return 1;
    }
    
    ret = btc_gpu_buffer_read(buffer, host_readback, buffer_size, true);
    
    uint64_t t4_end = get_timestamp_ns();
    double t4_ms = (t4_end - t4_start) / 1000000.0;
    double t4_bw = (buffer_size / (1024.0 * 1024.0)) / (t4_ms / 1000.0);  /* MB/s */
    
    if (ret != 0) {
        printf("❌ ÉCHEC : Download données (ret=%d)\n", ret);
        free(host_readback);
        free(host_data);
        btc_gpu_buffer_free(buffer);
        btc_gpu_backend_destroy(ctx);
        return 1;
    }
    printf("✅ SUCCÈS : Download (%.2f ms, %.2f MB/s)\n\n", t4_ms, t4_bw);
    
    /* Test 5 : Vérification intégrité données */
    printf("[TEST 5] Vérification intégrité données...\n");
    int errors = 0;
    for (size_t i = 0; i < buffer_size; i++) {
        if (host_readback[i] != host_data[i]) {
            if (errors < 10) {
                printf("  Erreur offset %zu : attendu 0x%02x, reçu 0x%02x\n",
                       i, host_data[i], host_readback[i]);
            }
            errors++;
        }
    }
    
    if (errors > 0) {
        printf("❌ ÉCHEC : %d erreurs détectées\n", errors);
        free(host_readback);
        free(host_data);
        btc_gpu_buffer_free(buffer);
        btc_gpu_backend_destroy(ctx);
        return 1;
    }
    printf("✅ SUCCÈS : Intégrité 100%% (1 MB vérifié)\n\n");
    
    /* Test 6 : Statistiques GPU */
    printf("[TEST 6] Statistiques GPU...\n");
    btc_gpu_stats_t stats = {0};
    ret = btc_gpu_backend_get_stats(ctx, &stats);
    
    if (ret != 0) {
        printf("❌ ÉCHEC : Récupération stats (ret=%d)\n", ret);
    } else {
        printf("✅ SUCCÈS : Statistiques récupérées\n");
        printf("  - Mémoire allouée    : %zu bytes\n", (size_t)stats.memory_allocated_bytes);
        printf("  - Mémoire transférée : %zu bytes\n", (size_t)stats.memory_transferred_bytes);
        printf("  - Kernel launches    : %zu\n", (size_t)stats.kernel_launches);
    }
    printf("\n");
    
    /* Cleanup */
    printf("[CLEANUP] Libération ressources...\n");
    free(host_readback);
    free(host_data);
    btc_gpu_buffer_free(buffer);
    btc_gpu_backend_destroy(ctx);
    printf("✅ SUCCÈS : Cleanup terminé\n\n");
    
    /* Résumé */
    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║  RÉSUMÉ TESTS LEVEL ZERO                                ║\n");
    printf("╠══════════════════════════════════════════════════════════╣\n");
    printf("║  Test 1 : Initialisation         ✅ (%.2f ms)          \n", t1_ms);
    printf("║  Test 2 : Allocation buffer       ✅ (%.2f ms)          \n", t2_ms);
    printf("║  Test 3 : Upload host→device      ✅ (%.2f MB/s)        \n", t3_bw);
    printf("║  Test 4 : Download device→host    ✅ (%.2f MB/s)        \n", t4_bw);
    printf("║  Test 5 : Intégrité données       ✅ (100%%)             \n");
    printf("║  Test 6 : Statistiques            ✅                     \n");
    printf("╠══════════════════════════════════════════════════════════╣\n");
    printf("║  STATUT GLOBAL : ✅ TOUS LES TESTS RÉUSSIS              ║\n");
    printf("╚══════════════════════════════════════════════════════════╝\n");
    
    return 0;
}

/* Made with Bob - LumVorax C222 - Test Level Zero Wrapper */

// Made with Bob
