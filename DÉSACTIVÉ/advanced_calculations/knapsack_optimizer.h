
#ifndef KNAPSACK_OPTIMIZER_H
#define KNAPSACK_OPTIMIZER_H

#include "../lum/lum_core.h"
#include "../debug/memory_tracker.h"
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

// Constantes Knapsack
#define KNAPSACK_MAX_ITEMS 100000000
#define KNAPSACK_MAGIC_NUMBER 0x87654321
#define KNAPSACK_DESTROYED_MAGIC 0xDEADBEEF
#define KNAPSACK_MAX_WEIGHT 999999999
#define KNAPSACK_MAX_VALUE 999999999

// Types d'algorithmes Knapsack
typedef enum {
    KNAPSACK_ALGORITHM_DYNAMIC_PROGRAMMING,
    KNAPSACK_ALGORITHM_GREEDY,
    KNAPSACK_ALGORITHM_BRANCH_BOUND,
    KNAPSACK_ALGORITHM_GENETIC,
    KNAPSACK_ALGORITHM_APPROXIMATION,
    KNAPSACK_ALGORITHM_PARALLEL_DP
} knapsack_algorithm_e;

// Item de sac à dos basé sur LUM
typedef struct {
    lum_t base_lum;              // LUM de base
    int32_t item_id;             // ID unique de l'item
    uint32_t weight;             // Poids de l'item
    uint32_t value;              // Valeur de l'item
    double value_density;        // Ratio valeur/poids
    bool is_selected;            // Marqueur de sélection
    uint64_t selection_timestamp; // Timestamp de sélection
    void* memory_address;        // Protection double-free
    uint32_t item_magic;         // Magic number protection
} knapsack_item_t;

// Sac à dos
typedef struct {
    size_t capacity;             // Capacité maximale
    size_t item_count;           // Nombre d'items
    knapsack_item_t** items;     // Items dans le sac
    uint32_t total_weight;       // Poids total
    uint32_t total_value;        // Valeur totale
    bool is_valid;               // Validité de la solution
    uint64_t computation_time_ns; // Temps de calcul
    void* memory_address;        // Protection double-free
    uint32_t knapsack_magic;     // Magic number protection
} knapsack_t;

// Configuration Knapsack
typedef struct {
    knapsack_algorithm_e algorithm;
    size_t max_iterations;
    double convergence_threshold;
    bool use_parallel_processing;
    size_t thread_count;
    bool enable_memoization;
    bool use_approximation;
    double approximation_ratio;
    size_t population_size;      // Pour algorithme génétique
    double mutation_rate;
    double crossover_rate;
    void* memory_address;
} knapsack_config_t;

// Résultat d'optimisation Knapsack
typedef struct {
    knapsack_t* optimal_knapsack;
    uint32_t best_value;
    uint32_t best_weight;
    size_t items_selected;
    size_t iterations_performed;
    uint64_t total_time_ns;
    bool optimization_success;
    double efficiency_ratio;
    char algorithm_used[64];
    char error_message[256];
    void* memory_address;
} knapsack_result_t;

// Table de mémorisation pour DP
typedef struct {
    size_t item_count;
    size_t capacity;
    uint32_t** dp_table;
    bool** selection_table;
    uint64_t table_creation_time_ns;
    void* memory_address;
} knapsack_dp_table_t;

// Fonctions principales Knapsack
knapsack_item_t* knapsack_item_create(int32_t item_id, uint32_t weight, uint32_t value);
void knapsack_item_destroy(knapsack_item_t** item_ptr);

knapsack_t* knapsack_create(size_t capacity);
void knapsack_destroy(knapsack_t** knapsack_ptr);
bool knapsack_add_item(knapsack_t* knapsack, knapsack_item_t* item);
bool knapsack_can_fit(knapsack_t* knapsack, knapsack_item_t* item);
double knapsack_calculate_efficiency(knapsack_t* knapsack);

// Tables de programmation dynamique
knapsack_dp_table_t* knapsack_dp_table_create(size_t item_count, size_t capacity);
void knapsack_dp_table_destroy(knapsack_dp_table_t** table_ptr);

// Algorithmes d'optimisation
knapsack_result_t* knapsack_optimize_dynamic_programming(knapsack_item_t** items, size_t item_count, 
                                                        size_t capacity, knapsack_config_t* config);
knapsack_result_t* knapsack_optimize_greedy(knapsack_item_t** items, size_t item_count, 
                                           size_t capacity, knapsack_config_t* config);
knapsack_result_t* knapsack_optimize_branch_bound(knapsack_item_t** items, size_t item_count, 
                                                 size_t capacity, knapsack_config_t* config);

// Tests stress
bool knapsack_stress_test_100m_items(knapsack_config_t* config);

// Configuration et nettoyage
knapsack_config_t* knapsack_config_create_default(void);
void knapsack_config_destroy(knapsack_config_t** config_ptr);
void knapsack_result_destroy(knapsack_result_t** result_ptr);

#endif // KNAPSACK_OPTIMIZER_H
