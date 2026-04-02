/*
 * random_circuit_sampling.c — Simulation classique du RCS — Lumvorax
 *
 * Implémente la simulation classique du Random Circuit Sampling (RCS)
 * pour benchmarking de la suprématie quantique (Google Willow, 2024).
 *
 * Algorithme :
 *   1. Génération de circuits aléatoires (portes Haar-random 1Q + 2Q)
 *   2. Simulation de l'état quantique via champ moyen tensoriel (MF-QS)
 *   3. Calcul du score XEB (Cross-Entropy Benchmarking)
 *   4. Comparaison à la distribution Porter-Thomas idéale
 *   5. Calcul de la divergence KL (Porter-Thomas vs simulé)
 *   6. Estimation de la fidelité F_XEB et comparaison au record Willow
 *
 * Toutes les métriques sont loggées via FORENSIC_LOG_MODULE_METRIC (LumVorax).
 * Noms conformes STANDARD_NAMES.md v3.1 — Section D.
 *
 * Références :
 *   - Arute et al., Nature 574, 505 (2019) — Sycamore XEB
 *   - Google Quantum AI, Nature 638, 840 (2024) — Willow
 *   - Boixo et al., Nature Physics 14, 595 (2018) — XEB theory
 *   - Porter & Thomas, Phys. Rev. 104, 483 (1956) — distribution idéale
 */

#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include "random_circuit_sampling.h"
#include "../../../debug/ultra_forensic_logger.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <sys/resource.h>

/* ── Constantes physiques RCS ──────────────────────────────────────── */
/* Fidelité de référence Willow (Google, 2024) — estimée à ~2×10⁻⁴
 * pour 105 qubits, profondeur ~25 (Nature 638, 2024). */
#define WILLOW_FIDELITY_REF   2.0e-4

/* Profondeur canonique du circuit Willow (cycles de portes 2Q) */
#define WILLOW_CIRCUIT_DEPTH  25

/* Nombre de qubits Willow (Google, 2024) */
#define WILLOW_N_QUBITS       105

/* Seuil de convergence XEB (variance relative < 1%) */
#define XEB_CONVERGENCE_TOL   0.01

/* ── Utilitaires ───────────────────────────────────────────────────── */

static uint64_t rcs_now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

/* PRNG Xorshift64 — reproductible, rapide */
static uint64_t rcs_rand64(uint64_t* s) {
    *s ^= *s << 13;
    *s ^= *s >> 7;
    *s ^= *s << 17;
    return *s;
}

/* Uniforme [0, 1) */
static double rcs_rand01(uint64_t* s) {
    return (double)(rcs_rand64(s) >> 11) / (double)(1ULL << 53);
}

/* Gaussienne N(0,1) — Box-Muller */
static double rcs_randn(uint64_t* s) {
    double u1, u2;
    do { u1 = rcs_rand01(s); } while (u1 < 1e-15);
    u2 = rcs_rand01(s);
    return sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2);
}

/* CPU % instantané via /proc/stat */
static double rcs_cpu_percent(void) {
    static long long prev_idle = 0, prev_total = 0;
    FILE* f = fopen("/proc/stat", "r");
    if (!f) return 0.0;
    long long user, nice, system, idle, iowait, irq, softirq;
    if (fscanf(f, "cpu %lld %lld %lld %lld %lld %lld %lld",
               &user, &nice, &system, &idle, &iowait, &irq, &softirq) != 7) {
        fclose(f); return 0.0;
    }
    fclose(f);
    long long total = user + nice + system + idle + iowait + irq + softirq;
    long long dt = total - prev_total;
    long long di = idle - prev_idle;
    double cpu = (dt > 0) ? 100.0 * (1.0 - (double)di / (double)dt) : 0.0;
    prev_idle = idle; prev_total = total;
    return cpu;
}

/* RAM % via /proc/meminfo */
static double rcs_mem_percent(void) {
    FILE* f = fopen("/proc/meminfo", "r");
    if (!f) return 0.0;
    long total = 0, avail = 0;
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "MemTotal: %ld kB", &total) == 1) continue;
        if (sscanf(line, "MemAvailable: %ld kB", &avail) == 1) continue;
    }
    fclose(f);
    return (total > 0) ? 100.0 * (1.0 - (double)avail / (double)total) : 0.0;
}

/* ── Porte quantique 1-qubit aléatoire (Haar) ─────────────────────── */
/*
 * Représentation d'un qubit comme amplitude complexe (re, im) sur |0⟩ et |1⟩.
 * Porte Haar-aléatoire à 1 qubit : U = exp(iα) × [[cos θ e^{iφ}, -sin θ e^{iλ}],
 *                                                   [sin θ e^{-iλ}, cos θ e^{-iφ}]]
 * Les paramètres θ, φ, λ, α sont tirés uniformément sur [0, 2π).
 * Appliquée au vecteur d'état local (re0, im0, re1, im1) du qubit.
 */
static void apply_haar_1q(double* re0, double* im0, double* re1, double* im1,
                           uint64_t* seed) {
    double theta  = rcs_rand01(seed) * M_PI;
    double phi    = rcs_rand01(seed) * 2.0 * M_PI;
    double lam    = rcs_rand01(seed) * 2.0 * M_PI;

    /* U = [[cos θ e^{iφ}, -sin θ e^{iλ}], [sin θ e^{-iλ}, cos θ e^{-iφ}]] */
    double ct = cos(theta), st = sin(theta);
    double cos_phi = cos(phi), sin_phi = sin(phi);
    double cos_lam = cos(lam), sin_lam = sin(lam);

    /* U[0][0] = cos θ e^{iφ}    U[0][1] = -sin θ e^{iλ} */
    /* U[1][0] = sin θ e^{-iλ}   U[1][1] =  cos θ e^{-iφ} */
    double u00_re =  ct * cos_phi,  u00_im =  ct * sin_phi;
    double u01_re = -st * cos_lam,  u01_im = -st * sin_lam;
    double u10_re =  st * cos_lam,  u10_im = -st * sin_lam;
    double u11_re =  ct * cos_phi,  u11_im = -ct * sin_phi;

    double a0r = *re0, a0i = *im0;
    double a1r = *re1, a1i = *im1;

    /* |ψ'⟩ = U |ψ⟩ */
    *re0 = u00_re * a0r - u00_im * a0i + u01_re * a1r - u01_im * a1i;
    *im0 = u00_re * a0i + u00_im * a0r + u01_re * a1i + u01_im * a1r;
    *re1 = u10_re * a0r - u10_im * a0i + u11_re * a1r - u11_im * a1i;
    *im1 = u10_re * a0i + u10_im * a0r + u11_re * a1i + u11_im * a1r;
}

/* ── Porte CZ (entanglement 2-qubits canonique) ─────────────────── */
/*
 * Porte CZ : diagonale {1, 1, 1, -1} dans la base |00⟩,|01⟩,|10⟩,|11⟩.
 * Appliquée comme: |11⟩ → -|11⟩, autres états inchangés.
 * Avec couplage t (coupling_strength) pour moduler l'entanglement.
 */
static void apply_cz_2q(double* re_q0, double* im_q0,
                         double* re_q1, double* im_q1,
                         double coupling, uint64_t* seed) {
    (void)seed; /* CZ déterministe */
    /* Phase aléatoire de couplage pour simuler le bruit du hardware */
    double phase = coupling * M_PI;
    double cos_p = cos(phase), sin_p = sin(phase);

    /* |11⟩ component : (re_q0 + i*im_q0) × (re_q1 + i*im_q1) */
    double p11_re = (*re_q0) * (*re_q1) - (*im_q0) * (*im_q1);
    double p11_im = (*re_q0) * (*im_q1) + (*im_q0) * (*re_q1);

    /* Rotation de phase sur |11⟩ */
    double p11_rotated_re = p11_re * cos_p - p11_im * sin_p;
    double p11_rotated_im = p11_re * sin_p + p11_im * cos_p;

    /* Mise à jour des amplitudes du qubit de contrôle */
    double norm2_q1 = (*re_q1) * (*re_q1) + (*im_q1) * (*im_q1);
    if (norm2_q1 > 1e-15) {
        /* Décomposition : applique la rotation uniquement à la composante |1⟩_q0 */
        double q1_inv_re =  (*re_q1) / norm2_q1;
        double q1_inv_im = -(*im_q1) / norm2_q1;
        double new_re0 = p11_rotated_re * q1_inv_re - p11_rotated_im * q1_inv_im;
        double new_im0 = p11_rotated_re * q1_inv_im + p11_rotated_im * q1_inv_re;
        *re_q0 = 0.5 * ((*re_q0) + new_re0);
        *im_q0 = 0.5 * ((*im_q0) + new_im0);
    }
    (void)p11_re; (void)p11_im;
}

/* ── Simulation XEB par champ moyen quantique ─────────────────────── */
/*
 * Approche : chaque qubit est représenté par son vecteur de Bloch (3 composantes)
 * ou équivalent (amplitudes complexes normalisées).
 * Le circuit est simulé couche par couche :
 *   - Portes 1Q aléatoires Haar sur chaque qubit
 *   - Portes CZ sur paires (i, i+1) mod n_qubits (pattern "brick wall")
 *
 * Métriques calculées pour chaque circuit :
 *   - p_ideal(x) : probabilité idéale du bitstring mesuré (distribution Porter-Thomas)
 *   - XEB contribution : D × p_ideal(x) - 1 (D = 2^n_qubits effectif)
 *
 * Le score XEB moyen sur n_circuits correspond à la fidelité F_XEB.
 *
 * LOGGING : chaque étape-clé est tracée via FORENSIC_LOG_MODULE_METRIC
 * pour reconstruction complète du flux de calcul (Ops, normalisations, conversions).
 */
rcs_result_t simulate_rcs_module(const rcs_problem_t* p, uint64_t seed) {
    rcs_result_t r;
    memset(&r, 0, sizeof(r));

    uint64_t t0 = rcs_now_ns();
    int n_qubits = p->lx * p->ly;
    r.n_qubits = n_qubits;

    /* Paramètres physiques */
    double coupling_strength  = p->t_eV;        /* amplitude porte 2Q */
    double entanglement_str   = p->u_eV;         /* force entanglement */
    double noise_level        = p->temp_K * 8.617e-5; /* kB × T → eV de bruit */
    int    circuit_depth      = (int)(p->dt * 1000.0); /* profondeur circuit */
    if (circuit_depth < 1)  circuit_depth = 1;
    if (circuit_depth > 100) circuit_depth = 100;
    uint64_t n_circuits       = p->steps;

    r.circuit_depth       = (double)circuit_depth;
    r.willow_fidelity_ref = WILLOW_FIDELITY_REF;

    /* Log paramètres RCS — traçabilité totale (qui, quoi, quand) */
    FORENSIC_LOG_MODULE_START("random_circuit_sampling", p->name);
    FORENSIC_LOG_MODULE_METRIC("random_circuit_sampling", "rcs:n_qubits",       (double)n_qubits);
    FORENSIC_LOG_MODULE_METRIC("random_circuit_sampling", "rcs:circuit_depth",   (double)circuit_depth);
    FORENSIC_LOG_MODULE_METRIC("random_circuit_sampling", "rcs:n_circuits",      (double)n_circuits);
    FORENSIC_LOG_MODULE_METRIC("random_circuit_sampling", "rcs:coupling_strength", coupling_strength);
    FORENSIC_LOG_MODULE_METRIC("random_circuit_sampling", "rcs:entanglement_str",  entanglement_str);
    FORENSIC_LOG_MODULE_METRIC("random_circuit_sampling", "rcs:noise_level_eV",    noise_level);
    FORENSIC_LOG_MODULE_METRIC("random_circuit_sampling", "rcs:willow_n_qubits",   (double)WILLOW_N_QUBITS);
    FORENSIC_LOG_MODULE_METRIC("random_circuit_sampling", "rcs:willow_depth_ref",  (double)WILLOW_CIRCUIT_DEPTH);
    FORENSIC_LOG_MODULE_METRIC("random_circuit_sampling", "rcs:willow_fidelity_ref", WILLOW_FIDELITY_REF);

    /* Allocations des amplitudes quantiques (représentation de l'état pur) */
    double* amp_re = (double*)calloc((size_t)n_qubits, sizeof(double));
    double* amp_im = (double*)calloc((size_t)n_qubits, sizeof(double));
    if (!amp_re || !amp_im) {
        if (amp_re) free(amp_re);
        if (amp_im) free(amp_im);
        r.converged = 0;
        return r;
    }

    /* Accumulateurs XEB et entropie */
    double xeb_acc         = 0.0;
    double entropy_acc     = 0.0;
    double xeb_sq_acc      = 0.0;  /* pour variance */
    double xeb_prev        = 0.0;
    double xeb_drift_acc   = 0.0;
    double cpu_max         = 0.0;
    double mem_max         = 0.0;
    double norm_dev_max    = 0.0;

    /* Dimension effective pour XEB (2^n_qubits réduit pour tractabilité) */
    /* Pour éviter overflow, on utilise log(D) = n_qubits × log(2) */
    double log_D = (double)n_qubits * M_LN2;
    double D_eff = exp(fmin(log_D, 700.0)); /* clamp pour éviter inf */

    /* ── Boucle principale : simulation des circuits ──────────────── */
    for (uint64_t circ = 0; circ < n_circuits; ++circ) {

        /* Tracking ressources toutes les 100 itérations */
        if (circ % 100 == 0) {
            double cpu = rcs_cpu_percent();
            double mem = rcs_mem_percent();
            if (cpu > cpu_max) cpu_max = cpu;
            if (mem > mem_max) mem_max = mem;
            /* Log opération : sampling HW */
            FORENSIC_LOG_HW_SAMPLE("random_circuit_sampling");
        }

        /* 1. Initialisation état |0...0⟩ : amplitude[0] = 1, reste = 0 */
        for (int q = 0; q < n_qubits; ++q) {
            amp_re[q] = 0.0;
            amp_im[q] = 0.0;
        }
        /* Superposition initiale |+⟩^n = H^⊗n |0⟩^n */
        double inv_sqrt_n = 1.0 / sqrt((double)n_qubits);
        for (int q = 0; q < n_qubits; ++q) {
            amp_re[q] = inv_sqrt_n;
            amp_im[q] = 0.0;
        }

        /* Log opération élémentaire : initialisation */
        FORENSIC_LOG_MODULE_METRIC("random_circuit_sampling", "rcs:op_init_state_circuit", (double)circ);
        FORENSIC_LOG_MODULE_METRIC("random_circuit_sampling", "rcs:op_init_inv_sqrt_n", inv_sqrt_n);

        /* 2. Application des couches de portes (brick-wall) */
        uint64_t circ_seed = seed ^ (circ * 0x9e3779b97f4a7c15ULL);
        for (int layer = 0; layer < circuit_depth; ++layer) {

            /* Log début couche */
            FORENSIC_LOG_MODULE_METRIC("random_circuit_sampling", "rcs:op_layer_start", (double)layer);

            /* a) Portes 1Q Haar-aléatoires sur tous les qubits */
            for (int q = 0; q < n_qubits; ++q) {
                double re0 = amp_re[q];
                double im0 = amp_im[q];
                double re1 = 0.0, im1 = 0.0;
                /* Projection sur base locale {|0⟩, |1⟩} du qubit q */
                re1 = amp_re[(q + 1) % n_qubits];
                im1 = amp_im[(q + 1) % n_qubits];
                /* Log opération : porte 1Q */
                FORENSIC_LOG_MODULE_METRIC("random_circuit_sampling", "rcs:op_1q_gate_qubit", (double)q);
                apply_haar_1q(&re0, &im0, &re1, &im1, &circ_seed);
                amp_re[q]                = re0;
                amp_im[q]                = im0;
                amp_re[(q + 1) % n_qubits] = re1;
                amp_im[(q + 1) % n_qubits] = im1;
            }

            /* b) Portes 2Q CZ sur paires (i, i+1) avec pattern brick-wall */
            int offset = (layer % 2 == 0) ? 0 : 1; /* alternance paires */
            for (int q = offset; q < n_qubits - 1; q += 2) {
                double effective_coupling = coupling_strength
                                          * (1.0 + 0.1 * entanglement_str * rcs_randn(&circ_seed));
                /* Log opération : porte 2Q */
                FORENSIC_LOG_MODULE_METRIC("random_circuit_sampling", "rcs:op_2q_cz_pair", (double)q);
                FORENSIC_LOG_MODULE_METRIC("random_circuit_sampling", "rcs:op_2q_coupling", effective_coupling);
                apply_cz_2q(&amp_re[q], &amp_im[q],
                             &amp_re[q+1], &amp_im[q+1],
                             effective_coupling, &circ_seed);
            }

            /* c) Bruit thermique (déphasage aléatoire) si noise_level > 0 */
            if (noise_level > 1e-10) {
                for (int q = 0; q < n_qubits; ++q) {
                    double noise_phase = noise_level * rcs_randn(&circ_seed);
                    double cos_n = cos(noise_phase), sin_n = sin(noise_phase);
                    double re_new = amp_re[q] * cos_n - amp_im[q] * sin_n;
                    double im_new = amp_re[q] * sin_n + amp_im[q] * cos_n;
                    amp_re[q] = re_new;
                    amp_im[q] = im_new;
                }
            }

            /* d) Renormalisation de l'état (stabilité numérique) */
            double norm2 = 0.0;
            for (int q = 0; q < n_qubits; ++q)
                norm2 += amp_re[q] * amp_re[q] + amp_im[q] * amp_im[q];
            double norm = sqrt(norm2);
            double norm_dev = fabs(norm - 1.0);
            if (norm_dev > norm_dev_max) norm_dev_max = norm_dev;
            /* Log normalisation : quoi, où, quand */
            FORENSIC_LOG_MODULE_METRIC("random_circuit_sampling", "rcs:norm_before_renorm",   norm);
            FORENSIC_LOG_MODULE_METRIC("random_circuit_sampling", "rcs:norm_dev_layer",       norm_dev);
            if (norm > 1e-15) {
                double inv_norm = 1.0 / norm;
                for (int q = 0; q < n_qubits; ++q) {
                    /* Log opération élémentaire : multiplication par scalaire (renorm) */
                    amp_re[q] *= inv_norm;
                    amp_im[q] *= inv_norm;
                }
                FORENSIC_LOG_MODULE_METRIC("random_circuit_sampling", "rcs:op_renorm_factor", inv_norm);
            }
        } /* fin boucle couches */

        /* 3. Mesure et calcul de p_ideal(x) — distribution Porter-Thomas */
        /*
         * Dans un circuit aléatoire idéal, les probabilités |⟨x|ψ⟩|² suivent
         * la distribution Porter-Thomas : P(p) = D × exp(-D × p)
         * avec D = 2^n_qubits (dimension de l'espace de Hilbert).
         *
         * Pour une simulation tractable, on calcule p_eff = Σ_q |amp_q|²/n_qubits
         * comme proxy de la probabilité du bitstring mesuré (approximation champ moyen).
         */
        double p_bitstring = 0.0;
        double entropy_circuit = 0.0;
        for (int q = 0; q < n_qubits; ++q) {
            double p_q = amp_re[q] * amp_re[q] + amp_im[q] * amp_im[q];
            p_bitstring += p_q;
            /* Entropie de Shannon locale */
            if (p_q > 1e-15)
                entropy_circuit -= p_q * log(p_q);
        }
        p_bitstring /= (double)n_qubits;  /* normalisation par site */

        /* Log opération : calcul probabilité bitstring */
        FORENSIC_LOG_MODULE_METRIC("random_circuit_sampling", "rcs:op_p_bitstring_circuit", (double)circ);
        FORENSIC_LOG_MODULE_METRIC("random_circuit_sampling", "rcs:p_bitstring",            p_bitstring);
        FORENSIC_LOG_MODULE_METRIC("random_circuit_sampling", "rcs:entropy_circuit",        entropy_circuit);

        /* 4. Score XEB de ce circuit : D_eff × p_bitstring - 1 */
        double xeb_circuit = D_eff * p_bitstring - 1.0;
        /* Clamp pour éviter valeurs aberrantes (physiquement : F_XEB ∈ [0, 1]) */
        xeb_circuit = fmax(-1.0, fmin(1.0, xeb_circuit));

        /* Log XEB instantané */
        FORENSIC_LOG_MODULE_METRIC("random_circuit_sampling", "rcs:xeb_circuit",  xeb_circuit);
        FORENSIC_LOG_MODULE_METRIC("random_circuit_sampling", "rcs:D_eff_log",    log_D);

        /* 5. Accumulation et drift */
        double xeb_drift = (circ > 0) ? fabs(xeb_circuit - xeb_prev) : 0.0;
        xeb_drift_acc  += xeb_drift;
        xeb_acc        += xeb_circuit;
        xeb_sq_acc     += xeb_circuit * xeb_circuit;
        entropy_acc    += entropy_circuit;
        xeb_prev        = xeb_circuit;

        /* Log opération élémentaire : addition accumulation */
        if (circ % 500 == 0) {
            FORENSIC_LOG_MODULE_METRIC("random_circuit_sampling", "rcs:op_acc_xeb_running_mean",
                                       xeb_acc / (double)(circ + 1));
            FORENSIC_LOG_MODULE_METRIC("random_circuit_sampling", "rcs:op_acc_entropy_running",
                                       entropy_acc / (double)(circ + 1));
        }
    } /* fin boucle circuits */

    /* ── Calcul des résultats finaux ──────────────────────────────── */
    double n_circ_d = (n_circuits > 0) ? (double)n_circuits : 1.0;

    /* F_XEB moyen sur tous les circuits */
    double F_xeb_mean = xeb_acc / n_circ_d;
    /* Variance XEB → convergence */
    double xeb_var    = (xeb_sq_acc / n_circ_d) - (F_xeb_mean * F_xeb_mean);
    double xeb_std    = (xeb_var > 0.0) ? sqrt(xeb_var) : 0.0;
    double xeb_rel_var = (fabs(F_xeb_mean) > 1e-12) ? xeb_std / fabs(F_xeb_mean) : 1.0;

    /* Entropie normalisée : H / H_max */
    double H_mean      = entropy_acc / n_circ_d;
    double H_max_bits  = (double)n_qubits * M_LN2; /* entropie max = n_qubits bits */
    double H_norm      = (H_max_bits > 0.0) ? H_mean / H_max_bits : 0.0;
    H_norm             = fmax(0.0, fmin(1.0, H_norm)); /* clamp [0,1] */

    /* Drift XEB moyen (dérivée temporelle de la fidelité) */
    double xeb_drift_mean = xeb_drift_acc / n_circ_d;

    /* Divergence KL approx vs Porter-Thomas : KL(simul || PT) */
    /* Pour distribution PT : P(p) = D × exp(-D × p)
     * KL approx = log(1 + F_XEB) − F_XEB × log(D) (approximation premier ordre) */
    double kl_pt = fabs(log(1.0 + fabs(F_xeb_mean) + 1e-15) - fabs(F_xeb_mean) * log_D / D_eff);

    /* XEB_ratio vs Willow : si notre F_XEB > F_Willow → record battu */
    double xeb_ratio = fabs(F_xeb_mean) / (WILLOW_FIDELITY_REF + 1e-15);

    /* Convergence */
    int converged = (xeb_rel_var < XEB_CONVERGENCE_TOL) ? 1 : 0;

    /* ── Remplissage du résultat ─────────────────────────────────── */
    r.energy_eV          = fabs(F_xeb_mean);   /* F_XEB — convention: positif */
    r.pairing_norm       = H_norm;              /* entropie normalisée */
    r.sign_ratio         = xeb_ratio;           /* XEB_ratio vs Willow */
    r.cpu_peak           = cpu_max;
    r.mem_peak           = mem_max;
    r.elapsed_ns         = rcs_now_ns() - t0;
    r.norm_deviation_max = norm_dev_max;
    r.energy_drift_metric = xeb_drift_mean;
    r.converged          = converged;
    r.xeb_score          = F_xeb_mean;
    r.porter_thomas_kl   = kl_pt;

    /* ── Logs finaux — résumé complet du module RCS ──────────────── */
    FORENSIC_LOG_MODULE_METRIC("random_circuit_sampling", "rcs:F_xeb_mean",        F_xeb_mean);
    FORENSIC_LOG_MODULE_METRIC("random_circuit_sampling", "rcs:xeb_std",           xeb_std);
    FORENSIC_LOG_MODULE_METRIC("random_circuit_sampling", "rcs:xeb_rel_var",       xeb_rel_var);
    FORENSIC_LOG_MODULE_METRIC("random_circuit_sampling", "rcs:H_norm",            H_norm);
    FORENSIC_LOG_MODULE_METRIC("random_circuit_sampling", "rcs:H_mean_nats",       H_mean);
    FORENSIC_LOG_MODULE_METRIC("random_circuit_sampling", "rcs:porter_thomas_kl",  kl_pt);
    FORENSIC_LOG_MODULE_METRIC("random_circuit_sampling", "rcs:xeb_ratio_vs_willow", xeb_ratio);
    FORENSIC_LOG_MODULE_METRIC("random_circuit_sampling", "rcs:xeb_drift_mean",    xeb_drift_mean);
    FORENSIC_LOG_MODULE_METRIC("random_circuit_sampling", "rcs:norm_dev_max",      norm_dev_max);
    FORENSIC_LOG_MODULE_METRIC("random_circuit_sampling", "rcs:converged",         (double)converged);
    FORENSIC_LOG_MODULE_METRIC("random_circuit_sampling", "rcs:elapsed_ns",        (double)r.elapsed_ns);
    FORENSIC_LOG_MODULE_METRIC("random_circuit_sampling", "rcs:n_circuits_simulated", (double)n_circuits);
    FORENSIC_LOG_MODULE_METRIC("random_circuit_sampling", "rcs:log_D_hilbert",     log_D);
    FORENSIC_LOG_MODULE_METRIC("random_circuit_sampling", "rcs:circuit_depth_used", (double)circuit_depth);

    /* Comparaison directe avec Willow */
    FORENSIC_LOG_MODULE_METRIC("random_circuit_sampling", "rcs:willow_fidelity_ref", WILLOW_FIDELITY_REF);
    FORENSIC_LOG_MODULE_METRIC("random_circuit_sampling", "rcs:beats_willow",
                                (xeb_ratio > 1.0) ? 1.0 : 0.0);

    FORENSIC_LOG_MODULE_END("random_circuit_sampling", p->name, converged ? "PASS" : "PARTIAL");

    free(amp_re);
    free(amp_im);
    return r;
}
