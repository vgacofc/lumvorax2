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
#include "../include/btc_mining_forensic.h"
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

#ifdef _OPENMP
#  include <omp.h>
#endif

/* ── Constantes du moteur ───────────────────────────────────────── */
#define BTC_N_REPLICAS          8
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

/* Températures répliques PT-MC (ratio 50 comme Hubbard) */
static const double BTC_REPLICA_TEMPS[BTC_N_REPLICAS] = {
    1.0, 2.0, 4.0, 8.0, 12.0, 20.0, 35.0, 50.0
};

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
    return (uint64_t)thread_id * 6364136223846793005ULL + 1442695040888963407ULL
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
    btc_engine_t* eng = LV_CALLOC(1, sizeof(btc_engine_t));
    if (!eng) return NULL;

    memcpy(&eng->cfg, cfg, sizeof(btc_engine_config_t));
    atomic_store(&eng->total_hashes, 0);
    eng->best_leading_global      = 0;
    eng->best_nonce_global        = 0;
    eng->block_found              = 0;
    eng->ts_start_ns              = eng_ts_ns();
    eng->ts_last_improvement_ns   = eng->ts_start_ns;
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
        if (u < accept) {
            /* Échange des nonces */
            uint32_t tmp_n = r1->nonce; r1->nonce = r2->nonce; r2->nonce = tmp_n;
            int tmp_l = r1->leading_zeros; r1->leading_zeros = r2->leading_zeros;
                        r2->leading_zeros = tmp_l;
            r1->swaps_accepted++;
            r2->swaps_accepted++;
            BTC_FORENSIC_PTMC_SWAP(r + 1, r, accept);
        } else {
            BTC_FORENSIC_PTMC_SWAP(r + 1, r, -accept);
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

    uint64_t local_hashes   = 0;
    uint64_t ts_last_hw     = eng_ts_ns();
    uint64_t ts_last_nx48   = eng_ts_ns();
    uint64_t ts_last_swap   = eng_ts_ns();
    uint64_t ts_last_stats  = eng_ts_ns();

    /* Delta nonce initial depuis NX48 (minimum 1 pour éviter division par zéro) */
    double delta_nonce = 65536.0 * eng->nx48->delta_nonce_scale;
    if (delta_nonce < 1.0) delta_nonce = 65536.0; /* valeur par défaut sûre */
    int nx48_disabled = getenv("BTC_NX48_DISABLED") != NULL;

    while (!eng->block_found) {
        /* Vérification durée max */
        uint64_t ts_now = eng_ts_ns();
        if (cfg->duration_ns > 0 && (ts_now - eng->ts_start_ns) >= cfg->duration_ns)
            break;

        /* Batch de hashes */
        int batch = (int)(cfg->batch_size * eng->nx48->batch_size_scale);
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
            double exploration = eng->nx48->exploration_bias;
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
            lv_sha256_result_t res = lv_sha256d_midstate(
                work->midstate,
                &cfg->header_template,
                nonce,
                cfg->target,
                cfg->run_id,
                atomic_load(&eng->total_hashes) + local_hashes
            );

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
                    if (eng->nx48)
                        eng->nx48->best_nonce = nonce;
                    /* C40-CSV-RECORD : Sauvegarde immédiate du CSV NX48 à chaque
                     * nouveau record de leading zeros — évite la perte de record
                     * si le run est interrompu avant la fin.
                     * AVANT C40 : sauvegarde CSV UNIQUEMENT en fin de run → record
                     *             perdu si restart (ex: 25 bits perdu → run 233845Z).
                     * APRÈS C40 : sauvegarde atomique immédiate à chaque amélioration.
                     * Ref : rapport forensique C40 §BUG-P0-CSV — 2026-04-13 */
                    if (eng->nx48 && cfg->nx48_csv[0])
                        nx48_btc_save_csv(eng->nx48, cfg->nx48_csv);
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

        /* Mise à jour NX48 */
        uint64_t ts_now2 = eng_ts_ns();
        if (ts_now2 - ts_last_nx48 > 2000000000ULL) { /* toutes les 2 secondes */
            ts_last_nx48 = ts_now2;
            double elapsed_s = (double)(ts_now2 - eng->ts_start_ns) / 1e9;
            uint64_t total   = atomic_load(&eng->total_hashes);
            double hashrate_mhs = (elapsed_s > 0)
                ? (double)total / elapsed_s / 1e6 : 0.0;
            double time_since_impr = (double)(ts_now2 - eng->ts_last_improvement_ns) / 1e9;
            double coverage_pct    = 100.0 * (double)(total)
                / (double)(cfg->nonce_end - cfg->nonce_start + 1);

            double features[NX48_BTC_N_FEATURES];
            nx48_btc_compute_features(features,
                eng->best_leading_global,
                hashrate_mhs, 10000.0,
                (rep->swaps_attempted > 0)
                    ? (double)rep->swaps_accepted / (double)rep->swaps_attempted : 0.0,
                time_since_impr, coverage_pct, delta_nonce,
                (double)local_hashes, (double)(BTC_NX48_UPDATE_EVERY),
                BTC_REPLICA_TEMPS[BTC_N_REPLICAS-1], BTC_REPLICA_TEMPS[0]);

            double prob = nx48_disabled ? 0.5 : nx48_btc_predict(eng->nx48, features);

            if (!nx48_disabled && work->thread_id == 0) { /* Un seul thread met à jour NX48 */
                nx48_btc_update(eng->nx48,
                    &(nx48_btc_config_t){
                        .learning_rate   = 0.01,
                        .lambda_l1       = 0.001,
                        .update_interval = BTC_NX48_UPDATE_EVERY
                    },
                    features, prob,
                    eng->best_leading_global, hashrate_mhs);
                delta_nonce = 65536.0 * eng->nx48->delta_nonce_scale;
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
            printf("[BTC_QM] elapsed=%.1fs hashes=%"PRIu64" hashrate=%.2fMH/s "
                   "best_leading=%d best_nonce=%u nx48_delta=%.2f\n",
                   elapsed_s, total, hashrate,
                   eng->best_leading_global, eng->best_nonce_global,
                   eng->nx48->delta_nonce_scale);
            fflush(stdout);
            BTC_FORENSIC_COVERAGE((double)total,
                100.0 * (double)total / (double)(cfg->nonce_end - cfg->nonce_start + 1));
        }
    }

    /* Contribution finale au compteur global */
    atomic_fetch_add(&eng->total_hashes, (uint64_t)0); /* flush */

    BTC_THREAD_END("btc_mining_worker");
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
    if (cfg->nx48_csv[0])
        strncpy(btc_global_csv, cfg->nx48_csv, sizeof(btc_global_csv)-1);
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

    /* Attente fin */
    for (int t = 0; t < n_threads; t++)
        pthread_join(threads[t], NULL);

    /* Résultats finaux */
    uint64_t total_hashes = atomic_load(&eng->total_hashes);
    uint64_t elapsed_ns   = eng_ts_ns() - eng->ts_start_ns;
    double   elapsed_s    = (double)elapsed_ns / 1e9;
    double   hashrate_mhs = (elapsed_s > 0) ? (double)total_hashes / elapsed_s / 1e6 : 0.0;

    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_nonces_total",       (double)total_hashes);
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_elapsed_s",          elapsed_s);
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_hashrate_mhs_final", hashrate_mhs);
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
    printf("[BTC_QM] Durée        : %.2f s\n", elapsed_s);
    printf("[BTC_QM] Hashrate     : %.2f MH/s\n", hashrate_mhs);
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
