#include "nqbit_nx.h"

#include <math.h>

static uint64_t nx_prng_step(uint64_t *state) {
    uint64_t x = *state;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    *state = x;
    return x;
}

static double nx_uniform(uint64_t *state) {
    const uint64_t value = nx_prng_step(state);
    return (double)(value >> 11) * (1.0 / 9007199254740992.0);
}

static double nx_gaussian(uint64_t *state, double sigma) {
    const double u1 = fmax(nx_uniform(state), 1e-12);
    const double u2 = nx_uniform(state);
    const double radius = sqrt(-2.0 * log(u1));
    const double theta = 6.28318530717958647692 * u2;
    return sigma * radius * cos(theta);
}

void nqbit_default_config(nqbit_config_t *config) {
    if (config == NULL) {
        return;
    }
    config->junction_noise_sigma = 0.05;
    config->thermal_factor = 1.2;
    config->lyapunov_gain = 0.25;
    config->dimensions = 4U;
    config->seed = 0xC0FFEEULL;
}

void nqbit_set_seed(nqbit_config_t *config, uint64_t seed) {
    if (config == NULL) {
        return;
    }
    config->seed = seed;
}

nqbit_result_t nqbit_run_single(const nqbit_config_t *config,
                                double target,
                                double initial,
                                size_t steps) {
    nqbit_result_t result;
    result.nx_energy = 0.0;
    result.classical_energy = 0.0;
    result.nx_score = 0.0;
    result.classical_score = 0.0;
    result.nx_better = false;
    result.steps = steps;

    if (config == NULL || steps == 0U) {
        return result;
    }

    uint64_t rng_nx = config->seed ^ 0x9E3779B185EBCA87ULL;
    uint64_t rng_classic = config->seed ^ 0xD1B54A32D192ED03ULL;

    double nx_state = initial;
    double classical_state = initial;

    for (size_t i = 0; i < steps; ++i) {
        const double noise = nx_gaussian(&rng_nx, config->junction_noise_sigma * config->thermal_factor);
        const double grad = target - nx_state;
        const double lyapunov_push = config->lyapunov_gain * tanh(grad);

        nx_state += noise + lyapunov_push;

        const double fake_superposition = nx_gaussian(&rng_classic, config->junction_noise_sigma * 0.7);
        classical_state += 0.03 * (target - classical_state) + fake_superposition;

        result.nx_energy += fabs(noise);
        result.classical_energy += fabs(fake_superposition);
    }

    const double nx_error = fabs(target - nx_state);
    const double classic_error = fabs(target - classical_state);

    result.nx_score = 1.0 / (1.0 + nx_error + 0.05 * result.nx_energy);
    result.classical_score = 1.0 / (1.0 + classic_error + 0.20 * result.classical_energy);
    result.nx_better = result.nx_score > result.classical_score;

    return result;
}

bool nqbit_run_test_case(const nqbit_test_case_t *test_case,
                         nqbit_result_t *out_result) {
    if (test_case == NULL || out_result == NULL) {
        return false;
    }
    *out_result = nqbit_run_single(&test_case->config,
                                   test_case->target,
                                   test_case->initial,
                                   1200U + (size_t)(test_case->test_id % 300U));
    return true;
}
