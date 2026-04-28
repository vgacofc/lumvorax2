#ifndef NX48_BTC_CONTROLLER_H
#define NX48_BTC_CONTROLLER_H

/*
 * LumVorax — Module 17 — Bitcoin Quantum Mining Engine
 * nx48_btc_controller.h — Contrôleur NX48 autonome — Architecture C61
 *
 * STANDARD_NAMES.md v4.4 §M-BTC17-C61 — Cycle C61 — 2026-04-18
 *
 * REFONTE COMPLÈTE C61 — NX48 AUTONOME 100% :
 *  [C61-SUBNEURONS]    8 sous-neurones dynamiques par neurone principal (16 total)
 *                      Chaque sous-neurone spécialisé : GPU, threads, PT-MC, batch,
 *                      delta, AVX, exploration, QDAYPRIZE feedback
 *  [C61-HW-DETECT]     Détection autonome GPU OpenCL, AVX-512 SHA-NI, threads CPU
 *                      NX48 lit /proc/cpuinfo, clinfo, /dev/dri en temps réel
 *  [C61-CTRL-ALL]      NX48 contrôle 100% des paramètres via atomiques partagées :
 *                      n_threads, T_hot, T_cold, batch_size, exploration, GPU
 *  [C61-EXPLOR-VEL]    exploration_bias avec vélocité + momentum (Adam-like)
 *                      Débloque le plateau 48-50% — plage réelle [0.05, 0.95]
 *                      Adaptation vitesse selon stagnation longue durée
 *  [C61-LUM-NATIVE]    Persistance NX48 en format LUM binaire natif (≠ CSV seul)
 *                      lum_nx48_entry_t 64 bytes — 5× plus rapide que CSV
 *  [C61-DELTA-UNLOCK]  delta_nonce_scale déplafonné : max 50→500
 *                      Exploration de tout l'espace nonce 2^32 si nécessaire
 *  [C61-QDAYPRIZE]     Feedback QDAYPRIZE → NX48 : success_rate→exploration_bias
 *                      Si QDAYPRIZE success_rate > 80% → NX48 monte exploitation
 *
 * Conformité : STANDARD_NAMES.md v4.4 — prompt.txt v2.1
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>

/* ════════════════════════════════════════════════════════════════════
 * CONSTANTES ARCHITECTURE
 * ════════════════════════════════════════════════════════════════════ */
#define NX48_BTC_N_FEATURES    8    /* Features neurone principal */
#define NX48_N_SUBNEURONS      8    /* Sous-neurones par neurone principal */
#define NX48_SN_FEATURES       4    /* Features par sous-neurone (compact) */
#define NX48_SN_VERSION        0x0004  /* Version format LUM C61 */

/* ── Indices des features principales (conformes §M-BTC17) ──────── */
#define NX48_BTC_F_BEST_LEADING    0   /* btc_best_leading_zeros / 256 */
#define NX48_BTC_F_HASHRATE_NORM   1   /* hashrate_mhs / hashrate_max */
#define NX48_BTC_F_SWAP_RATE       2   /* ptmc_swap_rate */
#define NX48_BTC_F_TIME_STALL      3   /* -log(time_since_improvement_s)/10 */
#define NX48_BTC_F_COVERAGE        4   /* nonce_coverage_pct / 100 */
#define NX48_BTC_F_DELTA_NORM      5   /* delta_nonce / NONCE_MAX */
#define NX48_BTC_F_THREAD_EFF      6   /* hashes_done / hashes_expected */
#define NX48_BTC_F_TEMP_RATIO      7   /* T_hot / T_cold */

/* ── Spécialités des 8 sous-neurones ───────────────────────────── */
#define NX48_SN_EXPLORATION   0   /* Ratio exploration/exploitation */
#define NX48_SN_THREADS       1   /* Contrôle nombre de threads */
#define NX48_SN_GPU           2   /* Activation GPU OpenCL */
#define NX48_SN_TEMP_HOT      3   /* Température réplique chaude T_hot */
#define NX48_SN_TEMP_COLD     4   /* Température réplique froide T_cold */
#define NX48_SN_BATCH         5   /* Taille batch SHA-256 */
#define NX48_SN_AVX           6   /* Sélection chemin AVX-512/AVX2/scalaire */
#define NX48_SN_QDAYPRIZE     7   /* Intégration feedback QDAYPRIZE */

/* ── Niveaux hardware détectés ──────────────────────────────────── */
#define NX48_HW_SCALAR      0   /* SHA-256 scalaire (baseline) */
#define NX48_HW_AVX2        1   /* SHA-256 AVX2 8-way */
#define NX48_HW_AVX512      2   /* SHA-256 AVX-512 + SHA-NI */
#define NX48_HW_OPENCL      3   /* SHA-256 GPU OpenCL */

/* ════════════════════════════════════════════════════════════════════
 * STRUCTURE SOUS-NEURONE (compact — 4 features, 1 sortie)
 * ════════════════════════════════════════════════════════════════════ */
typedef struct {
    double  weights[NX48_SN_FEATURES];  /* Poids du sous-neurone */
    double  bias;                        /* Biais */
    double  output;                      /* Sortie sigmoid [0,1] */
    double  loss;                        /* Loss locale du sous-neurone */
    double  grad_norm;                   /* Norme gradient */
    double  learning_rate;               /* η adaptatif par sous-neurone */
    double  momentum[NX48_SN_FEATURES]; /* Adam m1 (momentum 1er ordre) */
    double  velocity[NX48_SN_FEATURES]; /* Adam m2 (velocity 2ème ordre) */
    int     update_count;                /* Nombre de mises à jour */
    int     specialty;                   /* NX48_SN_* */
    char    name[16];                    /* Nom lisible */
} nx48_subneuron_t;

/* ════════════════════════════════════════════════════════════════════
 * ÉTAT HARDWARE DÉTECTÉ PAR NX48 (mis à jour chaque NX48_HW_DETECT_S)
 * ════════════════════════════════════════════════════════════════════ */
typedef struct {
    /* CPU */
    int     n_cores_physical;    /* Cœurs physiques /proc/cpuinfo */
    int     n_threads_max;       /* Threads HyperThreading max */
    int     avx_level;           /* NX48_HW_SCALAR/AVX2/AVX512 */
    int     sha_ni;              /* 1 si SHA-NI disponible (EPYC 9B14) */
    double  cpu_load_pct;        /* Charge CPU process (%) */
    long    ram_available_mb;    /* RAM disponible en MB */

    /* GPU */
    int     gpu_opencl_present;  /* 1 si GPU OpenCL détecté (clinfo) */
    int     gpu_opencl_active;   /* 1 si NX48 a activé le GPU */
    char    gpu_name[64];        /* ex: "Intel(R) UHD Graphics 620" */
    double  gpu_hashrate_est;    /* Estimé en MH/s (0 si non mesuré) */
    int     dri_present;         /* 1 si /dev/dri/renderD128 existe */

    /* QDAYPRIZE feedback */
    double  qdayprize_success_rate;  /* Dernière success_rate QDAYPRIZE */
    int     qdayprize_bits;          /* Derniers bits simulés */
    double  qdayprize_runtime_s;     /* Durée dernière simulation */

    /* Timestamp dernière mise à jour HW */
    uint64_t last_hw_detect_ns;
} nx48_hw_state_t;

/* ════════════════════════════════════════════════════════════════════
 * FORMAT BINAIRE LUM NATIF — nx48_lum_entry_t (64 bytes)
 * Remplace CSV pour la persistance NX48 (×5 plus rapide)
 * Magic : 0x4E583438 = "NX48"
 * ════════════════════════════════════════════════════════════════════ */
#define NX48_LUM_MAGIC     0x4E583438U  /* "NX48" */
#define NX48_LUM_VERSION   0x0004U      /* C61 */

typedef struct __attribute__((packed)) {
    uint32_t  magic;                /* 0x4E583438 */
    uint16_t  version;              /* NX48_LUM_VERSION */
    uint16_t  reserved;
    uint32_t  update_count;
    uint32_t  best_leading_zeros;
    uint64_t  best_nonce;
    float     weights[8];           /* Poids producteur (float pour compacité) */
    float     executor_weights[8];  /* Poids applicateur */
    float     exploration_bias;
    float     exploration_vel;      /* Vélocité exploration (C61) */
    float     delta_nonce_scale;
    float     batch_size_scale;
    float     loss_curr;
    float     dual_blend;
    uint32_t  crc32;                /* Intégrité */
} nx48_lum_entry_t;  /* = 64 bytes */

/* ════════════════════════════════════════════════════════════════════
 * ATOMIQUES PARTAGÉES MOTEUR ↔ NX48 (contrôle total)
 * Déclarées extern ici, définies dans btc_mining_engine.c
 * ════════════════════════════════════════════════════════════════════ */
extern _Atomic int    nx48_ctrl_n_threads;       /* NX48→moteur: threads actifs */
extern _Atomic int    nx48_ctrl_T_hot_idx;       /* NX48→moteur: index T_hot [0..7] */
extern _Atomic int    nx48_ctrl_T_cold_idx;      /* NX48→moteur: index T_cold [0..7] */
extern _Atomic int    nx48_ctrl_gpu_active;      /* NX48→moteur: GPU OpenCL actif */
extern _Atomic int    nx48_ctrl_avx_level;       /* NX48→moteur: niveau AVX */
extern _Atomic int    nx48_ctrl_batch_size;      /* NX48→moteur: batch override */
/* C116-P4 : QDPR adaptatif — NX48 ajuste dynamiquement le seuil near-miss GPU */
extern _Atomic int    nx48_ctrl_near_miss_bits;  /* NX48→GPU: seuil near-miss [20..38] */

/* ════════════════════════════════════════════════════════════════════
 * ÉTAT COMPLET NX48_BTC — C61
 * ════════════════════════════════════════════════════════════════════ */
typedef struct {
    /* ── Neurone producteur (apprend par gradient ISTA) ─────────── */
    double weights[NX48_BTC_N_FEATURES];
    double bias;

    /* ── Neurone applicateur (distillé du producteur) ────────────── */
    double executor_weights[NX48_BTC_N_FEATURES];
    double executor_bias;
    double dual_blend;

    /* ── Sous-neurones dynamiques (C61) ──────────────────────────── */
    nx48_subneuron_t subneurons_prod[NX48_N_SUBNEURONS];   /* sous-producteurs */
    nx48_subneuron_t subneurons_exec[NX48_N_SUBNEURONS];   /* sous-applicateurs */

    /* ── Hyper-paramètres appris ─────────────────────────────────── */
    double delta_nonce_scale;   /* Rayon voisinage nonce [0.1, 500.0] — C61 déplafonné */
    double n_replicas_scale;    /* Échelle répliques PT [1.0, 2.0] */
    double swap_temp_scale;     /* Température échange PT [0.5, 3.0] */
    double batch_size_scale;    /* Taille batch SIMD [0.5, 8.0] */
    double exploration_bias;    /* Ratio exploration [0.05, 0.95] */
    double exploration_vel;     /* Vélocité momentum exploration (C61) */
    double exploration_acc;     /* Accélération (2ème dérivée) */

    /* ── Contrôle hardware direct (C61) ─────────────────────────── */
    int    n_threads_target;    /* Threads cibles (NX48 décide) */
    int    T_hot_idx;           /* Index température chaude [0..7] */
    int    T_cold_idx;          /* Index température froide [0..7] */
    double T_hot_actual;        /* T_hot courante (modifiable) */
    double T_cold_actual;       /* T_cold courante */

    /* ── État hardware détecté ───────────────────────────────────── */
    nx48_hw_state_t hw;

    /* ── Métriques d'apprentissage ───────────────────────────────── */
    double loss_prev;
    double loss_curr;
    double grad_norm;
    int    update_count;
    int    stall_count;
    int    stall_long_count;    /* Compteur stagnation longue (>100 updates) */

    /* ── Meilleur état observé ───────────────────────────────────── */
    uint32_t best_nonce;
    int      best_leading_zeros;
    uint64_t best_ts_ns;

    /* ── Adam optimizer état (pour les 8 poids principaux) ──────── */
    double adam_m1[NX48_BTC_N_FEATURES];  /* 1er moment (momentum) */
    double adam_m2[NX48_BTC_N_FEATURES];  /* 2ème moment (velocity) */
    int    adam_t;                          /* Pas de temps Adam */

    /* ── Persistance LUM natif (C61) ─────────────────────────────── */
    char   lum_path[256];   /* Chemin btc_nx48_last.lum (format binaire) */
    char   csv_path[256];   /* Chemin CSV (compatibilité) */

    char   run_id[64];
} nx48_btc_state_t;

/* ════════════════════════════════════════════════════════════════════
 * CONFIGURATION NX48_BTC — C61
 * ════════════════════════════════════════════════════════════════════ */
typedef struct {
    double learning_rate;       /* η ISTA [0.001, 0.1] */
    double lambda_l1;           /* Régularisation L1 ISTA */
    double label_target;        /* Label cible (1.0 = 256 leading zeros) */
    int    update_interval;     /* Hashes entre mises à jour NX48 */
    int    n_replicas_base;     /* Nombre répliques PT de base */
    double T_cold;              /* Température réplique froide initiale */
    double T_hot;               /* Température réplique chaude initiale */
    int    n_threads_initial;   /* Threads initiaux */
    char   csv_path[256];       /* Chemin CSV (compatibilité C42) */
    char   lum_path[256];       /* Chemin LUM binaire natif (C61) */
    int    hw_detect_interval_s; /* Secondes entre détections HW (défaut: 30) */
    /* C99 P0.1 — Couplage neural Izhikevich+STDP (off par défaut) */
    int    use_neural_coupler;   /* 1 = active le coupleur, 0 = NX48 seul */
    double coupler_influence;    /* Facteur ±[0..1] (défaut 0.15 = ±15%) */
} nx48_btc_config_t;

/* ════════════════════════════════════════════════════════════════════
 * API NX48_BTC — C61
 * ════════════════════════════════════════════════════════════════════ */

/* Initialise NX48_BTC depuis LUM natif ou CSV (ou valeurs par défaut) */
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

/* Prédiction NX48_BTC : prob ∈ [0,1] */
double nx48_btc_predict(nx48_btc_state_t* s, const double features[NX48_BTC_N_FEATURES]);

/* Mise à jour gradient ISTA + sous-neurones + contrôle hardware
 * C107 : ajout best_nonce pour persistance bound au nonce qui a produit le record */
void nx48_btc_update(
    nx48_btc_state_t*        s,
    const nx48_btc_config_t* cfg,
    const double             features[NX48_BTC_N_FEATURES],
    double                   prob,
    int                      best_leading_zeros,
    uint32_t                 best_nonce,
    double                   hashrate_mhs
);

/* C61 : Détection hardware autonome (GPU, AVX, threads, RAM) */
void nx48_btc_hw_detect(nx48_btc_state_t* s);

/* C61 : Appliquer le contrôle total via atomiques partagées */
void nx48_btc_control_all(nx48_btc_state_t* s);

/* C61 : Mise à jour d'un sous-neurone (Adam + ISTA) */
void nx48_subneuron_update(
    nx48_subneuron_t* sn,
    const double      features[NX48_SN_FEATURES],
    double            label,
    double            parent_err
);

/* C61 : Calcul features d'un sous-neurone depuis l'état NX48 */
void nx48_subneuron_compute_features(
    double                    feat[NX48_SN_FEATURES],
    const nx48_btc_state_t*   s,
    int                       specialty
);

/* C61 : Sauvegarde LUM binaire natif (+ CSV pour compatibilité) */
int nx48_btc_save_lum(const nx48_btc_state_t* s, const char* lum_path);
int nx48_btc_load_lum(nx48_btc_state_t* s, const char* lum_path);

/* Compatibilité C42 : CSV */
int nx48_btc_save_csv(const nx48_btc_state_t* s, const char* csv_path);
int nx48_btc_load_csv(nx48_btc_state_t* s, const char* csv_path);

/* Libère la mémoire */
void nx48_btc_destroy(nx48_btc_state_t* s);

/* Clamp des scales dans les bornes C61 */
void nx48_btc_clamp_scales(nx48_btc_state_t* s);

#endif /* NX48_BTC_CONTROLLER_H */
