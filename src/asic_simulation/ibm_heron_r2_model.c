/* ibm_heron_r2_model.c — Modèle calibré IBM Heron R2 (ibm_kingston, 156Q).
 *
 * Cycle C95. Utilise les constantes IBM_C93_* du header
 * src/advanced_calculations/quantum_problem_hubbard_hts/include/ibm_quantum_constants.h
 * pour calibrer un modèle classique du QPU.
 */
#include "asic_simulation.h"
#include <math.h>
#include <stdlib.h>

/* Inclure le header IBM (chemin relatif depuis src/asic_simulation/) */
#include "../advanced_calculations/quantum_problem_hubbard_hts/include/ibm_quantum_constants.h"

asic_ibm_heron_r2_t *asic_ibm_create_kingston(void) {
    asic_ibm_heron_r2_t *m = (asic_ibm_heron_r2_t *)calloc(1, sizeof(*m));
    if (!m) return NULL;
    /* 156 qubits = 13 × 12 grille (approximation Heron R2 layout heavy-hex) */
    m->base = asic_quantum_create(13, 12);
    if (!m->base) { free(m); return NULL; }
    /* Calibration depuis IBM_C93 réels */
    asic_quantum_set_uniform_params(m->base, 150.0, 100.0, 0.040, 0.200);
    m->s_pi_target = IBM_C93_S_PI;             /* 0.9944 */
    m->depth_phys_target = IBM_C93_DEPTH_PHYS;  /* 14 */
    m->n2q_phys_target = IBM_C93_N2Q_PHYS;      /* 2 */
    return m;
}

void asic_ibm_destroy(asic_ibm_heron_r2_t *m) {
    if (!m) return;
    asic_quantum_destroy(m->base);
    free(m);
}

double asic_ibm_simulate_s_pi(const asic_ibm_heron_r2_t *m, int N, int depth) {
    if (!m || N <= 0 || depth <= 0) return 0.0;
    /* Modèle : S(π) ≈ s_pi_target × F_2q^(depth × n2q_per_layer) */
    const double F2q = asic_quantum_estimated_2q_fidelity(m->base);
    /* n2q_per_layer ≈ N/4 pour brickwork ADAPT-VQE light */
    const double n2q = (double)(N) / 4.0;
    const double exposure = (double)depth * n2q;
    const double decay = pow(F2q, exposure);
    return m->s_pi_target * decay;
}
