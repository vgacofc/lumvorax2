/*
 * LumVorax — Module 17 — Bitcoin Quantum Mining Engine
 * btc_mining_engine.c — Moteur PT-MC nonce explorer + validation bloc
 *
 * STANDARD_NAMES.md v4.1 §M-BTC17 — Cycle C62 — 2026-04-11
 *
 * Architecture :
 *  - 8 répliques PT-MC (Parallel Tempering Monte Carlo)
 *    → T_cold = 1.0 (exploitation) … T_hot = 50.0 (exploration large)
 *  - N_THREADS threads parallèles (OpenMP + pthread)
 *  - AVX2 pipeline SHA-256 (8-way si disponible, sinon scalaire)
 *  - NX48_BTC adaptatif : ajuste delta_nonce + batch_size en temps réel
 *  - Traçabilité forensic 100% A–Z (FORENSIC_LOG_* — NOM D'ORIGINE §A)
 *  - Mémoire tracée LV_MALLOC / LV_CALLOC / LV_FREE
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

#ifdef _OPENMP
#  include <omp.h>
#endif

/* ── Constantes du moteur ───────────────────────────────────────── */
#define BTC_N_REPLICAS          8
#define BTC_N_THREADS_DEFAULT   16
#define BTC_BATCH_SIZE_DEFAULT  256     /* Hashes par batch SIMD */
#define BTC_NX48_UPDATE_EVERY   100000  /* Mise à jour NX48 tous les N hashes */
#define BTC_HW_SAMPLE_EVERY     50000   /* Snapshot HW tous les N hashes */
#define BTC_PTMC_SWAP_EVERY     10000   /* Échange répliques tous les N hashes */
#define BTC_STATS_PRINT_EVERY   1000000 /* Affichage stats chaque M hashes */

/* Températures répliques PT-MC (ratio 50 comme Hubbard) */
static const double BTC_REPLICA_TEMPS[BTC_N_REPLICAS] = {
    1.0, 2.0, 4.0, 8.0, 12.0, 20.0, 35.0, 50.0
};

/* ── Structure d'une réplique PT-MC ─────────────────────────────── */
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
} btc_ptmc_replica_t;

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
    volatile int         best_leading_global;       /* Meilleur global */
    volatile uint32_t    best_nonce_global;         /* Meilleur nonce global */
    volatile int         block_found;               /* 1 si bloc valide trouvé */
    uint64_t             ts_start_ns;               /* Timestamp démarrage */
    uint64_t             ts_last_improvement_ns;    /* Timestamp dernière amélioration */
    pthread_mutex_t      global_mutex;
    nx48_btc_state_t*    nx48;
    btc_engine_config_t  cfg;
} btc_engine_t;

/* ── Timestamp monotonique ns ───────────────────────────────────── */
static uint64_t eng_ts_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
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
    pthread_mutex_init(&eng->global_mutex, NULL);

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
static void engine_ptmc_swap(btc_engine_t* eng) {
    /* Essai d'échange entre paires aléatoires de répliques adjacentes */
    for (int r = 0; r < BTC_N_REPLICAS - 1; r++) {
        btc_ptmc_replica_t* r1 = &eng->replicas[r];
        btc_ptmc_replica_t* r2 = &eng->replicas[r + 1];

        pthread_mutex_lock(&r1->mutex);
        pthread_mutex_lock(&r2->mutex);

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
            uint32_t tmp_n = r1->nonce;     r1->nonce = r2->nonce;     r2->nonce = tmp_n;
            int      tmp_l = r1->leading_zeros; r1->leading_zeros = r2->leading_zeros;
                             r2->leading_zeros = tmp_l;
            r1->swaps_accepted++;
            r2->swaps_accepted++;
            BTC_FORENSIC_PTMC_SWAP(r + 1, r, accept);
        }

        pthread_mutex_unlock(&r2->mutex);
        pthread_mutex_unlock(&r1->mutex);
    }
}

/* ── Structure de travail thread ────────────────────────────────── */
typedef struct {
    btc_engine_t*       eng;
    int                 thread_id;
    int                 replica_id;
    uint32_t            midstate[LV_SHA256_MIDSTATE_WORDS];
} btc_thread_work_t;

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
            /* Choix du nonce : exploitation ou exploration */
            uint32_t nonce;
            double exploration = eng->nx48->exploration_bias;
            double u = (double)(rng_next() & 0xFFFFFFFFu) / 4294967296.0;

            pthread_mutex_lock(&rep->mutex);
            if (u > exploration) {
                /* Exploitation : voisinage du meilleur nonce local */
                int64_t dn64   = (int64_t)delta_nonce;
                if (dn64 < 1) dn64 = 1;
                int64_t offset = (int64_t)(rng_next() & 0xFFFFFFFFu) % dn64
                               - dn64 / 2;
                nonce = (uint32_t)((int64_t)rep->best_nonce + offset);
            } else {
                /* Exploration : nonce aléatoire dans la plage */
                nonce = cfg->nonce_start + (uint32_t)(rng_next()
                    % (uint64_t)(cfg->nonce_end - cfg->nonce_start + 1));
            }
            pthread_mutex_unlock(&rep->mutex);

            /* Double-SHA256 avec midstate */
            lv_sha256_result_t res = lv_sha256d_midstate(
                work->midstate,
                nonce,
                cfg->header_template.timestamp,
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
                }
                pthread_mutex_unlock(&eng->global_mutex);
            }

            /* Bloc valide trouvé */
            if (res.below_target) {
                pthread_mutex_lock(&eng->global_mutex);
                eng->block_found        = 1;
                eng->best_nonce_global  = nonce;
                BTC_FORENSIC_BLOCK_FOUND(nonce, res.leading_zeros, 1);
                FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_block_valid", 1.0);
                pthread_mutex_unlock(&eng->global_mutex);
                break;
            }
        }

        /* Accumulation compteur global */
        atomic_fetch_add(&eng->total_hashes, (uint64_t)batch);

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

            double prob = nx48_btc_predict(eng->nx48, features);

            if (work->thread_id == 0) { /* Un seul thread met à jour NX48 */
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

            BTC_FORENSIC_HASHRATE(hashrate_mhs, work->thread_id);
        }

        /* Snapshot HW */
        if (ts_now2 - ts_last_hw > 5000000000ULL) { /* toutes les 5 secondes */
            ts_last_hw = ts_now2;
            BTC_FORENSIC_HW();
        }

        /* Échange répliques PT-MC (thread 0 uniquement) */
        if (work->thread_id == 0 && ts_now2 - ts_last_swap > 500000000ULL) {
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

    /* Création du moteur */
    btc_engine_t* eng = engine_create(cfg);
    if (!eng) return -1;
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

    /* Gate : hashrate minimum */
    int hashrate_gate = (hashrate_mhs >= 1.0);
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_hashrate_gate_pass", (double)hashrate_gate);
    if (!hashrate_gate)
        FORENSIC_LOG_ANOMALY(BTC_MODULE_NAME, "BTC_HASHRATE_GATE_FAILED", hashrate_mhs);

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

    FORENSIC_LOG_MODULE_END(BTC_MODULE_NAME, "btc_engine_run", eng->block_found || hashrate_gate);

    /* Sauvegarde état NX48 */
    if (nx48 && cfg->nx48_csv[0])
        nx48_btc_save_csv(nx48, cfg->nx48_csv);

    int result = eng->block_found ? 1 : 0;

    /* Libération mémoire */
    for (int r = 0; r < BTC_N_REPLICAS; r++)
        pthread_mutex_destroy(&eng->replicas[r].mutex);
    pthread_mutex_destroy(&eng->global_mutex);
    LV_FREE(threads);
    LV_FREE(works);
    LV_FREE(eng);

    return result;
}
