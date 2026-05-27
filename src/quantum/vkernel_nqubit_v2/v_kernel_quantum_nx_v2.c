#define _POSIX_C_SOURCE 200809L
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static uint64_t now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
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
        fprintf(stderr, "usage: %s <output_jsonl> [scenarios] [steps]\n", argv[0]);
        return 2;
    }

    size_t scenarios = 360U;
    size_t steps = 1400U;
    if (argc >= 3) {
        scenarios = (size_t)strtoull(argv[2], NULL, 10);
    }
    if (argc >= 4) {
        steps = (size_t)strtoull(argv[3], NULL, 10);
    }
    if (scenarios == 0U || steps == 0U) {
        fprintf(stderr, "scenarios and steps must be > 0\n");
        return 3;
    }

    FILE *logf = fopen(argv[1], "w");
    if (!logf) {
        perror("fopen");
        return 1;
    }

    const uint64_t t0_ns = now_ns();

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
            nx_state += noise_nx + lyapunov_gain * tanh(grad);

            const double noise_q = gaussian(&rng_q, sigma * 0.7);
            q_state += 0.03 * (target - q_state) + noise_q;
        }

        const double nx_score = 1.0 / (1.0 + fabs(target - nx_state));
        const double q_score = 1.0 / (1.0 + fabs(target - q_state));
        nx_total_score += nx_score;
        q_total_score += q_score;
        if (nx_score > q_score) nx_wins++;

        if ((i % 64U) == 0U) {
            const uint64_t t_ns = now_ns();
            fprintf(logf,
                    "{\"ts_ns\":%llu,\"delta_ns\":%llu,\"event\":\"scenario_margin\",\"scenario\":%zu,\"margin\":%.12f}\n",
                    (unsigned long long)t_ns,
                    (unsigned long long)(t_ns - t0_ns),
                    i,
                    nx_score - q_score);
        }
    }

    const uint64_t t1_ns = now_ns();
    const uint64_t elapsed_ns = t1_ns - t0_ns;
    const double elapsed_s = (double)elapsed_ns / 1e9;
    const double nqubits_simulated = (double)(scenarios * steps);
    const double nqubits_per_sec = nqubits_simulated / (elapsed_s > 0.0 ? elapsed_s : 1e-12);
    const double nx_avg = nx_total_score / (double)scenarios;
    const double q_avg = q_total_score / (double)scenarios;
    const double win_rate = (double)nx_wins / (double)scenarios;

    fprintf(logf, "{\"event\":\"summary\",\"nqubits_simulated\":%.0f,\"nqubits_per_sec\":%.3f,\"elapsed_ns\":%llu,\"nqubit_avg_score\":%.12f,\"baseline_qubit_avg_score\":%.12f,\"nqubit_win_rate\":%.12f}\n",
            nqubits_simulated,
            nqubits_per_sec,
            (unsigned long long)elapsed_ns,
            nx_avg,
            q_avg,
            win_rate);
    fclose(logf);

    printf("scenarios=%zu\n", scenarios);
    printf("steps=%zu\n", steps);
    printf("nqubits_simulated=%.0f\n", nqubits_simulated);
    printf("nqubits_per_sec=%.3f\n", nqubits_per_sec);
    printf("elapsed_ns=%llu\n", (unsigned long long)elapsed_ns);
    printf("nqubit_avg_score=%.9f\n", nx_avg);
    printf("baseline_qubit_avg_score=%.9f\n", q_avg);
    printf("nqubit_win_rate=%.9f\n", win_rate);
    return 0;
}
