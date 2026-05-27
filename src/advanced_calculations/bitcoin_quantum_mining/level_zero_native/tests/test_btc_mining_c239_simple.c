/*
 * LumVorax — Module 17 — Bitcoin Quantum Mining Engine
 * test_btc_mining_c239_simple.c — Test Augmentation Progressive Batch Size
 *
 * CYCLE C239 — Découverte Limite Maximale GPU (Approche Simple)
 * STANDARD_NAMES.md v4.6 §M-BTC-TEST-C239-SIMPLE
 *
 * OBJECTIF : DÉCOUVRIR BATCH SIZE MAXIMUM PAR DISPATCH
 * - Test batch sizes: 262144, 524288, 1048576, 2097152, 4194304, ...
 * - Réinitialisation contexte entre chaque test
 * - Maximum 20 tests (jusqu'à ~268M hashes)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>

/* Interface Gen9 Native */
extern void* btc_gen9_init(const char* kernel_path, const char* log_path, uint32_t batch_size);
extern int btc_gen9_execute(void* ctx);
extern void btc_gen9_cleanup(void* ctx);

/* ══════════════════════════════════════════════════════════════════════
 * CONFIGURATION
 * ══════════════════════════════════════════════════════════════════════ */

#define KERNEL_PATH "kernels/btc_sha256_gen9.bin"
#define LOG_PATH "logs/forensic/btc_mining_c239_simple.log"

/* Batch sizes à tester (puissances de 2) */
static const uint32_t BATCH_SIZES[] = {
    262144,      /* 256K - baseline C234 */
    524288,      /* 512K - ×2 */
    1048576,     /* 1M - ×4 */
    2097152,     /* 2M - ×8 */
    4194304,     /* 4M - ×16 */
    8388608,     /* 8M - ×32 */
    16777216,    /* 16M - ×64 */
    33554432,    /* 32M - ×128 */
    67108864,    /* 64M - ×256 */
    134217728,   /* 128M - ×512 */
    268435456    /* 256M - ×1024 */
};

#define NUM_BATCH_SIZES (sizeof(BATCH_SIZES) / sizeof(BATCH_SIZES[0]))

/* ══════════════════════════════════════════════════════════════════════
 * FONCTION PRINCIPALE
 * ══════════════════════════════════════════════════════════════════════ */

int main(void) {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  LumVorax C239 — Test Augmentation Progressive Batch Size ║\n");
    printf("║  Découverte Limite Maximale GPU (Approche Simple)         ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    
    printf("Configuration:\n");
    printf("  Nombre de tests: %zu\n", NUM_BATCH_SIZES);
    printf("  Batch sizes: 256K → 256M (puissances de 2)\n");
    printf("  Stratégie: Réinitialisation contexte entre chaque test\n");
    printf("  Log: %s\n", LOG_PATH);
    printf("\n");
    
    /* Tableau résultats */
    typedef struct {
        uint32_t batch_size;
        double time_sec;
        bool success;
        double hashrate_mhs;
        int factor;
    } result_t;
    
    result_t results[NUM_BATCH_SIZES];
    int total_success = 0;
    int total_failed = 0;
    uint32_t max_batch_size = 0;
    
    /* Boucle tests */
    printf("🚀 Démarrage tests progressifs...\n\n");
    
    for (size_t i = 0; i < NUM_BATCH_SIZES; i++) {
        uint32_t batch_size = BATCH_SIZES[i];
        int factor = batch_size / 262144;
        
        printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
        printf("📦 Test %zu/%zu : batch_size=%u (×%d)\n", 
               i + 1, NUM_BATCH_SIZES, batch_size, factor);
        printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
        
        /* Initialiser contexte */
        printf("   Initialisation contexte...\n");
        void* ctx = btc_gen9_init(KERNEL_PATH, LOG_PATH, batch_size);
        if (!ctx) {
            fprintf(stderr, "   ✗ ÉCHEC initialisation (batch_size=%u)\n\n", batch_size);
            results[i].batch_size = batch_size;
            results[i].time_sec = 0.0;
            results[i].success = false;
            results[i].hashrate_mhs = 0.0;
            results[i].factor = factor;
            total_failed++;
            break;
        }
        printf("   ✓ Contexte initialisé\n");
        
        /* Exécuter dispatch */
        printf("   Exécution dispatch...\n");
        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);
        
        int ret = btc_gen9_execute(ctx);
        
        clock_gettime(CLOCK_MONOTONIC, &end);
        double dispatch_time = (end.tv_sec - start.tv_sec) + 
                               (end.tv_nsec - start.tv_nsec) / 1e9;
        
        /* Cleanup */
        btc_gen9_cleanup(ctx);
        
        /* Enregistrer résultat */
        results[i].batch_size = batch_size;
        results[i].time_sec = dispatch_time;
        results[i].success = (ret == 0);
        results[i].hashrate_mhs = (ret == 0) ? (batch_size / dispatch_time) / 1e6 : 0.0;
        results[i].factor = factor;
        
        if (ret == 0) {
            total_success++;
            max_batch_size = batch_size;
            printf("   ✓ SUCCÈS : %.3f sec (%.1f MH/s)\n\n", 
                   dispatch_time, results[i].hashrate_mhs);
        } else {
            total_failed++;
            printf("   ✗ ÉCHEC : GPU ne peut pas gérer batch_size=%u\n", batch_size);
            if (i > 0) {
                printf("   Limite maximale trouvée : batch_size=%u (×%d)\n\n",
                       results[i-1].batch_size, results[i-1].factor);
            }
            break;
        }
        
        /* Pause sécurité */
        printf("   Pause 1 sec...\n\n");
        sleep(1);
    }
    
    /* Résultats finaux */
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  RÉSULTATS C239 — AUGMENTATION PROGRESSIVE                ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    
    printf("Tests réussis: %d/%d\n", total_success, total_success + total_failed);
    printf("Tests échoués: %d\n", total_failed);
    printf("\n");
    
    /* Tableau résultats */
    printf("┌─────────┬──────────────┬───────────┬──────────┬──────────┐\n");
    printf("│ Facteur │ Batch Size   │ Temps (s) │ Status   │ MH/s     │\n");
    printf("├─────────┼──────────────┼───────────┼──────────┼──────────┤\n");
    
    for (int i = 0; i < total_success + total_failed; i++) {
        printf("│ ×%-6d │ %12u │ %9.3f │ %8s │ %8.1f │\n",
               results[i].factor,
               results[i].batch_size,
               results[i].time_sec,
               results[i].success ? "✓ OK" : "✗ FAIL",
               results[i].hashrate_mhs);
    }
    
    printf("└─────────┴──────────────┴───────────┴──────────┴──────────┘\n");
    printf("\n");
    
    /* Analyse limite maximale */
    if (total_success > 0) {
        result_t* best = &results[total_success - 1];
        printf("🏆 LIMITE MAXIMALE DÉCOUVERTE:\n");
        printf("   Batch size max: %u hashes/dispatch\n", best->batch_size);
        printf("   Facteur vs baseline: ×%d\n", best->factor);
        printf("   Temps dispatch: %.3f sec\n", best->time_sec);
        printf("   Hashrate max: %.1f MH/s\n", best->hashrate_mhs);
        printf("\n");
        
        /* Comparaison C234 baseline */
        double c234_hashrate = 371.5; /* MH/s */
        double gain_percent = ((best->hashrate_mhs - c234_hashrate) / c234_hashrate) * 100.0;
        printf("Comparaison C234 → C239:\n");
        printf("  C234 baseline: %.1f MH/s (batch 262144)\n", c234_hashrate);
        printf("  C239 maximum: %.1f MH/s (batch %u)\n", best->hashrate_mhs, best->batch_size);
        printf("  Gain: %+.1f%% (%s)\n", gain_percent,
               gain_percent > 0 ? "✓ AMÉLIORATION" : "⚠ RÉGRESSION");
        printf("\n");
        
        /* Recommandation */
        printf("💡 RECOMMANDATION PRODUCTION:\n");
        printf("   Utiliser batch_size=%u pour maximiser hashrate\n", best->batch_size);
        printf("   Hashrate attendu: %.1f MH/s par dispatch\n", best->hashrate_mhs);
        printf("\n");
    }
    
    printf("✅ Test C239 terminé\n");
    printf("📊 Logs forensiques: %s\n", LOG_PATH);
    printf("\n");
    
    return 0;
}

// Made with Bob
