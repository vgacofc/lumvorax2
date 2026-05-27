#include "../nqbit_nx.h"

#include <math.h>
#include <stdio.h>

int main(void) {
    nqbit_config_t cfg;
    nqbit_default_config(&cfg);

    size_t pass = 0U;
    size_t total = 0U;

    for (size_t i = 0; i < 360U; ++i) {
        ++total;
        nqbit_test_case_t tc;
        tc.test_id = (uint64_t)i;
        tc.target = 1.0 + 0.05 * (double)(i % 7U);
        tc.initial = -1.0 + 0.01 * (double)i;
        tc.config = cfg;
        tc.config.seed = 1234ULL + (uint64_t)i * 13ULL;
        tc.config.junction_noise_sigma = 0.01 + 0.0005 * (double)(i % 19U);
        tc.config.thermal_factor = 1.0 + 0.01 * (double)(i % 21U);

        nqbit_result_t out;
        if (!nqbit_run_test_case(&tc, &out)) {
            fprintf(stderr, "Echec test %zu: run_test_case false\n", i);
            continue;
        }

        if (!(out.nx_score > 0.0 && out.classical_score > 0.0)) {
            fprintf(stderr, "Echec test %zu: score non positif\n", i);
            continue;
        }

        if (!(isfinite(out.nx_energy) && isfinite(out.classical_energy))) {
            fprintf(stderr, "Echec test %zu: energie non-finie\n", i);
            continue;
        }

        ++pass;
    }

    printf("Tests unitaires: %zu/%zu passes\n", pass, total);
    return (pass == total) ? 0 : 1;
}
