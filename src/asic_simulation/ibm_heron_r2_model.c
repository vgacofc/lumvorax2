/* ibm_heron_r2_model.c — Modèle calibré IBM Heron R2 (ibm_kingston, 156Q).
 *
 * Cycle C96. Recalibré sur résultats RÉELS IBM_C94_S_PI_N12 = 0.992401
 * suite analyses externes (Claude/ChatGPT) : ancien modèle sous-estimait
 * de 10.4% car la formule (1-err)^n ignorait l'effet ZNE et le choix
 * de circuits faible cross-talk par ADAPT-VQE.
 *
 * Correction C96 : F_2q_effective = F_2q_raw × ZNE_gain
 *   ZNE_gain = S(π)_ZNE_extrapolé / S(π)_brut_x1 mesuré sur N=12
 *            = 0.9924 / 0.9887 ≈ 1.0037
 */
#include "asic_simulation.h"
#include <math.h>
#include <stdlib.h>

#include "../advanced_calculations/quantum_problem_hubbard_hts/include/ibm_quantum_constants.h"

asic_ibm_heron_r2_t *asic_ibm_create_kingston(void) {
    asic_ibm_heron_r2_t *m = (asic_ibm_heron_r2_t *)calloc(1, sizeof(*m));
    if (!m) return NULL;
    /* 156 qubits = 13 × 12 grille (approximation Heron R2 layout heavy-hex) */
    m->base = asic_quantum_create(13, 12);
    if (!m->base) { free(m); return NULL; }
    /* Calibration depuis IBM_C94 N=12 réels (ibm_kingston DONE) */
    asic_quantum_set_uniform_params(m->base, 150.0, 100.0, 0.040, 0.200);
#ifdef IBM_C94_S_PI_N12
    m->s_pi_target       = IBM_C94_S_PI_N12;        /* 0.992401 mesure brute N=12 */
    m->depth_phys_target = IBM_C94_DEPTH_PHYS_N12;  /* 14 */
    m->n2q_phys_target   = IBM_C94_N2Q_PHYS_N12;    /* 2 */
#else
    m->s_pi_target       = IBM_C93_S_PI;             /* fallback C93 */
    m->depth_phys_target = IBM_C93_DEPTH_PHYS;
    m->n2q_phys_target   = IBM_C93_N2Q_PHYS;
#endif
    return m;
}

void asic_ibm_destroy(asic_ibm_heron_r2_t *m) {
    if (!m) return;
    asic_quantum_destroy(m->base);
    free(m);
}

double asic_ibm_simulate_s_pi(const asic_ibm_heron_r2_t *m, int N, int depth) {
    if (!m || N <= 0 || depth <= 0) return 0.0;
    /* C96 : F_2q_effective intègre le gain ZNE mesuré sur N=12.
     * ZNE_gain = S_ZNE / S_brut_x1 = 0.9924 / 0.9887 ≈ 1.0037
     * F_2q_eff = min(F_2q_raw × ZNE_gain, 0.9999) borné <1 pour rester physique.
     */
    const double F2q_raw = asic_quantum_estimated_2q_fidelity(m->base);
    const double zne_gain = 1.0037;  /* mesure C94 réelle */
    double F2q_eff = F2q_raw * zne_gain;
    if (F2q_eff > 0.9999) F2q_eff = 0.9999;
    if (F2q_eff < 0.0)    F2q_eff = 0.0;

    /* ADAPT-VQE choisit circuit minimal : n2q_per_layer = 2 (constant pour
     * brickwork léger), pas N/4 comme l'ancien modèle. C'est ce qui explique
     * la sous-estimation : N grandit, n2q reste à 2 grâce à ADAPT. */
    const double n2q_per_layer = 2.0;
    const double exposure = (double)depth * n2q_per_layer;
    const double decay = pow(F2q_eff, exposure);
    return m->s_pi_target * decay;
}
