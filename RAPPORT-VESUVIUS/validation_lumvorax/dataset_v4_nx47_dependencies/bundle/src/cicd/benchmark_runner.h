#ifndef BENCHMARK_RUNNER_H
#define BENCHMARK_RUNNER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define BENCHMARK_VERSION "1.0.0"
#define BENCHMARK_MAX_NAME 128
#define BENCHMARK_DEFAULT_ITERATIONS 10000
#define BENCHMARK_WARMUP_ITERATIONS 1000

typedef struct {
    uint64_t min_ns;
    uint64_t max_ns;
    uint64_t total_ns;
    uint64_t mean_ns;
    uint64_t median_ns;
    uint64_t stddev_ns;
    uint64_t p99_ns;
    double ops_per_second;
    size_t iterations;
} benchmark_stats_t;

typedef struct {
    char name[BENCHMARK_MAX_NAME];
    void (*setup)(void* context);
    void (*teardown)(void* context);
    void (*benchmark)(void* context);
    void* context;
    size_t iterations;
    size_t warmup_iterations;
    benchmark_stats_t stats;
    bool completed;
} benchmark_t;

typedef struct {
    benchmark_t* benchmarks;
    size_t benchmark_count;
    size_t benchmark_capacity;
    uint64_t total_time_ns;
    const char* suite_name;
    bool verbose;
} benchmark_suite_t;

typedef struct {
    const char* baseline_name;
    const char* current_name;
    benchmark_stats_t baseline;
    benchmark_stats_t current;
    double speedup;
    double regression_percent;
    bool is_regression;
    double threshold_percent;
} benchmark_comparison_t;

benchmark_suite_t* benchmark_suite_create(const char* name);
void benchmark_suite_destroy(benchmark_suite_t* suite);
int benchmark_suite_add(benchmark_suite_t* suite, const char* name,
                        void (*benchmark)(void* context), void* context);
int benchmark_suite_add_full(benchmark_suite_t* suite, const char* name,
                              void (*setup)(void* context),
                              void (*benchmark)(void* context),
                              void (*teardown)(void* context),
                              void* context, size_t iterations);
int benchmark_suite_run(benchmark_suite_t* suite);
int benchmark_suite_run_single(benchmark_suite_t* suite, const char* name);
benchmark_stats_t* benchmark_suite_get_stats(benchmark_suite_t* suite, const char* name);
int benchmark_suite_export_json(benchmark_suite_t* suite, char* buffer, size_t buffer_size);
int benchmark_suite_export_csv(benchmark_suite_t* suite, char* buffer, size_t buffer_size);
int benchmark_compare(const benchmark_stats_t* baseline, const benchmark_stats_t* current,
                      double threshold_percent, benchmark_comparison_t* result);
void benchmark_print_stats(const benchmark_stats_t* stats, const char* name);
void benchmark_suite_set_verbose(benchmark_suite_t* suite, bool verbose);

#endif
