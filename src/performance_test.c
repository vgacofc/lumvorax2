#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "src/lum/lum_core.h"
#include "src/vorax/vorax_operations.h"
#include "src/binary/binary_lum_converter.h"
#include "src/crypto/crypto_validator.h"

double get_time_seconds() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

void test_lum_performance() {
    printf("=== TEST PERFORMANCE LUM ===\n");
    
    double start = get_time_seconds();
    const int count = 50000;
    
    // Test creation massive
    for (int i = 0; i < count; i++) {
        lum_t* lum = lum_create(i % 2, i, i*2, LUM_STRUCTURE_LINEAR);
        lum_destroy(lum);
    }
    
    double elapsed = get_time_seconds() - start;
    printf("Création/destruction de %d LUMs: %.3f secondes\n", count, elapsed);
    printf("Performance: %.0f LUMs/seconde\n", count / elapsed);
}

void test_crypto_performance() {
    printf("\n=== TEST PERFORMANCE CRYPTO ===\n");
    
    const char* test_data = "Ceci est un test de performance SHA-256 pour valider la rapidité du système LUM/VORAX";
    const int iterations = 10000;
    
    double start = get_time_seconds();
    
    for (int i = 0; i < iterations; i++) {
        uint8_t hash[32];
        sha256_hash((const uint8_t*)test_data, strlen(test_data), hash);
    }
    
    double elapsed = get_time_seconds() - start;
    printf("SHA-256 %d itérations: %.3f secondes\n", iterations, elapsed);
    printf("Performance: %.0f hashes/seconde\n", iterations / elapsed);
    printf("Débit: %.2f MB/s\n", (strlen(test_data) * iterations) / (1024.0 * 1024.0) / elapsed);
}

void test_conversion_performance() {
    printf("\n=== TEST PERFORMANCE CONVERSION ===\n");
    
    const int iterations = 5000;
    double start = get_time_seconds();
    
    for (int i = 0; i < iterations; i++) {
        // Test conversion int32 -> LUM -> int32
        binary_lum_result_t* result = convert_int32_to_lum(i);
        if (result && result->success) {
            int32_t back = convert_lum_to_int32(result->lum_group);
            if (back != i) {
                printf("Erreur conversion: %d != %d\n", i, back);
            }
            binary_lum_result_destroy(result);
        }
    }
    
    double elapsed = get_time_seconds() - start;
    printf("Conversion bidirectionnelle %d entiers: %.3f secondes\n", iterations, elapsed);
    printf("Performance: %.0f conversions/seconde\n", iterations / elapsed);
}

void test_vorax_performance() {
    printf("\n=== TEST PERFORMANCE VORAX ===\n");
    
    // Créer des groupes de test
    lum_group_t* group1 = lum_group_create(1000);
    lum_group_t* group2 = lum_group_create(1000);
    
    for (int i = 0; i < 1000; i++) {
        lum_t* lum1 = lum_create(i % 2, i, 0, LUM_STRUCTURE_LINEAR);
        lum_t* lum2 = lum_create((i+1) % 2, i, 1, LUM_STRUCTURE_LINEAR);
        lum_group_add(group1, lum1);
        lum_group_add(group2, lum2);
        lum_destroy(lum1);
        lum_destroy(lum2);
    }
    
    const int iterations = 1000;
    double start = get_time_seconds();
    
    for (int i = 0; i < iterations; i++) {
        vorax_result_t* result = vorax_fuse(group1, group2);
        if (result) {
            vorax_result_destroy(result);
        }
    }
    
    double elapsed = get_time_seconds() - start;
    printf("Opérations FUSE %d fois: %.3f secondes\n", iterations, elapsed);
    printf("Performance: %.0f opérations/seconde\n", iterations / elapsed);
    
    lum_group_destroy(group1);
    lum_group_destroy(group2);
}

void test_memory_usage() {
    printf("\n=== TEST USAGE MÉMOIRE ===\n");
    
    // Test allocation progressive
    const int sizes[] = {1000, 5000, 10000, 20000};
    const int num_sizes = sizeof(sizes) / sizeof(sizes[0]);
    
    for (int s = 0; s < num_sizes; s++) {
        int count = sizes[s];
        
        double start = get_time_seconds();
        
        lum_group_t* group = lum_group_create(count);
        for (int i = 0; i < count; i++) {
            lum_t* lum = lum_create(i % 2, i, i, LUM_STRUCTURE_LINEAR);
            lum_group_add(group, lum);
            lum_destroy(lum);
        }
        
        double creation_time = get_time_seconds() - start;
        
        start = get_time_seconds();
        lum_group_destroy(group);
        double destruction_time = get_time_seconds() - start;
        
        printf("%d LUMs - Création: %.3fs, Destruction: %.3fs\n", 
               count, creation_time, destruction_time);
    }
}

int main() {
    printf("=== BENCHMARK PERFORMANCE LUM/VORAX ===\n");
    printf("Début des tests de performance...\n\n");
    
    double total_start = get_time_seconds();
    
    test_lum_performance();
    test_crypto_performance();
    test_conversion_performance();
    test_vorax_performance();
    test_memory_usage();
    
    double total_time = get_time_seconds() - total_start;
    
    printf("\n=== RÉSUMÉ PERFORMANCE ===\n");
    printf("Temps total: %.3f secondes\n", total_time);
    printf("Système: %ld cœurs CPU\n", sysconf(_SC_NPROCESSORS_ONLN));
    printf("Tests terminés avec succès!\n");
    
    return 0;
}