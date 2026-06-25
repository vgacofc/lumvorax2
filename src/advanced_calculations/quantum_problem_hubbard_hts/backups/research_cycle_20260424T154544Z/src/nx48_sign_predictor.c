/*
 * nx48_sign_predictor.c — Prédicteur de signe QMC NX47AtomNeuron (NX48)
 *
 * Implémentation C natif de NX47AtomNeuron pour la mitigation du problème
 * du signe Monte Carlo dans LumVorax / Hubbard HTS.
 *
 * Algorithme ISTA (Iterative Shrinkage-Thresholding Algorithm) :
 *   Pour chaque paramètre θ ∈ {w, alpha, beta} :
 *     grad_θ = xᵀ·err / n + l2·θ        (gradient BCE + L2)
 *     θ_new  = prox_L1(θ - lr·grad_θ)   (seuillage doux L1)
 *   où prox_L1(v, λ) = sign(v) × max(|v| - λ, 0)
 *
 * Méta-neurones compétiteurs (NX48_META_NEURONS = 3) :
 *   - 3 neurones avec taux d'apprentissage distincts (×1, ×0.5, ×2)
 *   - Le meilleur (F1 proxy maximal) est sélectionné après chaque batch
 *   - En cas de stagnation (spread F1 < 0.001 sur 5 epochs) : lr × 0.65
 *
 * Intégration forensique LumVorax :
 *   Toutes les métriques loggées via FORENSIC_LOG_MODULE_METRIC("nx48_sign")
 *   Noms conformes STANDARD_NAMES.md v3.7 — Section L (C55)
 *
 * C55 — 2026-04-09 — Module nouveau (importé depuis src/nx_versions/nx48_unified_ultra)
 */

#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include "nx48_sign_predictor.h"
#include "../../../debug/ultra_forensic_logger.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <stdarg.h>

/* ── Utilitaires timestamp ─────────────────────────────────────────── */

static uint64_t nx48_now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

/* ── Fonctions mathématiques ───────────────────────────────────────── */

/*
 * nx48_sigmoid — Fonction sigmoïde stable numériquement.
 * Évite l'overflow pour |z| > 700.
 */
double nx48_sigmoid(double z) {
    if (z >= 0.0) {
        double e = exp(-z);
        return 1.0 / (1.0 + e);
    } else {
        double e = exp(z);
        return e / (1.0 + e);
    }
}

/*
 * nx48_prox_l1 — Opérateur proximal L1 (seuillage doux).
 * prox_L1(w, λ) = sign(w) × max(|w| - λ, 0)
 * Produit la sparsité naturelle des poids.
 */
double nx48_prox_l1(double w, double lr_l1) {
    if (w > lr_l1)       return w - lr_l1;
    else if (w < -lr_l1) return w + lr_l1;
    else                  return 0.0;
}

/*
 * nx48_forward — Calcul du logit z pour un neurone et un échantillon.
 * z = x·w + x²·alpha + ∇x·beta + b
 */
double nx48_forward(const nx48_neuron_t *n, const nx48_sample_t *s) {
    double z = n->b;
    for (int f = 0; f < NX48_N_FEATURES; f++) {
        double xf    = s->x[f];
        double gxf   = s->grad_x[f];
        z += xf    * n->w[f];
        z += xf*xf * n->alpha[f];
        z += gxf   * n->beta[f];
    }
    return z;
}

/*
 * nx48_bce_loss — Perte Binary Cross-Entropy clippée.
 * BCE = -(y·log(p) + (1-y)·log(1-p))
 */
double nx48_bce_loss(double prob, double label) {
    const double eps = 1e-9;
    double p = prob  < eps ? eps : (prob  > 1.0-eps ? 1.0-eps : prob);
    return -(label * log(p) + (1.0 - label) * log(1.0 - p));
}

/*
 * nx48_f1_proxy — F1 proxy sur un ensemble d'échantillons.
 * F1 ≈ 2·TP / (2·TP + FP + FN) avec seuil 0.5.
 */
double nx48_f1_proxy(const nx48_neuron_t *n,
                     const nx48_sample_t *samples, int n_samples) {
    int tp = 0, fp = 0, fn = 0;
    for (int i = 0; i < n_samples; i++) {
        double prob = nx48_sigmoid(nx48_forward(n, &samples[i]));
        int pred  = (prob >= 0.5) ? 1 : 0;
        int truth = (samples[i].label >= 0.5) ? 1 : 0;
        if (pred == 1 && truth == 1) tp++;
        else if (pred == 1 && truth == 0) fp++;
        else if (pred == 0 && truth == 1) fn++;
    }
    int denom = 2*tp + fp + fn;
    return (denom > 0) ? (2.0 * tp / denom) : 0.0;
}

/* ── Initialisation ────────────────────────────────────────────────── */

void nx48_model_init(nx48_model_t *m, const char *module_name, const char *run_id) {
    if (!m) return;
    memset(m, 0, sizeof(*m));

    strncpy(m->module_name, module_name ? module_name : "unknown", sizeof(m->module_name)-1);
    strncpy(m->run_id,      run_id      ? run_id      : "none",    sizeof(m->run_id)-1);

    /* Initialiser les méta-neurones avec lr distincts (×0.5, ×1, ×2) */
    const double lr_factors[NX48_META_NEURONS] = {0.5, 1.0, 2.0};
    for (int k = 0; k < NX48_META_NEURONS; k++) {
        nx48_neuron_t *n = &m->neurons[k];
        n->lr  = NX48_LR_DEFAULT * lr_factors[k];
        n->l1  = NX48_L1_DEFAULT;
        n->l2  = NX48_L2_DEFAULT;
        n->b   = 0.0;
        for (int f = 0; f < NX48_N_FEATURES; f++) {
            n->w[f]     = 0.0;
            n->alpha[f] = 0.0;
            n->beta[f]  = 0.0;
        }
    }

    m->best_neuron  = 0;
    m->best_f1      = 0.0;
    m->hist_head    = 0;
    m->hist_count   = 0;
    m->initialized  = true;

    FORENSIC_LOG_MODULE_METRIC("nx48_sign", "init", 1.0);
    FORENSIC_LOG_MODULE_METRIC("nx48_sign", "n_features", (double)NX48_N_FEATURES);
    FORENSIC_LOG_MODULE_METRIC("nx48_sign", "meta_neurons", (double)NX48_META_NEURONS);
    FORENSIC_LOG_MODULE_METRIC("nx48_sign", "threshold", NX48_THRESHOLD_SIGN);
}

void nx48_model_destroy(nx48_model_t *m) {
    if (!m || !m->initialized) return;
    nx48_log_stats(m);
    m->initialized = false;
}

/* ── Construction d'échantillon ────────────────────────────────────── */

/*
 * nx48_compute_grad — Calcule le gradient numérique ∂x/∂step via
 * différences finies sur le dernier point de l'historique.
 * grad_x[f] = (x[f]_now - x[f]_prev) / 1  (pas=1 step MC)
 */
static void nx48_compute_grad(nx48_model_t *m,
                               const double *x_now, double *grad_x_out) {
    if (m->hist_count < 1) {
        memset(grad_x_out, 0, NX48_N_FEATURES * sizeof(double));
        return;
    }
    int prev_idx = (m->hist_head - 1 + NX48_HISTORY_LEN) % NX48_HISTORY_LEN;
    const double *x_prev = m->history[prev_idx].x;
    for (int f = 0; f < NX48_N_FEATURES; f++) {
        grad_x_out[f] = x_now[f] - x_prev[f];
    }
}

nx48_sample_t nx48_build_sample(nx48_model_t *m,
                                 double energy, double pairing,
                                 double sign_local, double U_t,
                                 double temp_red, double sign_true,
                                 uint64_t step_id) {
    nx48_sample_t s;
    memset(&s, 0, sizeof(s));

    /* Features [0..5] : energy_density, pairing, sign_local, U_t, temp_red, |grad_E| */
    s.x[0] = energy;
    s.x[1] = pairing;
    s.x[2] = sign_local;
    s.x[3] = U_t;
    s.x[4] = temp_red;
    /* x[5] = |∂energy/∂step| — calculé après grad */

    /* Calcul du gradient numérique */
    nx48_compute_grad(m, s.x, s.grad_x);
    s.x[5] = fabs(s.grad_x[0]);   /* |∂E/∂step| comme 6ème feature */

    s.label   = (sign_true >= 0.5) ? 1.0 : 0.0;
    s.step_id = step_id;

    /* Mettre à jour l'historique circulaire */
    m->history[m->hist_head] = s;
    m->hist_head = (m->hist_head + 1) % NX48_HISTORY_LEN;
    if (m->hist_count < NX48_HISTORY_LEN) m->hist_count++;

    return s;
}

/* ── Inférence ─────────────────────────────────────────────────────── */

nx48_prediction_t nx48_predict(const nx48_model_t *m, const nx48_sample_t *s) {
    nx48_prediction_t pred;
    memset(&pred, 0, sizeof(pred));

    if (!m->initialized) {
        pred.prob_positive = 0.5;
        pred.skip_config   = false;
        return pred;
    }

    const nx48_neuron_t *best = &m->neurons[m->best_neuron];
    double z    = nx48_forward(best, s);
    double prob = nx48_sigmoid(z);

    pred.z_logit       = z;
    pred.prob_positive = prob;
    pred.skip_config   = (prob < NX48_THRESHOLD_SIGN);
    pred.neuron_used   = (uint32_t)m->best_neuron;

    return pred;
}

/* ── Mise à jour en ligne (1 échantillon) ─────────────────────────── */

double nx48_update_online(nx48_model_t *m, const nx48_sample_t *s) {
    if (!m->initialized) return 0.0;

    nx48_neuron_t *n = &m->neurons[m->best_neuron];
    double z    = nx48_forward(n, s);
    double prob = nx48_sigmoid(z);
    double err  = prob - s->label;        /* BCE gradient : p - y */
    double loss = nx48_bce_loss(prob, s->label);

    /* Mise à jour ISTA pour w, alpha, beta */
    for (int f = 0; f < NX48_N_FEATURES; f++) {
        double xf  = s->x[f];
        double gxf = s->grad_x[f];

        /* Gradient BCE + L2 */
        double gw     = err * xf         + n->l2 * n->w[f];
        double galpha = err * xf * xf    + n->l2 * n->alpha[f];
        double gbeta  = err * gxf        + n->l2 * n->beta[f];

        /* Descente de gradient */
        double w_mid     = n->w[f]     - n->lr * gw;
        double alpha_mid = n->alpha[f] - n->lr * galpha;
        double beta_mid  = n->beta[f]  - n->lr * gbeta;

        /* Seuillage proximal L1 */
        n->w[f]     = nx48_prox_l1(w_mid,     n->lr * n->l1);
        n->alpha[f] = nx48_prox_l1(alpha_mid, n->lr * n->l1);
        n->beta[f]  = nx48_prox_l1(beta_mid,  n->lr * n->l1);
    }
    /* Biais — pas de régularisation L1 sur le biais */
    n->b -= n->lr * err;

    m->n_samples_seen++;
    m->sum_loss += loss;
    if (prob >= 0.5) m->n_positive_pred++;

    return loss;
}

/* ── Entraînement batch (NX48_MAX_ITER itérations ISTA) ───────────── */

static void nx48_fit_one_neuron(nx48_neuron_t *n,
                                 const nx48_sample_t *samples, int n_samp) {
    if (n_samp <= 0) return;

    for (int iter = 0; iter < NX48_MAX_ITER; iter++) {
        /* Accumulation des gradients sur tout le batch */
        double grad_w[NX48_N_FEATURES]     = {0};
        double grad_alpha[NX48_N_FEATURES] = {0};
        double grad_beta[NX48_N_FEATURES]  = {0};
        double grad_b = 0.0;

        for (int i = 0; i < n_samp; i++) {
            const nx48_sample_t *s = &samples[i];
            double z   = nx48_forward(n, s);
            double p   = nx48_sigmoid(z);
            double err = p - s->label;

            for (int f = 0; f < NX48_N_FEATURES; f++) {
                double xf  = s->x[f];
                double gxf = s->grad_x[f];
                grad_w[f]     += err * xf;
                grad_alpha[f] += err * xf * xf;
                grad_beta[f]  += err * gxf;
            }
            grad_b += err;
        }

        double inv_n = 1.0 / n_samp;

        /* Application ISTA : descente + seuillage proximal */
        for (int f = 0; f < NX48_N_FEATURES; f++) {
            double gw     = grad_w[f]     * inv_n + n->l2 * n->w[f];
            double galpha = grad_alpha[f] * inv_n + n->l2 * n->alpha[f];
            double gbeta  = grad_beta[f]  * inv_n + n->l2 * n->beta[f];

            n->w[f]     = nx48_prox_l1(n->w[f]     - n->lr * gw,
                                        n->lr * n->l1);
            n->alpha[f] = nx48_prox_l1(n->alpha[f] - n->lr * galpha,
                                        n->lr * n->l1);
            n->beta[f]  = nx48_prox_l1(n->beta[f]  - n->lr * gbeta,
                                        n->lr * n->l1);
        }
        n->b -= n->lr * (grad_b * inv_n);
    }
}

void nx48_fit_batch(nx48_model_t *m,
                    const nx48_sample_t *samples, int n_samples) {
    if (!m->initialized || n_samples <= 0) return;

    uint64_t t0 = nx48_now_ns();

    /* Entraîner chaque méta-neurone */
    for (int k = 0; k < NX48_META_NEURONS; k++) {
        nx48_fit_one_neuron(&m->neurons[k], samples, n_samples);
    }

    /* Sélectionner le meilleur méta-neurone par F1 proxy */
    double best_f1 = -1.0;
    int    best_k  = 0;
    double f1_vals[NX48_META_NEURONS];
    for (int k = 0; k < NX48_META_NEURONS; k++) {
        f1_vals[k] = nx48_f1_proxy(&m->neurons[k], samples, n_samples);
        if (f1_vals[k] > best_f1) {
            best_f1 = f1_vals[k];
            best_k  = k;
        }
    }
    m->best_neuron = best_k;
    m->best_f1     = best_f1;

    /* Détection stagnation — réduire lr si spread F1 < 0.001 */
    m->f1_history[m->f1_hist_idx] = best_f1;
    m->f1_hist_idx = (m->f1_hist_idx + 1) % NX48_STAGNATION_WIN;

    if (m->n_samples_seen > (uint64_t)(NX48_STAGNATION_WIN * n_samples)) {
        double f1_min = m->f1_history[0], f1_max = m->f1_history[0];
        for (int i = 1; i < NX48_STAGNATION_WIN; i++) {
            if (m->f1_history[i] < f1_min) f1_min = m->f1_history[i];
            if (m->f1_history[i] > f1_max) f1_max = m->f1_history[i];
        }
        if ((f1_max - f1_min) < 0.001) {
            /* Stagnation détectée : réduire lr × 0.65 (facteur NX47 V144.3) */
            for (int k = 0; k < NX48_META_NEURONS; k++) {
                m->neurons[k].lr *= 0.65;
            }
            FORENSIC_LOG_MODULE_METRIC("nx48_sign", "stagnation_lr_decay", 0.65);
        }
    }

    uint64_t elapsed = nx48_now_ns() - t0;
    FORENSIC_LOG_MODULE_METRIC("nx48_sign", "fit_batch_samples",    (double)n_samples);
    FORENSIC_LOG_MODULE_METRIC("nx48_sign", "fit_batch_best_f1",    best_f1);
    FORENSIC_LOG_MODULE_METRIC("nx48_sign", "fit_batch_best_neuron",(double)best_k);
    FORENSIC_LOG_MODULE_METRIC("nx48_sign", "fit_batch_elapsed_ns", (double)elapsed);
    for (int k = 0; k < NX48_META_NEURONS; k++) {
        FORENSIC_LOG_MODULE_METRIC("nx48_sign", "fit_batch_f1_k0_to_k2",
                                   f1_vals[k]);
    }
}

/* ── Sauvegarde / Chargement des poids ────────────────────────────── */

int nx48_save_weights(const nx48_model_t *m, const char *path) {
    if (!m || !path) return -1;
    FILE *fp = fopen(path, "w");
    if (!fp) return -1;

    fprintf(fp, "# NX48 weights — module=%s run=%s\n",
            m->module_name, m->run_id);
    fprintf(fp, "best_neuron %d\n", m->best_neuron);

    for (int k = 0; k < NX48_META_NEURONS; k++) {
        const nx48_neuron_t *n = &m->neurons[k];
        fprintf(fp, "neuron %d\n", k);
        fprintf(fp, "lr %.17g\n", n->lr);
        for (int f = 0; f < NX48_N_FEATURES; f++)
            fprintf(fp, "w%d %.17g\n", f, n->w[f]);
        for (int f = 0; f < NX48_N_FEATURES; f++)
            fprintf(fp, "alpha%d %.17g\n", f, n->alpha[f]);
        for (int f = 0; f < NX48_N_FEATURES; f++)
            fprintf(fp, "beta%d %.17g\n", f, n->beta[f]);
        fprintf(fp, "b %.17g\n", n->b);
    }
    fclose(fp);
    return 0;
}

int nx48_load_weights(nx48_model_t *m, const char *path) {
    if (!m || !path) return -1;
    FILE *fp = fopen(path, "r");
    if (!fp) return -1;

    char key[64];
    double val;
    int cur_k = 0;

    while (fscanf(fp, "%63s", key) == 1) {
        if (key[0] == '#') { char buf[256]; if (fgets(buf,256,fp)){} continue; }
        if (strcmp(key, "best_neuron") == 0 && fscanf(fp, "%d", &m->best_neuron)==1) continue;
        if (strcmp(key, "neuron")      == 0 && fscanf(fp, "%d", &cur_k)==1) continue;
        if (strcmp(key, "lr")          == 0 && fscanf(fp, "%lf", &val)==1) {
            m->neurons[cur_k].lr = val; continue;
        }
        if (strcmp(key, "b")           == 0 && fscanf(fp, "%lf", &val)==1) {
            m->neurons[cur_k].b = val; continue;
        }
        /* w0..w5, alpha0..alpha5, beta0..beta5 */
        for (int f = 0; f < NX48_N_FEATURES; f++) {
            char kw[16], ka[16], kb[16];
            snprintf(kw, sizeof(kw), "w%d", f);
            snprintf(ka, sizeof(ka), "alpha%d", f);
            snprintf(kb, sizeof(kb), "beta%d", f);
            if (strcmp(key, kw)==0 && fscanf(fp, "%lf", &val)==1) { m->neurons[cur_k].w[f]     = val; break; }
            if (strcmp(key, ka)==0 && fscanf(fp, "%lf", &val)==1) { m->neurons[cur_k].alpha[f] = val; break; }
            if (strcmp(key, kb)==0 && fscanf(fp, "%lf", &val)==1) { m->neurons[cur_k].beta[f]  = val; break; }
        }
    }
    fclose(fp);
    return 0;
}

/* ── Logging forensique ────────────────────────────────────────────── */

void nx48_log_stats(const nx48_model_t *m) {
    if (!m) return;

    double skip_rate = (m->n_samples_seen > 0)
        ? (double)m->n_skipped / m->n_samples_seen : 0.0;
    double avg_loss  = (m->n_samples_seen > 0)
        ? m->sum_loss / m->n_samples_seen : 0.0;

    FORENSIC_LOG_MODULE_METRIC("nx48_sign", "n_samples_total",  (double)m->n_samples_seen);
    FORENSIC_LOG_MODULE_METRIC("nx48_sign", "n_skipped",        (double)m->n_skipped);
    FORENSIC_LOG_MODULE_METRIC("nx48_sign", "skip_rate",        skip_rate);
    FORENSIC_LOG_MODULE_METRIC("nx48_sign", "avg_bce_loss",     avg_loss);
    FORENSIC_LOG_MODULE_METRIC("nx48_sign", "best_f1",          m->best_f1);
    FORENSIC_LOG_MODULE_METRIC("nx48_sign", "best_neuron_idx",  (double)m->best_neuron);
    FORENSIC_LOG_MODULE_METRIC("nx48_sign", "n_positive_pred",  (double)m->n_positive_pred);

    /* Sparsité des poids du meilleur neurone */
    const nx48_neuron_t *best = &m->neurons[m->best_neuron];
    int n_zero_w = 0, n_zero_a = 0, n_zero_b2 = 0;
    for (int f = 0; f < NX48_N_FEATURES; f++) {
        if (fabs(best->w[f])     < 1e-12) n_zero_w++;
        if (fabs(best->alpha[f]) < 1e-12) n_zero_a++;
        if (fabs(best->beta[f])  < 1e-12) n_zero_b2++;
    }
    double sparsity = (double)(n_zero_w + n_zero_a + n_zero_b2)
                      / (3.0 * NX48_N_FEATURES);
    FORENSIC_LOG_MODULE_METRIC("nx48_sign", "weight_sparsity",  sparsity);
    FORENSIC_LOG_MODULE_METRIC("nx48_sign", "best_neuron_lr",   best->lr);
    FORENSIC_LOG_MODULE_METRIC("nx48_sign", "best_neuron_bias", best->b);
}

/* ── Fonction utilitaire publique : overhead estimé ──────────────────
 *
 * nx48_estimate_overhead_reduction — Calcule le facteur de réduction
 * d'overhead estimé donné le skip_rate.
 *
 * overhead_original : (1/sign_ratio)² pour le module sans NX48
 * skip_rate         : fraction de configs sautées par NX48
 * precision         : fraction de vrais positifs parmi les configs gardées
 *
 * Retourne : overhead réduit estimé = overhead_orig × (1-skip_rate) / precision
 */
double nx48_estimate_overhead_reduction(double overhead_original,
                                         double skip_rate,
                                         double precision) {
    if (precision < 1e-9) precision = 1e-9;
    return overhead_original * (1.0 - skip_rate) / precision;
}
