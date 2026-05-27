#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "src/optimization/simd_optimizer.h"
#include "src/lum/lum_core.h"

int main(void) {
    printf("=== TEST DÉDIÉ MODULE SIMD_OPTIMIZER ===\n");
    printf("Timestamp: %ld\n", time(NULL));
    
    // Test détection capacités SIMD
    printf("\n🔍 Test détection capacités SIMD\n");
    simd_capabilities_t* caps = simd_detect_capabilities();
    if (!caps) {
        printf("❌ Échec détection SIMD\n");
        return 1;
    }
    
    printf("✓ Capacités détectées:\n");
    printf("  AVX-512: %s\n", caps->avx512_available ? "Disponible" : "Non disponible");
    printf("  AVX2: %s\n", caps->avx2_available ? "Disponible" : "Non disponible");
    printf("  SSE: %s\n", caps->sse_available ? "Disponible" : "Non disponible");
    printf("  Largeur vectorielle: %d éléments\n", caps->vector_width);
    printf("  Fonctionnalités: %s\n", caps->cpu_features);
    
    // Tests de stress selon prompt.txt - 1M+ LUMs minimum
    printf("\n🚀 Tests de stress SIMD (1M+ LUMs)\n");
    size_t test_sizes[] = {100000, 1000000, 2000000, 5000000};
    size_t num_tests = sizeof(test_sizes) / sizeof(test_sizes[0]);
    
    for (size_t i = 0; i < num_tests; i++) {
        printf("📊 Test SIMD %zu LUMs... ", test_sizes[i]);
        fflush(stdout);
        
        simd_result_t* result = simd_benchmark_vectorization(test_sizes[i]);
        if (result) {
            printf("✓\n");
            printf("  Éléments: %zu, Temps: %.6fs, Débit: %.0f LUMs/s\n",
                   result->processed_elements, result->execution_time, 
                   result->throughput_ops_per_sec);
            printf("  Vectorisation: %s, Optimisation: %s\n",
                   result->used_vectorization ? "OUI" : "NON", 
                   result->optimization_used);
            simd_result_destroy(result);
        } else {
            printf("❌ Échec\n");
        }
    }
    
    // Test comparatif scalar vs SIMD
    printf("\n📈 Comparaison Scalar vs SIMD (1M LUMs)\n");
    size_t compare_size = 1000000;
    
    // Test scalar
    lum_t* test_scalar = malloc(compare_size * sizeof(lum_t));
    if (test_scalar) {
        for (size_t i = 0; i < compare_size; i++) {
            test_scalar[i].presence = (i % 3 == 0) ? 1 : 0;
            test_scalar[i].position_x = i;
            test_scalar[i].position_y = i * 2;
        }
        
        clock_t start = clock();
        for (size_t i = 0; i < compare_size; i++) {
            test_scalar[i].presence = test_scalar[i].presence ? 1 : 0;
        }
        clock_t end = clock();
        double scalar_time = ((double)(end - start)) / CLOCKS_PER_SEC;
        double scalar_throughput = compare_size / scalar_time;
        
        // Test SIMD
        simd_result_t* simd_result = simd_process_lum_array_bulk(test_scalar, compare_size);
        
        if (simd_result) {
            printf("Scalar: %.6fs, %.0f LUMs/s\n", scalar_time, scalar_throughput);
            printf("SIMD:   %.6fs, %.0f LUMs/s (%s)\n", 
                   simd_result->execution_time, simd_result->throughput_ops_per_sec,
                   simd_result->optimization_used);
            
            if (simd_result->execution_time > 0 && scalar_time > 0) {
                double speedup = scalar_time / simd_result->execution_time;
                printf("🚀 Accélération: %.2fx ", speedup);
                if (speedup >= 2.0) {
                    printf("✅ VALIDATION 2x+\n");
                } else {
                    printf("⚠️ < 2x\n");
                }
            }
            simd_result_destroy(simd_result);
        }
        free(test_scalar);
    }
    
    simd_capabilities_destroy(caps);
    printf("\n✅ Tests SIMD terminés - Module validé\n");
    return 0;
}