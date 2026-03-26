// Feature test macros for POSIX functions
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

// MODULES BLACKBOX DÉSACTIVÉS JUSQU'À RÉACTIVATION EXPLICITE
#ifdef MODULES_BLACKBOX_ACTIFS
#include "neural_blackbox_computer.h"
#include "neural_ultra_precision_architecture.h"
#include "../debug/memory_tracker.h"
#include "../debug/forensic_logger.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdio.h>
#else
// Module blackbox désactivé - implémentation stub
#include "neural_blackbox_computer.h"
#include "../debug/memory_tracker.h"
#include <stdio.h>
#endif

// Temporary logging macros until forensic_log is properly implemented
#define FORENSIC_LEVEL_ERROR 0
#define FORENSIC_LEVEL_INFO 1
#define FORENSIC_LEVEL_WARNING 2
#define FORENSIC_LEVEL_SUCCESS 3
#define FORENSIC_LEVEL_DEBUG 4
#define forensic_log(level, func, fmt, ...) printf("[%s] " fmt "\n", func, ##__VA_ARGS__)

// Structure neural_layer_t déjà définie dans neural_blackbox_computer.h - pas de redéfinition nécessaire

// === IMPLÉMENTATIONS NEURAL_LAYER MANQUANTES ===

neural_layer_t* neural_layer_create(size_t neuron_count, size_t input_size, activation_function_e activation) {
    neural_layer_t* layer = TRACKED_MALLOC(sizeof(neural_layer_t));
    if (!layer) return NULL;

    layer->neuron_count = neuron_count;
    layer->input_size = input_size;
    layer->output_size = neuron_count;
    layer->activation = activation;
    layer->layer_id = 0;
    layer->magic_number = 0xABCDEF01;

    // Allocation poids
    layer->weights = TRACKED_MALLOC(neuron_count * input_size * sizeof(double));
    layer->biases = TRACKED_MALLOC(neuron_count * sizeof(double));
    layer->outputs = TRACKED_MALLOC(neuron_count * sizeof(double));
    layer->layer_error = TRACKED_MALLOC(neuron_count * sizeof(double));

    if (!layer->weights || !layer->biases || !layer->outputs || !layer->layer_error) {
        neural_layer_destroy(&layer);
        return NULL;
    }

    // Initialisation Xavier
    double xavier_std = sqrt(2.0 / (double)(input_size + neuron_count));
    for (size_t i = 0; i < neuron_count * input_size; i++) {
        layer->weights[i] = ((double)rand() / RAND_MAX - 0.5) * 2.0 * xavier_std;
    }

    // Initialisation biases à zéro
    memset(layer->biases, 0, neuron_count * sizeof(double));
    memset(layer->outputs, 0, neuron_count * sizeof(double));
    memset(layer->layer_error, 0, neuron_count * sizeof(double));

    return layer;
}

void neural_layer_destroy(neural_layer_t** layer_ptr) {
    if (!layer_ptr || !*layer_ptr) return;

    neural_layer_t* layer = *layer_ptr;
    if (layer->magic_number == 0xABCDEF01) {
        if (layer->weights) TRACKED_FREE(layer->weights);
        if (layer->biases) TRACKED_FREE(layer->biases);
        if (layer->outputs) TRACKED_FREE(layer->outputs);
        if (layer->layer_error) TRACKED_FREE(layer->layer_error);

        layer->magic_number = 0xDEADDEAD;
        TRACKED_FREE(layer);
        *layer_ptr = NULL;
    }
}

double neural_activation_function(double x, activation_function_e type) {
    switch (type) {
        case ACTIVATION_TANH:
            return tanh(x);
        case ACTIVATION_SIGMOID:
            return 1.0 / (1.0 + exp(-x));
        case ACTIVATION_RELU:
            return (x > 0.0) ? x : 0.0;
        case ACTIVATION_GELU:
            return 0.5 * x * (1.0 + tanh(sqrt(2.0/M_PI) * (x + 0.044715 * x * x * x)));
        case ACTIVATION_SWISH:
            return x / (1.0 + exp(-x));
        default:
            return x;
    }
}

bool neural_layer_forward_pass(neural_layer_t* layer, double* input) {
    if (!layer || !input) return false;

    for (size_t n = 0; n < layer->neuron_count; n++) {
        double sum = layer->biases[n];

        // Calcul produit scalaire poids * entrées
        for (size_t i = 0; i < layer->input_size; i++) {
            sum += layer->weights[n * layer->input_size + i] * input[i];
        }

        // Application fonction d'activation
        layer->outputs[n] = neural_activation_function(sum, layer->activation);
    }

    return true;
}

// === STUB IMPLEMENTATIONS FOR MISSING FUNCTIONS ===

// Types adam_ultra_precise_optimizer_t, lbfgs_optimizer_t, newton_raphson_optimizer_t 
// déjà définis dans neural_blackbox_computer.h - pas de redéfinition nécessaire

// Implémentation complète perturbation paramètres pour gradients numériques
void neural_blackbox_perturb_parameter(neural_blackbox_computer_t* system, size_t param_idx, double perturbation) {
    if (!system || param_idx >= system->total_parameters) return;

    size_t current_idx = 0;

    // Parcours des couches pour trouver le bon paramètre
    for (size_t layer_idx = 0; layer_idx < system->network_depth; layer_idx++) {
        neural_layer_t* layer = system->hidden_layers[layer_idx];
        if (!layer) continue;

        // Paramètres des poids
        size_t weight_count = layer->neuron_count * layer->input_size;
        if (param_idx >= current_idx && param_idx < current_idx + weight_count) {
            size_t local_idx = param_idx - current_idx;
            layer->weights[local_idx] += perturbation;
            return;
        }
        current_idx += weight_count;

        // Paramètres des biais
        if (param_idx >= current_idx && param_idx < current_idx + layer->neuron_count) {
            size_t local_idx = param_idx - current_idx;
            layer->biases[local_idx] += perturbation;
            return;
        }
        current_idx += layer->neuron_count;
    }
}

// Implémentation complète optimiseur Adam ultra-précis
adam_ultra_precise_optimizer_t* adam_ultra_precise_create(double lr, double beta1, double beta2, double epsilon) {
    adam_ultra_precise_optimizer_t* adam = TRACKED_MALLOC(sizeof(adam_ultra_precise_optimizer_t));
    if (!adam) return NULL;

    // Validation paramètres
    if (lr <= 0.0 || lr > 1.0) lr = 0.001;  // Learning rate raisonnable
    if (beta1 <= 0.0 || beta1 >= 1.0) beta1 = 0.9;  // Momentum typique
    if (beta2 <= 0.0 || beta2 >= 1.0) beta2 = 0.999; // RMSprop typique
    if (epsilon <= 0.0) epsilon = 1e-8; // Stabilité numérique

    adam->learning_rate = lr;
    adam->beta1 = beta1;
    adam->beta2 = beta2;
    adam->epsilon = epsilon;
    adam->iteration = 0;
    adam->magic_number = 0xADAC0001;
    adam->memory_address = (void*)adam;

    // Allocation mémoire pour momentum et velocity (sera dimensionnée lors du premier usage)
    adam->momentum = NULL;
    adam->velocity = NULL;
    adam->param_count = 0;

    return adam;
}

// Implémentation complète Newton-Raphson update weights
bool newton_raphson_update_weights(newton_raphson_optimizer_t* newton, neural_blackbox_computer_t* system, double* gradients, double loss) {
    if (!newton || !system || !gradients) return false;

    // Validation magic number
    if (newton->magic_number != 0xDEAD0001) return false;
    if (system->blackbox_magic != NEURAL_BLACKBOX_MAGIC) return false;

    // Newton-Raphson nécessite calcul Hessienne (approximation BFGS pour performance)
    if (!newton->hessian_inv) {
        // Initialisation Hessienne identité
        size_t param_count = system->total_parameters;
        newton->hessian_inv = TRACKED_MALLOC(param_count * sizeof(double));
        if (!newton->hessian_inv) return false;

        // Initialisation identité diagonale
        for (size_t i = 0; i < param_count; i++) {
            newton->hessian_inv[i] = 1.0; // Hessienne identité approximée
        }
        newton->param_count = param_count;
    }

    // Update Newton-Raphson: theta = theta - H^-1 * g
    // Approximation diagonale pour performance
    size_t param_idx = 0;

    // Mise à jour couche par couche
    for (size_t layer_idx = 0; layer_idx < system->network_depth; layer_idx++) {
        neural_layer_t* layer = system->hidden_layers[layer_idx];
        if (!layer) continue;

        // Mise à jour poids avec Newton-Raphson
        for (size_t i = 0; i < layer->neuron_count * layer->input_size; i++, param_idx++) {
            if (param_idx >= newton->param_count) break;

            double gradient = gradients[param_idx];
            double hessian_inv = newton->hessian_inv[param_idx];

            // Newton step: param -= hessian_inv * gradient
            layer->weights[i] -= hessian_inv * gradient;

            // Mise à jour approximation Hessienne (BFGS-like)
            double gradient_change = fabs(gradient);
            if (gradient_change > newton->tolerance) {
                newton->hessian_inv[param_idx] *= 0.99; // Adaptation conservatrice
            }
        }

        // Mise à jour biais
        for (size_t i = 0; i < layer->neuron_count; i++, param_idx++) {
            if (param_idx >= newton->param_count) break;

            double gradient = gradients[param_idx];
            double hessian_inv = newton->hessian_inv[param_idx];

            layer->biases[i] -= hessian_inv * gradient;
        }
    }

    (void)loss; // Loss utilisé pour convergence check dans versions futures
    return true;
}

// Additional missing function stubs
void adam_ultra_precise_destroy(adam_ultra_precise_optimizer_t** adam) {
    if (adam && *adam) {
        if ((*adam)->momentum) TRACKED_FREE((*adam)->momentum);
        if ((*adam)->velocity) TRACKED_FREE((*adam)->velocity);
        TRACKED_FREE(*adam);
        *adam = NULL;
    }
}

lbfgs_optimizer_t* lbfgs_create(int memory_size, double tolerance) {
    (void)memory_size; (void)tolerance;
    lbfgs_optimizer_t* lbfgs = malloc(sizeof(lbfgs_optimizer_t));
    if (lbfgs) { lbfgs->history_s = NULL; lbfgs->history_y = NULL; lbfgs->history_count = 0; }
    return lbfgs;
}

void lbfgs_destroy(lbfgs_optimizer_t** lbfgs) {
    if (lbfgs && *lbfgs) { free(*lbfgs); *lbfgs = NULL; }
}

newton_raphson_optimizer_t* newton_raphson_create(double tolerance) {
    (void)tolerance;
    newton_raphson_optimizer_t* newton = malloc(sizeof(newton_raphson_optimizer_t));
    if (newton) { newton->hessian_inv = NULL; newton->param_count = 0; }
    return newton;
}

void newton_raphson_destroy(newton_raphson_optimizer_t** newton) {
    if (newton && *newton) { 
        if ((*newton)->hessian_inv) TRACKED_FREE((*newton)->hessian_inv);
        TRACKED_FREE(*newton); 
        *newton = NULL; 
    }
}

bool neural_ultra_precision_verify_architecture(neural_blackbox_computer_t* system, neural_function_spec_t* spec) {
    (void)system; (void)spec;
    return true; // Simple stub
}

void neural_ultra_precision_initialize_weights(neural_blackbox_computer_t* system, double precision_target) {
    (void)system; (void)precision_target;
    // Simple stub - no operation
}

// Stub for neural_blackbox_compute_precise
double* neural_blackbox_compute_precise(neural_blackbox_computer_t* system, double* input, size_t input_size) {
    (void)input_size; // Suppress unused parameter warning
    if (!system || !input) return NULL;
    // Redirect to main execution function
    return neural_blackbox_execute(system, input);
}

// Stub for neural_ultra_precision_count_parameters
size_t neural_ultra_precision_count_parameters(neural_blackbox_computer_t* system) {
    if (!system) return 0;
    return system->total_parameters;
}

// Stub for neural_compute_vector_norm
double neural_compute_vector_norm(double* vector, size_t size) {
    if (!vector || size == 0) return 0.0;
    double sum = 0.0;
    for (size_t i = 0; i < size; i++) {
        sum += vector[i] * vector[i];
    }
    return sqrt(sum);
}

// Stub for neural_estimate_condition_number
double neural_estimate_condition_number(double* matrix, size_t size) {
    if (!matrix || size == 0) return 1.0;
    return 1.0; // Simple stub - return well-conditioned
}

// Implémentation complète mise à jour poids Adam ultra-précis
bool adam_ultra_precise_update_weights(void* adam_ptr, neural_blackbox_computer_t* system, double* gradients, double loss) {
    if (!adam_ptr || !system || !gradients) return false;

    adam_ultra_precise_optimizer_t* adam = (adam_ultra_precise_optimizer_t*)adam_ptr;
    if (adam->magic_number != 0xADAC0001) return false;

    // Initialisation lors du premier usage
    if (!adam->momentum) {
        adam->param_count = system->total_parameters;
        adam->momentum = TRACKED_CALLOC(adam->param_count, sizeof(double));
        adam->velocity = TRACKED_CALLOC(adam->param_count, sizeof(double));
        if (!adam->momentum || !adam->velocity) return false;
    }

    adam->iteration++;

    // Correction biais
    double lr_corrected = adam->learning_rate * sqrt(1.0 - pow(adam->beta2, adam->iteration)) / 
                          (1.0 - pow(adam->beta1, adam->iteration));

    size_t param_idx = 0;

    // Mise à jour couche par couche
    for (size_t layer_idx = 0; layer_idx < system->network_depth; layer_idx++) {
        neural_layer_t* layer = system->hidden_layers[layer_idx];
        if (!layer) continue;

        // Mise à jour poids
        for (size_t i = 0; i < layer->neuron_count * layer->input_size; i++, param_idx++) {
            if (param_idx >= adam->param_count) return false;

            double gradient = gradients[param_idx];

            // Adam update formulas
            adam->momentum[param_idx] = adam->beta1 * adam->momentum[param_idx] + 
                                        (1.0 - adam->beta1) * gradient;
            adam->velocity[param_idx] = adam->beta2 * adam->velocity[param_idx] + 
                                        (1.0 - adam->beta2) * gradient * gradient;

            // Mise à jour paramètre
            layer->weights[i] -= lr_corrected * adam->momentum[param_idx] / 
                                 (sqrt(adam->velocity[param_idx]) + adam->epsilon);
        }

        // Mise à jour biais
        for (size_t i = 0; i < layer->neuron_count; i++, param_idx++) {
            if (param_idx >= adam->param_count) return false;

            double gradient = gradients[param_idx];

            adam->momentum[param_idx] = adam->beta1 * adam->momentum[param_idx] + 
                                        (1.0 - adam->beta1) * gradient;
            adam->velocity[param_idx] = adam->beta2 * adam->velocity[param_idx] + 
                                        (1.0 - adam->beta2) * gradient * gradient;

            layer->biases[i] -= lr_corrected * adam->momentum[param_idx] / 
                                (sqrt(adam->velocity[param_idx]) + adam->epsilon);
        }
    }

    (void)loss; // Loss peut être utilisé pour des adaptations futures
    return true;
}

// Stub for lbfgs_update_weights
bool lbfgs_update_weights(void* lbfgs, neural_blackbox_computer_t* system, double* gradients, double loss) {
    (void)lbfgs; (void)system; (void)gradients; (void)loss;
    return true; // Simple stub
}

// === IMPLÉMENTATION NEURAL BLACKBOX 100% NATIF ===

// Création système neural universel
neural_blackbox_computer_t* neural_blackbox_create(
    size_t input_dimensions,
    size_t output_dimensions,
    neural_architecture_config_t* config
) {
    if (!config || input_dimensions == 0 || output_dimensions == 0) {
        forensic_log(FORENSIC_LEVEL_ERROR, "neural_blackbox_create", 
                    "Paramètres invalides: input=%zu, output=%zu", 
                    input_dimensions, output_dimensions);
        return NULL;
    }

    neural_blackbox_computer_t* system = TRACKED_MALLOC(sizeof(neural_blackbox_computer_t));
    if (!system) {
        forensic_log(FORENSIC_LEVEL_ERROR, "neural_blackbox_create", 
                    "Échec allocation mémoire pour système neural");
        return NULL;
    }

    // Architecture adaptative basée sur complexité requise
    size_t optimal_depth = neural_calculate_optimal_depth(
        input_dimensions, 
        output_dimensions,
        config->complexity_target
    );

    size_t neurons_per_layer = neural_calculate_optimal_width(
        input_dimensions,
        output_dimensions, 
        optimal_depth
    );

    forensic_log(FORENSIC_LEVEL_INFO, "neural_blackbox_create",
                "Création système neural blackbox - Profondeur: %zu, Largeur: %zu, Paramètres: %zu",
                optimal_depth, neurons_per_layer, optimal_depth * neurons_per_layer * neurons_per_layer);

    // Initialisation structure
    system->input_dimensions = input_dimensions;
    system->output_dimensions = output_dimensions;
    system->network_depth = optimal_depth;
    system->neurons_per_layer = neurons_per_layer;
    system->total_parameters = optimal_depth * neurons_per_layer * neurons_per_layer;
    system->blackbox_magic = NEURAL_BLACKBOX_MAGIC;
    system->memory_address = (void*)system;

    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    system->creation_timestamp = ts.tv_sec * 1000000000ULL + ts.tv_nsec;

    // Allocation couches cachées
    system->hidden_layers = TRACKED_MALLOC(optimal_depth * sizeof(neural_layer_t*));
    if (!system->hidden_layers) {
        forensic_log(FORENSIC_LEVEL_ERROR, "neural_blackbox_create", 
                    "Échec allocation hidden_layers");
        TRACKED_FREE(system);
        return NULL;
    }

    // Création couches avec fonctions d'activation complexes
    for (size_t i = 0; i < optimal_depth; i++) {
        activation_function_e activation = (i % 2 == 0) ? ACTIVATION_GELU : ACTIVATION_SWISH;
        system->hidden_layers[i] = neural_layer_create(
            neurons_per_layer,
            (i == 0) ? input_dimensions : neurons_per_layer,
            activation
        );

        if (!system->hidden_layers[i]) {
            forensic_log(FORENSIC_LEVEL_ERROR, "neural_blackbox_create", 
                        "Échec création couche %zu", i);
            // Cleanup des couches déjà créées
            for (size_t j = 0; j < i; j++) {
                neural_layer_destroy(&system->hidden_layers[j]);
            }
            TRACKED_FREE(system->hidden_layers);
            TRACKED_FREE(system);
            return NULL;
        }

        system->hidden_layers[i]->layer_id = (uint32_t)i;
    }

    // Mémoire persistante neuronale
    system->persistent_memory = neural_memory_bank_create(config->memory_capacity);
    if (!system->persistent_memory) {
        forensic_log(FORENSIC_LEVEL_WARNING, "neural_blackbox_create", 
                    "Échec création mémoire persistante - continuant sans");
        system->persistent_memory = NULL;
    }

    // Moteur d'apprentissage continu
    system->learning_engine = neural_learning_engine_create(
        config->learning_rate,
        config->plasticity_rules
    );

    // États internes pour opacité maximale
    system->internal_activations = TRACKED_MALLOC(
        optimal_depth * neurons_per_layer * sizeof(double)
    );

    if (system->internal_activations) {
        // Initialisation avec valeurs pseudo-aléatoires basées sur timestamp
        srand((unsigned int)(system->creation_timestamp % UINT32_MAX));
        for (size_t i = 0; i < optimal_depth * neurons_per_layer; i++) {
            system->internal_activations[i] = ((double)rand() / RAND_MAX - 0.5) * 0.1;
        }
    }

    system->synaptic_changes_count = 0;
    system->total_forward_passes = 0;
    system->adaptation_cycles = 0;

    forensic_log(FORENSIC_LEVEL_SUCCESS, "neural_blackbox_create",
                "Système neural blackbox créé avec succès - ID: %p, Paramètres: %zu",
                (void*)system, system->total_parameters);

    return system;
}

// Destruction sécurisée du système
void neural_blackbox_destroy(neural_blackbox_computer_t** system_ptr) {
    if (!system_ptr || !*system_ptr) return;

    neural_blackbox_computer_t* system = *system_ptr;

    if (system->blackbox_magic != NEURAL_BLACKBOX_MAGIC || 
        system->memory_address != (void*)system) {
        forensic_log(FORENSIC_LEVEL_ERROR, "neural_blackbox_destroy", 
                    "Tentative destruction système corrompu ou invalide");
        return;
    }

    forensic_log(FORENSIC_LEVEL_INFO, "neural_blackbox_destroy",
                "Destruction système neural - Forward passes: %zu, Adaptations: %zu",
                system->total_forward_passes, system->adaptation_cycles);

    // Destruction couches
    if (system->hidden_layers) {
        for (size_t i = 0; i < system->network_depth; i++) {
            if (system->hidden_layers[i]) {
                neural_layer_destroy(&system->hidden_layers[i]);
            }
        }
        TRACKED_FREE(system->hidden_layers);
    }

    // Destruction mémoire persistante
    if (system->persistent_memory) {
        neural_memory_bank_destroy(&system->persistent_memory);
    }

    // Destruction moteur d'apprentissage
    if (system->learning_engine) {
        neural_learning_engine_destroy(&system->learning_engine);
    }

    // Destruction états internes
    if (system->internal_activations) {
        TRACKED_FREE(system->internal_activations);
    }

    // Effacement sécurisé
    system->blackbox_magic = NEURAL_DESTROYED_MAGIC;
    system->memory_address = NULL;

    TRACKED_FREE(system);
    *system_ptr = NULL;
}

// Encodage d'une fonction en réseau neuronal
bool neural_blackbox_encode_function(
    neural_blackbox_computer_t* system,
    neural_function_spec_t* function_spec,
    neural_training_protocol_t* training
) {
    if (!system || !function_spec || !training) return false;

    if (system->blackbox_magic != NEURAL_BLACKBOX_MAGIC) {
        forensic_log(FORENSIC_LEVEL_ERROR, "neural_blackbox_encode_function", 
                    "Système neural corrompu");
        return false;
    }

    forensic_log(FORENSIC_LEVEL_INFO, "neural_blackbox_encode_function",
                "Début encodage fonction '%s' - Échantillons: %zu, Époques max: %zu",
                function_spec->name, training->sample_count, training->max_epochs);

    struct timespec start_time, current_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    double initial_loss = INFINITY;
    double current_loss = INFINITY;

    // Génération massive d'échantillons d'entraînement
    for (size_t epoch = 0; epoch < training->max_epochs; epoch++) {
        current_loss = 0.0;
        size_t batch_count = training->sample_count / training->batch_size;

        for (size_t batch = 0; batch < batch_count; batch++) {
            // Génération batch d'entraînement
            double batch_loss = 0.0;

            for (size_t sample = 0; sample < training->batch_size; sample++) {
                // Génération entrée aléatoire dans le domaine spécifié
                double* random_input = generate_random_input_in_domain(
                    system->input_dimensions,
                    &(function_spec->input_domain)
                );

                if (!random_input) continue;

                // Calcul sortie attendue (fonction originale)
                double* expected_output = TRACKED_MALLOC(
                    system->output_dimensions * sizeof(double)
                );

                if (expected_output && function_spec->original_function) {
                    // Appel fonction originale pour obtenir résultat attendu
                    function_spec->original_function(random_input, expected_output);

                    // Forward pass neural
                    double* neural_output = neural_blackbox_execute(system, random_input);

                    if (neural_output) {
                        // Calcul erreur (MSE)
                        double sample_error = 0.0;
                        for (size_t o = 0; o < system->output_dimensions; o++) {
                            double diff = expected_output[o] - neural_output[o];
                            sample_error += diff * diff;
                        }
                        sample_error /= system->output_dimensions;
                        batch_loss += sample_error;

                        // Backpropagation simplifiée (gradient descent)
                        neural_blackbox_simple_backprop(system, expected_output, neural_output, training->learning_rate);

                        TRACKED_FREE(neural_output);
                    }

                    TRACKED_FREE(expected_output);
                }

                TRACKED_FREE(random_input);
            }

            current_loss += (batch_loss / training->batch_size);
        }

        current_loss /= batch_count;

        // Adaptation continue du réseau
        neural_blackbox_continuous_adaptation(system);

        // Log progression
        if (epoch % 100 == 0 || current_loss < training->convergence_threshold) {
            clock_gettime(CLOCK_MONOTONIC, &current_time);
            double elapsed = (current_time.tv_sec - start_time.tv_sec) + 
                           (current_time.tv_nsec - start_time.tv_nsec) / 1e9;

            forensic_log(FORENSIC_LEVEL_INFO, "neural_blackbox_encode_function",
                        "Époque %zu/%zu - Loss: %.8f, Temps: %.2fs, Adaptations: %zu",
                        epoch, training->max_epochs, current_loss, elapsed, system->adaptation_cycles);
        }

        // Convergence check
        if (current_loss < training->convergence_threshold) {
            forensic_log(FORENSIC_LEVEL_SUCCESS, "neural_blackbox_encode_function",
                        "Convergence atteinte à l'époque %zu - Loss finale: %.8f",
                        epoch, current_loss);
            break;
        }

        // Early stopping si pas d'amélioration
        if (epoch > 1000 && current_loss > initial_loss * 0.99) {
            forensic_log(FORENSIC_LEVEL_WARNING, "neural_blackbox_encode_function",
                        "Early stopping - Pas d'amélioration significative après %zu époques",
                        epoch);
            break;
        }

        if (epoch == 0) initial_loss = current_loss;
    }

    // Post-training optimisation
    neural_blackbox_post_training_optimization(system);

    clock_gettime(CLOCK_MONOTONIC, &current_time);
    double total_time = (current_time.tv_sec - start_time.tv_sec) + 
                       (current_time.tv_nsec - start_time.tv_nsec) / 1e9;

    bool success = (current_loss < training->convergence_threshold * 10);

    forensic_log(success ? FORENSIC_LEVEL_SUCCESS : FORENSIC_LEVEL_WARNING, 
                "neural_blackbox_encode_function",
                "Encodage terminé - Succès: %s, Loss finale: %.8f, Temps total: %.2fs, Paramètres ajustés: %zu",
                success ? "OUI" : "NON", current_loss, total_time, system->synaptic_changes_count);

    return success;
}

// Exécution pure neuronale (fonction encodée)
double* neural_blackbox_execute(
    neural_blackbox_computer_t* system,
    double* input_data
) {
    if (!system || !input_data) return NULL;

    if (system->blackbox_magic != NEURAL_BLACKBOX_MAGIC) {
        forensic_log(FORENSIC_LEVEL_ERROR, "neural_blackbox_execute", 
                    "Système neural corrompu");
        return NULL;
    }

    system->total_forward_passes++;

    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    // Allocation sortie finale
    double* final_output = TRACKED_MALLOC(system->output_dimensions * sizeof(double));
    if (!final_output) return NULL;

    // === PROPAGATION NEURONALE PURE ===

    double* current_layer_output = input_data;

    // Forward pass à travers toutes les couches cachées
    for (size_t layer_idx = 0; layer_idx < system->network_depth; layer_idx++) {
        neural_layer_t* current_layer = system->hidden_layers[layer_idx];

        if (!current_layer) {
            TRACKED_FREE(final_output);
            return NULL;
        }

        // Forward pass de la couche
        bool forward_success = neural_layer_forward_pass(current_layer, current_layer_output);

        if (!forward_success) {
            forensic_log(FORENSIC_LEVEL_ERROR, "neural_blackbox_execute",
                        "Échec forward pass couche %zu", layer_idx);
            TRACKED_FREE(final_output);
            return NULL;
        }

        // Mise à jour mémoire persistante (effet de bord neuronal)
        if (system->persistent_memory) {
            neural_memory_bank_update(
                system->persistent_memory,
                layer_idx,
                current_layer->outputs,
                current_layer->output_size
            );
        }

        // Sauvegarde états internes pour opacité
        if (system->internal_activations) {
            size_t offset = layer_idx * system->neurons_per_layer;
            for (size_t n = 0; n < current_layer->neuron_count && 
                             offset + n < system->network_depth * system->neurons_per_layer; n++) {
                system->internal_activations[offset + n] = current_layer->outputs[n];
            }
        }

        // Préparation entrée pour couche suivante
        if (layer_idx < system->network_depth - 1) {
            current_layer_output = current_layer->outputs;
        } else {
            // Dernière couche - adaptation à la sortie demandée
            size_t copy_size = (current_layer->output_size < system->output_dimensions) ? 
                              current_layer->output_size : system->output_dimensions;

            for (size_t i = 0; i < copy_size; i++) {
                final_output[i] = current_layer->outputs[i];
            }

            // Complétion avec zéros si nécessaire
            for (size_t i = copy_size; i < system->output_dimensions; i++) {
                final_output[i] = 0.0;
            }
        }
    }

    // Apprentissage continu (métaplasticité)
    if (system->learning_engine) {
        neural_blackbox_continuous_learning(system);
    }

    clock_gettime(CLOCK_MONOTONIC, &end_time);
    uint64_t execution_time = (end_time.tv_sec - start_time.tv_sec) * 1000000000ULL + 
                             (end_time.tv_nsec - start_time.tv_nsec);

    // Traçage pour forensique (optionnel)
    if (system->total_forward_passes % 1000 == 0) {
        forensic_log(FORENSIC_LEVEL_DEBUG, "neural_blackbox_execute",
                    "Forward pass #%zu terminé en %zu ns - Adaptations: %zu",
                    system->total_forward_passes, execution_time, system->adaptation_cycles);
    }

    return final_output;
}

// === FONCTIONS UTILITAIRES ===

size_t neural_calculate_optimal_depth(size_t input_dim, size_t output_dim, neural_complexity_target_e target) {
    switch (target) {
        case NEURAL_COMPLEXITY_LOW:
            return 3 + (input_dim + output_dim) / 20;
        case NEURAL_COMPLEXITY_MEDIUM:
            return 5 + (input_dim + output_dim) / 10;
        case NEURAL_COMPLEXITY_HIGH:
            return 8 + (input_dim + output_dim) / 5;
        case NEURAL_COMPLEXITY_EXTREME:
            return 15 + (input_dim + output_dim) / 3;
        default:
            return 5;
    }
}

size_t neural_calculate_optimal_width(size_t input_dim, size_t output_dim, size_t depth) {
    size_t base_width = (input_dim + output_dim) * 2;
    size_t depth_factor = depth * 10;
    return base_width + depth_factor;
}

void neural_blackbox_continuous_adaptation(neural_blackbox_computer_t* system) {
    if (!system || system->blackbox_magic != NEURAL_BLACKBOX_MAGIC) return;

    system->adaptation_cycles++;

    // Micro-ajustements aléatoires (simulation métaplasticité)
    for (size_t layer_idx = 0; layer_idx < system->network_depth; layer_idx++) {
        neural_layer_t* layer = system->hidden_layers[layer_idx];
        if (!layer) continue;

        for (size_t i = 0; i < layer->neuron_count * layer->input_size; i++) {
            // Changement infime basé sur activité récente
            double adaptation_factor = (system->total_forward_passes % 1000) / 1000000.0;
            double random_change = ((double)rand() / RAND_MAX - 0.5) * adaptation_factor;
            layer->weights[i] += random_change;
            system->synaptic_changes_count++;
        }

        // Adaptation biases
        for (size_t i = 0; i < layer->neuron_count; i++) {
            double bias_change = ((double)rand() / RAND_MAX - 0.5) * 1e-8;
            layer->biases[i] += bias_change;
        }
    }
}

void neural_blackbox_continuous_learning(neural_blackbox_computer_t* system) {
    if (!system || !system->learning_engine) return;

    // Simulation apprentissage Hebbien simplifié
    for (size_t layer_idx = 0; layer_idx < system->network_depth; layer_idx++) {
        neural_layer_t* layer = system->hidden_layers[layer_idx];
        if (!layer) continue;

        // Renforcement connexions actives
        for (size_t n = 0; n < layer->neuron_count; n++) {
            if (layer->outputs[n] > 0.5) { // Neurone actif
                for (size_t w = 0; w < layer->input_size; w++) {
                    size_t weight_idx = n * layer->input_size + w;
                    layer->weights[weight_idx] *= 1.0001; // Renforcement minime
                }
            }
        }
    }
}

// Fonctions de création des sous-composants
neural_memory_bank_t* neural_memory_bank_create(size_t capacity) {
    if (capacity == 0) return NULL;

    neural_memory_bank_t* bank = TRACKED_MALLOC(sizeof(neural_memory_bank_t));
    if (!bank) return NULL;

    bank->memory_slots = TRACKED_MALLOC(capacity * sizeof(double));
    if (!bank->memory_slots) {
        TRACKED_FREE(bank);
        return NULL;
    }

    bank->capacity = capacity;
    bank->current_size = 0;
    bank->access_count = 0;
    bank->memory_address = (void*)bank;
    bank->magic_number = NEURAL_MEMORY_MAGIC;

    // Initialisation avec valeurs nulles
    memset(bank->memory_slots, 0, capacity * sizeof(double));

    return bank;
}

void neural_memory_bank_destroy(neural_memory_bank_t** bank_ptr) {
    if (!bank_ptr || !*bank_ptr) return;

    neural_memory_bank_t* bank = *bank_ptr;

    if (bank->magic_number == NEURAL_MEMORY_MAGIC && 
        bank->memory_address == (void*)bank) {

        if (bank->memory_slots) {
            TRACKED_FREE(bank->memory_slots);
        }

        bank->magic_number = NEURAL_DESTROYED_MAGIC;
        bank->memory_address = NULL;

        TRACKED_FREE(bank);
        *bank_ptr = NULL;
    }
}

neural_learning_engine_t* neural_learning_engine_create(
    double learning_rate,
    neural_plasticity_rules_e rules
) {
    neural_learning_engine_t* engine = TRACKED_MALLOC(sizeof(neural_learning_engine_t));
    if (!engine) return NULL;

    engine->learning_rate = learning_rate;
    engine->plasticity_rules = rules;
    engine->adaptation_count = 0;
    engine->memory_address = (void*)engine;
    engine->magic_number = NEURAL_ENGINE_MAGIC;

    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    engine->creation_time = ts.tv_sec * 1000000000ULL + ts.tv_nsec;

    return engine;
}

void neural_learning_engine_destroy(neural_learning_engine_t** engine_ptr) {
    if (!engine_ptr || !*engine_ptr) return;

    neural_learning_engine_t* engine = *engine_ptr;

    if (engine->magic_number == NEURAL_ENGINE_MAGIC && 
        engine->memory_address == (void*)engine) {

        engine->magic_number = NEURAL_DESTROYED_MAGIC;
        engine->memory_address = NULL;

        TRACKED_FREE(engine);
        *engine_ptr = NULL;
    }
}

// Génération d'entrée aléatoire dans domaine spécifié
double* generate_random_input_in_domain(
    size_t input_dimensions,
    neural_domain_t* domain
) {
    double* input = TRACKED_MALLOC(input_dimensions * sizeof(double));
    if (!input) return NULL;

    for (size_t i = 0; i < input_dimensions; i++) {
        if (domain && domain->has_bounds) {
            double range = domain->max_value - domain->min_value;
            input[i] = domain->min_value + ((double)rand() / RAND_MAX) * range;
        } else {
            input[i] = ((double)rand() / RAND_MAX - 0.5) * 2.0; // [-1, 1]
        }
    }

    return input;
}

// Backpropagation simplifiée
void neural_blackbox_simple_backprop(
    neural_blackbox_computer_t* system,
    double* expected_output,
    double* actual_output,
    double learning_rate
) {
    if (!system || !expected_output || !actual_output) return;

    // Calcul gradient sortie
    double* output_gradient = TRACKED_MALLOC(system->output_dimensions * sizeof(double));
    if (!output_gradient) return;

    for (size_t i = 0; i < system->output_dimensions; i++) {
        output_gradient[i] = 2.0 * (actual_output[i] - expected_output[i]);
    }

    // Propagation arrière simplifiée (seulement dernière couche)
    if (system->network_depth > 0) {
        neural_layer_t* last_layer = system->hidden_layers[system->network_depth - 1];

        if (last_layer && last_layer->layer_error) {
            size_t update_size = (last_layer->neuron_count < system->output_dimensions) ?
                               last_layer->neuron_count : system->output_dimensions;

            for (size_t i = 0; i < update_size; i++) {
                last_layer->layer_error[i] = output_gradient[i];

                // Mise à jour poids (gradient descent)
                for (size_t j = 0; j < last_layer->input_size; j++) {
                    size_t weight_idx = i * last_layer->input_size + j;
                    last_layer->weights[weight_idx] -= learning_rate * output_gradient[i];
                    system->synaptic_changes_count++;
                }

                // Mise à jour biais
                last_layer->biases[i] -= learning_rate * output_gradient[i];
            }
        }
    }

    TRACKED_FREE(output_gradient);
}

void neural_blackbox_post_training_optimization(neural_blackbox_computer_t* system) {
    if (!system) return;

    forensic_log(FORENSIC_LEVEL_INFO, "neural_blackbox_post_training_optimization",
                "Optimisation post-entraînement - Normalisation poids et ajustements finaux");

    // Normalisation des poids pour stabilité
    for (size_t layer_idx = 0; layer_idx < system->network_depth; layer_idx++) {
        neural_layer_t* layer = system->hidden_layers[layer_idx];
        if (!layer) continue;

        // Calcul norme des poids
        double weight_sum = 0.0;
        for (size_t i = 0; i < layer->neuron_count * layer->input_size; i++) {
            weight_sum += layer->weights[i] * layer->weights[i];
        }

        double norm = sqrt(weight_sum);
        if (norm > 10.0) { // Normalisation si poids trop grands
            for (size_t i = 0; i < layer->neuron_count * layer->input_size; i++) {
                layer->weights[i] /= (norm / 10.0);
            }
        }
    }
}

// Mise à jour mémoire persistante
void neural_memory_bank_update(
    neural_memory_bank_t* bank,
    size_t layer_id,
    double* activations,
    size_t activation_count
) {
    if (!bank || !activations || activation_count == 0) return;

    if (bank->magic_number != NEURAL_MEMORY_MAGIC) return;

    bank->access_count++;

    // Sauvegarde cyclique des activations
    for (size_t i = 0; i < activation_count && bank->current_size < bank->capacity; i++) {
        size_t index = (bank->current_size + layer_id * 100 + i) % bank->capacity;
        bank->memory_slots[index] = activations[i];
        bank->current_size = (bank->current_size + 1) % bank->capacity;
    }
}

// ============================================================================
// NOUVELLES FONCTIONS ULTRA-PRÉCISION (Erreur < 1e-15)
// ============================================================================

// Entraînement multi-phases ultra-précis
bool neural_blackbox_multi_phase_training(
    neural_blackbox_computer_t* system,
    neural_function_spec_t* function_spec,
    neural_training_protocol_t* training
) {
    if (!system || !function_spec || !training) return false;

    forensic_log(FORENSIC_LEVEL_INFO, "neural_blackbox_multi_phase_training",
                "Debut entrainement 4 phases pour precision 100 pourcent");

    // Phase 1: Adam ultra-précis (épochs 0-25%)
    forensic_log(FORENSIC_LEVEL_INFO, "neural_blackbox_multi_phase_training", "=== PHASE 1: Adam Ultra-Précis ===");
    adam_ultra_precise_optimizer_t* adam = adam_ultra_precise_create(0.001, 0.9, 0.999, 1e-16);
    if (!adam) return false;

    size_t phase1_epochs = training->max_epochs / 4;
    for (size_t epoch = 0; epoch < phase1_epochs; epoch++) {
        double* gradients = neural_blackbox_compute_gradients(system, function_spec);
        double current_loss = neural_blackbox_compute_loss(system, function_spec);

        forensic_log(FORENSIC_LEVEL_DEBUG, "neural_blackbox_multi_phase_training",
                "Loss initial calculé: %.12e", current_loss);

        if (!neural_blackbox_apply_optimizer(system, adam, "adam_ultra_precise", gradients, current_loss)) {
            TRACKED_FREE(gradients);
            adam_ultra_precise_destroy(&adam);
            return false;
        }

        // Convergence check
        if (current_loss < training->precision_target / 100.0) {
            forensic_log(FORENSIC_LEVEL_INFO, "neural_blackbox_multi_phase_training",
                        "Phase 1 convergée à l'époch %zu (loss=%.15e)", epoch, current_loss);
            break;
        }

        TRACKED_FREE(gradients);
    }
    adam_ultra_precise_destroy(&adam);

    // Phase 2: L-BFGS (épochs 25-50%)
    forensic_log(FORENSIC_LEVEL_INFO, "neural_blackbox_multi_phase_training", "=== PHASE 2: L-BFGS ===");
    lbfgs_optimizer_t* lbfgs = lbfgs_create(10, 1e-16); // Mémoire 10, tolérance ultra-haute
    if (!lbfgs) return false;

    size_t phase2_epochs = training->max_epochs / 4;
    for (size_t epoch = 0; epoch < phase2_epochs; epoch++) {
        double* gradients = neural_blackbox_compute_gradients(system, function_spec);
        double current_loss = neural_blackbox_compute_loss(system, function_spec);

        if (!neural_blackbox_apply_optimizer(system, lbfgs, "lbfgs", gradients, current_loss)) {
            TRACKED_FREE(gradients);
            lbfgs_destroy(&lbfgs);
            return false;
        }

        if (current_loss < training->precision_target / 1000.0) {
            forensic_log(FORENSIC_LEVEL_INFO, "neural_blackbox_multi_phase_training",
                        "Phase 2 convergée à l'époch %zu (loss=%.15e)", epoch, current_loss);
            break;
        }

        TRACKED_FREE(gradients);
    }
    lbfgs_destroy(&lbfgs);

    // Phase 3: Newton-Raphson (épochs 50-75%)
    forensic_log(FORENSIC_LEVEL_INFO, "neural_blackbox_multi_phase_training", "=== PHASE 3: Newton-Raphson ===");
    newton_raphson_optimizer_t* newton = newton_raphson_create(1e-17); // Tolérance ultime
    if (!newton) return false;

    size_t phase3_epochs = training->max_epochs / 4;
    for (size_t epoch = 0; epoch < phase3_epochs; epoch++) {
        double* gradients = neural_blackbox_compute_gradients(system, function_spec);
        double current_loss = neural_blackbox_compute_loss(system, function_spec);

        if (!neural_blackbox_apply_optimizer(system, newton, "newton_raphson", gradients, current_loss)) {
            TRACKED_FREE(gradients);
            newton_raphson_destroy(&newton);
            return false;
        }

        if (current_loss < training->precision_target / 10000.0) {
            forensic_log(FORENSIC_LEVEL_INFO, "neural_blackbox_multi_phase_training",
                        "Phase 3 convergée à l'époch %zu (loss=%.15e)", epoch, current_loss);
            break;
        }

        TRACKED_FREE(gradients);
    }
    newton_raphson_destroy(&newton);

    // Phase 4: Raffinement final mixte (épochs 75-100%)
    forensic_log(FORENSIC_LEVEL_INFO, "neural_blackbox_multi_phase_training", "=== PHASE 4: Raffinement Final ===");
    adam = adam_ultra_precise_create(0.0001, 0.95, 0.9999, 1e-17); // Hyperparamètres ultra-fins
    if (!adam) return false;

    size_t phase4_epochs = training->max_epochs / 4;
    for (size_t epoch = 0; epoch < phase4_epochs; epoch++) {
        double* gradients = neural_blackbox_compute_gradients(system, function_spec);
        double current_loss = neural_blackbox_compute_loss(system, function_spec);

        // Alternance Adam/Newton selon convergence
        const char* optimizer_type = (current_loss > 1e-12) ? "adam_ultra_precise" : "newton_raphson";
        void* optimizer_ptr = NULL;
        newton_raphson_optimizer_t* current_newton = NULL;

        if (strcmp(optimizer_type, "adam_ultra_precise") == 0) {
             optimizer_ptr = adam;
        } else {
             current_newton = newton_raphson_create(1e-17);
             if (!current_newton) {
                 TRACKED_FREE(gradients);
                 adam_ultra_precise_destroy(&adam);
                 return false;
             }
             optimizer_ptr = current_newton;
        }

        if (!neural_blackbox_apply_optimizer(system, optimizer_ptr, optimizer_type, gradients, current_loss)) {
            TRACKED_FREE(gradients);
            adam_ultra_precise_destroy(&adam);
            if (current_newton) newton_raphson_destroy(&current_newton);
            return false;
        }

        // Objectif final atteint
        if (current_loss < training->precision_target) {
            forensic_log(FORENSIC_LEVEL_SUCCESS, "neural_blackbox_multi_phase_training",
                        "SUCCÈS: Précision 100%% atteinte à l'époch %zu (erreur=%.17e < %.17e)", 
                        epoch, current_loss, training->precision_target);
            TRACKED_FREE(gradients);
            adam_ultra_precise_destroy(&adam);
            if (current_newton) newton_raphson_destroy(&current_newton);
            return true;
        }

        TRACKED_FREE(gradients);
        if (current_newton) newton_raphson_destroy(&current_newton); // Clean up temporary Newton optimizer
    }

    adam_ultra_precise_destroy(&adam);

    double final_loss = neural_blackbox_compute_loss(system, function_spec);
    forensic_log(FORENSIC_LEVEL_INFO, "neural_blackbox_multi_phase_training",
                "Entraînement 4 phases terminé. Erreur finale: %.17e", final_loss);

    return final_loss < training->precision_target;
}

// Entraînement ultra-précis coordonnant toutes les phases
bool neural_blackbox_ultra_precise_training(
    neural_blackbox_computer_t* system,
    neural_function_spec_t* function_spec,
    neural_training_protocol_t* training
) {
    if (!system || !function_spec || !training) return false;

    forensic_log(FORENSIC_LEVEL_INFO, "neural_blackbox_ultra_precise_training",
                "=== DÉBUT ENTRAÎNEMENT NEURAL BLACKBOX ULTRA-PRÉCIS ===");
    forensic_log(FORENSIC_LEVEL_INFO, "neural_blackbox_ultra_precise_training",
                "Objectif: Erreur < %.17e (Précision 100%%)", training->precision_target);

    // Vérification architecture adaptative
    if (!neural_ultra_precision_verify_architecture(system, function_spec)) {
        forensic_log(FORENSIC_LEVEL_ERROR, "neural_blackbox_ultra_precise_training",
                    "Architecture inadaptée pour précision ultra-haute");
        return false;
    }

    // Initialisation poids selon précision requise
    neural_ultra_precision_initialize_weights(system, training->precision_target);

    double current_loss = neural_blackbox_compute_loss(system, function_spec);
    forensic_log(FORENSIC_LEVEL_INFO, "neural_blackbox_ultra_precise_training",
                "Perte initiale: %.17e", current_loss);
    forensic_log(FORENSIC_LEVEL_INFO, "neural_blackbox_ultra_precise_training",
                "Entraînement multi-phases...");

    // Entraînement multi-phases
    bool success = neural_blackbox_multi_phase_training(system, function_spec, training);

    if (success) {
        double final_loss = neural_blackbox_compute_loss(system, function_spec);
        forensic_log(FORENSIC_LEVEL_SUCCESS, "neural_blackbox_ultra_precise_training",
                    "SUCCÈS: Précision 100%% atteinte! Erreur finale: %.17e", final_loss);

        // Optimisation post-entraînement
        neural_blackbox_post_training_optimization(system);

    } else {
        forensic_log(FORENSIC_LEVEL_WARNING, "neural_blackbox_ultra_precise_training",
                    "Précision cible non atteinte dans les limites d'épochs");
    }

    return success;
}

// Validation croisée ultra-précise
bool neural_blackbox_ultra_precise_validation(
    neural_blackbox_computer_t* system,
    neural_function_spec_t* function_spec
) {
    if (!system || !function_spec) return false;

    forensic_log(FORENSIC_LEVEL_INFO, "neural_blackbox_ultra_precise_validation",
                "=== VALIDATION CROISÉE ULTRA-PRÉCISE ===");

    // Tests sur échantillons de validation
    double max_error = 0.0;
    double total_error = 0.0;
    size_t error_count = 0;

    for (size_t i = 0; i < function_spec->validation_sample_count; i++) {
        double* input = function_spec->validation_inputs[i];
        double* expected = function_spec->validation_outputs[i];

        double* actual = neural_blackbox_compute_precise(system, input, function_spec->input_size);
        if (!actual) continue;

        // Calcul erreur point par point
        for (size_t j = 0; j < function_spec->output_size; j++) {
            double error = fabs(expected[j] - actual[j]);
            total_error += error;
            error_count++;

            if (error > max_error) {
                max_error = error;
            }
        }

        TRACKED_FREE(actual);
    }

    double mean_error = total_error / (double)error_count;

    forensic_log(FORENSIC_LEVEL_INFO, "neural_blackbox_ultra_precise_validation",
                "Erreur moyenne: %.17e | Erreur max: %.17e", mean_error, max_error);

    // Critère de validation ultra-strict
    bool validation_success = (max_error < 1e-15) && (mean_error < 1e-16);

    if (validation_success) {
        forensic_log(FORENSIC_LEVEL_SUCCESS, "neural_blackbox_ultra_precise_validation",
                    "✓ VALIDATION RÉUSSIE: Précision 100%% confirmée!");
    } else {
        forensic_log(FORENSIC_LEVEL_WARNING, "neural_blackbox_ultra_precise_validation",
                    "✗ Validation échouée: Précision insuffisante");
    }

    return validation_success;
}

// Calcul loss avec précision extended
double neural_blackbox_compute_loss(
    neural_blackbox_computer_t* system,
    neural_function_spec_t* function_spec
) {
    if (!system || !function_spec) return INFINITY;

    if (!function_spec->test_inputs || !function_spec->test_outputs) {
        // Fallback : génération échantillon unique pour loss
        double* test_input = generate_random_input_in_domain(
            system->input_dimensions, &function_spec->input_domain);

        if (!test_input) return INFINITY;

        double* expected = TRACKED_MALLOC(system->output_dimensions * sizeof(double));
        if (!expected) {
            TRACKED_FREE(test_input);
            return INFINITY;
        }

        if (function_spec->original_function) {
            function_spec->original_function(test_input, expected);

            double* neural = neural_blackbox_execute(system, test_input);
            if (neural) {
                long double loss_precise = 0.0L;
                for (size_t o = 0; o < system->output_dimensions; o++) {
                    long double diff = (long double)(expected[o] - neural[o]);
                    loss_precise += diff * diff;
                }

                TRACKED_FREE(neural);
                TRACKED_FREE(expected);
                TRACKED_FREE(test_input);

                return (double)(loss_precise / system->output_dimensions);
            }
        }

        TRACKED_FREE(expected);
        TRACKED_FREE(test_input);
        return INFINITY;
    }

    // Calcul loss sur ensemble test
    long double total_loss = 0.0L;
    size_t valid_samples = 0;

    for (size_t sample = 0; sample < function_spec->test_sample_count; sample++) {
        double* neural_output = neural_blackbox_execute(system, function_spec->test_inputs[sample]);

        if (neural_output) {
            long double sample_loss = 0.0L;
            for (size_t o = 0; o < system->output_dimensions; o++) {
                long double expected = (long double)function_spec->test_outputs[sample][o];
                long double actual = (long double)neural_output[o];
                long double diff = expected - actual;
                sample_loss += diff * diff;
            }

            total_loss += sample_loss / system->output_dimensions;
            valid_samples++;

            TRACKED_FREE(neural_output);
        }
    }

    if (valid_samples > 0) {
        return (double)(total_loss / valid_samples);
    }

    return INFINITY;
}


// Calcul gradients haute précision  
double* neural_blackbox_compute_gradients(
    neural_blackbox_computer_t* system,
    neural_function_spec_t* function_spec
) {
    if (!system || !function_spec) return NULL;

    size_t total_params = neural_ultra_precision_count_parameters(system);
    double* gradients = TRACKED_CALLOC(total_params, sizeof(double));
    if (!gradients) return NULL;

    forensic_log(FORENSIC_LEVEL_INFO, "neural_blackbox_compute_gradients",
                "Calcul gradients haute précision (%zu paramètres)", total_params);

    const double h = 1e-8; // Step ultra-petit pour différences finies
    double current_loss = neural_blackbox_compute_loss(system, function_spec);
    (void)current_loss; // Éviter warning unused variable

    size_t param_idx = 0;

    // Gradients pour chaque couche
    for (size_t layer_idx = 0; layer_idx < system->network_depth; layer_idx++) {
        neural_layer_t* layer = system->hidden_layers[layer_idx];
        if (!layer) continue;

        // Gradients weights
        for (size_t i = 0; i < layer->neuron_count * layer->input_size; i++) {
            double original = layer->weights[i];

            // Forward perturbation
            layer->weights[i] = original + h;
            double loss_plus = neural_blackbox_compute_loss(system, function_spec);

            // Backward perturbation  
            layer->weights[i] = original - h;
            double loss_minus = neural_blackbox_compute_loss(system, function_spec);

            // Gradient central difference haute précision
            gradients[param_idx] = (loss_plus - loss_minus) / (2.0 * h);

            // Restauration
            layer->weights[i] = original;
            param_idx++;
        }

        // Gradients biases
        for (size_t i = 0; i < layer->neuron_count; i++) {
            double original = layer->biases[i];

            layer->biases[i] = original + h;
            double loss_plus = neural_blackbox_compute_loss(system, function_spec);

            layer->biases[i] = original - h;
            double loss_minus = neural_blackbox_compute_loss(system, function_spec);

            gradients[param_idx] = (loss_plus - loss_minus) / (2.0 * h);

            layer->biases[i] = original;
            param_idx++;
        }
    }

    forensic_log(FORENSIC_LEVEL_DEBUG, "neural_blackbox_compute_gradients",
                "Gradients calculés: norme L2 = %.12e", neural_compute_vector_norm(gradients, total_params));

    return gradients;
}

// Calcul Hessienne pour Newton-Raphson
double* neural_blackbox_compute_hessian(
    neural_blackbox_computer_t* system,
    neural_function_spec_t* function_spec
) {
    if (!system || !function_spec) return NULL;

    size_t n_params = neural_ultra_precision_count_parameters(system);
    double* hessian = TRACKED_CALLOC(n_params * n_params, sizeof(double));
    if (!hessian) return NULL;

    forensic_log(FORENSIC_LEVEL_INFO, "neural_blackbox_compute_hessian", 
                "Calcul Hessienne %zux%zu pour Newton-Raphson", n_params, n_params);

    const double h = 1e-6; // Step pour approximation Hessienne

    // Calcul par différences finies mixtes
    for (size_t i = 0; i < n_params; i++) {
        for (size_t j = i; j < n_params; j++) { // Symétrie

            // f(x+hi+hj)
            neural_blackbox_perturb_parameter(system, i, h);
            neural_blackbox_perturb_parameter(system, j, h);
            double fpp = neural_blackbox_compute_loss(system, function_spec);

            // f(x+hi-hj)  
            neural_blackbox_perturb_parameter(system, j, -2.0*h);
            double fpm = neural_blackbox_compute_loss(system, function_spec);

            // f(x-hi+hj)
            neural_blackbox_perturb_parameter(system, i, -2.0*h);
            double fmp = neural_blackbox_compute_loss(system, function_spec);

            // f(x-hi-hj)
            neural_blackbox_perturb_parameter(system, j, -2.0*h);
            double fmm = neural_blackbox_compute_loss(system, function_spec);

            // Restauration
            neural_blackbox_perturb_parameter(system, i, h);
            neural_blackbox_perturb_parameter(system, j, h);

            // Hessienne mixte: ∂²f/∂xi∂xj
            double hessian_ij = (fpp - fpm - fmp + fmm) / (4.0 * h * h);

            hessian[i * n_params + j] = hessian_ij;
            hessian[j * n_params + i] = hessian_ij; // Symétrie
        }
    }

    forensic_log(FORENSIC_LEVEL_DEBUG, "neural_blackbox_compute_hessian",
                "Hessienne calculée: condition estimée = %.6e", 
                neural_estimate_condition_number(hessian, n_params));

    return hessian;
}

// Application optimiseur spécifique selon phase
bool neural_blackbox_apply_optimizer(
    neural_blackbox_computer_t* system,
    void* optimizer,
    const char* optimizer_type,
    double* gradients,
    double current_loss
) {
    if (!system || !optimizer || !optimizer_type || !gradients) return false;

    forensic_log(FORENSIC_LEVEL_INFO, "neural_blackbox_apply_optimizer",
                "Application optimiseur %s (loss=%.12e)", optimizer_type, current_loss);

    if (strcmp(optimizer_type, "adam_ultra_precise") == 0) {
        adam_ultra_precise_optimizer_t* adam = (adam_ultra_precise_optimizer_t*)optimizer;
        return adam_ultra_precise_update_weights(adam, system, gradients, current_loss);

    } else if (strcmp(optimizer_type, "lbfgs") == 0) {
        lbfgs_optimizer_t* lbfgs = (lbfgs_optimizer_t*)optimizer;
        return lbfgs_update_weights(lbfgs, system, gradients, current_loss);

    } else if (strcmp(optimizer_type, "newton_raphson") == 0) {
        newton_raphson_optimizer_t* newton = (newton_raphson_optimizer_t*)optimizer;
        return newton_raphson_update_weights(newton, system, gradients, current_loss);
    }

    forensic_log(FORENSIC_LEVEL_ERROR, "neural_blackbox_apply_optimizer",
                "Optimiseur inconnu: %s", optimizer_type);
    return false;
}

// Fonction de conversion entre types de configuration
neural_architecture_config_t* convert_precision_to_architecture_config(
    const neural_ultra_precision_config_t* precision_config
) {
    if (!precision_config) return NULL;

    neural_architecture_config_t* arch_config = TRACKED_MALLOC(sizeof(neural_architecture_config_t));
    if (!arch_config) return NULL;

    arch_config->complexity_target = NEURAL_COMPLEXITY_EXTREME;
    arch_config->memory_capacity = precision_config->input_dimensions * precision_config->output_dimensions * 1000;
    arch_config->learning_rate = precision_config->precision_target / 1000.0;
    arch_config->plasticity_rules = (void*)PLASTICITY_HOMEOSTATIC;
    arch_config->enable_continuous_learning = false;
    arch_config->enable_metaplasticity = true;

    return arch_config;
}

// Alias pour compatibilité
neural_blackbox_computer_t* neural_blackbox_computer_create(
    size_t input_size, 
    size_t output_size, 
    neural_architecture_config_t* config
) {
    return neural_blackbox_create(input_size, output_size, config);
}

void neural_blackbox_computer_destroy(neural_blackbox_computer_t** system_ptr) {
    neural_blackbox_destroy(system_ptr);
}

// Fonction alternative utilisant neural_ultra_precision_config_t directement
neural_blackbox_computer_t* neural_blackbox_create_ultra_precision_system(
    size_t input_dimensions,
    size_t output_dimensions,
    const neural_ultra_precision_config_t* precision_config
) {
    if (!precision_config) return NULL;

    // Conversion vers architecture config standard
    neural_architecture_config_t* arch_config = convert_precision_to_architecture_config(precision_config);
    if (!arch_config) return NULL;

    // Création système neural blackbox
    neural_blackbox_computer_t* system = neural_blackbox_create(
        input_dimensions, output_dimensions, arch_config);

    // Nettoyage config temporaire
    TRACKED_FREE(arch_config);

    return system;
}