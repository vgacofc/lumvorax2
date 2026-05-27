#ifndef QUANTUM_SIMULATOR_FUSION_V3_H
#define QUANTUM_SIMULATOR_FUSION_V3_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "quantum_simulator.h"

typedef enum {
    QUANTUM_RNG_HARDWARE_ONLY = 0,
    QUANTUM_RNG_HARDWARE_PREFERRED = 1,
    QUANTUM_RNG_DETERMINISTIC_SEEDED = 2
} quantum_rng_mode_e;

typedef struct {
    size_t scenarios;
    size_t steps;
    double nqubits_simulated;
    double nqubits_per_sec;
    double nqubit_avg_score;
    double baseline_qubit_avg_score;
    double nqubit_win_rate;
    size_t nqubit_wins;
    size_t baseline_wins;
    uint64_t elapsed_ns;
    bool used_hardware_entropy;
    uint64_t latency_p50_ns;
    uint64_t latency_p95_ns;
    uint64_t latency_p99_ns;
} quantum_fusion_v3_summary_t;

bool quantum_fusion_v3_run_forensic_benchmark(
    const char* output_jsonl,
    size_t scenarios,
    size_t steps,
    quantum_rng_mode_e rng_mode,
    uint64_t seed,
    quantum_fusion_v3_summary_t* out_summary
);

#endif
