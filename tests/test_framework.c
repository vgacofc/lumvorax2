/**
 * @file test_framework.c
 * @brief Implémentation Framework Tests Unitaires LumVorax
 * @author LumVorax Team
 * @date 2026-04-30
 * @version 1.0.0
 *
 * Conformité : STANDARD_NAMES.md v4.2
 * Compilation : gcc -Wall -Wextra -Werror -std=c11
 */

#define _POSIX_C_SOURCE 199309L

#include "test_framework.h"
#include <stdarg.h>
#include <sys/time.h>

/* ========================================================================
 * VARIABLES GLOBALES
 * ======================================================================== */

test_context_t g_test_context = {0};

/* ========================================================================
 * FONCTIONS UTILITAIRES
 * ======================================================================== */

/**
 * @brief Obtient le temps actuel en millisecondes
 */
static double get_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
}

/**
 * @brief Calcule la durée depuis start_time
 */
static double get_duration_ms(struct timespec* start) {
    struct timespec end;
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    double start_ms = (double)start->tv_sec * 1000.0 + (double)start->tv_nsec / 1000000.0;
    double end_ms = (double)end.tv_sec * 1000.0 + (double)end.tv_nsec / 1000000.0;
    
    return end_ms - start_ms;
}

/* ========================================================================
 * API PUBLIQUE
 * ======================================================================== */

void test_framework_init(void) {
    memset(&g_test_context, 0, sizeof(test_context_t));
    
    printf("\n");
    printf(COLOR_CYAN "╔════════════════════════════════════════════════════════════╗\n" COLOR_RESET);
    printf(COLOR_CYAN "║" COLOR_RESET "  " COLOR_MAGENTA "LumVorax Test Framework v1.0.0" COLOR_RESET "                        " COLOR_CYAN "║\n" COLOR_RESET);
    printf(COLOR_CYAN "╚════════════════════════════════════════════════════════════╝\n" COLOR_RESET);
    printf("\n");
}

void test_start(const char* name) {
    if (g_test_context.total_tests >= TEST_MAX_TESTS) {
        fprintf(stderr, COLOR_RED "ERROR: Maximum number of tests reached (%d)\n" COLOR_RESET, 
                TEST_MAX_TESTS);
        return;
    }
    
    test_info_t* test = &g_test_context.tests[g_test_context.total_tests];
    g_test_context.current_test = test;
    
    strncpy(test->name, name, TEST_MAX_NAME_LEN - 1);
    test->name[TEST_MAX_NAME_LEN - 1] = '\0';
    test->result = TEST_PASS;  // Par défaut PASS
    test->message[0] = '\0';
    test->duration_ms = 0.0;
    
    clock_gettime(CLOCK_MONOTONIC, &g_test_context.start_time);
    
    printf(COLOR_BLUE "[ RUN      ]" COLOR_RESET " %s\n", name);
}

void test_end(void) {
    if (g_test_context.current_test == NULL) {
        return;
    }
    
    test_info_t* test = g_test_context.current_test;
    
    // Calculer durée
    test->duration_ms = get_duration_ms(&g_test_context.start_time);
    g_test_context.total_duration_ms += test->duration_ms;
    
    // Afficher résultat
    if (test->result == TEST_PASS) {
        printf(COLOR_GREEN "[       OK ]" COLOR_RESET " %s (%.2f ms)\n", 
               test->name, test->duration_ms);
        g_test_context.passed_tests++;
    }
    
    g_test_context.total_tests++;
    g_test_context.current_test = NULL;
}

void test_fail(const char* file, int line, const char* format, ...) {
    if (g_test_context.current_test == NULL) {
        return;
    }
    
    test_info_t* test = g_test_context.current_test;
    test->result = TEST_FAIL;
    test->file = file;
    test->line = line;
    
    // Formater message
    va_list args;
    va_start(args, format);
    vsnprintf(test->message, TEST_MAX_MESSAGE_LEN, format, args);
    va_end(args);
    
    // Calculer durée
    test->duration_ms = get_duration_ms(&g_test_context.start_time);
    g_test_context.total_duration_ms += test->duration_ms;
    
    // Afficher échec
    printf(COLOR_RED "[  FAILED  ]" COLOR_RESET " %s (%.2f ms)\n", 
           test->name, test->duration_ms);
    printf(COLOR_RED "  %s:%d\n" COLOR_RESET, file, line);
    printf(COLOR_RED "  %s\n" COLOR_RESET, test->message);
    
    g_test_context.failed_tests++;
    g_test_context.total_tests++;
    g_test_context.current_test = NULL;
}

void test_skip(const char* name, const char* reason) {
    if (g_test_context.total_tests >= TEST_MAX_TESTS) {
        return;
    }
    
    test_info_t* test = &g_test_context.tests[g_test_context.total_tests];
    
    strncpy(test->name, name, TEST_MAX_NAME_LEN - 1);
    test->name[TEST_MAX_NAME_LEN - 1] = '\0';
    test->result = TEST_SKIP;
    strncpy(test->message, reason, TEST_MAX_MESSAGE_LEN - 1);
    test->message[TEST_MAX_MESSAGE_LEN - 1] = '\0';
    test->duration_ms = 0.0;
    
    printf(COLOR_YELLOW "[ SKIPPED  ]" COLOR_RESET " %s\n", name);
    printf(COLOR_YELLOW "  Reason: %s\n" COLOR_RESET, reason);
    
    g_test_context.skipped_tests++;
    g_test_context.total_tests++;
}

void test_print_results(void) {
    printf("\n");
    printf(COLOR_CYAN "╔════════════════════════════════════════════════════════════╗\n" COLOR_RESET);
    printf(COLOR_CYAN "║" COLOR_RESET "  " COLOR_MAGENTA "Test Results Summary" COLOR_RESET "                                   " COLOR_CYAN "║\n" COLOR_RESET);
    printf(COLOR_CYAN "╚════════════════════════════════════════════════════════════╝\n" COLOR_RESET);
    printf("\n");
    
    printf("Total tests:   %u\n", g_test_context.total_tests);
    printf(COLOR_GREEN "Passed:        %u\n" COLOR_RESET, g_test_context.passed_tests);
    
    if (g_test_context.failed_tests > 0) {
        printf(COLOR_RED "Failed:        %u\n" COLOR_RESET, g_test_context.failed_tests);
    } else {
        printf("Failed:        %u\n", g_test_context.failed_tests);
    }
    
    if (g_test_context.skipped_tests > 0) {
        printf(COLOR_YELLOW "Skipped:       %u\n" COLOR_RESET, g_test_context.skipped_tests);
    } else {
        printf("Skipped:       %u\n", g_test_context.skipped_tests);
    }
    
    printf("Total time:    %.2f ms\n", g_test_context.total_duration_ms);
    
    // Calculer taux succès
    if (g_test_context.total_tests > 0) {
        double success_rate = (double)g_test_context.passed_tests / 
                             (double)g_test_context.total_tests * 100.0;
        printf("Success rate:  %.1f%%\n", success_rate);
    }
    
    printf("\n");
    
    // Afficher tests échoués
    if (g_test_context.failed_tests > 0) {
        printf(COLOR_RED "Failed tests:\n" COLOR_RESET);
        for (uint32_t i = 0; i < g_test_context.total_tests; i++) {
            test_info_t* test = &g_test_context.tests[i];
            if (test->result == TEST_FAIL) {
                printf(COLOR_RED "  - %s\n" COLOR_RESET, test->name);
                printf(COLOR_RED "    %s:%d\n" COLOR_RESET, test->file, test->line);
                printf(COLOR_RED "    %s\n" COLOR_RESET, test->message);
            }
        }
        printf("\n");
    }
    
    // Verdict final
    if (g_test_context.failed_tests == 0) {
        printf(COLOR_GREEN "╔════════════════════════════════════════════════════════════╗\n" COLOR_RESET);
        printf(COLOR_GREEN "║" COLOR_RESET "  " COLOR_GREEN "✓ ALL TESTS PASSED" COLOR_RESET "                                     " COLOR_GREEN "║\n" COLOR_RESET);
        printf(COLOR_GREEN "╚════════════════════════════════════════════════════════════╝\n" COLOR_RESET);
    } else {
        printf(COLOR_RED "╔════════════════════════════════════════════════════════════╗\n" COLOR_RESET);
        printf(COLOR_RED "║" COLOR_RESET "  " COLOR_RED "✗ SOME TESTS FAILED" COLOR_RESET "                                    " COLOR_RED "║\n" COLOR_RESET);
        printf(COLOR_RED "╚════════════════════════════════════════════════════════════╝\n" COLOR_RESET);
    }
    printf("\n");
}

int test_generate_forensic_report(const char* filename) {
    FILE* fp = fopen(filename, "w");
    if (fp == NULL) {
        fprintf(stderr, "ERROR: Cannot create forensic report: %s\n", filename);
        return -1;
    }
    
    // Header
    fprintf(fp, "# RAPPORT FORENSIQUE TESTS — LumVorax\n\n");
    fprintf(fp, "**Date** : %s\n", __DATE__);
    fprintf(fp, "**Heure** : %s\n", __TIME__);
    fprintf(fp, "**Framework** : LumVorax Test Framework v1.0.0\n\n");
    
    fprintf(fp, "---\n\n");
    
    // Résumé
    fprintf(fp, "## Résumé Exécution\n\n");
    fprintf(fp, "- **Total tests** : %u\n", g_test_context.total_tests);
    fprintf(fp, "- **Tests réussis** : %u\n", g_test_context.passed_tests);
    fprintf(fp, "- **Tests échoués** : %u\n", g_test_context.failed_tests);
    fprintf(fp, "- **Tests skippés** : %u\n", g_test_context.skipped_tests);
    fprintf(fp, "- **Durée totale** : %.2f ms\n", g_test_context.total_duration_ms);
    
    if (g_test_context.total_tests > 0) {
        double success_rate = (double)g_test_context.passed_tests / 
                             (double)g_test_context.total_tests * 100.0;
        fprintf(fp, "- **Taux succès** : %.1f%%\n", success_rate);
    }
    
    fprintf(fp, "\n---\n\n");
    
    // Détails tests
    fprintf(fp, "## Détails Tests\n\n");
    
    for (uint32_t i = 0; i < g_test_context.total_tests; i++) {
        test_info_t* test = &g_test_context.tests[i];
        
        fprintf(fp, "### Test %u : %s\n\n", i + 1, test->name);
        
        if (test->result == TEST_PASS) {
            fprintf(fp, "- **Résultat** : ✅ PASS\n");
        } else if (test->result == TEST_FAIL) {
            fprintf(fp, "- **Résultat** : ❌ FAIL\n");
            fprintf(fp, "- **Fichier** : %s:%d\n", test->file, test->line);
            fprintf(fp, "- **Message** : %s\n", test->message);
        } else {
            fprintf(fp, "- **Résultat** : ⏭️ SKIP\n");
            fprintf(fp, "- **Raison** : %s\n", test->message);
        }
        
        fprintf(fp, "- **Durée** : %.2f ms\n", test->duration_ms);
        fprintf(fp, "\n");
    }
    
    fprintf(fp, "---\n\n");
    
    // Verdict
    fprintf(fp, "## Verdict Final\n\n");
    
    if (g_test_context.failed_tests == 0) {
        fprintf(fp, "✅ **TOUS LES TESTS SONT PASSÉS**\n\n");
        fprintf(fp, "Le module est validé et prêt pour intégration.\n");
    } else {
        fprintf(fp, "❌ **CERTAINS TESTS ONT ÉCHOUÉ**\n\n");
        fprintf(fp, "Le module nécessite des corrections avant intégration.\n\n");
        fprintf(fp, "### Tests à corriger :\n\n");
        
        for (uint32_t i = 0; i < g_test_context.total_tests; i++) {
            test_info_t* test = &g_test_context.tests[i];
            if (test->result == TEST_FAIL) {
                fprintf(fp, "- %s (%s:%d)\n", test->name, test->file, test->line);
            }
        }
    }
    
    fclose(fp);
    
    printf(COLOR_CYAN "Forensic report generated: %s\n" COLOR_RESET, filename);
    
    return 0;
}

void test_framework_cleanup(void) {
    memset(&g_test_context, 0, sizeof(test_context_t));
}

// Made with Bob
