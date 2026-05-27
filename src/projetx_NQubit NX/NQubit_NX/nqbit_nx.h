#ifndef NQBIT_NX_H
#define NQBIT_NX_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define NQUBIT_NX_VERSION "0.1.0"
#define NQUBIT_MAX_DIMENSION 32U

typedef enum {
    NX_PHASE_INIT = 0,
    NX_PHASE_NOISE_INJECTION = 1,
    NX_PHASE_LYAPUNOV_GUIDANCE = 2,
    NX_PHASE_COLLAPSE = 3,
    NX_PHASE_DONE = 4
} nx_phase_e;

typedef struct {
    double junction_noise_sigma;
    double thermal_factor;
    double lyapunov_gain;
    uint32_t dimensions;
    uint64_t seed;
} nqbit_config_t;

typedef struct {
    double nx_energy;
    double classical_energy;
    double nx_score;
    double classical_score;
    bool nx_better;
    size_t steps;
} nqbit_result_t;

typedef struct {
    uint64_t test_id;
    double target;
    double initial;
    nqbit_config_t config;
} nqbit_test_case_t;

void nqbit_default_config(nqbit_config_t *config);
void nqbit_set_seed(nqbit_config_t *config, uint64_t seed);

nqbit_result_t nqbit_run_single(const nqbit_config_t *config,
                                double target,
                                double initial,
                                size_t steps);

bool nqbit_run_test_case(const nqbit_test_case_t *test_case,
                         nqbit_result_t *out_result);

#endif
