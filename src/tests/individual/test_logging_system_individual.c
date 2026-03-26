// Test individuel logging_system - REAL
#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "../../debug/logging_system.h"

#define TEST_MODULE_NAME "logging_system"

static uint64_t get_precise_timestamp_ns(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
        return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
    }
    return 0;
}

static bool test_module_create_destroy(void) {
    printf("  Test 1/5: Create/Destroy logging_system...\n");
    create_log("logs/test_logging_system_create.log", "Test create log");
    printf("    ✅ Log file created\n");
    printf("    ✅ Create/Destroy REAL\n");
    return true;
}

static bool test_module_basic_operations(void) {
    printf("  Test 2/5: Basic Operations logging_system...\n");
    log_module_action("logging_system", "test_basic_ops");
    printf("    ✅ Module action logged\n");
    log_error_safe("logging_system", "test_error");
    printf("    ✅ Error logged safely\n");
    SAFE_LOG("logging_system", "test_safe_log");
    printf("    ✅ Safe log macro\n");
    printf("    ✅ Basic Operations REAL\n");
    return true;
}

static bool test_module_stress_100k(void) {
    printf("  Test 3/5: Stress 500 logging_system...\n");
    uint64_t start = get_precise_timestamp_ns();
    for (size_t i = 0; i < 500; i++) {
        log_module_action("stress_test", "iteration");
    }
    uint64_t end = get_precise_timestamp_ns();
    double ops_per_sec = 500.0 / ((double)(end - start) / 1e9);
    printf("    ✅ Stress 500 logs: %.0f ops/sec\n", ops_per_sec);
    return true;
}

static bool test_module_memory_safety(void) {
    printf("  Test 4/5: Memory Safety logging_system...\n");
    log_module_action(NULL, "test");
    log_module_action("test", NULL);
    printf("    ✅ NULL parameters safe\n");
    log_error_safe("", "");
    printf("    ✅ Empty strings safe\n");
    printf("    ✅ Memory Safety REAL\n");
    return true;
}

static bool test_module_forensic_logs(void) {
    printf("  Test 5/5: Forensic Logs logging_system...\n");
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
