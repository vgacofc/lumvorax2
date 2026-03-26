
#ifndef NEURAL_BLACKBOX_COMPUTER_H
#define NEURAL_BLACKBOX_COMPUTER_H

#include <stddef.h>
#include <stdint.h>
#include "../common/common_types.h"

// Structures complètes pour optimiseurs ultra-précis
typedef struct adam_ultra_precise_optimizer_t {
    double learning_rate;         // Taux d'apprentissage
    double beta1, beta2;          // Paramètres momentum
    double epsilon;               // Stabilité numérique
    size_t iteration;             // Numéro itération courante
    size_t param_count;           // Nombre paramètres
    double* momentum;             // Momentum accumulé
    double* velocity;             // Velocity accumulée
    uint32_t magic_number;        // Protection intégrité
    void* memory_address;         // Protection double-free
} adam_ultra_precise_optimizer_t;

typedef struct lbfgs_optimizer_t {
    int memory_size;              // Taille mémoire L-BFGS
    double tolerance;             // Tolérance convergence
    double* history_s;            // Historique gradients
    double* history_y;            // Historique positions
    size_t history_count;         // Nombre éléments historique
    uint32_t magic_number;        // Protection intégrité
    void* memory_address;         // Protection double-free
} lbfgs_optimizer_t;

typedef struct newton_raphson_optimizer_t {
    double tolerance;             // Tolérance convergence
    double* hessian_inv;          // Inverse Hessienne approximée
    size_t param_count;           // Nombre paramètres
    uint32_t magic_number;        // Protection intégrité
    void* memory_address;         // Protection double-free
} newton_raphson_optimizer_t;


#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "neural_ultra_precision_architecture.h"
#include "../common/common_types.h"

// Types d'activation neuronale (définis dans common_types.h)

// neural_layer_t structure is defined in common_types.h

// Forward declarations des fonctions neural_layer
neural_layer_t* neural_layer_create(size_t neuron_count, size_t input_size, activation_function_e activation);
void neural_layer_destroy(neural_layer_t** layer_ptr);
bool neural_layer_forward_pass(neural_layer_t* layer, double* input);
double neural_activation_function(double x, activation_function_e type);

// === MODULE NEURAL BLACKBOX COMPUTER 100% NATIF ===
// Concept révolutionnaire : Utilisation authentique des réseaux neuronaux
// pour créer une opacité naturelle sans simulation artificielle

// Utilisation enum défini dans neural_ultra_precision_architecture.h

// Règles de plasticité neuronale (définies dans common_types.h)

// Types d'activation neuronale (suppression duplication)

// Domaine d'entrée pour génération d'échantillons
typedef struct {
    double min_value;               // Valeur minimale
    double max_value;               // Valeur maximale
    bool has_bounds;                // Domaine borné ou non
    void* custom_generator;         // Générateur personnalisé (optionnel)
} neural_domain_t;

// Spécification de fonction à encoder
typedef struct {
    char name[128];                 // Nom de la fonction
    void (*original_function)(void* input, void* output);  // Fonction originale
    neural_domain_t input_domain;   // Domaine d'entrée
    neural_domain_t output_domain;  // Domaine de sortie
    size_t complexity_hint;         // Indice de complexité
    // Champs ajoutés pour compatibilité avec les tests
    size_t input_size;              // Taille entrée pour tests
    size_t output_size;             // Taille sortie pour tests
    double** test_inputs;           // Données d'entrée de test
    double** test_outputs;          // Données de sortie attendues
    size_t test_sample_count;       // Nombre échantillons test
    double** validation_inputs;     // Données validation
    double** validation_outputs;    // Sorties validation attendues
    size_t validation_sample_count; // Nombre échantillons validation
} neural_function_spec_t;

// Protocole d'entraînement
typedef struct {
    size_t sample_count;            // Nombre d'échantillons d'entraînement
    size_t max_epochs;              // Époques maximales
    double convergence_threshold;   // Seuil de convergence
    double learning_rate;           // Taux d'apprentissage
    size_t batch_size;              // Taille des batchs
    bool enable_early_stopping;    // Arrêt précoce
    double validation_split;        // Proportion validation
    // Champs ajoutés pour compatibilité ultra-précision
    double precision_target;        // Cible de précision (ex: 1e-15)
    double tolerance;               // Tolérance d'erreur
} neural_training_protocol_t;

// Using definition from neural_ultra_precision_architecture.h

// Banque de mémoire neuronale persistante
typedef struct {
    double* memory_slots;           // Slots de mémoire
    size_t capacity;                // Capacité totale
    size_t current_size;            // Taille actuelle
    uint64_t access_count;          // Nombre d'accès
    void* memory_address;           // Protection double-free
    uint32_t magic_number;          // Validation intégrité
} neural_memory_bank_t;

// Moteur d'apprentissage continu
typedef struct {
    double learning_rate;                          // Taux d'apprentissage
    neural_plasticity_rules_e plasticity_rules;   // Règles appliquées
    uint64_t adaptation_count;                     // Nombre d'adaptations
    uint64_t creation_time;                        // Timestamp création
    void* memory_address;                          // Protection double-free
    uint32_t magic_number;                         // Validation intégrité
} neural_learning_engine_t;

// Structure principale : Neural Blackbox Computer 100% natif
typedef struct {
    // Architecture réseau
    size_t input_dimensions;        // Dimensions d'entrée
    size_t output_dimensions;       // Dimensions de sortie
    size_t network_depth;           // Profondeur du réseau
    size_t neurons_per_layer;       // Neurones par couche
    size_t total_parameters;        // Paramètres totaux
    
    // Couches cachées (cœur du système)
    neural_layer_t** hidden_layers; // Array de couches cachées
    
    // Composants pour opacité native
    neural_memory_bank_t* persistent_memory;      // Mémoire persistante
    neural_learning_engine_t* learning_engine;    // Moteur apprentissage
    
    // États internes (millions de valeurs pour opacité)
    double* internal_activations;   // États d'activation internes
    uint64_t synaptic_changes_count;// Compteur changements synaptiques
    
    // Métriques et traçage
    uint64_t total_forward_passes;  // Nombre de passages avant
    uint64_t adaptation_cycles;     // Cycles d'adaptation
    uint64_t creation_timestamp;    // Timestamp création
    
    // Sécurité et intégrité
    uint32_t blackbox_magic;        // Magic number validation
    void* memory_address;           // Protection double-free
} neural_blackbox_computer_t;

// === FONCTIONS PRINCIPALES ===

// Gestion cycle de vie
neural_blackbox_computer_t* neural_blackbox_create(
    size_t input_dimensions,
    size_t output_dimensions,
    neural_architecture_config_t* config
);

// Alias pour compatibilité avec les tests
#define neural_blackbox_computer_create neural_blackbox_create

void neural_blackbox_destroy(neural_blackbox_computer_t** system_ptr);

// Alias pour compatibilité avec les tests
#define neural_blackbox_computer_destroy(ptr) neural_blackbox_destroy(ptr)

// Encodage de fonction en réseau neuronal
bool neural_blackbox_encode_function(
    neural_blackbox_computer_t* system,
    neural_function_spec_t* function_spec,
    neural_training_protocol_t* training
);

// Exécution pure neuronale
double* neural_blackbox_execute(
    neural_blackbox_computer_t* system,
    double* input_data
);

// === FONCTIONS UTILITAIRES ===

// Calcul architecture optimale
size_t neural_calculate_optimal_depth(
    size_t input_dim, 
    size_t output_dim, 
    neural_complexity_target_e target
);

size_t neural_calculate_optimal_width(
    size_t input_dim, 
    size_t output_dim, 
    size_t depth
);

// Adaptation continue
void neural_blackbox_continuous_adaptation(neural_blackbox_computer_t* system);
void neural_blackbox_continuous_learning(neural_blackbox_computer_t* system);
void neural_blackbox_post_training_optimization(neural_blackbox_computer_t* system);

// Gestion mémoire neuronale
neural_memory_bank_t* neural_memory_bank_create(size_t capacity);
void neural_memory_bank_destroy(neural_memory_bank_t** bank_ptr);
void neural_memory_bank_update(
    neural_memory_bank_t* bank,
    size_t layer_id,
    double* activations,
    size_t activation_count
);

// Gestion moteur d'apprentissage
neural_learning_engine_t* neural_learning_engine_create(
    double learning_rate,
    neural_plasticity_rules_e rules
);
void neural_learning_engine_destroy(neural_learning_engine_t** engine_ptr);

// Fonctions d'entraînement
double* generate_random_input_in_domain(
    size_t input_dimensions,
    neural_domain_t* domain
);

void neural_blackbox_simple_backprop(
    neural_blackbox_computer_t* system,
    double* expected_output,
    double* actual_output,
    double learning_rate
);

// === TESTS ET VALIDATION ===
bool neural_blackbox_test_simple_function(void);
bool neural_blackbox_test_opacity_analysis(void);
bool neural_blackbox_stress_test_encoding(neural_architecture_config_t* config);

// === NOUVELLES FONCTIONS ULTRA-PRÉCISION ===

// Entraînement multi-phases pour précision 100%
bool neural_blackbox_ultra_precise_training(
    neural_blackbox_computer_t* system,
    neural_function_spec_t* function_spec,
    neural_training_protocol_t* training
);

// Validation croisée ultra-précise
bool neural_blackbox_ultra_precise_validation(
    neural_blackbox_computer_t* system,
    neural_function_spec_t* function_spec
);

// Calcul gradients haute précision
double* neural_blackbox_compute_gradients(
    neural_blackbox_computer_t* system,
    neural_function_spec_t* function_spec
);

// Calcul Hessienne pour Newton-Raphson
double* neural_blackbox_compute_hessian(
    neural_blackbox_computer_t* system,
    neural_function_spec_t* function_spec
);

// Calcul loss avec précision extended
double neural_blackbox_compute_loss(
    neural_blackbox_computer_t* system,
    neural_function_spec_t* function_spec
);

// Entraînement progressif 4 phases (grossier → ultra-fin)
bool neural_blackbox_multi_phase_training(
    neural_blackbox_computer_t* system,
    neural_function_spec_t* function_spec,
    neural_training_protocol_t* training
);

// Application optimiseur spécifique selon phase
bool neural_blackbox_apply_optimizer(
    neural_blackbox_computer_t* system,
    void* optimizer,
    const char* optimizer_type,
    double* gradients,
    double current_loss
);

// === CONSTANTES ===
#define NEURAL_BLACKBOX_MAGIC 0x4E424243     // "NBBC" en ASCII
#define NEURAL_MEMORY_MAGIC 0x4E4D454D       // "NMEM" en ASCII  
#define NEURAL_ENGINE_MAGIC 0x4E454E47       // "NENG" en ASCII
#define NEURAL_DESTROYED_MAGIC 0xDEADDEAD

#define NEURAL_MAX_LAYERS_BLACKBOX 50
#define NEURAL_MAX_NEURONS_PER_LAYER_BLACKBOX 5000
#define NEURAL_DEFAULT_MEMORY_CAPACITY 1048576  // 1MB
#define NEURAL_DEFAULT_LEARNING_RATE 0.001
#define NEURAL_MIN_CONVERGENCE_THRESHOLD 1e-8

// Using definition from neural_ultra_precision_architecture.h

// Fonctions de conversion entre types de configuration
neural_architecture_config_t* convert_precision_to_architecture_config(
    const neural_ultra_precision_config_t* precision_config
);

#endif // NEURAL_BLACKBOX_COMPUTER_H
