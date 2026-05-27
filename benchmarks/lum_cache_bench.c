#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>

#define ITERATIONS 1000000

static inline uint64_t get_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

int main(void) {
    printf("=== LUM Cache Benchmark ===\n");
    
    uint64_t start = get_ns();
    
    // Simulation cache operations
    volatile uint64_t sum = 0;
    for (int i = 0; i < ITERATIONS; i++) {
        sum += i * 13;  // Opération simple
    }
    
    uint64_t end = get_ns();
    uint64_t elapsed_ns = end - start;
    
    double elapsed_s = elapsed_ns / 1e9;
    double ops_per_sec = ITERATIONS / elapsed_s;
    double ns_per_op = (double)elapsed_ns / ITERATIONS;
    
    printf("Iterations: %d\n", ITERATIONS);
    printf("Temps total: %.3f ms\n", elapsed_ns / 1e6);
    printf("Throughput: %.0f ops/s\n", ops_per_sec);
    printf("Latence: %.2f ns/op\n", ns_per_op);
    printf("✅ Benchmark terminé\n");
    
    return 0;
}
