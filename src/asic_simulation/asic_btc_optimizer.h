/* ============================================================================
 * ASIC BTC Optimizer — Cycle C115
 * Simulateur d'optimisation de minage Bitcoin classique (stratégie nonce,
 * batch size, thermique, pipeline SHA-256).
 *
 * OBJECTIF : optimiser (pas seulement benchmarker) les paramètres de minage
 * pour maximiser le hashrate réel et minimiser les W/TH.
 *
 * Stratégies de nonce :
 *   NONCE_SEQUENTIAL  : scan 0→0xFFFFFFFF (standard, prévisible)
 *   NONCE_RANDOM      : tirage aléatoire uniforme (meilleure couverture nonce64)
 *   NONCE_DELTA_NX48  : scan biaisé par delta NX48 (inspiré des logs Ubuntu C112)
 *   NONCE_QUANTUM_BIAS: nonces biaisés par amplitudes quantiques pré-mesure (C115)
 * ============================================================================ */
#ifndef ASIC_BTC_OPTIMIZER_H
#define ASIC_BTC_OPTIMIZER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Paramètres ASIC SHA-256 pipeline
 * ============================================================================ */
typedef enum {
    NONCE_SEQUENTIAL   = 0,   /* Scan linéaire 0→2^32 */
    NONCE_RANDOM       = 1,   /* Tirage uniforme PRNG */
    NONCE_DELTA_NX48   = 2,   /* Delta adaptatif (NX48-C62 inspiré) */
    NONCE_QUANTUM_BIAS = 3    /* Biaisé par amplitudes pré-mesure quantique */
} asic_nonce_strategy_t;

typedef struct {
    /* Paramètres hardware */
    int      pipeline_depth;        /* Stages SHA-256 (64 par défaut) */
    int      parallel_engines;      /* Moteurs parallèles (Antminer S19 XP: 280) */
    double   clock_GHz;             /* Fréquence d'horloge */
    double   watts_peak;            /* Consommation max (W) */
    double   thermal_throttle_s;    /* Début throttling thermique (secondes) */
    double   thermal_factor;        /* Facteur de ralentissement sous throttle [0-1] */

    /* Paramètres stratégie nonce */
    asic_nonce_strategy_t strategy;
    double   delta_nx48_initial;    /* Delta initial pour NONCE_DELTA_NX48 */
    double   delta_nx48_cap;        /* Plafond delta (500.0 par défaut, C112 observé) */
    double   quantum_alpha;         /* Amplitude α (complexité quantique) [0-1] */
    double   quantum_beta;          /* Amplitude β = sqrt(1 - α²) */

    /* Paramètres simulation */
    uint32_t batch_size;            /* Nonces par batch (256K par défaut) */
    uint32_t target_bits;           /* Difficulté cible (bits de zéros requis) */
    double   run_duration_s;        /* Durée de simulation (secondes) */
    bool     enable_thermal_model;  /* Activer modèle thermique progressif */
} asic_btc_optimizer_cfg_t;

/* ============================================================================
 * Résultats d'optimisation
 * ============================================================================ */
typedef struct {
    /* Métriques hashrate */
    double   hashrate_peak_MH_s;    /* Hashrate crête (MH/s) */
    double   hashrate_avg_MH_s;     /* Hashrate moyen sur toute la durée */
    double   hashrate_final_MH_s;   /* Hashrate en fin de run (effet thermique) */
    double   hashrate_decline_pct;  /* Déclin relatif crête→final (%) */

    /* Métriques efficacité */
    double   W_per_GH;              /* W/GH — indicateur PUE minage */
    double   J_per_hash;            /* Joules par hash */
    double   total_hashes;          /* Total hashes calculés */
    double   run_time_s;            /* Durée réelle simulée */

    /* Métriques qualité nonce */
    uint32_t best_leading_bits;     /* Record leading bits 0 obtenus */
    uint32_t best_nonce;            /* Nonce correspondant au record */
    double   near_miss_rate_per_Mh; /* Near-miss ≥20 bits par MH */
    uint64_t batches_processed;     /* Nombre de batches traités */

    /* Métriques NX48 */
    double   nx48_delta_final;      /* Delta NX48 final */
    uint32_t nx48_resets;           /* Nombre de resets C62 (stall détecté) */

    /* Score global optimisation [0-100] */
    double   optimization_score;    /* Score composite (hashrate × efficacité × qualité) */
} asic_btc_result_t;

/* ============================================================================
 * API principale
 * ============================================================================ */

/* Initialise une configuration par défaut (profil Intel UHD 620 observé C112) */
void asic_btc_optimizer_default_cfg(asic_btc_optimizer_cfg_t *cfg);

/* Initialise un profil Antminer S19 XP (ASIC industriel référence) */
void asic_btc_optimizer_antminer_cfg(asic_btc_optimizer_cfg_t *cfg);

/* Lance la simulation complète et remplit result.
 * Retourne 0 succès, -1 si paramètres invalides. */
int asic_btc_optimizer_run(const asic_btc_optimizer_cfg_t *cfg,
                            asic_btc_result_t *result);

/* Compare deux stratégies et retourne l'indice de la meilleure (0 ou 1).
 * Critère : optimization_score le plus élevé. */
int asic_btc_optimizer_compare(const asic_btc_optimizer_cfg_t *cfg_a,
                                const asic_btc_optimizer_cfg_t *cfg_b,
                                asic_btc_result_t *res_a,
                                asic_btc_result_t *res_b);

/* Recherche automatique de batch_size optimal pour un profil hardware donné.
 * Teste batch_sizes[] (tableau de tailles), renvoie le meilleur.
 * Remplit best_result et retourne l'indice optimal dans batch_sizes. */
int asic_btc_optimizer_tune_batch(asic_btc_optimizer_cfg_t *cfg,
                                   const uint32_t *batch_sizes,
                                   int n_sizes,
                                   asic_btc_result_t *best_result);

/* ============================================================================
 * C125 — OPTIMIZE-RUNTIME : sweep multi-dimensionnel complet
 *
 * Balaye 4 axes :
 *   - batch_size     : 4 valeurs (ex: 256K, 512K, 1M, 2M)
 *   - strategy       : 4 valeurs (SEQ, RANDOM, DELTA_NX48, QUANTUM)
 *   - delta_nx48_init: 4 valeurs (ex: 1.0, 6.0, 32.0, 128.0)
 *   - thermal_throttle_s : 3 valeurs (ex: 60, 300, 900)
 *
 * Total : 4×4×4×3 = 192 simulations (chacune ~5s = 16 min total au pire,
 *                                     mais avec run_duration_s=0.5s → 96s total)
 *
 * Retourne le profil OPTIMAL (meilleur optimization_score), prêt à être
 * injecté dans les atomics nx48_ctrl_* pour le run de mining réel.
 * ============================================================================ */
typedef struct {
    /* Best profile found (à injecter dans atomics) */
    uint32_t              best_batch_size;
    asic_nonce_strategy_t best_strategy;
    double                best_delta_nx48_init;
    double                best_thermal_throttle_s;
    /* Métriques du best */
    asic_btc_result_t     best_result;
    /* Statistiques sweep */
    int                   total_combinations;
    int                   successful_combinations;
    double                worst_score;
    double                best_score;
    double                mean_score;
    double                stddev_score;
    /* Top-5 indices (ordre décroissant) */
    int                   top5_indices[5];
    double                top5_scores[5];
    /* Durée totale du sweep en secondes */
    double                sweep_duration_s;
} asic_btc_tune_full_result_t;

/* Lance le sweep complet. n_per_dim_* = 0 utilise les valeurs par défaut.
 * Retourne 0 succès, -1 si paramètres invalides. */
int asic_btc_optimizer_tune_full(const asic_btc_optimizer_cfg_t *base_cfg,
                                  const uint32_t *batch_sizes,    int n_batch,
                                  const int      *strategies,     int n_strat,
                                  const double   *delta_inits,    int n_delta,
                                  const double   *thermal_throttles, int n_thermal,
                                  asic_btc_tune_full_result_t *out);

/* Affiche un rapport détaillé du sweep. */
void asic_btc_optimizer_print_tune_full_report(const asic_btc_tune_full_result_t *r);

/* Rapport texte résumant les résultats (imprimé sur stdout). */
void asic_btc_optimizer_print_report(const asic_btc_optimizer_cfg_t *cfg,
                                      const asic_btc_result_t *result);

#ifdef __cplusplus
}
#endif

#endif /* ASIC_BTC_OPTIMIZER_H */
