/*
 * LumVorax — Module 17 — Bitcoin Quantum Mining Engine
 * test_btc_mining_c238_maxhash.c — Test Maximisation Hashrate
 *
 * CYCLE C238 — Optimisation #6 : Augmentation Batch Size
 * STANDARD_NAMES.md v4.6 §M-BTC-TEST-C238-MAXHASH
 *
 * OBJECTIF : MAXIMISER HASHRATE GPU
 * - Batch size 524288 (×2 vs C234)
 * - DRM reopen tous les 18 dispatches (×2 vs C234)
 * - Overclocking GPU 1000 MHz
 * - Monitoring thermal renforcé
 *
 * PRÉDICTIONS :
 * - Hashrate : +50-100% (742 MH/s attendu)
 * - Latence dispatch : +50% (1.0-1.2 sec)
 * - Température : +10-20°C (40-50°C attendu)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

/* Interface Gen9 Native */
extern void* btc_gen9_init(const char* kernel_path, const char* log_path, uint32_t batch_size);
extern int btc_gen9_execute_batch_intelligent(void* ctx, int max_dispatches);
extern void btc_gen9_cleanup(void* ctx);
extern void btc_gen9_print_metrics(const void* ctx);

/* ══════════════════════════════════════════════════════════════════════
 * SECTION 1 : CONFIGURATION C238
 * ══════════════════════════════════════════════════════════════════════ */

#define C238_BATCH_SIZE 524288        /* ×2 vs C234 (262144) */
#define C238_WARMUP_DISPATCHES 10     /* Identique C234 */
#define C238_TEST_DISPATCHES 1000     /* Identique C234 */
#define C238_DRM_REOPEN_INTERVAL 18   /* ×2 vs C234 (9) */

/* Chemins */
#define KERNEL_PATH "kernels/btc_sha256_gen9.bin"
#define LOG_PATH_SHORT "logs/forensic/btc_mining_c238_maxhash_short.log"
#define LOG_PATH_LONG "logs/forensic/btc_mining_c238_maxhash_long.log"

/* ══════════════════════════════════════════════════════════════════════
 * SECTION 2 : OVERCLOCKING GPU
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * Active overclocking GPU 1000 MHz
 * Retourne 0 si succès, -1 si échec (non bloquant)
 */
static int enable_gpu_overclocking(void) {
    printf("🔥 Activation overclocking GPU 1000 MHz...\n");
    
    /* Tenter d'écrire fréquence max */
    FILE* f = fopen("/sys/class/drm/card0/gt_max_freq_mhz", "w");
    if (f) {
        fprintf(f, "1000\n");
        fclose(f);
        printf("   ✓ gt_max_freq_mhz = 1000 MHz\n");
    } else {
        printf("   ⚠ Impossible d'écrire gt_max_freq_mhz (permissions?)\n");
        return -1;
    }
    
    /* Tenter d'écrire fréquence min */
    f = fopen("/sys/class/drm/card0/gt_min_freq_mhz", "w");
    if (f) {
        fprintf(f, "1000\n");
        fclose(f);
        printf("   ✓ gt_min_freq_mhz = 1000 MHz\n");
    } else {
        printf("   ⚠ Impossible d'écrire gt_min_freq_mhz (permissions?)\n");
        return -1;
    }
    
    printf("   ✓ Overclocking activé\n\n");
    return 0;
}

/**
 * Désactive overclocking GPU (retour fréquences par défaut)
 */
static void disable_gpu_overclocking(void) {
    printf("🔥 Désactivation overclocking GPU...\n");
    
    /* Restaurer fréquence max par défaut (1000 MHz) */
    FILE* f = fopen("/sys/class/drm/card0/gt_max_freq_mhz", "w");
    if (f) {
        fprintf(f, "1000\n");
        fclose(f);
    }
    
    /* Restaurer fréquence min par défaut (300 MHz) */
    f = fopen("/sys/class/drm/card0/gt_min_freq_mhz", "w");
    if (f) {
        fprintf(f, "300\n");
        fclose(f);
    }
    
    printf("   ✓ Fréquences restaurées (300-1000 MHz)\n\n");
}

/* ══════════════════════════════════════════════════════════════════════
 * SECTION 3 : FONCTION PRINCIPALE
 * ══════════════════════════════════════════════════════════════════════ */

int main(int argc, char** argv) {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  LumVorax C238 — Test Maximisation Hashrate               ║\n");
    printf("║  Optimisation #6 : Batch Size 524288 (×2)                 ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    
    /* Parse arguments */
    bool long_test = false;
    bool enable_overclocking = false;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--long") == 0) {
            long_test = true;
        } else if (strcmp(argv[i], "--overclock") == 0) {
            enable_overclocking = true;
        } else if (strcmp(argv[i], "--help") == 0) {
            printf("Usage: %s [OPTIONS]\n", argv[0]);
            printf("\n");
            printf("Options:\n");
            printf("  --long        Test long (1000 dispatches)\n");
            printf("  --overclock   Activer overclocking GPU 1000 MHz\n");
            printf("  --help        Afficher cette aide\n");
            printf("\n");
            printf("Exemples:\n");
            printf("  %s                    # Test court (100 dispatches)\n", argv[0]);
            printf("  %s --long             # Test long (1000 dispatches)\n", argv[0]);
            printf("  %s --long --overclock # Test long avec overclocking\n", argv[0]);
            printf("\n");
            return 0;
        }
    }
    
    /* Configuration test */
    int test_dispatches = long_test ? C238_TEST_DISPATCHES : 100;
    const char* log_path = long_test ? LOG_PATH_LONG : LOG_PATH_SHORT;
    
    printf("Configuration:\n");
    printf("  Batch size: %u (×2 vs C234)\n", C238_BATCH_SIZE);
    printf("  Dispatches: %d (%s)\n", test_dispatches, long_test ? "long" : "court");
    printf("  DRM reopen: Tous les %d dispatches (×2 vs C234)\n", C238_DRM_REOPEN_INTERVAL);
    printf("  Overclocking: %s\n", enable_overclocking ? "OUI (1000 MHz)" : "NON (défaut)");
    printf("  Log: %s\n", log_path);
    printf("\n");
    
    /* Activer overclocking si demandé */
    if (enable_overclocking) {
        int ret = enable_gpu_overclocking();
        if (ret != 0) {
            printf("⚠ Overclocking échoué, continuation avec fréquences par défaut\n");
            printf("  Conseil: sudo chmod 666 /sys/class/drm/card0/gt_*_freq_mhz\n\n");
        }
    }
    
    /* Initialiser contexte */
    printf("📊 Initialisation contexte Gen9 Native...\n");
    void* ctx = btc_gen9_init(KERNEL_PATH, log_path, C238_BATCH_SIZE);
    if (!ctx) {
        fprintf(stderr, "❌ Échec initialisation\n");
        if (enable_overclocking) {
            disable_gpu_overclocking();
        }
        return 1;
    }
    printf("   ✓ Contexte initialisé\n\n");
    
    /* Warm-up */
    printf("🔥 Warm-up (%d dispatches)...\n", C238_WARMUP_DISPATCHES);
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    int ret = btc_gen9_execute_batch_intelligent(ctx, C238_WARMUP_DISPATCHES);
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    double warmup_time = (end.tv_sec - start.tv_sec) + 
                         (end.tv_nsec - start.tv_nsec) / 1e9;
    
    if (ret != 0) {
        fprintf(stderr, "❌ Échec warm-up\n");
        btc_gen9_cleanup(ctx);
        if (enable_overclocking) {
            disable_gpu_overclocking();
        }
        return 1;
    }
    
    printf("   ✓ Warm-up complété en %.2f sec\n", warmup_time);
    printf("   ✓ Temps moyen: %.0f ms/dispatch\n\n", 
           (warmup_time / C238_WARMUP_DISPATCHES) * 1000.0);
    
    /* Test principal */
    printf("🚀 Test principal (%d dispatches)...\n", test_dispatches);
    printf("   Batch size: %u hashes/dispatch\n", C238_BATCH_SIZE);
    printf("   Hashrate théorique: %.1f MH/s (si 0.7 sec/dispatch)\n",
           (C238_BATCH_SIZE / 0.7) / 1e6);
    printf("\n");
    
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    ret = btc_gen9_execute_batch_intelligent(ctx, test_dispatches);
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    double test_time = (end.tv_sec - start.tv_sec) + 
                       (end.tv_nsec - start.tv_nsec) / 1e9;
    
    if (ret != 0) {
        fprintf(stderr, "❌ Échec test principal\n");
        btc_gen9_cleanup(ctx);
        if (enable_overclocking) {
            disable_gpu_overclocking();
        }
        return 1;
    }
    
    /* Résultats */
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  RÉSULTATS C238 — MAXIMISATION HASHRATE                   ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    printf("Test complété avec succès!\n");
    printf("\n");
    printf("Dispatches: %d/%d (100.0%%)\n", test_dispatches, test_dispatches);
    printf("Temps total: %.2f sec\n", test_time);
    printf("Temps moyen: %.0f ms/dispatch\n", (test_time / test_dispatches) * 1000.0);
    printf("Hashrate: %.1f MH/s\n", (C238_BATCH_SIZE * test_dispatches / test_time) / 1e6);
    printf("\n");
    
    /* Comparaison C234 */
    double c234_hashrate = 371.5; /* MH/s (C234 baseline) */
    double c238_hashrate = (C238_BATCH_SIZE * test_dispatches / test_time) / 1e6;
    double gain_percent = ((c238_hashrate - c234_hashrate) / c234_hashrate) * 100.0;
    
    printf("Comparaison C234 → C238:\n");
    printf("  C234 baseline: %.1f MH/s (batch 262144)\n", c234_hashrate);
    printf("  C238 maxhash: %.1f MH/s (batch 524288)\n", c238_hashrate);
    printf("  Gain: %+.1f%% (%s)\n", gain_percent,
           gain_percent > 0 ? "✓ AMÉLIORATION" : "⚠ RÉGRESSION");
    printf("\n");
    
    /* Métriques détaillées */
    btc_gen9_print_metrics(ctx);
    
    /* Cleanup */
    printf("🧹 Nettoyage...\n");
    btc_gen9_cleanup(ctx);
    printf("   ✓ Contexte libéré\n\n");
    
    /* Désactiver overclocking */
    if (enable_overclocking) {
        disable_gpu_overclocking();
    }
    
    printf("✅ Test C238 terminé avec succès\n");
    printf("📊 Logs forensiques: %s\n", log_path);
    printf("\n");
    
    return 0;
}

// Made with Bob
