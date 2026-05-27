/* ion_channel_markov.c — Canaux ioniques Hodgkin-Huxley étendus.
 *
 * Cycle C95. Implémentation des 5 types de canaux (Na_v 1.5, K_v 1.1, K leak,
 * Ca L-type, Cl GABA-gated) via gating Markov simplifié de Hodgkin-Huxley.
 *
 * Référence : Hodgkin & Huxley, J Physiol 117:500-544, 1952.
 *             Hille, "Ion Channels of Excitable Membranes", 3rd ed., 2001.
 */
#include "neural_network.h"
#include <math.h>
#include <string.h>

/* α/β rates Hodgkin-Huxley standards (T=6.3°C, units: 1/ms). */
static double alpha_m_Na(double V) {
    const double x = -(V + 40.0);
    if (fabs(x) < 1e-6) return 1.0;   /* L'Hôpital limit */
    return 0.1 * x / (exp(x / 10.0) - 1.0);
}
static double beta_m_Na(double V)  { return 4.0  * exp(-(V + 65.0) / 18.0); }
static double alpha_h_Na(double V) { return 0.07 * exp(-(V + 65.0) / 20.0); }
static double beta_h_Na(double V)  { return 1.0  / (1.0 + exp(-(V + 35.0) / 10.0)); }
static double alpha_n_K(double V) {
    const double x = -(V + 55.0);
    if (fabs(x) < 1e-6) return 0.1;
    return 0.01 * x / (exp(x / 10.0) - 1.0);
}
static double beta_n_K(double V)   { return 0.125 * exp(-(V + 65.0) / 80.0); }

void ion_channel_step(ion_channel_t *ch, double V, double dt) {
    if (!ch) return;
    double am = 0, bm = 0, ah = 0, bh = 0, an = 0, bn = 0;
    switch (ch->type) {
        case ION_CHANNEL_NA_V15:
            am = alpha_m_Na(V); bm = beta_m_Na(V);
            ah = alpha_h_Na(V); bh = beta_h_Na(V);
            ch->m += dt * (am * (1.0 - ch->m) - bm * ch->m);
            ch->h += dt * (ah * (1.0 - ch->h) - bh * ch->h);
            ch->tau_m = 1.0 / (am + bm);
            ch->tau_h = 1.0 / (ah + bh);
            break;
        case ION_CHANNEL_K_V11:
            an = alpha_n_K(V); bn = beta_n_K(V);
            ch->n += dt * (an * (1.0 - ch->n) - bn * ch->n);
            break;
        case ION_CHANNEL_K_LEAK:
            /* pas de gating, conductance constante */
            break;
        case ION_CHANNEL_CA_L:
            /* approximation : gating type m² (instantané) */
            ch->m = 1.0 / (1.0 + exp(-(V + 20.0) / 5.0));
            break;
        case ION_CHANNEL_CL_GABA:
            /* géré par récepteur GABA-A */
            break;
        case ION_CHANNEL_COUNT:
        default:
            break;
    }
    /* clamp [0,1] pour stabilité numérique */
    if (ch->m < 0) ch->m = 0; else if (ch->m > 1) ch->m = 1;
    if (ch->h < 0) ch->h = 0; else if (ch->h > 1) ch->h = 1;
    if (ch->n < 0) ch->n = 0; else if (ch->n > 1) ch->n = 1;
}

double ion_channel_current(const ion_channel_t *ch, double V) {
    if (!ch) return 0.0;
    double g = 0.0;
    switch (ch->type) {
        case ION_CHANNEL_NA_V15:
            g = ch->g_max * ch->m * ch->m * ch->m * ch->h;
            break;
        case ION_CHANNEL_K_V11:
            g = ch->g_max * ch->n * ch->n * ch->n * ch->n;
            break;
        case ION_CHANNEL_K_LEAK:
            g = ch->g_max;
            break;
        case ION_CHANNEL_CA_L:
            g = ch->g_max * ch->m * ch->m;
            break;
        case ION_CHANNEL_CL_GABA:
            g = ch->g_max * ch->m;
            break;
        case ION_CHANNEL_COUNT:
        default:
            break;
    }
    return g * (V - ch->e_rev);   /* convention : I positif = sortant */
}
