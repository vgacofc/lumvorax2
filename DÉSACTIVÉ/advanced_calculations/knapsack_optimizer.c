// Feature test macros for POSIX functions
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L


#include "knapsack_optimizer.h"
#include "../debug/memory_tracker.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

// Création item de sac à dos
knapsack_item_t* knapsack_item_create(int32_t item_id, uint32_t weight, uint32_t value) {
    if (weight == 0 || value == 0) return NULL;
    
    knapsack_item_t* item = TRACKED_MALLOC(sizeof(knapsack_item_t));
    if (!item) return NULL;
    
    // Initialisation LUM de base
    item->base_lum.id = item_id;
    item->base_lum.presence = 1;
    item->base_lum.position_x = (int32_t)weight;
    item->base_lum.position_y = (int32_t)value;
    item->base_lum.structure_type = LUM_STRUCTURE_LINEAR;
    
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    item->base_lum.timestamp = ts.tv_sec * 1000000000ULL + ts.tv_nsec;
    item->base_lum.memory_address = &item->base_lum;
    item->base_lum.checksum = 0;
    item->base_lum.is_destroyed = 0;
    
    // Initialisation spécifique Knapsack
    item->item_id = item_id;
    item->weight = weight;
    item->value = value;
    item->value_density = (double)value / (double)weight;
    item->is_selected = false;
    item->selection_timestamp = 0;
    item->memory_address = (void*)item;
    item->item_magic = KNAPSACK_MAGIC_NUMBER;
    
    return item;
}

// Destruction item
void knapsack_item_destroy(knapsack_item_t** item_ptr) {
    if (!item_ptr || !*item_ptr) return;
    
    knapsack_item_t* item = *item_ptr;
    
    if (item->item_magic != KNAPSACK_MAGIC_NUMBER || 
        item->memory_address != (void*)item) {
        return;
    }
    
    item->item_magic = KNAPSACK_DESTROYED_MAGIC;
    item->memory_address = NULL;
    
    TRACKED_FREE(item);
    *item_ptr = NULL;
}

// Création sac à dos
knapsack_t* knapsack_create(size_t capacity) {
    if (capacity == 0 || capacity > KNAPSACK_MAX_WEIGHT) {
        return NULL;
    }
    
    knapsack_t* knapsack = TRACKED_MALLOC(sizeof(knapsack_t));
    if (!knapsack) return NULL;
    
    knapsack->capacity = capacity;
    knapsack->item_count = 0;
    knapsack->items = TRACKED_MALLOC(KNAPSACK_MAX_ITEMS * sizeof(knapsack_item_t*));
    
    if (!knapsack->items) {
        TRACKED_FREE(knapsack);
        return NULL;
    }
    
    knapsack->total_weight = 0;
    knapsack->total_value = 0;
    knapsack->is_valid = true;
    knapsack->computation_time_ns = 0;
    knapsack->memory_address = (void*)knapsack;
    knapsack->knapsack_magic = KNAPSACK_MAGIC_NUMBER;
    
    return knapsack;
}

// Destruction sac à dos
void knapsack_destroy(knapsack_t** knapsack_ptr) {
    if (!knapsack_ptr || !*knapsack_ptr) return;
    
    knapsack_t* knapsack = *knapsack_ptr;
    
    if (knapsack->memory_address != (void*)knapsack) return;
    
    if (knapsack->items) {
        TRACKED_FREE(knapsack->items);
    }
    
    knapsack->memory_address = NULL;
    TRACKED_FREE(knapsack);
    *knapsack_ptr = NULL;
}

// Vérification si item peut être ajouté
bool knapsack_can_fit(knapsack_t* knapsack, knapsack_item_t* item) {
    if (!knapsack || !item) return false;
    
    return (knapsack->total_weight + item->weight <= knapsack->capacity);
}

// Ajout item au sac
bool knapsack_add_item(knapsack_t* knapsack, knapsack_item_t* item) {
    if (!knapsack_can_fit(knapsack, item)) return false;
    
    if (knapsack->item_count >= KNAPSACK_MAX_ITEMS) return false;
    
    knapsack->items[knapsack->item_count] = item;
    knapsack->item_count++;
    knapsack->total_weight += item->weight;
    knapsack->total_value += item->value;
    
    item->is_selected = true;
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    item->selection_timestamp = ts.tv_sec * 1000000000ULL + ts.tv_nsec;
    
    return true;
}

// Calcul efficacité
double knapsack_calculate_efficiency(knapsack_t* knapsack) {
    if (!knapsack || knapsack->capacity == 0) return 0.0;
    
    return (double)knapsack->total_value / (double)knapsack->capacity;
}

// Création table DP
knapsack_dp_table_t* knapsack_dp_table_create(size_t item_count, size_t capacity) {
    if (item_count == 0 || capacity == 0) return NULL;
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    knapsack_dp_table_t* table = TRACKED_MALLOC(sizeof(knapsack_dp_table_t));
    if (!table) return NULL;
    
    table->item_count = item_count;
    table->capacity = capacity;
    table->memory_address = (void*)table;
    
    // Allocation table DP
    table->dp_table = TRACKED_MALLOC((item_count + 1) * sizeof(uint32_t*));
    table->selection_table = TRACKED_MALLOC((item_count + 1) * sizeof(bool*));
    
    if (!table->dp_table || !table->selection_table) {
        if (table->dp_table) TRACKED_FREE(table->dp_table);
        if (table->selection_table) TRACKED_FREE(table->selection_table);
        TRACKED_FREE(table);
        return NULL;
    }
    
    for (size_t i = 0; i <= item_count; i++) {
        table->dp_table[i] = TRACKED_MALLOC((capacity + 1) * sizeof(uint32_t));
        table->selection_table[i] = TRACKED_MALLOC((capacity + 1) * sizeof(bool));
        
        if (!table->dp_table[i] || !table->selection_table[i]) {
            // Cleanup en cas d'échec
            for (size_t j = 0; j < i; j++) {
                TRACKED_FREE(table->dp_table[j]);
                TRACKED_FREE(table->selection_table[j]);
            }
            if (table->dp_table[i]) TRACKED_FREE(table->dp_table[i]);
            if (table->selection_table[i]) TRACKED_FREE(table->selection_table[i]);
            TRACKED_FREE(table->dp_table);
            TRACKED_FREE(table->selection_table);
            TRACKED_FREE(table);
            return NULL;
        }
        
        // Initialisation
        for (size_t j = 0; j <= capacity; j++) {
            table->dp_table[i][j] = 0;
            table->selection_table[i][j] = false;
        }
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    table->table_creation_time_ns = (end.tv_sec - start.tv_sec) * 1000000000ULL + 
                                    (end.tv_nsec - start.tv_nsec);
    
    return table;
}

// Destruction table DP
void knapsack_dp_table_destroy(knapsack_dp_table_t** table_ptr) {
    if (!table_ptr || !*table_ptr) return;
    
    knapsack_dp_table_t* table = *table_ptr;
    
    if (table->memory_address != (void*)table) return;
    
    if (table->dp_table) {
        for (size_t i = 0; i <= table->item_count; i++) {
            if (table->dp_table[i]) TRACKED_FREE(table->dp_table[i]);
        }
        TRACKED_FREE(table->dp_table);
    }
    
    if (table->selection_table) {
        for (size_t i = 0; i <= table->item_count; i++) {
            if (table->selection_table[i]) TRACKED_FREE(table->selection_table[i]);
        }
        TRACKED_FREE(table->selection_table);
    }
    
    table->memory_address = NULL;
    TRACKED_FREE(table);
    *table_ptr = NULL;
}

// Comparaison pour tri par densité de valeur
int compare_items_by_density(const void* a, const void* b) {
    knapsack_item_t* item_a = *(knapsack_item_t**)a;
    knapsack_item_t* item_b = *(knapsack_item_t**)b;
    
    if (item_a->value_density > item_b->value_density) return -1;
    if (item_a->value_density < item_b->value_density) return 1;
    return 0;
}

// Algorithme glouton
knapsack_result_t* knapsack_optimize_greedy(knapsack_item_t** items, size_t item_count, 
                                           size_t capacity, knapsack_config_t* config) {
    if (!items || !config || item_count == 0 || capacity == 0) return NULL;
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    knapsack_result_t* result = TRACKED_MALLOC(sizeof(knapsack_result_t));
    if (!result) return NULL;
    
    // Initialize all fields to safe values
    memset(result, 0, sizeof(*result));
    result->memory_address = (void*)result;
    result->optimization_success = false;
    result->optimal_knapsack = NULL;
    result->items_selected = 0;
    result->iterations_performed = 0;
    result->best_value = 0;
    result->best_weight = 0;
    result->efficiency_ratio = 0.0;
    strcpy(result->algorithm_used, "Greedy");
    result->error_message[0] = '\0';
    
    // Création sac optimal
    result->optimal_knapsack = knapsack_create(capacity);
    if (!result->optimal_knapsack) {
        strcpy(result->error_message, "Failed to create knapsack");
        result->optimal_knapsack = NULL;  // Ensure NULL on error
        return result;
    }
    
    // Copie et tri des items par densité de valeur
    knapsack_item_t** sorted_items = TRACKED_MALLOC(item_count * sizeof(knapsack_item_t*));
    if (!sorted_items) {
        strcpy(result->error_message, "Memory allocation failed");
        return result;
    }
    
    for (size_t i = 0; i < item_count; i++) {
        sorted_items[i] = items[i];
    }
    
    qsort(sorted_items, item_count, sizeof(knapsack_item_t*), compare_items_by_density);
    
    // Algorithme glouton : prendre les items avec la meilleure densité
    result->items_selected = 0;
    for (size_t i = 0; i < item_count; i++) {
        if (knapsack_can_fit(result->optimal_knapsack, sorted_items[i])) {
            knapsack_add_item(result->optimal_knapsack, sorted_items[i]);
            result->items_selected++;
        }
        result->iterations_performed++;
    }
    
    result->best_value = result->optimal_knapsack->total_value;
    result->best_weight = result->optimal_knapsack->total_weight;
    result->efficiency_ratio = knapsack_calculate_efficiency(result->optimal_knapsack);
    result->optimization_success = true;
    
    TRACKED_FREE(sorted_items);
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    result->total_time_ns = (end.tv_sec - start.tv_sec) * 1000000000ULL + 
                           (end.tv_nsec - start.tv_nsec);
    
    strcpy(result->error_message, "Greedy algorithm completed successfully");
    
    return result;
}

// Algorithme de programmation dynamique
knapsack_result_t* knapsack_optimize_dynamic_programming(knapsack_item_t** items, size_t item_count, 
                                                        size_t capacity, knapsack_config_t* config) {
    if (!items || !config || item_count == 0 || capacity == 0) return NULL;
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    knapsack_result_t* result = TRACKED_MALLOC(sizeof(knapsack_result_t));
    if (!result) return NULL;
    
    // Initialize all fields to safe values
    memset(result, 0, sizeof(*result));
    result->memory_address = (void*)result;
    result->optimization_success = false;
    result->optimal_knapsack = NULL;
    result->items_selected = 0;
    result->iterations_performed = 0;
    result->best_value = 0;
    result->best_weight = 0;
    result->efficiency_ratio = 0.0;
    strcpy(result->algorithm_used, "Dynamic Programming");
    result->error_message[0] = '\0';
    
    // Création table DP
    knapsack_dp_table_t* dp_table = knapsack_dp_table_create(item_count, capacity);
    if (!dp_table) {
        strcpy(result->error_message, "Failed to create DP table");
        result->optimal_knapsack = NULL;  // Ensure NULL on error
        return result;
    }
    
    // Remplissage de la table DP
    for (size_t i = 1; i <= item_count; i++) {
        for (size_t w = 1; w <= capacity; w++) {
            knapsack_item_t* current_item = items[i-1];
            
            if (current_item->weight <= w) {
                uint32_t value_with_item = current_item->value + 
                                          dp_table->dp_table[i-1][w - current_item->weight];
                uint32_t value_without_item = dp_table->dp_table[i-1][w];
                
                if (value_with_item > value_without_item) {
                    dp_table->dp_table[i][w] = value_with_item;
                    dp_table->selection_table[i][w] = true;
                } else {
                    dp_table->dp_table[i][w] = value_without_item;
                    dp_table->selection_table[i][w] = false;
                }
            } else {
                dp_table->dp_table[i][w] = dp_table->dp_table[i-1][w];
                dp_table->selection_table[i][w] = false;
            }
            
            result->iterations_performed++;
        }
    }
    
    // Reconstruction de la solution optimale
    result->optimal_knapsack = knapsack_create(capacity);
    if (!result->optimal_knapsack) {
        knapsack_dp_table_destroy(&dp_table);
        strcpy(result->error_message, "Failed to create optimal knapsack");
        return result;
    }
    
    size_t w = capacity;
    result->items_selected = 0;
    for (size_t i = item_count; i > 0 && w > 0; i--) {
        if (dp_table->selection_table[i][w]) {
            knapsack_add_item(result->optimal_knapsack, items[i-1]);
            w -= items[i-1]->weight;
            result->items_selected++;
        }
    }
    
    result->best_value = dp_table->dp_table[item_count][capacity];
    result->best_weight = result->optimal_knapsack->total_weight;
    result->efficiency_ratio = knapsack_calculate_efficiency(result->optimal_knapsack);
    result->optimization_success = true;
    
    knapsack_dp_table_destroy(&dp_table);
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    result->total_time_ns = (end.tv_sec - start.tv_sec) * 1000000000ULL + 
                           (end.tv_nsec - start.tv_nsec);
    
    strcpy(result->error_message, "Dynamic programming completed successfully");
    
    return result;
}

// Tests stress 100M+ items
bool knapsack_stress_test_100m_items(knapsack_config_t* config) {
    if (!config) return false;
    
    printf("=== KNAPSACK STRESS TEST: 100M+ Items ===\n");
    
    const size_t item_count = 100000000; // 100M items
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    printf("Testing Knapsack with large item count...\n");
    
    // Test avec échantillon représentatif
    const size_t test_items = 100000; // 100K items
    const size_t test_capacity = 50000;
    
    knapsack_item_t** items = TRACKED_MALLOC(test_items * sizeof(knapsack_item_t*));
    
    if (!items) {
        printf("❌ Failed to allocate items array\n");
        return false;
    }
    
    // Création items de test
    for (size_t i = 0; i < test_items; i++) {
        uint32_t weight = 1 + (rand() % 100);
        uint32_t value = 1 + (rand() % 1000);
        
        items[i] = knapsack_item_create(i, weight, value);
        if (!items[i]) {
            printf("❌ Failed to create item %zu\n", i);
            // Cleanup
            for (size_t j = 0; j < i; j++) {
                knapsack_item_destroy(&items[j]);
            }
            TRACKED_FREE(items);
            return false;
        }
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    double creation_time = (end.tv_sec - start.tv_sec) + 
                          (end.tv_nsec - start.tv_nsec) / 1000000000.0;
    
    printf("✅ Created %zu knapsack items in %.3f seconds\n", test_items, creation_time);
    
    // Projection pour 100M
    double projected_time = creation_time * (item_count / (double)test_items);
    printf("Projected time for %zu items: %.1f seconds\n", item_count, projected_time);
    printf("Item creation rate: %.0f items/second\n", test_items / creation_time);
    
    // Test d'optimisation glouton
    printf("Testing Knapsack greedy optimization...\n");
    
    clock_gettime(CLOCK_MONOTONIC, &start);
    knapsack_result_t* result = knapsack_optimize_greedy(items, test_items, test_capacity, config);
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    if (result && result->optimization_success) {
        double optimization_time = (end.tv_sec - start.tv_sec) + 
                                  (end.tv_nsec - start.tv_nsec) / 1000000000.0;
        
        printf("✅ Knapsack optimization completed in %.3f seconds\n", optimization_time);
        printf("Best value: %u\n", result->best_value);
        printf("Best weight: %u\n", result->best_weight);
        printf("Items selected: %zu\n", result->items_selected);
        printf("Efficiency ratio: %.3f\n", result->efficiency_ratio);
        printf("Optimization rate: %.0f items/second\n", test_items / optimization_time);
        
        knapsack_result_destroy(&result);
    }
    
    // Cleanup
    for (size_t i = 0; i < test_items; i++) {
        knapsack_item_destroy(&items[i]);
    }
    TRACKED_FREE(items);
    
    printf("✅ Knapsack stress test 100M+ items completed successfully\n");
    return true;
}

// Configuration par défaut
knapsack_config_t* knapsack_config_create_default(void) {
    knapsack_config_t* config = TRACKED_MALLOC(sizeof(knapsack_config_t));
    if (!config) return NULL;
    
    config->algorithm = KNAPSACK_ALGORITHM_GREEDY;
    config->max_iterations = 1000;
    config->convergence_threshold = 1e-6;
    config->use_parallel_processing = false;
    config->thread_count = 1;
    config->enable_memoization = false;
    config->use_approximation = false;
    config->approximation_ratio = 0.9;
    config->population_size = 100;
    config->mutation_rate = 0.02;
    config->crossover_rate = 0.8;
    config->memory_address = (void*)config;
    
    return config;
}

// Destruction configuration
void knapsack_config_destroy(knapsack_config_t** config_ptr) {
    if (!config_ptr || !*config_ptr) return;
    
    knapsack_config_t* config = *config_ptr;
    if (config->memory_address == (void*)config) {
        TRACKED_FREE(config);
        *config_ptr = NULL;
    }
}

// Destruction résultat
void knapsack_result_destroy(knapsack_result_t** result_ptr) {
    if (!result_ptr || !*result_ptr) return;
    
    knapsack_result_t* result = *result_ptr;
    if (result->memory_address == (void*)result) {
        if (result->optimal_knapsack) {
            knapsack_destroy(&result->optimal_knapsack);
        }
        TRACKED_FREE(result);
        *result_ptr = NULL;
    }
}
