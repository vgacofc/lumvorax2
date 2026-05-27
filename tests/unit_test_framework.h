/**
 * @file unit_test_framework.h
 * @brief Framework de tests unitaires complet pour LumVorax
 * @version 1.0
 * @date 2026-04-30
 * @cycle C147
 * 
 * Framework de tests avec:
 * - Assertions complètes (ASSERT_EQ, ASSERT_NE, ASSERT_TRUE, etc.)
 * - Fixtures pour setup/teardown
 * - Mocks pour dépendances externes
 * - Rapports détaillés avec métriques
 * - Support multi-thread
 * - Couverture de code
 */

#ifndef UNIT_TEST_FRAMEWORK_H
#define UNIT_TEST_FRAMEWORK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * CONSTANTES ET MACROS
 * ============================================================================ */

#define TEST_FRAMEWORK_VERSION "1.0.0"
#define TEST_FRAMEWORK_MAGIC 0x54455354  // 'TEST'
#define MAX_TEST_NAME_LEN 128
#define MAX_TEST_SUITES 100
#define MAX_TESTS_PER_SUITE 1000
#define MAX_FIXTURES 50

/* Codes de retour */
#define TEST_SUCCESS 0
#define TEST_FAILURE 1
#define TEST_SKIPPED 2
#define TEST_ERROR 3

/* Couleurs pour output terminal */
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_BOLD    "\033[1m"

/* ============================================================================
 * STRUCTURES DE DONNÉES
 * ============================================================================ */

/**
 * @brief Résultat d'un test unitaire
 */
typedef struct {
    char name[MAX_TEST_NAME_LEN];
    int result;                    // TEST_SUCCESS, TEST_FAILURE, etc.
    uint64_t duration_ns;          // Durée d'exécution en nanosecondes
    char failure_message[256];     // Message d'erreur si échec
    const char* file;              // Fichier source du test
    int line;                      // Ligne du test
    uint32_t magic;                // Protection intégrité
} test_result_t;

/**
 * @brief Statistiques d'une suite de tests
 */
typedef struct {
    uint32_t total_tests;
    uint32_t passed_tests;
    uint32_t failed_tests;
    uint32_t skipped_tests;
    uint32_t error_tests;
    uint64_t total_duration_ns;
    double pass_rate;              // Pourcentage de réussite
} test_stats_t;

/**
 * @brief Fixture pour setup/teardown
 */
typedef struct {
    void* (*setup)(void);          // Fonction de setup (retourne contexte)
    void (*teardown)(void*);       // Fonction de teardown (reçoit contexte)
    void* context;                 // Contexte partagé entre tests
    char name[MAX_TEST_NAME_LEN];
    uint32_t magic;
} test_fixture_t;

/**
 * @brief Fonction de test unitaire
 */
typedef int (*test_func_t)(void* context);

/**
 * @brief Définition d'un test
 */
typedef struct {
    char name[MAX_TEST_NAME_LEN];
    test_func_t func;
    test_fixture_t* fixture;       // Fixture optionnelle
    bool enabled;                  // Test activé/désactivé
    uint32_t magic;
} test_case_t;

/**
 * @brief Suite de tests
 */
typedef struct {
    char name[MAX_TEST_NAME_LEN];
    test_case_t tests[MAX_TESTS_PER_SUITE];
    uint32_t test_count;
    test_stats_t stats;
    pthread_mutex_t mutex;         // Thread-safe
    uint32_t magic;
} test_suite_t;

/**
 * @brief Contexte global du framework de tests
 */
typedef struct {
    test_suite_t suites[MAX_TEST_SUITES];
    uint32_t suite_count;
    test_fixture_t fixtures[MAX_FIXTURES];
    uint32_t fixture_count;
    bool verbose;                  // Mode verbeux
    bool color_output;             // Sortie colorée
    FILE* report_file;             // Fichier de rapport
    pthread_mutex_t global_mutex;
    uint32_t magic;
} test_framework_t;

/* ============================================================================
 * API PUBLIQUE - GESTION DU FRAMEWORK
 * ============================================================================ */

/**
 * @brief Initialise le framework de tests
 * @param verbose Mode verbeux (true/false)
 * @param color_output Sortie colorée (true/false)
 * @return Contexte du framework ou NULL si erreur
 */
test_framework_t* test_framework_init(bool verbose, bool color_output);

/**
 * @brief Détruit le framework de tests
 * @param framework Contexte du framework
 */
void test_framework_destroy(test_framework_t* framework);

/**
 * @brief Crée une nouvelle suite de tests
 * @param framework Contexte du framework
 * @param suite_name Nom de la suite
 * @return Pointeur vers la suite créée ou NULL si erreur
 */
test_suite_t* test_suite_create(test_framework_t* framework, const char* suite_name);

/**
 * @brief Ajoute un test à une suite
 * @param suite Suite de tests
 * @param test_name Nom du test
 * @param test_func Fonction de test
 * @param fixture Fixture optionnelle (peut être NULL)
 * @return 0 si succès, -1 si erreur
 */
int test_suite_add_test(test_suite_t* suite, const char* test_name, 
                        test_func_t test_func, test_fixture_t* fixture);

/**
 * @brief Exécute tous les tests d'une suite
 * @param suite Suite de tests
 * @return Statistiques d'exécution
 */
test_stats_t test_suite_run(test_suite_t* suite);

/**
 * @brief Exécute toutes les suites du framework
 * @param framework Contexte du framework
 * @return Statistiques globales
 */
test_stats_t test_framework_run_all(test_framework_t* framework);

/* ============================================================================
 * API PUBLIQUE - FIXTURES
 * ============================================================================ */

/**
 * @brief Crée une fixture
 * @param framework Contexte du framework
 * @param name Nom de la fixture
 * @param setup Fonction de setup
 * @param teardown Fonction de teardown
 * @return Pointeur vers la fixture créée ou NULL si erreur
 */
test_fixture_t* test_fixture_create(test_framework_t* framework, const char* name,
                                    void* (*setup)(void), void (*teardown)(void*));

/* ============================================================================
 * API PUBLIQUE - ASSERTIONS
 * ============================================================================ */

/**
 * @brief Assertion d'égalité entière
 */
#define ASSERT_EQ(actual, expected) \
    do { \
        if ((actual) != (expected)) { \
            fprintf(stderr, "%s[FAIL]%s %s:%d: Expected %ld, got %ld\n", \
                    COLOR_RED, COLOR_RESET, __FILE__, __LINE__, \
                    (long)(expected), (long)(actual)); \
            return TEST_FAILURE; \
        } \
    } while(0)

/**
 * @brief Assertion de non-égalité
 */
#define ASSERT_NE(actual, expected) \
    do { \
        if ((actual) == (expected)) { \
            fprintf(stderr, "%s[FAIL]%s %s:%d: Values should not be equal: %ld\n", \
                    COLOR_RED, COLOR_RESET, __FILE__, __LINE__, (long)(actual)); \
            return TEST_FAILURE; \
        } \
    } while(0)

/**
 * @brief Assertion de vérité
 */
#define ASSERT_TRUE(condition) \
    do { \
        if (!(condition)) { \
            fprintf(stderr, "%s[FAIL]%s %s:%d: Condition is false: %s\n", \
                    COLOR_RED, COLOR_RESET, __FILE__, __LINE__, #condition); \
            return TEST_FAILURE; \
        } \
    } while(0)

/**
 * @brief Assertion de fausseté
 */
#define ASSERT_FALSE(condition) \
    do { \
        if (condition) { \
            fprintf(stderr, "%s[FAIL]%s %s:%d: Condition is true: %s\n", \
                    COLOR_RED, COLOR_RESET, __FILE__, __LINE__, #condition); \
            return TEST_FAILURE; \
        } \
    } while(0)

/**
 * @brief Assertion de pointeur NULL
 */
#define ASSERT_NULL(ptr) \
    do { \
        if ((ptr) != NULL) { \
            fprintf(stderr, "%s[FAIL]%s %s:%d: Pointer should be NULL\n", \
                    COLOR_RED, COLOR_RESET, __FILE__, __LINE__); \
            return TEST_FAILURE; \
        } \
    } while(0)

/**
 * @brief Assertion de pointeur non-NULL
 */
#define ASSERT_NOT_NULL(ptr) \
    do { \
        if ((ptr) == NULL) { \
            fprintf(stderr, "%s[FAIL]%s %s:%d: Pointer should not be NULL\n", \
                    COLOR_RED, COLOR_RESET, __FILE__, __LINE__); \
            return TEST_FAILURE; \
        } \
    } while(0)

/**
 * @brief Assertion d'égalité de chaînes
 */
#define ASSERT_STR_EQ(actual, expected) \
    do { \
        if (strcmp((actual), (expected)) != 0) { \
            fprintf(stderr, "%s[FAIL]%s %s:%d: Expected \"%s\", got \"%s\"\n", \
                    COLOR_RED, COLOR_RESET, __FILE__, __LINE__, \
                    (expected), (actual)); \
            return TEST_FAILURE; \
        } \
    } while(0)

/**
 * @brief Assertion d'égalité de doubles (avec epsilon)
 */
#define ASSERT_DOUBLE_EQ(actual, expected, epsilon) \
    do { \
        double _diff = (actual) - (expected); \
        if (_diff < 0) _diff = -_diff; \
        if (_diff > (epsilon)) { \
            fprintf(stderr, "%s[FAIL]%s %s:%d: Expected %f, got %f (diff: %f > %f)\n", \
                    COLOR_RED, COLOR_RESET, __FILE__, __LINE__, \
                    (expected), (actual), _diff, (epsilon)); \
            return TEST_FAILURE; \
        } \
    } while(0)

/* ============================================================================
 * API PUBLIQUE - RAPPORTS
 * ============================================================================ */

/**
 * @brief Génère un rapport de tests au format texte
 * @param framework Contexte du framework
 * @param output_file Fichier de sortie (NULL pour stdout)
 */
void test_framework_generate_report(test_framework_t* framework, const char* output_file);

/**
 * @brief Génère un rapport de tests au format JSON
 * @param framework Contexte du framework
 * @param output_file Fichier de sortie
 */
void test_framework_generate_json_report(test_framework_t* framework, const char* output_file);

/**
 * @brief Affiche les statistiques globales
 * @param stats Statistiques à afficher
 */
void test_stats_print(const test_stats_t* stats);

#ifdef __cplusplus
}
#endif

#endif /* UNIT_TEST_FRAMEWORK_H */

// Made with Bob
