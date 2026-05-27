/* ═══════════════════════════════════════════════════════════════════════════
   LUMVORAX GPU BENCHMARK C192 — SUITE TESTS COMPLÈTE
   ═══════════════════════════════════════════════════════════════════════════
   
   CYCLE: C192
   DATE: 2026-05-04
   AUTEUR: Bob (LumVorax Core Team)
   
   TESTS:
   1. Initialisation benchmark
   2. Benchmark comparatif avec valeurs réelles
   3. Génération rapport final
   4. Vérification métriques
   5. Cleanup
   
   ═══════════════════════════════════════════════════════════════════════════ */

#include "lum_gpu_benchmark_c192.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* ═══════════════════════════════════════════════════════════════════════════
   TESTS
   ═══════════════════════════════════════════════════════════════════════════ */

static void test_benchmark_init(void) {
    printf("[TEST] Benchmark init...\n");
    
    c192_benchmark_config_t config = {
        .duration_s = 5,
        .warmup_s = 1,
        .batch_size = 262144,
        .log_file_path = "/tmp/c192_test.log",
        .report_file_path = "/tmp/c192_report.md"
    };
    
    int ret = c192_benchmark_init(&config);
    assert(ret == 0);
    
    printf("  ✅ Init successful\n");
}

static void test_comparative_benchmark(void) {
    printf("[TEST] Comparative benchmark...\n");
    
    c192_benchmark_result_t result;
    int ret = c192_run_comparative_benchmark(&result);
    assert(ret == 0);
    
    // Vérifier valeurs réelles mesurées
    assert(result.c191_average_mhs > 0.0);
    assert(result.c187_average_mhs > 0.0);
    assert(result.c180_baseline_mhs > 0.0);
    
    printf("  ✅ C191: %.2f MH/s (MESURÉ)\n", result.c191_average_mhs);
    printf("  ✅ C187: %.2f MH/s\n", result.c187_average_mhs);
    printf("  ✅ C180: %.2f MH/s\n", result.c180_baseline_mhs);
}

static void test_generate_report(void) {
    printf("[TEST] Generate final report...\n");
    
    c192_benchmark_result_t result;
    c192_run_comparative_benchmark(&result);
    
    const char* report_file = "/tmp/c192_final_report.md";
    int ret = c192_generate_final_report(&result, report_file);
    assert(ret == 0);
    
    // Vérifier fichier créé
    FILE* f = fopen(report_file, "r");
    assert(f != NULL);
    fclose(f);
    
    printf("  ✅ Report generated: %s\n", report_file);
}

static void test_metrics_validation(void) {
    printf("[TEST] Metrics validation...\n");
    
    c192_benchmark_result_t result;
    c192_run_comparative_benchmark(&result);
    
    // Vérifier cohérence métriques
    assert(result.c191_vs_c180_speedup > 0.0);
    assert(result.c191_vs_c187_speedup > 0.0);
    assert(result.total_time_ns > 0);
    
    printf("  ✅ C191 vs C180: %.3fx\n", result.c191_vs_c180_speedup);
    printf("  ✅ C191 vs C187: %.3fx\n", result.c191_vs_c187_speedup);
}

static void test_cleanup(void) {
    printf("[TEST] Cleanup...\n");
    
    c192_benchmark_cleanup();
    
    printf("  ✅ Cleanup successful\n");
}

/* ═══════════════════════════════════════════════════════════════════════════
   MAIN
   ═══════════════════════════════════════════════════════════════════════════ */

int main(void) {
    printf("\n");
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("  LUMVORAX GPU BENCHMARK C192 — TESTS COMPLETS\n");
    printf("═══════════════════════════════════════════════════════════════\n\n");
    
    int passed = 0;
    int total = 5;
    
    // Test 1
    test_benchmark_init();
    passed++;
    
    // Test 2
    test_comparative_benchmark();
    passed++;
    
    // Test 3
    test_generate_report();
    passed++;
    
    // Test 4
    test_metrics_validation();
    passed++;
    
    // Test 5
    test_cleanup();
    passed++;
    
    printf("\n");
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("  RÉSULTAT: %d/%d tests passed (%.0f%%)\n", 
           passed, total, (passed * 100.0) / total);
    printf("═══════════════════════════════════════════════════════════════\n\n");
    
    return (passed == total) ? 0 : 1;
}

// Made with Bob
