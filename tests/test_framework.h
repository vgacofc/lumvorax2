/**
 * @file test_framework.h
 * @brief Framework Tests Unitaires LumVorax
 * @author LumVorax Team
 * @date 2026-04-30
 * @version 1.0.0
 * 
 * Framework léger pour tests unitaires avec :
 * - Macros assertions
 * - Reporting automatique
 * - Coverage tracking
 * - Forensic logging
 * 
 * Conformité : STANDARD_NAMES.md v4.2
 */

#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================
 * CONSTANTES
 * ======================================================================== */

#define TEST_MAX_NAME_LEN 128
#define TEST_MAX_MESSAGE_LEN 256
#define TEST_MAX_TESTS 1000

/* Couleurs ANSI pour output */
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN    "\033[36m"

/* ========================================================================
 * TYPES ET STRUCTURES
 * ======================================================================== */

/**
 * @brief Résultat test
 */
typedef enum {
    TEST_PASS = 0,
    TEST_FAIL = 1,
    TEST_SKIP = 2
} test_result_t;

/**
 * @brief Information test
 */
typedef struct {
    char name[TEST_MAX_NAME_LEN];
    test_result_t result;
    char message[TEST_MAX_MESSAGE_LEN];
    double duration_ms;
    const char* file;
    int line;
} test_info_t;

/**
 * @brief Contexte tests
 */
typedef struct {
    test_info_t tests[TEST_MAX_TESTS];
    uint32_t total_tests;
    uint32_t passed_tests;
    uint32_t failed_tests;
    uint32_t skipped_tests;
    double total_duration_ms;
    
    /* Test en cours */
    test_info_t* current_test;
    struct timespec start_time;
} test_context_t;

/* ========================================================================
 * VARIABLES GLOBALES
 * ======================================================================== */

extern test_context_t g_test_context;

/* ========================================================================
 * MACROS ASSERTIONS
 * ======================================================================== */

/**
 * @brief Assert égalité
 */
#define ASSERT_EQ(actual, expected, msg) \
    do { \
        if ((actual) != (expected)) { \
            test_fail(__FILE__, __LINE__, \
                "ASSERT_EQ failed: %s\n  Expected: %ld\n  Actual: %ld\n  Message: %s", \
                #actual " == " #expected, (long)(expected), (long)(actual), msg); \
            return; \
        } \
    } while(0)

/**
 * @brief Assert non égalité
 */
#define ASSERT_NE(actual, expected, msg) \
    do { \
        if ((actual) == (expected)) { \
            test_fail(__FILE__, __LINE__, \
                "ASSERT_NE failed: %s\n  Not expected: %ld\n  Message: %s", \
                #actual " != " #expected, (long)(expected), msg); \
            return; \
        } \
    } while(0)

/**
 * @brief Assert supérieur
 */
#define ASSERT_GT(actual, expected, msg) \
    do { \
        if ((actual) <= (expected)) { \
            test_fail(__FILE__, __LINE__, \
                "ASSERT_GT failed: %s\n  Expected > %ld\n  Actual: %ld\n  Message: %s", \
                #actual " > " #expected, (long)(expected), (long)(actual), msg); \
            return; \
        } \
    } while(0)

/**
 * @brief Assert inférieur
 */
#define ASSERT_LT(actual, expected, msg) \
    do { \
        if ((actual) >= (expected)) { \
            test_fail(__FILE__, __LINE__, \
                "ASSERT_LT failed: %s\n  Expected < %ld\n  Actual: %ld\n  Message: %s", \
                #actual " < " #expected, (long)(expected), (long)(actual), msg); \
            return; \
        } \
    } while(0)

/**
 * @brief Assert vrai
 */
#define ASSERT_TRUE(condition, msg) \
    do { \
        if (!(condition)) { \
            test_fail(__FILE__, __LINE__, \
                "ASSERT_TRUE failed: %s\n  Message: %s", \
                #condition, msg); \
            return; \
        } \
    } while(0)

/**
 * @brief Assert faux
 */
#define ASSERT_FALSE(condition, msg) \
    do { \
        if (condition) { \
            test_fail(__FILE__, __LINE__, \
                "ASSERT_FALSE failed: %s\n  Message: %s", \
                #condition, msg); \
            return; \
        } \
    } while(0)

/**
 * @brief Assert NULL
 */
#define ASSERT_NULL(ptr, msg) \
    do { \
        if ((ptr) != NULL) { \
            test_fail(__FILE__, __LINE__, \
                "ASSERT_NULL failed: %s is not NULL\n  Message: %s", \
                #ptr, msg); \
            return; \
        } \
    } while(0)

/**
 * @brief Assert NOT NULL
 */
#define ASSERT_NOT_NULL(ptr, msg) \
    do { \
        if ((ptr) == NULL) { \
            test_fail(__FILE__, __LINE__, \
                "ASSERT_NOT_NULL failed: %s is NULL\n  Message: %s", \
                #ptr, msg); \
            return; \
        } \
    } while(0)

/**
 * @brief Assert chaînes égales
 */
#define ASSERT_STR_EQ(actual, expected, msg) \
    do { \
        if (strcmp((actual), (expected)) != 0) { \
            test_fail(__FILE__, __LINE__, \
                "ASSERT_STR_EQ failed:\n  Expected: \"%s\"\n  Actual: \"%s\"\n  Message: %s", \
                (expected), (actual), msg); \
            return; \
        } \
    } while(0)

/* ========================================================================
 * MACROS EXÉCUTION TESTS
 * ======================================================================== */

/**
 * @brief Exécute un test
 */
#define RUN_TEST(test_func) \
    do { \
        test_start(#test_func); \
        test_func(); \
        test_end(); \
    } while(0)

/**
 * @brief Skip un test
 */
#define SKIP_TEST(test_func, reason) \
    do { \
        test_skip(#test_func, reason); \
    } while(0)

/**
 * @brief Affiche résultats
 */
#define PRINT_TEST_RESULTS() \
    test_print_results()

/* ========================================================================
 * API PUBLIQUE
 * ======================================================================== */

/**
 * @brief Initialise le framework tests
 */
void test_framework_init(void);

/**
 * @brief Démarre un test
 * @param name Nom du test
 */
void test_start(const char* name);

/**
 * @brief Termine un test (succès)
 */
void test_end(void);

/**
 * @brief Marque un test comme échoué
 * @param file Fichier source
 * @param line Ligne
 * @param format Format message (printf-style)
 */
void test_fail(const char* file, int line, const char* format, ...);

/**
 * @brief Skip un test
 * @param name Nom du test
 * @param reason Raison du skip
 */
void test_skip(const char* name, const char* reason);

/**
 * @brief Affiche les résultats
 */
void test_print_results(void);

/**
 * @brief Génère rapport forensique
 * @param filename Nom fichier rapport
 * @return 0 si succès, -1 si erreur
 */
int test_generate_forensic_report(const char* filename);

/**
 * @brief Nettoie le framework
 */
void test_framework_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif /* TEST_FRAMEWORK_H */

// Made with Bob
