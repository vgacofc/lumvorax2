#ifndef NEURAL_BLACKBOX_ULTRA_PRECISION_TESTS_H
#define NEURAL_BLACKBOX_ULTRA_PRECISION_TESTS_H

#include <stdbool.h>
#include <stdint.h>
#include "neural_blackbox_computer.h"

#include "neural_ultra_precision_architecture.h"
// Configuration ultra-précision compatible - utiliser directement le type défini dans neural_ultra_precision_architecture.h
#include "neural_advanced_optimizers.h"

// ============================================================================
// MODULE DE TESTS ULTRA-PRÉCISION (Phase 4)
// Objectif: Valider précision 100% (erreur < 1e-15)
// ============================================================================

// === CONSTANTES DE TEST ===
#define ULTRA_PRECISION_TARGET 1e-15
#define ULTRA_PRECISION_TOLERANCE 1e-16
#define TEST_MAX_EPOCHS 10000
#define TEST_SAMPLE_COUNT 1000

// === TYPES DE TESTS ===
typedef enum {
    TEST_POLYNOMIAL_SIMPLE,
    TEST_POLYNOMIAL_COMPLEXE,
    TEST_TRIGONOMETRIC,
    TEST_EXPONENTIAL,
    TEST_LOGARITHMIC,
    TEST_CUSTOM_MATHEMATICAL,
    TEST_COUNT
} ultra_precision_test_type_e;

typedef struct {
    const char* name;
    ultra_precision_test_type_e type;
    size_t input_dims;
    size_t output_dims;
    double precision_target;
    bool (*generate_data)(double** inputs, double** outputs, size_t count);
    bool (*validate_result)(double* expected, double* actual, size_t dims);
} ultra_precision_test_spec_t;

typedef struct {
    char test_name[128];
    ultra_precision_test_type_e type;
    bool success;
    double final_error;
    double computation_time;
    size_t epochs_required;
    double memory_usage_mb;
} ultra_precision_test_result_t;

// === FONCTIONS DE TEST PRINCIPALES ===

// Exécution complète de tous les tests ultra-précision
bool neural_blackbox_run_all_ultra_precision_tests(void);

// Test spécifique d'une fonction mathématique
bool neural_blackbox_test_specific_function(
    ultra_precision_test_spec_t* spec,
    ultra_precision_test_result_t* result
);

// Tests de régression pour maintenir la précision
bool neural_blackbox_regression_tests_ultra_precision(void);

// === TESTS INDIVIDUELS ===

// Test fonction polynomiale simple: f(x) = x² + 2x + 1
bool test_polynomial_simple(ultra_precision_test_result_t* result);

// Test fonction polynomiale complexe: f(x,y) = x³y² - 2x²y + xy³ - 3
bool test_polynomial_complexe(ultra_precision_test_result_t* result);

// Test fonction trigonométrique: f(x) = sin(x) * cos(x) + tan(x/2)
bool test_trigonometric(ultra_precision_test_result_t* result);

// Test fonction exponentielle: f(x) = e^(x²) - log(x+1)
bool test_exponential(ultra_precision_test_result_t* result);

// Test fonction logarithmique: f(x,y) = ln(x²+y²) * log₁₀(xy)
bool test_logarithmic(ultra_precision_test_result_t* result);

// Test fonction personnalisée ultra-complexe
bool test_custom_mathematical(ultra_precision_test_result_t* result);

// === TESTS DE PERFORMANCE ===

// Test vitesse de convergence des optimiseurs
bool test_optimizer_convergence_speed(ultra_precision_test_result_t* result);

// Test stabilité numérique des calculs extended precision
bool test_numerical_stability(ultra_precision_test_result_t* result);

// Test mémoire et fuites pendant entraînement ultra-précis
bool test_memory_integrity_ultra_precision(ultra_precision_test_result_t* result);

// === TESTS DE VALIDATION CROISÉE ===

// Validation croisée k-fold ultra-précise
bool test_k_fold_cross_validation_ultra_precise(
    size_t k_folds,
    ultra_precision_test_result_t* result
);

// Test robustesse face aux perturbations numériques
bool test_numerical_robustness(ultra_precision_test_result_t* result);

// Test consistance entre différents optimiseurs
bool test_optimizer_consistency(ultra_precision_test_result_t* result);

// === UTILITAIRES DE TEST ===

// Génération de données de test pour fonction spécifiée
bool generate_test_data_for_function(
    ultra_precision_test_type_e type,
    double*** inputs,
    double*** outputs,
    size_t* input_dims,
    size_t* output_dims,
    size_t sample_count
);

// Validation erreur ultra-précise
bool validate_ultra_precision_error(
    double* expected,
    double* actual,
    size_t dims,
    double tolerance
);

// Calcul statistiques de test
void compute_test_statistics(
    ultra_precision_test_result_t* results,
    size_t result_count,
    double* mean_error,
    double* max_error,
    double* success_rate
);

// Affichage rapport de test détaillé
void print_ultra_precision_test_report(
    ultra_precision_test_result_t* results,
    size_t result_count
);

// === TESTS DE STRESS ===

// Test stress avec millions de paramètres
bool test_ultra_precision_stress_million_params(ultra_precision_test_result_t* result);

// Test endurance sur longue durée
bool test_ultra_precision_endurance(ultra_precision_test_result_t* result);

// Test limite mémoire
bool test_ultra_precision_memory_limits(ultra_precision_test_result_t* result);

#endif // NEURAL_BLACKBOX_ULTRA_PRECISION_TESTS_H