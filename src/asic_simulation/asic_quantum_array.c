/* asic_quantum_array.c — Modèle d'array de qubits supraconducteurs.
 *
 * Cycle C95. Fournit fidélité 2-qubit estimée à partir des paramètres T1/T2/durées.
 * Modèle simple : F_2q ≈ exp(-gate_2q/T_eff) avec T_eff = min(T1, T2) / 2.
 */
#include "asic_simulation.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

asic_quantum_array_t *asic_quantum_create(int rows, int cols) {
    if (rows <= 0 || cols <= 0) return NULL;
    asic_quantum_array_t *q = (asic_quantum_array_t *)calloc(1, sizeof(*q));
    if (!q) return NULL;
    q->n_rows = rows;
    q->n_cols = cols;
    q->qubit_freq_GHz = (double *)calloc((size_t)(rows * cols), sizeof(double));
    if (!q->qubit_freq_GHz) { free(q); return NULL; }
    /* defaults plausibles pour Heron R2 */
    q->T1_us = 150.0;
    q->T2_us = 100.0;
    q->gate_1q_us = 0.04;
    q->gate_2q_us = 0.20;
    q->crosstalk_zz_kHz = 30.0;
    q->readout_fidelity = 0.985;
    for (int i = 0; i < rows * cols; ++i) {
        q->qubit_freq_GHz[i] = 5.0 + 0.001 * (double)i;   /* léger spread */
    }
    return q;
}

void asic_quantum_destroy(asic_quantum_array_t *q) {
    if (!q) return;
    free(q->qubit_freq_GHz);
    free(q);
}

void asic_quantum_set_uniform_params(asic_quantum_array_t *q,
                                     double T1, double T2,
                                     double g1, double g2) {
    if (!q) return;
    q->T1_us = T1;
    q->T2_us = T2;
    q->gate_1q_us = g1;
    q->gate_2q_us = g2;
}

double asic_quantum_estimated_2q_fidelity(const asic_quantum_array_t *q) {
    if (!q) return 0.0;
    const double T_eff = (q->T1_us < q->T2_us ? q->T1_us : q->T2_us) * 0.5;
    if (T_eff <= 0.0) return 0.0;
    return exp(-q->gate_2q_us / T_eff);
}

/* ── C98 — Hook VORAX (audit C97.7 résolu) ───────────────────────────────── */
#include <stdint.h>

int asic_quantum_extract_vorax_signal(const asic_quantum_array_t *q,
                                      double signal[ASIC_VORAX_SIGNAL_DIM]) {
    if (!q || !signal) return -1;

    const double F2q = asic_quantum_estimated_2q_fidelity(q);
    signal[0] = F2q;
    signal[1] = (q->T2_us > 0.0) ? q->T1_us / q->T2_us : 0.0;
    signal[2] = (q->gate_1q_us > 0.0) ? q->gate_2q_us / q->gate_1q_us : 0.0;
    signal[3] = q->crosstalk_zz_kHz / 100.0;
    signal[4] = q->readout_fidelity;

    /* Profondeur utile : depth_max tel que F_2q^depth ≥ 0.99 */
    if (F2q >= 1.0 || F2q <= 0.0) {
        signal[5] = 0.0;
    } else {
        signal[5] = log(0.99) / log(F2q);
    }

    /* Checksum FNV1a-64 sur la grille de fréquences pour audit forensique */
    const int n = q->n_rows * q->n_cols;
    uint64_t hash = 0xcbf29ce484222325ULL;
    for (int i = 0; i < n; ++i) {
        const unsigned char *p = (const unsigned char *)&q->qubit_freq_GHz[i];
        for (size_t k = 0; k < sizeof(double); ++k) {
            hash ^= p[k];
            hash *= 0x100000001b3ULL;
        }
    }
    signal[6] = (double)hash / 1.8446744073709552e19;  /* normalisé 2^64 */
    signal[7] = (double)n / 256.0;
    return 0;
}
