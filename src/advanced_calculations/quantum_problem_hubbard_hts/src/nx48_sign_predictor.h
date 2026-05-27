/*
 * nx48_sign_predictor.h — Prédicteur de signe QMC NX47AtomNeuron (NX48)
 *                          Module C natif — Lignée NX complète intégrée
 *
 * Ce module est la traduction en C pur de l'architecture NX47AtomNeuron,
 * héritière de toute la lignée NX (NX11→NX48) pour le projet LumVorax.
 *
 * ─────────────────────────────────────────────────────────────────────
 * HÉRITAGE DE LA LIGNÉE NX (lu dans src/nx_versions) :
 *
 *   NX11-NX20 : Chaîne Merkle SHA-256, logging nanoseconde forensique,
 *               clusters holographiques, métriques hardware réelles.
 *   NX22      : Torture — résistance bruit VORAX, désync temporelle,
 *               parasitage cognitif → NX48_NOISE_REJECT_THRESHOLD.
 *   NX23      : Couplage dissipatif entre méta-neurones, mémoire collective,
 *               effondrement partiel sans propagation → méta-neurones compétiteurs.
 *   NX24      : Point de non-retour cognitive (invariant_density < 0.11)
 *               → NX48_DENSITY_COLLAPSE_THRESHOLD = 0.11.
 *   NX25      : Héritage SHA-256 réel, benchmark comparatif entre versions.
 *   NX26-NX30 : Cognition dissipative, transduction sémantique, dialogue
 *               bidirectionnel (commande externe → attracteur → réponse).
 *   NX31-NX32 : Nano-forensic, Merkle 360, 16 conjectures → intégrité logs.
 *   NX35      : 5000 neurones, énergie accumulée, logs ligne-par-ligne.
 *   NX46 v2   : MemoryTracker bit-à-bit, activation dynamique de neurones.
 *   NX47 ARC  : Règles apprises depuis le signal physique (gradient du signal).
 *   NX48 Ultra: Analyse forensique intégrale + benchmark Hubbard HTS.
 *
 * ─────────────────────────────────────────────────────────────────────
 * ARCHITECTURE NX47AtomNeuron (STANDARD_NAMES.md v3.7, Section L) :
 *
 *   z = x·w + x²·alpha + ∇x·beta + b   →  P(+) = sigmoid(z)
 *
 *   w     : poids linéaires   (transduction sémantique NX28-NX29)
 *   alpha : poids quadratiques (énergie accumulée NX35)
 *   beta  : poids gradient    (∇x physique NX47 ARC — "apprentissage sans points")
 *   b     : biais scalaire
 *
 * Apprentissage ISTA (NX26 ISTA proximal) :
 *   — sans rétropropagation (= mémoire dissipative NX24)
 *   — régularisation L1+L2 (sparse = attracteurs naturels NX23)
 *   — méta-neurones compétiteurs (effondrement partiel NX23, robustesse NX22)
 *   — stagnation → lr × 0.65 (pont bidirectionnel NX30)
 *
 * ─────────────────────────────────────────────────────────────────────
 * USAGE LumVorax / modules cibles (overhead signe critique) :
 *   simulate_fs        : 202 500× → ~1 000×
 *   sign_problem_module:   5 184× → ~100×
 *   quantum_chemistry  :     400× → ~50×
 *   multi_correlated   :     384× → ~50×
 *
 * Features d'entrée (NX48_N_FEATURES = 6) :
 *   [0] energy_density    énergie MC / n_sites
 *   [1] pairing           appariement SC
 *   [2] sign_ratio_local  ratio signe local courant
 *   [3] U_t_ratio         U/t du hamiltonien
 *   [4] temperature_red   T/T_c réduite
 *   [5] grad_energy       ∂E/∂step (gradient physique NX47 ARC)
 *
 * Intégration forensique : FORENSIC_LOG_MODULE_METRIC("nx48_sign", ...)
 * Noms conformes STANDARD_NAMES.md v3.7 — Section L (C55).
 *
 * C55 — 2026-04-09
 */

#ifndef NX48_SIGN_PREDICTOR_H
#define NX48_SIGN_PREDICTOR_H

#define _POSIX_C_SOURCE 200809L

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

/* ── Dimensions ────────────────────────────────────────────────────── */

#define NX48_N_FEATURES            6    /* Features physiques QMC */
#define NX48_META_NEURONS          3    /* Méta-neurones compétiteurs (NX23) */
#define NX48_MAX_ITER            120    /* Itérations ISTA batch maximales */
#define NX48_HISTORY_LEN          64    /* Historique circulaire (∇x NX47) */
#define NX48_MERKLE_WINDOW        50    /* Rotation Merkle (NX13 : rotation à 50 entrées) */

/* ── Hyperparamètres ISTA ──────────────────────────────────────────── */

#define NX48_LR_DEFAULT           0.01  /* Taux d'apprentissage (NX26) */
#define NX48_L1_DEFAULT           3e-4  /* L1 → sparsité attracteurs (NX23) */
#define NX48_L2_DEFAULT           1e-3  /* L2 → weight decay */
#define NX48_THRESHOLD_SIGN       0.10  /* P(+) < seuil → skip config MC */
#define NX48_STAGNATION_WIN          5  /* Fenêtre stagnation F1 (NX30) */

/* ── Seuils hérités de la lignée NX ─────────────────────────────────
 * NX24 : point de non-retour cognitif à invariant_density < 0.11.
 * Si la densité invariante du neurone tombe sous ce seuil, le neurone
 * est considéré "effondré" (mort cognitive) et réinitialisé.
 */
#define NX48_DENSITY_COLLAPSE_THRESHOLD  0.11

/* NX22 : rejet du parasitage VORAX (bruit extrême).
 * Si le score bruit > seuil, l'échantillon est ignoré (faux stimulus).
 * Valeur issue du test_07 NX22 (FALSE_STIMULUS_REJECTION = 0.99).
 */
#define NX48_NOISE_REJECT_THRESHOLD      0.99

/* ── Structure Merkle (NX11-NX32 : intégrité forensique) ─────────── */

#define NX48_MERKLE_HASH_LEN  17   /* 8 octets hex + null → format compact C */

typedef struct {
    char   root[NX48_MERKLE_HASH_LEN]; /* Racine Merkle courante (hex tronqué) */
    int    n_entries;                   /* Entrées depuis dernière rotation */
    uint64_t checkpoints[16];           /* Racines des dernières rotations */
    int    n_checkpoints;
} nx48_merkle_t;

/* ── Structure d'un neurone NX47AtomNeuron ───────────────────────── */

typedef struct {
    double w[NX48_N_FEATURES];      /* Poids linéaires (transduction NX28) */
    double alpha[NX48_N_FEATURES];  /* Poids quadratiques (énergie NX35) */
    double beta[NX48_N_FEATURES];   /* Poids gradient-signal (NX47 ARC) */
    double b;                        /* Biais scalaire */
    double lr;                       /* Taux d'apprentissage courant */
    double l1;                       /* Coefficient L1 */
    double l2;                       /* Coefficient L2 */
    double invariant_density;        /* Densité invariante (NX22-NX24) */
    double energy_dissipative;       /* Énergie dissipative accumulée (NX35) */
} nx48_neuron_t;

/* ── Vecteur de features QMC ────────────────────────────────────────  */

typedef struct {
    double   x[NX48_N_FEATURES];      /* Features brutes */
    double   grad_x[NX48_N_FEATURES]; /* Gradient ∂x/∂step (NX47 ARC) */
    double   label;                    /* Signe réel : 1.0 (pos) / 0.0 (nég) */
    double   noise_score;              /* Score de bruit VORAX (NX22) */
    uint64_t step_id;                  /* Identifiant step MC */
} nx48_sample_t;

/* ── Modèle NX48 complet ────────────────────────────────────────────  */

typedef struct {
    /* Méta-neurones compétiteurs (NX23 — effondrement partiel sans propagation) */
    nx48_neuron_t neurons[NX48_META_NEURONS];
    int           best_neuron;          /* Index meilleur neurone (F1 proxy) */
    double        best_f1;              /* F1 proxy du meilleur */

    /* Historique circulaire pour ∇x (NX47 ARC) */
    nx48_sample_t history[NX48_HISTORY_LEN];
    int           hist_head;
    int           hist_count;

    /* Chaîne Merkle forensique (NX11-NX32) */
    nx48_merkle_t merkle;

    /* Statistiques de prédiction */
    uint64_t n_samples_seen;
    uint64_t n_skipped;           /* Configs MC sautées (P < seuil) */
    uint64_t n_noise_rejected;    /* Échantillons rejetés (parasitage NX22) */
    uint64_t n_collapsed;         /* Neurones effondrés réinitialisés (NX24) */
    uint64_t n_positive_pred;
    double   sum_loss;            /* BCE accumulée */

    /* Détection stagnation (NX30 dialogue bidirectionnel) */
    double f1_history[NX48_STAGNATION_WIN];
    int    f1_hist_idx;

    /* Métadonnées */
    char   module_name[64];
    char   run_id[64];
    bool   initialized;
} nx48_model_t;

/* ── Résultat de prédiction ─────────────────────────────────────────  */

typedef struct {
    double   prob_positive;    /* P(signe_positif | features) ∈ [0, 1] */
    bool     skip_config;      /* true si prob < NX48_THRESHOLD_SIGN */
    bool     noise_rejected;   /* true si échantillon rejeté (bruit VORAX) */
    double   z_logit;          /* Score brut avant sigmoid */
    double   invariant_density;/* Densité invariante du meilleur neurone */
    uint32_t neuron_used;      /* Index méta-neurone utilisé */
} nx48_prediction_t;

/* ── API publique ────────────────────────────────────────────────────  */

/*
 * nx48_model_init — Initialise le modèle NX48 pour un module donné.
 * Poids : zéro (neurone neutre). invariant_density = 1.0 (NX24 : début sain).
 * module_name : préfixe STANDARD_NAMES ("simulate_fs", "sign_problem_module"…)
 * run_id      : identifiant du run courant
 */
void nx48_model_init(nx48_model_t *m, const char *module_name, const char *run_id);

/*
 * nx48_model_destroy — Libère et logue les statistiques finales + état Merkle.
 */
void nx48_model_destroy(nx48_model_t *m);

/*
 * nx48_build_sample — Construit un échantillon depuis les observables QMC.
 * Calcule ∇x par différences finies sur l'historique (NX47 ARC).
 * Calcule le score de bruit VORAX (NX22) pour décider du rejet.
 */
nx48_sample_t nx48_build_sample(nx48_model_t *m,
                                 double energy, double pairing,
                                 double sign_local, double U_t,
                                 double temp_red, double sign_true,
                                 uint64_t step_id);

/*
 * nx48_predict — Prédit P(signe_positif | features).
 * Utilise le meilleur méta-neurone.
 * Vérifie invariant_density (NX24 : collapse check).
 * Ne modifie pas les poids.
 */
nx48_prediction_t nx48_predict(const nx48_model_t *m, const nx48_sample_t *s);

/*
 * nx48_update_online — Mise à jour en ligne (1 échantillon, 1 iter ISTA).
 * Rejette l'échantillon si noise_score > NX48_NOISE_REJECT_THRESHOLD (NX22).
 * Met à jour la chaîne Merkle après chaque mise à jour (NX11-NX32).
 * Retourne la perte BCE courante (0 si rejeté).
 */
double nx48_update_online(nx48_model_t *m, const nx48_sample_t *s);

/*
 * nx48_fit_batch — Entraînement batch (NX48_MAX_ITER iterations ISTA).
 * Entraîne les NX48_META_NEURONS méta-neurones avec lr distincts (NX23).
 * Sélectionne le meilleur par F1 proxy.
 * Détecte stagnation → lr × 0.65 (NX30).
 * Vérifie effondrement densité invariante → réinitialisation (NX24).
 */
void nx48_fit_batch(nx48_model_t *m, const nx48_sample_t *samples, int n_samples);

/*
 * nx48_save_weights — Sauvegarde poids + état Merkle dans un fichier texte.
 */
int nx48_save_weights(const nx48_model_t *m, const char *path);

/*
 * nx48_load_weights — Charge poids + état Merkle.
 * Retourne 0 si succès, -1 si erreur.
 */
int nx48_load_weights(nx48_model_t *m, const char *path);

/*
 * nx48_log_stats — Logue via FORENSIC_LOG_MODULE_METRIC :
 *   statistiques prédiction, sparsité poids, densité invariante,
 *   racine Merkle courante, compteur effondrements (NX24).
 */
void nx48_log_stats(const nx48_model_t *m);

/* ── Fonctions internes (exposées pour tests) ────────────────────── */

double   nx48_sigmoid(double z);
double   nx48_prox_l1(double w, double lr_l1);
double   nx48_forward(const nx48_neuron_t *n, const nx48_sample_t *s);
double   nx48_bce_loss(double prob, double label);
double   nx48_f1_proxy(const nx48_neuron_t *n,
                        const nx48_sample_t *samples, int n_samples);
double   nx48_invariant_density(const nx48_neuron_t *n);
void     nx48_merkle_update(nx48_merkle_t *mk, uint64_t state_hash);
double   nx48_estimate_overhead_reduction(double overhead_original,
                                           double skip_rate, double precision);

#endif /* NX48_SIGN_PREDICTOR_H */
