#ifndef NX48_BTC_CONTROLLER_H
#define NX48_BTC_CONTROLLER_H

/*
 * LumVorax — Module 17 — Bitcoin Quantum Mining Engine
 * nx48_btc_controller.h — Contrôleur NX48 adapté espace nonce Bitcoin
 *
 * Conformité : STANDARD_NAMES.md v4.1 §M-BTC17
 * Cycle C62 — 2026-04-11
 *
 * Principe :
 *   Le neurone NX48_BTC apprend la distribution des leading_zeros
 *   dans le voisinage nonce courant et adapte les hyper-paramètres
 *   du moteur PT-MC en temps réel (gradient ISTA).
 *
 *   C'est le même principe que NX48 Hubbard (nx48_adaptive_controller.c)
 *   mais adapté aux features Bitcoin au lieu des features QMC.
 */

#include <stdint.h>
#include <stdbool.h>

/* ── Nombre de features NX48_BTC ────────────────────────────────── */
#define NX48_BTC_N_FEATURES    8

/* ── Indices des features (conformes §M-BTC17) ──────────────────── */
#define NX48_BTC_F_BEST_LEADING    0   /* btc_best_leading_zeros normalisé [0,1] */
#define NX48_BTC_F_HASHRATE_NORM   1   /* btc_hashrate_mhs / hashrate_max [0,1] */
#define NX48_BTC_F_SWAP_RATE       2   /* btc_ptmc_swap_rate [0,1] */
#define NX48_BTC_F_TIME_STALL      3   /* -log(btc_time_since_improvement_s)/10 */
#define NX48_BTC_F_COVERAGE        4   /* btc_nonce_coverage_pct [0,1] */
#define NX48_BTC_F_DELTA_NORM      5   /* delta_nonce / NONCE_MAX [0,1] */
#define NX48_BTC_F_THREAD_EFF      6   /* hashes_done / hashes_expected [0,1] */
#define NX48_BTC_F_TEMP_RATIO      7   /* T_hot / T_cold des répliques PT */

/* ── État persistant NX48_BTC (sauvegardé dans btc_nx48_last.csv) ─ */
typedef struct {
    /* Poids du neurone (ISTA gradient) */
    double weights[NX48_BTC_N_FEATURES];  /* w[0..7] */
    double bias;                           /* biais du neurone */

    /* Hyper-paramètres appris */
    double delta_nonce_scale;   /* Rayon voisinage nonce [0.1, 10.0] */
    double n_replicas_scale;    /* Échelle nombre répliques PT [1.0, 2.0] */
    double swap_temp_scale;     /* Température échange PT [0.5, 3.0] */
    double batch_size_scale;    /* Taille batch SIMD [0.5, 4.0] */
    double exploration_bias;    /* Biais exploration vs exploitation [0.0, 1.0] */

    /* Métriques d'apprentissage */
    double loss_prev;           /* Loss précédente (BCE) */
    double loss_curr;           /* Loss courante */
    double grad_norm;           /* Norme du gradient ISTA */
    int    update_count;        /* Nombre de mises à jour NX48 */

    /* Meilleur état observé */
    uint32_t best_nonce;        /* Meilleur nonce (leading_zeros max) */
    int      best_leading_zeros;/* Record leading_zeros */
    uint64_t best_ts_ns;        /* Timestamp du meilleur record */

    /* run_id courant */
    char run_id[64];
} nx48_btc_state_t;

/* ── Configuration NX48_BTC ─────────────────────────────────────── */
typedef struct {
    double learning_rate;       /* η ISTA [0.001, 0.1] */
    double lambda_l1;           /* Régularisation L1 ISTA */
    double label_target;        /* Label cible (1.0 = bloc trouvé) */
    int    update_interval;     /* Mise à jour toutes les N hashes */
    int    n_replicas_base;     /* Nombre répliques PT de base */
    double T_cold;              /* Température réplique froide */
    double T_hot;               /* Température réplique chaude */
    char   csv_path[256];       /* Chemin btc_nx48_last.csv */
} nx48_btc_config_t;

/* ── API NX48_BTC ───────────────────────────────────────────────── */

/* Initialise l'état NX48_BTC depuis CSV (ou valeurs par défaut) */
nx48_btc_state_t* nx48_btc_init(const nx48_btc_config_t* cfg, const char* run_id);

/* Calcule les features depuis les métriques courantes */
void nx48_btc_compute_features(
    double features[NX48_BTC_N_FEATURES],
    int    best_leading_zeros,
    double hashrate_mhs,
    double hashrate_max,
    double swap_rate,
    double time_since_improvement_s,
    double nonce_coverage_pct,
    double delta_nonce,
    double hashes_done,
    double hashes_expected,
    double T_hot,
    double T_cold
);

/* Prédiction NX48_BTC : prob ∈ [0,1] (probabilité de succès) */
double nx48_btc_predict(nx48_btc_state_t* s, const double features[NX48_BTC_N_FEATURES]);

/* Mise à jour gradient ISTA (appel toutes les update_interval hashes) */
void nx48_btc_update(
    nx48_btc_state_t*     s,
    const nx48_btc_config_t* cfg,
    const double features[NX48_BTC_N_FEATURES],
    double prob,
    int best_leading_zeros,
    double hashrate_mhs
);

/* Sauvegarde état dans btc_nx48_last.csv */
int nx48_btc_save_csv(const nx48_btc_state_t* s, const char* csv_path);

/* Charge état depuis btc_nx48_last.csv */
int nx48_btc_load_csv(nx48_btc_state_t* s, const char* csv_path);

/* Libère la mémoire NX48_BTC */
void nx48_btc_destroy(nx48_btc_state_t* s);

/* Clamp des scales dans les bornes définies */
void nx48_btc_clamp_scales(nx48_btc_state_t* s);

#endif /* NX48_BTC_CONTROLLER_H */
