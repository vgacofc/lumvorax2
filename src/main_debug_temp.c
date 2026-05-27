#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>

// INCLUDES MINIMAL POUR DEBUG
#include "common/common_types.h"
#include "lum/lum_core.h"
#include "debug/memory_tracker.h"

// Fonction pour vérifier existence répertoire
bool check_directory_exists(const char* path) {
    struct stat st;
    bool exists = (stat(path, &st) == 0 && S_ISDIR(st.st_mode));
    printf("[DEBUG] Vérification répertoire %s: %s\n", path, exists ? "EXISTS" : "MISSING");
    return exists;
}

// Fonction pour créer répertoire si nécessaire
bool ensure_directory_exists(const char* path) {
    if (check_directory_exists(path)) {
        return true;
    }
    
    printf("[DEBUG] Création répertoire %s...\n", path);
    if (mkdir(path, 0755) == 0) {
        printf("[SUCCESS] Répertoire créé: %s\n", path);
        return true;
    } else {
        printf("[ERROR] Échec création répertoire: %s\n", path);
        return false;
    }
}

// Test minimal LUM core sans forensique
void test_basic_lum_operations() {
    printf("[TEST] === TEST BASIC LUM OPERATIONS ===\n");
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    // Test création LUM simple
    printf("  Test 1: Création LUM...\n");
    lum_t* test_lum = lum_create(1, 100, 200, LUM_STRUCTURE_LINEAR);
    if (test_lum) {
        printf("  [SUCCESS] LUM créée: ID=%u, pos_x=%d, pos_y=%d\n", test_lum->id, test_lum->position_x, test_lum->position_y);
        
        // Test destruction
        printf("  Test 2: Destruction LUM...\n");
        lum_destroy(test_lum);
        printf("  [SUCCESS] LUM détruite\n");
    } else {
        printf("  [ERROR] Échec création LUM\n");
        return;
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("[TIMING] Test terminé en %.6f secondes\n", elapsed);
}

// Test stress progressif simplifié
void test_progressive_stress_simple() {
    printf("[TEST] === TEST STRESS PROGRESSIF SIMPLIFIÉ ===\n");
    
    size_t scales[] = {10, 100, 1000, 10000};
    size_t num_scales = sizeof(scales) / sizeof(scales[0]);
    
    for (size_t i = 0; i < num_scales; i++) {
        size_t scale = scales[i];
        printf("\n[TEST] Échelle %zu éléments...\n", scale);
        
        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);
        
        // Créer un groupe LUM
        lum_group_t* group = lum_group_create(scale > 1000 ? 1000 : scale);
        if (!group) {
            printf("  [ERROR] Échec création groupe LUM\n");
            continue;
        }
        
        size_t created = 0;
        size_t batch_size = scale > 5000 ? 5000 : scale;
        
        for (size_t j = 0; j < batch_size; j++) {
            lum_t* lum = lum_create(j % 2, (int32_t)(j % 1000), (int32_t)(j / 10), LUM_STRUCTURE_LINEAR);
            if (lum) {
                if (lum_group_add(group, lum)) {
                    created++;
                }
                lum_destroy(lum);
            }
            
            // Progress report
            if (j > 0 && j % 100 == 0) {
                printf("    Progress: %zu/%zu (%.1f%%)\n", j, batch_size, (double)j/batch_size*100);
                fflush(stdout);
            }
        }
        
        clock_gettime(CLOCK_MONOTONIC, &end);
        double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
        
        printf("  [SUCCESS] Échelle %zu: %zu LUMs créées en %.3f sec (%.0f ops/sec)\n", 
               scale, created, elapsed, created / elapsed);
        
        lum_group_destroy(group);
    }
}

int main(int argc, char* argv[]) {
    printf("[TEST] === MAIN DEBUG TEMPORAIRE - DIAGNOSTIC LUM/VORAX ===\n");
    printf("Version: DEBUG v1.0\n");
    printf("Date: %s %s\n", __DATE__, __TIME__);
    
    // Étape 1: Vérifier les répertoires
    printf("\n[SETUP] === VÉRIFICATION RÉPERTOIRES ===\n");
    ensure_directory_exists("logs");
    ensure_directory_exists("logs/forensic");
    ensure_directory_exists("logs/tests");
    ensure_directory_exists("logs/execution");
    
    // Étape 2: Initialisation minimale
    printf("\n[SETUP] === INITIALISATION MEMORY TRACKER ===\n");
    memory_tracker_init();
    printf("[SUCCESS] Memory tracker initialisé\n");
    
    // Étape 3: Tests selon argument
    if (argc > 1 && strcmp(argv[1], "--progressive-stress-all") == 0) {
        printf("\n[TEST] === MODE STRESS PROGRESSIF ===\n");
        test_progressive_stress_simple();
    } else if (argc > 1 && strcmp(argv[1], "--basic-test") == 0) {
        printf("\n[TEST] === MODE TEST BASIC ===\n");
        test_basic_lum_operations();
    } else {
        printf("\n[HELP] === AIDE ===\n");
        printf("Usage: %s [--basic-test|--progressive-stress-all]\n", argv[0]);
        printf("  --basic-test            : Test minimal LUM core\n");
        printf("  --progressive-stress-all: Test stress progressif 10→10K\n");
        
        // Test par défaut
        printf("\n[TEST] === EXÉCUTION TEST PAR DÉFAUT ===\n");
        test_basic_lum_operations();
    }
    
    // Rapport final
    printf("\n[METRICS] === RAPPORT FINAL ===\n");
    memory_tracker_report();
    
    // Nettoyage
    printf("\n[CLEANUP] === NETTOYAGE ===\n");
    memory_tracker_destroy();
    printf("[SUCCESS] Nettoyage terminé\n");
    
    return 0;
}