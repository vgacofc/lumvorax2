/*
 * nx48_adaptive_controller.h — Contrôleur Adaptatif NX48 (C55 — 2026-04-09)
 *
 * Extension du prédicteur de signe NX48 vers un contrôleur adaptatif
 * gérant la TOTALITÉ des paramètres de simulation de façon dynamique.
 *
 * Hérite de toute la lignée NX (NX11→NX48 Ultra) pour LumVorax.
 * Module C natif — aucun Python. Respecte prompt.txt + STANDARD_NAMES.md v3.7.
 *
 * ─────────────────────────────────────────────────────────────────────
 * PARAMÈTRES GÉRÉS DYNAMIQUEMENT (20 features) :
 *
 *   Hardware (lus via /proc/stat + /proc/meminfo — VALEURS RÉELLES) :
 *     [0] cpu_pct_norm      CPU % normalisé [0,1]
 *     [1] ram_pct_norm      RAM % normalisé [0,1]
 *     [2] n_threads_norm    threads actifs / N_MAX_THREADS
 *
 *   Simulation QMC / PTMC :
 *     [3] energy_density    énergie / n_sites (eV)
 *     [4] pairing           appariement SC [0,1]
 *     [5] sign_ratio        ratio signe [0,1]
 *     [6] U_t_ratio_norm    U/t normalisé [0, 20] → /20
 *     [7] temp_red          T réduite [0, 1]
 *     [8] n_steps_log2      log2(n_steps) / 20
 *     [9] n_sweeps_log2     log2(n_sweeps) / 20
 *    [10] n_sites_log2      log2(n_sites) / 20
 *    [11] bench_err_log     -log10(max(bench_abs_err, 1e-9)) / 9
 *    [12] elapsed_log       log(elapsed_s + 1) / 10
 *
 *   Circuit quantique (RCS) :
 *    [13] n_qubits_log2     log2(n_qubits) / 16
 *    [14] circuit_depth_norm circuit_depth / 200
 *    [15] n_circuits_log2   log2(n_circuits) / 20
 *    [16] F_xeb_rm          F_XEB running mean [0, 1]
 *    [17] entropy_density   entropy / n_qubits / 2
 *
 *   Métadonnées simulation :
 *    [18] module_idx_norm   index module [0..15] / 15
 *    [19] grad_energy       |∂E/∂step| (NX47 ARC)
 *
 * SORTIES ADAPTATIVES (nx48_ctrl_params_t) :
 *   - circuit_depth_scale   : multiplicateur depth [0.5, 2.0]
 *   - n_circuits_scale      : multiplicateur n_circuits [0.5, 3.0]
 *   - n_steps_scale         : multiplicateur n_steps [0.5, 2.0]
 *   - n_sweeps_scale        : multiplicateur n_sweeps [0.5, 2.0]
 *   - skip_sign_config      : skipper cette config MC (signe < seuil)
 *   - lr_decay_factor       : facteur lr_decay [0.5, 1.0]
 *   - throttle_cpu          : throttler les threads si CPU > 95%
 *
 * INTÉGRATION FORENSIQUE :
 *   Toutes métriques loggées via FORENSIC_LOG_MODULE_METRIC("nx48_adaptive", ...)
 *   Noms conformes STANDARD_NAMES.md v3.7 — Section M (C55).
 *
 * SOURCE NX48 : src/nx_versions/nx48_unified_ultra/nx48_unified_ultra.py
 *               src/nx_versions/nx47_arc_kernel.py (gradient physique ARC)
 *               Lignée NX11→NX48 complète (src/nx_versions/)
 *
 * C55 — 2026-04-09
 */

#ifndef NX48_ADAPTIVE_CONTROLLER_H
#define NX48_ADAPTIVE_CONTROLLER_H

#define _POSIX_C_SOURCE 200809L

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

/* ── Dimensions étendues ─────────────────────────────────────────────── */

#define NX48C_N_FEATURES      20   /* Toutes les features dynamiques */
#define NX48C_META_NEURONS      3   /* Méta-neurones compétiteurs (NX23) */
#define NX48C_MAX_ITER        120   /* Itérations ISTA batch maximales */
#define NX48C_HISTORY_LEN      64   /* Historique circulaire (NX47 ARC) */

/* ── Indices de features (layout complet) ─────────────────────────────── */

#define NX48F_CPU_PCT_NORM     0   /* CPU % réel /proc/stat */
#define NX48F_RAM_PCT_NORM     1   /* RAM % réel /proc/meminfo */
#define NX48F_N_THREADS_NORM   2   /* threads actifs / max_threads */
#define NX48F_ENERGY_DENSITY   3   /* énergie QMC / n_sites */
#define NX48F_PAIRING          4   /* appariement SC */
#define NX48F_SIGN_RATIO       5   /* ratio signe local */
#define NX48F_U_T_NORM         6   /* U/t / 20.0 */
#define NX48F_TEMP_RED         7   /* T réduite */
#define NX48F_STEPS_LOG2       8   /* log2(n_steps) / 20 */
#define NX48F_SWEEPS_LOG2      9   /* log2(n_sweeps) / 20 */
#define NX48F_SITES_LOG2      10   /* log2(n_sites) / 20 */
#define NX48F_BENCH_ERR_LOG   11   /* -log10(bench_err) / 9 */
#define NX48F_ELAPSED_LOG     12   /* log(elapsed+1) / 10 */
#define NX48F_QUBITS_LOG2     13   /* log2(n_qubits) / 16 */
#define NX48F_DEPTH_NORM      14   /* circuit_depth / 200 */
#define NX48F_CIRCUITS_LOG2   15   /* log2(n_circuits) / 20 */
#define NX48F_F_XEB_RM        16   /* F_XEB running mean */
#define NX48F_ENTROPY_DENS    17   /* entropy / n_qubits / 2 */
#define NX48F_MODULE_IDX      18   /* module index / 15 */
#define NX48F_GRAD_ENERGY     19   /* |∂E/∂step| NX47 ARC */

/* ── Hyperparamètres ISTA ──────────────────────────────────────────────── */

#define NX48C_LR_DEFAULT       0.01
#define NX48C_L1_DEFAULT       3e-4
#define NX48C_L2_DEFAULT       1e-3
#define NX48C_THRESHOLD_SIGN   0.10   /* P(+) < seuil → skip config MC */
#define NX48C_STAGNATION_WIN     5

/* ── Seuils hérités de la lignée NX ──────────────────────────────────── */

#define NX48C_DENSITY_COLLAPSE   0.11   /* NX24 : collapse cognitif */
#define NX48C_NOISE_REJECT       0.99   /* NX22 : rejet VORAX */
#define NX48C_CPU_THROTTLE_PCT   0.95   /* CPU > 95% → throttle threads */

/* ── Paramètres adaptatifs recommandés par le contrôleur ─────────────── */

typedef struct {
    double circuit_depth_scale;  /* multiplicateur depth (0.5→2.0) */
    double n_circuits_scale;     /* multiplicateur n_circuits (0.5→3.0) */
    double n_steps_scale;        /* multiplicateur n_steps (0.5→2.0) */
    double n_sweeps_scale;       /* multiplicateur n_sweeps (0.5→2.0) */
    bool   skip_sign_config;     /* skipper config MC (signe trop faible) */
    double lr_decay_factor;      /* facteur lr_decay [0.5, 1.0] */
    bool   throttle_cpu;         /* réduire threads (CPU > 95%) */
    double overhead_reduction;   /* overhead estimé réduit (NX48 skip) */
} nx48_ctrl_params_t;

/* ── Feature vector étendu ────────────────────────────────────────────── */

typedef struct {
    double   x[NX48C_N_FEATURES];      /* Features normalisées */
    double   grad_x[NX48C_N_FEATURES]; /* Gradient physique (NX47 ARC) */
    double   label;                     /* 1.0=signe positif, 0.0=négatif */
    uint64_t step_id;                   /* Identifiant step MC */
    char     module_name[64];           /* Nom du module source */
} nx48c_sample_t;

/* ── Neurone NX47AtomNeuron étendu ───────────────────────────────────── */

typedef struct {
    double w[NX48C_N_FEATURES];      /* Poids linéaires */
    double alpha[NX48C_N_FEATURES];  /* Poids quadratiques (énergie NX35) */
    double beta[NX48C_N_FEATURES];   /* Poids gradient-signal (NX47 ARC) */
    double b;                          /* Biais scalaire */
    double lr;                         /* Taux d'apprentissage courant */
    double l1, l2;                     /* Régularisation L1/L2 */
    double invariant_density;          /* Densité invariante (NX24) */
} nx48c_neuron_t;

/* ── Modèle contrôleur NX48 complet ──────────────────────────────────── */

typedef struct {
    /* Méta-neurones compétiteurs (NX23) */
    nx48c_neuron_t neurons[NX48C_META_NEURONS];
    int            best_neuron;
    double         best_f1;

    /* Historique circulaire pour gradient ∂x/∂step (NX47 ARC) */
    nx48c_sample_t history[NX48C_HISTORY_LEN];
    int            hist_head;
    int            hist_count;

    /* Statistiques */
    uint64_t n_seen;
    uint64_t n_skipped;
    uint64_t n_throttled;
    double   sum_loss;
    double   overhead_total_reduced;

    /* Stagnation (NX30) */
    double f1_history[NX48C_STAGNATION_WIN];
    int    f1_hist_idx;

    /* Métadonnées */
    char   run_id[64];
    bool   initialized;

    /* Cache hardware (lecture /proc toutes les 500ms) */
    uint64_t hw_last_read_ns;
    double   hw_cpu_pct;         /* CPU % actuel (RÉEL /proc/stat) */
    double   hw_ram_pct;         /* RAM % actuel (RÉEL /proc/meminfo) */
    int      hw_n_threads;       /* Nombre de threads OMP actifs */
} nx48_ctrl_t;

/* ── API publique ─────────────────────────────────────────────────────── */

/*
 * nx48_ctrl_init — Initialise le contrôleur pour un run donné.
 */
void nx48_ctrl_init(nx48_ctrl_t *c, const char *run_id);

/*
 * nx48_ctrl_destroy — Logue les stats finales et libère.
 */
void nx48_ctrl_destroy(nx48_ctrl_t *c);

/*
 * nx48_ctrl_build_sample — Construit un nx48c_sample_t depuis TOUS les
 * paramètres de simulation courants. Lit CPU/RAM via /proc (réels).
 * Calcule gradient physique ∂x/∂step (NX47 ARC).
 */
nx48c_sample_t nx48_ctrl_build_sample(
    nx48_ctrl_t *c,
    const char  *module_name,
    int          module_idx,           /* 0..15 */
    /* QMC */
    double       energy_eV,
    double       pairing,
    double       sign_ratio,
    double       U_t,
    double       temp_K,
    double       temp_Kc,              /* T_critique pour normalisation */
    uint64_t     n_steps,
    uint64_t     n_sweeps,
    int          n_sites,
    double       bench_abs_err,
    double       elapsed_s,
    /* RCS */
    int          n_qubits,
    int          circuit_depth,
    uint64_t     n_circuits,
    double       F_xeb_rm,
    double       entropy,
    /* Signe */
    double       sign_true,
    uint64_t     step_id
);

/*
 * nx48_ctrl_predict — Prédit P(signe_positif) et génère les paramètres
 * adaptatifs recommandés pour ce step de simulation.
 */
nx48_ctrl_params_t nx48_ctrl_predict(nx48_ctrl_t *c, const nx48c_sample_t *s,
                                      double overhead_original);

/*
 * nx48_ctrl_update — Mise à jour en ligne (ISTA, 1 sample).
 */
double nx48_ctrl_update(nx48_ctrl_t *c, const nx48c_sample_t *s);

/*
 * nx48_ctrl_fit — Entraînement batch sur l'historique complet.
 */
void nx48_ctrl_fit(nx48_ctrl_t *c);

/*
 * nx48_ctrl_log_stats — Logue TOUTES les métriques adaptatifs.
 */
void nx48_ctrl_log_stats(const nx48_ctrl_t *c);

/*
 * nx48_ctrl_read_hw — Lit CPU/RAM depuis /proc (RÉEL, pas simulé).
 * Appelée automatiquement par build_sample toutes les 500ms.
 */
void nx48_ctrl_read_hw(nx48_ctrl_t *c);

/* ── Fonctions mathématiques internes (exposées pour tests) ───────────── */

double nx48c_sigmoid(double z);
double nx48c_prox_l1(double w, double lr_l1);
double nx48c_forward(const nx48c_neuron_t *n, const nx48c_sample_t *s);

#endif /* NX48_ADAPTIVE_CONTROLLER_H */
