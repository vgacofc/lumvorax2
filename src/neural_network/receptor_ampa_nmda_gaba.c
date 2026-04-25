/* receptor_ampa_nmda_gaba.c — Récepteurs synaptiques chimiques.
 *
 * Cycle C95. Modèle dual-exponential (rise + decay) pour AMPA, NMDA, GABA-A.
 *
 * Références :
 *   AMPA  : tau_rise=0.5 ms, tau_decay=3 ms, e_rev=0 mV
 *   NMDA  : tau_rise=2 ms,   tau_decay=80 ms, e_rev=0 mV (Mg-block ignoré ici)
 *   GABA-A: tau_rise=0.5 ms, tau_decay=10 ms, e_rev=-70 mV
 */
#include "neural_network.h"
#include <math.h>
#include <string.h>

void receptor_spike(receptor_t *r) {
    if (!r) return;
    r->s += 1.0;
    if (r->s > 5.0) r->s = 5.0;     /* saturation */
}

void receptor_step(receptor_t *r, double dt) {
    if (!r) return;
    /* Approximation dual-exponential avec une seule variable s :
     *   s = α exp(-t/tau_decay) - α exp(-t/tau_rise)
     * Pour simplifier, on intègre l'enveloppe avec une seule constante effective. */
    const double tau_eff = (r->tau_rise > 0 && r->tau_decay > 0)
                           ? (r->tau_decay - r->tau_rise) > 0
                             ? r->tau_decay
                             : r->tau_decay
                           : 5.0;
    r->s *= exp(-dt / tau_eff);
    if (r->s < 1e-10) r->s = 0.0;
}

double receptor_current(const receptor_t *r, double V_post) {
    if (!r) return 0.0;
    /* g_eff = g_peak × s normalisé */
    return r->g_peak * r->s * (V_post - r->e_rev_mV);
}
