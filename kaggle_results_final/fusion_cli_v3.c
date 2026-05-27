#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "quantum_simulator_fusion_v3.h"

int main(int argc, char** argv) {
    if (argc < 7) {
        fprintf(stderr, "usage: %s <output_jsonl> <mode:hardware_only|hardware_preferred|deterministic_seeded> <seed> <scenarios> <steps> <summary_json>\n", argv[0]);
        return 2;
    }

    quantum_rng_mode_e mode = QUANTUM_RNG_HARDWARE_PREFERRED;
    if (strcmp(argv[2], "hardware_only") == 0) mode = QUANTUM_RNG_HARDWARE_ONLY;
    else if (strcmp(argv[2], "hardware_preferred") == 0) mode = QUANTUM_RNG_HARDWARE_PREFERRED;
    else if (strcmp(argv[2], "deterministic_seeded") == 0) mode = QUANTUM_RNG_DETERMINISTIC_SEEDED;
    else {
        fprintf(stderr, "invalid mode: %s\n", argv[2]);
        return 3;
    }

    const uint64_t seed = (uint64_t)strtoull(argv[3], NULL, 10);
    const size_t scenarios = (size_t)strtoull(argv[4], NULL, 10);
    const size_t steps = (size_t)strtoull(argv[5], NULL, 10);

    quantum_fusion_v3_summary_t s;
    const bool ok = quantum_fusion_v3_run_forensic_benchmark(argv[1], scenarios, steps, mode, seed, &s);
    if (!ok) {
        fprintf(stderr, "benchmark_failed\n");
        return 1;
    }

    FILE* jf = fopen(argv[6], "w");
    if (!jf) {
        perror("fopen");
        return 4;
    }

    fprintf(jf,
            "{\n"
            "  \"mode\": \"%s\",\n"
            "  \"seed\": %llu,\n"
            "  \"scenarios\": %zu,\n"
            "  \"steps\": %zu,\n"
            "  \"nqubits_simulated\": %.0f,\n"
            "  \"nqubits_per_sec\": %.3f,\n"
            "  \"nqubit_avg_score\": %.12f,\n"
            "  \"baseline_qubit_avg_score\": %.12f,\n"
            "  \"nqubit_win_rate\": %.12f,\n"
            "  \"nqubit_wins\": %zu,\n"
            "  \"baseline_wins\": %zu,\n"
            "  \"elapsed_ns\": %llu,\n"
            "  \"used_hardware_entropy\": %s,\n"
            "  \"latency_p50_ns\": %llu,\n"
            "  \"latency_p95_ns\": %llu,\n"
            "  \"latency_p99_ns\": %llu\n"
            "}\n",
            argv[2],
            (unsigned long long)seed,
            s.scenarios,
            s.steps,
            s.nqubits_simulated,
            s.nqubits_per_sec,
            s.nqubit_avg_score,
            s.baseline_qubit_avg_score,
            s.nqubit_win_rate,
            s.nqubit_wins,
            s.baseline_wins,
            (unsigned long long)s.elapsed_ns,
            s.used_hardware_entropy ? "true" : "false",
            (unsigned long long)s.latency_p50_ns,
            (unsigned long long)s.latency_p95_ns,
            (unsigned long long)s.latency_p99_ns);

    fclose(jf);
    printf("wins=%zu losses=%zu win_rate=%.9f nqubits_per_sec=%.3f p95=%llu\n",
           s.nqubit_wins,
           s.baseline_wins,
           s.nqubit_win_rate,
           s.nqubits_per_sec,
           (unsigned long long)s.latency_p95_ns);
    return 0;
}
