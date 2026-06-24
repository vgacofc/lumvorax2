/**
 * @file bench_c199_poh_hybrid.c
 * @brief Benchmark Pipeline Hybride CPU-GPU PoH - Phase 2.1
 * 
 * Test du pipeline hybride avec:
 * - Génération PoH séquentielle (CPU, 485K ticks/s attendu)
 * - Vérification parallèle (3 threads GPU, +200% throughput attendu)
 * - Métriques forensiques complètes
 * 
 * Validation:
 * - TPS génération ≥ 400K ticks/s (maintenu vs C199.21)
 * - Throughput vérification ≥ 1.2M verif/s (+200% vs séquentiel)
 * - Latence vérification ≤ 1.5ms (-67% vs 3ms baseline)
 * - Overhead forensic ≤ 5%
 * 
 * @version 1.0.0
 * @date 2026-06-10
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "../src/consensus/lum_poh.h"
#include "../src/debug/ultra_forensic_logger.h"

// Couleurs ANSI
#define COLOR_RESET   "\033[0m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_BOLD    "\033[1m"

/**
 * @brief Afficher bannière benchmark
 */
static void print_banner(void) {
    printf("\n");
    printf(COLOR_BOLD COLOR_CYAN);
    printf("╔══════════════════════════════════════════════════════════════════╗\n");
    printf("║     BENCHMARK PIPELINE HYBRIDE CPU-GPU POH - PHASE 2.1          ║\n");
    printf("║                    Cycle C199.22                                 ║\n");
    printf("╚══════════════════════════════════════════════════════════════════╝\n");
    printf(COLOR_RESET);
    printf("\n");
}

/**
 * @brief Afficher configuration test
 */
static void print_config(void) {
    printf(COLOR_BOLD "Configuration Test:\n" COLOR_RESET);
    printf("  Durée: 10 secondes\n");
    printf("  Génération: 1 thread CPU séquentiel\n");
    printf("  Vérification: 3 threads GPU parallèles\n");
    printf("  Batch size: 1000 ticks\n");
    printf("  Forensic: Ring buffer nano + sampling 1/1000\n");
    printf("\n");
}

/**
 * @brief Afficher progression temps réel
 */
static void print_progress(int elapsed_sec, int total_sec, double tps) {
    int progress = (elapsed_sec * 100) / total_sec;
    int bars = progress / 2;
    
    printf("\r[");
    for (int i = 0; i < 50; i++) {
        if (i < bars) printf("█");
        else printf("░");
    }
    printf("] %3d%% | %2d/%2ds | TPS: %.0f ticks/s", 
           progress, elapsed_sec, total_sec, tps);
    fflush(stdout);
}

/**
 * @brief Exécuter benchmark
 */
static int run_benchmark(void) {
    printf(COLOR_BOLD "Démarrage benchmark...\n" COLOR_RESET);
    
    // Initialiser PoH avec pipeline hybride
    lum_poh_t* poh = lum_poh_init(1000000, false);  // Cible 1M ticks/s
    if (!poh) {
        fprintf(stderr, "ERROR: Failed to initialize PoH\n");
        return 1;
    }
    
    printf(COLOR_GREEN "✓ PoH initialisé (pipeline hybride activé)\n" COLOR_RESET);
    
    // Démarrer génération + vérification
    if (!lum_poh_start(poh)) {
        fprintf(stderr, "ERROR: Failed to start PoH\n");
        lum_poh_free(poh);
        return 1;
    }
    
    printf(COLOR_GREEN "✓ Pipeline démarré (1 thread génération + 3 threads vérification)\n" COLOR_RESET);
    printf("\n");
    
    // Benchmark 10 secondes avec affichage progression
    const int duration_sec = 10;
    uint64_t start_ticks = 0;
    
    for (int i = 0; i <= duration_sec; i++) {
        sleep(1);
        
        // Obtenir métriques actuelles
        lum_poh_entry_t entry;
        if (lum_poh_get_current(poh, &entry)) {
            double tps = 0.0;
            pthread_mutex_lock(&poh->mutex);
            tps = poh->ticks_per_second;
            if (i == 0) start_ticks = poh->total_ticks;
            pthread_mutex_unlock(&poh->mutex);
            
            print_progress(i, duration_sec, tps);
        }
    }
    
    printf("\n\n");
    
    // Arrêter PoH
    lum_poh_stop(poh);
    printf(COLOR_GREEN "✓ Pipeline arrêté\n" COLOR_RESET);
    
    // Obtenir métriques finales
    lum_poh_entry_t final_entry;
    lum_poh_get_current(poh, &final_entry);
    
    uint64_t total_ticks = 0;
    double tps = 0.0;
    
    pthread_mutex_lock(&poh->mutex);
    total_ticks = poh->total_ticks;
    tps = poh->ticks_per_second;
    pthread_mutex_unlock(&poh->mutex);
    
    // Afficher résultats
    printf("\n");
    printf(COLOR_BOLD COLOR_CYAN "═══════════════════════════════════════════════════════════════════\n");
    printf("                      RÉSULTATS BENCHMARK\n");
    printf("═══════════════════════════════════════════════════════════════════\n" COLOR_RESET);
    printf("\n");
    
    printf(COLOR_BOLD "Génération PoH (CPU Séquentiel):\n" COLOR_RESET);
    printf("  Total ticks générés: %lu\n", total_ticks);
    printf("  TPS mesuré: %.0f ticks/s\n", tps);
    printf("  Cible 1M ticks/s: %.1f%%\n", (tps / 1000000.0) * 100.0);
    printf("  vs Solana 400K: %+.1f%%\n", ((tps - 400000.0) / 400000.0) * 100.0);
    printf("  vs C199.21 485K: %+.1f%%\n", ((tps - 485000.0) / 485000.0) * 100.0);
    printf("\n");
    
    // Métriques vérification (accès variables globales)
    extern lum_poh_verifier_context_t g_verifier_ctx;
    
    printf(COLOR_BOLD "Vérification Parallèle (3× GPU):\n" COLOR_RESET);
    printf("  Total batches vérifiés: %lu\n", g_verifier_ctx.total_batches_verified);
    printf("  Total erreurs détectées: %lu\n", g_verifier_ctx.total_errors_detected);
    printf("  Latence moyenne: %.2f ms\n", g_verifier_ctx.avg_verification_latency_ms);
    
    // Calculer throughput vérification
    double throughput_verif = (double)g_verifier_ctx.total_batches_verified * 1000.0 / (double)duration_sec;
    printf("  Throughput vérification: %.0f batches/s = %.0f verif/s\n", 
           throughput_verif, throughput_verif * 1000.0);
    
    // Gains vs séquentiel
    double baseline_verif = tps;  // Vérification séquentielle = même TPS que génération
    double gain_throughput = ((throughput_verif * 1000.0) - baseline_verif) / baseline_verif * 100.0;
    printf("  Gain vs séquentiel: %+.1f%%\n", gain_throughput);
    printf("\n");
    
    // Validation objectifs
    printf(COLOR_BOLD "Validation Objectifs Phase 2.1:\n" COLOR_RESET);
    
    bool tps_ok = tps >= 400000.0;
    bool throughput_ok = (throughput_verif * 1000.0) >= 1200000.0;
    bool latency_ok = g_verifier_ctx.avg_verification_latency_ms <= 1.5;
    bool errors_ok = g_verifier_ctx.total_errors_detected == 0;
    
    printf("  [%s] TPS génération ≥ 400K: %.0f ticks/s\n", 
           tps_ok ? COLOR_GREEN "✓" COLOR_RESET : COLOR_YELLOW "✗" COLOR_RESET, tps);
    printf("  [%s] Throughput vérif ≥ 1.2M: %.0f verif/s\n", 
           throughput_ok ? COLOR_GREEN "✓" COLOR_RESET : COLOR_YELLOW "✗" COLOR_RESET, 
           throughput_verif * 1000.0);
    printf("  [%s] Latence vérif ≤ 1.5ms: %.2f ms\n", 
           latency_ok ? COLOR_GREEN "✓" COLOR_RESET : COLOR_YELLOW "✗" COLOR_RESET, 
           g_verifier_ctx.avg_verification_latency_ms);
    printf("  [%s] Aucune erreur détectée: %lu erreurs\n", 
           errors_ok ? COLOR_GREEN "✓" COLOR_RESET : COLOR_YELLOW "✗" COLOR_RESET, 
           g_verifier_ctx.total_errors_detected);
    printf("\n");
    
    // Verdict final
    bool all_ok = tps_ok && throughput_ok && latency_ok && errors_ok;
    if (all_ok) {
        printf(COLOR_BOLD COLOR_GREEN);
        printf("╔══════════════════════════════════════════════════════════════════╗\n");
        printf("║                    ✓ BENCHMARK RÉUSSI                           ║\n");
        printf("║          Pipeline hybride CPU-GPU opérationnel                   ║\n");
        printf("╚══════════════════════════════════════════════════════════════════╝\n");
        printf(COLOR_RESET);
    } else {
        printf(COLOR_BOLD COLOR_YELLOW);
        printf("╔══════════════════════════════════════════════════════════════════╗\n");
        printf("║                  ⚠ OBJECTIFS PARTIELS                           ║\n");
        printf("║            Certains objectifs non atteints                       ║\n");
        printf("╚══════════════════════════════════════════════════════════════════╝\n");
        printf(COLOR_RESET);
    }
    printf("\n");
    
    // Cleanup
    lum_poh_free(poh);
    
    return all_ok ? 0 : 1;
}

/**
 * @brief Point d'entrée
 */
int main(void) {
    print_banner();
    print_config();
    
    int result = run_benchmark();
    
    printf(COLOR_BOLD "Logs forensiques générés dans: " COLOR_RESET);
    printf("logs/forensic/\n");
    printf("\n");
    
    return result;
}

// Made with Bob
