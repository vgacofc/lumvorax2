#ifndef HARDWARE_NOISE_H
#define HARDWARE_NOISE_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    size_t samples;
    double mean;
    double stddev;
    double min_value;
    double max_value;
    double p01;
    double p99;
} hardware_noise_stats_t;

int hardware_noise_capture(uint64_t seed, size_t samples, double *out_values, hardware_noise_stats_t *out_stats);

#endif
