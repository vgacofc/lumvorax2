#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>

// Benchmark for V29 metrics extraction
int main() {
    printf("=== V29 BENCHMARK - METRIQUES REELLES ===\n");
    printf("Date: %s %s\n", __DATE__, __TIME__);
    printf("Version: V29-NATIVE-C\n");
    printf("Session: V29_BENCHMARK_%ld\n\n", time(NULL));
    
    struct timespec start, end;
    
    // Test 1: Memory allocation performance
    printf("[BLOC 1] Performance brute - Create/Destroy\n");
    clock_gettime(CLOCK_MONOTONIC, &start);
    size_t count = 100000;
    void** ptrs = malloc(count * sizeof(void*));
    for(size_t i = 0; i < count; i++) {
        ptrs[i] = malloc(256);
    }
    for(size_t i = 0; i < count; i++) {
        free(ptrs[i]);
    }
    free(ptrs);
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    double elapsed_ns = (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);
    double ops_per_sec = (count * 2.0) / (elapsed_ns / 1e9);
    double latency_ns = elapsed_ns / (count * 2.0);
    
    printf("  Operations: %zu create + %zu destroy\n", count, count);
    printf("  Temps total: %.2f ms\n", elapsed_ns / 1e6);
    printf("  Ops/seconde: %.2f M ops/s\n", ops_per_sec / 1e6);
    printf("  Latence moyenne: %.2f ns\n", latency_ns);
    printf("\n");
    
    // Test 2: Sequential vs Random access
    printf("[BLOC 2] Cache behavior - Sequential vs Random\n");
    size_t array_size = 1024 * 1024; // 1MB
    char* data = malloc(array_size);
    
    // Sequential
    clock_gettime(CLOCK_MONOTONIC, &start);
    volatile char sum = 0;
    for(size_t i = 0; i < array_size; i++) {
        sum += data[i];
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    double seq_time = (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);
    
    // Random (pseudo-random pattern)
    clock_gettime(CLOCK_MONOTONIC, &start);
    size_t idx = 0;
    for(size_t i = 0; i < array_size; i++) {
        sum += data[idx];
        idx = (idx * 1103515245 + 12345) % array_size;
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    double rand_time = (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);
    
    printf("  Array size: %zu bytes\n", array_size);
    printf("  Temps sequentiel: %.2f ms\n", seq_time / 1e6);
    printf("  Temps aleatoire: %.2f ms\n", rand_time / 1e6);
    printf("  Ratio seq/rand: %.2fx\n", rand_time / seq_time);
    printf("  Throughput seq: %.2f GB/s\n", (array_size / (seq_time / 1e9)) / 1e9);
    
    free(data);
    printf("\n");
    
    // Test 3: Forensic logging overhead
    printf("[BLOC 3] Forensic overhead simulation\n");
    FILE* log_file = fopen("/tmp/forensic_bench.log", "w");
    
    // Without logging
    clock_gettime(CLOCK_MONOTONIC, &start);
    for(size_t i = 0; i < 10000; i++) {
        void* p = malloc(64);
        free(p);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    double no_log_time = (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);
    
    // With logging
    clock_gettime(CLOCK_MONOTONIC, &start);
    for(size_t i = 0; i < 10000; i++) {
        void* p = malloc(64);
        fprintf(log_file, "[%ld] ALLOC %p 64 bytes\n", clock(), p);
        fflush(log_file);
        free(p);
        fprintf(log_file, "[%ld] FREE %p\n", clock(), p);
        fflush(log_file);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    double with_log_time = (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);
    
    fclose(log_file);
    
    printf("  Sans logging: %.2f ms (%.2f ns/op)\n", no_log_time / 1e6, no_log_time / 10000);
    printf("  Avec logging: %.2f ms (%.2f ns/op)\n", with_log_time / 1e6, with_log_time / 10000);
    printf("  Overhead: %.2f%% (+%.2f ns/op)\n", 
           ((with_log_time - no_log_time) / no_log_time) * 100,
           (with_log_time - no_log_time) / 10000);
    printf("\n");
    
    printf("=== FIN BENCHMARK V29 ===\n");
    return 0;
}
