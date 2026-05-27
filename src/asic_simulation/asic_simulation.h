/* asic_simulation.h — Simulation ASIC classique + quantique + IBM Heron R2.
 *
 * Cycle C95 — squelette d'API publique. Voir CAHIER_DES_CHARGES_C95_MAITRE.md §7.
 *
 * 3 niveaux :
 *   1. ASIC classique : pipeline SHA-256 type Antminer S19 (TH/s)
 *   2. ASIC quantique : grille N×N qubits (T1/T2/cross-talk)
 *   3. ASIC hybride IBM-like : modèle calibré Heron R2 (utilise IBM_C93_*)
 */
#ifndef LUMVORAX_ASIC_SIMULATION_H
#define LUMVORAX_ASIC_SIMULATION_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 1. ASIC SHA-256 classique (pipeline 64 rounds)
 * ============================================================================ */
typedef struct {
    int   pipeline_depth;        /* 64 par défaut */
    int   parallel_engines;      /* p.ex. 280 dans Antminer S19 XP */
    double clock_GHz;            /* p.ex. 0.5 GHz */
    double watts_per_TH;         /* p.ex. 21.5 W/TH (S19 XP) */
    /* État interne */
    uint64_t total_hashes;
    double   wall_time_s;
} asic_sha256_classical_t;

void asic_sha256_init(asic_sha256_classical_t *a, int engines, double clock_GHz, double w_per_th);
double asic_sha256_estimated_th_s(const asic_sha256_classical_t *a);   /* TH/s */
double asic_sha256_power_watts(const asic_sha256_classical_t *a);

/* ============================================================================
 * 2. ASIC quantique (array de qubits supraconducteurs)
 * ============================================================================ */
typedef struct {
    int    n_rows, n_cols;
    double T1_us;             /* relaxation amplitude */
    double T2_us;             /* déphasage */
    double gate_1q_us;        /* durée porte 1Q */
    double gate_2q_us;        /* durée porte 2Q (ECR/CZ) */
    double crosstalk_zz_kHz;  /* couplage parasite */
    double readout_fidelity;
    double *qubit_freq_GHz;   /* taille n_rows*n_cols */
} asic_quantum_array_t;

asic_quantum_array_t *asic_quantum_create(int rows, int cols);
void asic_quantum_destroy(asic_quantum_array_t *q);
void asic_quantum_set_uniform_params(asic_quantum_array_t *q,
                                     double T1, double T2,
                                     double g1, double g2);
double asic_quantum_estimated_2q_fidelity(const asic_quantum_array_t *q);

/* ── C98 — Hook VORAX (homogénéisation pipeline ASIC ↔ VORAX, audit C97.7) ──
 * Extrait un vecteur signal compact à partir de l'état ASIC quantique :
 *   signal[0] = F_2q estimé
 *   signal[1] = T1_us / T2_us (ratio cohérence)
 *   signal[2] = gate_2q_us / gate_1q_us (ratio temporel)
 *   signal[3] = crosstalk_zz_kHz normalisé (/100)
 *   signal[4] = readout_fidelity
 *   signal[5] = depth max sans dépasser F_2q^depth = 0.99 (profondeur utile)
 *   signal[6] = checksum FNV1a-64 normalisé (/2^64) pour audit forensique
 *   signal[7] = n_qubits / 256 (taille système normalisée)
 *
 * Cette fonction permet à VORAX (vorax_kernel) de consommer l'état ASIC
 * via correlation_vector_t sans dépendre du module quantum_problem_hubbard_hts. */
#define ASIC_VORAX_SIGNAL_DIM 8
int asic_quantum_extract_vorax_signal(const asic_quantum_array_t *q,
                                      double signal[ASIC_VORAX_SIGNAL_DIM]);

/* ── C99 — ASIC CONTRÔLEUR (et non plus seulement capteur) ────────────────
 * Critique experte C98 : « Tu n'utilises pas encore ce signal pour modifier
 * l'ansatz, adapter depth, choisir topology. → c'est un capteur, pas un
 * contrôleur. » C99 corrige : ces fonctions FOURNISSENT des recommandations
 * basées sur l'état hardware. */

/* Recommande la profondeur de circuit max pour atteindre une fidélité totale
 * cible target_total (ex 0.95). Renvoie depth_max ≥ 0 (entier).
 *   target_total = F_2q^depth → depth_max = floor(log(target) / log(F_2q))
 */
int asic_quantum_recommend_depth(const asic_quantum_array_t *q,
                                 double target_total_fidelity);

/* Recommande l'ansatz approprié selon les caractéristiques hardware :
 *   "trotter1"     si depth_useful >= 4 et crosstalk faible (<30 kHz)
 *   "rxx_brick"    si depth_useful 2-3 et crosstalk modéré
 *   "single_layer" si depth_useful < 2 (trop bruité pour multi-couches)
 *   "shallow_he"   si T2 << T1 (déphasage dominant) — Hardware Efficient peu profond
 * Le buffer de sortie doit faire au moins 32 octets. */
int asic_quantum_recommend_ansatz(const asic_quantum_array_t *q,
                                  char ansatz_name[32]);

/* Recommande resilience_level IBM (0-3) :
 *   0 si F_2q > 0.998         (hardware très propre, bypass mitigation)
 *   1 si F_2q in [0.99, 0.998] (TREX1 + ZNE)
 *   2 si F_2q in [0.97, 0.99]  (PEC partial)
 *   3 si F_2q < 0.97           (PEC full + DD XY4)
 */
int asic_quantum_recommend_resilience(const asic_quantum_array_t *q);

/* ============================================================================
 * 3. ASIC IBM Heron R2 (calibré sur ibm_kingston via IBM_C93_*)
 * ============================================================================ */
typedef struct {
    asic_quantum_array_t *base;
    /* Paramètres calibrés C93 */
    double s_pi_target;            /* 0.9944 d'après IBM_C93_S_PI */
    int    depth_phys_target;      /* 14 d'après IBM_C93_DEPTH_PHYS */
    int    n2q_phys_target;        /* 2  d'après IBM_C93_N2Q_PHYS */
} asic_ibm_heron_r2_t;

asic_ibm_heron_r2_t *asic_ibm_create_kingston(void);   /* 156 qubits, params Kingston */
void asic_ibm_destroy(asic_ibm_heron_r2_t *m);

/* Simule un circuit ADAPT-VQE+ZNE et compare au S(π) IBM réel. */
double asic_ibm_simulate_s_pi(const asic_ibm_heron_r2_t *m, int N, int depth);

#ifdef __cplusplus
}
#endif

#endif /* LUMVORAX_ASIC_SIMULATION_H */
