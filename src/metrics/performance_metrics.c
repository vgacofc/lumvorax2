// _GNU_SOURCE is already defined in Makefile
#include "performance_metrics.h"
#include "../lum/lum_core.h"  // CORRECTION PRIORITÉ 1.1: Pour sizeof(lum_t)
#include "../debug/memory_tracker.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <math.h>
#include <stdint.h>

// Constantes pour conversions métriques LUM (conforme STANDARD_NAMES)
// CORRECTION CRITIQUE PRIORITÉ 1.1: Taille dynamique authentique
#define LUM_SIZE_BYTES sizeof(lum_t)  // Dynamique = 48 bytes EXACT
#define LUM_SIZE_BITS (LUM_SIZE_BYTES * 8)  // 384 bits EXACT
#define BITS_PER_GIGABIT 1000000000ULL
#define MICROSECONDS_PER_SECOND 1000000ULL

// Conversion LUMs/seconde vers bits/seconde
uint64_t convert_lums_per_second_to_bits_per_second(uint64_t lums_per_second) {
    return lums_per_second * LUM_SIZE_BITS;
}

// Conversion LUMs/seconde vers Gigabits/seconde
double convert_lums_per_second_to_gigabits_per_second(uint64_t lums_per_second) {
    uint64_t bits_per_second = convert_lums_per_second_to_bits_per_second(lums_per_second);
    return (double)bits_per_second / (double)BITS_PER_GIGABIT;
}

// Calcul débit LUM authentique depuis métriques temporelles
uint64_t calculate_authentic_lum_throughput(uint64_t lum_count, uint64_t microseconds_elapsed) {
    if (microseconds_elapsed == 0) return 0;

    // LUMs/seconde = (lum_count * 1000000) / microseconds_elapsed
    return (lum_count * MICROSECONDS_PER_SECOND) / microseconds_elapsed;
}

// Static variables for system-wide metrics
static size_t global_memory_usage = 0;
static double global_cpu_usage = 0.0;
static struct timeval last_cpu_time = {0, 0};
// Removed unused static variable last_cpu_clock

performance_metrics_t* performance_metrics_create(void) {
    performance_metrics_t* metrics = TRACKED_MALLOC(sizeof(performance_metrics_t));
    if (!metrics) return NULL;

    metrics->total_operations = 0;
    clock_gettime(CLOCK_MONOTONIC, &metrics->start_time);
    metrics->last_update = time(NULL);
    metrics->memory_peak = 0;
    metrics->cpu_peak = 0.0;
    metrics->operation_count = 0;
    metrics->error_count = 0;
    metrics->metric_count = 0;
    metrics->is_initialized = true;
    metrics->cpu_usage = 0.0;
    metrics->memory_usage = 0;
    metrics->peak_memory = 0;

    // Initialize performance counters
    for (int i = 0; i < MAX_PERFORMANCE_COUNTERS; i++) {
        metrics->counters[i].name[0] = '\0';
        metrics->counters[i].value = 0.0;
        metrics->counters[i].type = METRIC_COUNTER;
        metrics->counters[i].is_active = false;
        metrics->counters[i].min_value = 0.0;
        metrics->counters[i].max_value = 0.0;
        metrics->counters[i].sum_value = 0.0;
        metrics->counters[i].count = 0;
        clock_gettime(CLOCK_MONOTONIC, &metrics->counters[i].last_updated);
    }

    return metrics;
}

void performance_metrics_destroy(performance_metrics_t* metrics) {
    if (metrics) {
        TRACKED_FREE(metrics);
    }
}

operation_timer_t* operation_timer_create(void) {
    operation_timer_t* timer = TRACKED_MALLOC(sizeof(operation_timer_t));
    if (!timer) return NULL;

    timer->start_time.tv_sec = 0;
    timer->start_time.tv_nsec = 0;
    timer->end_time.tv_sec = 0;
    timer->end_time.tv_nsec = 0;
    timer->is_running = false;
    timer->total_elapsed = 0.0;
    timer->elapsed_seconds = 0.0;

    return timer;
}

void operation_timer_destroy(operation_timer_t* timer) {
    if (timer) {
        TRACKED_FREE(timer);
    }
}

bool operation_timer_start(operation_timer_t* timer) {
    if (!timer) return false;

    if (clock_gettime(CLOCK_MONOTONIC, &timer->start_time) != 0) {
        return false;
    }

    timer->is_running = true;
    return true;
}

bool operation_timer_stop(operation_timer_t* timer) {
    if (!timer || !timer->is_running) return false;

    if (clock_gettime(CLOCK_MONOTONIC, &timer->end_time) != 0) {
        return false;
    }

    timer->is_running = false;

    // Calculate elapsed time in seconds
    double start_sec = timer->start_time.tv_sec + timer->start_time.tv_nsec / 1e9;
    double end_sec = timer->end_time.tv_sec + timer->end_time.tv_nsec / 1e9;
    timer->total_elapsed = end_sec - start_sec;

    return true;
}

double operation_timer_get_elapsed(operation_timer_t* timer) {
    if (!timer) return 0.0;
    return timer->total_elapsed;
}

size_t performance_metrics_get_memory_usage(void) {
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        // Convert kilobytes to bytes
        global_memory_usage = usage.ru_maxrss * 1024;
        return global_memory_usage;
    }
    return 0;
}

double performance_metrics_get_cpu_usage(void) {
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        double user_time = usage.ru_utime.tv_sec + usage.ru_utime.tv_usec / 1e6;
        double sys_time = usage.ru_stime.tv_sec + usage.ru_stime.tv_usec / 1e6;
        double total_cpu_time = user_time + sys_time;

        // Simple CPU usage calculation (not perfectly accurate but functional)
        global_cpu_usage = total_cpu_time * 100.0 / (time(NULL) - last_cpu_time.tv_sec + 1);
        if (global_cpu_usage > 100.0) global_cpu_usage = 100.0;

        gettimeofday(&last_cpu_time, NULL);
        return global_cpu_usage;
    }
    return 0.0;
}

bool performance_metrics_register(performance_metrics_t* metrics, const char* name, metric_type_e type) {
    if (!metrics || !name) return false;

    // Find empty slot
    for (int i = 0; i < MAX_PERFORMANCE_COUNTERS; i++) {
        if (!metrics->counters[i].is_active) {
            strncpy(metrics->counters[i].name, name, sizeof(metrics->counters[i].name) - 1);
            metrics->counters[i].name[sizeof(metrics->counters[i].name) - 1] = '\0';
            metrics->counters[i].type = type;
            metrics->counters[i].value = 0.0;
            metrics->counters[i].is_active = true;
            return true;
        }
    }

    return false; // No empty slots
}

bool performance_metrics_update_counter(performance_metrics_t* metrics, const char* name, double value) {
    if (!metrics || !name) return false;

    for (int i = 0; i < MAX_PERFORMANCE_COUNTERS; i++) {
        if (metrics->counters[i].is_active && strcmp(metrics->counters[i].name, name) == 0) {
            if (metrics->counters[i].type == METRIC_COUNTER) {
                metrics->counters[i].value += value;
            } else {
                metrics->counters[i].value = value;
            }
            return true;
        }
    }

    return false; // Counter not found
}

double performance_metrics_get_counter(performance_metrics_t* metrics, const char* name) {
    if (!metrics || !name) return 0.0;

    for (int i = 0; i < MAX_PERFORMANCE_COUNTERS; i++) {
        if (metrics->counters[i].is_active && strcmp(metrics->counters[i].name, name) == 0) {
            return metrics->counters[i].value;
        }
    }

    return 0.0; // Counter not found
}

void performance_metrics_print_summary(performance_metrics_t* metrics) {
    if (!metrics) return;

    printf("=== Performance Metrics Summary ===\n");
    printf("Total Operations: %zu\n", metrics->total_operations);
    printf("Start Time: %ld.%ld\n", metrics->start_time.tv_sec, metrics->start_time.tv_nsec);
    printf("Last Update: %ld\n", metrics->last_update);
    printf("Memory Peak: %zu bytes\n", metrics->memory_peak);
    printf("CPU Peak: %.2f%%\n", metrics->cpu_peak);
    printf("Operation Count: %zu\n", metrics->operation_count);
    printf("Error Count: %zu\n", metrics->error_count);

    printf("\nActive Counters:\n");
    for (int i = 0; i < MAX_PERFORMANCE_COUNTERS; i++) {
        if (metrics->counters[i].is_active) {
            printf("  %s: %.2f (%s)\n", 
                   metrics->counters[i].name,
                   metrics->counters[i].value,
                   metrics->counters[i].type == METRIC_COUNTER ? "counter" : "gauge");
        }
    }
}

void performance_metrics_update_system_stats(performance_metrics_t* metrics) {
    if (!metrics) return;

    size_t current_memory = performance_metrics_get_memory_usage();
    double current_cpu = performance_metrics_get_cpu_usage();

    if (current_memory > metrics->memory_peak) {
        metrics->memory_peak = current_memory;
    }

    if (current_cpu > metrics->cpu_peak) {
        metrics->cpu_peak = current_cpu;
    }

    metrics->last_update = time(NULL);
}

// Remove duplicate structure definitions

// Benchmark and profiling functions
benchmark_result_t* performance_benchmark_function(void (*func)(void), int iterations) {
    if (!func || iterations <= 0) return NULL;

    benchmark_result_t* result = TRACKED_MALLOC(sizeof(benchmark_result_t));
    if (!result) return NULL;

    operation_timer_t* timer = operation_timer_create();
    if (!timer) {
        TRACKED_FREE(result);
        return NULL;
    }

    double total_time = 0.0;
    double min_time = INFINITY;
    double max_time = 0.0;

    for (int i = 0; i < iterations; i++) {
        operation_timer_start(timer);
        func();
        operation_timer_stop(timer);

        double elapsed = operation_timer_get_elapsed(timer);
        total_time += elapsed;

        if (elapsed < min_time) min_time = elapsed;
        if (elapsed > max_time) max_time = elapsed;
    }

    result->iterations = iterations;
    result->total_time = total_time;
    result->average_time = total_time / iterations;
    result->min_time = min_time;
    result->max_time = max_time;
    result->operations_per_second = iterations / total_time;

    operation_timer_destroy(timer);
    return result;
}

void benchmark_result_destroy(benchmark_result_t* result) {
    if (result) {
        TRACKED_FREE(result);
    }
}

throughput_calculator_t* throughput_calculator_create(void) {
    throughput_calculator_t* calc = TRACKED_MALLOC(sizeof(throughput_calculator_t));
    if (!calc) return NULL;

    calc->total_operations = 0;
    clock_gettime(CLOCK_MONOTONIC, &calc->start_time);
    calc->last_update = calc->start_time;
    calc->current_throughput = 0.0;
    calc->peak_throughput = 0.0;

    return calc;
}

void throughput_calculator_destroy(throughput_calculator_t* calc) {
    if (calc) {
        TRACKED_FREE(calc);
    }
}

void throughput_calculator_add_operations(throughput_calculator_t* calc, size_t operations) {
    if (!calc) return;

    calc->total_operations += operations;

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    double time_diff = (now.tv_sec - calc->last_update.tv_sec) + 
                      (now.tv_nsec - calc->last_update.tv_nsec) / 1e9;

    if (time_diff > 0.1) { // Update every 100ms
        calc->current_throughput = operations / time_diff;

        if (calc->current_throughput > calc->peak_throughput) {
            calc->peak_throughput = calc->current_throughput;
        }

        calc->last_update = now;
    }
}

double throughput_calculator_get_current(throughput_calculator_t* calc) {
    if (!calc) return 0.0;
    return calc->current_throughput;
}

double throughput_calculator_get_average(throughput_calculator_t* calc) {
    if (!calc) return 0.0;

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    double total_time = (now.tv_sec - calc->start_time.tv_sec) + 
                       (now.tv_nsec - calc->start_time.tv_nsec) / 1e9;

    if (total_time <= 0.0) return 0.0;

    return calc->total_operations / total_time;
}

double throughput_calculator_get_peak(throughput_calculator_t* calc) {
    if (!calc) return 0.0;
    return calc->peak_throughput;
}

// Remove duplicate function definitions

// Performance counter management
performance_counter_t* performance_counter_create(void) {
    performance_counter_t* counter = TRACKED_MALLOC(sizeof(performance_counter_t));
    if (!counter) return NULL;

    counter->name[0] = '\0';
    counter->type = METRIC_COUNTER;
    counter->value = 0.0;
    counter->min_value = 0.0;
    counter->max_value = 0.0;
    counter->sum_value = 0.0;
    counter->count = 0;
    counter->is_active = false;
    clock_gettime(CLOCK_MONOTONIC, &counter->last_updated);

    return counter;
}

void performance_counter_destroy(performance_counter_t* counter) {
    if (counter) {
        TRACKED_FREE(counter);
    }
}

void performance_counter_start(performance_counter_t* counter) {
    if (counter) {
        clock_gettime(CLOCK_MONOTONIC, &counter->last_updated);
        counter->is_active = true;
    }
}

double performance_counter_stop(performance_counter_t* counter) {
    if (!counter || !counter->is_active) return 0.0;

    struct timespec end_time;
    clock_gettime(CLOCK_MONOTONIC, &end_time);

    double start_sec = counter->last_updated.tv_sec + counter->last_updated.tv_nsec / 1e9;
    double end_sec = end_time.tv_sec + end_time.tv_nsec / 1e9;
    double elapsed = end_sec - start_sec;

    counter->value = elapsed;
    counter->is_active = false;
    counter->count++;

    return elapsed;
}

// Memory footprint structure étendue (conforme STANDARD_NAMES.md)
// Structure already defined in header file

// Memory footprint tracking
memory_footprint_t* memory_footprint_create(void) {
    memory_footprint_t* footprint = TRACKED_MALLOC(sizeof(memory_footprint_t));
    if (!footprint) return NULL;

    footprint->heap_usage = 0;
    footprint->stack_usage = 0;
    footprint->peak_heap = 0;
    footprint->peak_stack = 0;
    footprint->allocation_count = 0;
    footprint->deallocation_count = 0;

    return footprint;
}

void memory_footprint_destroy(memory_footprint_t* footprint) {
    if (footprint) {
        TRACKED_FREE(footprint);
    }
}

void memory_footprint_update(memory_footprint_t* footprint) {
    if (!footprint) return;

    footprint->heap_usage = performance_metrics_get_memory_usage();

    // Estimation simple du stack usage (eviter null pointer arithmetic)
    void* stack_ptr = &footprint;
    footprint->stack_usage = (size_t)((uintptr_t)stack_ptr % 8192);

    if (footprint->stack_usage > footprint->peak_stack) {
        footprint->peak_stack = footprint->stack_usage;
    }
}

size_t memory_footprint_get_heap_usage(memory_footprint_t* footprint) {
    if (!footprint) return 0;
    return footprint->heap_usage;
}

size_t memory_footprint_get_stack_usage(memory_footprint_t* footprint) {
    if (!footprint) return 0;
    return footprint->stack_usage;
}

void memory_footprint_track_allocation(memory_footprint_t* footprint, size_t size) {
    if (!footprint) return;
    (void)size; // Parameter used for future allocation tracking
    footprint->allocation_count++;
}

void memory_footprint_track_deallocation(memory_footprint_t* footprint, size_t size) {
    if (!footprint) return;
    (void)size; // Parameter used for future deallocation tracking
    footprint->deallocation_count++;
}