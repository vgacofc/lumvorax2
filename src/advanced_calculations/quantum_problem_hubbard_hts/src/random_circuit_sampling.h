/*
 * random_circuit_sampling.h — Module RCS Lumvorax
 *
 * Simulation classique du Random Circuit Sampling (RCS) :
 * protocole utilisé pour la démonstration de suprématie quantique
 * (Google Sycamore 2019, Google Willow 2024).
 *
 * Objectif scientifique :
 *   Simuler classiquement le RCS sur un réseau de qubits (lx × ly)
 *   avec des portes Haar-aléatoires à 2 qubits intercalées de portes
 *   aléatoires à 1 qubit. Calculer le score XEB (Cross-Entropy
 *   Benchmarking) et comparer à la distribution Porter-Thomas idéale.
 *
 * Sortie (convention unifiée sim_result_t) :
 *   energy_eV    → F_XEB (fidelité XEB, sans unité, [0, 1])
 *   pairing_norm → H_shannon / H_max (entropie normalisée, [0, 1])
 *   sign_ratio   → XEB_ratio vs borne Willow record (>1 = record battu)
 *   cpu_peak     → utilisation CPU max pendant la simulation (%)
 *   mem_peak     → utilisation RAM max (%)
 *   elapsed_ns   → durée totale nanoseconde
 *
 * Noms conformes STANDARD_NAMES.md v3.1 — Section D (module RCS).
 *
 * Références :
 *   - Arute et al., Nature 574, 505 (2019) — Google Sycamore
 *   - Google Quantum AI, Nature 638, 840 (2024) — Google Willow
 *   - Boixo et al., Nature Physics 14, 595 (2018) — XEB theory
 *   - Porter & Thomas, Phys. Rev. 104, 483 (1956) — distribution idéale
 */

#ifndef RANDOM_CIRCUIT_SAMPLING_H
#define RANDOM_CIRCUIT_SAMPLING_H

#include <stdint.h>

/* Résultat unifié (même struct que sim_result_t dans le runner principal) */
typedef struct {
    double energy_eV;           /* F_XEB — fidelité Cross-Entropy Benchmarking   */
    double pairing_norm;        /* H_shannon / H_max — entropie normalisée         */
    double sign_ratio;          /* XEB_ratio vs borne Willow (1.0 = égale Willow) */
    double cpu_peak;            /* CPU peak % pendant simulation                   */
    double mem_peak;            /* RAM peak %                                      */
    uint64_t elapsed_ns;        /* Durée totale nanoseconde                        */
    double norm_deviation_max;  /* Max |ψ†ψ - 1| (stabilité numérique)            */
    double energy_drift_metric; /* Drift XEB entre circuits consécutifs            */
    int    converged;           /* 1 si XEB convergé (variance < seuil)            */
    /* Métriques RCS spécifiques */
    double xeb_score;           /* Score XEB brut (D × <p_ideal> - 1)             */
    double porter_thomas_kl;    /* Divergence KL vs distribution Porter-Thomas     */
    double circuit_depth;       /* Profondeur effective du circuit simulé          */
    int    n_qubits;            /* Nombre total de qubits (lx × ly)                */
    double willow_fidelity_ref; /* Fidelité de référence Willow (0.0002 estimé)   */
} rcs_result_t;

/* Paramètres d'un problème RCS (aligné sur problem_t du runner) */
typedef struct {
    const char* name;   /* "random_circuit_sampling" */
    int    lx, ly;      /* Dimensions réseau qubits */
    double t_eV;        /* coupling_strength (amplitude porte 2-qubits) */
    double u_eV;        /* entanglement_strength */
    double mu_eV;       /* phase_offset */
    double temp_K;      /* noise_level_K (bruit thermique, 0 = idéal) */
    double dt;          /* circuit_depth_increment */
    uint64_t steps;     /* n_circuits_sampled */
} rcs_problem_t;

/*
 * simulate_rcs_module() — Point d'entrée principal
 *
 * Simule le RCS sur le réseau rcs_problem_t et retourne rcs_result_t.
 * Le résultat est aussi loggé via FORENSIC_LOG_MODULE_METRIC (LumVorax).
 *
 * seed : graine PRNG (reproductibilité garantie)
 */
rcs_result_t simulate_rcs_module(const rcs_problem_t* p, uint64_t seed);

#endif /* RANDOM_CIRCUIT_SAMPLING_H */
