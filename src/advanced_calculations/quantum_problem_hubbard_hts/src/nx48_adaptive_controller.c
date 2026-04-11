/*
 * nx48_adaptive_controller.c — Contrôleur Adaptatif NX48 (C55)
 *
 * Module C natif gérant dynamiquement la TOTALITÉ des paramètres :
 *   taille, circuit_depth, n_qubits, n_steps, n_sweeps, RAM, CPU, etc.
 *
 * Architecture NX47AtomNeuron étendue (20 features) :
 *   z = x·w + x²·alpha + ∇x·beta + b  →  P(+) = sigmoid(z)
 *
 * Apprentissage ISTA proximal (NX26) — sans rétropropagation.
 * Méta-neurones compétiteurs (NX23) — robustesse VORAX (NX22).
 * Effondrement densité invariante (NX24) — réinitialisation automatique.
 * Gradient physique ∂x/∂step (NX47 ARC) — apprentissage sans points.
 *
 * RÈGLE ABSOLUE : 0 métrique simulée. CPU/RAM lus via /proc (RÉELS).
 * Conforme prompt.txt + STANDARD_NAMES.md v3.7 — Section M (C55).
 *
 * Source NX :
 *   src/nx_versions/nx48_unified_ultra/nx48_unified_ultra.py
 *   src/nx_versions/nx47_arc_kernel.py
 *   src/nx_versions/ NX11→NX48 (toute la lignée)
 *
 * C55 — 2026-04-09
 */

#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include "nx48_adaptive_controller.h"
#include "../../../debug/ultra_forensic_logger.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

/* ── Constantes internes ───────────────────────────────────────────────── */

#define NX48C_HW_READ_INTERVAL_NS  500000000ULL   /* 500ms entre lectures /proc */
#define NX48C_N_MAX_THREADS        4               /* threads OMP max (Replit) */
#define NX48C_SCALE_MIN            0.5             /* échelle minimale paramètres */
#define NX48C_SCALE_MAX_CIRC       3.0             /* échelle max n_circuits */
#define NX48C_SCALE_MAX_STD        2.0             /* échelle max autres */

/* ── Timestamp monotone (ns) ───────────────────────────────────────────── */

static uint64_t nx48c_now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

/* ── Fonctions mathématiques ────────────────────────────────────────────── */

double nx48c_sigmoid(double z) {
    if (z >= 0.0) { double e = exp(-z); return 1.0 / (1.0 + e); }
    else           { double e = exp( z); return e   / (1.0 + e); }
}

double nx48c_prox_l1(double w, double lr_l1) {
    if (w > lr_l1)       return w - lr_l1;
    else if (w < -lr_l1) return w + lr_l1;
    else                  return 0.0;
}

double nx48c_forward(const nx48c_neuron_t *n, const nx48c_sample_t *s) {
    double z = n->b;
    for (int f = 0; f < NX48C_N_FEATURES; f++) {
        double xf  = s->x[f];
        double gxf = s->grad_x[f];
        z += xf     * n->w[f];
        z += xf*xf  * n->alpha[f];
        z += gxf    * n->beta[f];
    }
    return z;
}

/* ── Lecture réelle CPU via /proc/stat ─────────────────────────────────── */

/* Lit /proc/stat ligne cpu. Retourne le % CPU global en [0, 1].
 * Méthode : différence entre deux lectures successives.
 * Si pas de lecture précédente → retourne 0.0 (safe). */

static double nx48c_prev_idle = 0.0, nx48c_prev_total = 0.0;

static double nx48c_read_cpu_pct(void) {
    FILE *fp = fopen("/proc/stat", "r");
    if (!fp) return 0.0;

    unsigned long long user = 0, nice = 0, sys = 0, idle = 0,
                       iowait = 0, irq = 0, softirq = 0;
    int matched = fscanf(fp, "cpu %llu %llu %llu %llu %llu %llu %llu",
                          &user, &nice, &sys, &idle, &iowait, &irq, &softirq);
    fclose(fp);
    if (matched < 4) return 0.0;

    double total_now = (double)(user + nice + sys + idle + iowait + irq + softirq);
    double idle_now  = (double)(idle + iowait);

    double dtotal = total_now - nx48c_prev_total;
    double didle  = idle_now  - nx48c_prev_idle;

    nx48c_prev_total = total_now;
    nx48c_prev_idle  = idle_now;

    if (dtotal < 1.0) return 0.0;
    double cpu_pct = (dtotal - didle) / dtotal;   /* [0, 1] */
    if (cpu_pct < 0.0) cpu_pct = 0.0;
    if (cpu_pct > 1.0) cpu_pct = 1.0;
    return cpu_pct;
}

/* ── Lecture réelle RAM via /proc/meminfo ─────────────────────────────── */

static double nx48c_read_ram_pct(void) {
    FILE *fp = fopen("/proc/meminfo", "r");
    if (!fp) return 0.0;

    unsigned long total = 0, free_mem = 0, buffers = 0, cached = 0;
    char line[128];
    while (fgets(line, sizeof(line), fp)) {
        unsigned long val = 0;
        if (strncmp(line, "MemTotal:", 9)  == 0) { sscanf(line + 9, " %lu", &val); total    = val; }
        if (strncmp(line, "MemFree:",  8)  == 0) { sscanf(line + 8, " %lu", &val); free_mem = val; }
        if (strncmp(line, "Buffers:",  8)  == 0) { sscanf(line + 8, " %lu", &val); buffers  = val; }
        if (strncmp(line, "Cached:",   7)  == 0) { sscanf(line + 7, " %lu", &val);
            /* Ne prendre Cached que si buffers déjà lu (évite SwapCached) */
            if (buffers > 0 || cached == 0) cached = val;
        }
        if (strncmp(line, "MemAvailable:", 13) == 0 && total > 0) {
            sscanf(line + 13, " %lu", &val);
            double used_pct = 1.0 - (double)val / (double)total;
            fclose(fp);
            return (used_pct < 0.0) ? 0.0 : (used_pct > 1.0 ? 1.0 : used_pct);
        }
    }
    fclose(fp);
    if (total == 0) return 0.0;
    double avail = (double)(free_mem + buffers + cached);
    double used  = 1.0 - avail / (double)total;
    return (used < 0.0) ? 0.0 : (used > 1.0 ? 1.0 : used);
}

/* ── Lecture hardware groupée (toutes les 500ms) ──────────────────────── */

void nx48_ctrl_read_hw(nx48_ctrl_t *c) {
    uint64_t now = nx48c_now_ns();
    if ((now - c->hw_last_read_ns) < NX48C_HW_READ_INTERVAL_NS) return;
    c->hw_last_read_ns = now;
    c->hw_cpu_pct      = nx48c_read_cpu_pct();
    c->hw_ram_pct      = nx48c_read_ram_pct();
    /* n_threads — lu depuis OMP si disponible, sinon NX48C_N_MAX_THREADS */
#ifdef _OPENMP
    #pragma omp parallel
    { c->hw_n_threads = omp_get_num_threads(); }
#else
    c->hw_n_threads = 1;
#endif
}

/* ── Initialisation ─────────────────────────────────────────────────────── */

void nx48_ctrl_init(nx48_ctrl_t *c, const char *run_id) {
    if (!c) return;
    memset(c, 0, sizeof(*c));

    strncpy(c->run_id, run_id ? run_id : "none", sizeof(c->run_id) - 1);

    /* Méta-neurones avec lr distincts (×0.5, ×1, ×2) — NX23 */
    const double lr_factors[NX48C_META_NEURONS] = {0.5, 1.0, 2.0};
    for (int k = 0; k < NX48C_META_NEURONS; k++) {
        nx48c_neuron_t *n = &c->neurons[k];
        n->lr  = NX48C_LR_DEFAULT * lr_factors[k];
        n->l1  = NX48C_L1_DEFAULT;
        n->l2  = NX48C_L2_DEFAULT;
        n->b   = 0.0;
        n->invariant_density = 1.0;   /* NX24 : début sain */
        memset(n->w,     0, sizeof(n->w));
        memset(n->alpha, 0, sizeof(n->alpha));
        memset(n->beta,  0, sizeof(n->beta));
    }

    c->best_neuron  = 0;
    c->best_f1      = 0.0;
    c->hist_head    = 0;
    c->hist_count   = 0;
    c->initialized  = true;

    /* Première lecture hardware (valeur initiale) */
    nx48_ctrl_read_hw(c);

    FORENSIC_LOG_MODULE_METRIC("nx48_adaptive", "ctrl_init",        1.0);
    FORENSIC_LOG_MODULE_METRIC("nx48_adaptive", "ctrl_n_features",  (double)NX48C_N_FEATURES);
    FORENSIC_LOG_MODULE_METRIC("nx48_adaptive", "ctrl_meta_neurons",(double)NX48C_META_NEURONS);
    FORENSIC_LOG_MODULE_METRIC("nx48_adaptive", "ctrl_hw_cpu_pct0", c->hw_cpu_pct * 100.0);
    FORENSIC_LOG_MODULE_METRIC("nx48_adaptive", "ctrl_hw_ram_pct0", c->hw_ram_pct * 100.0);
    FORENSIC_LOG_MODULE_METRIC("nx48_adaptive", "ctrl_hw_threads",  (double)c->hw_n_threads);
}

void nx48_ctrl_destroy(nx48_ctrl_t *c) {
    if (!c || !c->initialized) return;
    nx48_ctrl_log_stats(c);
    c->initialized = false;
}

/* ── Gradient physique ∂x/∂step (NX47 ARC) ─────────────────────────────── */

static void nx48c_compute_grad(nx48_ctrl_t *c,
                                const double *x_now, double *grad_out) {
    if (c->hist_count < 1) {
        memset(grad_out, 0, NX48C_N_FEATURES * sizeof(double));
        return;
    }
    int prev = (c->hist_head - 1 + NX48C_HISTORY_LEN) % NX48C_HISTORY_LEN;
    const double *x_prev = c->history[prev].x;
    for (int f = 0; f < NX48C_N_FEATURES; f++)
        grad_out[f] = x_now[f] - x_prev[f];
}

/* ── Safe log2 ─────────────────────────────────────────────────────────── */

static double safe_log2(double v) {
    return (v > 0.0) ? log2(v) : 0.0;
}

/* ── Construction d'un échantillon complet ─────────────────────────────── */

nx48c_sample_t nx48_ctrl_build_sample(
    nx48_ctrl_t *c,
    const char  *module_name,
    int          module_idx,
    double       energy_eV,
    double       pairing,
    double       sign_ratio,
    double       U_t,
    double       temp_K,
    double       temp_Kc,
    uint64_t     n_steps,
    uint64_t     n_sweeps,
    int          n_sites,
    double       bench_abs_err,
    double       elapsed_s,
    int          n_qubits,
    int          circuit_depth,
    uint64_t     n_circuits,
    double       F_xeb_rm,
    double       entropy,
    double       sign_true,
    uint64_t     step_id)
{
    nx48c_sample_t s;
    memset(&s, 0, sizeof(s));
    strncpy(s.module_name, module_name ? module_name : "unknown",
            sizeof(s.module_name) - 1);
    s.step_id = step_id;
    s.label   = (sign_true >= 0.5) ? 1.0 : 0.0;

    /* Lecture hardware réelle (throttlée à 500ms) */
    nx48_ctrl_read_hw(c);

    /* ─── Normalisation des features ────────────────────────────────── */

    /* [0] CPU % — /proc/stat — RÉEL */
    s.x[NX48F_CPU_PCT_NORM]  = c->hw_cpu_pct;

    /* [1] RAM % — /proc/meminfo — RÉEL */
    s.x[NX48F_RAM_PCT_NORM]  = c->hw_ram_pct;

    /* [2] threads actifs / max */
    s.x[NX48F_N_THREADS_NORM] =
        (NX48C_N_MAX_THREADS > 0)
        ? (double)c->hw_n_threads / NX48C_N_MAX_THREADS : 1.0;

    /* [3] énergie density */
    double n_sites_safe = (n_sites > 0) ? (double)n_sites : 1.0;
    s.x[NX48F_ENERGY_DENSITY] = energy_eV / n_sites_safe;
    /* Clamping énergétique pour éviter divergence */
    if (s.x[NX48F_ENERGY_DENSITY] > 100.0)  s.x[NX48F_ENERGY_DENSITY] = 100.0;
    if (s.x[NX48F_ENERGY_DENSITY] < -100.0) s.x[NX48F_ENERGY_DENSITY] = -100.0;

    /* [4] pairing [0, 1] */
    s.x[NX48F_PAIRING] = (pairing < 0.0) ? 0.0 : (pairing > 1.0 ? 1.0 : pairing);

    /* [5] sign_ratio — clampé [-1, 1] */
    s.x[NX48F_SIGN_RATIO] = (sign_ratio < -1.0) ? -1.0 :
                             (sign_ratio >  1.0) ?  1.0 : sign_ratio;

    /* [6] U/t normalisé (max=20) */
    s.x[NX48F_U_T_NORM] = (U_t >= 0.0) ? (U_t / 20.0) : 0.0;
    if (s.x[NX48F_U_T_NORM] > 1.0) s.x[NX48F_U_T_NORM] = 1.0;

    /* [7] T réduite T/T_c */
    double Tc = (temp_Kc > 0.0) ? temp_Kc : 150.0;   /* défaut T_c = 150K HTS */
    s.x[NX48F_TEMP_RED] = temp_K / Tc;
    if (s.x[NX48F_TEMP_RED] > 1.0) s.x[NX48F_TEMP_RED] = 1.0;
    if (s.x[NX48F_TEMP_RED] < 0.0) s.x[NX48F_TEMP_RED] = 0.0;

    /* [8] log2(n_steps) / 20 */
    s.x[NX48F_STEPS_LOG2] = safe_log2((double)(n_steps > 0 ? n_steps : 1)) / 20.0;

    /* [9] log2(n_sweeps) / 20 */
    s.x[NX48F_SWEEPS_LOG2] = safe_log2((double)(n_sweeps > 0 ? n_sweeps : 1)) / 20.0;

    /* [10] log2(n_sites) / 20 */
    s.x[NX48F_SITES_LOG2] = safe_log2(n_sites_safe) / 20.0;

    /* [11] -log10(bench_abs_err) / 9 — plus précis → valeur plus haute */
    {
        double berr = (bench_abs_err > 1e-9) ? bench_abs_err : 1e-9;
        double logberr = -log10(berr) / 9.0;
        if (logberr < 0.0) logberr = 0.0;
        if (logberr > 1.0) logberr = 1.0;
        s.x[NX48F_BENCH_ERR_LOG] = logberr;
    }

    /* [12] log(elapsed_s + 1) / 10 */
    s.x[NX48F_ELAPSED_LOG] = log(elapsed_s + 1.0) / 10.0;
    if (s.x[NX48F_ELAPSED_LOG] > 1.0) s.x[NX48F_ELAPSED_LOG] = 1.0;

    /* [13] log2(n_qubits) / 16 */
    s.x[NX48F_QUBITS_LOG2] =
        (n_qubits > 0) ? (safe_log2((double)n_qubits) / 16.0) : 0.0;

    /* [14] circuit_depth / 200 */
    s.x[NX48F_DEPTH_NORM] =
        (circuit_depth > 0) ? ((double)circuit_depth / 200.0) : 0.0;
    if (s.x[NX48F_DEPTH_NORM] > 1.0) s.x[NX48F_DEPTH_NORM] = 1.0;

    /* [15] log2(n_circuits) / 20 */
    s.x[NX48F_CIRCUITS_LOG2] =
        (n_circuits > 0) ? (safe_log2((double)n_circuits) / 20.0) : 0.0;

    /* [16] F_XEB running mean [0, 1] */
    {
        double fxeb = F_xeb_rm;
        /* F_XEB ∈ [-1, 1] → normaliser en [0, 1] */
        fxeb = (fxeb + 1.0) / 2.0;
        if (fxeb < 0.0) fxeb = 0.0;
        if (fxeb > 1.0) fxeb = 1.0;
        s.x[NX48F_F_XEB_RM] = fxeb;
    }

    /* [17] entropy density = entropy / max(n_qubits, 1) / 2 */
    {
        double qsafe = (double)(n_qubits > 0 ? n_qubits : 1);
        double ed = entropy / (qsafe * 2.0);
        if (ed < 0.0) ed = 0.0;
        if (ed > 1.0) ed = 1.0;
        s.x[NX48F_ENTROPY_DENS] = ed;
    }

    /* [18] module index / 15 */
    {
        double midx = (double)module_idx / 15.0;
        if (midx < 0.0) midx = 0.0;
        if (midx > 1.0) midx = 1.0;
        s.x[NX48F_MODULE_IDX] = midx;
    }

    /* [19] |∂E/∂step| — calculé après gradient (NX47 ARC) */
    /* Valeur provisoire : 0.0, mise à jour après compute_grad ci-dessous */
    s.x[NX48F_GRAD_ENERGY] = 0.0;

    /* Gradient ∂x/∂step (NX47 ARC) depuis historique */
    nx48c_compute_grad(c, s.x, s.grad_x);

    /* Mise à jour feature [19] avec |∂energy/∂step| */
    s.x[NX48F_GRAD_ENERGY] = fabs(s.grad_x[NX48F_ENERGY_DENSITY]);
    if (s.x[NX48F_GRAD_ENERGY] > 1.0) s.x[NX48F_GRAD_ENERGY] = 1.0;

    /* Recalcul gradient avec feature [19] mise à jour */
    s.grad_x[NX48F_GRAD_ENERGY] = 0.0;   /* auto-référence → 0 */

    /* C54-P2-NX48-SHADOW : Phase A — logging des gradients physiques (sans modification).
     * Shadow Mode = observation pure, les paramètres ne sont PAS modifiés ici.
     * Le NX48 calcule grad_x = ∂feature/∂step via l'historique circulaire (NX47 ARC).
     * Ces logs permettent de valider la sensibilité du contrôleur avant Phase B (actif).
     * Source : analysechatgpt91.21.md §PRIORITÉS C54 P2 — NX48 Phase A Shadow Mode. */
    FORENSIC_LOG_MODULE_METRIC("nx48_shadow", "grad_energy_density", s.grad_x[NX48F_ENERGY_DENSITY]);
    FORENSIC_LOG_MODULE_METRIC("nx48_shadow", "grad_sign_ratio",     s.grad_x[NX48F_SIGN_RATIO]);
    FORENSIC_LOG_MODULE_METRIC("nx48_shadow", "grad_pairing",        s.grad_x[NX48F_PAIRING]);
    FORENSIC_LOG_MODULE_METRIC("nx48_shadow", "grad_bench_err",      s.grad_x[NX48F_BENCH_ERR_LOG]);
    FORENSIC_LOG_MODULE_METRIC("nx48_shadow", "grad_f_xeb",          s.grad_x[NX48F_F_XEB_RM]);
    FORENSIC_LOG_MODULE_METRIC("nx48_shadow", "x_energy_density",    s.x[NX48F_ENERGY_DENSITY]);
    FORENSIC_LOG_MODULE_METRIC("nx48_shadow", "x_sign_ratio",        s.x[NX48F_SIGN_RATIO]);

    /* Enregistrement dans historique circulaire */
    c->history[c->hist_head] = s;
    c->hist_head = (c->hist_head + 1) % NX48C_HISTORY_LEN;
    if (c->hist_count < NX48C_HISTORY_LEN) c->hist_count++;

    return s;
}

/* ── Prédiction + recommandations adaptatives ─────────────────────────── */

/*
 * nx48_ctrl_adaptive_scale — Convertit prob_positive en échelle
 * de paramètre de simulation.
 *
 * Logique :
 *   prob ≈ 0.0 (signe très négatif)  → réduire les paramètres (×0.5-0.7)
 *   prob ≈ 0.5 (neutre)              → garder (×1.0)
 *   prob ≈ 1.0 (signe très positif)  → augmenter (×1.3-max)
 *
 * Exemple pour circuit_depth (max=2.0) :
 *   scale = clamp(0.5 + 1.5 × prob, 0.5, 2.0)
 */
static double adaptive_scale(double prob, double scale_min, double scale_max) {
    double s = scale_min + (scale_max - scale_min) * prob;
    if (s < scale_min) s = scale_min;
    if (s > scale_max) s = scale_max;
    return s;
}

nx48_ctrl_params_t nx48_ctrl_predict(nx48_ctrl_t *c,
                                      const nx48c_sample_t *s,
                                      double overhead_original) {
    nx48_ctrl_params_t p;
    memset(&p, 0, sizeof(p));

    /* Valeurs par défaut (pas d'adaptation) */
    p.circuit_depth_scale = 1.0;
    p.n_circuits_scale    = 1.0;
    p.n_steps_scale       = 1.0;
    p.n_sweeps_scale      = 1.0;
    p.skip_sign_config    = false;
    p.lr_decay_factor     = 1.0;
    p.throttle_cpu        = false;
    p.overhead_reduction  = overhead_original;

    if (!c->initialized) return p;

    const nx48c_neuron_t *best = &c->neurons[c->best_neuron];
    double z    = nx48c_forward(best, s);
    double prob = nx48c_sigmoid(z);

    /* ─── Recommandations adaptatives ─────────────────────────────── */

    /* 1. Skip config MC si signe trop faible (NX48 mitigation) */
    p.skip_sign_config = (prob < NX48C_THRESHOLD_SIGN);
    if (p.skip_sign_config) c->n_skipped++;

    /* 2. circuit_depth : prob élevé (signe positif) → profondeur plus grande */
    p.circuit_depth_scale = adaptive_scale(prob,
                                            NX48C_SCALE_MIN,
                                            NX48C_SCALE_MAX_STD);

    /* 3. n_circuits : signe positif → plus de circuits (meilleure stat) */
    p.n_circuits_scale = adaptive_scale(prob,
                                         NX48C_SCALE_MIN,
                                         NX48C_SCALE_MAX_CIRC);

    /* 4. n_steps QMC : signe positif → plus de steps (précision) */
    p.n_steps_scale = adaptive_scale(prob,
                                      NX48C_SCALE_MIN,
                                      NX48C_SCALE_MAX_STD);

    /* 4b. C58-03 : boost steps si bench_err > 0.025 (convergence insuffisante).
     * NX48F_BENCH_ERR_LOG = -log10(bench_err) / 9.
     * Seuil 0.025 ↔ logberr_normalized = -log10(0.025)/9 ≈ 0.1780.
     * Réf : analysechatgpt91.27.md §QCD_BENCH_ERR=0.029164 > 0.025.
     * Facteur prudent 1.20 : +20% steps sans explosion mémoire.
     * Réf : analysechatgpt91.28.md §C58-QCD-STEPS. */
    {
        double logberr_norm = s->x[NX48F_BENCH_ERR_LOG];
        /* bench_err = 10^(-logberr_norm * 9) ; seuil 0.025 → logberr_norm < 0.1780 */
        if (logberr_norm > 1e-9 && logberr_norm < 0.1780) {
            p.n_steps_scale *= 1.20;
            FORENSIC_LOG_MODULE_METRIC(s->module_name, "c58_steps_boost_bench_err",
                                       pow(10.0, -logberr_norm * 9.0));
        }
    }

    /* 5. n_sweeps Worm MC : similaire à steps */
    p.n_sweeps_scale = adaptive_scale(prob,
                                       NX48C_SCALE_MIN,
                                       NX48C_SCALE_MAX_STD);

    /* 6. lr_decay si signe faible (stabilisation) */
    if (prob < 0.3)       p.lr_decay_factor = 0.65;
    else if (prob < 0.5)  p.lr_decay_factor = 0.85;
    else                  p.lr_decay_factor = 1.0;

    /* 7. CPU throttle si CPU > 95% */
    p.throttle_cpu = (c->hw_cpu_pct > NX48C_CPU_THROTTLE_PCT);
    if (p.throttle_cpu) c->n_throttled++;

    /* 8. Overhead estimé réduit */
    if (c->n_seen > 0) {
        double skip_rate = (double)c->n_skipped / (double)c->n_seen;
        /* Précision approximative : 1 - skip_rate × (1 - prob) */
        double precision = (skip_rate < 1.0) ? (1.0 - skip_rate * (1.0 - prob)) : 0.01;
        if (precision < 1e-9) precision = 1e-9;
        double oh_reduced = overhead_original * (1.0 - skip_rate) / precision;
        p.overhead_reduction = oh_reduced;
        c->overhead_total_reduced += oh_reduced;
    }

    /* ─── C56 — Phase B : 5 paramètres dynamiques additionnels ───────────
     *
     * 9. n_sites_scale : prob élevé (signe positif, bon signal) → augmenter les sites.
     *    Plage [1.0, 1.5] — prudent pour éviter explosion RAM sur 14×14.
     *    Basé sur NX48F_SITES_LOG2 : si log2(n_sites)/20 < 0.4 → encore marge de montée. */
    {
        double sites_headroom = 1.0 - s->x[NX48F_SITES_LOG2]; /* 0 si sites max, 1 si petit */
        p.n_sites_scale = 1.0 + (adaptive_scale(prob, 0.0, 0.5) * sites_headroom);
        if (p.n_sites_scale > 1.5) p.n_sites_scale = 1.5;
        if (p.n_sites_scale < 1.0) p.n_sites_scale = 1.0;
    }

    /* 10. n_replicas_scale : prob faible (signe problématique) → plus de répliques PT-MC.
     *     Raisonnement : plus de répliques = meilleure couverture de l'espace T → aide le signe.
     *     Plage [1.0, 2.0] — échange répliques contre précision. */
    p.n_replicas_scale = adaptive_scale(1.0 - prob, 1.0, 2.0);
    if (p.n_replicas_scale < 1.0) p.n_replicas_scale = 1.0;
    if (p.n_replicas_scale > 2.0) p.n_replicas_scale = 2.0;

    /* 11. temp_K_scale : moduler la température selon la convergence du signe.
     *     C60-BENCHFIX : Borne stricte ±3% pour préserver la calibration benchmark.
     *     Raison empirique : C59/C60 démontrent que scale=0.94 (−6%) → RMSE ×4.
     *     Les benchmarks (qmc_dmrg_reference_runtime.csv) sont définis aux T_base
     *     invariantes. Tout écart > ±3% décale les points d'opération hors référence.
     *     Plage réduite [0.97, 1.03] : adaptation thermique minimale sûre.
     *     Ref : analysechatgpt91.29.md §6.1 CAUSE PRINCIPALE RÉGRESSION RMSE C59/C60.
     *     Chaque module reçoit son scale indépendant via nx48_ctrl_predict() → CSV. */
    if      (prob < 0.3) p.temp_K_scale = 0.97;
    else if (prob < 0.5) p.temp_K_scale = 0.99;
    else if (prob < 0.7) p.temp_K_scale = 1.00;
    else                 p.temp_K_scale = 1.02;

    /* 12. U_eV_scale : moduler U selon le ratio U/t (feature NX48F_U_T_NORM = U/t / 20).
     *     U/t élevé (feature > 0.5 → U/t > 10) → légère réduction U pour améliorer signe.
     *     U/t faible → légère hausse U pour explorer le gap de Mott.
     *     Plage [0.90, 1.10]. */
    {
        double u_t_feat = s->x[NX48F_U_T_NORM]; /* ∈ [0, 1] */
        p.U_eV_scale = 1.0 + 0.10 * (0.5 - u_t_feat); /* 0.95→1.05 selon U/t */
        if (p.U_eV_scale < 0.90) p.U_eV_scale = 0.90;
        if (p.U_eV_scale > 1.10) p.U_eV_scale = 1.10;
    }

    /* 13. t_eV_scale : moduler t selon la stabilité de l'énergie (gradient NX47 ARC).
     *     Gradient élevé (feature NX48F_GRAD_ENERGY > 0.5) → instabilité → réduire t.
     *     Gradient faible (convergé) → monter légèrement t pour explorer.
     *     Plage [0.90, 1.10]. */
    {
        double grad_feat = s->x[NX48F_GRAD_ENERGY]; /* ∈ [0, 1] normalisé */
        p.t_eV_scale = 1.0 - 0.10 * grad_feat;     /* 0.90 si instable, 1.00 si stable */
        if (p.t_eV_scale < 0.90) p.t_eV_scale = 0.90;
        if (p.t_eV_scale > 1.10) p.t_eV_scale = 1.10;
    }

    /* Log forensique des nouveaux scales C56 */
    FORENSIC_LOG_MODULE_METRIC("nx48_adaptive", "c56_n_sites_scale",    p.n_sites_scale);
    FORENSIC_LOG_MODULE_METRIC("nx48_adaptive", "c56_n_replicas_scale", p.n_replicas_scale);
    FORENSIC_LOG_MODULE_METRIC("nx48_adaptive", "c56_temp_K_scale",     p.temp_K_scale);
    FORENSIC_LOG_MODULE_METRIC("nx48_adaptive", "c56_U_eV_scale",       p.U_eV_scale);
    FORENSIC_LOG_MODULE_METRIC("nx48_adaptive", "c56_t_eV_scale",       p.t_eV_scale);

    /* ─── C57 — Phase B : 3 paramètres supplémentaires ──────────────────
     * 14. dt_scale : moduler le pas de temps selon la stabilité de l'intégrateur.
     *     Gradient élevé (instabilité) → réduire dt pour stabilité RK2.
     *     Convergence correcte → augmenter légèrement dt pour accélérer.
     *     Plage [0.5, 1.5] — borné par stabilité RK2 (Δt × λ_max < 0.2). */
    {
        double grad_feat = s->x[NX48F_GRAD_ENERGY]; /* ∈ [0,1] normalisé */
        p.dt_scale = 1.0 - 0.50 * grad_feat;        /* 0.5 si instable, 1.0 si stable */
        if (p.dt_scale < 0.5) p.dt_scale = 0.5;
        if (p.dt_scale > 1.5) p.dt_scale = 1.5;
    }

    /* 15. mu_eV_scale : moduler le doping dynamique selon le taux de remplissage.
     *     Signe positif fort (prob ≥ 0.6) + pairing élevé → léger doping (μ > 0).
     *     Signe faible (prob < 0.3) → réduire le doping (retour à demi-remplissage).
     *     Plage [0.8, 1.2] — μ=0 au demi-remplissage est physiquement stable. */
    {
        double pairing_feat = s->x[NX48F_PAIRING]; /* ∈ [0,1] */
        p.mu_eV_scale = 1.0 + 0.20 * (prob - 0.5) * pairing_feat;
        if (p.mu_eV_scale < 0.8) p.mu_eV_scale = 0.8;
        if (p.mu_eV_scale > 1.2) p.mu_eV_scale = 1.2;
    }

    /* 16. T_ratio_scale : rapport T_max/T_min du PT-MC (actuellement 50.0 fixe → adaptatif).
     *     prob faible (mauvais signe) → augmenter T_ratio pour explorer plus grand espace T.
     *     prob élevé (bon signe) → réduire T_ratio (se concentrer autour de T_c).
     *     Plage [0.7, 1.5] — T_ratio_effective = 50.0 × T_ratio_scale. */
    p.T_ratio_scale = adaptive_scale(1.0 - prob, 0.7, 1.5);
    if (p.T_ratio_scale < 0.7) p.T_ratio_scale = 0.7;
    if (p.T_ratio_scale > 1.5) p.T_ratio_scale = 1.5;

    /* Log forensique des 3 nouveaux scales C57 (STANDARD_NAMES.md §M-C57) */
    FORENSIC_LOG_MODULE_METRIC("nx48_adaptive", "c57_dt_scale",       p.dt_scale);
    FORENSIC_LOG_MODULE_METRIC("nx48_adaptive", "c57_mu_eV_scale",    p.mu_eV_scale);
    FORENSIC_LOG_MODULE_METRIC("nx48_adaptive", "c57_T_ratio_scale",  p.T_ratio_scale);

    return p;
}

/* ── Mise à jour en ligne (ISTA, 1 sample) ─────────────────────────────── */

double nx48_ctrl_update(nx48_ctrl_t *c, const nx48c_sample_t *s) {
    if (!c->initialized) return 0.0;

    nx48c_neuron_t *n = &c->neurons[c->best_neuron];
    double z    = nx48c_forward(n, s);
    double prob = nx48c_sigmoid(z);
    double err  = prob - s->label;
    double bce  = -(s->label * log(fmax(prob, 1e-9))
                  + (1.0 - s->label) * log(fmax(1.0 - prob, 1e-9)));

    /* ISTA : grad BCE + L2, puis seuillage proximal L1 */
    for (int f = 0; f < NX48C_N_FEATURES; f++) {
        double xf  = s->x[f];
        double gxf = s->grad_x[f];

        double gw     = err * xf      + n->l2 * n->w[f];
        double galpha = err * xf*xf   + n->l2 * n->alpha[f];
        double gbeta  = err * gxf     + n->l2 * n->beta[f];

        n->w[f]     = nx48c_prox_l1(n->w[f]     - n->lr * gw,     n->lr * n->l1);
        n->alpha[f] = nx48c_prox_l1(n->alpha[f] - n->lr * galpha, n->lr * n->l1);
        n->beta[f]  = nx48c_prox_l1(n->beta[f]  - n->lr * gbeta,  n->lr * n->l1);
    }
    n->b -= n->lr * err;

    /* Mise à jour densité invariante (NX24) :
     * invariant_density = fraction de poids non-nuls (sparsité inverse).
     * Si < NX48C_DENSITY_COLLAPSE → neurone "effondré" → réinit.    */
    {
        int n_nonzero = 0;
        for (int f = 0; f < NX48C_N_FEATURES; f++) {
            if (fabs(n->w[f]) > 1e-12) n_nonzero++;
        }
        n->invariant_density = (double)n_nonzero / (double)NX48C_N_FEATURES;
        if (n->invariant_density < NX48C_DENSITY_COLLAPSE) {
            /* Réinitialisation du neurone (NX24 — résurrection) */
            for (int f = 0; f < NX48C_N_FEATURES; f++) {
                n->w[f] = 0.0; n->alpha[f] = 0.0; n->beta[f] = 0.0;
            }
            n->b = 0.0;
            n->invariant_density = 1.0;
            FORENSIC_LOG_MODULE_METRIC("nx48_adaptive", "neuron_collapse_reset", 1.0);
        }
    }

    c->n_seen++;
    c->sum_loss += bce;
    return bce;
}

/* ── Entraînement batch ─────────────────────────────────────────────────── */

static double nx48c_f1_proxy(const nx48c_neuron_t *n,
                               const nx48c_sample_t *samples, int n_s) {
    int tp = 0, fp = 0, fn = 0;
    for (int i = 0; i < n_s; i++) {
        double prob  = nx48c_sigmoid(nx48c_forward(n, &samples[i]));
        int pred     = (prob  >= 0.5) ? 1 : 0;
        int truth    = (samples[i].label >= 0.5) ? 1 : 0;
        if (pred == 1 && truth == 1) tp++;
        else if (pred == 1 && truth == 0) fp++;
        else if (pred == 0 && truth == 1) fn++;
    }
    int den = 2*tp + fp + fn;
    return (den > 0) ? (2.0*tp / den) : 0.0;
}

static void nx48c_fit_one(nx48c_neuron_t *n,
                           const nx48c_sample_t *samples, int n_samp) {
    if (n_samp <= 0) return;
    for (int iter = 0; iter < NX48C_MAX_ITER; iter++) {
        double gw    [NX48C_N_FEATURES] = {0};
        double galpha[NX48C_N_FEATURES] = {0};
        double gbeta [NX48C_N_FEATURES] = {0};
        double gb = 0.0;
        for (int i = 0; i < n_samp; i++) {
            const nx48c_sample_t *s = &samples[i];
            double p = nx48c_sigmoid(nx48c_forward(n, s));
            double e = p - s->label;
            for (int f = 0; f < NX48C_N_FEATURES; f++) {
                gw[f]     += e * s->x[f];
                galpha[f] += e * s->x[f] * s->x[f];
                gbeta[f]  += e * s->grad_x[f];
            }
            gb += e;
        }
        double inv_n = 1.0 / n_samp;
        for (int f = 0; f < NX48C_N_FEATURES; f++) {
            n->w[f]     = nx48c_prox_l1(n->w[f]     - n->lr*(gw[f]*inv_n     + n->l2*n->w[f]),     n->lr*n->l1);
            n->alpha[f] = nx48c_prox_l1(n->alpha[f] - n->lr*(galpha[f]*inv_n + n->l2*n->alpha[f]), n->lr*n->l1);
            n->beta[f]  = nx48c_prox_l1(n->beta[f]  - n->lr*(gbeta[f]*inv_n  + n->l2*n->beta[f]),  n->lr*n->l1);
        }
        n->b -= n->lr * (gb * inv_n);
    }
}

void nx48_ctrl_fit(nx48_ctrl_t *c) {
    if (!c->initialized || c->hist_count <= 0) return;

    int n_s = c->hist_count;   /* Entraîne sur tout l'historique */

    uint64_t t0 = nx48c_now_ns();

    for (int k = 0; k < NX48C_META_NEURONS; k++)
        nx48c_fit_one(&c->neurons[k], c->history, n_s);

    /* Sélection du meilleur méta-neurone par F1 proxy */
    double best_f1 = -1.0;
    int    best_k  = 0;
    double f1_vals[NX48C_META_NEURONS];
    for (int k = 0; k < NX48C_META_NEURONS; k++) {
        f1_vals[k] = nx48c_f1_proxy(&c->neurons[k], c->history, n_s);
        if (f1_vals[k] > best_f1) { best_f1 = f1_vals[k]; best_k = k; }
    }
    c->best_neuron = best_k;
    c->best_f1     = best_f1;

    /* Stagnation (NX30) : fenêtre F1 */
    c->f1_history[c->f1_hist_idx] = best_f1;
    c->f1_hist_idx = (c->f1_hist_idx + 1) % NX48C_STAGNATION_WIN;

    if (c->n_seen > (uint64_t)(NX48C_STAGNATION_WIN * n_s)) {
        double f1min = c->f1_history[0], f1max = c->f1_history[0];
        for (int i = 1; i < NX48C_STAGNATION_WIN; i++) {
            if (c->f1_history[i] < f1min) f1min = c->f1_history[i];
            if (c->f1_history[i] > f1max) f1max = c->f1_history[i];
        }
        if ((f1max - f1min) < 0.001) {
            for (int k = 0; k < NX48C_META_NEURONS; k++)
                c->neurons[k].lr *= 0.65;
            FORENSIC_LOG_MODULE_METRIC("nx48_adaptive", "stagnation_lr_decay", 0.65);
        }
    }

    uint64_t elapsed = nx48c_now_ns() - t0;
    FORENSIC_LOG_MODULE_METRIC("nx48_adaptive", "fit_n_samples",     (double)n_s);
    FORENSIC_LOG_MODULE_METRIC("nx48_adaptive", "fit_best_f1",       best_f1);
    FORENSIC_LOG_MODULE_METRIC("nx48_adaptive", "fit_best_neuron",   (double)best_k);
    FORENSIC_LOG_MODULE_METRIC("nx48_adaptive", "fit_elapsed_ns",    (double)elapsed);
    for (int k = 0; k < NX48C_META_NEURONS; k++)
        FORENSIC_LOG_MODULE_METRIC("nx48_adaptive", "fit_f1_meta_k", f1_vals[k]);
}

/* ── Logging forensique complet ─────────────────────────────────────────── */

void nx48_ctrl_log_stats(const nx48_ctrl_t *c) {
    if (!c) return;

    double skip_rate = (c->n_seen > 0)
        ? (double)c->n_skipped / (double)c->n_seen : 0.0;
    double avg_loss  = (c->n_seen > 0)
        ? c->sum_loss / (double)c->n_seen : 0.0;
    double throttle_rate = (c->n_seen > 0)
        ? (double)c->n_throttled / (double)c->n_seen : 0.0;

    FORENSIC_LOG_MODULE_METRIC("nx48_adaptive", "ctrl_n_seen",          (double)c->n_seen);
    FORENSIC_LOG_MODULE_METRIC("nx48_adaptive", "ctrl_n_skipped",       (double)c->n_skipped);
    FORENSIC_LOG_MODULE_METRIC("nx48_adaptive", "ctrl_skip_rate",       skip_rate);
    FORENSIC_LOG_MODULE_METRIC("nx48_adaptive", "ctrl_n_throttled",     (double)c->n_throttled);
    FORENSIC_LOG_MODULE_METRIC("nx48_adaptive", "ctrl_throttle_rate",   throttle_rate);
    FORENSIC_LOG_MODULE_METRIC("nx48_adaptive", "ctrl_avg_bce_loss",    avg_loss);
    FORENSIC_LOG_MODULE_METRIC("nx48_adaptive", "ctrl_best_f1",         c->best_f1);
    FORENSIC_LOG_MODULE_METRIC("nx48_adaptive", "ctrl_best_neuron_idx", (double)c->best_neuron);
    FORENSIC_LOG_MODULE_METRIC("nx48_adaptive", "ctrl_overhead_red",    c->overhead_total_reduced);

    /* Hardware actuel */
    FORENSIC_LOG_MODULE_METRIC("nx48_adaptive", "ctrl_hw_cpu_pct",   c->hw_cpu_pct * 100.0);
    FORENSIC_LOG_MODULE_METRIC("nx48_adaptive", "ctrl_hw_ram_pct",   c->hw_ram_pct * 100.0);
    FORENSIC_LOG_MODULE_METRIC("nx48_adaptive", "ctrl_hw_n_threads", (double)c->hw_n_threads);

    /* Sparsité des poids du meilleur neurone */
    const nx48c_neuron_t *best = &c->neurons[c->best_neuron];
    int n_zero = 0;
    for (int f = 0; f < NX48C_N_FEATURES; f++) {
        if (fabs(best->w[f])     < 1e-12) n_zero++;
        if (fabs(best->alpha[f]) < 1e-12) n_zero++;
        if (fabs(best->beta[f])  < 1e-12) n_zero++;
    }
    double sparsity = (double)n_zero / (3.0 * NX48C_N_FEATURES);
    FORENSIC_LOG_MODULE_METRIC("nx48_adaptive", "ctrl_weight_sparsity",  sparsity);
    FORENSIC_LOG_MODULE_METRIC("nx48_adaptive", "ctrl_best_neuron_lr",   best->lr);
    FORENSIC_LOG_MODULE_METRIC("nx48_adaptive", "ctrl_best_neuron_bias", best->b);
    FORENSIC_LOG_MODULE_METRIC("nx48_adaptive", "ctrl_inv_density",      best->invariant_density);

    /* Poids par feature (pour audit forensique) */
    for (int f = 0; f < NX48C_N_FEATURES; f++) {
        FORENSIC_LOG_MODULE_METRIC("nx48_adaptive", "ctrl_w_feature",    best->w[f]);
        FORENSIC_LOG_MODULE_METRIC("nx48_adaptive", "ctrl_alpha_feature",best->alpha[f]);
        FORENSIC_LOG_MODULE_METRIC("nx48_adaptive", "ctrl_beta_feature", best->beta[f]);
    }
}
