#ifndef BTC_MINING_FORENSIC_H
#define BTC_MINING_FORENSIC_H

/*
 * LumVorax — Module 17 — Bitcoin Quantum Mining Engine
 * btc_mining_forensic.h — Macros forensic spécialisées BTC
 *
 * STANDARD_NAMES.md v4.1 §M-BTC17
 * Toutes les macros BTC_FORENSIC_* appellent en interne
 * FORENSIC_LOG_MODULE_METRIC (NOM D'ORIGINE §A) — NE JAMAIS RENOMMER.
 *
 * Conformité : STANDARD_NAMES.md v4.1 — Cycle C62
 */

#include "ultra_forensic_logger.h"

/* ── Nom canonique du module 17 ─────────────────────────────────── */
#define BTC_MODULE_NAME   "btc_qm_engine"
#define BTC_MODULE_VER    "1.0.0-C62"

/* ── Log hash individuel (nonce + résultat + timing) ───────────── */
/* Conforme §A : FORENSIC_LOG_MODULE_METRIC = NOM D'ORIGINE       */
#define BTC_FORENSIC_HASH(nonce, leading, elapsed_ns) do { \
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_sha256_nonce",      (double)(nonce)); \
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_leading_zeros",     (double)(leading)); \
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_sha256_elapsed_ns", (double)(elapsed_ns)); \
} while(0)

/* ── Log échange PT-MC inter-répliques ─────────────────────────── */
#define BTC_FORENSIC_PTMC_SWAP(rep_hot, rep_cold, accept) do { \
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_ptmc_rep_hot",  (double)(rep_hot)); \
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_ptmc_rep_cold", (double)(rep_cold)); \
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_ptmc_accept",   (double)(accept)); \
} while(0)

/* ── Log adaptation NX48 BTC ───────────────────────────────────── */
#define BTC_FORENSIC_NX48_ADAPT(old_delta, new_delta, old_batch, new_batch) do { \
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_nx48_delta_nonce_old", (double)(old_delta)); \
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_nx48_delta_nonce_new", (double)(new_delta)); \
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_nx48_batch_old",       (double)(old_batch)); \
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_nx48_batch_new",       (double)(new_batch)); \
} while(0)

/* ── Log hashrate mesuré ───────────────────────────────────────── */
#define BTC_FORENSIC_HASHRATE(mhs, thread_id) do { \
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_hashrate_mhs",         (double)(mhs)); \
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_hashrate_thread_id",   (double)(thread_id)); \
} while(0)

/* ── Log bloc valide trouvé (near-miss ou réel) ────────────────── */
#define BTC_FORENSIC_BLOCK_FOUND(nonce, leading, is_valid) do { \
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_block_nonce",       (double)(nonce)); \
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_best_leading_zeros",(double)(leading)); \
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_block_valid",       (double)(is_valid)); \
    FORENSIC_LOG_ANOMALY(BTC_MODULE_NAME, "btc_near_miss_or_valid", (double)(leading)); \
} while(0)

/* ── Log snapshot HW dédié BTC ─────────────────────────────────── */
#define BTC_FORENSIC_HW() \
    FORENSIC_LOG_HW_SAMPLE(BTC_MODULE_NAME)

/* ── Log couverture espace nonce ───────────────────────────────── */
#define BTC_FORENSIC_COVERAGE(nonces_total, coverage_pct) do { \
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_nonces_total",       (double)(nonces_total)); \
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_nonce_coverage_pct", (double)(coverage_pct)); \
} while(0)

/* ── Log état réplique PT-MC ───────────────────────────────────── */
#define BTC_FORENSIC_REPLICA_STATE(rep_idx, nonce, temp, leading) do { \
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_replica_idx",     (double)(rep_idx)); \
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_replica_nonce",   (double)(nonce)); \
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_replica_temp",    (double)(temp)); \
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_replica_leading", (double)(leading)); \
} while(0)

/* ── Nano-log timestamp précis (ring buffer sans I/O) ──────────── */
#define BTC_NANO(key, value) \
    FORENSIC_LOG_NANO(BTC_MODULE_NAME, key, value)

/* ── Log thread lifecycle ──────────────────────────────────────── */
#define BTC_THREAD_START(label) \
    FORENSIC_LOG_THREAD_START(BTC_MODULE_NAME, label)
#define BTC_THREAD_END(label) \
    FORENSIC_LOG_THREAD_END(BTC_MODULE_NAME, label)

#endif /* BTC_MINING_FORENSIC_H */
