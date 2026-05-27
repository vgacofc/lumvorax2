/* stdp_plasticity.c — Réseau spiking + STDP (plasticité Hebbienne temporelle).
 *
 * Cycle C95. Implémentation L4 du cahier des charges.
 *
 * STDP additive (Song, Miller, Abbott, 2000) :
 *   Δw = A_+ exp(-Δt/τ_+) si Δt > 0  (post après pre → potentiation)
 *   Δw = -A_- exp(Δt/τ_-)  si Δt < 0  (pre après post → depression)
 *
 * On utilise des traces exponentielles pour éviter de stocker l'historique :
 *   trace_pre[i]  *= exp(-dt/τ_+);  spike pre  → trace_pre  += 1
 *   trace_post[j] *= exp(-dt/τ_-);  spike post → trace_post += 1
 *   spike post → w[i,j] += A_+ × trace_pre[i]
 *   spike pre  → w[i,j] -= A_- × trace_post[j]
 */
#include "neural_network.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

neural_network_t *nn_create(int n_neurons, int n_synapses) {
    if (n_neurons <= 0 || n_synapses < 0) return NULL;
    neural_network_t *nn = (neural_network_t *)calloc(1, sizeof(*nn));
    if (!nn) return NULL;
    nn->n_neurons = n_neurons;
    nn->n_synapses = n_synapses;
    nn->neurons = (izh_neuron_t *)calloc((size_t)n_neurons, sizeof(izh_neuron_t));
    nn->trace_post = (double *)calloc((size_t)n_neurons, sizeof(double));
    nn->trace_pre  = (double *)calloc((size_t)n_neurons, sizeof(double));
    if (n_synapses > 0) {
        nn->src    = (int *)calloc((size_t)n_synapses, sizeof(int));
        nn->dst    = (int *)calloc((size_t)n_synapses, sizeof(int));
        nn->weight = (double *)calloc((size_t)n_synapses, sizeof(double));
    }
    if (!nn->neurons || !nn->trace_post || !nn->trace_pre
        || (n_synapses > 0 && (!nn->src || !nn->dst || !nn->weight))) {
        nn_destroy(nn);
        return NULL;
    }
    /* Tous les neurones par défaut en regular spiking */
    for (int i = 0; i < n_neurons; ++i) {
        izh_preset_regular_spiking(&nn->neurons[i]);
    }
    nn->tau_pre  = 20.0;
    nn->tau_post = 20.0;
    nn->A_plus   = 0.005;
    nn->A_minus  = 0.005 * 1.05;   /* asymétrie classique pour stabilité */
    nn->w_min = 0.0;
    nn->w_max = 1.0;
    return nn;
}

void nn_destroy(neural_network_t *nn) {
    if (!nn) return;
    free(nn->neurons);
    free(nn->src);
    free(nn->dst);
    free(nn->weight);
    free(nn->trace_pre);
    free(nn->trace_post);
    free(nn);
}

static int next_synapse_idx = 0;
static neural_network_t *current_nn = NULL;
void nn_connect(neural_network_t *nn, int src, int dst, double w0) {
    if (!nn) return;
    if (current_nn != nn) { current_nn = nn; next_synapse_idx = 0; }
    if (next_synapse_idx >= nn->n_synapses) return;
    if (src < 0 || src >= nn->n_neurons) return;
    if (dst < 0 || dst >= nn->n_neurons) return;
    nn->src[next_synapse_idx] = src;
    nn->dst[next_synapse_idx] = dst;
    if (w0 < nn->w_min) w0 = nn->w_min;
    if (w0 > nn->w_max) w0 = nn->w_max;
    nn->weight[next_synapse_idx] = w0;
    next_synapse_idx++;
}

void nn_step(neural_network_t *nn, const double *I_ext, double dt) {
    if (!nn) return;
    /* 1. propagation des spikes du pas précédent dans I_synaptique */
    double *I_syn = (double *)calloc((size_t)nn->n_neurons, sizeof(double));
    if (!I_syn) return;
    for (int s = 0; s < nn->n_synapses; ++s) {
        if (nn->neurons[nn->src[s]].spiked) {
            I_syn[nn->dst[s]] += nn->weight[s] * 15.0;   /* gain courant arbitraire */
        }
    }
    /* 2. décroissance des traces STDP */
    const double dec_pre  = exp(-dt / nn->tau_pre);
    const double dec_post = exp(-dt / nn->tau_post);
    for (int i = 0; i < nn->n_neurons; ++i) {
        nn->trace_pre[i]  *= dec_pre;
        nn->trace_post[i] *= dec_post;
    }
    /* 3. step Izhikevich */
    for (int i = 0; i < nn->n_neurons; ++i) {
        const double I = (I_ext ? I_ext[i] : 0.0) + I_syn[i];
        izh_neuron_step(&nn->neurons[i], I, dt);
        if (nn->neurons[i].spiked) {
            nn->trace_pre[i]  += 1.0;
            nn->trace_post[i] += 1.0;
        }
    }
    free(I_syn);
}

void nn_apply_stdp(neural_network_t *nn) {
    if (!nn) return;
    for (int s = 0; s < nn->n_synapses; ++s) {
        const int i = nn->src[s];
        const int j = nn->dst[s];
        if (nn->neurons[j].spiked) {
            /* post fired → potentiate using pre trace */
            nn->weight[s] += nn->A_plus * nn->trace_pre[i];
        }
        if (nn->neurons[i].spiked) {
            /* pre fired → depress using post trace */
            nn->weight[s] -= nn->A_minus * nn->trace_post[j];
        }
        if (nn->weight[s] < nn->w_min) nn->weight[s] = nn->w_min;
        if (nn->weight[s] > nn->w_max) nn->weight[s] = nn->w_max;
    }
}
