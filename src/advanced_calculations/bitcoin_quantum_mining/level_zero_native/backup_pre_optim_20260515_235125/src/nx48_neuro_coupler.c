/* nx48_neuro_coupler.c — Implémentation couplage NX48 ↔ Izhikevich+STDP.
 *
 * Cycle C98. Voir header pour spec complète.
 * Conformité STANDARD_NAMES.md §M-BTC18-C98 + prompt.txt v2.1.
 */
#include "nx48_neuro_coupler.h"
#include "../include/common/lvx_log_rotate.h"  /* C99 P2 : rotation log 50 Mo auto */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* Connexion full-mesh sans self-loop : 8 × 7 = 56 synapses excitatrices */
static void coupler_wire_full_mesh(neural_network_t *net, double w_init) {
    int idx = 0;
    for (int i = 0; i < NX48_COUPLER_N_NEURONS; ++i) {
        for (int j = 0; j < NX48_COUPLER_N_NEURONS; ++j) {
            if (i == j) continue;  /* pas de self-loop */
            net->src[idx]    = i;
            net->dst[idx]    = j;
            net->weight[idx] = w_init;
            idx++;
        }
    }
    /* idx doit valoir 56 */
}

/* C99 : initialise la matrice de décorrélation 8x8 (proche identité avec
 * léger mélange anti-redondance, recommandation expert Q1). La feature_i
 * reçoit 70% d'elle-même + 30% mélangé uniformément des 7 autres → casse
 * les corrélations triviales sans bouleverser la sémantique 1-pour-1. */
static void coupler_init_decorrelation(double W[NX48_COUPLER_N_NEURONS][NX48_COUPLER_N_NEURONS]) {
    const double diag = 0.70;
    const double off  = 0.30 / (double)(NX48_COUPLER_N_NEURONS - 1);
    for (int i = 0; i < NX48_COUPLER_N_NEURONS; ++i) {
        for (int j = 0; j < NX48_COUPLER_N_NEURONS; ++j) {
            W[i][j] = (i == j) ? diag : off;
        }
    }
}

nx48_coupler_t *nx48_coupler_create(const char *run_id, double w_init) {
    nx48_coupler_t *c = (nx48_coupler_t *)calloc(1, sizeof(*c));
    if (!c) return NULL;
    c->net = nn_create(NX48_COUPLER_N_NEURONS, NX48_COUPLER_N_SYNAPSES);
    if (!c->net) { free(c); return NULL; }
    if (w_init < 0.0) w_init = 0.0;
    if (w_init > 1.0) w_init = 1.0;
    coupler_wire_full_mesh(c->net, w_init);
    c->coupler_active = 1;
    c->mean_rate_hz   = 0.0;
    c->last_modulation = 0.0;
    /* C99 : init seuil near-miss = 20 (par défaut, sera adapté par p90) */
    c->near_miss_threshold = 20;
    c->lz_history_count = 0;
    c->lz_history_idx = 0;
    coupler_init_decorrelation(c->decorr_W);
    c->w_spread_ema = 0.0;
    c->w_mean_ema = w_init;
    if (run_id) {
        strncpy(c->run_id, run_id, sizeof(c->run_id) - 1);
        c->run_id[sizeof(c->run_id) - 1] = '\0';
    } else {
        snprintf(c->run_id, sizeof(c->run_id), "default");
    }
    return c;
}

void nx48_coupler_destroy(nx48_coupler_t *c) {
    if (!c) return;
    nn_destroy(c->net);
    free(c);
}

void nx48_coupler_set_active(nx48_coupler_t *c, int active) {
    if (!c) return;
    c->coupler_active = active ? 1 : 0;
}

double nx48_coupler_step(nx48_coupler_t *c,
                         const double features[NX48_COUPLER_N_NEURONS],
                         int near_miss_event) {
    if (!c || !c->coupler_active || !features) {
        if (c) c->last_modulation = 0.0;
        return 0.0;
    }

    /* C99 — 1. Décorrélation features via matrice 8x8 (Q1 critique expert).
     * input_i = Σ_j W_ij * features[j] avec W = 0.7·I + 0.3/7 · J_off
     * Cela casse les corrélations triviales sans dénaturer la sémantique. */
    double feat_decorr[NX48_COUPLER_N_NEURONS];
    for (int i = 0; i < NX48_COUPLER_N_NEURONS; ++i) {
        double s = 0.0;
        for (int j = 0; j < NX48_COUPLER_N_NEURONS; ++j) {
            double f = features[j];
            if (f < 0.0) f = 0.0;
            if (f > 1.0) f = 1.0;
            s += c->decorr_W[i][j] * f;
        }
        feat_decorr[i] = s;
    }

    /* 2. Conversion features décorrélées → courants stimulants (pA) C99 calibré. */
    double I_ext[NX48_COUPLER_N_NEURONS];
    for (int i = 0; i < NX48_COUPLER_N_NEURONS; ++i) {
        I_ext[i] = feat_decorr[i] * NX48_COUPLER_I_GAIN_PA;
    }

    /* 3. Injection récompense (C99 : 25 pA proportionnel au gain réduit) */
    if (near_miss_event) {
        for (int i = 0; i < NX48_COUPLER_N_NEURONS; ++i) {
            I_ext[i] += NX48_COUPLER_REWARD_PA;
        }
        c->near_miss_total++;
    }

    /* 4. Pas de simulation Izhikevich + STDP */
    nn_step(c->net, I_ext, NX48_COUPLER_DT_MS);
    nn_apply_stdp(c->net);

    /* 5. Compter spikes émis ce pas */
    int spikes_this_step = 0;
    for (int i = 0; i < c->net->n_neurons; ++i) {
        if (c->net->neurons[i].spiked) spikes_this_step++;
    }
    c->spike_count += (uint64_t)spikes_this_step;
    c->step_count++;

    /* 6. Taux moyen EMA (Hz) */
    const double spikes_per_neuron_per_ms =
        (double)spikes_this_step / (double)c->net->n_neurons / NX48_COUPLER_DT_MS;
    const double instant_hz = spikes_per_neuron_per_ms * 1000.0;
    const double alpha = NX48_COUPLER_DT_MS / NX48_COUPLER_WINDOW_MS;
    c->mean_rate_hz = (1.0 - alpha) * c->mean_rate_hz + alpha * instant_hz;

    /* 7. C99 — Statistiques poids (EMA) pour audit STDP en temps réel */
    double w_min = +1e30, w_max = -1e30, w_sum = 0.0;
    for (int s = 0; s < c->net->n_synapses; ++s) {
        double w = c->net->weight[s];
        if (w < w_min) w_min = w;
        if (w > w_max) w_max = w;
        w_sum += w;
    }
    const double w_spread = w_max - w_min;
    const double w_mean = (c->net->n_synapses > 0) ? w_sum / c->net->n_synapses : 0.0;
    c->w_spread_ema = (1.0 - alpha) * c->w_spread_ema + alpha * w_spread;
    c->w_mean_ema   = (1.0 - alpha) * c->w_mean_ema   + alpha * w_mean;

    /* 8. C99 — Signal modulation : (rate - 30) / 30, borné [-1,+1].
     * 30 Hz = milieu régime RS sain (20-40 Hz), pas 20 Hz comme en C98. */
    double mod = (c->mean_rate_hz - 30.0) / 30.0;
    if (mod < -1.0) mod = -1.0;
    if (mod > +1.0) mod = +1.0;
    c->last_modulation = mod;
    return mod;
}

/* C99 : seuil near-miss DYNAMIQUE percentile_90 (Q1 expert).
 * Met à jour l'historique circulaire LZ et recalcule le seuil. */
static void coupler_update_threshold(nx48_coupler_t *c, int leading_zeros) {
    /* Insère LZ dans buffer circulaire */
    c->lz_history[c->lz_history_idx] = leading_zeros;
    c->lz_history_idx = (c->lz_history_idx + 1) % NX48_COUPLER_LZ_HIST_SIZE;
    if (c->lz_history_count < NX48_COUPLER_LZ_HIST_SIZE) {
        c->lz_history_count++;
    }
    /* Recalcule p90 toutes les 10 entrées (économie CPU) */
    if ((c->step_count % 10) != 0 || c->lz_history_count < 10) return;
    int sorted[NX48_COUPLER_LZ_HIST_SIZE];
    for (int i = 0; i < c->lz_history_count; ++i) sorted[i] = c->lz_history[i];
    /* Tri insertion (n≤100, suffisant) */
    for (int i = 1; i < c->lz_history_count; ++i) {
        int v = sorted[i], j = i - 1;
        while (j >= 0 && sorted[j] > v) { sorted[j+1] = sorted[j]; --j; }
        sorted[j+1] = v;
    }
    int p90_idx = (int)(0.9 * (double)c->lz_history_count);
    if (p90_idx >= c->lz_history_count) p90_idx = c->lz_history_count - 1;
    int p90 = sorted[p90_idx];
    /* Seuil minimum 18 LZ (sécurité contre démarrage froid) */
    if (p90 < 18) p90 = 18;
    c->near_miss_threshold = p90;
}

double nx48_coupler_step_auto(nx48_coupler_t *c,
                              const double features[NX48_COUPLER_N_NEURONS],
                              int current_leading_zeros) {
    if (!c) return 0.0;
    /* Met à jour le seuil avant de juger l'événement */
    coupler_update_threshold(c, current_leading_zeros);
    int near_miss = (current_leading_zeros >= c->near_miss_threshold) ? 1 : 0;
    return nx48_coupler_step(c, features, near_miss);
}

int nx48_coupler_log_step_jsonl(const nx48_coupler_t *c, const char *log_path,
                                int near_miss_event, int leading_zeros) {
    if (!c || !log_path) return -1;
    /* C99 P2 : écriture via lvx_log_append_jsonl → rotation 50 Mo auto. */
    char line[512];
    int n = snprintf(line, sizeof(line),
        "{\"step\":%llu,\"rate_hz\":%.4f,\"mod\":%.4f,\"w_spread\":%.6f,"
        "\"w_mean\":%.6f,\"near_miss\":%d,\"lz\":%d,\"thr\":%d,\"nm_total\":%llu}\n",
        (unsigned long long)c->step_count,
        c->mean_rate_hz, c->last_modulation,
        c->w_spread_ema, c->w_mean_ema,
        near_miss_event, leading_zeros, c->near_miss_threshold,
        (unsigned long long)c->near_miss_total);
    if (n < 0 || (size_t)n >= sizeof(line)) return -3;
    return (lvx_log_append_jsonl(log_path, line) == 0) ? 0 : -2;
}

int nx48_coupler_serialize_jsonl(const nx48_coupler_t *c, const char *path) {
    if (!c || !path) return -1;

    /* Calcul checksum FNV1a-64 sur les poids */
    uint64_t hash = 0xcbf29ce484222325ULL;
    for (int s = 0; s < c->net->n_synapses; ++s) {
        const unsigned char *p = (const unsigned char *)&c->net->weight[s];
        for (size_t k = 0; k < sizeof(double); ++k) {
            hash ^= p[k];
            hash *= 0x100000001b3ULL;
        }
    }

    /* Statistiques poids */
    double w_min = +1e30, w_max = -1e30, w_mean = 0.0;
    for (int s = 0; s < c->net->n_synapses; ++s) {
        double w = c->net->weight[s];
        if (w < w_min) w_min = w;
        if (w > w_max) w_max = w;
        w_mean += w;
    }
    if (c->net->n_synapses > 0) w_mean /= (double)c->net->n_synapses;

    /* C99 P2 : écriture via lvx_log_append_jsonl → rotation 50 Mo auto. */
    char line[1024];
    int n = snprintf(line, sizeof(line),
        "{\"run_id\":\"%s\",\"step\":%llu,\"spikes_total\":%llu,"
        "\"mean_rate_hz\":%.6f,\"last_modulation\":%.6f,"
        "\"coupler_active\":%d,\"n_synapses\":%d,"
        "\"w_min\":%.6e,\"w_max\":%.6e,\"w_mean\":%.6e,"
        "\"checksum\":\"0x%016llx\"}\n",
        c->run_id,
        (unsigned long long)c->step_count,
        (unsigned long long)c->spike_count,
        c->mean_rate_hz, c->last_modulation,
        c->coupler_active, c->net->n_synapses,
        w_min, w_max, w_mean,
        (unsigned long long)hash);
    if (n < 0 || (size_t)n >= sizeof(line)) return -3;
    return (lvx_log_append_jsonl(path, line) == 0) ? 0 : -2;
}
