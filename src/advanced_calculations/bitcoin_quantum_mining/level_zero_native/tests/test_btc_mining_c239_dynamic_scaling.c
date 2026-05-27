/*
 * LumVorax — Module 17 — Bitcoin Quantum Mining Engine
 * test_btc_mining_c239_dynamic_scaling.c — Test Augmentation Dynamique Batch Size
 *
 * CYCLE C239 — Découverte Limite Maximale GPU
 * STANDARD_NAMES.md v4.6 §M-BTC-TEST-C239-DYNAMIC
 *
 * OBJECTIF : DÉCOUVRIR BATCH SIZE MAXIMUM PAR DISPATCH
 * - Augmentation progressive : 262144 → 524288 → 1048576 → ...
 * - Test jusqu'à échec ou limite GPU
 * - Maximum 100 dispatches
 * - Traçabilité forensique complète
 *
 * STRATÉGIE :
 * - Dispatch 1 : 262144 hashes (baseline C234)
 * - Dispatch 2 : 524288 hashes (×2)
 * - Dispatch 3 : 1048576 hashes (×4)
 * - Dispatch 4 : 2097152 hashes (×8)
 * - ...
 * - Dispatch N : Jusqu'à échec GPU ou 100 dispatches
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>

/* Interface Gen9 Native */
extern void* btc_gen9_init(const char* kernel_path, const char* log_path, uint32_t batch_size);
extern int btc_gen9_execute(void* ctx);
extern void btc_gen9_cleanup(void* ctx);
extern void btc_gen9_print_metrics(const void* ctx);

/* Accès interne pour modifier batch_size dynamiquement */
typedef struct {
    int drm_fd;
    uint32_t ctx_id;
    uint32_t ctx_pool[3];
    int ctx_pool_index;
    uint32_t batch_bo;
    uint32_t kernel_bo;
    uint32_t input_bo;
    uint32_t output_bo;
    void* batch_map;
    void* kernel_map;
    void* input_map;
    void* output_map;
    size_t batch_size;
    size_t kernel_size;
    size_t input_size;
    size_t output_size;
    uint32_t nonce_start;
    uint64_t total_dispatches;
    uint64_t total_hashes;
    double total_time_sec;
    int thermal_throttle_count;
    int last_gpu_temp_celsius;
    void* async_save_thread;
    void* save_mutex;
    int async_save_active;
    uint32_t* async_save_buffer;
    size_t async_save_size;
    int async_save_pending;
    FILE* log_file;
    uint64_t event_counter;
} btc_gen9_context_internal_t;

/* ══════════════════════════════════════════════════════════════════════
 * SECTION 1 : CONFIGURATION C239
 * ══════════════════════════════════════════════════════════════════════ */

#define C239_BASELINE_BATCH 262144    /* Baseline C234 */
#define C239_MAX_DISPATCHES 100       /* Maximum 100 dispatches */
#define C239_SCALING_FACTOR 2.0       /* Facteur multiplication (×2 chaque dispatch) */

/* Chemins */
#define KERNEL_PATH "kernels/btc_sha256_gen9.bin"
#define LOG_PATH "logs/forensic/btc_mining_c239_dynamic_scaling.log"

/* ══════════════════════════════════════════════════════════════════════
 * SECTION 2 : FONCTION PRINCIPALE
 * ══════════════════════════════════════════════════════════════════════ */

int main(int argc, char** argv) {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  LumVorax C239 — Test Augmentation Dynamique Batch Size   ║\n");
    printf("║  Découverte Limite Maximale GPU                           ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    
    printf("Configuration:\n");
    printf("  Batch size initial: %u hashes\n", C239_BASELINE_BATCH);
    printf("  Facteur scaling: ×%.1f par dispatch\n", C239_SCALING_FACTOR);
    printf("  Maximum dispatches: %d\n", C239_MAX_DISPATCHES);
    printf("  Log: %s\n", LOG_PATH);
    printf("\n");
    
    printf("Stratégie:\n");
    printf("  Dispatch 1: %u hashes (baseline)\n", C239_BASELINE_BATCH);
    printf("  Dispatch 2: %u hashes (×2)\n", (uint32_t)(C239_BASELINE_BATCH * 2));
    printf("  Dispatch 3: %u hashes (×4)\n", (uint32_t)(C239_BASELINE_BATCH * 4));
    printf("  Dispatch 4: %u hashes (×8)\n", (uint32_t)(C239_BASELINE_BATCH * 8));
    printf("  ...\n");
    printf("  Dispatch N: Jusqu'à échec GPU\n");
    printf("\n");
    
    /* Initialiser contexte avec batch size baseline */
    printf("📊 Initialisation contexte Gen9 Native...\n");
    void* ctx = btc_gen9_init(KERNEL_PATH, LOG_PATH, C239_BASELINE_BATCH);
    if (!ctx) {
        fprintf(stderr, "❌ Échec initialisation\n");
        return 1;
    }
    printf("   ✓ Contexte initialisé (batch_size=%u)\n\n", C239_BASELINE_BATCH);
    
    /* Cast vers structure interne pour accès batch_size */
    btc_gen9_context_internal_t* ctx_internal = (btc_gen9_context_internal_t*)ctx;
    
    /* Tableau résultats */
    typedef struct {
        int dispatch_id;
        uint32_t batch_size;
        double time_sec;
        bool success;
        double hashrate_mhs;
    } dispatch_result_t;
    
    dispatch_result_t results[C239_MAX_DISPATCHES];
    int total_success = 0;
    int total_failed = 0;
    
    /* Boucle dispatches avec augmentation dynamique */
    printf("🚀 Démarrage test augmentation dynamique...\n\n");
    
    for (int i = 0; i < C239_MAX_DISPATCHES; i++) {
        /* Calculer nouveau batch size */
        uint32_t current_batch = (uint32_t)(C239_BASELINE_BATCH * pow(C239_SCALING_FACTOR, i));
        
        /* Vérifier limite mémoire (4 GB max pour Gen9) */
        size_t required_memory = current_batch * 32; /* 32 bytes par hash SHA256 */
        if (required_memory > 1073741824) { /* 1 GB limite sécurité */
            printf("⚠ Dispatch %d : Batch size %u dépasse limite mémoire (1 GB)\n", 
                   i + 1, current_batch);
            printf("   Arrêt test à dispatch %d\n\n", i);
            break;
        }
        
        /* Modifier batch_size dans contexte */
        ctx_internal->batch_size = current_batch;
        
        printf("📦 Dispatch %d/%d : batch_size=%u hashes (×%.0f)\n", 
               i + 1, C239_MAX_DISPATCHES, current_batch, pow(C239_SCALING_FACTOR, i));
        
        /* Exécuter dispatch */
        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);
        
        int ret = btc_gen9_execute(ctx);
        
        clock_gettime(CLOCK_MONOTONIC, &end);
        double dispatch_time = (end.tv_sec - start.tv_sec) + 
                               (end.tv_nsec - start.tv_nsec) / 1e9;
        
        /* Enregistrer résultat */
        results[i].dispatch_id = i + 1;
        results[i].batch_size = current_batch;
        results[i].time_sec = dispatch_time;
        results[i].success = (ret == 0);
        results[i].hashrate_mhs = (ret == 0) ? (current_batch / dispatch_time) / 1e6 : 0.0;
        
        if (ret == 0) {
            total_success++;
            printf("   ✓ Succès : %.3f sec (%.1f MH/s)\n\n", 
                   dispatch_time, results[i].hashrate_mhs);
        } else {
            total_failed++;
            printf("   ✗ ÉCHEC : GPU ne peut pas gérer batch_size=%u\n", current_batch);
            printf("   Limite maximale trouvée : batch_size=%u (dispatch %d)\n\n",
                   results[i-1].batch_size, i);
            break;
        }
        
        /* Pause sécurité entre dispatches */
        usleep(100000); /* 100 ms */
    }
    
    /* Résultats finaux */
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  RÉSULTATS C239 — AUGMENTATION DYNAMIQUE                  ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    
    printf("Dispatches réussis: %d/%d (%.1f%%)\n", 
           total_success, total_success + total_failed,
           (total_success * 100.0) / (total_success + total_failed));
    printf("Dispatches échoués: %d\n", total_failed);
    printf("\n");
    
    /* Tableau résultats */
    printf("┌─────────┬──────────────┬───────────┬──────────┬──────────┐\n");
    printf("│ Dispatch│ Batch Size   │ Temps (s) │ Status   │ MH/s     │\n");
    printf("├─────────┼──────────────┼───────────┼──────────┼──────────┤\n");
    
    for (int i = 0; i < total_success + total_failed; i++) {
        printf("│ %7d │ %12u │ %9.3f │ %8s │ %8.1f │\n",
               results[i].dispatch_id,
               results[i].batch_size,
               results[i].time_sec,
               results[i].success ? "✓ OK" : "✗ FAIL",
               results[i].hashrate_mhs);
    }
    
    printf("└─────────┴──────────────┴───────────┴──────────┴──────────┘\n");
    printf("\n");
    
    /* Analyse limite maximale */
    if (total_success > 0) {
        dispatch_result_t* best = &results[total_success - 1];
        printf("🏆 LIMITE MAXIMALE DÉCOUVERTE:\n");
        printf("   Batch size max: %u hashes/dispatch\n", best->batch_size);
        printf("   Temps dispatch: %.3f sec\n", best->time_sec);
        printf("   Hashrate max: %.1f MH/s\n", best->hashrate_mhs);
        printf("   Facteur vs baseline: ×%.0f\n", 
               (double)best->batch_size / C239_BASELINE_BATCH);
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
    }
    
    /* Métriques détaillées */
    btc_gen9_print_metrics(ctx);
    
    /* Cleanup */
    printf("🧹 Nettoyage...\n");
    btc_gen9_cleanup(ctx);
    printf("   ✓ Contexte libéré\n\n");
    
    printf("✅ Test C239 terminé\n");
    printf("📊 Logs forensiques: %s\n", LOG_PATH);
    printf("\n");
    
    return 0;
}

// Made with Bob
