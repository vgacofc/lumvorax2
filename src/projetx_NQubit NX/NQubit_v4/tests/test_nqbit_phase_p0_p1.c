#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define SCENARIOS 360U
#define RUNS_PER_SCENARIO 30U
#define BASE_STEPS 1300U

typedef struct {
    double nx_mean;
    double base_mean;
    double win_rate;
    double delta_mean;
    double delta_std;
    double delta_ci95_low;
    double delta_ci95_high;
} experiment_result_t;

typedef struct {
    size_t n;
    double mean;
    double m2;
} online_stats_t;

static void online_push(online_stats_t *s, double x) {
    s->n += 1U;
    const double delta = x - s->mean;
    s->mean += delta / (double)s->n;
    const double delta2 = x - s->mean;
    s->m2 += delta * delta2;
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
    return sigma * sqrt(-2.0 * log(u1)) * cos(6.28318530717958647692 * u2);
}

static double cauchy_clipped(uint64_t *state, double scale, double clip) {
    const double u = uniform01(state);
    const double x = scale * tan(3.14159265358979323846 * (u - 0.5));
    if (x > clip) {
        return clip;
    }
    if (x < -clip) {
        return -clip;
    }
    return x;
}

static double score_with_coeff(double error, double energy, double coeff) {
    return 1.0 / (1.0 + fabs(error) + coeff * energy);
}

static experiment_result_t run_experiment(bool no_lyapunov,
                                          bool no_noise,
                                          bool impulsive_noise,
                                          bool symmetric_scoring,
                                          bool strong_baseline,
                                          bool ood_drift) {
    size_t wins = 0U;
    double sum_nx = 0.0;
    double sum_base = 0.0;
    online_stats_t delta_stats = {0U, 0.0, 0.0};

    for (size_t s = 0U; s < SCENARIOS; ++s) {
        for (size_t r = 0U; r < RUNS_PER_SCENARIO; ++r) {
            uint64_t seed = 0xABC000ULL + (uint64_t)s * 7919ULL + (uint64_t)r * 104729ULL;
            uint64_t rng_nx = seed ^ 0x9E3779B185EBCA87ULL;
            uint64_t rng_base = seed ^ 0xD1B54A32D192ED03ULL;

            const double initial = -1.5 + 3.0 * ((double)s / (double)SCENARIOS);
            const double target = 0.7 + 0.4 * (double)(s % 11U) / 10.0;
            const double base_sigma = no_noise ? 0.0 : (0.02 + 0.001 * (double)(s % 20U));
            const double thermal = 1.0 + 0.02 * (double)(s % 15U);
            const double lyap = no_lyapunov ? 0.0 : 0.25;

            double nx_state = initial;
            double base_state = initial;
            double nx_energy = 0.0;
            double base_energy = 0.0;

            for (size_t i = 0U; i < BASE_STEPS; ++i) {
                double local_thermal = thermal;
                if (ood_drift) {
                    local_thermal += 0.0003 * (double)i;
                }

                double nx_noise;
                if (impulsive_noise) {
                    nx_noise = gaussian(&rng_nx, base_sigma * local_thermal);
                    if ((i % 17U) == 0U) {
                        nx_noise += cauchy_clipped(&rng_nx, base_sigma * 2.0, 0.8);
                    }
                } else {
                    nx_noise = gaussian(&rng_nx, base_sigma * local_thermal);
                }

                const double grad = target - nx_state;
                nx_state += nx_noise + lyap * tanh(grad);
                nx_energy += fabs(nx_noise);

                double base_noise = gaussian(&rng_base, base_sigma * 0.7);
                if (ood_drift && (i % 23U) == 0U) {
                    base_noise += cauchy_clipped(&rng_base, base_sigma, 0.4);
                }

                if (strong_baseline) {
                    const double step_gain = 0.10 / (1.0 + 0.0005 * (double)i);
                    const double correction = step_gain * (target - base_state);
                    base_state += correction + base_noise;
                } else {
                    base_state += 0.03 * (target - base_state) + base_noise;
                }
                base_energy += fabs(base_noise);
            }

            const double nx_error = fabs(target - nx_state);
            const double base_error = fabs(target - base_state);
            const double nx_coeff = symmetric_scoring ? 0.10 : 0.05;
            const double base_coeff = symmetric_scoring ? 0.10 : 0.20;

            const double nx_score = score_with_coeff(nx_error, nx_energy, nx_coeff);
            const double base_score = score_with_coeff(base_error, base_energy, base_coeff);
            const double delta = nx_score - base_score;

            sum_nx += nx_score;
            sum_base += base_score;
            wins += (delta > 0.0) ? 1U : 0U;
            online_push(&delta_stats, delta);
        }
    }

    const double denom = (double)(SCENARIOS * RUNS_PER_SCENARIO);
    const double variance = (delta_stats.n > 1U) ? (delta_stats.m2 / (double)(delta_stats.n - 1U)) : 0.0;
    const double std = sqrt(variance);
    const double sem = (delta_stats.n > 0U) ? (std / sqrt((double)delta_stats.n)) : 0.0;
    const double ci = 1.96 * sem;

    experiment_result_t out;
    out.nx_mean = sum_nx / denom;
    out.base_mean = sum_base / denom;
    out.win_rate = (double)wins / denom;
    out.delta_mean = delta_stats.mean;
    out.delta_std = std;
    out.delta_ci95_low = delta_stats.mean - ci;
    out.delta_ci95_high = delta_stats.mean + ci;
    return out;
}

static void write_result(FILE *csv,
                         FILE *log,
                         const char *name,
                         experiment_result_t r) {
    const double delta_pct = (r.base_mean != 0.0) ? (r.delta_mean / r.base_mean) * 100.0 : 0.0;
    fprintf(csv, "%s,%.12f,%.12f,%.12f,%.6f,%.12f,%.12f,%.12f,%.12f\n",
            name,
            r.nx_mean,
            r.base_mean,
            r.delta_mean,
            delta_pct,
            r.win_rate,
            r.delta_std,
            r.delta_ci95_low,
            r.delta_ci95_high);
    fprintf(log,
            "experiment=%s nx_mean=%.12f base_mean=%.12f delta=%.12f delta_pct=%.6f win_rate=%.12f delta_std=%.12f ci95=[%.12f,%.12f]\n",
            name,
            r.nx_mean,
            r.base_mean,
            r.delta_mean,
            delta_pct,
            r.win_rate,
            r.delta_std,
            r.delta_ci95_low,
            r.delta_ci95_high);
}

int main(void) {
    FILE *csv = fopen("results/phase_p0_p1_metrics.csv", "w");
    FILE *log = fopen("results/phase_p0_p1_forensic.log", "w");
    if (csv == NULL || log == NULL) {
        fprintf(stderr, "Erreur ouverture resultats phase_p0_p1\n");
        if (csv != NULL) {
            fclose(csv);
        }
        if (log != NULL) {
            fclose(log);
        }
        return 1;
    }

    fprintf(csv, "experiment,nx_mean,base_mean,delta_abs,delta_pct,win_rate,delta_std,delta_ci95_low,delta_ci95_high\n");
    fprintf(log, "# PHASE_P0_P1 FORENSIC START\n");

    write_result(csv, log, "baseline_current", run_experiment(false, false, false, false, false, false));
    write_result(csv, log, "p0_no_lyapunov", run_experiment(true, false, false, false, false, false));
    write_result(csv, log, "p0_no_noise", run_experiment(false, true, false, false, false, false));
    write_result(csv, log, "p0_impulsive_noise", run_experiment(false, false, true, false, false, false));
    write_result(csv, log, "p0_symmetric_scoring", run_experiment(false, false, false, true, false, false));
    write_result(csv, log, "p1_strong_baseline", run_experiment(false, false, false, true, true, false));
    write_result(csv, log, "p1_ood_drift_heavy", run_experiment(false, false, true, true, true, true));

    fprintf(log, "# PHASE_P0_P1 FORENSIC END\n");
    fclose(csv);
    fclose(log);

    printf("Tests P0/P1 executes: %u scenarios x %u runs\n", SCENARIOS, RUNS_PER_SCENARIO);
    return 0;
}
