/* test_neural.c — Tests unitaires du module neural_network.
 *
 * Cycle C95. Couvre :
 *   - Izhikevich regular spiking (au moins 1 spike sous I=10 pendant 1 s)
 *   - HH alpha/beta non-NaN à V=-65 mV
 *   - STDP : 100 neurones × 1000 synapses, simulation 200 ms, poids restent dans [0,1]
 */
#include "neural_network.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

static int g_pass = 0, g_fail = 0;
#define CHECK(cond, msg) do { \
    if (cond) { g_pass++; printf("[OK]   %s\n", msg); } \
    else      { g_fail++; printf("[FAIL] %s\n", msg); } \
} while(0)

static void test_izh_regular_spiking(void) {
    izh_neuron_t n;
    izh_preset_regular_spiking(&n);
    int n_spikes = 0;
    const double dt = 0.5;            /* ms */
    const int   n_steps = 2000;       /* 1000 ms */
    for (int i = 0; i < n_steps; ++i) {
        izh_neuron_step(&n, 10.0, dt);
        if (n.spiked) n_spikes++;
    }
    CHECK(n_spikes >= 5,  "Izh RS spike count >= 5 sous I=10 (1 s)");
    CHECK(n_spikes <= 50, "Izh RS spike count <= 50 (pas pathologique)");
    CHECK(!isnan(n.v),    "Izh v non-NaN");
    CHECK(n.v > -90 && n.v < 50, "Izh v dans [-90, 50] mV");
    printf("       (n_spikes = %d)\n", n_spikes);
}

static void test_ion_channel_hh(void) {
    ion_channel_t na = {
        .type = ION_CHANNEL_NA_V15,
        .g_max = 120.0, .e_rev = 50.0,
        .m = 0.05, .h = 0.6, .n = 0.0,
        .tau_m = 0.0, .tau_h = 0.0
    };
    ion_channel_step(&na, -65.0, 0.01);
    CHECK(!isnan(na.m) && !isnan(na.h), "Na_v 1.5 m,h non-NaN à V=-65 mV");
    CHECK(na.m >= 0 && na.m <= 1,        "Na_v 1.5 m dans [0,1]");
    CHECK(na.h >= 0 && na.h <= 1,        "Na_v 1.5 h dans [0,1]");
    const double I = ion_channel_current(&na, -65.0);
    CHECK(!isnan(I), "Na_v 1.5 I non-NaN");
}

static void test_stdp_network(void) {
    const int N = 100;
    const int S = 1000;
    neural_network_t *nn = nn_create(N, S);
    CHECK(nn != NULL, "nn_create(100, 1000)");
    if (!nn) return;

    /* connexions aléatoires reproductibles */
    srand(42);
    for (int s = 0; s < S; ++s) {
        const int src = rand() % N;
        int dst = rand() % N;
        if (dst == src) dst = (dst + 1) % N;
        nn_connect(nn, src, dst, 0.5);
    }
    /* simulation 200 ms */
    double *I = (double *)calloc((size_t)N, sizeof(double));
    if (!I) { nn_destroy(nn); CHECK(0, "calloc I"); return; }
    /* injection : 10 premiers neurones reçoivent I=15 (forcer activité) */
    for (int i = 0; i < 10; ++i) I[i] = 15.0;
    int total_spikes = 0;
    for (int t = 0; t < 400; ++t) {     /* 400 × 0.5 ms = 200 ms */
        nn_step(nn, I, 0.5);
        nn_apply_stdp(nn);
        for (int i = 0; i < N; ++i) if (nn->neurons[i].spiked) total_spikes++;
    }
    int weights_in_range = 1;
    for (int s = 0; s < S; ++s) {
        if (nn->weight[s] < 0.0 || nn->weight[s] > 1.0) { weights_in_range = 0; break; }
    }
    CHECK(weights_in_range, "STDP : tous les poids restent dans [0,1]");
    CHECK(total_spikes > 0, "STDP : au moins quelques spikes (réseau actif)");
    printf("       (total_spikes = %d sur %d ms)\n", total_spikes, 200);

    free(I);
    nn_destroy(nn);
}

int main(void) {
    printf("=== Tests neural_network (C95) ===\n");
    test_izh_regular_spiking();
    test_ion_channel_hh();
    test_stdp_network();
    printf("\nRésultat : %d/%d OK, %d FAIL\n", g_pass, g_pass + g_fail, g_fail);
    return g_fail == 0 ? 0 : 1;
}
