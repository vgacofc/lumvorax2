#include "neural_ultra_precision_architecture.h"
// #include "neural_blackbox_computer.h" // DÉSACTIVÉ par prompt.txt
#include "../debug/memory_tracker.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

// Architecture neuronale selon précision requise
neural_architecture_config_t* neural_calculate_ultra_precision_architecture(
    size_t input_dim __attribute__((unused)),
    size_t output_dim __attribute__((unused)),
    size_t precision_digits
) {
    if (precision_digits == 0 || precision_digits > MAX_PRECISION_DIGITS) {
        return NULL;
    }

    neural_architecture_config_t* config = TRACKED_MALLOC(sizeof(neural_architecture_config_t));
    if (!config) return NULL;

    // Configuration adaptée à la précision
    config->complexity_target = (precision_digits > 15) ? NEURAL_COMPLEXITY_EXTREME : NEURAL_COMPLEXITY_HIGH;
    config->memory_capacity = precision_digits * 1048576; // 1MB par digit de précision
    config->learning_rate = 1.0 / (precision_digits * 1000.0); // LR inversement proportionnel
    config->plasticity_rules = (void*)0; // PLASTICITY_HOMEOSTATIC pour stabilité
    config->enable_continuous_learning = false; // Pas d'adaptation pendant ultra-précision
    config->enable_metaplasticity = true; // Meta-adaptation pour convergence

    return config;
}

// Calcul profondeur optimale selon complexité (version ultra_precision - renommée pour éviter conflit)
__attribute__((unused)) static size_t neural_calculate_optimal_depth_ultra(
    size_t input_dim,
    size_t output_dim,
    neural_complexity_target_e complexity
) {
    size_t base_depth = 3;
    size_t dim_factor = (input_dim + output_dim) / 10;

    switch (complexity) {
        case NEURAL_COMPLEXITY_LOW:
            return base_depth + dim_factor / 4;
        case NEURAL_COMPLEXITY_MEDIUM:
            return base_depth + dim_factor / 2;
        case NEURAL_COMPLEXITY_HIGH:
            return base_depth + dim_factor;
        case NEURAL_COMPLEXITY_EXTREME:
            return base_depth + dim_factor * 2;
        default:
            return base_depth;
    }
}

// Calcul largeur optimale selon dimensions (version ultra_precision - renommée pour éviter conflit)
__attribute__((unused)) static size_t neural_calculate_optimal_width_ultra(
    size_t input_dim,
    size_t output_dim,
    size_t depth
) {
    size_t base_width = (input_dim + output_dim) * 2;
    size_t depth_scaling = depth * 50; // 50 neurones par couche supplémentaire
    return base_width + depth_scaling;
}

// Fonctions d'activation ultra-précises
double activation_ultra_precise_tanh(double x) {
    if (x > 20.0) return 1.0;
    if (x < -20.0) return -1.0;

    double exp_2x = exp(2.0 * x);
    return (exp_2x - 1.0) / (exp_2x + 1.0);
}

double activation_ultra_precise_sigmoid(double x) {
    if (x > 20.0) return 1.0;
    if (x < -20.0) return 0.0;

    return 1.0 / (1.0 + exp(-x));
}

double activation_ultra_precise_piecewise(double x) {
    if (x <= -1.0) return 0.0;
    if (x >= 1.0) return 1.0;
    return 0.5 * (x + 1.0); // Linear between -1 and 1
}

// Configuration ultra-précise
neural_ultra_precision_config_t* neural_ultra_precision_config_create(
    size_t precision_digits,
    size_t input_dims,
    size_t output_dims
) {
    if (precision_digits == 0 || precision_digits > MAX_PRECISION_DIGITS) {
        return NULL;
    }

    neural_ultra_precision_config_t* config = TRACKED_MALLOC(
        sizeof(neural_ultra_precision_config_t));
    if (!config) return NULL;

    // Initialisation avec valeurs par défaut
    config->precision_target_digits = precision_digits;
    config->precision_target = 1.0 / pow(10.0, (double)precision_digits);
    config->base_depth = precision_digits / 5 + 5;  // Profondeur adaptative
    config->precision_layers = DEFAULT_PRECISION_LAYERS;
    config->neurons_per_precision_digit = DEFAULT_NEURONS_PER_DIGIT;
    config->input_dimensions = input_dims;
    config->output_dimensions = output_dims;
    config->memory_scaling_factor = 1.0 + (double)precision_digits * 0.1;
    config->computation_scaling_factor = 1.0 + (double)precision_digits * 0.05;
    config->enable_adaptive_precision = true;
    config->enable_error_correction = true;
    config->magic_number = NEURAL_ULTRA_PRECISION_MAGIC;

    return config;
}

void neural_ultra_precision_config_destroy(neural_ultra_precision_config_t* config) {
    if (!config) return;

    // Vérification magic number
    if (config->magic_number != NEURAL_ULTRA_PRECISION_MAGIC) {
        printf("[MEMORY_TRACKER] WARNING: Invalid magic number in ultra precision config\n");
        return;
    }

    config->magic_number = 0;  // Invalidation
    TRACKED_FREE(config);
}

bool neural_ultra_precision_config_validate(const neural_ultra_precision_config_t* config) {
    if (!config) return false;
    if (config->magic_number != NEURAL_ULTRA_PRECISION_MAGIC) return false;
    if (config->precision_target_digits == 0) return false;
    if (config->precision_target_digits > MAX_PRECISION_DIGITS) return false;
    if (config->input_dimensions == 0) return false;
    if (config->output_dimensions == 0) return false;

    return true;
}

// Validation architecture ultra-précise
bool neural_validate_ultra_precision_architecture(
    neural_architecture_config_t* config,
    size_t precision_target_digits
) {
    if (!config || precision_target_digits == 0) return false;

    // Vérifier que la complexité est suffisante pour la précision demandée
    if (precision_target_digits > 10 && config->complexity_target < NEURAL_COMPLEXITY_HIGH) {
        return false;
    }

    // Vérifier capacité mémoire suffisante
    size_t min_memory = precision_target_digits * 512 * 1024; // 512KB par digit minimum
    if (config->memory_capacity < min_memory) {
        return false;
    }

    return true;
}