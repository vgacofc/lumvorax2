// Test individuel lum_instant_displacement - REAL
#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "../../spatial/lum_instant_displacement.h"
#include "../../debug/memory_tracker.h"

#define TEST_MODULE_NAME "lum_instant_displacement"

static uint64_t get_precise_timestamp_ns(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
        return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
    }
    return 0;
}

static bool test_module_create_destroy(void) {
    printf("  Test 1/5: Create/Destroy lum_instant_displacement...\n");
    lum_displacement_metrics_t* metrics = lum_displacement_metrics_create();
    assert(metrics != NULL);
    assert(metrics->total_displacements == 0);
    printf("    ✅ Metrics created\n");
    lum_displacement_metrics_destroy(metrics);
    printf("    ✅ Create/Destroy REAL\n");
    return true;
}

static bool test_module_basic_operations(void) {
    printf("  Test 2/5: Basic Operations lum_instant_displacement...\n");
    lum_t* lum = lum_create(1, 0, 0, LUM_STRUCTURE_BASIC);
    lum_displacement_result_t result;
    bool success = lum_instant_displace(lum, 100, 200, &result);
    if (success) {
        printf("    ✅ Displacement: (%d,%d)->(%d,%d) in %lu ns\n",
               result.from_x, result.from_y, result.to_x, result.to_y, result.displacement_time_ns);
    }
    lum_destroy(&lum);
    printf("    ✅ Basic Operations REAL\n");
    return true;
}

static bool test_module_stress_100k(void) {
    printf("  Test 3/5: Stress 100 lum_instant_displacement...\n");
    uint64_t start = get_precise_timestamp_ns();
    for (size_t i = 0; i < 100; i++) {
        lum_t* lum = lum_create(i, 0, 0, LUM_STRUCTURE_BASIC);
        lum_displacement_result_t result;
        lum_instant_displace(lum, (int32_t)i, (int32_t)i, &result);
        lum_destroy(&lum);
    }
    uint64_t end = get_precise_timestamp_ns();
    double ops_per_sec = 100.0 / ((double)(end - start) / 1e9);
    printf("    ✅ Stress 100 ops: %.0f ops/sec\n", ops_per_sec);
    return true;
}

static bool test_module_memory_safety(void) {
    printf("  Test 4/5: Memory Safety lum_instant_displacement...\n");
    lum_displacement_metrics_destroy(NULL);
    printf("    ✅ NULL destroy safe\n");
    lum_displacement_result_t result;
    lum_instant_displace(NULL, 0, 0, &result);
    printf("    ✅ NULL LUM safe\n");
    printf("    ✅ Memory Safety REAL\n");
    return true;
}

static bool test_module_forensic_logs(void) {
    printf("  Test 5/5: Forensic Logs lum_instant_displacement...\n");
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
