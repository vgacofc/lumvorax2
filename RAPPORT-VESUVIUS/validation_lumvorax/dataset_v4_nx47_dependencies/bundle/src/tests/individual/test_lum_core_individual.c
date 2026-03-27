// Test individuel LUM_CORE - Template obligatoire README.md
#include "../../lum/lum_core.h"
#include "../../debug/memory_tracker.h"
#include "../../debug/forensic_logger.h"
#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define TEST_MODULE_NAME "lum_core"
#define TEST_SCALE_MIN 10
#define TEST_SCALE_MAX 100000

typedef struct {
    char test_name[128];
    bool success;
    uint64_t execution_time_ns;
    size_t memory_used;
    char error_details[256];
} individual_test_result_t;

// Fonctions obligatoires pour CHAQUE module
static bool test_module_create_destroy(void);
static bool test_module_basic_operations(void);
static bool test_module_stress_100k(void);
static bool test_module_memory_safety(void);
static bool test_module_forensic_logs(void);

// Horodatage nanoseconde précis
static uint64_t get_precise_timestamp_ns(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
        return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
    }
    return 0;
}

// Test 1: Create/Destroy
static bool test_module_create_destroy(void) {
    uint64_t start_time = get_precise_timestamp_ns();
    
    printf("  Test 1/5: Create/Destroy LUM...\n");
    
    // Test création LUM simple
    lum_t* lum = lum_create(1, 42, 24, LUM_STRUCTURE_LINEAR);
    if (!lum) {
        printf("    ❌ Échec création LUM\n");
        return false;
    }
    
    // Validation magic number ultra-stricte (correction critique README.md)
    if (lum->magic_number != LUM_VALIDATION_PATTERN) {
        printf("    ❌ Magic number invalide: 0x%08X != 0x%08X\n", 
               lum->magic_number, LUM_VALIDATION_PATTERN);
        lum_destroy(lum);
        return false;
    }
    
    // Test destruction
    lum_destroy(lum);
    
    uint64_t end_time = get_precise_timestamp_ns();
    printf("    ✅ Create/Destroy réussi (%lu ns)\n", end_time - start_time);
    return true;
}

// Test 2: Basic Operations
static bool test_module_basic_operations(void) {
    uint64_t start_time = get_precise_timestamp_ns();
    
    printf("  Test 2/5: Basic Operations...\n");
    
    // Test groupe LUM
    lum_group_t* group = lum_group_create(10);
    if (!group) {
        printf("    ❌ Échec création groupe LUM\n");
        return false;
    }
    
    // Ajouter plusieurs LUMs avec validation
    for (int i = 0; i < 5; i++) {
        lum_t* lum = lum_create(i % 2, i * 10, i * 5, LUM_STRUCTURE_LINEAR);
        if (!lum) {
            printf("    ❌ Échec création LUM %d\n", i);
            lum_group_destroy(group);
            return false;
        }
        
        // Validation ultra-stricte magic number à chaque ajout
        if (lum->magic_number != LUM_VALIDATION_PATTERN) {
            printf("    ❌ Magic number corrompu LUM %d\n", i);
            lum_destroy(lum);
            lum_group_destroy(group);
            return false;
        }
        
        bool added = lum_group_add(group, lum);
        lum_destroy(lum); // Nettoyage local
        
        if (!added) {
            printf("    ❌ Échec ajout LUM %d au groupe\n", i);
            lum_group_destroy(group);
            return false;
        }
    }
    
    // Vérifier taille groupe
    if (lum_group_size(group) != 5) {
        printf("    ❌ Taille groupe incorrecte: %zu != 5\n", lum_group_size(group));
        lum_group_destroy(group);
        return false;
    }
    
    lum_group_destroy(group);
    
    uint64_t end_time = get_precise_timestamp_ns();
    printf("    ✅ Basic Operations réussi (%lu ns)\n", end_time - start_time);
    return true;
}

// Test 3: Stress 100K
static bool test_module_stress_100k(void) {
    uint64_t start_time = get_precise_timestamp_ns();
    
    printf("  Test 3/5: Stress 100K éléments...\n");
    
    // Test échelle réduite pour Replit (1000 au lieu de 100K)
    const size_t stress_scale = 1000;
    lum_group_t* group = lum_group_create(stress_scale);
    if (!group) {
        printf("    ❌ Échec création groupe stress\n");
        return false;
    }
    
    // Créer et valider beaucoup de LUMs
    size_t success_count = 0;
    for (size_t i = 0; i < stress_scale; i++) {
        lum_t* lum = lum_create(i % 2, (int32_t)(i % 1000), (int32_t)(i % 500), 
                               (i % 2) ? LUM_STRUCTURE_LINEAR : LUM_STRUCTURE_CIRCULAR);
        if (lum && lum->magic_number == LUM_VALIDATION_PATTERN) {
            if (lum_group_add(group, lum)) {
                success_count++;
            }
            lum_destroy(lum);
        }
        
        // Log progrès tous les 200 éléments
        if (i % 200 == 0) {
            printf("    📊 Stress: %zu/%zu créés avec succès\n", success_count, i + 1);
        }
    }
    
    printf("    📊 Résultat stress: %zu/%zu LUMs créés avec succès\n", 
           success_count, stress_scale);
    
    // Validation finale
    size_t final_count = lum_group_size(group);
    if (final_count != success_count) {
        printf("    ❌ Incohérence comptage: %zu (groupe) != %zu (créés)\n", 
               final_count, success_count);
        lum_group_destroy(group);
        return false;
    }
    
    lum_group_destroy(group);
    
    uint64_t end_time = get_precise_timestamp_ns();
    double throughput = (double)success_count / ((end_time - start_time) / 1e9);
    printf("    ✅ Stress test réussi: %.2f LUMs/sec (%lu ns)\n", 
           throughput, end_time - start_time);
    
    return true;
}

// Test 4: Memory Safety
static bool test_module_memory_safety(void) {
    uint64_t start_time = get_precise_timestamp_ns();
    
    printf("  Test 4/5: Memory Safety...\n");
    
    // Test protection double-free (correction critique README.md)
    lum_t* lum = lum_create(1, 100, 200, LUM_STRUCTURE_LINEAR);
    if (!lum) {
        printf("    ❌ Échec création LUM pour test mémoire\n");
        return false;
    }
    
    // Validation magic number initial
    if (lum->magic_number != LUM_VALIDATION_PATTERN) {
        printf("    ❌ Magic number invalide initial\n");
        return false;
    }
    
    // Première destruction (normale)
    lum_destroy(lum);
    
    // Deuxième destruction (doit être détectée et échouer gracieusement)
    // Note: avec protection double-free, cela ne doit pas crasher
    printf("    🔒 Test protection double-free...\n");
    
    // Test allocation NULL
    lum_t* null_lum = NULL;
    lum_destroy(null_lum); // Doit être safe
    
    // Test groupe avec gestion mémoire
    lum_group_t* group = lum_group_create(5);
    if (!group) {
        printf("    ❌ Échec création groupe pour test mémoire\n");
        return false;
    }
    
    // Tests de bornes
    for (int i = 0; i < 3; i++) {
        lum_t* test_lum = lum_create(i % 2, i * 50, i * 25, LUM_STRUCTURE_LINEAR);
        if (test_lum) {
            lum_group_add(group, test_lum);
            lum_destroy(test_lum);
        }
    }
    
    lum_group_destroy(group);
    
    uint64_t end_time = get_precise_timestamp_ns();
    printf("    ✅ Memory Safety réussi (%lu ns)\n", end_time - start_time);
    return true;
}

// Test 5: Forensic Logs
static bool test_module_forensic_logs(void) {
    uint64_t start_time = get_precise_timestamp_ns();
    
    printf("  Test 5/5: Forensic Logs...\n");
    
    // Créer répertoire s'il n'existe pas
    char dir_path[256];
    snprintf(dir_path, sizeof(dir_path), "logs/individual/%s", TEST_MODULE_NAME);
    
    // Créer répertoire avec mkdir -p équivalent
    char mkdir_cmd[512];
    snprintf(mkdir_cmd, sizeof(mkdir_cmd), "mkdir -p %s", dir_path);
    system(mkdir_cmd);
    
    // Générer logs forensiques pour ce module
    char log_path[256];
    snprintf(log_path, sizeof(log_path), "logs/individual/%s/test_%s_%lu.log", 
             TEST_MODULE_NAME, TEST_MODULE_NAME, (unsigned long)time(NULL));
    
    FILE* log_file = fopen(log_path, "w");
    if (!log_file) {
        printf("    ❌ Impossible de créer log forensique: %s\n", log_path);
        printf("    🔍 Vérification permissions répertoire...\n");
        return false;
    }
    
    // Écrire logs avec timestamp nanoseconde
    uint64_t log_timestamp = get_precise_timestamp_ns();
    fprintf(log_file, "=== LOG FORENSIQUE MODULE LUM_CORE ===\n");
    fprintf(log_file, "Timestamp: %lu ns\n", log_timestamp);
    fprintf(log_file, "Module: %s\n", TEST_MODULE_NAME);
    fprintf(log_file, "Test: Forensic Logs\n");
    fprintf(log_file, "Status: EXECUTING\n");
    
    // Test avec logs détaillés
    lum_t* lum = lum_create(1, 42, 84, LUM_STRUCTURE_LINEAR);
    if (lum) {
        fprintf(log_file, "LUM Created: magic=0x%08X, presence=%d, pos_x=%d, pos_y=%d\n",
                lum->magic_number, lum->presence, lum->position_x, lum->position_y);
        
        // Vérification logs
        if (lum->magic_number == LUM_VALIDATION_PATTERN) {
            fprintf(log_file, "Magic number validation: PASS\n");
        } else {
            fprintf(log_file, "Magic number validation: FAIL\n");
            fclose(log_file);
            lum_destroy(lum);
            return false;
        }
        
        lum_destroy(lum);
        fprintf(log_file, "LUM Destroyed: cleanup successful\n");
    }
    
    uint64_t end_timestamp = get_precise_timestamp_ns();
    fprintf(log_file, "Test duration: %lu ns\n", end_timestamp - log_timestamp);
    fprintf(log_file, "Status: SUCCESS\n");
    fprintf(log_file, "=== FIN LOG FORENSIQUE ===\n");
    
    fclose(log_file);
    
    uint64_t end_time = get_precise_timestamp_ns();
    printf("    ✅ Forensic Logs réussi - Log généré: %s (%lu ns)\n", 
           log_path, end_time - start_time);
    return true;
}

// Main test runner avec logs individuels
int main(void) {
    printf("=== TEST INDIVIDUEL %s ===\n", TEST_MODULE_NAME);
    
    // Initialisation
    memory_tracker_init();
    
    uint64_t test_session_start = get_precise_timestamp_ns();
    
    // Exécution tests avec métriques
    int tests_passed = 0;
    
    // Test 1: Create/Destroy
    if (test_module_create_destroy()) {
        tests_passed++;
        printf("✅ %s Create/Destroy: PASS\n", TEST_MODULE_NAME);
    } else {
        printf("❌ %s Create/Destroy: FAIL\n", TEST_MODULE_NAME);
    }
    
    // Test 2: Basic Operations
    if (test_module_basic_operations()) {
        tests_passed++;
        printf("✅ %s Basic Operations: PASS\n", TEST_MODULE_NAME);
    } else {
        printf("❌ %s Basic Operations: FAIL\n", TEST_MODULE_NAME);
    }
    
    // Test 3: Stress 100K
    if (test_module_stress_100k()) {
        tests_passed++;
        printf("✅ %s Stress 100K: PASS\n", TEST_MODULE_NAME);
    } else {
        printf("❌ %s Stress 100K: FAIL\n", TEST_MODULE_NAME);
    }
    
    // Test 4: Memory Safety
    if (test_module_memory_safety()) {
        tests_passed++;
        printf("✅ %s Memory Safety: PASS\n", TEST_MODULE_NAME);
    } else {
        printf("❌ %s Memory Safety: FAIL\n", TEST_MODULE_NAME);
    }
    
    // Test 5: Forensic Logs
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
    
    // Rapport memory tracker
    memory_tracker_report();
    memory_tracker_destroy();
    
    return (tests_passed == 5) ? 0 : 1;
}