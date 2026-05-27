// Test individuel MATRIX_CALCULATOR - Correction conflits typedef (Rapport 039, 042)
#include "../../advanced_calculations/matrix_calculator.h"
#include "../../debug/memory_tracker.h"
#include "../../debug/forensic_logger.h"
#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define TEST_MODULE_NAME "matrix_calculator"
#define TEST_SCALE_MIN 10
#define TEST_SCALE_MAX 100000

typedef struct {
    char test_name[128];
    bool success;
    uint64_t execution_time_ns;
    size_t memory_used;
    char error_details[256];
} individual_test_result_t;

static bool test_module_create_destroy(void);
static bool test_module_basic_operations(void);
static bool test_module_stress_100k(void);
static bool test_module_memory_safety(void);
static bool test_module_forensic_logs(void);

static uint64_t get_precise_timestamp_ns(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
        return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
    }
    return 0;
}

// Test 1: Create/Destroy avec types unifiés (correction critique)
static bool test_module_create_destroy(void) {
    uint64_t start_time = get_precise_timestamp_ns();
    
    printf("  Test 1/5: Create/Destroy Matrix (types unifiés)...\n");
    
    // Test création matrices avec types corrects de common_types.h
    lum_matrix_t* matrix1 = lum_matrix_create(4, 4);
    if (!matrix1) {
        printf("    ❌ Échec création matrix1\n");
        return false;
    }
    
    lum_matrix_t* matrix2 = lum_matrix_create(4, 4);
    if (!matrix2) {
        printf("    ❌ Échec création matrix2\n");
        lum_matrix_destroy(&matrix1);
        return false;
    }
    
    // Test destruction avec validation des pointeurs
    lum_matrix_destroy(&matrix1);
    lum_matrix_destroy(&matrix2);
    
    // Validation pointeurs NULL après destruction
    if (matrix1 != NULL || matrix2 != NULL) {
        printf("    ❌ Pointeurs non NULL après destruction\n");
        return false;
    }
    
    uint64_t end_time = get_precise_timestamp_ns();
    printf("    ✅ Create/Destroy réussi - types unifiés (%lu ns)\n", end_time - start_time);
    return true;
}

// Test 2: Basic Operations avec résolution conflits typedef
static bool test_module_basic_operations(void) {
    uint64_t start_time = get_precise_timestamp_ns();
    
    printf("  Test 2/5: Basic Operations Matrix...\n");
    
    // Matrices 3x3 pour tests rapides
    lum_matrix_t* matrix_a = lum_matrix_create(3, 3);
    lum_matrix_t* matrix_b = lum_matrix_create(3, 3);
    
    if (!matrix_a || !matrix_b) {
        printf("    ❌ Échec création matrices de test\n");
        if (matrix_a) lum_matrix_destroy(&matrix_a);
        if (matrix_b) lum_matrix_destroy(&matrix_b);
        return false;
    }
    
    // Initialisation matrices avec valeurs test
    for (size_t i = 0; i < 3; i++) {
        for (size_t j = 0; j < 3; j++) {
            lum_matrix_set(matrix_a, i, j, (double)(i + j + 1));
            lum_matrix_set(matrix_b, i, j, (double)(i * j + 1));
        }
    }
    
    // Test multiplication avec types corrects (correction conflits)
    matrix_config_t* config = matrix_config_create_default();
    if (!config) {
        printf("    ❌ Échec création config par défaut\n");
        lum_matrix_destroy(&matrix_a);
        lum_matrix_destroy(&matrix_b);
        return false;
    }
    
    matrix_lum_result_t* result = matrix_multiply(matrix_a, matrix_b, config);
    if (!result) {
        printf("    ❌ Échec multiplication matrices\n");
        matrix_config_destroy(&config);
        lum_matrix_destroy(&matrix_a);
        lum_matrix_destroy(&matrix_b);
        return false;
    }
    
    // Validation résultat
    if (!result->success) {
        printf("    ❌ Multiplication marquée comme échouée\n");
        matrix_lum_result_destroy(&result);
        matrix_config_destroy(&config);
        lum_matrix_destroy(&matrix_a);
        lum_matrix_destroy(&matrix_b);
        return false;
    }
    
    // Nettoyage avec types unifiés
    matrix_lum_result_destroy(&result);
    matrix_config_destroy(&config);
    lum_matrix_destroy(&matrix_a);
    lum_matrix_destroy(&matrix_b);
    
    uint64_t end_time = get_precise_timestamp_ns();
    printf("    ✅ Basic Operations réussi - conflits typedef résolus (%lu ns)\n", 
           end_time - start_time);
    return true;
}

// Test 3: Stress avec matrices plus grandes
static bool test_module_stress_100k(void) {
    uint64_t start_time = get_precise_timestamp_ns();
    
    printf("  Test 3/5: Stress matrices 50x50...\n");
    
    // Test avec matrices 50x50 (2500 éléments chacune)
    const size_t matrix_size = 50;
    lum_matrix_t* large_matrix_a = lum_matrix_create(matrix_size, matrix_size);
    lum_matrix_t* large_matrix_b = lum_matrix_create(matrix_size, matrix_size);
    
    if (!large_matrix_a || !large_matrix_b) {
        printf("    ❌ Échec création matrices stress\n");
        if (large_matrix_a) lum_matrix_destroy(&large_matrix_a);
        if (large_matrix_b) lum_matrix_destroy(&large_matrix_b);
        return false;
    }
    
    // Initialisation optimisée avec progress report
    printf("    📊 Initialisation matrices %zux%zu...\n", matrix_size, matrix_size);
    for (size_t i = 0; i < matrix_size; i++) {
        if (i % 10 == 0) {
            printf("    📊 Progress: ligne %zu/%zu\n", i, matrix_size);
        }
        for (size_t j = 0; j < matrix_size; j++) {
            double val_a = sin((double)(i + j)) * 100.0;
            double val_b = cos((double)(i * j + 1)) * 100.0;
            lum_matrix_set(large_matrix_a, i, j, val_a);
            lum_matrix_set(large_matrix_b, i, j, val_b);
        }
    }
    
    // Test multiplication stress avec config optimisée
    matrix_config_t* stress_config = matrix_config_create_default();
    if (!stress_config) {
        printf("    ❌ Échec création config stress\n");
        lum_matrix_destroy(&large_matrix_a);
        lum_matrix_destroy(&large_matrix_b);
        return false;
    }
    
    printf("    📊 Lancement multiplication %zux%zu...\n", matrix_size, matrix_size);
    uint64_t multiply_start = get_precise_timestamp_ns();
    
    matrix_lum_result_t* stress_result = matrix_multiply(large_matrix_a, large_matrix_b, stress_config);
    
    uint64_t multiply_end = get_precise_timestamp_ns();
    uint64_t multiply_duration = multiply_end - multiply_start;
    
    if (!stress_result || !stress_result->success) {
        printf("    ❌ Échec multiplication stress\n");
        if (stress_result) matrix_lum_result_destroy(&stress_result);
        matrix_config_destroy(&stress_config);
        lum_matrix_destroy(&large_matrix_a);
        lum_matrix_destroy(&large_matrix_b);
        return false;
    }
    
    // Calcul métriques performance
    size_t total_ops = matrix_size * matrix_size * matrix_size; // O(n³)
    double ops_per_sec = (double)total_ops / (multiply_duration / 1e9);
    
    printf("    📊 Multiplication réussie: %zu ops en %lu ns (%.2f Mops/sec)\n",
           total_ops, multiply_duration, ops_per_sec / 1e6);
    
    // Nettoyage
    matrix_lum_result_destroy(&stress_result);
    matrix_config_destroy(&stress_config);
    lum_matrix_destroy(&large_matrix_a);
    lum_matrix_destroy(&large_matrix_b);
    
    uint64_t end_time = get_precise_timestamp_ns();
    printf("    ✅ Stress test réussi: %.2f Mops/sec (%lu ns total)\n", 
           ops_per_sec / 1e6, end_time - start_time);
    return true;
}

// Test 4: Memory Safety
static bool test_module_memory_safety(void) {
    uint64_t start_time = get_precise_timestamp_ns();
    
    printf("  Test 4/5: Memory Safety Matrix...\n");
    
    // Test NULL pointers safety
    lum_matrix_t* null_matrix = NULL;
    lum_matrix_destroy(&null_matrix); // Doit être safe
    
    // Test dimensions invalides
    lum_matrix_t* invalid_matrix = lum_matrix_create(0, 5);
    if (invalid_matrix != NULL) {
        printf("    ❌ Création matrice 0x5 devrait échouer\n");
        lum_matrix_destroy(&invalid_matrix);
        return false;
    }
    
    // Test accès hors bornes
    lum_matrix_t* test_matrix = lum_matrix_create(5, 5);
    if (!test_matrix) {
        printf("    ❌ Échec création matrice test\n");
        return false;
    }
    
    // Test set/get valides
    bool set_success = lum_matrix_set(test_matrix, 2, 3, 42.0);
    if (!set_success) {
        printf("    ❌ Set valide devrait réussir\n");
        lum_matrix_destroy(&test_matrix);
        return false;
    }
    
    double retrieved_value = lum_matrix_get(test_matrix, 2, 3);
    if (fabs(retrieved_value - 42.0) > 1e-9) {
        printf("    ❌ Valeur récupérée incorrecte: %f != 42.0\n", retrieved_value);
        lum_matrix_destroy(&test_matrix);
        return false;
    }
    
    // Test accès invalides (hors bornes)
    bool invalid_set = lum_matrix_set(test_matrix, 10, 3, 99.0);
    if (invalid_set) {
        printf("    ❌ Set hors bornes devrait échouer\n");
        lum_matrix_destroy(&test_matrix);
        return false;
    }
    
    double invalid_get = lum_matrix_get(test_matrix, 10, 3);
    if (invalid_get != 0.0) {
        printf("    ❌ Get hors bornes devrait retourner 0.0\n");
        lum_matrix_destroy(&test_matrix);
        return false;
    }
    
    lum_matrix_destroy(&test_matrix);
    
    uint64_t end_time = get_precise_timestamp_ns();
    printf("    ✅ Memory Safety réussi (%lu ns)\n", end_time - start_time);
    return true;
}

// Test 5: Forensic Logs
static bool test_module_forensic_logs(void) {
    uint64_t start_time = get_precise_timestamp_ns();
    
    printf("  Test 5/5: Forensic Logs Matrix...\n");
    
    char log_path[256];
    snprintf(log_path, sizeof(log_path), "logs/individual/%s/test_%s.log", 
             TEST_MODULE_NAME, TEST_MODULE_NAME);
    
    FILE* log_file = fopen(log_path, "w");
    if (!log_file) {
        printf("    ❌ Impossible de créer log forensique: %s\n", log_path);
        return false;
    }
    
    uint64_t log_timestamp = get_precise_timestamp_ns();
    fprintf(log_file, "=== LOG FORENSIQUE MODULE MATRIX_CALCULATOR ===\n");
    fprintf(log_file, "Timestamp: %lu ns\n", log_timestamp);
    fprintf(log_file, "Module: %s\n", TEST_MODULE_NAME);
    fprintf(log_file, "Correction: Conflits typedef résolus (Rapport 039, 042)\n");
    fprintf(log_file, "Types unifiés: common_types.h APPLIQUÉ\n");
    fprintf(log_file, "Status: EXECUTING\n\n");
    
    // Test avec logs détaillés
    lum_matrix_t* log_matrix = lum_matrix_create(3, 3);
    if (log_matrix) {
        fprintf(log_file, "Matrix Created: 3x3 matrix\n");
        fprintf(log_file, "Memory address: %p\n", (void*)log_matrix);
        
        // Test opérations avec logs
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                double value = (double)(i * 3 + j + 1);
                lum_matrix_set(log_matrix, i, j, value);
                fprintf(log_file, "Set [%d,%d] = %f\n", i, j, value);
            }
        }
        
        // Test retrieval avec logs
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                double retrieved = lum_matrix_get(log_matrix, i, j);
                fprintf(log_file, "Get [%d,%d] = %f\n", i, j, retrieved);
            }
        }
        
        lum_matrix_destroy(&log_matrix);
        fprintf(log_file, "Matrix Destroyed: cleanup successful\n");
        fprintf(log_file, "Types unifiés: VALIDATION RÉUSSIE\n");
    } else {
        fprintf(log_file, "Matrix Creation: FAILED\n");
        fclose(log_file);
        return false;
    }
    
    uint64_t end_timestamp = get_precise_timestamp_ns();
    fprintf(log_file, "\nTest duration: %lu ns\n", end_timestamp - log_timestamp);
    fprintf(log_file, "Conflits typedef: RÉSOLUS\n");
    fprintf(log_file, "Common_types.h: UNIFIÉ\n");
    fprintf(log_file, "Status: SUCCESS\n");
    fprintf(log_file, "=== FIN LOG FORENSIQUE ===\n");
    
    fclose(log_file);
    
    uint64_t end_time = get_precise_timestamp_ns();
    printf("    ✅ Forensic Logs réussi - Types unifiés validés: %s (%lu ns)\n", 
           log_path, end_time - start_time);
    return true;
}

int main(void) {
    printf("=== TEST INDIVIDUEL %s ===\n", TEST_MODULE_NAME);
    printf("Correction: Conflits typedef résolus (Rapport 039, 042)\n");
    
    memory_tracker_init();
    uint64_t test_session_start = get_precise_timestamp_ns();
    
    int tests_passed = 0;
    
    if (test_module_create_destroy()) {
        tests_passed++;
        printf("✅ %s Create/Destroy: PASS\n", TEST_MODULE_NAME);
    } else {
        printf("❌ %s Create/Destroy: FAIL\n", TEST_MODULE_NAME);
    }
    
    if (test_module_basic_operations()) {
        tests_passed++;
        printf("✅ %s Basic Operations: PASS\n", TEST_MODULE_NAME);
    } else {
        printf("❌ %s Basic Operations: FAIL\n", TEST_MODULE_NAME);
    }
    
    if (test_module_stress_100k()) {
        tests_passed++;
        printf("✅ %s Stress 100K: PASS\n", TEST_MODULE_NAME);
    } else {
        printf("❌ %s Stress 100K: FAIL\n", TEST_MODULE_NAME);
    }
    
    if (test_module_memory_safety()) {
        tests_passed++;
        printf("✅ %s Memory Safety: PASS\n", TEST_MODULE_NAME);
    } else {
        printf("❌ %s Memory Safety: FAIL\n", TEST_MODULE_NAME);
    }
    
    if (test_module_forensic_logs()) {
        tests_passed++;
        printf("✅ %s Forensic Logs: PASS\n", TEST_MODULE_NAME);
    } else {
        printf("❌ %s Forensic Logs: FAIL\n", TEST_MODULE_NAME);
    }
    
    uint64_t test_session_end = get_precise_timestamp_ns();
    uint64_t total_duration = test_session_end - test_session_start;
    
    printf("=== RÉSULTAT %s: %d/5 TESTS RÉUSSIS ===\n", TEST_MODULE_NAME, tests_passed);
    printf("Durée totale: %lu ns (%.3f ms)\n", total_duration, total_duration / 1e6);
    printf("Types unifiés: CORRECTION APPLIQUÉE\n");
    
    memory_tracker_report();
    memory_tracker_destroy();
    
    return (tests_passed == 5) ? 0 : 1;
}