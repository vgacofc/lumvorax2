#define _POSIX_C_SOURCE 200809L
#include "hardware_noise.h"

#include <fcntl.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

static int cmp_double(const void *a, const void *b) {
    const double da = *(const double *)a;
    const double db = *(const double *)b;
    if (da < db) {
        return -1;
    }
    if (da > db) {
        return 1;
    }
    return 0;
}

static double to_unit(uint64_t x) {
    return ((double)(x >> 11) * (1.0 / 9007199254740992.0)) * 2.0 - 1.0;
}

int hardware_noise_capture(uint64_t seed, size_t samples, double *out_values, hardware_noise_stats_t *out_stats) {
    if (samples == 0U || out_values == NULL || out_stats == NULL) {
        return 0;
    }

    int fd = open("/dev/urandom", O_RDONLY);
    if (fd < 0) {
        return 0;
    }

    uint64_t prev_ns = (uint64_t)time(NULL) * 1000000000ULL;
    struct timespec ts;

    double mean = 0.0;
    double m2 = 0.0;
    double min_v = 1e9;
    double max_v = -1e9;

    for (size_t i = 0U; i < samples; ++i) {
        uint64_t rnd = 0U;
        ssize_t got = read(fd, &rnd, sizeof(rnd));
        if (got != (ssize_t)sizeof(rnd)) {
            close(fd);
            return 0;
        }

        clock_gettime(CLOCK_MONOTONIC, &ts);
        const uint64_t now_ns = (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
        const uint64_t delta_ns = now_ns - prev_ns;
        prev_ns = now_ns;

        uint64_t mixed = rnd ^ ((uint64_t)delta_ns << 17) ^ (seed + (uint64_t)i * 7919ULL);
        const double v = to_unit(mixed);
        out_values[i] = v;

        const double d = v - mean;
        mean += d / (double)(i + 1U);
        const double d2 = v - mean;
        m2 += d * d2;

        if (v < min_v) {
            min_v = v;
        }
        if (v > max_v) {
            max_v = v;
        }
    }

    close(fd);

    double *tmp = (double *)malloc(samples * sizeof(double));
    if (tmp == NULL) {
        return 0;
    }
    for (size_t i = 0U; i < samples; ++i) {
        tmp[i] = out_values[i];
    }
    qsort(tmp, samples, sizeof(double), cmp_double);

    const size_t idx01 = (samples - 1U) / 100U;
    const size_t idx99 = ((samples - 1U) * 99U) / 100U;

    out_stats->samples = samples;
    out_stats->mean = mean;
    out_stats->stddev = (samples > 1U) ? sqrt(m2 / (double)(samples - 1U)) : 0.0;
    out_stats->min_value = min_v;
    out_stats->max_value = max_v;
    out_stats->p01 = tmp[idx01];
    out_stats->p99 = tmp[idx99];

    free(tmp);
    return 1;
}
