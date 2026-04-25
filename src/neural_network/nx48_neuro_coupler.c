/* nx48_neuro_coupler.c — Implémentation couplage NX48 ↔ Izhikevich+STDP.
 *
 * Cycle C98. Voir header pour spec complète.
 * Conformité STANDARD_NAMES.md §M-BTC18-C98 + prompt.txt v2.1.
 */
#include "nx48_neuro_coupler.h"
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

    /* 1. Conversion features → courants stimulants (pA) */
    double I_ext[NX48_COUPLER_N_NEURONS];
    for (int i = 0; i < NX48_COUPLER_N_NEURONS; ++i) {
        double f = features[i];
        if (f < 0.0) f = 0.0;
        if (f > 1.0) f = 1.0;
        I_ext[i] = f * NX48_COUPLER_I_GAIN_PA;
    }

    /* 2. Injection récompense en cas de near-miss BTC : tous neurones boostés */
    if (near_miss_event) {
        for (int i = 0; i < NX48_COUPLER_N_NEURONS; ++i) {
            I_ext[i] += NX48_COUPLER_REWARD_PA;
        }
    }

    /* 3. Pas de simulation Izhikevich + STDP */
    nn_step(c->net, I_ext, NX48_COUPLER_DT_MS);
    nn_apply_stdp(c->net);

    /* 4. Compter spikes émis ce pas */
    int spikes_this_step = 0;
    for (int i = 0; i < c->net->n_neurons; ++i) {
        if (c->net->neurons[i].spiked) spikes_this_step++;
    }
    c->spike_count += (uint64_t)spikes_this_step;
    c->step_count++;

    /* 5. Calcul du taux moyen sur fenêtre glissante (EMA pour fluidité) */
    /* Conversion : spikes/step → Hz, sachant dt = NX48_COUPLER_DT_MS ms */
    const double spikes_per_neuron_per_ms =
        (double)spikes_this_step / (double)c->net->n_neurons / NX48_COUPLER_DT_MS;
    const double instant_hz = spikes_per_neuron_per_ms * 1000.0;

    /* EMA avec alpha basé sur la fenêtre WINDOW_MS */
    const double alpha = NX48_COUPLER_DT_MS / NX48_COUPLER_WINDOW_MS;
    c->mean_rate_hz = (1.0 - alpha) * c->mean_rate_hz + alpha * instant_hz;

    /* 6. Signal modulation : (rate - 20) / 20, borné [-1,+1] */
    double mod = (c->mean_rate_hz - 20.0) / 20.0;
    if (mod < -1.0) mod = -1.0;
    if (mod > +1.0) mod = +1.0;
    c->last_modulation = mod;
    return mod;
}

int nx48_coupler_serialize_jsonl(const nx48_coupler_t *c, const char *path) {
    if (!c || !path) return -1;
    FILE *fp = fopen(path, "a");
    if (!fp) return -2;

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

    fprintf(fp,
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
    fclose(fp);
    return 0;
}
