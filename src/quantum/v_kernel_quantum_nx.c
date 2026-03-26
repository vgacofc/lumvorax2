#define _POSIX_C_SOURCE 200809L
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static uint64_t now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static void log_jsonl(FILE *f, const char *event, double value, uint64_t run_t0_ns) {
    const uint64_t t_ns = now_ns();
    fprintf(f,
            "{\"ts_ns\":%llu,\"delta_ns\":%llu,\"event\":\"%s\",\"value\":%.12f}\n",
            (unsigned long long)t_ns,
            (unsigned long long)(t_ns - run_t0_ns),
            event,
            value);
}

static uint64_t prng_step(uint64_t *state) {
    uint64_t x = *state;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    *state = x;
    return x;
}

static double uniform01(uint64_t *state) {
    const uint64_t value = prng_step(state);
    return (double)(value >> 11) * (1.0 / 9007199254740992.0);
}

static double gaussian(uint64_t *state, double sigma) {
    const double u1 = fmax(uniform01(state), 1e-12);
    const double u2 = uniform01(state);
    const double radius = sqrt(-2.0 * log(u1));
    const double theta = 6.28318530717958647692 * u2;
    return sigma * radius * cos(theta);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s <output_jsonl>\n", argv[0]);
        return 2;
    }

    FILE *logf = fopen(argv[1], "w");
    if (!logf) {
        perror("fopen");
        return 1;
    }

    const uint64_t t0_ns = now_ns();
    const size_t scenarios = 360U;
    const size_t steps = 1400U;

    uint64_t rng_nx = 0xC0FFEE12345678ULL;
    uint64_t rng_q = 0xABCDEF456789ULL;

    double nx_total_score = 0.0;
    double q_total_score = 0.0;
    size_t nx_wins = 0U;

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
            const double lyapunov_push = lyapunov_gain * tanh(grad);
            nx_state += noise_nx + lyapunov_push;

            const double noise_q = gaussian(&rng_q, sigma * 0.7);
            q_state += 0.03 * (target - q_state) + noise_q;
        }

        const double nx_error = fabs(target - nx_state);
        const double q_error = fabs(target - q_state);
        const double nx_score = 1.0 / (1.0 + nx_error);
        const double q_score = 1.0 / (1.0 + q_error);

        nx_total_score += nx_score;
        q_total_score += q_score;
        if (nx_score > q_score) {
            nx_wins++;
        }

        if ((i % 60U) == 0U) {
            log_jsonl(logf, "scenario_margin", nx_score - q_score, t0_ns);
        }
    }

    const double nx_avg = nx_total_score / (double)scenarios;
    const double q_avg = q_total_score / (double)scenarios;
    const double win_rate = (double)nx_wins / (double)scenarios;

    log_jsonl(logf, "nqubit_avg_score", nx_avg, t0_ns);
    log_jsonl(logf, "baseline_qubit_avg_score", q_avg, t0_ns);
    log_jsonl(logf, "nqubit_win_rate", win_rate, t0_ns);
    log_jsonl(logf, "nqubits_simulated", (double)(scenarios * steps), t0_ns);
    log_jsonl(logf, "qubits_simulated_equivalent", (double)(scenarios * steps), t0_ns);

    fclose(logf);

    printf("nqubit_avg_score=%.9f\n", nx_avg);
    printf("baseline_qubit_avg_score=%.9f\n", q_avg);
    printf("nqubit_win_rate=%.9f\n", win_rate);
    printf("nqubits_simulated=%zu\n", scenarios * steps);
    return 0;
}
