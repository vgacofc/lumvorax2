/* izhikevich_spiking.c — implémentation L3 du modèle Izhikevich.
 *
 * Référence : E.M. Izhikevich, "Simple Model of Spiking Neurons",
 * IEEE Trans. Neural Networks, 14(6):1569-1572, 2003.
 *
 * Cycle C95 — squelette compilable.
 */
#include "neural_network.h"
#include <string.h>

void izh_neuron_init(izh_neuron_t *n, double a, double b, double c, double d) {
    if (!n) return;
    n->a = a;
    n->b = b;
    n->c = c;
    n->d = d;
    n->v = c;          /* repos initial */
    n->u = b * c;
    n->spiked = 0;
}

void izh_neuron_step(izh_neuron_t *n, double I_input, double dt_ms) {
    if (!n) return;
    n->spiked = 0;
    /* Euler 2 sous-pas pour stabilité (dt typique 0.5 ms) */
    const double dt_half = 0.5 * dt_ms;
    for (int k = 0; k < 2; ++k) {
        const double v = n->v;
        const double u = n->u;
        const double dv = (0.04 * v * v + 5.0 * v + 140.0 - u + I_input) * dt_half;
        const double du = (n->a * (n->b * v - u)) * dt_half;
        n->v += dv;
        n->u += du;
        if (n->v >= 30.0) {
            n->v = n->c;
            n->u += n->d;
            n->spiked = 1;
            break;
        }
    }
}

void izh_preset_regular_spiking(izh_neuron_t *n) {
    izh_neuron_init(n, 0.02, 0.2, -65.0, 8.0);
}
void izh_preset_fast_spiking(izh_neuron_t *n) {
    izh_neuron_init(n, 0.1, 0.2, -65.0, 2.0);
}
void izh_preset_chattering(izh_neuron_t *n) {
    izh_neuron_init(n, 0.02, 0.2, -50.0, 2.0);
}
