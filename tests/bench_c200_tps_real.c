/**
 * @file bench_c200_tps_real.c
 * @brief Benchmark TPS RÉEL LumVorax - Transactions Blockchain Complètes
 * 
 * OBJECTIF : Mesurer le TPS RÉEL (transactions validées/seconde) sans falsification
 * 
 * Architecture complète :
 * 1. PoH : Horloge cryptographique (287K ticks/s)
 * 2. GPU OpenCL : Vérification parallèle hashes
 * 3. Gulf Stream : Mempool sans cache (1M tx)
 * 4. Pipeline : 4 étapes (Fetch/SigVerify/Banking/Write)
 * 5. VM Sealevel : Exécution parallèle smart contracts
 * 6. Tous les cores CPU : 8 threads (Intel i5-8265U)
 * 
 * MESURE TPS RÉEL :
 * - TPS = transactions_validées / temps_écoulé
 * - Pas de multiplication × 1000 (simulation)
 * - Transactions réelles soumises au pipeline
 * - Validation complète bout-en-bout
 * 
 * @version 1.0.0
 * @date 2026-06-11
 * @author Bob (LumVorax Core Team)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/stat.h>
#include <errno.h>

// Modules LumVorax
#include "../src/consensus/lum_poh.h"
#include "../src/consensus/lum_poh_gpu.h"
#include "../src/pipeline/lum_pipeline.h"
#include "../src/mempool/lum_gulf_stream.h"
#include "../src/vm/lum_sealevel.h"
#include "../src/debug/ultra_forensic_logger.h"

/* ============================================================================
 * CONFIGURATION BENCHMARK
 * ============================================================================ */

#define BENCH_DURATION_SEC 60              // Durée benchmark (60 secondes)
#define BENCH_NUM_THREADS 8                // 8 threads (tous les cores)
#define BENCH_TX_PER_BATCH 1000            // 1000 transactions par batch
#define BENCH_TARGET_TPS 50000             // Cible 50K TPS
#define BENCH_LOG_INTERVAL_SEC 5           // Log toutes les 5 secondes

/* ============================================================================
 * STRUCTURES GLOBALES
 * ============================================================================ */

typedef struct {
    // Composants système
    lum_poh_t* poh;                        // Proof of History
    lum_poh_gpu_context_t* gpu;            // GPU OpenCL
    lum_pipeline_t* pipeline;              // Pipeline 4 étapes
    lum_gulf_stream_t* gulf_stream;        // Mempool Gulf Stream
    lum_sealevel_t* sealevel;              // VM Sealevel
    
    // État benchmark
    volatile bool running;                 // Benchmark actif ?
    uint64_t start_time_ns;                // Timestamp début
    uint64_t end_time_ns;                  // Timestamp fin
    
    // Métriques TPS RÉEL
    volatile uint64_t tx_submitted;        // Transactions soumises
    volatile uint64_t tx_validated;        // Transactions validées
    volatile uint64_t tx_failed;           // Transactions échouées
    
    // Métriques composants
    uint64_t poh_ticks;                    // Ticks PoH générés
    uint64_t gpu_verifications;            // Vérifications GPU
    uint64_t pipeline_throughput;          // Throughput pipeline
    uint64_t sealevel_executed;            // Transactions exécutées VM
    
    // Fichiers logs
    FILE* csv_log;                         // Log CSV temps réel
    FILE* forensic_log;                    // Log forensique
    
} bench_context_t;

static bench_context_t g_bench = {0};

/* ============================================================================
 * UTILITAIRES
 * ============================================================================ */

static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static void signal_handler(int sig) {
    (void)sig;
    printf("\n[SIGNAL] Arrêt benchmark...\n");
    g_bench.running = false;
}

/* ============================================================================
 * GÉNÉRATION TRANSACTIONS RÉELLES
 * ============================================================================ */

/**
 * @brief Générer transaction réelle avec signature SHA-256 VALIDE
 *
 * Format signature[64]:
 *   [0-31]  : prev_hash (32 bytes)
 *   [32-63] : curr_hash = SHA256(prev_hash) (32 bytes)
 */
static bool generate_real_transaction(uint8_t signature[64],
                                      uint8_t** data_out,
                                      uint32_t* size_out) {
    // Générer prev_hash aléatoire (32 bytes)
    uint8_t prev_hash[32];
    for (int i = 0; i < 32; i++) {
        prev_hash[i] = (uint8_t)(rand() % 256);
    }
    
    // Calculer curr_hash = SHA256(prev_hash) pour signature VALIDE
    uint8_t curr_hash[32];
    SHA256(prev_hash, 32, curr_hash);
    
    // Construire signature[64] = prev_hash || curr_hash
    memcpy(signature, prev_hash, 32);      // [0-31]
    memcpy(signature + 32, curr_hash, 32); // [32-63]
    
    // Générer données transaction (100-500 bytes)
    uint32_t size = 100 + (rand() % 400);
    uint8_t* data = (uint8_t*)malloc(size);
    if (!data) return false;
    
    for (uint32_t i = 0; i < size; i++) {
        data[i] = (uint8_t)(rand() % 256);
    }
    
    *data_out = data;
    *size_out = size;
    
    return true;
}

/* ============================================================================
 * THREAD SOUMISSION TRANSACTIONS
 * ============================================================================ */

static void* thread_submit_transactions(void* arg) {
    (void)arg;
    
    printf("[THREAD] Démarrage soumission transactions...\n");
    
    while (g_bench.running) {
        // Générer batch de transactions
        for (int i = 0; i < BENCH_TX_PER_BATCH && g_bench.running; i++) {
            uint8_t signature[64];
            uint8_t* data = NULL;
            uint32_t size = 0;
            
            if (!generate_real_transaction(signature, &data, &size)) {
                continue;
            }
            
            // Soumettre au pipeline
            bool submitted = lum_pipeline_submit(g_bench.pipeline, signature, data, size);
            
            if (submitted) {
                __sync_fetch_and_add(&g_bench.tx_submitted, 1);
            } else {
                __sync_fetch_and_add(&g_bench.tx_failed, 1);
            }
            
            free(data);
        }
        
        // Pause minimale pour maximiser throughput
        usleep(100); // 0.1ms (10× plus rapide)
    }
    
    printf("[THREAD] Arrêt soumission transactions\n");
    return NULL;
}

/* ============================================================================
 * THREAD VALIDATION TRANSACTIONS
 * ============================================================================ */

static void* thread_validate_transactions(void* arg) {
    (void)arg;
    
    printf("[THREAD] Démarrage validation transactions...\n");
    
    while (g_bench.running) {
        // Récupérer métriques pipeline
        uint64_t fetched, verified, executed, written;
        double latency;
        
        lum_pipeline_get_metrics(g_bench.pipeline, &fetched, &verified, 
                                &executed, &written, &latency);
        
        // Transactions validées = transactions écrites
        g_bench.tx_validated = written;
        
        usleep(100000); // 100ms
    }
    
    printf("[THREAD] Arrêt validation transactions\n");
    return NULL;
}

/* ============================================================================
 * THREAD MONITORING TEMPS RÉEL
 * ============================================================================ */

static void* thread_realtime_monitoring(void* arg) {
    (void)arg;
    
    printf("[THREAD] Démarrage monitoring temps réel...\n");
    
    uint64_t last_log_time = get_timestamp_ns();
    uint64_t last_tx_validated = 0;
    
    while (g_bench.running) {
        uint64_t now = get_timestamp_ns();
        uint64_t elapsed_ns = now - last_log_time;
        
        if (elapsed_ns >= BENCH_LOG_INTERVAL_SEC * 1000000000ULL) {
            // Calculer TPS instantané
            uint64_t tx_delta = g_bench.tx_validated - last_tx_validated;
            double elapsed_sec = elapsed_ns / 1000000000.0;
            double tps_instant = tx_delta / elapsed_sec;
            
            // Calculer TPS moyen
            uint64_t total_elapsed_ns = now - g_bench.start_time_ns;
            double total_elapsed_sec = total_elapsed_ns / 1000000000.0;
            double tps_avg = g_bench.tx_validated / total_elapsed_sec;
            
            // Récupérer métriques composants
            double poh_tps = 0;
            double poh_uptime = 0;
            lum_poh_get_metrics(g_bench.poh, &poh_tps, &g_bench.poh_ticks, &poh_uptime);
            
            // Récupérer métriques GPU depuis pipeline
            uint64_t gpu_verif_per_sec = 0;
            if (g_bench.pipeline && elapsed_sec > 0) {
                gpu_verif_per_sec = (uint64_t)(g_bench.pipeline->gpu_verifications / elapsed_sec);
            }
            
            // Récupérer métriques Sealevel depuis pipeline
            uint64_t sealevel_executed = 0;
            if (g_bench.pipeline) {
                sealevel_executed = g_bench.pipeline->sealevel_executions;
            }
            
            // Afficher métriques
            printf("\n╔══════════════════════════════════════════════════════════╗\n");
            printf("║          TPS RÉEL - TEMPS RÉEL (%.0f sec)              ║\n", total_elapsed_sec);
            printf("╠══════════════════════════════════════════════════════════╣\n");
            printf("║ TPS Instantané  : %10.0f tx/s                      ║\n", tps_instant);
            printf("║ TPS Moyen       : %10.0f tx/s                      ║\n", tps_avg);
            printf("║ TX Soumises     : %10lu                            ║\n", g_bench.tx_submitted);
            printf("║ TX Validées     : %10lu                            ║\n", g_bench.tx_validated);
            // BUG-C200-001 FIX : Calcul dérivé des TX perdues
            uint64_t tx_lost = g_bench.tx_submitted - g_bench.tx_validated;
            printf("║ TX Échouées     : %10lu                            ║\n", tx_lost);
            printf("╠══════════════════════════════════════════════════════════╣\n");
            printf("║ PoH Ticks       : %10lu ticks                      ║\n", g_bench.poh_ticks);
            printf("║ GPU Vérif/s     : %10lu verif/s                    ║\n", gpu_verif_per_sec);
            printf("║ Sealevel Exec   : %10lu tx                         ║\n", sealevel_executed);
            printf("╚══════════════════════════════════════════════════════════╝\n");
            
            // Écrire dans CSV
            if (g_bench.csv_log) {
                // BUG-C200-001 FIX : Utiliser tx_lost au lieu de tx_failed
                fprintf(g_bench.csv_log, "%.3f,%lu,%lu,%lu,%.2f,%.2f,%lu,%lu,%lu\n",
                       total_elapsed_sec,
                       g_bench.tx_submitted,
                       g_bench.tx_validated,
                       tx_lost,
                       tps_instant,
                       tps_avg,
                       g_bench.poh_ticks,
                       gpu_verif_per_sec,
                       sealevel_executed);
                fflush(g_bench.csv_log);
            }
            
            last_log_time = now;
            last_tx_validated = g_bench.tx_validated;
        }
        
        usleep(100000); // 100ms
    }
    
    printf("[THREAD] Arrêt monitoring temps réel\n");
    return NULL;
}

/* ============================================================================
 * INITIALISATION BENCHMARK
 * ============================================================================ */

static bool init_benchmark(void) {
    printf("\n╔══════════════════════════════════════════════════════════╗\n");
    printf("║     BENCHMARK TPS RÉEL LUMVORAX - INITIALISATION        ║\n");
    printf("╚══════════════════════════════════════════════════════════╝\n\n");
    
    // Créer répertoire logs (chemin relatif depuis tests/)
    mkdir("../logs", 0755);
    mkdir("../logs/forensic", 0755);
    
    // Ouvrir fichiers logs
    g_bench.csv_log = fopen("../logs/forensic/bench_c200_tps_real.csv", "w");
    if (!g_bench.csv_log) {
        printf("[ERROR] Impossible de créer CSV log\n");
        return false;
    }
    
    fprintf(g_bench.csv_log, "time_sec,tx_submitted,tx_validated,tx_failed,tps_instant,tps_avg,poh_ticks,gpu_verif_per_sec,sealevel_executed\n");
    fflush(g_bench.csv_log);
    
    // 1. Initialiser PoH
    printf("[INIT] Initialisation PoH (287K ticks/s)...\n");
    g_bench.poh = lum_poh_init(287000, true);
    if (!g_bench.poh) {
        printf("[ERROR] Échec initialisation PoH\n");
        return false;
    }
    
    if (!lum_poh_start(g_bench.poh)) {
        printf("[ERROR] Échec démarrage PoH\n");
        return false;
    }
    printf("[OK] PoH démarré\n");
    
    // 2. Initialiser GPU OpenCL
    printf("[INIT] Initialisation GPU OpenCL...\n");
    g_bench.gpu = lum_poh_gpu_init(0, true); // Device 0, fallback CPU
    if (!g_bench.gpu) {
        printf("[WARNING] GPU non disponible, utilisation CPU\n");
    } else {
        printf("[OK] GPU initialisé\n");
        lum_poh_gpu_print_device_info(&g_bench.gpu->device_info);
    }
    
    // 3. Initialiser Pipeline
    printf("[INIT] Initialisation Pipeline (4 étapes)...\n");
    g_bench.pipeline = lum_pipeline_init();
    if (!g_bench.pipeline) {
        printf("[ERROR] Échec initialisation Pipeline\n");
        return false;
    }
    
    // Connecter GPU au pipeline
    g_bench.pipeline->gpu_context = g_bench.gpu;
    printf("[OK] GPU connecté au pipeline\n");
    
    if (!lum_pipeline_start(g_bench.pipeline)) {
        printf("[ERROR] Échec démarrage Pipeline\n");
        return false;
    }
    printf("[OK] Pipeline démarré\n");
    
    // 4. Initialiser Gulf Stream
    printf("[INIT] Initialisation Gulf Stream (mempool)...\n");
    g_bench.gulf_stream = lum_gulf_stream_init(g_bench.poh);
    if (!g_bench.gulf_stream) {
        printf("[ERROR] Échec initialisation Gulf Stream\n");
        return false;
    }
    
    if (!lum_gulf_stream_start(g_bench.gulf_stream)) {
        printf("[ERROR] Échec démarrage Gulf Stream\n");
        return false;
    }
    printf("[OK] Gulf Stream démarré\n");
    
    // 5. Initialiser VM Sealevel
    printf("[INIT] Initialisation VM Sealevel (%d threads)...\n", BENCH_NUM_THREADS);
    g_bench.sealevel = lum_sealevel_init(BENCH_NUM_THREADS);
    if (!g_bench.sealevel) {
        printf("[ERROR] Échec initialisation Sealevel\n");
        return false;
    }
    
    // Connecter Sealevel au pipeline
    g_bench.pipeline->sealevel_vm = g_bench.sealevel;
    printf("[OK] Sealevel initialisé avec %d threads et connecté au pipeline\n", BENCH_NUM_THREADS);
    
    printf("\n[OK] Tous les composants initialisés avec succès\n\n");
    return true;
}

/* ============================================================================
 * NETTOYAGE BENCHMARK
 * ============================================================================ */

static void cleanup_benchmark(void) {
    printf("\n[CLEANUP] Nettoyage composants...\n");
    
    if (g_bench.sealevel) {
        lum_sealevel_free(g_bench.sealevel);
        printf("[OK] Sealevel libéré\n");
    }
    
    if (g_bench.gulf_stream) {
        lum_gulf_stream_stop(g_bench.gulf_stream);
        lum_gulf_stream_free(g_bench.gulf_stream);
        printf("[OK] Gulf Stream libéré\n");
    }
    
    if (g_bench.pipeline) {
        lum_pipeline_stop(g_bench.pipeline);
        lum_pipeline_free(g_bench.pipeline);
        printf("[OK] Pipeline libéré\n");
    }
    
    if (g_bench.gpu) {
        lum_poh_gpu_free(g_bench.gpu);
        printf("[OK] GPU libéré\n");
    }
    
    if (g_bench.poh) {
        lum_poh_stop(g_bench.poh);
        lum_poh_free(g_bench.poh);
        printf("[OK] PoH libéré\n");
    }
    
    if (g_bench.csv_log) {
        fclose(g_bench.csv_log);
        printf("[OK] CSV log fermé\n");
    }
}

/* ============================================================================
 * MAIN
 * ============================================================================ */

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║                                                          ║\n");
    printf("║        LUMVORAX BENCHMARK TPS RÉEL - C200               ║\n");
    printf("║                                                          ║\n");
    printf("║  Mesure TPS RÉEL sans falsification                     ║\n");
    printf("║  Architecture complète bout-en-bout                      ║\n");
    printf("║                                                          ║\n");
    printf("╚══════════════════════════════════════════════════════════╝\n");
    
    // Installer signal handler
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Initialiser benchmark
    if (!init_benchmark()) {
        printf("[ERROR] Échec initialisation benchmark\n");
        return 1;
    }
    
    // Démarrer benchmark
    g_bench.running = true;
    g_bench.start_time_ns = get_timestamp_ns();
    
    printf("\n╔══════════════════════════════════════════════════════════╗\n");
    printf("║              BENCHMARK DÉMARRÉ (%d secondes)            ║\n", BENCH_DURATION_SEC);
    printf("╚══════════════════════════════════════════════════════════╝\n");
    
    // Créer threads
    pthread_t thread_submit, thread_validate, thread_monitor;
    
    pthread_create(&thread_submit, NULL, thread_submit_transactions, NULL);
    pthread_create(&thread_validate, NULL, thread_validate_transactions, NULL);
    pthread_create(&thread_monitor, NULL, thread_realtime_monitoring, NULL);
    
    // Attendre durée benchmark
    sleep(BENCH_DURATION_SEC);
    
    // Arrêter benchmark
    g_bench.running = false;
    g_bench.end_time_ns = get_timestamp_ns();
    
    // Attendre threads
    pthread_join(thread_submit, NULL);
    pthread_join(thread_validate, NULL);
    pthread_join(thread_monitor, NULL);
    
    // Calculer résultats finaux
    double total_time_sec = (g_bench.end_time_ns - g_bench.start_time_ns) / 1000000000.0;
    double tps_final = g_bench.tx_validated / total_time_sec;
    double success_rate = (g_bench.tx_submitted > 0) ?
                         (100.0 * g_bench.tx_validated / g_bench.tx_submitted) : 0.0;
    
    // BUG-C200-001 FIX : Calcul dérivé des TX perdues (résultats finaux)
    uint64_t tx_lost_final = g_bench.tx_submitted - g_bench.tx_validated;
    
    // Afficher résultats finaux
    printf("\n\n");
    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║              RÉSULTATS FINAUX TPS RÉEL                  ║\n");
    printf("╠══════════════════════════════════════════════════════════╣\n");
    printf("║ Durée totale    : %.2f secondes                         ║\n", total_time_sec);
    printf("║ TX Soumises     : %lu                                   ║\n", g_bench.tx_submitted);
    printf("║ TX Validées     : %lu                                   ║\n", g_bench.tx_validated);
    printf("║ TX Échouées     : %lu                                   ║\n", tx_lost_final);
    printf("║ Taux succès     : %.2f%%                                ║\n", success_rate);
    printf("╠══════════════════════════════════════════════════════════╣\n");
    printf("║ TPS RÉEL FINAL  : %.0f tx/s                            ║\n", tps_final);
    printf("╠══════════════════════════════════════════════════════════╣\n");
    printf("║ PoH Ticks       : %lu ticks                             ║\n", g_bench.poh_ticks);
    printf("║ GPU Vérif       : %lu hashes                            ║\n", g_bench.gpu ? g_bench.gpu->total_hashes_verified : 0);
    printf("║ Sealevel Exec   : %lu tx                                ║\n", g_bench.sealevel ? g_bench.sealevel->total_executed : 0);
    printf("╚══════════════════════════════════════════════════════════╝\n");
    
    printf("\n[INFO] Logs sauvegardés : ../logs/forensic/bench_c200_tps_real.csv\n");
    
    // Nettoyage
    cleanup_benchmark();
    
    printf("\n[OK] Benchmark terminé avec succès\n\n");
    
    return 0;
}

// Made with Bob - LumVorax TPS RÉEL Benchmark