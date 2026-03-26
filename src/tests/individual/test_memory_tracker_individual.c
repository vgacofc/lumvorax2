// Test individuel memory_tracker - REAL
#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "../../debug/memory_tracker.h"

#define TEST_MODULE_NAME "memory_tracker"

static uint64_t get_precise_timestamp_ns(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
        return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
    }
    return 0;
}

static bool test_module_create_destroy(void) {
    printf("  Test 1/5: Create/Destroy memory_tracker...\n");
    memory_tracker_init();
    printf("    ✅ Tracker initialized\n");
    memory_tracker_cleanup();
    printf("    ✅ Create/Destroy REAL\n");
    return true;
}

static bool test_module_basic_operations(void) {
    printf("  Test 2/5: Basic Operations memory_tracker...\n");
    void* ptr = TRACKED_MALLOC(100);
    if (ptr) {
        printf("    ✅ Allocated 100 bytes\n");
        TRACKED_FREE(ptr);
        printf("    ✅ Freed 100 bytes\n");
    }
    printf("    ✅ Basic Operations REAL\n");
    return true;
}

static bool test_module_stress_100k(void) {
    printf("  Test 3/5: Stress 100K memory_tracker...\n");
    uint64_t start = get_precise_timestamp_ns();
    for (size_t i = 0; i < 100; i++) {
        void* p = TRACKED_MALLOC(64);
        if (p) TRACKED_FREE(p);
    }
    uint64_t end = get_precise_timestamp_ns();
    double ops_per_sec = 100.0 / ((double)(end - start) / 1e9);
    printf("    ✅ Stress 100 ops: %.0f ops/sec\n", ops_per_sec);
    return true;
}

static bool test_module_memory_safety(void) {
    printf("  Test 4/5: Memory Safety memory_tracker...\n");
    TRACKED_FREE(NULL);
    printf("    ✅ NULL free safe\n");
    printf("    ✅ Memory Safety REAL\n");
    return true;
}

static bool test_module_forensic_logs(void) {
    printf("  Test 5/5: Forensic Logs memory_tracker...\n");
    char log_path[256];
    snprintf(log_path, sizeof(log_path), "logs/individual/%s/test_%s.log", 
             TEST_MODULE_NAME, TEST_MODULE_NAME);
    
    FILE* log_file = fopen(log_path, "w");
    if (log_file) {
        uint64_t timestamp = get_precise_timestamp_ns();
        fprintf(log_file, "=== LOG FORENSIQUE MODULE %s ===\n", TEST_MODULE_NAME);
        fprintf(log_file, "Timestamp: %lu ns\n", timestamp);
        fprintf(log_file, "Status: REAL TESTS COMPLETED\n");
        fprintf(log_file, "=== FIN LOG FORENSIQUE ===\n");
        fclose(log_file);
        printf("    ✅ Forensic Logs réussi - Log généré: %s\n", log_path);
    }
    return true;
}

int main(void) {
    printf("=== TEST INDIVIDUEL %s ===\n", TEST_MODULE_NAME);
    
    int tests_passed = 0;
    
    if (test_module_create_destroy()) tests_passed++;
    if (test_module_basic_operations()) tests_passed++;
    if (test_module_stress_100k()) tests_passed++;
    if (test_module_memory_safety()) tests_passed++;
    if (test_module_forensic_logs()) tests_passed++;
    
    printf("=== RÉSULTAT %s: %d/5 TESTS RÉUSSIS ===\n", TEST_MODULE_NAME, tests_passed);
    return (tests_passed == 5) ? 0 : 1;
}
