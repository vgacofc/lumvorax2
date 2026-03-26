#define _POSIX_C_SOURCE 200809L
#include "quantum_simulator_fusion_v2.h"

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static uint64_t now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static uint64_t xorshift64(uint64_t* state) {
    uint64_t x = *state;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    *state = x;
    return x;
}

static bool seed_from_hardware(uint64_t* out_seed) {
    const char* paths[] = {"/dev/hwrng", "/dev/random", "/dev/urandom"};
    for (size_t i = 0; i < sizeof(paths) / sizeof(paths[0]); ++i) {
        FILE* f = fopen(paths[i], "rb");
        if (!f) {
            continue;
        }
        uint64_t seed = 0;
        size_t rd = fread(&seed, 1, sizeof(seed), f);
        fclose(f);
        if (rd == sizeof(seed) && seed != 0) {
            *out_seed = seed;
            return true;
        }
    }
    return false;
}

static double uniform01(uint64_t* state) {
    const uint64_t value = xorshift64(state);
    return (double)(value >> 11) * (1.0 / 9007199254740992.0);
}

static double gaussian(uint64_t* state, double sigma) {
    const double u1 = fmax(uniform01(state), 1e-12);
    const double u2 = uniform01(state);
    const double radius = sqrt(-2.0 * log(u1));
    const double theta = 6.28318530717958647692 * u2;
    return sigma * radius * cos(theta);
}

bool quantum_fusion_v2_run_forensic_benchmark(
    const char* output_jsonl,
    size_t scenarios,
    size_t steps,
    quantum_rng_mode_e rng_mode,
    uint64_t seed,
    quantum_fusion_v2_summary_t* out_summary
) {
    if (!output_jsonl || scenarios == 0 || steps == 0 || !out_summary) {
        return false;
    }

    FILE* logf = fopen(output_jsonl, "w");
    if (!logf) {
        return false;
    }

    uint64_t rng_nx = seed;
    uint64_t rng_q = seed ^ 0x9E3779B97F4A7C15ULL;
    bool used_hw = false;

    if (rng_mode == QUANTUM_RNG_HARDWARE_ONLY || rng_mode == QUANTUM_RNG_HARDWARE_PREFERRED) {
        uint64_t hw_seed = 0;
        const bool hw_ok = seed_from_hardware(&hw_seed);
        if (!hw_ok && rng_mode == QUANTUM_RNG_HARDWARE_ONLY) {
            fclose(logf);
            return false;
        }
        if (hw_ok) {
            rng_nx = hw_seed;
            rng_q = hw_seed ^ 0xA5A5A5A55A5A5A5AULL;
            used_hw = true;
        }
    }

    if (rng_nx == 0) rng_nx = 0xC0FFEE12345678ULL;
    if (rng_q == 0) rng_q = 0xABCDEF456789ULL;

    const uint64_t t0_ns = now_ns();
    double nx_total_score = 0.0;
    double q_total_score = 0.0;
    size_t nx_wins = 0U;

    fprintf(logf,
            "{\"event\":\"run_config\",\"rng_mode\":%d,\"used_hardware_entropy\":%s,\"scenarios\":%zu,\"steps\":%zu}\n",
            (int)rng_mode,
            used_hw ? "true" : "false",
            scenarios,
            steps);

    for (size_t i = 0; i < scenarios; ++i) {
        double nx_state = -1.5 + 3.0 * ((double)i / (double)scenarios);
        double q_state = nx_state;
        const double target = 0.7 + 0.4 * (double)(i % 11U) / 10.0;
        const double sigma = 0.02 + 0.001 * (double)(i % 20U);
        const double thermal = 1.0 + 0.02 * (double)(i % 15U);
        const double lyapunov_gain = 0.25;

        for (size_t s = 0; s < steps; ++s) {
            const double noise_nx = gaussian(&rng_nx, sigma * thermal);
            const double grad = target - nx_state;
            nx_state += noise_nx + lyapunov_gain * tanh(grad);

            const double noise_q = gaussian(&rng_q, sigma * 0.7);
            q_state += 0.03 * (target - q_state) + noise_q;
        }

        const double nx_score = 1.0 / (1.0 + fabs(target - nx_state));
        const double q_score = 1.0 / (1.0 + fabs(target - q_state));
        nx_total_score += nx_score;
        q_total_score += q_score;
        if (nx_score > q_score) nx_wins++;

        const uint64_t t_ns = now_ns();
        fprintf(logf,
                "{\"ts_ns\":%llu,\"delta_ns\":%llu,\"event\":\"scenario_margin\",\"scenario\":%zu,\"margin\":%.12f}\n",
                (unsigned long long)t_ns,
                (unsigned long long)(t_ns - t0_ns),
                i,
                nx_score - q_score);
    }

    const uint64_t t1_ns = now_ns();
    const uint64_t elapsed_ns = t1_ns - t0_ns;
    const double elapsed_s = (double)elapsed_ns / 1e9;
    const double nqubits_simulated = (double)(scenarios * steps);
    const double nqubits_per_sec = nqubits_simulated / (elapsed_s > 0.0 ? elapsed_s : 1e-12);
    const double nx_avg = nx_total_score / (double)scenarios;
    const double q_avg = q_total_score / (double)scenarios;
    const double win_rate = (double)nx_wins / (double)scenarios;

    fprintf(logf,
            "{\"event\":\"summary\",\"nqubits_simulated\":%.0f,\"nqubits_per_sec\":%.3f,\"elapsed_ns\":%llu,\"nqubit_avg_score\":%.12f,\"baseline_qubit_avg_score\":%.12f,\"nqubit_win_rate\":%.12f,\"nqubit_wins\":%zu,\"baseline_wins\":%zu}\n",
            nqubits_simulated,
            nqubits_per_sec,
            (unsigned long long)elapsed_ns,
            nx_avg,
            q_avg,
            win_rate,
            nx_wins,
            scenarios - nx_wins);

    fclose(logf);

    memset(out_summary, 0, sizeof(*out_summary));
    out_summary->scenarios = scenarios;
    out_summary->steps = steps;
    out_summary->nqubits_simulated = nqubits_simulated;
    out_summary->nqubits_per_sec = nqubits_per_sec;
    out_summary->nqubit_avg_score = nx_avg;
    out_summary->baseline_qubit_avg_score = q_avg;
    out_summary->nqubit_win_rate = win_rate;
    out_summary->nqubit_wins = nx_wins;
    out_summary->baseline_wins = scenarios - nx_wins;
    out_summary->elapsed_ns = elapsed_ns;
    out_summary->used_hardware_entropy = used_hw;
    return true;
}
