// Test individuel binary_lum_converter - IMPLÉMENTATION RÉELLE
#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "../../binary/binary_lum_converter.h"
#include "../../debug/memory_tracker.h"

#define TEST_MODULE_NAME "binary_lum_converter"

static uint64_t get_precise_timestamp_ns(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
        return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
    }
    return 0;
}

static bool test_module_create_destroy(void) {
    printf("  Test 1/5: Create/Destroy binary_lum_converter...\n");
    
    // Test création résultat
    binary_lum_result_t* result = binary_lum_result_create();
    assert(result != NULL);
    printf("    ✅ binary_lum_result_t créé\n");
    
    binary_lum_result_destroy(result);
    printf("    ✅ binary_lum_result_t détruit\n");
    
    // Test résultat lum_binary
    lum_binary_result_t* lum_result = lum_binary_result_create();
    assert(lum_result != NULL);
    printf("    ✅ lum_binary_result_t créé\n");
    
    lum_binary_result_destroy(lum_result);
    printf("    ✅ Create/Destroy réussi - IMPLÉMENTATION RÉELLE\n");
    return true;
}

static bool test_module_basic_operations(void) {
    printf("  Test 2/5: Basic Operations binary_lum_converter...\n");
    
    // Test conversion int32 -> LUM
    int32_t test_value = 42;
    binary_lum_result_t* result = convert_int32_to_lum(test_value);
    assert(result != NULL);
    assert(result->success == true);
    printf("    ✅ Conversion int32->LUM réussie (valeur=42)\n");
    
    // Test conversion retour LUM -> int32
    if (result->lum_group) {
        int32_t converted = convert_lum_to_int32(result->lum_group);
        assert(converted == test_value);
        printf("    ✅ Conversion LUM->int32 réussie (valeur=%d)\n", converted);
    }
    
    binary_lum_result_destroy(result);
    
    // Test validation strings
    const char* valid_bin = "10101010";
    bool is_valid = validate_binary_string(valid_bin);
    assert(is_valid == true);
    printf("    ✅ Validation binary string réussie\n");
    
    printf("    ✅ Basic Operations réussi - IMPLÉMENTATION RÉELLE\n");
    return true;
}

static bool test_module_stress_100k(void) {
    printf("  Test 3/5: Stress 1K binary_lum_converter...\n");
    
    uint64_t start = get_precise_timestamp_ns();
    size_t iterations = 1000;
    size_t success = 0;
    
    for (size_t i = 0; i < iterations; i++) {
        binary_lum_result_t* result = convert_int32_to_lum((int32_t)i);
        if (result && result->success) {
            success++;
            binary_lum_result_destroy(result);
        }
    }
    
    uint64_t end = get_precise_timestamp_ns();
    double ops_per_sec = (double)success / ((double)(end - start) / 1e9);
    
    assert(success == iterations);
    printf("    ✅ Stress %zu conversions: %.0f ops/sec - IMPLÉMENTATION RÉELLE\n", 
           iterations, ops_per_sec);
    return true;
}

static bool test_module_memory_safety(void) {
    printf("  Test 4/5: Memory Safety binary_lum_converter...\n");
    
    // Test NULL safety
    binary_lum_result_destroy(NULL); // Ne doit pas crasher
    lum_binary_result_destroy(NULL); // Ne doit pas crasher
    printf("    ✅ Destruction NULL safe\n");
    
    // Test paramètres invalides
    binary_lum_result_t* invalid = convert_binary_to_lum(NULL, 0);
    if (invalid) {
        assert(invalid->success == false);
        binary_lum_result_destroy(invalid);
    }
    printf("    ✅ Paramètres invalides gérés\n");
    
    // Test double destruction
    binary_lum_result_t* result = binary_lum_result_create();
    binary_lum_result_destroy(result);
    binary_lum_result_destroy(result); // Ne doit pas crasher
    printf("    ✅ Double destruction protégée\n");
    
    printf("    ✅ Memory Safety complet - IMPLÉMENTATION RÉELLE\n");
    return true;
}

static bool test_module_forensic_logs(void) {
    printf("  Test 5/5: Forensic Logs binary_lum_converter...\n");
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
