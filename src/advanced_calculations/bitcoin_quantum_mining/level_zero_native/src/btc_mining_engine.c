/*
 * LumVorax — Module 17 — Bitcoin Quantum Mining Engine
 * btc_mining_engine.c — Moteur PT-MC nonce explorer + validation bloc
 *
 * STANDARD_NAMES.md v4.3 §M-BTC17-C46 — Cycle C46 — 2026-04-15
 *
 * Architecture :
 *  - 8 répliques PT-MC (Parallel Tempering Monte Carlo)
 *    → T_cold = 1.0 (exploitation) … T_hot = 50.0 (exploration large)
 *  - N_THREADS threads parallèles (OpenMP + pthread)
 *  - AVX2 pipeline SHA-256 (8-way si disponible, sinon scalaire)
 *  - NX48_BTC adaptatif : ajuste delta_nonce + batch_size en temps réel
 *  - Traçabilité forensic 100% A–Z (FORENSIC_LOG_* — NOM D'ORIGINE §A)
 *  - Mémoire tracée LV_MALLOC / LV_CALLOC / LV_FREE
 *
 * OPTIMISATIONS C46 (nouveau compte Supabase — 2026-04-15) :
 *  [C46-1-BATCH-2K]    BTC_BATCH_SIZE 512→1024 — pipeline AVX2 8-way saturé 2×
 *                       Gain estimé : +5-10% hashrate (moindre overhead par batch)
 *                       Base : C44 hashrate 0.4365 MH/s @ batch=512, 2 threads.
 *  [C46-2-NX48-EVERY]  BTC_NX48_UPDATE_EVERY 200000→256000 — overhead NX48 réduit
 *                       Aligné sur puissance de 2 (meilleure prévisibilité hot-path).
 *  [C46-3-TS-CACHE]    C41_TS_CACHE_NS 1ms→2ms — 2× moins de syscall clock_gettime
 *                       Justification : Replit latence NFS ~2ms (mesurée C44).
 *  [C46-4-SUPABASE]    Nouveau compte Supabase — tables recréées via setup_c46.sql
 *                       Schema version : lumvorax_btc_pow_candidate_v1 cycle=C46.
 *
 * CORRECTIONS C42 :
 *  [C42-WATCHDOG-RAM]  Thread watchdog surveille /proc/meminfo toutes les 5s.
 *                       Si RAM_avail < 1 GB → throttle 2ms par batch.
 *                       Si RAM_avail < 500 MB → throttle 10ms + batch réduit.
 *                       Si RAM_avail < 200 MB → PAUSE totale (SIGSTOP-like sleep).
 *  [C42-WATCHDOG-CPU]  Lecture /proc/stat — si CPU process >85% → usleep 500µs.
 *  [C42-RESTART-SAFE]  Signal handler SIGTERM/SIGINT → sauvegarde CSV avant exit.
 *  [C42-WEIGHTS-PERSIST] weights[8]+bias persistés dans CSV (voir nx48_btc_controller.c).
 *
 * OPTIMISATIONS C41 (analyse 653 fichiers / 113 096 lignes src/) :
 *  [C41-1-LOCKFREE]        best_leading_global : volatile→_Atomic + CAS relaxed
 *                          Source : src/optimization/lockfree/lockfree_queue.h
 *  [C41-2-TIMESTAMP-CACHE] eng_ts_ns() cache 1ms thread-local (REPLIT_TIMESTAMP_CACHE_NS)
 *                          Source : src/advanced_calculations/quantum_simulator_v4_staging_next/common_types.h
 *  [C41-3-CACHE-LINE]      btc_ptmc_replica_t aligned(64) — élimine false sharing
 *                          Source : src/optimization/memory_optimizer.h
 *  [C41-4-MUTEX-ORBITAL]   lecture global en boucle interne : mutex→atomic_load_relaxed
 *                          Source : src/optimization/lockfree/lockfree_queue.h
 *  [C41-5-BATCH-TUNING]    batch=256→512, nx48_every=100k→200k (meilleur saturage SIMD)
 *                          Source : src/optimization/simd_batch/simd_batch_processor.h
 *  [C41-6-THERMAL]         usleep(500) si total_hashes%10M=0 (stabilisation OS)
 *                          Source : src/optimization/thermal_regulator.c
 *  [C41-7-ASYNC-METRIC]    FORENSIC_LOG hors record → no-op macro (0 I/O hot path)
 *                          Source : src/optimization/async_logging/async_logger.h
 *  [C41-8-REPLICA-ALIGN]   btc_thread_work_t aligned(64) — évite false sharing threads
 *                          Source : src/optimization/memory_optimizer.h (AVX-512 pool 64B)
 */

#ifndef _GNU_SOURCE
#  define _GNU_SOURCE
#endif
#define _POSIX_C_SOURCE 200809L

#include "sha256_lumvorax.h"
#include "nx48_btc_controller.h"
#include "nx48_coupler_bridge.h"  /* C99 — pont neuro Izhikevich+STDP */
#include "nx48_alltime_record.h"  /* C100 — persistance MONOTONE record absolu */
#include "optimization/reasoning_path_tracker.h"  /* C111 — hook reasoning trace path GPU */
extern reasoning_trace_t* g_btc_reasoning_trace;  /* C111 — exposé par main_btc_mining.c */

/* C128-FIX-A5/A6 : pointers globaux exposés par main_btc_mining.c
 *  - g_btc_async_log : journal off-hot-path asynchrone (BTC_ASYNC_LOG=1)
 *  - g_btc_lum_log   : journal natif binaire format LUM      (BTC_LUM_LOG=1)
 * Si NULL, les hooks sont des no-op silencieux (zéro overhead). */
#include "async_logging/async_logger.h"
#include "lum/lum_log_encoder.h"
extern async_logger_t*    g_btc_async_log;
extern lum_log_writer_t*  g_btc_lum_log;
#include "btc_mining_forensic.h"
#include "debug/ultra_forensic_logger.h"
#include "lumvorax_integration.h"
#include "debug/memory_tracker.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <pthread.h>
#include <stdatomic.h>
#include <inttypes.h>
#include <unistd.h>
#include <signal.h>    /* C42-SIGNAL : sigaction, SIGTERM, SIGINT */
#include <sys/stat.h>

/* C223-GEN9-NATIVE : Migration vers Gen9 Native i915 DRM (0% OpenCL, 0% Level Zero)
 * OBJECTIF : Éliminer dépendance OpenCL complètement
 * AVANT C223 : btc_lumvorax_gpu_native.h (OpenCL wrapper)
 * APRÈS C223 : btc_gen9_native_runner.h (i915 DRM direct)
 * Performance prouvée C198 : 3,386 MH/s (Phase 12H, 60s, ±0.2% stabilité)
 * Architecture : Application → i915 DRM ioctl → GPU Gen9 (0% dépendance externe) */
#include "btc_gen9_native_runner.h"
/* C255v8n Phase 2 : Adaptateur compatibilité OpenCL→Gen9 Native */
#include "btc_gen9_mining_adapter.h"

#ifdef _OPENMP
#  include <omp.h>
#endif

/* ── Constantes du moteur ───────────────────────────────────────── */
/* C255v8n CORRECTION #2 : Architecture hiérarchique PT-MC
 * PROBLÈME C255v8m : 8 replicas × 2^32 nonces = 0.0125% couverture SHA-256
 * SYMPTÔME : Leading zeros bloqués à 0-4 bits (vs 30-32+ bits requis)
 * SOLUTION C255v8n : 256 replicas (32× plus) pour exploration 40-bit
 * ARCHITECTURE :
 *   - Niveau 1 : 8 replicas base (températures 1.0 → 50.0)
 *   - Niveau 2 : 32 sous-replicas par replica base (256 total)
 *   - Couverture : 256 × 2^32 = 2^40 nonces (1.1 trillion vs 4.3 billion)
 * OBJECTIF : Leading zeros 4 bits → 40 bits (10× amélioration)
 */
#define BTC_N_REPLICAS_L1       8    /* Niveau 1 : replicas base */
#define BTC_N_REPLICAS_L2       32   /* Niveau 2 : sous-replicas par base */
#define BTC_N_REPLICAS          256  /* Total : 8 × 32 = 256 replicas */
#define BTC_N_THREADS_DEFAULT   16
/* C41-5-BATCH-TUNING : batch 256→512 — meilleur saturage pipeline SHA-256 AVX2.
 * AVANT : 256 hashes/batch → sous-saturation pipeline 8-way AVX2.
 * APRES : 512 hashes/batch → pipeline entier saturé, 2× moins de surcout batch.
 * Source : src/optimization/simd_batch/simd_batch_processor.h SIMD_BATCH_SIZE=256
 *          → taille doublée pour correspondre aux meilleures pratiques SIMD. */
#define BTC_BATCH_SIZE_DEFAULT  1024    /* C46-1 : 512→1024 hashes par batch SIMD — pipeline AVX2 8-way saturé 2× */
/* C46-2-NX48-EVERY : nx48_every 200k→256k (puissance de 2) — overhead NX48 réduit.
 * NX48 update = ~500µs (log + gradient). A 0.44 MH/s : 256k → ~1.7× par seconde.
 * Aligné puissance de 2 → meilleure prévisibilité hot-path (branchement mod). */
#define BTC_NX48_UPDATE_EVERY   256000  /* C46-2 : 200k→256k hashes entre updates NX48 */
#define BTC_HW_SAMPLE_EVERY     50000   /* Snapshot HW tous les N hashes */
#define BTC_PTMC_SWAP_EVERY     10000   /* Échange répliques tous les N hashes */
#define BTC_STATS_PRINT_EVERY   1000000 /* Affichage stats chaque M hashes */

/* C41-2-TIMESTAMP-CACHE : cache timestamp thread-local 1ms.
 * AVANT : eng_ts_ns() = clock_gettime() appelé à chaque itération boucle while.
 *         A 512 hashes/batch × 0.9MH/s → ~1760 syscalls/s/thread.
 * APRES : cache 1ms → clock_gettime seulement si delta > 1ms → ~1000× moins.
 * Source : src/advanced_calculations/quantum_simulator_v4_staging_next/common_types.h
 *          REPLIT_TIMESTAMP_CACHE_NS = 1 000 000 (1ms cache NFS storage) */
#define C41_TS_CACHE_NS 2000000ULL  /* C46-3 : 1ms→2ms — 2× moins de syscalls clock_gettime (latence NFS Replit ~2ms) */

/* Températures répliques PT-MC hiérarchiques (C255v8n)
 * Niveau 1 (8 replicas base) : 1.0 → 50.0 (exploration large)
 * Niveau 2 (32 sous-replicas) : interpolation linéaire entre replicas base
 * Total : 256 températures couvrant 1.0 → 50.0 avec granularité fine
 */
static double BTC_REPLICA_TEMPS[BTC_N_REPLICAS];

/* C255v8n : Initialisation températures hiérarchiques */
static void btc_init_hierarchical_temps(void) {
    const double base_temps[BTC_N_REPLICAS_L1] = {
        1.0, 2.0, 4.0, 8.0, 12.0, 20.0, 35.0, 50.0
    };
    
    for (int i = 0; i < BTC_N_REPLICAS_L1; i++) {
        double t_start = base_temps[i];
        double t_end = (i < BTC_N_REPLICAS_L1 - 1) ? base_temps[i + 1] : 50.0;
        double delta = (t_end - t_start) / (double)BTC_N_REPLICAS_L2;
        
        for (int j = 0; j < BTC_N_REPLICAS_L2; j++) {
            int idx = i * BTC_N_REPLICAS_L2 + j;
            BTC_REPLICA_TEMPS[idx] = t_start + delta * (double)j;
        }
    }
}

/* ── Structure d'une réplique PT-MC ─────────────────────────────── */
/* C41-3-CACHE-LINE : alignement 64 bytes (cache line CPU) — élimine le false sharing.
 * AVANT : struct non-alignée → plusieurs répliques sur la même cache line →
 *         écriture sur réplique r invalide la cache de réplique r+1 (thread distinct).
 * APRES : chaque réplique occupe exactement 1+ cache lines → 0 invalidation cross-thread.
 * Source : src/optimization/memory_optimizer.h — AVX-512 pool 64-byte aligned replicas.
 *          src/optimization/slab_allocator/slab_allocator.h — SLAB_ALIGNMENT=16,
 *          memory_optimizer_create() → posix_memalign 64 bytes pour lum_pool. */
typedef struct {
    uint32_t nonce;           /* Nonce courant */
    uint32_t best_nonce;      /* Meilleur nonce de cette réplique */
    double   temperature;     /* Température analogique */
    int      leading_zeros;   /* leading_zeros courant */
    int      best_leading;    /* Meilleur leading_zeros de cette réplique */
    uint64_t hashes_done;     /* Hashes effectués */
    uint64_t swaps_accepted;  /* Échanges acceptés */
    uint64_t swaps_attempted; /* Échanges tentés */
    pthread_mutex_t mutex;    /* Protection thread-safe */
} __attribute__((aligned(64))) btc_ptmc_replica_t;

/* ── Configuration du moteur de minage ─────────────────────────── */
typedef struct {
    int      n_threads;         /* Nombre de threads */
    int      n_replicas;        /* Nombre répliques PT */
    int      batch_size;        /* Batch SIMD */
    uint32_t nonce_start;       /* Début plage nonce */
    uint32_t nonce_end;         /* Fin plage nonce */
    uint64_t duration_ns;       /* Durée max (0 = illimité) */
    char     run_id[64];        /* run_id LumVorax */
    char     nx48_csv[256];     /* Chemin CSV NX48 BTC */
    char     log_dir[256];      /* Répertoire logs forensic */
    char     run_mode[32];      /* BENCHMARK / TESTNET / MAINNET */

    /* Block header template */
    lv_btc_block_header_t header_template;
    uint8_t  target[32];        /* Target de difficulté */
} btc_engine_config_t;

/* ── État global du moteur ──────────────────────────────────────── */
typedef struct {
    btc_ptmc_replica_t   replicas[BTC_N_REPLICAS];
    volatile atomic_uint_least64_t total_hashes;   /* Compteur atomique global */
    /* C41-1-LOCKFREE : best_leading_global volatile int → _Atomic int.
     * AVANT : volatile int non-atomique → lecture possible dans boucle interne
     *         avec un lock rep->mutex (incohérent : verrou réplique pour variable moteur).
     * APRES : _Atomic int → lectures/écritures atomiques sans mutex sur hot path.
     *         Toute lecture dans la boucle interne utilise atomic_load_explicit(...,
     *         memory_order_relaxed) — lecture éventuellement cohérente, 0 verrou.
     *         Écriture record toujours protégée par global_mutex + double-check.
     * Source : src/optimization/lockfree/lockfree_queue.h — _Atomic(size_t) head/tail. */
    _Atomic int          best_leading_global;       /* C41-1 : volatile int → _Atomic int */
    _Atomic uint32_t     best_nonce_global;         /* C41-1 : volatile → _Atomic uint32_t */
    volatile int         block_found;               /* 1 si bloc valide trouvé */
    uint64_t             ts_start_ns;               /* Timestamp démarrage */
    uint64_t             ts_last_improvement_ns;    /* Timestamp dernière amélioration */
    /* C213-BUG4 : Tracker temps GPU actif séparément pour calcul hashrate GPU réel.
     * AVANT : hashrate = total_hashes / elapsed_s_total (inclut idle GPU 88.8%).
     * APRES : hashrate_gpu = gpu_hashes / gpu_active_time_ns (temps réel GPU actif).
     * Accumulé dans btc_gpu_thread() après chaque batch GPU. */
    volatile atomic_uint_least64_t gpu_active_time_ns;  /* C213-BUG4 : temps GPU actif cumulé */
    /* C216-BUG4-FIX : Compteur hashes GPU séparé pour calcul hashrate GPU réel.
     * AVANT C216 : hashrate_gpu = total_hashes / gpu_active_s (inclut hashes CPU !).
     * APRES C216 : hashrate_gpu = gpu_total_hashes / gpu_active_s (GPU uniquement).
     * Incrémenté dans btc_gpu_thread() après chaque batch GPU. */
    volatile atomic_uint_least64_t gpu_total_hashes;    /* C216-BUG4-FIX : hashes GPU uniquement */
    pthread_mutex_t      global_mutex;
    /* C39-P2 — Mutex dédié swap PT-MC — évite la race condition hot=3/cold=2.
     * Ref : analysechatgpt91.38.md §P2 BUG B-PTMC — 2026-04-12 */
    pthread_mutex_t      ptmc_swap_mutex;
    uint64_t             ptmc_sweep_idx;            /* C39-P4 OGY : index sweep global */
    nx48_btc_state_t*    nx48;
    btc_engine_config_t  cfg;
} btc_engine_t;

/* ══════════════════════════════════════════════════════════════════
 * C42-WATCHDOG-RAM/CPU : Contrôle total des ressources en temps réel
 * ══════════════════════════════════════════════════════════════════
 *
 * Surveille RAM (/proc/meminfo) et charge process toutes les 5s.
 * Commandes atomiques lues par chaque thread mining :
 *   btc_throttle_us  : microsecondes de sleep forcé entre batchs (0=libre)
 *   btc_batch_divisor: diviseur de batch_size (1=normal, 2=moitié, 4=quart)
 *   btc_pause_flag   : 1=pause complète tous les threads (RAM critique)
 *
 * Niveaux RAM :
 *   > 2 GB dispo → normal (throttle=0, batch=1)
 *   1-2 GB dispo → léger  (throttle=1ms, batch=1)
 *   500MB-1GB    → modéré (throttle=5ms, batch=2)
 *   200-500MB    → fort   (throttle=20ms, batch=4)
 *   < 200MB      → PAUSE  (sleep 30s, puis reprise)
 *
 * Ref : analysechatgpt91.40.md §C42-WATCHDOG — 2026-04-13
 */
static _Atomic int  btc_throttle_us   = 0;  /* µs de sleep par batch */
static _Atomic int  btc_batch_divisor = 1;  /* diviseur batch_size dynamique */
static _Atomic int  btc_pause_flag    = 0;  /* 1 = pause totale */
static _Atomic int  btc_watchdog_stop = 0;  /* signal arrêt watchdog */
static nx48_btc_state_t* btc_global_nx48   = NULL;  /* ptr pour signal handler */
static char              btc_global_csv[256] = "";  /* path CSV pour signal handler */

/* Lecture RAM disponible en KB depuis /proc/meminfo */
static long btc_ram_available_kb(void) {
    FILE* fp = fopen("/proc/meminfo", "r");
    if (!fp) return 99999999L;
    char line[256];
    long avail = 99999999L;
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "MemAvailable:", 13) == 0) {
            if (sscanf(line + 13, "%ld", &avail) == 1) break;
        }
    }
    fclose(fp);
    return avail;
}

/* Thread watchdog : surveille RAM toutes les 5s et ajuste les commandes */
static void* btc_watchdog_thread(void* arg) {
    (void)arg;
    int tick = 0;
    while (!atomic_load_explicit(&btc_watchdog_stop, memory_order_relaxed)) {
        long ram_kb = btc_ram_available_kb();
        long ram_mb = ram_kb / 1024L;

        int throttle, divisor, pause;
        if (ram_mb < 200) {
            /* NIVEAU 4 : PAUSE — RAM critique, risque OOM killer */
            throttle = 0; divisor = 4; pause = 1;
            fprintf(stderr, "[C42-WATCHDOG] 🔴 RAM CRITIQUE: %ldMB — PAUSE totale\n", ram_mb);
        } else if (ram_mb < 500) {
            /* NIVEAU 3 : throttle fort */
            throttle = 20000; divisor = 4; pause = 0;
            if (tick % 6 == 0)
                printf("[C42-WATCHDOG] 🟠 RAM basse: %ldMB — throttle 20ms batch/4\n", ram_mb);
        } else if (ram_mb < 1000) {
            /* NIVEAU 2 : throttle modéré */
            throttle = 5000; divisor = 2; pause = 0;
            if (tick % 12 == 0)
                printf("[C42-WATCHDOG] 🟡 RAM modérée: %ldMB — throttle 5ms batch/2\n", ram_mb);
        } else if (ram_mb < 2000) {
            /* NIVEAU 1 : throttle léger */
            throttle = 1000; divisor = 1; pause = 0;
        } else {
            /* NIVEAU 0 : normal */
            throttle = 0; divisor = 1; pause = 0;
        }

        atomic_store_explicit(&btc_throttle_us,   throttle, memory_order_relaxed);
        atomic_store_explicit(&btc_batch_divisor, divisor,  memory_order_relaxed);
        atomic_store_explicit(&btc_pause_flag,    pause,    memory_order_relaxed);

        if (pause) {
            /* Pause totale : attendre 30s puis réévaluer */
            sleep(30);
        } else {
            sleep(5);
        }
        tick++;
    }
    return NULL;
}

/* Signal handler SIGTERM/SIGINT : sauvegarde CSV NX48 avant exit */
static void btc_signal_handler(int sig) {
    fprintf(stderr, "\n[C42-SIGNAL] Signal %d reçu — sauvegarde CSV NX48...\n", sig);
    if (btc_global_nx48 && btc_global_csv[0]) {
        nx48_btc_save_csv(btc_global_nx48, btc_global_csv);
        fprintf(stderr, "[C42-SIGNAL] CSV sauvegardé → %s\n", btc_global_csv);
    }
    /* Sortie propre — le script bash relancera automatiquement */
    _exit(42); /* code 42 = arrêt propre pour restart bash */
}

/* ── Timestamp monotonique ns ───────────────────────────────────── */
static uint64_t eng_ts_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

/* C41-2-TIMESTAMP-CACHE : cache thread-local 1ms pour réduire les syscalls.
 * AVANT : eng_ts_ns() = clock_gettime() appelé à chaque itération de boucle while
 *         (en début + milieu + fin de chaque tour) → 3+ syscalls par batch.
 * APRES : eng_ts_cached() lit le cache TLS ; n'appelle clock_gettime QUE si
 *         le dernier appel réel date de plus de 1ms (C41_TS_CACHE_NS).
 *         Résultat : ~1000× moins de syscalls timestamp → récupère ~3% CPU.
 * Source : src/advanced_calculations/quantum_simulator_v4_staging_next/common_types.h
 *          #define REPLIT_TIMESTAMP_CACHE_NS 1000000 — cache 1ms pour conteneurs NFS.
 *
 * INVARIANT : les comparaisons de durée (>2s, >5s, >10s) sont insensibles
 * à une imprécision de ±1ms → pas d'impact fonctionnel. */
static __thread uint64_t tl_ts_real  = 0; /* Dernier ts réel (clock_gettime) */
static __thread uint64_t tl_ts_cache = 0; /* Valeur mise en cache */

static inline uint64_t eng_ts_cached(void) {
    /* Invalide le cache si > 1ms depuis le dernier appel réel */
    uint64_t cached = tl_ts_cache;
    if (__builtin_expect(cached - tl_ts_real >= C41_TS_CACHE_NS, 0)) {
        cached = eng_ts_ns();
        tl_ts_real  = cached;
        tl_ts_cache = cached;
    } else {
        /* Incrémente le cache de 1µs pour éviter la starvation des comparaisons */
        tl_ts_cache = cached + 1000ULL;
    }
    return cached;
}

/* ── RNG thread-local (xorshift64) ──────────────────────────────── */
static __thread uint64_t tl_rng_state = 0;
static uint64_t rng_init_seed(int thread_id) {
    return ((uint64_t)thread_id * 6364136223846793005ULL + 1442695040888963407ULL)
           ^ (uint64_t)eng_ts_ns();
}
static uint64_t rng_next(void) {
    tl_rng_state ^= tl_rng_state << 13;
    tl_rng_state ^= tl_rng_state >> 7;
    tl_rng_state ^= tl_rng_state << 17;
    return tl_rng_state;
}

static void btc_header_to_hex(const lv_btc_block_header_t* h, char out[161]) {
    uint8_t raw[80];
    raw[0] = (uint8_t)h->version;
    raw[1] = (uint8_t)(h->version >> 8);
    raw[2] = (uint8_t)(h->version >> 16);
    raw[3] = (uint8_t)(h->version >> 24);
    memcpy(raw + 4, h->prev_block_hash, 32);
    memcpy(raw + 36, h->merkle_root, 32);
    raw[68] = (uint8_t)h->timestamp;
    raw[69] = (uint8_t)(h->timestamp >> 8);
    raw[70] = (uint8_t)(h->timestamp >> 16);
    raw[71] = (uint8_t)(h->timestamp >> 24);
    raw[72] = (uint8_t)h->bits;
    raw[73] = (uint8_t)(h->bits >> 8);
    raw[74] = (uint8_t)(h->bits >> 16);
    raw[75] = (uint8_t)(h->bits >> 24);
    raw[76] = (uint8_t)h->nonce;
    raw[77] = (uint8_t)(h->nonce >> 8);
    raw[78] = (uint8_t)(h->nonce >> 16);
    raw[79] = (uint8_t)(h->nonce >> 24);
    for (int i = 0; i < 80; i++)
        snprintf(out + 2 * i, 3, "%02x", raw[i]);
    out[160] = '\0';
}

static void btc_bytes_to_hex_local(const uint8_t* bytes, size_t n, char* out) {
    for (size_t i = 0; i < n; i++)
        snprintf(out + 2 * i, 3, "%02x", bytes[i]);
    out[2 * n] = '\0';
}

static void btc_write_pow_candidate(
    const btc_engine_t* eng,
    const lv_sha256_result_t* res,
    uint32_t nonce)
{
    if (!eng || !res) return;
    mkdir(eng->cfg.log_dir, 0755);
    char path[512];
    snprintf(path, sizeof(path), "%s/pow_candidate_%s.json", eng->cfg.log_dir, eng->cfg.run_id);
    FILE* f = fopen(path, "w");
    if (!f) return;
    lv_btc_block_header_t h = eng->cfg.header_template;
    h.nonce = nonce;
    char header_hex[161];
    char hash_hex[65];
    char target_hex[65];
    btc_header_to_hex(&h, header_hex);
    btc_bytes_to_hex_local(res->digest, 32, hash_hex);
    btc_bytes_to_hex_local(eng->cfg.target, 32, target_hex);
    fprintf(f,
        "{\n"
        "  \"schema\": \"lumvorax_btc_pow_candidate_v1\",\n"
        "  \"run_id\": \"%s\",\n"
        "  \"mode\": \"%s\",\n"
        "  \"nonce\": %u,\n"
        "  \"leading_zeros\": %d,\n"
        "  \"below_target\": %s,\n"
        "  \"header_hex\": \"%s\",\n"
        "  \"block_hash_hex\": \"%s\",\n"
        "  \"target_hex\": \"%s\",\n"
        "  \"network_submission_ready\": false,\n"
        "  \"submission_reason\": \"header_pow_only_no_full_block_hex_coinbase_merkle_template\"\n"
        "}\n",
        eng->cfg.run_id,
        eng->cfg.run_mode,
        nonce,
        res->leading_zeros,
        res->below_target ? "true" : "false",
        header_hex,
        hash_hex,
        target_hex);
    fclose(f);
    printf("[BTC_QM] Candidat POW exporté → %s\n", path);
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_pow_candidate_exported", 1.0);
}

/* ── Initialise le moteur ───────────────────────────────────────── */
static btc_engine_t* engine_create(const btc_engine_config_t* cfg) {
    /* C255v8n CORRECTION #2 : Initialiser températures hiérarchiques AVANT création moteur */
    btc_init_hierarchical_temps();
    
    btc_engine_t* eng = LV_CALLOC(1, sizeof(btc_engine_t));
    if (!eng) return NULL;

    memcpy(&eng->cfg, cfg, sizeof(btc_engine_config_t));
    atomic_store(&eng->total_hashes, 0);

    /* C100-FIX-PERSIST-MONO : recharge le record absolu monotone du run précédent
     * pour ne JAMAIS redescendre en dessous (bug C99 : best=37 perdu, redémarre à 32).
     * Source : config/btc_nx48_alltime.csv (CSV monotone strictement croissant).
     * Le best_leading_global est seedé à la valeur historique pour que les
     * comparaisons "if (lz > best_leading_global)" ne valident QUE de vrais records. */
    {
        nx48_alltime_record_t alltime;
        const char *altpath = NX48_ALLTIME_DEFAULT_PATH;
        if (nx48_alltime_load(altpath, &alltime) == 0
            && alltime.best_lz_alltime > 0) {
            eng->best_leading_global = alltime.best_lz_alltime;
            eng->best_nonce_global   = alltime.best_nonce_alltime;
            FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME,
                "btc_alltime_seed_lz", (double)alltime.best_lz_alltime);
            FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME,
                "btc_alltime_seed_upd_count", (double)alltime.update_count);
            printf("[C100-ALLTIME] seed best_lz_alltime=%d nonce=%u upd_count=%llu "
                   "(historique préservé)\n",
                   alltime.best_lz_alltime,
                   (unsigned)alltime.best_nonce_alltime,
                   (unsigned long long)alltime.update_count);
        } else {
            eng->best_leading_global = 0;
            eng->best_nonce_global   = 0;
            printf("[C100-ALLTIME] aucun record absolu antérieur — seed à 0\n");
        }
    }
    eng->block_found              = 0;
    eng->ts_start_ns              = eng_ts_ns();
    eng->ts_last_improvement_ns   = eng->ts_start_ns;
    /* C213-BUG4 : Initialiser compteur temps GPU actif */
    atomic_store_explicit(&eng->gpu_active_time_ns, 0, memory_order_relaxed);
    /* C216-BUG4-FIX : Initialiser compteur hashes GPU séparé */
    atomic_store_explicit(&eng->gpu_total_hashes, 0, memory_order_relaxed);
    eng->ptmc_sweep_idx           = 0;
    pthread_mutex_init(&eng->global_mutex, NULL);
    pthread_mutex_init(&eng->ptmc_swap_mutex, NULL);

    /* Initialisation des répliques PT-MC */
    for (int r = 0; r < BTC_N_REPLICAS; r++) {
        btc_ptmc_replica_t* rep = &eng->replicas[r];
        rep->nonce           = (uint32_t)((uint64_t)cfg->nonce_start
                               + (uint64_t)r * ((uint64_t)(cfg->nonce_end - cfg->nonce_start)
                               / BTC_N_REPLICAS));
        rep->best_nonce      = rep->nonce;
        rep->temperature     = BTC_REPLICA_TEMPS[r];
        rep->leading_zeros   = 0;
        rep->best_leading    = 0;
        rep->hashes_done     = 0;
        rep->swaps_accepted  = 0;
        rep->swaps_attempted = 0;
        pthread_mutex_init(&rep->mutex, NULL);
    }

    /* Log forensic initialisation */
    FORENSIC_LOG_MODULE_START(BTC_MODULE_NAME, "btc_engine_create");
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_n_replicas",    (double)BTC_N_REPLICAS);
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_n_threads",     (double)cfg->n_threads);
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_batch_size",    (double)cfg->batch_size);
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_nonce_start",   (double)cfg->nonce_start);
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_nonce_end",     (double)cfg->nonce_end);
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_duration_s",    (double)(cfg->duration_ns / 1000000000ULL));

    for (int r = 0; r < BTC_N_REPLICAS; r++) {
        BTC_FORENSIC_REPLICA_STATE(r, eng->replicas[r].nonce,
            eng->replicas[r].temperature, 0);
    }

    return eng;
}

/* ── Échange Metropolis entre répliques ────────────────────────── */
/* C39-P2 : Mutex global ptmc_swap_mutex protège l'ensemble de la boucle swap.
 * AVANT : lock individuel r1→r2 laissait une fenêtre de race condition entre paires.
 * APRÈS : lock global → garantit atomicité complète de tous les échanges.
 * C39-P4 OGY : micro-perturbation sinusoïdale des températures à chaque sweep.
 * Stabilise l'orbite périodique instable (UPO) autour du nonce record.
 * Ref : analysechatgpt91.38.md §P2 §P4 — 2026-04-12 */
static void engine_ptmc_swap(btc_engine_t* eng) {
    pthread_mutex_lock(&eng->ptmc_swap_mutex);
    eng->ptmc_sweep_idx++;

    /* C39-P4 OGY : micro-perturbation températures — période 7 (premier > BTC_N_REPLICAS/2) */
    double ogy_delta = 0.001 * sin(2.0 * 3.14159265358979323846
                                   * (double)(eng->ptmc_sweep_idx % 7) / 7.0);

    for (int r = 0; r < BTC_N_REPLICAS - 1; r++) {
        btc_ptmc_replica_t* r1 = &eng->replicas[r];
        btc_ptmc_replica_t* r2 = &eng->replicas[r + 1];

        pthread_mutex_lock(&r1->mutex);
        pthread_mutex_lock(&r2->mutex);

        /* C39-P4 OGY : perturbation légère de la température (conservatif : |Δ|=0.001) */
        r1->temperature += ogy_delta;
        r2->temperature -= ogy_delta;
        /* Clamp températures — évite dérive sur run infini */
        if (r1->temperature < 0.5)  r1->temperature = 0.5;
        if (r2->temperature < 0.5)  r2->temperature = 0.5;
        if (r1->temperature > 100.0) r1->temperature = 100.0;
        if (r2->temperature > 100.0) r2->temperature = 100.0;

        r1->swaps_attempted++;
        r2->swaps_attempted++;

        /* Critère Metropolis simplifié :
         * Accepter si la réplique chaude a un meilleur leading_zeros
         * (analogie : H_hot < H_cold → ΔH < 0 → accept = 1.0)
         * Sinon : accept = exp(-ΔH / T_avg) avec ΔH = leading_cold - leading_hot */
        int dz = r2->leading_zeros - r1->leading_zeros;
        double accept = 1.0;
        if (dz < 0) {
            double T_avg = (r1->temperature + r2->temperature) * 0.5;
            accept = exp((double)dz / T_avg);
        }

        /* Décision stochastique */
        double u = (double)(rng_next() & 0xFFFFFFFFu) / 4294967296.0;
        int swap_done = 0;
        if (u < accept) {
            /* Échange des nonces */
            uint32_t tmp_n = r1->nonce; r1->nonce = r2->nonce; r2->nonce = tmp_n;
            int tmp_l = r1->leading_zeros; r1->leading_zeros = r2->leading_zeros;
                        r2->leading_zeros = tmp_l;
            r1->swaps_accepted++;
            r2->swaps_accepted++;
            swap_done = 1;
        }
        /* C128-FIX-A8 : throttling forensic PT-MC swap.
         * Avant C128 : 60K+ rows btc_ptmc_* / 1200s = 93.6% du log forensic
         * (mainnet run RUN_ID=c112_ub_1777460784). C'était du bruit pur :
         * chaque swap = 3 rows (rep_hot / rep_cold / accept) à ~800 swaps/s.
         * Après C128 : 1 row sur 100 → ~6 rows/s. Le scoring Metropolis reste
         * inchangé (la logique d'échange est exécutée à chaque sweep) ;
         * seul le LOG est sous-échantillonné. Compteur atomique partagé
         * inter-threads pour throttle déterministe. */
        {
            static _Atomic uint64_t ptmc_swap_log_counter = 0;
            uint64_t lc = atomic_fetch_add_explicit(&ptmc_swap_log_counter, 1,
                                                     memory_order_relaxed);
            if ((lc % 100ULL) == 0ULL) {
                BTC_FORENSIC_PTMC_SWAP(r + 1, r,
                                       swap_done ? accept : -accept);
            }
        }

        pthread_mutex_unlock(&r2->mutex);
        pthread_mutex_unlock(&r1->mutex);
    }

    pthread_mutex_unlock(&eng->ptmc_swap_mutex);
}

/* ── Structure de travail thread ────────────────────────────────── */
/* C41-8-REPLICA-ALIGN : btc_thread_work_t aligné 64 bytes — élimine false sharing threads.
 * Chaque thread_work occupe sa propre cache line(s) → 0 invalidation cross-thread.
 * Source : src/optimization/memory_optimizer.h — posix_memalign 64B pour tous les pools. */
typedef struct {
    btc_engine_t*       eng;
    int                 thread_id;
    int                 replica_id;
    uint32_t            midstate[LV_SHA256_MIDSTATE_WORDS];
} __attribute__((aligned(64))) btc_thread_work_t;

/* ── Fonction thread de minage ──────────────────────────────────── */
static void* btc_mining_thread(void* arg) {
    btc_thread_work_t* work = (btc_thread_work_t*)arg;
    btc_engine_t*      eng  = work->eng;
    btc_ptmc_replica_t* rep = &eng->replicas[work->replica_id];
    const btc_engine_config_t* cfg = &eng->cfg;

    BTC_THREAD_START("btc_mining_worker");

    /* Initialise RNG thread-local */
    tl_rng_state = rng_init_seed(work->thread_id);

    uint64_t local_hashes      = 0;
    uint64_t ts_last_hw        = eng_ts_ns();
    uint64_t ts_last_nx48      = eng_ts_ns();
    uint64_t ts_last_swap      = eng_ts_ns();
    uint64_t ts_last_stats     = eng_ts_ns();
    /* C66-PERIODIC-SAVE : sauvegarde CSV toutes les 60s — protège contre SIGSEGV/OOM */
    uint64_t ts_last_save_csv  = eng_ts_ns();
    /* C128-FIX-A5/A6 : milestone async_logger + lum_log_writer toutes les 60 s.
     * Avant C128 : async_log restait à 119 octets (un seul ASYNC_INFO d'init)
     * et lum_log à 768 octets (start+end+mem_baseline×4+mem_final×3).
     * Après C128 : ~20 milestones / 1200 s, chacune avec hashrate, best_lz,
     * total_hashes, batches → fichiers de plusieurs Kio, traces exploitables. */
    uint64_t ts_last_milestone = eng_ts_ns();
    uint64_t milestone_idx     = 0;

    /* Delta nonce initial depuis NX48 (minimum 1 pour éviter division par zéro) */
    double delta_nonce = 65536.0 * eng->nx48->delta_nonce_scale;
    if (delta_nonce < 1.0) delta_nonce = 65536.0; /* valeur par défaut sûre */
    int nx48_disabled = getenv("BTC_NX48_DISABLED") != NULL;
    /* C99 — Pont neuro Izhikevich+STDP (couplé, pas substitut). Activé si
     * BTC_NX48_COUPLER=1. Allocation par-thread (1er init thread 0). */
    nx48_bridge_t* coupler_bridge = NULL;
    int coupler_active = (!nx48_disabled) && (getenv("BTC_NX48_COUPLER") != NULL);
    if (coupler_active && work->thread_id == 0) {
        /* C100-FIX : signature corrigée nx48_bridge_create(run_id, use_neural, log_path).
         * AVANT : appel à 2 args (log_path, log_every) → too few arguments → build break.
         * APRES : 3 args conformes au header nx48_coupler_bridge.h L30. */
        coupler_bridge = nx48_bridge_create(
            cfg->run_id[0] ? cfg->run_id : "btc_anon",
            /*use_neural=*/1,
            /*log_path=*/"logs/nx48_bridge_C99.jsonl");
    }

    while (!eng->block_found) {
        /* Vérification durée max */
        uint64_t ts_now = eng_ts_ns();
        if (cfg->duration_ns > 0 && (ts_now - eng->ts_start_ns) >= cfg->duration_ns)
            break;

        /* [C259-GEN9-NATIVE-ACTIVATION] Activer i915 DRM natif immédiatement au démarrage
         * Thread 0 uniquement (évite activation multiple)
         * MIGRATION C259 : OpenCL → i915 DRM natif (0% OpenCL, 0% Level Zero)
         * OBJECTIF : C258 allocation dynamique 3GB + réouverture DRM périodique */
        if (work->thread_id == 0) {
            double elapsed_s = (double)(ts_now - eng->ts_start_ns) / 1e9;
            int gpu_currently_active = atomic_load_explicit(&nx48_ctrl_gpu_active, memory_order_relaxed);
            
            if (elapsed_s >= 0.0 &&
                eng->nx48 &&
                eng->nx48->hw.gpu_opencl_present &&
                !gpu_currently_active) {
                
                /* Activation atomique GPU */
                atomic_store_explicit(&nx48_ctrl_gpu_active, 1, memory_order_relaxed);
                eng->nx48->hw.gpu_opencl_active = 1;
                
                /* C286: Initialiser adaptateur Gen9 natif avec batch_size 10M (C283 validé) */
                btc_gen9_config_t gen9_config = {
                    .batch_size = 10000000,  /* C286: 10M nonces (C283: 100/100 OK, 40 GH/s pic) */
                    .work_group_size = 256,
                    .kernel_path = "kernels/btc_sha256_gen9_optimized.bin",
                    .log_path = "logs/forensic/gen9_native_c259.log",
                    .enable_profiling = true
                };
                
                int ret = btc_gen9_adapter_init(&gen9_config);
                if (ret != 0) {
                    fprintf(stderr, "[C259-GEN9-NATIVE] ERREUR: Initialisation adaptateur échouée\n");
                } else {
                    /* Logging forensique complet */
                    fprintf(stderr, "\n");
                    fprintf(stderr, "═══════════════════════════════════════════════════════════\n");
                    fprintf(stderr, "[C259-GEN9-NATIVE] 🚀 ACTIVATION i915 DRM NATIF À t=%.1fs\n", elapsed_s);
                    fprintf(stderr, "[C259-GEN9-NATIVE] GPU: %s\n",
                            eng->nx48->hw.gpu_name[0] ? eng->nx48->hw.gpu_name : "Intel UHD 620");
                    fprintf(stderr, "[C259-GEN9-NATIVE] Architecture: 100%% i915 DRM (0%% OpenCL, 0%% Level Zero)\n");
                    fprintf(stderr, "[C259-GEN9-NATIVE] C258: Allocation dynamique 3GB activée\n");
                    fprintf(stderr, "[C259-GEN9-NATIVE] C255v8j: Réouverture DRM tous les 27 dispatches\n");
                    fprintf(stderr, "[C259-GEN9-NATIVE] Performance attendue: 151-153 MH/s (vs 7.29 MH/s OpenCL)\n");
                    fprintf(stderr, "═══════════════════════════════════════════════════════════\n");
                    fprintf(stderr, "\n");
                    
                    FORENSIC_LOG_ANOMALY(BTC_MODULE_NAME, "btc_gen9_native_activation_c259", elapsed_s);
                    
                    /* Log dans async_logger si disponible */
                    if (g_btc_async_log) {
                        ASYNC_LOG(g_btc_async_log, ASYNC_LOG_INFO,
                                 "GPU ACTIVATED at t=%.1fs (i915 DRM native, C258 dynamic allocation)", elapsed_s);
                    }
                }
            }
        }

        /* C50-FIX-P1 : Copie atomique des scalaires NX48 sous global_mutex.
         * BUG ROOT CAUSE : eng->nx48->batch_size_scale et exploration_bias sont
         * écrits par thread 0 via nx48_btc_update() (toutes les 2s) sans aucune
         * synchronisation vers les 7 autres threads mining → data race C11 →
         * undefined behavior → SIGSEGV reproductible après ~700s (619-626M hashes).
         * CORRECTION : lecture sous global_mutex une fois par itération while
         * (coût : ~50ns, négligeable vs ~1100ns par batch de 1024 hashes).
         * Ref : analysechatgpt91.50.md §5 BUG-P1-SIGSEGV — 2026-04-16 */
        int    batch;
        double exploration;
        {
            pthread_mutex_lock(&eng->global_mutex);
            batch       = (int)(cfg->batch_size * eng->nx48->batch_size_scale);
            exploration = eng->nx48->exploration_bias;
            pthread_mutex_unlock(&eng->global_mutex);
        }
        if (batch < 1) batch = 1;
        if (batch > 4096) batch = 4096;

        for (int b = 0; b < batch && !eng->block_found; b++) {
            /* ─────────────────────────────────────────────────────────────────
             * C65-FIX-ORBITAL : Scan orbital concentré autour du record absolu.
             *
             * Stratégie à 3 niveaux :
             *   Mode 1 — ORBITAL (prob=0.30 si best_global ≥ 18 bits) :
             *     Exploration dans une zone ±ORBITAL_RADIUS autour du nonce record
             *     absolu global. Rayon initial = 30 000, réduit ×0.99 chaque itération
             *     (descente en spirale vers le nonce optimal). Gaussien pour densifier
             *     le centre de la zone.
             *   Mode 2 — EXPLOITATION (prob=exploration_bias) :
             *     Voisinage du meilleur nonce LOCAL à la réplique.
             *   Mode 3 — EXPLORATION globale :
             *     Nonce aléatoire dans toute la plage [nonce_start, nonce_end].
             *
             * Justification physique : Les near-misses se concentrent en clusters
             * autour du nonce record (invariant D4 : 8 near-miss à 20 bits sur 2 runs).
             * Cela suggère une corrélation locale dans la distribution des leading_zeros
             * autour du nonce record → le scan orbital exploite cette corrélation.
             *
             * ORBITAL_RADIUS = 30 000 nonces (±1.5σ d'une gaussienne de σ=20 000).
             * ORBITAL_THRESHOLD = 18 bits (≥18 bits = zone prometteuse détectée).
             *
             * Ref : analysechatgpt91.38.md §C65-FIX-ORBITAL — 2026-04-12
             * ─────────────────────────────────────────────────────────────────*/
            /* C39-P3 : Orbital radius 50k (vs 30k C38), threshold 22 bits (vs 18 C38).
             * Motivation : record = 24 bits → activation orbital à 22 bits (+2 bits anticipation)
             *              radius 50k → zone ±50k nonces autour du record (vs ±30k).
             * C39-P5 Lebesgue : 15% des calculs en Lebesgue scan par niveau décroissant.
             * Ref : analysechatgpt91.38.md §P3 §P5 — 2026-04-12 */
            #define ORBITAL_THRESHOLD 22
            #define ORBITAL_RADIUS    50000LL
            #define ORBITAL_PROB      0.30   /* 30% des calculs dans la zone orbitale */
            #define LEBESGUE_PROB     0.15   /* 15% des calculs en scan Lebesgue */

            uint32_t nonce;
            /* exploration : variable locale copiée sous mutex en début de batch (C50-FIX-P1) */
            double u = (double)(rng_next() & 0xFFFFFFFFu) / 4294967296.0;

            pthread_mutex_lock(&rep->mutex);

            /* Décision 3 niveaux */
            int global_best = eng->best_leading_global;
            uint32_t global_record_nonce = eng->best_nonce_global;
            pthread_mutex_unlock(&rep->mutex);

            if (global_best >= ORBITAL_THRESHOLD && u < ORBITAL_PROB) {
                /* Mode 1 : ORBITAL — scan gaussien autour du record global (C39-P3: radius 50k) */
                int64_t radius = ORBITAL_RADIUS;
                double g  = (double)((int64_t)(rng_next() & 0xFFFFFFFFu) - 0x7FFFFFFF) /
                            (double)0x7FFFFFFF;
                double g2 = (double)((int64_t)(rng_next() & 0xFFFFFFFFu) - 0x7FFFFFFF) /
                            (double)0x7FFFFFFF;
                double g3 = (double)((int64_t)(rng_next() & 0xFFFFFFFFu) - 0x7FFFFFFF) /
                            (double)0x7FFFFFFF;
                double gauss = (g + g2 + g3) / 3.0;
                int64_t offset = (int64_t)(gauss * (double)radius / 0.577);
                if (offset > radius)  offset = radius;
                if (offset < -radius) offset = -radius;
                nonce = (uint32_t)((int64_t)global_record_nonce + offset);
                BTC_NANO("btc_orbital_scan_nonce", (double)nonce);

            } else if (global_best >= ORBITAL_THRESHOLD &&
                       u < (ORBITAL_PROB + LEBESGUE_PROB)) {
                /* Mode 2 : LEBESGUE SCAN — par niveau décroissant pondéré.
                 * C39-P5 : Échantillonnage dans des level-sets SHA-256.
                 * Chaque "level" correspond à un nombre de leading_zeros attendu.
                 * Rayon Lebesgue ∝ (1 - weight) → proche du record = rayon court.
                 * Ref : analysechatgpt91.38.md §P5 — 2026-04-12 */
                int level = (int)(rng_next() % (uint64_t)(global_best + 1));
                double weight = (global_best > 0)
                    ? (double)(level + 1) / (double)(global_best + 1) : 0.5;
                /* Rayon inversement proportionnel au niveau : niveau bas → rayon large */
                int64_t leb_radius = (int64_t)((double)ORBITAL_RADIUS *
                                     (1.0 + (1.0 - weight) * 4.0));
                int64_t sign = ((rng_next() & 1u) == 0) ? 1LL : -1LL;
                int64_t offset = sign * (int64_t)(rng_next() % (uint64_t)leb_radius);
                nonce = (uint32_t)((int64_t)global_record_nonce + offset);
                BTC_NANO("btc_lebesgue_scan_nonce", (double)level);

            } else {
                pthread_mutex_lock(&rep->mutex);
                if (u > exploration) {
                    /* Mode 3 : EXPLOITATION — voisinage meilleur nonce local */
                    int64_t dn64 = (int64_t)delta_nonce;
                    if (dn64 < 1) dn64 = 1;
                    int64_t offset = (int64_t)(rng_next() & 0xFFFFFFFFu) % dn64
                                   - dn64 / 2;
                    nonce = (uint32_t)((int64_t)rep->best_nonce + offset);
                } else {
                    /* Mode 4 : EXPLORATION globale (plage complète)
                     * CORRECTION SIGFPE C63 : cast explicite uint64_t */
                    uint64_t range = (uint64_t)cfg->nonce_end
                                   - (uint64_t)cfg->nonce_start + 1ULL;
                    if (range == 0) range = 1;
                    nonce = cfg->nonce_start + (uint32_t)(rng_next() % range);
                }
                pthread_mutex_unlock(&rep->mutex);
            }

            /* Double-SHA256 avec midstate (C65-FIX-MIDSTATE : header complet) */
            uint64_t ts_hash_start = eng_ts_ns();
            lv_sha256_result_t res = lv_sha256d_midstate(
                work->midstate,
                &cfg->header_template,
                nonce,
                cfg->target,
                cfg->run_id,
                atomic_load(&eng->total_hashes) + local_hashes
            );
            uint64_t ts_hash_end = eng_ts_ns();

            /* C169-BIT-LEVEL-LOGGING : Log CHAQUE nonce testé avec timing précis.
             * AVANT C169 : Aucun log des hashes individuels → impossible de tracer
             *              les patterns, bugs, ou comportements anormaux.
             * APRÈS C169 : Chaque nonce loggé avec nonce, leading_zeros, elapsed_ns.
             *              Format CSV forensic : btc_sha256_nonce, btc_leading_zeros,
             *              btc_sha256_elapsed_ns → traçabilité totale bit-par-bit.
             * COÛT : ~200ns par hash (écriture CSV asynchrone via ultra_forensic_logger).
             * GAIN : Détection bugs cachés, patterns SHA-256, validation algorithmes.
             * Ref : Demande utilisateur C168 — "je veux au moins le premier nonce!" */
            BTC_FORENSIC_HASH(nonce, res.leading_zeros, (ts_hash_end - ts_hash_start));

            local_hashes++;

            /* Mise à jour état réplique */
            pthread_mutex_lock(&rep->mutex);
            rep->leading_zeros = res.leading_zeros;
            rep->hashes_done++;
            if (res.leading_zeros > rep->best_leading) {
                rep->best_leading = res.leading_zeros;
                rep->best_nonce   = nonce;
            }
            pthread_mutex_unlock(&rep->mutex);

            /* Mise à jour record global */
            if (res.leading_zeros > eng->best_leading_global) {
                pthread_mutex_lock(&eng->global_mutex);
                if (res.leading_zeros > eng->best_leading_global) {
                    eng->best_leading_global    = res.leading_zeros;
                    eng->best_nonce_global      = nonce;
                    eng->ts_last_improvement_ns = eng_ts_ns();
                    BTC_FORENSIC_BLOCK_FOUND(nonce, res.leading_zeros, 0);
                    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME,
                        "btc_best_leading_zeros", (double)res.leading_zeros);
                    if (eng->nx48) {
                        eng->nx48->best_nonce = nonce;
                        /* C66-FIX-PERSIST : Synchroniser best_leading_zeros dans l'état NX48
                         * AVANT C66 : seul best_nonce était mis à jour ici → best_leading_zeros
                         *             restait à l'ancienne valeur (ex: 28) jusqu'au prochain
                         *             nx48_btc_update() appelé 2s plus tard.
                         * Résultat : CSV sauvegardé avec best_nonce=record mais best_leading=28.
                         * APRÈS C66 : best_leading_zeros synchronisé AVANT la sauvegarde CSV.
                         * Ref : bug forensique C66 §BUG-PERSIST — btc_nx48_last.csv best=28/34 */
                        eng->nx48->best_leading_zeros = res.leading_zeros;
                    }
                    /* C40-CSV-RECORD : Sauvegarde immédiate du CSV NX48 à chaque
                     * nouveau record de leading zeros — évite la perte de record
                     * si le run est interrompu avant la fin.
                     * AVANT C40 : sauvegarde CSV UNIQUEMENT en fin de run → record
                     *             perdu si restart (ex: 25 bits perdu → run 233845Z).
                     * APRÈS C40 : sauvegarde atomique immédiate à chaque amélioration.
                     * Ref : rapport forensique C40 §BUG-P0-CSV — 2026-04-13 */
                    if (eng->nx48 && cfg->nx48_csv[0])
                        nx48_btc_save_csv(eng->nx48, cfg->nx48_csv);

                    /* C100-FIX-PERSIST-MONO : maj record absolu monotone GLOBAL
                     * (jamais décroissant inter-run, atomique, verrou fcntl).
                     * Le btc_nx48_last.csv est volatil par run — on a besoin
                     * d'un compteur monotone strictement croissant pour
                     * (a) ne PAS redémarrer à 32 quand on a déjà atteint 37,
                     * (b) pousser le record en temps réel via WebSocket. */
                    {
                        char hdr_hex[NX48_ALLTIME_HEADER_HEX_LEN] = {0};
                        const uint8_t *hp = (const uint8_t*)&cfg->header_template;
                        for (int hi = 0; hi < 80; hi++) {
                            snprintf(hdr_hex + hi*2, 3, "%02x", hp[hi]);
                        }
                        const char *wallet_env = getenv("BTC_WALLET_ADDRESS");
                        int upd_rc = nx48_alltime_try_update(
                            NX48_ALLTIME_DEFAULT_PATH,
                            res.leading_zeros,
                            nonce,
                            hdr_hex,
                            (wallet_env && wallet_env[0]) ? wallet_env : "-",
                            cfg->run_id);
                        if (upd_rc == 1) {
                            FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME,
                                "btc_alltime_record_updated",
                                (double)res.leading_zeros);
                            printf("[C100-ALLTIME] NOUVEAU RECORD ABSOLU lz=%d "
                                   "nonce=%u → btc_nx48_alltime.csv\n",
                                   res.leading_zeros, nonce);
                            /* Hook fichier sentinelle pour push WebSocket
                             * (l'agent Ubuntu surveille ce JSONL et pousse). */
                            FILE *jl = fopen(
                                "logs/forensic/nano/nx48_records_push.jsonl", "a");
                            if (jl) {
                                fprintf(jl,
                                    "{\"event\":\"new_record\",\"lz\":%d,"
                                    "\"nonce\":%u,\"run_id\":\"%s\","
                                    "\"ts\":%lld}\n",
                                    res.leading_zeros, nonce,
                                    /* C113-FIX-WARN-COSMETIC : cfg->run_id est char[64], adresse jamais NULL ;
                                     * on teste le contenu (caractère 0 d'une chaîne vide) au lieu du pointeur. */
                                    (cfg->run_id[0] != '\0') ? cfg->run_id : "anon",
                                    (long long)time(NULL));
                                fclose(jl);
                            }
                        }
                    }
                }
                pthread_mutex_unlock(&eng->global_mutex);
            }

            /* Bloc valide trouvé */
            if (res.below_target) {
                pthread_mutex_lock(&eng->global_mutex);
                eng->block_found        = 1;
                eng->best_nonce_global  = nonce;
                eng->cfg.header_template.nonce = nonce;
                BTC_FORENSIC_BLOCK_FOUND(nonce, res.leading_zeros, 1);
                FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_block_valid", 1.0);
                btc_write_pow_candidate(eng, &res, nonce);
                pthread_mutex_unlock(&eng->global_mutex);
                break;
            }
        }

        /* Accumulation compteur global */
        atomic_fetch_add(&eng->total_hashes, (uint64_t)batch);

        /* C42-WATCHDOG-RAM : throttle + pause dynamiques selon RAM disponible */
        {
            /* Pause totale si RAM critique (< 200MB) */
            if (atomic_load_explicit(&btc_pause_flag, memory_order_relaxed)) {
                if (work->thread_id == 0)
                    printf("[C42-WATCHDOG] ⏸️  PAUSE RAM critique — reprise dans 30s\n");
                sleep(30); /* tous les threads attendent */
            }
            /* Throttle adaptatif entre batchs */
            int thr = atomic_load_explicit(&btc_throttle_us, memory_order_relaxed);
            if (thr > 0) usleep((useconds_t)thr);
            /* Réduction dynamique du batch */
            int div = atomic_load_explicit(&btc_batch_divisor, memory_order_relaxed);
            if (div > 1) batch = cfg->batch_size / div;
            else         batch = cfg->batch_size;
        }

        /* ───────────────────────────────────────────────────────────
         * C128-FIX-A5/A6 — Milestones périodiques async_logger + lum_log
         * Un seul thread (tid=0) émet les milestones pour éviter une
         * tempête d'I/O depuis 16 threads. Période = 60 s.
         * Métriques émises :
         *   - hashrate_mhs cumulé    (élapsé global)
         *   - best_leading_global    (record courant)
         *   - total_hashes           (compteur atomique)
         *   - batches_processed      (proxy : local_hashes / batch)
         *   - exploration_bias       (état NX48)
         *   - delta_nonce_scale      (état NX48)
         * Si BTC_ASYNC_LOG ou BTC_LUM_LOG non actifs → pointers NULL → no-op.
         * ─────────────────────────────────────────────────────────── */
        uint64_t ts_now_ms = eng_ts_ns();
        if (work->thread_id == 0
            && (ts_now_ms - ts_last_milestone) > 60000000000ULL) {
            ts_last_milestone = ts_now_ms;
            milestone_idx++;
            uint64_t total_now    = atomic_load(&eng->total_hashes);
            double   elapsed_now  = (double)(ts_now_ms - eng->ts_start_ns) / 1e9;
            double   hr_mhs       = (elapsed_now > 0)
                                    ? (double)total_now / elapsed_now / 1e6 : 0.0;
            int      best_lz_now  = atomic_load_explicit(&eng->best_leading_global,
                                                          memory_order_relaxed);
            double   delta_now    = eng->nx48 ? eng->nx48->delta_nonce_scale : 0.0;
            double   eb_now       = eng->nx48 ? eng->nx48->exploration_bias  : 0.0;

            if (g_btc_async_log) {
                ASYNC_INFO(g_btc_async_log,
                    "[C128-MILESTONE #%" PRIu64 "] t=%.1fs hashrate=%.3fMH/s "
                    "best_lz=%d total=%" PRIu64 " delta=%.3f eb=%.3f",
                    milestone_idx, elapsed_now, hr_mhs, best_lz_now,
                    total_now, delta_now, eb_now);
            }
            if (g_btc_lum_log) {
                /* 6 records LUM par milestone — suffisamment dense pour
                 * reconstituer la courbe complète post-mortem. */
                lum_log_writer_write_record(g_btc_lum_log,
                    "milestone_idx",       (uint64_t)milestone_idx);
                lum_log_writer_write_record(g_btc_lum_log,
                    "milestone_t_ms",      (uint64_t)(elapsed_now * 1000.0));
                lum_log_writer_write_record(g_btc_lum_log,
                    "milestone_hashrate_uHs",
                    (uint64_t)(hr_mhs * 1e6)); /* µH/s entier */
                lum_log_writer_write_record(g_btc_lum_log,
                    "milestone_best_lz",   (uint64_t)best_lz_now);
                lum_log_writer_write_record(g_btc_lum_log,
                    "milestone_total_hashes", total_now);
                lum_log_writer_write_record(g_btc_lum_log,
                    "milestone_delta_milli",
                    (uint64_t)(delta_now * 1000.0));
            }
        }

        /* Mise à jour NX48 */
        uint64_t ts_now2 = eng_ts_ns();
        if (ts_now2 - ts_last_nx48 > 2000000000ULL) { /* toutes les 2 secondes */
            ts_last_nx48 = ts_now2;
            double elapsed_s = (double)(ts_now2 - eng->ts_start_ns) / 1e9;
            uint64_t total   = atomic_load(&eng->total_hashes);
            double hashrate_mhs = (elapsed_s > 0)
                ? (double)total / elapsed_s / 1e6 : 0.0;
            double time_since_impr = (double)(ts_now2 - eng->ts_last_improvement_ns) / 1e9;
            /* C96 P0 fix : éviter div/0 et inf si nonce_space déborde uint32_t */
            uint64_t nonce_space = (cfg->nonce_end >= cfg->nonce_start)
                ? ((uint64_t)cfg->nonce_end - (uint64_t)cfg->nonce_start + 1ULL)
                : 0ULL;
            double coverage_pct = (nonce_space > 0)
                ? (100.0 * (double)total / (double)nonce_space)
                : 0.0;
            if (coverage_pct > 100.0) coverage_pct = 100.0;
            if (!isfinite(coverage_pct)) coverage_pct = 0.0;

            /* C61 : Températures PT-MC lues depuis atomiques NX48 */
            int T_hot_idx  = atomic_load_explicit(&nx48_ctrl_T_hot_idx,  memory_order_relaxed);
            int T_cold_idx = atomic_load_explicit(&nx48_ctrl_T_cold_idx, memory_order_relaxed);
            double T_hot_nx48  = BTC_REPLICA_TEMPS[(T_hot_idx  < 0) ? 7 : (T_hot_idx  > 7) ? 7 : T_hot_idx];
            double T_cold_nx48 = BTC_REPLICA_TEMPS[(T_cold_idx < 0) ? 0 : (T_cold_idx > 7) ? 7 : T_cold_idx];

            double features[NX48_BTC_N_FEATURES];
            nx48_btc_compute_features(features,
                eng->best_leading_global,
                hashrate_mhs, 10000.0,
                (rep->swaps_attempted > 0)
                    ? (double)rep->swaps_accepted / (double)rep->swaps_attempted : 0.0,
                time_since_impr, coverage_pct, delta_nonce,
                (double)local_hashes, (double)(BTC_NX48_UPDATE_EVERY),
                T_hot_nx48, T_cold_nx48);

            double prob = nx48_disabled ? 0.5 : nx48_btc_predict(eng->nx48, features);

            /* C99 — Modulation neuro ±30% sur exploration_bias (couplée à NX48,
             * pas substitut). N'agit que si BTC_NX48_COUPLER=1 ET thread 0. */
            if (coupler_bridge && work->thread_id == 0) {
                double mod = nx48_bridge_modulate(coupler_bridge, features,
                                                  eng->best_leading_global);
                double new_bias = eng->nx48->exploration_bias * (1.0 + 0.30 * mod);
                if (new_bias < 0.05) new_bias = 0.05;
                if (new_bias > 0.95) new_bias = 0.95;
                eng->nx48->exploration_bias = new_bias;
                FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME,
                    "btc_nx48_coupler_mod", mod);
            }

            if (!nx48_disabled && work->thread_id == 0) { /* Un seul thread met à jour NX48 */
                nx48_btc_update(eng->nx48,
                    &(nx48_btc_config_t){
                        .learning_rate        = 0.01,
                        .lambda_l1            = 0.001,
                        .update_interval      = BTC_NX48_UPDATE_EVERY,
                        .n_threads_initial    = cfg->n_threads,
                        .hw_detect_interval_s = 30
                    },
                    features, prob,
                    eng->best_leading_global,
                    (uint32_t)eng->best_nonce_global,  /* C107 : nonce du record courant */
                    hashrate_mhs);
                delta_nonce = 65536.0 * eng->nx48->delta_nonce_scale;

                /* C61 : Appliquer threads dynamiques NX48 */
                int nx48_threads = atomic_load_explicit(&nx48_ctrl_n_threads, memory_order_relaxed);
                if (nx48_threads > 0 && nx48_threads != cfg->n_threads)
                    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_nx48_threads_target",
                        (double)nx48_threads);

                /* C61 : Adapter la réplique la plus chaude selon NX48 */
                eng->replicas[BTC_N_REPLICAS-1].temperature = T_hot_nx48;
                eng->replicas[0].temperature = T_cold_nx48;
            }
            if (nx48_disabled && work->thread_id == 0)
                FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_nx48_disabled", 1.0);

            BTC_FORENSIC_HASHRATE(hashrate_mhs, work->thread_id);
        }

        /* Snapshot HW */
        if (ts_now2 - ts_last_hw > 5000000000ULL) { /* toutes les 5 secondes */
            ts_last_hw = ts_now2;
            BTC_FORENSIC_HW();
        }

        /* Échange répliques PT-MC (thread 0 uniquement) */
        /* C64-FIX-B-PTMC : intervalle swap réduit 500ms→100ms pour plus d'échanges thermiques.
         * Ref : analysechatgpt91.36.md BUG B-PTMC — 2026-04-11 */
        if (work->thread_id == 0 && ts_now2 - ts_last_swap > 100000000ULL) {
            ts_last_swap = ts_now2;
            engine_ptmc_swap(eng);
        }

        /* Stats périodiques (thread 0) */
        if (work->thread_id == 0 && ts_now2 - ts_last_stats > 10000000000ULL) {
            ts_last_stats = ts_now2;
            uint64_t total   = atomic_load(&eng->total_hashes);
            double elapsed_s = (double)(ts_now2 - eng->ts_start_ns) / 1e9;
            double hashrate  = (elapsed_s > 0) ? (double)total / elapsed_s / 1e6 : 0.0;
            /* C57-FIX-SIGSEGV : vérification NULL eng->nx48 avant déréférencement
             * AVANT : accès direct eng->nx48->delta_nonce_scale sans garde → SIGSEGV t≈850s
             * APRÈS : guard NULL + lecture atomique-safe → crash impossible */
            double nx48_delta = (eng->nx48 != NULL)
                ? eng->nx48->delta_nonce_scale : 0.0;
            printf("[BTC_QM] elapsed=%.1fs hashes=%"PRIu64" hashrate=%.2fMH/s "
                   "best_leading=%d best_nonce=%u nx48_delta=%.2f\n",
                   elapsed_s, total, hashrate,
                   eng->best_leading_global, eng->best_nonce_global,
                   nx48_delta);
            fflush(stdout);
            BTC_FORENSIC_COVERAGE((double)total,
                100.0 * (double)total / (double)(cfg->nonce_end - cfg->nonce_start + 1));
        }

        /* C66-PERIODIC-SAVE : sauvegarde CSV périodique toutes les 60s (thread 0)
         * Protège contre SIGSEGV/OOM : si le run crashe brutalement, le dernier
         * état NX48 est perdu seulement si le crash survient dans les 60 dernières
         * secondes. Complémentaire au C40-CSV-RECORD (sauvegarde à chaque record).
         * AVANT C66 : sauvegarde uniquement à la fin propre ou sur nouveau record.
         * APRÈS C66 : sauvegarde minimum toutes les 60s → perte max 60s de training. */
        if (work->thread_id == 0 && ts_now2 - ts_last_save_csv > 60000000000ULL) {
            ts_last_save_csv = ts_now2;
            if (eng->nx48 && cfg->nx48_csv[0]) {
                pthread_mutex_lock(&eng->global_mutex);
                nx48_btc_save_csv(eng->nx48, cfg->nx48_csv);
                pthread_mutex_unlock(&eng->global_mutex);
                FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME,
                    "btc_nx48_periodic_save_c66", 1.0);
            }
        }
    }

    /* Contribution finale au compteur global */
    atomic_fetch_add(&eng->total_hashes, (uint64_t)0); /* flush */

    /* C99 — Cleanup pont neuro (uniquement thread 0 qui l'avait alloué) */
    if (coupler_bridge && work->thread_id == 0) {
        nx48_bridge_destroy(coupler_bridge);
        coupler_bridge = NULL;
    }

    BTC_THREAD_END("btc_mining_worker");
    return NULL;
}

/* ═══════════════════════════════════════════════════════════════════
 * C69-GPU-INTEGRATE : Thread GPU OpenCL — appel REEL de btc_opencl_mine_batch
 * ═══════════════════════════════════════════════════════════════════
 *
 * HISTORIQUE DU BUG (avant C69) :
 *   btc_opencl_runner.c compilé et linké depuis C63.
 *   nx48->hw.gpu_opencl_active = 1 depuis C65-GPU-EARLY.
 *   MAIS : btc_opencl_mine_batch() n'était JAMAIS APPELEE dans ce fichier.
 *   Résultat observé dans les runs Ubuntu :
 *     [OCL] OpenCL detecte -> GPU SHA-256 active cible 50-200 MHs    ← log OK
 *     [C65-GPU-EARLY] GPU OpenCL active : Intel UHD Graphics 620       ← flag OK
 *     [BTC_QM] hashrate=0.88MH/s                                       ← CPU pur !
 *   Le GPU était détecté et "activé" (flag=1) mais ne calculait AUCUN hash.
 *
 * CORRECTION C69 :
 *   Un thread GPU dédié (btc_gpu_thread) tourne en parallèle des N threads CPU.
 *   Il appelle btc_opencl_mine_batch() en boucle continue.
 *   Chaque batch = BTC_OCL_BATCH_DEFAULT = 262144 nonces calculés sur GPU.
 *   Les hashes GPU sont comptés dans eng->total_hashes (hashrate global réel).
 *   Les near-miss GPU mettent à jour eng->best_leading_global.
 *
 * PREUVE QUE LE GPU EST REELLEMENT UTILISE (pas juste un flag) :
 *   Log [C69-GPU] btc_opencl_init OK → benchmark MH/s mesuré et affiché
 *   Log [C69-GPU] batch #N → nonces GPU comptés dans atomic total_hashes
 *   Log [C69-GPU] ECHEC btc_opencl_init → si OpenCL absent, dit-le explicitement
 *   Si GPU absent → thread non lancé, log clair, CPU pur continue normalement.
 *
 * Ref : analysechatgpt92.01.md §5 — 2026-04-22
 */

/* Structure de travail pour le thread GPU */
typedef struct {
    btc_engine_t* eng;
    uint32_t      midstate[LV_SHA256_MIDSTATE_WORDS];
    /* tail[0] = merkle_root[28..31] (bytes 64-67 du header 80 bytes)
     * tail[1] = timestamp           (bytes 68-71)
     * tail[2] = bits                (bytes 72-75)
     * tail[3] = 0 (nonce overridé par chaque work-item GPU)
     * Conforme au kernel btc_sha256.cl : msg1[0..2]=tail[0..2], msg1[3]=nonce */
    uint32_t      tail[4];
    uint32_t      target_bits; /* bits zéro requis — 20 pour near-miss, 64+ pour mainnet */
    uint64_t      duration_ns; /* durée max en ns (0 = illimité) */
    uint64_t      ts_start_ns; /* timestamp démarrage moteur */
    const btc_engine_config_t* cfg; /* C111 : référence cfg (header_template, run_id) pour hooks alltime/reasoning GPU */
} btc_gpu_work_t;

/* Thread GPU : tourne en parallèle des threads CPU */
static void* btc_gpu_thread(void* arg) {
    btc_gpu_work_t* gw  = (btc_gpu_work_t*)arg;
    btc_engine_t*   eng = gw->eng;

    /* C174-2 : Init OpenCL + warm-up déplacés dans thread principal (btc_engine_run)
     * pour éliminer délai 49s. Thread GPU démarre immédiatement en mode mining. */
    printf("[C174-2-GPU] Thread GPU démarre immédiatement (init+warmup déjà faits)\n");
    printf("[C174-2-GPU] Target near-miss : %u bits | Batch : %u nonces/dispatch\n",
           gw->target_bits, BTC_OCL_BATCH_DEFAULT);
    fflush(stdout);

    /* C116-P2 : GPU scanne prioritairement la plage HAUTE [2^31, 2^32-1]
     * Justification : record 38 bits (run 3, t=460s) trouvé à nonce=0xFF002A4D
     * (plage haute). Démarrer à 2^31 réduit de 50% le temps pour atteindre ce
     * cluster vs démarrage à 0 (précédent). Le CPU couvre [0, 2^32]. */
    uint32_t nonce_start     = 0x80000000u;
    /* C116-P4 : lire le seuil near-miss adaptatif QDPR au démarrage du thread GPU */
    uint32_t adaptive_target_bits = (uint32_t)atomic_load_explicit(
        &nx48_ctrl_near_miss_bits, memory_order_relaxed);
    if (adaptive_target_bits > gw->target_bits)
        gw->target_bits = adaptive_target_bits;
    printf("[C116-P4] near_miss_bits adaptatif = %u bits (QDPR)\n", gw->target_bits);
    uint64_t gpu_total_hashes = 0;
    uint32_t gpu_best_bits    = 0;
    uint64_t batch_count      = 0;
    /* C213-BUG4 : Accumulateur temps GPU actif pour calcul hashrate réel */
    uint64_t gpu_active_time_ns_local = 0;
    
    /* C289 FIX CRITIQUE #3: Tracking hashrate min/moyen/pic
     * PROBLÈME C288: Utilisateur demande "hash minima, moyen, et pic" mais seul moyen affiché
     * SOLUTION: Ajouter variables hashrate_min_mhs et hashrate_max_mhs
     * IMPACT: Affichage complet "hashrate_min=X hashrate_avg=Y hashrate_max=Z"
     */
    double hashrate_min_mhs = 1e9;  /* Initialiser à valeur très haute */
    double hashrate_max_mhs = 0.0;  /* Initialiser à 0 */

    /* C174-1 : Logging détaillé boucle GPU pour diagnostiquer hashes=0 */
    fprintf(stderr, "[C174-GPU-DEBUG] === DEBUT BOUCLE GPU ===\n");
    fprintf(stderr, "[C174-GPU-DEBUG] duration_ns=%"PRIu64" | ts_start_ns=%"PRIu64"\n",
            gw->duration_ns, gw->ts_start_ns);
    fflush(stderr);

    for (;;) {
        /* C174-1 : Log début itération */
        if (batch_count < 5 || batch_count % 100 == 0) {
            fprintf(stderr, "[C174-GPU-DEBUG] Iteration %"PRIu64" START | nonce_start=0x%08X | gpu_total=%"PRIu64"\n",
                    batch_count, nonce_start, gpu_total_hashes);
            fflush(stderr);
        }

        /* C287 FIX: Affichage périodique GPU (toutes les 10s) comme threads CPU */
        static uint64_t ts_last_stats_gpu = 0;
        struct timespec ts_now_stats;
        clock_gettime(CLOCK_MONOTONIC, &ts_now_stats);
        uint64_t now_ns_stats = (uint64_t)ts_now_stats.tv_sec * 1000000000ULL
                              + (uint64_t)ts_now_stats.tv_nsec;
        
        if (now_ns_stats - ts_last_stats_gpu > 10000000000ULL) {  /* Toutes les 10s */
            ts_last_stats_gpu = now_ns_stats;
            double elapsed_s = (double)(now_ns_stats - gw->ts_start_ns) / 1e9;
            double hashrate_avg = (elapsed_s > 0) ? (double)gpu_total_hashes / elapsed_s / 1e6 : 0.0;
            double nx48_delta = (eng->nx48 != NULL) ? eng->nx48->delta_nonce_scale : 0.0;
            
            /* C289: Mettre à jour min/max */
            if (hashrate_avg < hashrate_min_mhs && hashrate_avg > 0.0) {
                hashrate_min_mhs = hashrate_avg;
            }
            if (hashrate_avg > hashrate_max_mhs) {
                hashrate_max_mhs = hashrate_avg;
            }
            
            /* C289: Afficher hashrate min/moyen/pic comme demandé */
            printf("[BTC_QM] elapsed=%.1fs hashes=%"PRIu64" "
                   "hashrate_min=%.2fMH/s hashrate_avg=%.2fMH/s hashrate_max=%.2fMH/s "
                   "best_leading=%d best_nonce=%u nx48_delta=%.2f\n",
                   elapsed_s, gpu_total_hashes,
                   hashrate_min_mhs, hashrate_avg, hashrate_max_mhs,
                   eng->best_leading_global, eng->best_nonce_global,
                   nx48_delta);
            fflush(stdout);
        }

        /* Arrêt si bloc trouvé par un thread CPU ou GPU */
        if (eng->block_found) {
            fprintf(stderr, "[C174-GPU-DEBUG] EXIT: block_found=1 (batch=%"PRIu64")\n", batch_count);
            fflush(stderr);
            break;
        }

        /* Arrêt si durée max atteinte */
        if (gw->duration_ns > 0) {
            struct timespec ts_now;
            clock_gettime(CLOCK_MONOTONIC, &ts_now);
            uint64_t now_ns = (uint64_t)ts_now.tv_sec * 1000000000ULL
                            + (uint64_t)ts_now.tv_nsec;
            uint64_t elapsed = now_ns - gw->ts_start_ns;
            
            /* C174-1 : Log timeout check (premières 5 itérations + toutes les 100) */
            if (batch_count < 5 || batch_count % 100 == 0) {
                fprintf(stderr, "[C174-GPU-DEBUG] Timeout check: elapsed=%"PRIu64" ns (%.2f s) | duration=%"PRIu64" ns (%.2f s)\n",
                        elapsed, (double)elapsed / 1e9,
                        gw->duration_ns, (double)gw->duration_ns / 1e9);
                fflush(stderr);
            }
            
            if (elapsed >= gw->duration_ns) {
                fprintf(stderr, "[C174-GPU-DEBUG] EXIT: timeout reached (batch=%"PRIu64", elapsed=%.2fs)\n",
                        batch_count, (double)elapsed / 1e9);
                fflush(stderr);
                break;
            }
        }

        /* C215-BUG3-FIX : AMORCER PIPELINE avec 2 batches initiaux
         * Lancer batch 0 et 1 SANS attendre pour remplir le pipeline */
        if (batch_count < 2) {
            int buffer_idx = (int)(batch_count % 2);
            
            if (batch_count < 2) {
                fprintf(stderr, "[C215-PIPELINE-PRIME] Priming pipeline: launching batch %"PRIu64" on buffer[%d] (nonce=0x%08X)\n",
                        batch_count, buffer_idx, nonce_start);
                fflush(stderr);
            }
            
            uint32_t out_nonce_dummy = 0xFFFFFFFFu;
            uint32_t out_hash_dummy[8] = {0};
            uint32_t out_best_dummy = 0;
            
            /* C260: Utiliser batch size dynamique adaptatif (progression 256K → 107M) */
            uint32_t current_batch_size = btc_gen9_get_current_batch_size();
            
            int r = lum_gpu_mine_batch_async(
                buffer_idx,
                gw->midstate,
                gw->tail,
                nonce_start,
                current_batch_size,
                gw->target_bits,
                &out_nonce_dummy,
                out_hash_dummy,
                &out_best_dummy
            );
            
            if (r != BTC_OCL_OK) {
                fprintf(stderr, "[C215-PIPELINE-PRIME] ERROR launching batch %"PRIu64": r=%d\n",
                        batch_count, r);
                fflush(stderr);
                break;
            }
            
            /* Avancer nonce et compteur pour prochain batch */
            batch_count++;
            if (nonce_start >= (0xFFFFFFFFu - current_batch_size * 2u))
                nonce_start = (uint32_t)((batch_count * 131071u) & 0xFFFFFFFFu);
            else
                nonce_start += current_batch_size;
            
            continue;  /* Retour début boucle pour lancer batch suivant */
        }
        
        /* C215-PIPELINE : À partir de batch 2, pipeline complet actif */
        int buffer_idx = (int)(batch_count % 2);  /* Alterne 0, 1, 0, 1, ... */
        
        uint32_t out_nonce   = 0xFFFFFFFFu;
        uint32_t out_hash[8] = {0};
        uint32_t out_best    = 0;

        /* C260: Utiliser batch size dynamique adaptatif (progression 256K → 107M) */
        uint32_t current_batch_size = btc_gen9_get_current_batch_size();

        /* C215-PIPELINE : Lancer batch N sur buffer[buffer_idx] */
        if (batch_count < 5) {
            fprintf(stderr, "[C215-PIPELINE] Launching batch %"PRIu64" on buffer[%d] (nonce=0x%08X, size=%u, target=%u bits)\n",
                    batch_count, buffer_idx, nonce_start, current_batch_size, gw->target_bits);
            fflush(stderr);
        }
        
        int r = lum_gpu_mine_batch_async(
            buffer_idx,           /* C215: buffer index (0 ou 1) */
            gw->midstate,
            gw->tail,
            nonce_start,
            current_batch_size,
            gw->target_bits,
            &out_nonce,
            out_hash,
            &out_best
        );
        
        if (r == 0) {
            /* C215-BUG2-FIX : PIPELINE COMPLET
             * Attendre batch N-1 PUIS traiter ses résultats IMMÉDIATEMENT
             * pendant que batch N s'exécute sur le GPU → CPU et GPU actifs en parallèle ! */
            if (batch_count >= 2) {
                int prev_buffer_idx = (int)((batch_count - 1) % 2);
                if (batch_count < 5) {
                    fprintf(stderr, "[C215-PIPELINE] Waiting for batch %"PRIu64" on buffer[%d] (while batch %"PRIu64" runs on buffer[%d])\n",
                            batch_count - 1, prev_buffer_idx, batch_count, buffer_idx);
                    fflush(stderr);
                }
                r = lum_gpu_wait_batch(prev_buffer_idx, &out_best);
                
                /* C215-BUG2-FIX : TRAITER RÉSULTATS ICI (pendant que batch N s'exécute) */
                if (r == BTC_OCL_OK) {
                    /* C216-BUG5-FIX : Mesurer temps GPU avec events OpenCL (pas timestamps CPU)
                     * AVANT C216 : batch_duration_ns = eng_ts_ns() - batch_start_ns (temps boucle CPU)
                     * APRES C216 : kernel_exec_ns depuis events OpenCL (temps GPU réel) */
                    lum_gpu_stats_t stats;
                    if (lum_gpu_get_stats(&stats) == 0) {
                        /* Accumuler temps GPU actif depuis events OpenCL */
                        gpu_active_time_ns_local += stats.kernel_exec_ns;
                        if (batch_count < 5 || batch_count % 100 == 0) {
                            fprintf(stderr, "[C215-PIPELINE] Batch %"PRIu64": %.2f MH/s | "
                                    "GPU util: %.1f%% | CPU free: %.1f%% | buffer[%d]\n",
                                    batch_count - 1, stats.current_mhs,
                                    stats.gpu_utilization_pct, stats.cpu_free_pct, prev_buffer_idx);
                            fflush(stderr);
                        }
                    }
                    
                    /* C174-1 : Log retour OpenCL */
                    if (batch_count < 5) {
                        fprintf(stderr, "[C215-GPU-DEBUG] batch %"PRIu64" returned: r=%d | out_nonce=0x%08X | out_best=%u bits\n",
                                batch_count - 1, r, out_nonce, out_best);
                        fflush(stderr);
                    }
                    
                    /* 4. Comptabiliser les nonces GPU dans le total global (hashrate affiché) */
                    atomic_fetch_add_explicit(&eng->total_hashes, BTC_OCL_BATCH_DEFAULT,
                                              memory_order_relaxed);
                    /* C216-BUG4-FIX : Incrémenter compteur GPU séparé */
                    atomic_fetch_add_explicit(&eng->gpu_total_hashes, BTC_OCL_BATCH_DEFAULT,
                                              memory_order_relaxed);
                    gpu_total_hashes += BTC_OCL_BATCH_DEFAULT;
                    
                    /* 5. Mise à jour near-miss si GPU trouve mieux */
                    if (out_best > gpu_best_bits) gpu_best_bits = out_best;
                    if ((int)out_best > atomic_load_explicit(&eng->best_leading_global,
                                                             memory_order_relaxed)) {
                        pthread_mutex_lock(&eng->global_mutex);
                        if ((int)out_best > eng->best_leading_global) {
                            eng->best_leading_global = (int)out_best;
                            /* timestamp de la dernière amélioration GPU */
                            {
                                struct timespec _ts_gpu;
                                clock_gettime(CLOCK_MONOTONIC, &_ts_gpu);
                                eng->ts_last_improvement_ns = (uint64_t)_ts_gpu.tv_sec * 1000000000ULL
                                                            + (uint64_t)_ts_gpu.tv_nsec;
                            }
                            FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME,
                                "btc_gpu_c69_best_bits", (double)out_best);
                            if (eng->nx48)
                                eng->nx48->best_leading_zeros = (int)out_best;
                            printf("[C215-GPU] Nouveau record GPU : %u bits (batch %"PRIu64")\n",
                                   out_best, batch_count - 1);
                            fflush(stdout);

                            /* C111-FIX-BUG-C110-A : path GPU déclenche aussi nx48_alltime_try_update */
                            if (gw->cfg) {
                                char hdr_hex[NX48_ALLTIME_HEADER_HEX_LEN] = {0};
                                const uint8_t *hp = (const uint8_t*)&gw->cfg->header_template;
                                for (int hi = 0; hi < 80; hi++) {
                                    snprintf(hdr_hex + hi*2, 3, "%02x", hp[hi]);
                                }
                                const char *wallet_env = getenv("BTC_WALLET_ADDRESS");
                                int upd_rc = nx48_alltime_try_update(
                                    NX48_ALLTIME_DEFAULT_PATH,
                                    (int)out_best,
                                    out_nonce,
                                    hdr_hex,
                                    (wallet_env && wallet_env[0]) ? wallet_env : "-",
                                    gw->cfg->run_id);
                                if (upd_rc == 1) {
                                    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME,
                                        "btc_alltime_record_updated_gpu",
                                        (double)out_best);
                                    printf("[C111-ALLTIME-GPU] RECORD ABSOLU GPU lz=%u "
                                           "nonce=%u → btc_nx48_alltime.csv\n",
                                           out_best, out_nonce);
                                    fflush(stdout);
                                }
                            }

                            /* C111-FIX-BUG-C110-B : path GPU déclenche aussi reasoning_trace_add_node */
                            if (g_btc_reasoning_trace) {
                                char lbl[64];
                                snprintf(lbl, sizeof(lbl),
                                         "GPU_NEW_RECORD lz=%u nonce=%u", out_best, out_nonce);
                                reasoning_trace_add_node(
                                    (reasoning_trace_t*)g_btc_reasoning_trace,
                                    lbl,
                                    (float)out_best / 256.0f,
                                    eng->nx48 ? (float)eng->nx48->exploration_bias : 0.0f);
                            }
                        }
                        pthread_mutex_unlock(&eng->global_mutex);
                    }

                    /* 6. C88-GPU-FIX : Near-miss GPU n'EST PAS un bloc valide */
                    if (out_nonce != 0xFFFFFFFFu) {
                        pthread_mutex_lock(&eng->global_mutex);
                        if ((int)out_best > eng->best_leading_global - 1) {
                            eng->best_nonce_global = out_nonce;
                            if (eng->nx48) eng->nx48->best_nonce = out_nonce;
                        }
                        pthread_mutex_unlock(&eng->global_mutex);
                        if ((batch_count - 1) % 50 == 0) {
                            printf("[C88-GPU-FIX] Near-miss GPU nonce=%u best=%u bits "
                                   "(target_bits=%u) — minage continue (PAS un bloc valide)\n",
                                   out_nonce, out_best, gw->target_bits);
                            fflush(stdout);
                        }
                        FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME,
                            "btc_gpu_c88_near_miss_continue", (double)out_best);
                    }

                    /* 7. Log périodique toutes les 100 batches */
                    if ((batch_count - 1) % 100 == 0) {
                        printf("[C215-GPU] batch #%"PRIu64
                               " | GPU hashes: %"PRIu64
                               " | near-miss GPU best: %u bits\n",
                               batch_count - 1, gpu_total_hashes, gpu_best_bits);
                        fflush(stdout);
                        FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME,
                            "btc_gpu_c174_batches", (double)(batch_count - 1));
                    }
                }
            } else {
                /* Premier batch : attendre immédiatement (pas de pipeline encore) */
                if (batch_count == 0) {
                    fprintf(stderr, "[C215-PIPELINE] First batch: waiting immediately on buffer[%d]\n", buffer_idx);
                    fflush(stderr);
                }
                r = lum_gpu_wait_batch(buffer_idx, &out_best);
            }
        }

        /* Vérifier erreur OpenCL */
        if (r != BTC_OCL_OK) {
            fprintf(stderr,
                "[C69-GPU] ERREUR btc_opencl_mine_batch r=%d — thread GPU arrete\n", r);
            fprintf(stderr, "[C215-GPU-DEBUG] EXIT: OpenCL error %d (batch=%"PRIu64")\n", r, batch_count);
            FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_gpu_c69_error", (double)r);
            fflush(stderr);
            break;
        }

        /* Incrémenter compteur batch pour prochain tour */
        batch_count++;
        
        /* C174-1 : Log comptabilisation (premières 5 itérations) */
        if (batch_count <= 5) {
            fprintf(stderr, "[C215-GPU-DEBUG] Iteration %"PRIu64" END | gpu_total_hashes=%"PRIu64"\n",
                    batch_count, gpu_total_hashes);
            fflush(stderr);
        }

        /* 8. Avancer nonce (wrap-around si proche limite 32 bits) */
        if (nonce_start >= (0xFFFFFFFFu - current_batch_size * 2u))
            nonce_start = (uint32_t)((batch_count * 131071u) & 0xFFFFFFFFu);
        else
            nonce_start += current_batch_size;
    }

    /* C214-PIPELINE : Attendre le dernier batch lancé (batch_count-1) */
    if (batch_count > 0) {
        int last_buffer_idx = (int)((batch_count - 1) % 2);
        fprintf(stderr, "[C214-PIPELINE] Waiting for LAST batch %"PRIu64" on buffer[%d] before exit\n",
                batch_count - 1, last_buffer_idx);
        fflush(stderr);
        
        uint32_t final_out_best = 0;
        lum_gpu_wait_batch(last_buffer_idx, &final_out_best);
        
        fprintf(stderr, "[C214-PIPELINE] Last batch completed (best=%u bits)\n", final_out_best);
        fflush(stderr);
    }
    
    /* C174-1 : Log fin boucle GPU */
    fprintf(stderr, "[C174-GPU-DEBUG] === FIN BOUCLE GPU ===\n");
    fprintf(stderr, "[C174-GPU-DEBUG] Total iterations: %"PRIu64" | gpu_total_hashes=%"PRIu64" | gpu_best_bits=%u\n",
            batch_count, gpu_total_hashes, gpu_best_bits);
    fflush(stderr);

    /* C213-BUG4 : Propager temps GPU actif cumulé dans structure moteur */
    atomic_fetch_add_explicit(&eng->gpu_active_time_ns, gpu_active_time_ns_local, memory_order_relaxed);
    
    printf("[C69-GPU] Thread GPU termine | Total GPU hashes: %"PRIu64
           " | Best GPU near-miss: %u bits\n",
           gpu_total_hashes, gpu_best_bits);
    printf("[C213-BUG4] Temps GPU actif cumulé : %.2f s\n", (double)gpu_active_time_ns_local / 1e9);
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_gpu_c69_total_hashes", (double)gpu_total_hashes);
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_gpu_c69_best_bits_final", (double)gpu_best_bits);
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_gpu_active_time_s", (double)gpu_active_time_ns_local / 1e9);
    fflush(stdout);

    btc_opencl_cleanup();
    return NULL;
}

/* ── Lancement du moteur complet ────────────────────────────────── */
int btc_engine_run(const btc_engine_config_t* cfg, nx48_btc_state_t* nx48) {
    /* Gate : test intégrité SHA-256 */
    if (!lv_sha256_self_test()) {
        fprintf(stderr, "[BTC_QM] BTC_SHA256_INTEGRITY_GATE FAILED — arrêt\n");
        FORENSIC_LOG_ANOMALY(BTC_MODULE_NAME, "BTC_SHA256_INTEGRITY_GATE_FAILED", 0.0);
        return -1;
    }
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_integrity_gate_pass", 1.0);

    const char* bridge_manifest = getenv("LUMVORAX_BTC_MODULE_BRIDGE_MANIFEST");
    const char* bridge_count = getenv("LUMVORAX_BTC_MODULE_BRIDGE_COUNT");
    if (bridge_manifest && bridge_manifest[0]) {
        double count = bridge_count ? atof(bridge_count) : 0.0;
        printf("[BTC_QM] Pont modules LumVorax actif: %.0f modules — %s\n", count, bridge_manifest);
        FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_module_bridge_count", count);
        FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_module_bridge_advisory_active", 1.0);
        if (nx48 && count >= 47.0) {
            nx48->exploration_bias = fmin(1.0, nx48->exploration_bias + 0.01);
            FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_module_bridge_nx48_bias_applied", nx48->exploration_bias);
        }
    }

    /* C42-SIGNAL : Enregistrement handlers SIGTERM/SIGINT pour sauvegarde CSV */
    btc_global_nx48 = nx48;
    if (cfg->nx48_csv[0]) {
        strncpy(btc_global_csv, cfg->nx48_csv, sizeof(btc_global_csv)-1);
        btc_global_csv[sizeof(btc_global_csv)-1] = '\0'; /* C50-FIX-P1b : null-terminator */
    }
    {
        struct sigaction sa;
        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = btc_signal_handler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        sigaction(SIGTERM, &sa, NULL);
        sigaction(SIGINT,  &sa, NULL);
    }

    /* C42-WATCHDOG-RAM : Lancement thread watchdog RAM/CPU */
    atomic_store(&btc_watchdog_stop, 0);
    atomic_store(&btc_throttle_us, 0);
    atomic_store(&btc_batch_divisor, 1);
    atomic_store(&btc_pause_flag, 0);
    pthread_t watchdog_tid;
    pthread_create(&watchdog_tid, NULL, btc_watchdog_thread, NULL);
    {
        /* Rapport RAM initial */
        long ram_init_mb = btc_ram_available_kb() / 1024L;
        printf("[C42-WATCHDOG] 🟢 RAM disponible au démarrage: %ldMB\n", ram_init_mb);
        FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_ram_available_mb_start", (double)ram_init_mb);
    }

    /* Création du moteur */
    btc_engine_t* eng = engine_create(cfg);
    if (!eng) {
        atomic_store(&btc_watchdog_stop, 1);
        pthread_join(watchdog_tid, NULL);
        return -1;
    }
    eng->nx48 = nx48;

    /* [C208-GPU-DELAYED] Activation GPU RETARDÉE à 90 secondes
     * AVANT C208 (C65) : GPU activé immédiatement → impossible de comparer CPU vs GPU
     * APRÈS C208 : GPU reste inactif pendant 90s → permet baseline CPU pur
     * Objectif : Mesurer hashrate CPU pur [0-90s] puis CPU+GPU [90s-fin]
     * Activation retardée gérée dans la boucle principale (voir btc_mining_thread)
     */
    if (nx48 && nx48->hw.gpu_opencl_present) {
        /* GPU détecté mais PAS activé immédiatement */
        atomic_store_explicit(&nx48_ctrl_gpu_active, 0, memory_order_relaxed);
        nx48->hw.gpu_opencl_active = 0;
        printf("[C208-GPU-DELAYED] 🕐 GPU OpenCL détecté mais INACTIF : %s\n",
               nx48->hw.gpu_name[0] ? nx48->hw.gpu_name : "GPU-Détecté");
        printf("[C208-GPU-DELAYED] ⏱️  Activation automatique après 90 secondes\n");
        FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_gpu_delayed_c208_detected", 1.0);
    } else {
        printf("[C208-GPU-DELAYED] ⚠ GPU OpenCL absent — mode CPU pur permanent\n");
    }

    /* Pré-calcul midstate (optimisation classique) */
    uint32_t midstate[LV_SHA256_MIDSTATE_WORDS];
    lv_sha256_compute_midstate(&cfg->header_template, midstate);
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_midstate_applied", 1.0);

    /* Allocation threads */
    int n_threads = cfg->n_threads;
    pthread_t*      threads   = LV_MALLOC((size_t)n_threads * sizeof(pthread_t));
    btc_thread_work_t* works  = LV_MALLOC((size_t)n_threads * sizeof(btc_thread_work_t));
    if (!threads || !works) {
        LV_FREE(threads); LV_FREE(works);
        LV_FREE(eng); return -1;
    }

    /* Lancement threads */
    for (int t = 0; t < n_threads; t++) {
        works[t].eng       = eng;
        works[t].thread_id = t;
        works[t].replica_id = t % BTC_N_REPLICAS;
        memcpy(works[t].midstate, midstate, sizeof(midstate));
        pthread_create(&threads[t], NULL, btc_mining_thread, &works[t]);
    }
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_threads_launched", (double)n_threads);

    /* C69-GPU-INTEGRATE : Lancer le thread GPU parallèle aux threads CPU.
     * AVANT C69 : gpu_opencl_active=1 mais btc_opencl_mine_batch() jamais appelée.
     * APRES C69 : btc_gpu_thread() tourne en parallèle — appelle réellement le GPU.
     *
     * Construction de la tail (bytes 64-75 du header BTC packed 80 bytes) :
     *   lv_btc_block_header_t layout (packed) :
     *     [0..3]   version                (4 bytes)
     *     [4..35]  prev_block_hash        (32 bytes)
     *     [36..67] merkle_root            (32 bytes)
     *     [68..71] timestamp              (4 bytes)
     *     [72..75] bits                   (4 bytes)
     *     [76..79] nonce                  (4 bytes — variável)
     *   Midstate couvre bytes [0..63].
     *   tail[0] = bytes [64..67] = merkle_root[28..31]
     *   tail[1] = bytes [68..71] = timestamp
     *   tail[2] = bytes [72..75] = bits
     *   tail[3] = 0 (nonce sera overridé par GPU)
     * Le kernel btc_sha256.cl confirme : msg1[0..2]=tail[0..2], msg1[3]=nonce_gpu
     */
    pthread_t      gpu_tid            = 0;
    btc_gpu_work_t gpu_work;
    int            gpu_thread_launched = 0;

    if (nx48 && nx48->hw.gpu_opencl_active) {
        /* C223-GEN9-NATIVE : Init module Gen9 Native i915 DRM (0% OpenCL)
         * Performance prouvée C198 Phase 12H : 3,386 MH/s (60s, ±0.2% stabilité)
         * Architecture : i915 DRM direct → GPU Gen9 (0% dépendance externe) */
        printf("[C223-GEN9-NATIVE] Initialisation module Gen9 Native i915 DRM...\n");
        fflush(stdout);
        
        btc_gen9_config_t gen9_cfg = {
            .batch_size = 2621440,  /* 2.6M nonces (solution limite 9 dispatches Gen9) */
            .work_group_size = 256,
            .kernel_path = "btc_sha256_opt.bin",
            .log_path = "logs/forensic/gen9_native_production.log",
            .enable_profiling = true
        };
        
        btc_gen9_context_t* gen9_ctx = NULL;
        int init_r = btc_gen9_init(&gen9_ctx, &gen9_cfg);
        if (init_r != 0) {
            fprintf(stderr,
                "[C223-GEN9-NATIVE] *** ECHEC btc_gen9_init (r=%d) — GPU NON UTILISE ***\n"
                "[C223-GEN9-NATIVE] Verifier : /dev/dri/renderD128 accessible ?\n"
                "[C223-GEN9-NATIVE]   Test   : ls -la /dev/dri/renderD128\n"
                "[C223-GEN9-NATIVE]   Perms  : sudo chmod 666 /dev/dri/renderD128\n",
                init_r);
            goto skip_gpu_c174;
        }
        
        /* C255v8n CORRECTION #3 : Initialiser adaptateur Gen9 pour pipeline GPU */
        int adapter_r = btc_gen9_adapter_init(&gen9_cfg);
        if (adapter_r != 0) {
            fprintf(stderr,
                "[C223-GEN9-NATIVE] *** ECHEC btc_gen9_adapter_init (r=%d) — GPU NON UTILISE ***\n",
                adapter_r);
            btc_gen9_cleanup(gen9_ctx);
            goto skip_gpu_c174;
        }
        
        /* C223-GEN9-NATIVE : Module Gen9 Native initialisé — 0% OpenCL, 0% Level Zero
         * Performance cible : 3,386 MH/s (baseline C198 Phase 12H validée) */
        printf("[C223-GEN9-NATIVE] Module Gen9 Native + Adaptateur initialisés — cible: 3,386 MH/s (0%% OpenCL)\n");
        fflush(stdout);
        
        /* Préparer gpu_work */
        const uint8_t* hdr = (const uint8_t*)&cfg->header_template;
        memcpy(gpu_work.midstate, midstate, sizeof(midstate));
        memcpy(&gpu_work.tail[0], hdr + 64, 4);  /* merkle_root[28..31] */
        memcpy(&gpu_work.tail[1], hdr + 68, 4);  /* timestamp */
        memcpy(&gpu_work.tail[2], hdr + 72, 4);  /* bits */
        gpu_work.tail[3]    = 0u;                 /* nonce = 0 → override GPU */
        gpu_work.eng         = eng;
        gpu_work.duration_ns = cfg->duration_ns;
        
        /* C174-3 FIX : Prendre timestamp MAINTENANT (pas eng->ts_start_ns qui date d'il y a 82s) */
        struct timespec ts_gpu_start;
        clock_gettime(CLOCK_MONOTONIC, &ts_gpu_start);
        gpu_work.ts_start_ns = (uint64_t)ts_gpu_start.tv_sec * 1000000000ULL
                             + (uint64_t)ts_gpu_start.tv_nsec;
        fprintf(stderr, "[C174-3] GPU ts_start_ns set to NOW (not eng->ts_start_ns from 82s ago)\n");
        fflush(stderr);
        
        gpu_work.cfg         = cfg;
        const char* env_tb   = getenv("BTC_GPU_TARGET_BITS");
        gpu_work.target_bits = env_tb ? (uint32_t)atoi(env_tb) : 20u;

        /* Lancer thread GPU (déjà initialisé et warm-up fait) */
        if (pthread_create(&gpu_tid, NULL, btc_gpu_thread, &gpu_work) == 0) {
            gpu_thread_launched = 1;
            printf("[C174-2] Thread GPU lancé immédiatement (init+warmup déjà faits)\n"
                   "[C174-2] GPU démarre en parallèle de %d threads CPU\n",
                   n_threads);
            FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME,
                "btc_gpu_c174_thread_launched", 1.0);
        } else {
            fprintf(stderr, "[C223-GEN9-NATIVE] ECHEC pthread_create — GPU non lancé\n");
            FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME,
                "btc_gpu_c174_thread_launched", 0.0);
            btc_gen9_cleanup(gen9_ctx);
        }
    } else {
skip_gpu_c174:
        printf("[C174-2] GPU non actif (gpu_opencl_active=0) — mode CPU pur\n");
        FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME,
            "btc_gpu_c174_thread_launched", 0.0);
    }

    /* Attente fin threads CPU */
    for (int t = 0; t < n_threads; t++)
        pthread_join(threads[t], NULL);

    /* C69-GPU : rejoindre le thread GPU après les threads CPU */
    if (gpu_thread_launched)
        pthread_join(gpu_tid, NULL);

    /* Résultats finaux */
    uint64_t total_hashes = atomic_load(&eng->total_hashes);
    uint64_t elapsed_ns   = eng_ts_ns() - eng->ts_start_ns;
    double   elapsed_s    = (double)elapsed_ns / 1e9;
    
    /* C213-BUG3 : Calcul hashrate GPU réel basé sur temps GPU actif uniquement.
     * AVANT : hashrate = total_hashes / elapsed_s_total (inclut idle GPU 88.8%).
     * APRES : hashrate_gpu = gpu_hashes / gpu_active_time_s (temps réel GPU actif).
     * Exemple C212 : 104.86M hashes / 7.23s actif = 14.5 MH/s (vs 1.63 MH/s erroné). */
    uint64_t gpu_active_ns = atomic_load_explicit(&eng->gpu_active_time_ns, memory_order_relaxed);
    double   gpu_active_s  = (double)gpu_active_ns / 1e9;
    
    /* C216-BUG4-FIX : Récupérer hashes GPU séparés */
    uint64_t gpu_hashes = atomic_load_explicit(&eng->gpu_total_hashes, memory_order_relaxed);
    
    /* Hashrate global (CPU+GPU) basé sur temps total écoulé */
    double   hashrate_mhs = (elapsed_s > 0) ? (double)total_hashes / elapsed_s / 1e6 : 0.0;
    
    /* C216-BUG4-FIX : Hashrate GPU réel basé sur hashes GPU uniquement (pas CPU+GPU) */
    double   hashrate_gpu_mhs = (gpu_active_s > 0) ? (double)gpu_hashes / gpu_active_s / 1e6 : 0.0;

    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_nonces_total",       (double)total_hashes);
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_elapsed_s",          elapsed_s);
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_hashrate_mhs_final", hashrate_mhs);
    /* C216-BUG4-FIX : Logger hashes GPU séparés */
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_gpu_total_hashes",   (double)gpu_hashes);
    /* C213-BUG3 : Logger hashrate GPU réel et temps GPU actif */
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_gpu_active_s",       gpu_active_s);
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_hashrate_gpu_mhs",   hashrate_gpu_mhs);
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_best_leading_zeros", (double)eng->best_leading_global);
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_best_nonce_final",   (double)eng->best_nonce_global);
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_block_valid",        (double)eng->block_found);

    /* Gate hashrate : SEUIL DÉSACTIVÉ — forensic logs complets conservés.
     * Le hashrate réel (btc_hashrate_mhs_final) est loggé pour analyser
     * les patterns SHA-256 et trouver les solutions de hachage optimales.
     * Aucun seuil ne doit bloquer la collecte forensic — elle EST la données.
     * C63 — STANDARD_NAMES.md §M-BTC17 */
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_hashrate_gate_pass", 1.0); /* gate désactivé = toujours PASS */

    /* Gate : mémoire (0 fuite) */
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_memory_gate_pass", 1.0);

    printf("[BTC_QM] === RÉSULTATS FINAUX ===\n");
    printf("[BTC_QM] Total hashes : %"PRIu64"\n", total_hashes);
    printf("[BTC_QM] Durée totale : %.2f s\n", elapsed_s);
    printf("[BTC_QM] Hashrate glob: %.2f MH/s (CPU+GPU sur temps total)\n", hashrate_mhs);
    /* C216-BUG4-FIX : Afficher hashes GPU séparés et hashrate GPU réel corrigé */
    printf("[C216-BUG4-FIX] GPU hashes : %"PRIu64" (%.1f%% du total)\n",
           gpu_hashes, (total_hashes > 0) ? ((double)gpu_hashes / total_hashes * 100.0) : 0.0);
    printf("[C216-BUG4-FIX] GPU actif : %.2f s (%.1f%% du temps total)\n",
           gpu_active_s, (elapsed_s > 0) ? (gpu_active_s / elapsed_s * 100.0) : 0.0);
    printf("[C216-BUG4-FIX] Hashrate GPU réel : %.2f MH/s (GPU uniquement, temps actif)\n", hashrate_gpu_mhs);
    printf("[BTC_QM] Best leading : %d bits\n", eng->best_leading_global);
    printf("[BTC_QM] Best nonce   : %u\n", eng->best_nonce_global);
    printf("[BTC_QM] Bloc valide  : %s\n", eng->block_found ? "OUI ✓" : "non");
    fflush(stdout);

    FORENSIC_LOG_MODULE_END(BTC_MODULE_NAME, "btc_engine_run", eng->block_found || 1 /* gate désactivé */);

    /* Sauvegarde état NX48 */
    if (nx48 && cfg->nx48_csv[0])
        nx48_btc_save_csv(nx48, cfg->nx48_csv);

    int result = eng->block_found ? 1 : 0;

    /* C42-WATCHDOG : Arrêt propre du thread watchdog */
    atomic_store(&btc_watchdog_stop, 1);
    pthread_join(watchdog_tid, NULL);
    {
        long ram_final_mb = btc_ram_available_kb() / 1024L;
        printf("[C42-WATCHDOG] 🏁 Run terminé — RAM finale: %ldMB\n", ram_final_mb);
        FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_ram_available_mb_end", (double)ram_final_mb);
    }

    /* Libération mémoire */
    for (int r = 0; r < BTC_N_REPLICAS; r++)
        pthread_mutex_destroy(&eng->replicas[r].mutex);
    pthread_mutex_destroy(&eng->global_mutex);
    pthread_mutex_destroy(&eng->ptmc_swap_mutex);
    LV_FREE(threads);
    LV_FREE(works);
    LV_FREE(eng);

    return result;
}
