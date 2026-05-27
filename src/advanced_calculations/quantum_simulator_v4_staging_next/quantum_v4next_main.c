#define _POSIX_C_SOURCE 200809L
/* C33 — Runner principal V4 NEXT — LumVorax Quantum Simulator */
#include "quantum_simulator.h"
#include "quantum_forensic_logger.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
    (void)argc; (void)argv;

    /* C33 — Initialisation du logger ultra-granulaire QF avant tout */
    qf_logger_init("v4next_runner", "RAPPORTS");
    QF_LOG_MODULE_START("quantum_v4next_main", "runner_v4next");
    QF_LOG_HW_SAMPLE("quantum_v4next_main");
    QF_LOG_MODULE_METRIC("quantum_v4next_main", "quantum_lum:runner_version", 4.0);

    printf("=== LumVorax Quantum Simulator V4 NEXT — Runner Principal ===\n");

    /* Créer la configuration par défaut */
    quantum_config_t* cfg = quantum_config_create_default();
    if (!cfg) {
        QF_LOG_ANOMALY("quantum_v4next_main", "config_create_failed", 0.0);
        fprintf(stderr, "[ERREUR] Impossible de créer la configuration quantique\n");
        qf_logger_destroy();
        return 1;
    }
    QF_LOG_MODULE_METRIC("quantum_v4next_main", "quantum_lum:config_ready", 1.0);

    /* Stress test — validation de base */
    const bool stress_ok = quantum_stress_test_100m_qubits(cfg);
    QF_LOG_MODULE_METRIC("quantum_v4next_main", "quantum_lum:stress_test_passed",
                         stress_ok ? 1.0 : 0.0);
    if (!stress_ok) {
        QF_LOG_ANOMALY("quantum_v4next_main", "stress_test_failed", 0.0);
        fprintf(stderr, "[ERREUR] Stress test échoué\n");
        free(cfg);
        qf_logger_destroy();
        return 2;
    }

    /* Créer deux LUMs et les intriquer */
    quantum_lum_t* qlum_a = quantum_lum_create(0, 0, 2);
    quantum_lum_t* qlum_b = quantum_lum_create(1, 0, 2);
    if (!qlum_a || !qlum_b) {
        QF_LOG_ANOMALY("quantum_v4next_main", "lum_ab_create_failed", 0.0);
        if (qlum_a) quantum_lum_destroy(&qlum_a);
        if (qlum_b) quantum_lum_destroy(&qlum_b);
        free(cfg);
        qf_logger_destroy();
        return 3;
    }
    QF_LOG_MODULE_METRIC("quantum_v4next_main", "quantum_lum:a_fidelity", qlum_a->fidelity);
    QF_LOG_MODULE_METRIC("quantum_v4next_main", "quantum_lum:b_fidelity", qlum_b->fidelity);

    /* Appliquer la porte Hadamard sur A */
    bool gate_ok = quantum_apply_gate(qlum_a, QUANTUM_GATE_HADAMARD, cfg);
    QF_LOG_MODULE_METRIC("quantum_v4next_main", "quantum_gate:hadamard_ok", gate_ok ? 1.0 : 0.0);
    printf("[INFO] Porte Hadamard appliquée sur LUM A : %s — fidelity=%.6f\n",
           gate_ok ? "OK" : "FAIL", qlum_a->fidelity);

    /* Intriquer A et B */
    bool entangle_ok = quantum_entangle_lums(qlum_a, qlum_b, cfg);
    QF_LOG_MODULE_METRIC("quantum_v4next_main", "quantum_entangle:ab_ok",
                         entangle_ok ? 1.0 : 0.0);
    printf("[INFO] Intrication A↔B : %s — entanglement_count=%zu\n",
           entangle_ok ? "OK" : "FAIL", qlum_a->entanglement_count);

    /* Mesure de A */
    quantum_result_t* result = quantum_measure(qlum_a, cfg);
    if (result) {
        QF_LOG_MODULE_METRIC("quantum_v4next_main", "quantum_measure:result_state_count",
                             (double)result->state_count);
        printf("[INFO] Mesure LUM A : état mesuré — state_count=%zu\n", result->state_count);
        quantum_result_destroy(&result);
    } else {
        QF_LOG_ANOMALY("quantum_v4next_main", "measure_result_null", 0.0);
    }

    /* Nettoyage */
    quantum_lum_destroy(&qlum_a);
    quantum_lum_destroy(&qlum_b);
    free(cfg);

    QF_LOG_MODULE_END("quantum_v4next_main", "runner_v4next", true);
    /* Flush ring buffer nano + rapport final */
    qf_flush_nano_ring();
    qf_generate_summary_report();
    qf_logger_destroy();

    printf("=== V4 NEXT Runner terminé avec succès ===\n");
    return 0;
}
