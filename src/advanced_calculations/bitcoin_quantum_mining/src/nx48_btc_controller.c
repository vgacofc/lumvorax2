/*
 * LumVorax — Module 17 — Bitcoin Quantum Mining Engine
 * nx48_btc_controller.c — Contrôleur NX48 adapté espace nonce Bitcoin
 *
 * STANDARD_NAMES.md v4.1 §M-BTC17 — Cycle C62 — 2026-04-11
 *
 * Implémentation du neurone NX48_BTC avec gradient ISTA.
 * Même principe que nx48_adaptive_controller.c (Hubbard)
 * mais adapté aux features Bitcoin (leading_zeros, hashrate, swap_rate…)
 */

#ifndef _GNU_SOURCE
#  define _GNU_SOURCE
#endif
#define _POSIX_C_SOURCE 200809L

#include "nx48_btc_controller.h"
#include "../include/btc_mining_forensic.h"
#include "debug/ultra_forensic_logger.h"
#include "lumvorax_integration.h"
#include "debug/memory_tracker.h"

#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <inttypes.h>

/* ── Xoshiro256++ — PRNG état de l'art (Blackman & Vigna 2019) ────────────────
 * Période 2^256 - 1, quality statistique extrême (passe toutes les suites BigCrush).
 * Remplace l'oscillation déterministe ±2% par un vrai bruit stochastique.
 * Ref : C65-FIX-PRNG — analysechatgpt91.38.md §2.2 — 2026-04-12
 * ─────────────────────────────────────────────────────────────────────────────*/
static uint64_t xosh_s[4];  /* État 256 bits du PRNG — initialisé via /dev/urandom */

static void xosh_seed(void) {
    /* Initialisation par /dev/urandom — graine véritablement aléatoire */
    FILE* urnd = fopen("/dev/urandom", "rb");
    if (urnd) {
        size_t n = fread(xosh_s, sizeof(uint64_t), 4, urnd);
        fclose(urnd);
        if (n == 4 && (xosh_s[0] | xosh_s[1] | xosh_s[2] | xosh_s[3]) != 0) return;
    }
    /* Fallback : clock + PID (entropie faible mais non-zéro) */
    xosh_s[0] = (uint64_t)time(NULL) ^ 0x6C62272E07BB0142ULL;
    xosh_s[1] = (uint64_t)clock()    ^ 0x62B821756295C58DULL;
    xosh_s[2] = xosh_s[0] ^ (xosh_s[0] >> 33);
    xosh_s[3] = xosh_s[1] ^ (xosh_s[1] >> 17);
}

static inline uint64_t xosh_rotl(uint64_t x, int k) {
    return (x << k) | (x >> (64 - k));
}

/* Génère un uint64_t uniforme (Xoshiro256++) */
static inline uint64_t xosh_next(void) {
    const uint64_t result = xosh_rotl(xosh_s[0] + xosh_s[3], 23) + xosh_s[0];
    const uint64_t t = xosh_s[1] << 17;
    xosh_s[2] ^= xosh_s[0];
    xosh_s[3] ^= xosh_s[1];
    xosh_s[1] ^= xosh_s[2];
    xosh_s[0] ^= xosh_s[3];
    xosh_s[2] ^= t;
    xosh_s[3] = xosh_rotl(xosh_s[3], 45);
    return result;
}

/* Génère un double uniforme dans [0, 1) */
static inline double xosh_uniform(void) {
    return (double)(xosh_next() >> 11) * (1.0 / 9007199254740992.0); /* 2^53 */
}

/* Génère un bruit gaussien N(0,σ) via la méthode Ziggurat simplifiée (Box-Muller) */
static double xosh_gaussian(double sigma) {
    /* Box-Muller transform — qualité statistique excellente */
    static int have_extra = 0;
    static double extra = 0.0;
    if (have_extra) { have_extra = 0; return extra * sigma; }
    double u1, u2;
    do { u1 = xosh_uniform(); } while (u1 < 1e-300);
    u2 = xosh_uniform();
    double mag  = sqrt(-2.0 * log(u1));
    double angle = 2.0 * 3.14159265358979323846 * u2;
    extra = mag * sin(angle);
    have_extra = 1;
    return mag * cos(angle) * sigma;
}

/* ── Valeurs par défaut poids NX48_BTC ──────────────────────────── */
static const double NX48_BTC_WEIGHTS_DEFAULT[NX48_BTC_N_FEATURES] = {
    0.35,   /* btc_best_leading_zeros — feature la plus importante */
    0.20,   /* btc_hashrate_norm */
    0.15,   /* btc_ptmc_swap_rate */
    0.10,   /* btc_time_stall */
    0.08,   /* btc_nonce_coverage */
    0.05,   /* btc_delta_nonce_norm */
    0.04,   /* btc_thread_eff */
    0.03    /* btc_temp_ratio */
};

/* ── Sigmoid ─────────────────────────────────────────────────────── */
static inline double sigmoid(double x) {
    return 1.0 / (1.0 + exp(-x));
}

/* ── Clamp ───────────────────────────────────────────────────────── */
static inline double clamp(double v, double lo, double hi) {
    return (v < lo) ? lo : (v > hi) ? hi : v;
}

/* ── Initialise NX48_BTC ────────────────────────────────────────── */
nx48_btc_state_t* nx48_btc_init(const nx48_btc_config_t* cfg, const char* run_id) {
    nx48_btc_state_t* s = LV_CALLOC(1, sizeof(nx48_btc_state_t));
    if (!s) return NULL;

    /* Tenter de charger depuis CSV */
    int loaded = 0;
    if (cfg->csv_path[0] != '\0')
        loaded = nx48_btc_load_csv(s, cfg->csv_path);

    if (!loaded) {
        /* Initialisation par défaut */
        memcpy(s->weights, NX48_BTC_WEIGHTS_DEFAULT, sizeof(s->weights));
        s->bias               = 0.0;
        s->delta_nonce_scale  = 1.0;
        s->n_replicas_scale   = 1.0;
        s->swap_temp_scale    = 1.0;
        s->batch_size_scale   = 1.0;
        s->exploration_bias   = 0.5;
        s->loss_prev          = 1.0;
        s->loss_curr          = 1.0;
        s->grad_norm          = 0.0;
        s->update_count       = 0;
        s->best_leading_zeros = 0;
        s->best_nonce         = 0;
    }

    /* C65-FIX-PRNG : graine Xoshiro256++ depuis /dev/urandom à chaque session */
    xosh_seed();

    strncpy(s->run_id, run_id ? run_id : "unknown", sizeof(s->run_id)-1);
    s->run_id[sizeof(s->run_id)-1] = '\0';

    /* Log forensic init */
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_nx48_init_loaded",       (double)loaded);
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_nx48_delta_nonce_scale", s->delta_nonce_scale);
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_nx48_n_replicas_scale",  s->n_replicas_scale);
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_nx48_swap_temp_scale",   s->swap_temp_scale);
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_nx48_batch_size_scale",  s->batch_size_scale);
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_nx48_exploration_bias",  s->exploration_bias);

    return s;
}

/* ── Calcul des features normalisées ────────────────────────────── */
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
    double T_cold)
{
    /* F0 : leading_zeros / 256 (max 256 bits) → [0,1] */
    features[NX48_BTC_F_BEST_LEADING] = clamp((double)best_leading_zeros / 256.0, 0.0, 1.0);

    /* F1 : hashrate normalisé */
    features[NX48_BTC_F_HASHRATE_NORM] = (hashrate_max > 0.0)
        ? clamp(hashrate_mhs / hashrate_max, 0.0, 1.0) : 0.0;

    /* F2 : taux échange PT-MC */
    features[NX48_BTC_F_SWAP_RATE] = clamp(swap_rate, 0.0, 1.0);

    /* F3 : -log10(time_stall) normalisé (temps depuis amélioration) */
    /* Plus le temps est long, plus F3 est bas → signal "explorer plus" */
    features[NX48_BTC_F_TIME_STALL] = (time_since_improvement_s > 0.0)
        ? clamp(-log10(time_since_improvement_s) / 10.0 + 0.5, 0.0, 1.0) : 0.5;

    /* F4 : couverture nonce */
    features[NX48_BTC_F_COVERAGE] = clamp(nonce_coverage_pct / 100.0, 0.0, 1.0);

    /* F5 : rayon delta normalisé (/ 2^32) */
    features[NX48_BTC_F_DELTA_NORM] = clamp(delta_nonce / 4294967296.0, 0.0, 1.0);

    /* F6 : efficacité thread */
    features[NX48_BTC_F_THREAD_EFF] = (hashes_expected > 0.0)
        ? clamp(hashes_done / hashes_expected, 0.0, 1.0) : 0.0;

    /* F7 : ratio T_hot / T_cold (température PT) */
    features[NX48_BTC_F_TEMP_RATIO] = (T_cold > 0.0)
        ? clamp((T_hot / T_cold) / 100.0, 0.0, 1.0) : 0.5;
}

/* ── Prédiction NX48_BTC ────────────────────────────────────────── */
double nx48_btc_predict(nx48_btc_state_t* s, const double features[NX48_BTC_N_FEATURES]) {
    double z = s->bias;
    for (int i = 0; i < NX48_BTC_N_FEATURES; i++)
        z += s->weights[i] * features[i];
    return sigmoid(z);
}

/* ── Mise à jour gradient ISTA ──────────────────────────────────── */
void nx48_btc_update(
    nx48_btc_state_t*     s,
    const nx48_btc_config_t* cfg,
    const double features[NX48_BTC_N_FEATURES],
    double prob,
    int best_leading_zeros,
    double hashrate_mhs)
{
    /* C38-FIX-LABEL-256 : Label linéaire sur [0, 256] — objectif 256 bits.
     * AVANT (C65) : 1.0 - exp(-0.15 × lz) → sature à 0.95 dès 20 bits !
     *               Gradient quasi nul au-delà de 20 bits → stagnation garantie.
     *
     * APRÈS (C38) : label = lz / 256.0 (linéaire)
     *   → label(20)  = 0.078 — gradient fort vers l'objectif 256 bits
     *   → label(64)  = 0.250 — gradient significatif (niveau réseau Bitcoin ~75 bits)
     *   → label(256) = 1.000 — objectif atteint (256 leading zeros)
     *   → gradient ISTA reste actif sur TOUTE la plage [0, 256]
     *   → pousse continuellement vers le maximum théorique SHA-256
     *
     * Conformité : STANDARD_NAMES.md v4.2 §M-BTC17-C38
     * Ref : analysechatgpt91.37.md §4.2 objectif NX48 256 bits — 2026-04-12 */
    double label = clamp((double)best_leading_zeros / 256.0, 0.0, 1.0);

    /* BCE loss */
    double eps = 1e-12;
    double p   = clamp(prob, eps, 1.0 - eps);
    double bce = -(label * log(p) + (1.0 - label) * log(1.0 - p));

    s->loss_prev = s->loss_curr;
    s->loss_curr = bce;

    /* Gradient erreur */
    double err = prob - label;

    /* Gradient ISTA avec régularisation L1 */
    double grad_norm_sq = 0.0;
    double lr = cfg->learning_rate;
    double l1 = cfg->lambda_l1;

    for (int i = 0; i < NX48_BTC_N_FEATURES; i++) {
        double g = err * features[i];
        /* Soft-threshold L1 */
        double w_new = s->weights[i] - lr * g;
        if (w_new >  l1) w_new -= l1;
        else if (w_new < -l1) w_new += l1;
        else w_new = 0.0;
        grad_norm_sq += g * g;
        s->weights[i] = w_new;
    }
    s->bias -= lr * err;
    s->grad_norm = sqrt(grad_norm_sq);

    /* Adaptation hyper-paramètres selon gradient */
    double old_delta  = s->delta_nonce_scale;
    double old_batch  = s->batch_size_scale;

    /* ── C65-FIX-ADAPT : Adaptation delta_nonce avec bruit gaussien Xoshiro256++
     *
     * AVANT (C64) : oscillation déterministe ×1.02 / ×0.98 selon update_count%2
     *   → exploration CORRÉLÉE (alternance fixe) ≠ recuit simulé
     *   → identique à une sinusoïde de période 2 → biais systématique
     *
     * APRÈS (C65) : perturbation gaussienne N(0, σ) + signal gradient signé
     *   → décalage proportionnel à loss_delta (signal) + bruit stochastique (σ)
     *   → σ augmente quand le système stagne longtemps (exploration adaptative)
     *   → σ diminue quand loss s'améliore (exploitation locale renforcée)
     *
     * Formulation mathématique :
     *   σ = σ_base × exp(stagnation_pct × ln(σ_max/σ_base))
     *   perturbation = N(0, σ)
     *   delta_nonce_new = delta_nonce_old × exp(α × loss_delta + perturbation)
     *
     * Ref : analysechatgpt91.38.md §C65-FIX-ADAPT — 2026-04-12
     */
    {
        double loss_delta = (s->loss_prev > 1e-12)
            ? (s->loss_curr - s->loss_prev) / s->loss_prev : 0.0;

        /* Stagnation normalisée [0,1] : 1 = stagnation absolue, 0 = amélioration */
        double stagnation = clamp(loss_delta * 10.0, 0.0, 1.0);

        /* Sigma adaptatif : σ_base=0.05 quand performance optimale, σ_max=0.25 en stagnation */
        double sigma_base = 0.05;
        double sigma_max  = 0.25;
        double sigma = sigma_base * exp(stagnation * log(sigma_max / sigma_base));

        /* Perturbation gaussienne pure */
        double noise = xosh_gaussian(sigma);

        /* Gradient signé : si loss augmente → explorer plus (positif) */
        double alpha = 0.8;   /* gain adaptatif */
        double push  = alpha * (-loss_delta);  /* négatif si stagnation → agrandit espace */

        /* Mise à jour log-normale (multiplicative → toujours positif) */
        s->delta_nonce_scale *= exp(push + noise);

        /* Exploration_bias : monte quand stagnation, descend quand amélioration */
        if (loss_delta > 0.02)
            s->exploration_bias = clamp(s->exploration_bias + 0.04, 0.0, 1.0);
        else if (loss_delta < -0.02)
            s->exploration_bias = clamp(s->exploration_bias - 0.02, 0.0, 1.0);
        /* sinon : bruit gaussien léger sur exploration_bias */
        else
            s->exploration_bias = clamp(s->exploration_bias + xosh_gaussian(0.01), 0.0, 1.0);
    }

    /* ── C65-FIX-BATCH : Scheduling continu du learning rate batch
     *
     * AVANT (C64) : seuil binaire unique — grad_norm > 0.20 ? ×1.08 : ×1.02
     *   → transition brutale, calibration arbitraire, non différentiable
     *
     * APRÈS (C65) : interpolation lisse via tanh (Universal Approximation)
     *   adapt_rate = 1.0 + 0.10 × tanh(5.0 × grad_norm)
     *   → 1.00 quand grad_norm → 0   (plateau absolu → pas de croissance)
     *   → 1.10 quand grad_norm → ∞   (signal fort → expansion maximale)
     *   → transition douce centrée en grad_norm = 0.20 (tanh(1.0) ≈ 0.76)
     *
     * Ref : analysechatgpt91.38.md §C65-FIX-BATCH — 2026-04-12
     */
    if (hashrate_mhs > 0 && hashrate_mhs < 100.0) {
        double adapt_rate = 1.0 + 0.10 * tanh(5.0 * s->grad_norm);
        s->batch_size_scale = clamp(s->batch_size_scale * adapt_rate, 0.5, 4.0);
    }

    /* C38-FIX-B-NX48 : Correction stagnation delta_nonce.
     * AVANT : si aucun nouveau record, delta_nonce reste figé indéfiniment.
     * APRÈS : stall_count++ à chaque update sans record.
     *         Si stall_count ≥ 2 → delta_nonce_scale ×1.05 (exploration forcée).
     * Ref : analysechatgpt91.37.md §4.2 BUG B-NX48 delta_nonce figé 0.950 — 2026-04-12 */
    if (best_leading_zeros > s->best_leading_zeros) {
        s->best_leading_zeros = best_leading_zeros;
        s->stall_count = 0;
        FORENSIC_LOG_ANOMALY(BTC_MODULE_NAME,
            "btc_nx48_new_record_leading_zeros", (double)best_leading_zeros);
    } else {
        s->stall_count++;
        if (s->stall_count >= 2) {
            /* C40-DELTA-MAX : Cap corrigé 10.0 → 50.0 cohérent avec nx48_btc_clamp_scales().
             * AVANT C40 : stall_count cap à 10.0 → delta ne dépassait jamais 10 dans ce branch.
             * APRÈS C40 : cohérent avec le clamp global (max=50.0) pour run infini.
             * Ref : rapport forensique C40 §DELTA — 2026-04-13 */
            s->delta_nonce_scale = clamp(s->delta_nonce_scale * 1.05, 0.1, 50.0);
            FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME,
                "btc_nx48_stall_count", (double)s->stall_count);
            s->stall_count = 0;
        }
    }

    nx48_btc_clamp_scales(s);
    s->update_count++;

    /* Log forensic adaptation */
    BTC_FORENSIC_NX48_ADAPT(old_delta, s->delta_nonce_scale, old_batch, s->batch_size_scale);
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_nx48_loss",           s->loss_curr);
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_nx48_grad_norm",      s->grad_norm);
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_nx48_update_count",   (double)s->update_count);
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_nx48_exploration_bias", s->exploration_bias);
}

/* ── Clamp des scales dans les bornes physiques ─────────────────── */
/* C39 : delta_nonce_scale max étendu 10.0 → 50.0 pour run infini.
 * Sur run illimité, le moteur doit pouvoir explorer l'espace nonce en entier.
 * Ref : analysechatgpt91.38.md §P3 — 2026-04-12 */
void nx48_btc_clamp_scales(nx48_btc_state_t* s) {
    s->delta_nonce_scale  = clamp(s->delta_nonce_scale,  0.1,  50.0);
    s->n_replicas_scale   = clamp(s->n_replicas_scale,   1.0,   2.0);
    s->swap_temp_scale    = clamp(s->swap_temp_scale,    0.5,   3.0);
    s->batch_size_scale   = clamp(s->batch_size_scale,   0.5,   4.0);
    s->exploration_bias   = clamp(s->exploration_bias,   0.0,   1.0);
}

/* ── Sauvegarde CSV (format btc_nx48_last.csv) ──────────────────── */
int nx48_btc_save_csv(const nx48_btc_state_t* s, const char* csv_path) {
    if (!csv_path || csv_path[0] == '\0') return 0;
    FILE* f = fopen(csv_path, "w");
    if (!f) return 0;
    fprintf(f, "run_id,delta_nonce_scale,n_replicas_scale,swap_temp_scale,"
               "batch_size_scale,exploration_bias,best_leading_zeros,"
               "best_nonce,update_count,loss_curr,grad_norm\n");
    fprintf(f, "%s,%.6f,%.6f,%.6f,%.6f,%.6f,%d,%u,%d,%.9f,%.9f\n",
        s->run_id,
        s->delta_nonce_scale, s->n_replicas_scale,
        s->swap_temp_scale,   s->batch_size_scale,
        s->exploration_bias,  s->best_leading_zeros,
        s->best_nonce,        s->update_count,
        s->loss_curr,         s->grad_norm);
    fclose(f);
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_nx48_csv_saved", 1.0);
    return 1;
}

/* ── Chargement CSV ─────────────────────────────────────────────── */
int nx48_btc_load_csv(nx48_btc_state_t* s, const char* csv_path) {
    if (!csv_path || csv_path[0] == '\0') return 0;
    FILE* f = fopen(csv_path, "r");
    if (!f) return 0;
    char header[512]; fgets(header, sizeof(header), f); /* skip header */
    int ok = (fscanf(f,
        "%63[^,],%lf,%lf,%lf,%lf,%lf,%d,%u,%d,%lf,%lf",
        s->run_id,
        &s->delta_nonce_scale, &s->n_replicas_scale,
        &s->swap_temp_scale,   &s->batch_size_scale,
        &s->exploration_bias,  &s->best_leading_zeros,
        &s->best_nonce,        &s->update_count,
        &s->loss_curr,         &s->grad_norm) == 11);
    fclose(f);
    if (ok) {
        nx48_btc_clamp_scales(s);
        FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_nx48_csv_loaded", 1.0);
    }
    return ok;
}

/* ── Libère la mémoire ──────────────────────────────────────────── */
void nx48_btc_destroy(nx48_btc_state_t* s) {
    if (s) LV_FREE(s);
}
