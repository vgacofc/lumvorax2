#include "benchmark_runner.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

static uint64_t get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

static int compare_uint64(const void* a, const void* b) {
    uint64_t va = *(const uint64_t*)a;
    uint64_t vb = *(const uint64_t*)b;
    if (va < vb) return -1;
    if (va > vb) return 1;
    return 0;
}

static void calculate_stats(uint64_t* times, size_t count, benchmark_stats_t* stats) {
    if (!times || count == 0 || !stats) return;
    
    qsort(times, count, sizeof(uint64_t), compare_uint64);
    
    stats->min_ns = times[0];
    stats->max_ns = times[count - 1];
    stats->median_ns = times[count / 2];
    stats->iterations = count;
    
    uint64_t sum = 0;
    for (size_t i = 0; i < count; i++) {
        sum += times[i];
    }
    stats->total_ns = sum;
    stats->mean_ns = sum / count;
    
    double variance = 0;
    for (size_t i = 0; i < count; i++) {
        double diff = (double)times[i] - (double)stats->mean_ns;
        variance += diff * diff;
    }
    variance /= count;
    stats->stddev_ns = (uint64_t)sqrt(variance);
    
    size_t p99_idx = (size_t)(count * 0.99);
    if (p99_idx >= count) p99_idx = count - 1;
    stats->p99_ns = times[p99_idx];
    
    if (stats->mean_ns > 0) {
        stats->ops_per_second = 1e9 / (double)stats->mean_ns;
    }
}

benchmark_suite_t* benchmark_suite_create(const char* name) {
    benchmark_suite_t* suite = (benchmark_suite_t*)calloc(1, sizeof(benchmark_suite_t));
    if (!suite) return NULL;
    
    suite->benchmark_capacity = 64;
    suite->benchmarks = (benchmark_t*)calloc(suite->benchmark_capacity, sizeof(benchmark_t));
    if (!suite->benchmarks) {
        free(suite);
        return NULL;
    }
    
    suite->suite_name = name;
    suite->verbose = true;
    
    return suite;
}

void benchmark_suite_destroy(benchmark_suite_t* suite) {
    if (suite) {
        free(suite->benchmarks);
        free(suite);
    }
}

int benchmark_suite_add(benchmark_suite_t* suite, const char* name,
                        void (*benchmark)(void* context), void* context) {
    return benchmark_suite_add_full(suite, name, NULL, benchmark, NULL, context,
                                     BENCHMARK_DEFAULT_ITERATIONS);
}

int benchmark_suite_add_full(benchmark_suite_t* suite, const char* name,
                              void (*setup)(void* context),
                              void (*benchmark)(void* context),
                              void (*teardown)(void* context),
                              void* context, size_t iterations) {
    if (!suite || !name || !benchmark) return -1;
    
    if (suite->benchmark_count >= suite->benchmark_capacity) {
        size_t new_cap = suite->benchmark_capacity * 2;
        benchmark_t* new_benchmarks = (benchmark_t*)realloc(
            suite->benchmarks, new_cap * sizeof(benchmark_t));
        if (!new_benchmarks) return -2;
        suite->benchmarks = new_benchmarks;
        suite->benchmark_capacity = new_cap;
    }
    
    benchmark_t* b = &suite->benchmarks[suite->benchmark_count];
    strncpy(b->name, name, BENCHMARK_MAX_NAME - 1);
    b->setup = setup;
    b->benchmark = benchmark;
    b->teardown = teardown;
    b->context = context;
    b->iterations = iterations;
    b->warmup_iterations = BENCHMARK_WARMUP_ITERATIONS;
    b->completed = false;
    
    suite->benchmark_count++;
    return 0;
}

static int run_benchmark(benchmark_t* b, bool verbose) {
    if (!b || !b->benchmark) return -1;
    
    if (b->setup) {
        b->setup(b->context);
    }
    
    for (size_t i = 0; i < b->warmup_iterations; i++) {
        b->benchmark(b->context);
    }
    
    uint64_t* times = (uint64_t*)malloc(b->iterations * sizeof(uint64_t));
    if (!times) return -2;
    
    for (size_t i = 0; i < b->iterations; i++) {
        uint64_t start = get_time_ns();
        b->benchmark(b->context);
        uint64_t end = get_time_ns();
        times[i] = end - start;
    }
    
    calculate_stats(times, b->iterations, &b->stats);
    free(times);
    
    if (b->teardown) {
        b->teardown(b->context);
    }
    
    b->completed = true;
    
    if (verbose) {
        benchmark_print_stats(&b->stats, b->name);
    }
    
    return 0;
}

int benchmark_suite_run(benchmark_suite_t* suite) {
    if (!suite) return -1;
    
    uint64_t suite_start = get_time_ns();
    
    if (suite->verbose) {
        printf("\n=== BENCHMARK SUITE: %s ===\n\n", 
               suite->suite_name ? suite->suite_name : "unnamed");
    }
    
    int errors = 0;
    for (size_t i = 0; i < suite->benchmark_count; i++) {
        if (run_benchmark(&suite->benchmarks[i], suite->verbose) != 0) {
            errors++;
        }
    }
    
    suite->total_time_ns = get_time_ns() - suite_start;
    
    if (suite->verbose) {
        printf("\n=== SUITE COMPLETE: %zu benchmarks in %.2f ms ===\n",
               suite->benchmark_count, suite->total_time_ns / 1e6);
    }
    
    return errors;
}

int benchmark_suite_run_single(benchmark_suite_t* suite, const char* name) {
    if (!suite || !name) return -1;
    
    for (size_t i = 0; i < suite->benchmark_count; i++) {
        if (strcmp(suite->benchmarks[i].name, name) == 0) {
            return run_benchmark(&suite->benchmarks[i], suite->verbose);
        }
    }
    
    return -2;
}

benchmark_stats_t* benchmark_suite_get_stats(benchmark_suite_t* suite, const char* name) {
    if (!suite || !name) return NULL;
    
    for (size_t i = 0; i < suite->benchmark_count; i++) {
        if (strcmp(suite->benchmarks[i].name, name) == 0) {
            return &suite->benchmarks[i].stats;
        }
    }
    
    return NULL;
}

int benchmark_suite_export_json(benchmark_suite_t* suite, char* buffer, size_t buffer_size) {
    if (!suite || !buffer || buffer_size == 0) return -1;
    
    char* ptr = buffer;
    size_t remaining = buffer_size;
    int written;
    
    written = snprintf(ptr, remaining, "{\n  \"suite\": \"%s\",\n  \"benchmarks\": [\n",
                       suite->suite_name ? suite->suite_name : "unnamed");
    if (written < 0 || (size_t)written >= remaining) return -2;
    ptr += written;
    remaining -= written;
    
    for (size_t i = 0; i < suite->benchmark_count; i++) {
        benchmark_t* b = &suite->benchmarks[i];
        
        written = snprintf(ptr, remaining,
                           "    {\n"
                           "      \"name\": \"%s\",\n"
                           "      \"mean_ns\": %lu,\n"
                           "      \"min_ns\": %lu,\n"
                           "      \"max_ns\": %lu,\n"
                           "      \"ops_per_second\": %.2f\n"
                           "    }%s\n",
                           b->name, b->stats.mean_ns, b->stats.min_ns,
                           b->stats.max_ns, b->stats.ops_per_second,
                           (i < suite->benchmark_count - 1) ? "," : "");
        if (written < 0 || (size_t)written >= remaining) return -3;
        ptr += written;
        remaining -= written;
    }
    
    written = snprintf(ptr, remaining, "  ]\n}\n");
    if (written < 0 || (size_t)written >= remaining) return -4;
    
    return (int)(buffer_size - remaining + written);
}

int benchmark_suite_export_csv(benchmark_suite_t* suite, char* buffer, size_t buffer_size) {
    if (!suite || !buffer || buffer_size == 0) return -1;
    
    char* ptr = buffer;
    size_t remaining = buffer_size;
    int written;
    
    written = snprintf(ptr, remaining, "name,mean_ns,min_ns,max_ns,median_ns,stddev_ns,p99_ns,ops_per_sec\n");
    if (written < 0 || (size_t)written >= remaining) return -2;
    ptr += written;
    remaining -= written;
    
    for (size_t i = 0; i < suite->benchmark_count; i++) {
        benchmark_t* b = &suite->benchmarks[i];
        
        written = snprintf(ptr, remaining, "%s,%lu,%lu,%lu,%lu,%lu,%lu,%.2f\n",
                           b->name, b->stats.mean_ns, b->stats.min_ns,
                           b->stats.max_ns, b->stats.median_ns, b->stats.stddev_ns,
                           b->stats.p99_ns, b->stats.ops_per_second);
        if (written < 0 || (size_t)written >= remaining) return -3;
        ptr += written;
        remaining -= written;
    }
    
    return (int)(buffer_size - remaining);
}

int benchmark_compare(const benchmark_stats_t* baseline, const benchmark_stats_t* current,
                      double threshold_percent, benchmark_comparison_t* result) {
    if (!baseline || !current || !result) return -1;
    
    result->baseline = *baseline;
    result->current = *current;
    result->threshold_percent = threshold_percent;
    
    if (baseline->mean_ns > 0) {
        result->speedup = (double)baseline->mean_ns / (double)current->mean_ns;
        result->regression_percent = ((double)current->mean_ns - (double)baseline->mean_ns) 
                                     / (double)baseline->mean_ns * 100.0;
    } else {
        result->speedup = 1.0;
        result->regression_percent = 0.0;
    }
    
    result->is_regression = (result->regression_percent > threshold_percent);
    
    return 0;
}

void benchmark_print_stats(const benchmark_stats_t* stats, const char* name) {
    if (!stats) return;
    
    printf("%-40s: mean=%8lu ns, min=%8lu ns, max=%8lu ns, %.2f M ops/s\n",
           name ? name : "benchmark",
           stats->mean_ns, stats->min_ns, stats->max_ns,
           stats->ops_per_second / 1e6);
}

void benchmark_suite_set_verbose(benchmark_suite_t* suite, bool verbose) {
    if (suite) suite->verbose = verbose;
}
