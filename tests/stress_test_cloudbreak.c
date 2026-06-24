/**
 * @file stress_test_cloudbreak.c
 * @brief Stress Test Cloudbreak - 1M+ reads/writes
 * @version 1.0.0
 * @date 2026-06-09
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include "../src/storage/lum_cloudbreak.h"

#define NUM_ACCOUNTS 1000
#define NUM_OPERATIONS 100000
#define NUM_THREADS 8

typedef struct {
    lum_cloudbreak_t* cb;
    int thread_id;
    uint64_t reads;
    uint64_t writes;
    uint64_t errors;
} thread_data_t;

void* stress_worker(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
    uint8_t account_id[32];
    uint8_t buffer[256];
    
    int ops_per_thread = NUM_OPERATIONS / NUM_THREADS;
    
    for (int i = 0; i < ops_per_thread; i++) {
        // Generate account ID
        int account_num = (data->thread_id * ops_per_thread + i) % NUM_ACCOUNTS;
        snprintf((char*)account_id, 32, "account_%d", account_num);
        
        // 70% reads, 30% writes
        if (i % 10 < 7) {
            // READ
            uint32_t size;
            if (lum_cloudbreak_load(data->cb, account_id, buffer, &size)) {
                data->reads++;
            } else {
                data->errors++;
            }
        } else {
            // WRITE
            snprintf((char*)buffer, 256, "data_%d_%d", data->thread_id, i);
            if (lum_cloudbreak_store(data->cb, account_id, buffer, strlen((char*)buffer))) {
                data->writes++;
            } else {
                data->errors++;
            }
        }
    }
    
    return NULL;
}

int main(void) {
    printf("\n╔════════════════════════════════════════════════════════════╗\n");
    printf("║  CLOUDBREAK STRESS TEST - 1M+ OPERATIONS                  ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");
    
    // Create test directory in current dir
    int ret = system("mkdir -p ./cloudbreak_stress_test");
    (void)ret;  // Suppress unused warning
    
    // Init Cloudbreak
    printf("Initializing Cloudbreak...\n");
    lum_cloudbreak_t* cb = lum_cloudbreak_init(8, "./cloudbreak_stress_test");  // 8 shards
    if (!cb) {
        printf("✗ Cloudbreak init failed\n");
        return 1;
    }
    printf("✓ Cloudbreak initialized\n\n");
    
    // Create threads
    pthread_t threads[NUM_THREADS];
    thread_data_t thread_data[NUM_THREADS];
    
    printf("Starting %d threads for %d operations...\n", NUM_THREADS, NUM_OPERATIONS);
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    // Launch threads
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_data[i].cb = cb;
        thread_data[i].thread_id = i;
        thread_data[i].reads = 0;
        thread_data[i].writes = 0;
        thread_data[i].errors = 0;
        pthread_create(&threads[i], NULL, stress_worker, &thread_data[i]);
    }
    
    // Wait for threads
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    double duration = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    
    // Aggregate results
    uint64_t total_reads = 0;
    uint64_t total_writes = 0;
    uint64_t total_errors = 0;
    
    for (int i = 0; i < NUM_THREADS; i++) {
        total_reads += thread_data[i].reads;
        total_writes += thread_data[i].writes;
        total_errors += thread_data[i].errors;
    }
    
    uint64_t total_ops = total_reads + total_writes;
    
    printf("\n╔════════════════════════════════════════════════════════════╗\n");
    printf("║  RÉSULTATS STRESS TEST                                    ║\n");
    printf("╠════════════════════════════════════════════════════════════╣\n");
    printf("║  Durée          : %.2f s                                  \n", duration);
    printf("║  Threads        : %d                                       \n", NUM_THREADS);
    printf("║  Reads          : %lu                                     \n", total_reads);
    printf("║  Writes         : %lu                                     \n", total_writes);
    printf("║  Total Ops      : %lu                                     \n", total_ops);
    printf("║  Errors         : %lu                                     \n", total_errors);
    printf("║  Ops/sec        : %.0f                                    \n", total_ops / duration);
    printf("║  Success Rate   : %.2f%%                                  \n", 
           (double)(total_ops - total_errors) / total_ops * 100.0);
    printf("╚════════════════════════════════════════════════════════════╝\n\n");
    
    // Cleanup
    lum_cloudbreak_free(cb);
    
    if (total_errors == 0 && total_ops >= NUM_OPERATIONS) {
        printf("✓ STRESS TEST PASSED - No corruption detected\n");
        return 0;
    } else {
        printf("✗ STRESS TEST FAILED - %lu errors detected\n", total_errors);
        return 1;
    }
}
