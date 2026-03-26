#include "../hardware_noise.h"

#include <stdio.h>
#include <stdlib.h>

int main(void) {
    const size_t samples = 20000U;
    double *values = (double *)malloc(samples * sizeof(double));
    if (values == NULL) {
        fprintf(stderr, "allocation failed\n");
        return 1;
    }

    hardware_noise_stats_t stats;
    if (!hardware_noise_capture(0xDEADBEEFULL, samples, values, &stats)) {
        free(values);
        fprintf(stderr, "hardware_noise_capture failed\n");
        return 1;
    }

    FILE *csv = fopen("results/hardware_noise_samples_v4.csv", "w");
    FILE *log = fopen("results/hardware_noise_forensic_v4.log", "w");
    if (csv == NULL || log == NULL) {
        if (csv != NULL) fclose(csv);
        if (log != NULL) fclose(log);
        free(values);
        fprintf(stderr, "cannot open output files\n");
        return 1;
    }

    fprintf(csv, "idx,noise\n");
    for (size_t i = 0U; i < samples; ++i) {
        fprintf(csv, "%zu,%.12f\n", i, values[i]);
    }

    fprintf(log, "# HARDWARE NOISE FORENSIC V4 START\n");
    fprintf(log, "samples=%zu\n", stats.samples);
    fprintf(log, "mean=%.12f\n", stats.mean);
    fprintf(log, "stddev=%.12f\n", stats.stddev);
    fprintf(log, "min=%.12f\n", stats.min_value);
    fprintf(log, "max=%.12f\n", stats.max_value);
    fprintf(log, "p01=%.12f\n", stats.p01);
    fprintf(log, "p99=%.12f\n", stats.p99);
    fprintf(log, "# HARDWARE NOISE FORENSIC V4 END\n");

    fclose(csv);
    fclose(log);
    free(values);
    printf("hardware noise capture ok: %zu samples\n", samples);
    return 0;
}
