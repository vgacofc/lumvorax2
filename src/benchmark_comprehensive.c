#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>

#include "src/lum/lum_core.h"
#include "src/vorax/vorax_operations.h"
#include "src/binary/binary_lum_converter.h"
#include "src/crypto/crypto_validator.h"
#include "src/parallel/parallel_processor.h"
#include "src/metrics/performance_metrics.h"

// Utilities for precise timing
double get_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
}

long get_memory_usage_kb() {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    return usage.ru_maxrss; // Peak memory usage in KB
}

// Comprehensive benchmark suite
void benchmark_lum_operations() {
    printf("\n=== BENCHMARK LUM OPERATIONS ===\n");
    
    double start_time = get_time_ms();
    long start_memory = get_memory_usage_kb();
    
    // Test 1: Mass LUM creation and destruction
    const int lum_count = 100000;
    lum_t** lums = malloc(lum_count * sizeof(lum_t*));
    
    double creation_start = get_time_ms();
    for (int i = 0; i < lum_count; i++) {
        lums[i] = lum_create(i % 2, i % 1000, (i * 7) % 1000, LUM_STRUCTURE_LINEAR);
    }
    double creation_time = get_time_ms() - creation_start;
    
    printf("  Creation: %d LUMs in %.2f ms (%.0f LUMs/sec)\n", 
           lum_count, creation_time, lum_count / (creation_time / 1000.0));
    
    // Test 2: Group operations
    lum_group_t* group = lum_group_create(lum_count);
    double group_start = get_time_ms();
    for (int i = 0; i < lum_count; i++) {
        lum_group_add(group, lums[i]);
    }
    double group_time = get_time_ms() - group_start;
    
    printf("  Group add: %d operations in %.2f ms (%.0f ops/sec)\n", 
           lum_count, group_time, lum_count / (group_time / 1000.0));
    
    // Cleanup
    double cleanup_start = get_time_ms();
    for (int i = 0; i < lum_count; i++) {
        lum_destroy(lums[i]);
    }
    lum_group_destroy(group);
    free(lums);
    double cleanup_time = get_time_ms() - cleanup_start;
    
    printf("  Cleanup: %.2f ms\n", cleanup_time);
    
    long end_memory = get_memory_usage_kb();
    double total_time = get_time_ms() - start_time;
    
    printf("  Total time: %.2f ms\n", total_time);
    printf("  Peak memory: %ld KB\n", end_memory - start_memory);
}

void benchmark_crypto_operations() {
    printf("\n=== BENCHMARK CRYPTO OPERATIONS ===\n");
    
    // Test SHA-256 performance with various sizes
    const int iterations = 1000;
    const size_t test_sizes[] = {64, 256, 1024, 4096, 16384};
    const int num_sizes = sizeof(test_sizes) / sizeof(test_sizes[0]);
    
    for (int s = 0; s < num_sizes; s++) {
        size_t size = test_sizes[s];
        uint8_t* data = malloc(size);
        memset(data, 0x42, size);
        uint8_t hash[32];
        
        double start_time = get_time_ms();
        for (int i = 0; i < iterations; i++) {
            sha256_hash(data, size, hash);
        }
        double elapsed = get_time_ms() - start_time;
        
        double throughput_mb = (double)(size * iterations) / (1024.0 * 1024.0) / (elapsed / 1000.0);
        
        printf("  SHA-256 %zu bytes: %.2f ms (%d iterations) - %.2f MB/s\n", 
               size, elapsed, iterations, throughput_mb);
        
        free(data);
    }
}

void benchmark_binary_conversions() {
    printf("\n=== BENCHMARK BINARY CONVERSIONS ===\n");
    
    const int iterations = 10000;
    
    // Test integer conversions
    double start_time = get_time_ms();
    for (int i = 0; i < iterations; i++) {
        binary_lum_result_t* result = convert_int32_to_lum(i);
        int32_t back = convert_lum_to_int32(result->lum_group);
        (void)back; // Avoid unused variable warning
        binary_lum_result_destroy(result);
    }
    double elapsed = get_time_ms() - start_time;
    
    printf("  Int32 conversions: %d round-trips in %.2f ms (%.0f ops/sec)\n", 
           iterations, elapsed, iterations / (elapsed / 1000.0));
    
    // Test bit string conversions
    const char* test_bits = "11010110101011010110101011010110";
    start_time = get_time_ms();
    for (int i = 0; i < iterations; i++) {
        binary_lum_result_t* result = convert_bits_to_lum(test_bits);
        lum_binary_result_t* back = convert_lum_to_bits(result->lum_group);
        binary_lum_result_destroy(result);
        lum_binary_result_destroy(back);
    }
    elapsed = get_time_ms() - start_time;
    
    printf("  Bit string conversions: %d round-trips in %.2f ms (%.0f ops/sec)\n", 
           iterations, elapsed, iterations / (elapsed / 1000.0));
}

void benchmark_vorax_operations() {
    printf("\n=== BENCHMARK VORAX OPERATIONS ===\n");
    
    const int group_size = 1000;
    const int iterations = 1000;
    
    // Setup test groups
    lum_group_t* group1 = lum_group_create(group_size);
    lum_group_t* group2 = lum_group_create(group_size);
    
    for (int i = 0; i < group_size; i++) {
        lum_t* lum1 = lum_create(i % 2, i, 0, LUM_STRUCTURE_LINEAR);
        lum_t* lum2 = lum_create((i+1) % 2, i, 1, LUM_STRUCTURE_LINEAR);
        lum_group_add(group1, lum1);
        lum_group_add(group2, lum2);
        lum_destroy(lum1);
        lum_destroy(lum2);
    }
    
    // Benchmark FUSE operations
    double start_time = get_time_ms();
    for (int i = 0; i < iterations; i++) {
        vorax_result_t* result = vorax_fuse(group1, group2);
        vorax_result_destroy(result);
    }
    double elapsed = get_time_ms() - start_time;
    
    printf("  FUSE operations: %d operations in %.2f ms (%.0f ops/sec)\n", 
           iterations, elapsed, iterations / (elapsed / 1000.0));
    
    // Benchmark SPLIT operations
    start_time = get_time_ms();
    for (int i = 0; i < iterations; i++) {
        vorax_result_t* result = vorax_split(group1, 4);
        vorax_result_destroy(result);
    }
    elapsed = get_time_ms() - start_time;
    
    printf("  SPLIT operations: %d operations in %.2f ms (%.0f ops/sec)\n", 
           iterations, elapsed, iterations / (elapsed / 1000.0));
    
    lum_group_destroy(group1);
    lum_group_destroy(group2);
}

void benchmark_parallel_processing() {
    printf("\n=== BENCHMARK PARALLEL PROCESSING ===\n");
    
    const int lum_count = 10000;
    lum_t** lums = malloc(lum_count * sizeof(lum_t*));
    
    // Create test LUMs
    for (int i = 0; i < lum_count; i++) {
        lums[i] = malloc(sizeof(lum_t));
        lums[i]->presence = i % 2;
        lums[i]->position_x = i;
        lums[i]->position_y = i * 2;
        lums[i]->id = i;
    }
    
    // Test different thread counts
    int thread_counts[] = {1, 2, 4, 8};
    int num_thread_tests = sizeof(thread_counts) / sizeof(thread_counts[0]);
    
    for (int t = 0; t < num_thread_tests; t++) {
        int threads = thread_counts[t];
        
        double start_time = get_time_ms();
        parallel_process_result_t result = parallel_process_lums(lums, lum_count, threads);
        double elapsed = get_time_ms() - start_time;
        
        if (result.success) {
            printf("  %d threads: %d LUMs in %.2f ms (%.0f LUMs/sec)\n", 
                   threads, lum_count, elapsed, lum_count / (elapsed / 1000.0));
        } else {
            printf("  %d threads: FAILED\n", threads);
        }
    }
    
    // Cleanup
    for (int i = 0; i < lum_count; i++) {
        free(lums[i]);
    }
    free(lums);
}

void benchmark_memory_usage() {
    printf("\n=== BENCHMARK MEMORY USAGE ===\n");
    
    long start_memory = get_memory_usage_kb();
    
    // Test progressive memory allocation
    const int steps = 10;
    const int lums_per_step = 10000;
    
    for (int step = 1; step <= steps; step++) {
        int total_lums = step * lums_per_step;
        
        lum_group_t* group = lum_group_create(total_lums);
        for (int i = 0; i < total_lums; i++) {
            lum_t* lum = lum_create(i % 2, i, i, LUM_STRUCTURE_LINEAR);
            lum_group_add(group, lum);
            lum_destroy(lum);
        }
        
        long current_memory = get_memory_usage_kb();
        printf("  %d LUMs: %ld KB (%.2f KB/LUM)\n", 
               total_lums, current_memory - start_memory, 
               (double)(current_memory - start_memory) / total_lums);
        
        lum_group_destroy(group);
    }
}

int main() {
    printf("=== LUM/VORAX COMPREHENSIVE BENCHMARK ===\n");
    printf("Starting comprehensive performance analysis...\n");
    
    double total_start = get_time_ms();
    long start_memory = get_memory_usage_kb();
    
    benchmark_lum_operations();
    benchmark_crypto_operations();
    benchmark_binary_conversions();
    benchmark_vorax_operations();
    benchmark_parallel_processing();
    benchmark_memory_usage();
    
    double total_time = get_time_ms() - total_start;
    long total_memory = get_memory_usage_kb();
    
    printf("\n=== BENCHMARK SUMMARY ===\n");
    printf("Total benchmark time: %.2f seconds\n", total_time / 1000.0);
    printf("Peak memory usage: %ld KB\n", total_memory - start_memory);
    printf("System specs: %ld CPU cores available\n", sysconf(_SC_NPROCESSORS_ONLN));
    
    return 0;
}