#include "../nqbit_nx.h"

#include <inttypes.h>
#include <math.h>
#include <stdio.h>

int main(void) {
    const size_t tiers[] = {10U, 100U, 1000U, 10000U};
    const size_t tier_count = sizeof(tiers) / sizeof(tiers[0]);

    FILE *log = fopen("results/test_extended_forensic.log", "w");
    if (log == NULL) {
        fprintf(stderr, "[extended] Impossible d'ouvrir results/test_extended_forensic.log\n");
        return 1;
    }
    fprintf(log, "# EXTENDED TEST FORENSIC START\n");

    size_t total = 0U;
    size_t pass = 0U;

    for (size_t t = 0U; t < tier_count; ++t) {
        const size_t scenarios = tiers[t];
        nqbit_config_t base;
        nqbit_default_config(&base);
        size_t tier_pass = 0U;

        for (size_t i = 0U; i < scenarios; ++i) {
            ++total;
            nqbit_test_case_t tc;
            tc.test_id = (uint64_t)(t * 100000U + i);
            tc.initial = -2.0 + 4.0 * ((double)i / (double)scenarios);
            tc.target = 0.65 + 0.5 * (double)(i % 17U) / 16.0;
            tc.config = base;
            tc.config.seed = 0xBEEF000ULL + (uint64_t)i * 7919ULL + (uint64_t)t * 131071ULL;
            tc.config.junction_noise_sigma = 0.008 + 0.0003 * (double)(i % 73U);
            tc.config.thermal_factor = 0.9 + 0.01 * (double)(i % 61U);
            tc.config.lyapunov_gain = 0.20 + 0.001 * (double)(i % 41U);

            nqbit_result_t out;
            if (!nqbit_run_test_case(&tc, &out)) {
                fprintf(stderr, "[extended] Echec run_test_case tier=%zu idx=%zu\n", scenarios, i);
                fprintf(log, "tier=%zu idx=%zu status=FAIL reason=run_test_case\n", scenarios, i);
                continue;
            }

            if (!(isfinite(out.nx_score) && isfinite(out.classical_score) &&
                  isfinite(out.nx_energy) && isfinite(out.classical_energy))) {
                fprintf(stderr, "[extended] Valeurs non-finies tier=%zu idx=%zu\n", scenarios, i);
                fprintf(log, "tier=%zu idx=%zu status=FAIL reason=non_finite\n", scenarios, i);
                continue;
            }

            if (!(out.nx_score > 0.0 && out.classical_score > 0.0)) {
                fprintf(stderr, "[extended] Scores non positifs tier=%zu idx=%zu\n", scenarios, i);
                fprintf(log, "tier=%zu idx=%zu status=FAIL reason=non_positive_scores\n", scenarios, i);
                continue;
            }

            if (out.steps < 1200U || out.steps > 1499U) {
                fprintf(stderr, "[extended] Steps hors plage tier=%zu idx=%zu steps=%zu\n", scenarios, i, out.steps);
                fprintf(log, "tier=%zu idx=%zu status=FAIL reason=steps_range steps=%zu\n", scenarios, i, out.steps);
                continue;
            }

            ++pass;
            ++tier_pass;
        }

        fprintf(log, "tier=%zu status=SUMMARY pass=%zu total=%zu\n", scenarios, tier_pass, scenarios);
    }

    fprintf(log, "summary pass=%zu total=%zu\n", pass, total);
    fprintf(log, "# EXTENDED TEST FORENSIC END\n");
    fclose(log);

    printf("Tests etendus progressifs: %zu/%zu passes\n", pass, total);
    return (pass == total) ? 0 : 1;
}
