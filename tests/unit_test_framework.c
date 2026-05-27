/**
 * @file unit_test_framework.c
 * @brief Implémentation du framework de tests unitaires LumVorax
 * @version 1.0
 * @date 2026-04-30
 * @cycle C147
 */

#define _POSIX_C_SOURCE 200809L
#include "unit_test_framework.h"
#include <time.h>
#include <unistd.h>

/* ============================================================================
 * FONCTIONS UTILITAIRES INTERNES
 * ============================================================================ */

/**
 * @brief Obtient le timestamp actuel en nanosecondes
 */
static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return 0;
    }
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

/**
 * @brief Vérifie si la sortie colorée est supportée
 */
static bool is_color_supported(void) {
    return isatty(STDOUT_FILENO) != 0;
}

/* ============================================================================
 * IMPLÉMENTATION - GESTION DU FRAMEWORK
 * ============================================================================ */

test_framework_t* test_framework_init(bool verbose, bool color_output) {
    test_framework_t* framework = (test_framework_t*)calloc(1, sizeof(test_framework_t));
    if (!framework) {
        return NULL;
    }
    
    framework->magic = TEST_FRAMEWORK_MAGIC;
    framework->verbose = verbose;
    framework->color_output = color_output && is_color_supported();
    framework->suite_count = 0;
    framework->fixture_count = 0;
    framework->report_file = NULL;
    
    if (pthread_mutex_init(&framework->global_mutex, NULL) != 0) {
        free(framework);
        return NULL;
    }
    
    return framework;
}

void test_framework_destroy(test_framework_t* framework) {
    if (!framework || framework->magic != TEST_FRAMEWORK_MAGIC) {
        return;
    }
    
    // Fermer le fichier de rapport si ouvert
    if (framework->report_file) {
        fclose(framework->report_file);
    }
    
    // Détruire les mutex des suites
    for (uint32_t i = 0; i < framework->suite_count; i++) {
        pthread_mutex_destroy(&framework->suites[i].mutex);
    }
    
    pthread_mutex_destroy(&framework->global_mutex);
    framework->magic = 0;
    free(framework);
}

test_suite_t* test_suite_create(test_framework_t* framework, const char* suite_name) {
    if (!framework || !suite_name || framework->suite_count >= MAX_TEST_SUITES) {
        return NULL;
    }
    
    pthread_mutex_lock(&framework->global_mutex);
    
    test_suite_t* suite = &framework->suites[framework->suite_count];
    strncpy(suite->name, suite_name, MAX_TEST_NAME_LEN - 1);
    suite->name[MAX_TEST_NAME_LEN - 1] = '\0';
    suite->test_count = 0;
    suite->magic = TEST_FRAMEWORK_MAGIC;
    
    memset(&suite->stats, 0, sizeof(test_stats_t));
    
    if (pthread_mutex_init(&suite->mutex, NULL) != 0) {
        pthread_mutex_unlock(&framework->global_mutex);
        return NULL;
    }
    
    framework->suite_count++;
    pthread_mutex_unlock(&framework->global_mutex);
    
    return suite;
}

int test_suite_add_test(test_suite_t* suite, const char* test_name, 
                        test_func_t test_func, test_fixture_t* fixture) {
    if (!suite || !test_name || !test_func || suite->test_count >= MAX_TESTS_PER_SUITE) {
        return -1;
    }
    
    pthread_mutex_lock(&suite->mutex);
    
    test_case_t* test = &suite->tests[suite->test_count];
    strncpy(test->name, test_name, MAX_TEST_NAME_LEN - 1);
    test->name[MAX_TEST_NAME_LEN - 1] = '\0';
    test->func = test_func;
    test->fixture = fixture;
    test->enabled = true;
    test->magic = TEST_FRAMEWORK_MAGIC;
    
    suite->test_count++;
    pthread_mutex_unlock(&suite->mutex);
    
    return 0;
}

test_stats_t test_suite_run(test_suite_t* suite) {
    test_stats_t stats = {0};
    
    if (!suite || suite->magic != TEST_FRAMEWORK_MAGIC) {
        return stats;
    }
    
    printf("\n%s=== Running Test Suite: %s ===%s\n", 
           COLOR_BOLD, suite->name, COLOR_RESET);
    
    uint64_t suite_start = get_timestamp_ns();
    
    for (uint32_t i = 0; i < suite->test_count; i++) {
        test_case_t* test = &suite->tests[i];
        
        if (!test->enabled) {
            stats.skipped_tests++;
            continue;
        }
        
        printf("  [%3u/%3u] %s ... ", i + 1, suite->test_count, test->name);
        fflush(stdout);
        
        void* context = NULL;
        
        // Setup fixture si présente
        if (test->fixture && test->fixture->setup) {
            context = test->fixture->setup();
        }
        
        uint64_t test_start = get_timestamp_ns();
        int result = test->func(context);
        uint64_t test_end = get_timestamp_ns();
        
        // Teardown fixture si présente
        if (test->fixture && test->fixture->teardown) {
            test->fixture->teardown(context);
        }
        
        uint64_t duration_ns = test_end - test_start;
        stats.total_duration_ns += duration_ns;
        stats.total_tests++;
        
        switch (result) {
            case TEST_SUCCESS:
                printf("%s[PASS]%s (%.3f ms)\n", 
                       COLOR_GREEN, COLOR_RESET, duration_ns / 1000000.0);
                stats.passed_tests++;
                break;
            case TEST_FAILURE:
                printf("%s[FAIL]%s (%.3f ms)\n", 
                       COLOR_RED, COLOR_RESET, duration_ns / 1000000.0);
                stats.failed_tests++;
                break;
            case TEST_SKIPPED:
                printf("%s[SKIP]%s\n", COLOR_YELLOW, COLOR_RESET);
                stats.skipped_tests++;
                break;
            default:
                printf("%s[ERROR]%s (%.3f ms)\n", 
                       COLOR_MAGENTA, COLOR_RESET, duration_ns / 1000000.0);
                stats.error_tests++;
                break;
        }
    }
    
    uint64_t suite_end = get_timestamp_ns();
    stats.total_duration_ns = suite_end - suite_start;
    
    if (stats.total_tests > 0) {
        stats.pass_rate = (double)stats.passed_tests / stats.total_tests * 100.0;
    }
    
    suite->stats = stats;
    
    return stats;
}

test_stats_t test_framework_run_all(test_framework_t* framework) {
    test_stats_t global_stats = {0};
    
    if (!framework || framework->magic != TEST_FRAMEWORK_MAGIC) {
        return global_stats;
    }
    
    printf("\n%s╔════════════════════════════════════════════════════════════╗%s\n", 
           COLOR_CYAN, COLOR_RESET);
    printf("%s║         LumVorax Unit Test Framework v%s         ║%s\n", 
           COLOR_CYAN, TEST_FRAMEWORK_VERSION, COLOR_RESET);
    printf("%s╚════════════════════════════════════════════════════════════╝%s\n", 
           COLOR_CYAN, COLOR_RESET);
    
    uint64_t global_start = get_timestamp_ns();
    
    for (uint32_t i = 0; i < framework->suite_count; i++) {
        test_stats_t suite_stats = test_suite_run(&framework->suites[i]);
        
        global_stats.total_tests += suite_stats.total_tests;
        global_stats.passed_tests += suite_stats.passed_tests;
        global_stats.failed_tests += suite_stats.failed_tests;
        global_stats.skipped_tests += suite_stats.skipped_tests;
        global_stats.error_tests += suite_stats.error_tests;
    }
    
    uint64_t global_end = get_timestamp_ns();
    global_stats.total_duration_ns = global_end - global_start;
    
    if (global_stats.total_tests > 0) {
        global_stats.pass_rate = (double)global_stats.passed_tests / global_stats.total_tests * 100.0;
    }
    
    return global_stats;
}

/* ============================================================================
 * IMPLÉMENTATION - FIXTURES
 * ============================================================================ */

test_fixture_t* test_fixture_create(test_framework_t* framework, const char* name,
                                    void* (*setup)(void), void (*teardown)(void*)) {
    if (!framework || !name || framework->fixture_count >= MAX_FIXTURES) {
        return NULL;
    }
    
    pthread_mutex_lock(&framework->global_mutex);
    
    test_fixture_t* fixture = &framework->fixtures[framework->fixture_count];
    strncpy(fixture->name, name, MAX_TEST_NAME_LEN - 1);
    fixture->name[MAX_TEST_NAME_LEN - 1] = '\0';
    fixture->setup = setup;
    fixture->teardown = teardown;
    fixture->context = NULL;
    fixture->magic = TEST_FRAMEWORK_MAGIC;
    
    framework->fixture_count++;
    pthread_mutex_unlock(&framework->global_mutex);
    
    return fixture;
}

/* ============================================================================
 * IMPLÉMENTATION - RAPPORTS
 * ============================================================================ */

void test_stats_print(const test_stats_t* stats) {
    if (!stats) {
        return;
    }
    
    printf("\n%s╔════════════════════════════════════════════════════════════╗%s\n", 
           COLOR_BOLD, COLOR_RESET);
    printf("%s║                    TEST SUMMARY                            ║%s\n", 
           COLOR_BOLD, COLOR_RESET);
    printf("%s╠════════════════════════════════════════════════════════════╣%s\n", 
           COLOR_BOLD, COLOR_RESET);
    printf("║  Total Tests:    %s%5u%s                                      ║\n", 
           COLOR_CYAN, stats->total_tests, COLOR_RESET);
    printf("║  Passed:         %s%5u%s                                      ║\n", 
           COLOR_GREEN, stats->passed_tests, COLOR_RESET);
    printf("║  Failed:         %s%5u%s                                      ║\n", 
           COLOR_RED, stats->failed_tests, COLOR_RESET);
    printf("║  Skipped:        %s%5u%s                                      ║\n", 
           COLOR_YELLOW, stats->skipped_tests, COLOR_RESET);
    printf("║  Errors:         %s%5u%s                                      ║\n", 
           COLOR_MAGENTA, stats->error_tests, COLOR_RESET);
    printf("║  Pass Rate:      %s%5.1f%%%s                                    ║\n", 
           stats->pass_rate >= 80.0 ? COLOR_GREEN : COLOR_RED, 
           stats->pass_rate, COLOR_RESET);
    printf("║  Duration:       %s%5.3f s%s                                  ║\n", 
           COLOR_CYAN, stats->total_duration_ns / 1000000000.0, COLOR_RESET);
    printf("%s╚════════════════════════════════════════════════════════════╝%s\n\n", 
           COLOR_BOLD, COLOR_RESET);
}

void test_framework_generate_report(test_framework_t* framework, const char* output_file) {
    if (!framework) {
        return;
    }
    
    FILE* fp = output_file ? fopen(output_file, "w") : stdout;
    if (!fp) {
        return;
    }
    
    fprintf(fp, "LumVorax Unit Test Report\n");
    fprintf(fp, "=========================\n");
    fprintf(fp, "Framework Version: %s\n", TEST_FRAMEWORK_VERSION);
    fprintf(fp, "Generated: %ld\n\n", time(NULL));
    
    for (uint32_t i = 0; i < framework->suite_count; i++) {
        test_suite_t* suite = &framework->suites[i];
        fprintf(fp, "\nTest Suite: %s\n", suite->name);
        fprintf(fp, "  Total: %u, Passed: %u, Failed: %u, Skipped: %u\n",
                suite->stats.total_tests, suite->stats.passed_tests,
                suite->stats.failed_tests, suite->stats.skipped_tests);
        fprintf(fp, "  Pass Rate: %.1f%%\n", suite->stats.pass_rate);
        fprintf(fp, "  Duration: %.3f s\n", suite->stats.total_duration_ns / 1000000000.0);
    }
    
    if (output_file) {
        fclose(fp);
    }
}

void test_framework_generate_json_report(test_framework_t* framework, const char* output_file) {
    if (!framework || !output_file) {
        return;
    }
    
    FILE* fp = fopen(output_file, "w");
    if (!fp) {
        return;
    }
    
    fprintf(fp, "{\n");
    fprintf(fp, "  \"framework_version\": \"%s\",\n", TEST_FRAMEWORK_VERSION);
    fprintf(fp, "  \"timestamp\": %ld,\n", time(NULL));
    fprintf(fp, "  \"suites\": [\n");
    
    for (uint32_t i = 0; i < framework->suite_count; i++) {
        test_suite_t* suite = &framework->suites[i];
        fprintf(fp, "    {\n");
        fprintf(fp, "      \"name\": \"%s\",\n", suite->name);
        fprintf(fp, "      \"total_tests\": %u,\n", suite->stats.total_tests);
        fprintf(fp, "      \"passed\": %u,\n", suite->stats.passed_tests);
        fprintf(fp, "      \"failed\": %u,\n", suite->stats.failed_tests);
        fprintf(fp, "      \"skipped\": %u,\n", suite->stats.skipped_tests);
        fprintf(fp, "      \"pass_rate\": %.2f,\n", suite->stats.pass_rate);
        fprintf(fp, "      \"duration_ns\": %lu\n", suite->stats.total_duration_ns);
        fprintf(fp, "    }%s\n", (i < framework->suite_count - 1) ? "," : "");
    }
    
    fprintf(fp, "  ]\n");
    fprintf(fp, "}\n");
    
    fclose(fp);
}

// Made with Bob
