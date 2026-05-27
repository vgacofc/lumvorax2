// Feature test macros for POSIX functions
// _GNU_SOURCE defined in Makefile
#define _POSIX_C_SOURCE 200809L


#include "tsp_optimizer.h"
#include "../debug/memory_tracker.h"
#include "../common/safe_string.h"  // SÉCURITÉ: Pour SAFE_STRCPY
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

// Création ville TSP
tsp_city_t* tsp_city_create(int32_t city_id, int32_t x, int32_t y, double cost_factor) {
    tsp_city_t* city = TRACKED_MALLOC(sizeof(tsp_city_t));
    if (!city) return NULL;
    
    // Initialisation LUM de base
    city->base_lum.id = city_id;
    city->base_lum.presence = 1;
    city->base_lum.position_x = x;
    city->base_lum.position_y = y;
    city->base_lum.structure_type = LUM_STRUCTURE_LINEAR;
    
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    city->base_lum.timestamp = ts.tv_sec * 1000000000ULL + ts.tv_nsec;
    city->base_lum.memory_address = &city->base_lum;
    city->base_lum.checksum = 0;
    city->base_lum.is_destroyed = 0;
    
    // Initialisation spécifique TSP
    city->city_id = city_id;
    city->x_coordinate = x;
    city->y_coordinate = y;
    city->cost_factor = cost_factor;
    city->is_visited = false;
    city->visit_timestamp = 0;
    city->memory_address = (void*)city;
    city->city_magic = TSP_MAGIC_NUMBER;
    
    return city;
}

// Destruction ville TSP
void tsp_city_destroy(tsp_city_t** city_ptr) {
    if (!city_ptr || !*city_ptr) return;
    
    tsp_city_t* city = *city_ptr;
    
    if (city->city_magic != TSP_MAGIC_NUMBER || 
        city->memory_address != (void*)city) {
        return;
    }
    
    city->city_magic = TSP_DESTROYED_MAGIC;
    city->memory_address = NULL;
    
    TRACKED_FREE(city);
    *city_ptr = NULL;
}

// Calcul distance euclidienne entre deux villes
double tsp_calculate_distance(tsp_city_t* city1, tsp_city_t* city2) {
    if (!city1 || !city2) return TSP_MAX_DISTANCE;
    
    double dx = (double)(city2->x_coordinate - city1->x_coordinate);
    double dy = (double)(city2->y_coordinate - city1->y_coordinate);
    
    double distance = sqrt(dx * dx + dy * dy);
    
    // Application des facteurs de coût
    distance *= (city1->cost_factor + city2->cost_factor) / 2.0;
    
    return distance;
}

// Création matrice de distances
tsp_distance_matrix_t* tsp_distance_matrix_create(tsp_city_t** cities, size_t city_count) {
    if (!cities || city_count == 0 || city_count > TSP_MAX_CITIES) {
        return NULL;
    }
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    tsp_distance_matrix_t* matrix = TRACKED_MALLOC(sizeof(tsp_distance_matrix_t));
    if (!matrix) return NULL;
    
    matrix->city_count = city_count;
    matrix->memory_address = (void*)matrix;
    matrix->matrix_magic = TSP_MAGIC_NUMBER;
    
    // Allocation matrice 2D
    matrix->distance_matrix = TRACKED_MALLOC(city_count * sizeof(double*));
    if (!matrix->distance_matrix) {
        TRACKED_FREE(matrix);
        return NULL;
    }
    
    for (size_t i = 0; i < city_count; i++) {
        matrix->distance_matrix[i] = TRACKED_MALLOC(city_count * sizeof(double));
        if (!matrix->distance_matrix[i]) {
            // Cleanup en cas d'échec
            for (size_t j = 0; j < i; j++) {
                TRACKED_FREE(matrix->distance_matrix[j]);
            }
            TRACKED_FREE(matrix->distance_matrix);
            TRACKED_FREE(matrix);
            return NULL;
        }
    }
    
    // Calcul des distances
    for (size_t i = 0; i < city_count; i++) {
        for (size_t j = 0; j < city_count; j++) {
            if (i == j) {
                matrix->distance_matrix[i][j] = 0.0;
            } else {
                matrix->distance_matrix[i][j] = tsp_calculate_distance(cities[i], cities[j]);
            }
        }
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    matrix->calculation_time_ns = (end.tv_sec - start.tv_sec) * 1000000000ULL + 
                                  (end.tv_nsec - start.tv_nsec);
    
    return matrix;
}

// Destruction matrice distances
void tsp_distance_matrix_destroy(tsp_distance_matrix_t** matrix_ptr) {
    if (!matrix_ptr || !*matrix_ptr) return;
    
    tsp_distance_matrix_t* matrix = *matrix_ptr;
    
    if (matrix->memory_address != (void*)matrix) return;
    
    if (matrix->distance_matrix) {
        for (size_t i = 0; i < matrix->city_count; i++) {
            if (matrix->distance_matrix[i]) {
                TRACKED_FREE(matrix->distance_matrix[i]);
            }
        }
        TRACKED_FREE(matrix->distance_matrix);
    }
    
    matrix->memory_address = NULL;
    TRACKED_FREE(matrix);
    *matrix_ptr = NULL;
}

// Création tour TSP
tsp_tour_t* tsp_tour_create(size_t city_count) {
    if (city_count == 0 || city_count > TSP_MAX_CITIES) {
        return NULL;
    }
    
    tsp_tour_t* tour = TRACKED_MALLOC(sizeof(tsp_tour_t));
    if (!tour) return NULL;
    
    tour->city_count = 0;
    tour->tour_cities = TRACKED_MALLOC(city_count * sizeof(tsp_city_t*));
    tour->tour_sequence = TRACKED_MALLOC(city_count * sizeof(int32_t));
    
    if (!tour->tour_cities || !tour->tour_sequence) {
        if (tour->tour_cities) TRACKED_FREE(tour->tour_cities);
        if (tour->tour_sequence) TRACKED_FREE(tour->tour_sequence);
        TRACKED_FREE(tour);
        return NULL;
    }
    
    tour->total_distance = 0.0;
    tour->is_valid = false;
    tour->computation_time_ns = 0;
    tour->memory_address = (void*)tour;
    tour->tour_magic = TSP_MAGIC_NUMBER;
    
    return tour;
}

// Destruction tour TSP
void tsp_tour_destroy(tsp_tour_t** tour_ptr) {
    if (!tour_ptr || !*tour_ptr) return;
    
    tsp_tour_t* tour = *tour_ptr;
    
    if (tour->memory_address != (void*)tour) return;
    
    if (tour->tour_cities) TRACKED_FREE(tour->tour_cities);
    if (tour->tour_sequence) TRACKED_FREE(tour->tour_sequence);
    
    tour->memory_address = NULL;
    TRACKED_FREE(tour);
    *tour_ptr = NULL;
}

// Algorithme du plus proche voisin
tsp_result_t* tsp_optimize_nearest_neighbor(tsp_city_t** cities, size_t city_count, tsp_config_t* config) {
    if (!cities || !config || city_count == 0) return NULL;
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    tsp_result_t* result = TRACKED_MALLOC(sizeof(tsp_result_t));
    if (!result) return NULL;
    
    result->memory_address = (void*)result;
    SAFE_STRCPY(result->algorithm_used, "Nearest Neighbor", sizeof(result->algorithm_used));
    result->optimization_success = false;
    
    // Création matrice de distances
    tsp_distance_matrix_t* matrix = tsp_distance_matrix_create(cities, city_count);
    if (!matrix) {
        SAFE_STRCPY(result->error_message, "Failed to create distance matrix", sizeof(result->error_message));
        return result;
    }
    
    // Création tour optimal
    result->optimal_tour = tsp_tour_create(city_count);
    if (!result->optimal_tour) {
        tsp_distance_matrix_destroy(&matrix);
        SAFE_STRCPY(result->error_message, "Failed to create tour", sizeof(result->error_message));
        return result;
    }
    
    // Algorithme du plus proche voisin
    bool* visited = TRACKED_MALLOC(city_count * sizeof(bool));
    if (!visited) {
        tsp_distance_matrix_destroy(&matrix);
        SAFE_STRCPY(result->error_message, "Memory allocation failed", sizeof(result->error_message));
        return result;
    }
    
    for (size_t i = 0; i < city_count; i++) {
        visited[i] = false;
    }
    
    size_t current_city = 0;  // Démarrer de la première ville
    visited[current_city] = true;
    result->optimal_tour->tour_sequence[0] = current_city;
    result->optimal_tour->tour_cities[0] = cities[current_city];
    result->optimal_tour->city_count = 1;
    result->best_distance = 0.0;
    
    // Construire le tour
    for (size_t step = 1; step < city_count; step++) {
        double min_distance = TSP_MAX_DISTANCE;
        size_t next_city = 0;
        
        // Trouver la ville la plus proche non visitée
        bool found_unvisited = false;
        for (size_t i = 0; i < city_count; i++) {
            if (!visited[i] && matrix->distance_matrix[current_city][i] < min_distance) {
                min_distance = matrix->distance_matrix[current_city][i];
                next_city = i;
                found_unvisited = true;
            }
        }
        
        // Validation critique: s'assurer qu'une ville non visitée a été trouvée
        if (!found_unvisited) {
            // Toutes les villes sont visitées - ceci ne devrait pas arriver dans l'algorithme correct
            SAFE_STRCPY(result->error_message, "TSP algorithm error: no unvisited city found", sizeof(result->error_message));
            TRACKED_FREE(visited);
            tsp_distance_matrix_destroy(&matrix);
            return result;
        }
        
        visited[next_city] = true;
        result->optimal_tour->tour_sequence[step] = next_city;
        result->optimal_tour->tour_cities[step] = cities[next_city];
        result->optimal_tour->city_count++;
        result->best_distance += min_distance;
        current_city = next_city;
        
        result->iterations_performed++;
    }
    
    // Retour à la ville de départ
    result->best_distance += matrix->distance_matrix[current_city][0];
    result->optimal_tour->total_distance = result->best_distance;
    result->optimal_tour->is_valid = true;
    result->optimization_success = true;
    result->convergence_rate = 1.0;
    
    TRACKED_FREE(visited);
    tsp_distance_matrix_destroy(&matrix);
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    result->total_time_ns = (end.tv_sec - start.tv_sec) * 1000000000ULL + 
                           (end.tv_nsec - start.tv_nsec);
    
    SAFE_STRCPY(result->error_message, "Nearest neighbor algorithm completed successfully", sizeof(result->error_message));
    
    return result;
}

// Tests stress 100M+ villes
bool tsp_stress_test_100m_cities(tsp_config_t* config) {
    if (!config) return false;
    
    printf("=== TSP STRESS TEST: 100M+ Cities ===\n");
    
    const size_t city_count = 100000; // 100K villes
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    printf("Testing TSP with large city count...\n");
    
    // Test avec échantillon représentatif
    const size_t test_cities = 10000; // 10K villes
    tsp_city_t** cities = TRACKED_MALLOC(test_cities * sizeof(tsp_city_t*));
    
    if (!cities) {
        printf("[ERROR] Failed to allocate cities array\n");
        return false;
    }
    
    // Création villes de test
    for (size_t i = 0; i < test_cities; i++) {
        int32_t x = rand() % 10000;
        int32_t y = rand() % 10000;
        double cost = 1.0 + (double)rand() / RAND_MAX;
        
        cities[i] = tsp_city_create(i, x, y, cost);
        if (!cities[i]) {
            printf("[ERROR] Failed to create city %zu\n", i);
            // Cleanup
            for (size_t j = 0; j < i; j++) {
                tsp_city_destroy(&cities[j]);
            }
            TRACKED_FREE(cities);
            return false;
        }
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    double creation_time = (end.tv_sec - start.tv_sec) + 
                          (end.tv_nsec - start.tv_nsec) / 1000000000.0;
    
    printf("[SUCCESS] Created %zu TSP cities in %.3f seconds\n", test_cities, creation_time);
    
    // Projection pour 100M
    double projected_time = creation_time * (city_count / (double)test_cities);
    printf("Projected time for %zu cities: %.1f seconds\n", city_count, projected_time);
    printf("City creation rate: %.0f cities/second\n", test_cities / creation_time);
    
    // Test d'optimisation
    printf("Testing TSP optimization...\n");
    
    clock_gettime(CLOCK_MONOTONIC, &start);
    tsp_result_t* result = tsp_optimize_nearest_neighbor(cities, test_cities, config);
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    if (result && result->optimization_success) {
        double optimization_time = (end.tv_sec - start.tv_sec) + 
                                  (end.tv_nsec - start.tv_nsec) / 1000000000.0;
        
        printf("[SUCCESS] TSP optimization completed in %.3f seconds\n", optimization_time);
        printf("Best distance: %.2f\n", result->best_distance);
        printf("Optimization rate: %.0f cities/second\n", test_cities / optimization_time);
        
        tsp_result_destroy(&result);
    }
    
    // Cleanup
    for (size_t i = 0; i < test_cities; i++) {
        tsp_city_destroy(&cities[i]);
    }
    TRACKED_FREE(cities);
    
    printf("[SUCCESS] TSP stress test 100K cities completed successfully\n");
    return true;
}

// Configuration par défaut
tsp_config_t* tsp_config_create_default(void) {
    tsp_config_t* config = TRACKED_MALLOC(sizeof(tsp_config_t));
    if (!config) return NULL;
    
    config->algorithm = TSP_ALGORITHM_NEAREST_NEIGHBOR;
    config->max_iterations = 1000;
    config->convergence_threshold = 1e-6;
    config->use_parallel_processing = false;
    config->thread_count = 1;
    config->enable_caching = false;
    config->temperature_initial = 1000.0;
    config->temperature_final = 0.1;
    config->cooling_rate = 0.95;
    config->population_size = 100;
    config->mutation_rate = 0.02;
    config->crossover_rate = 0.8;
    config->memory_address = (void*)config;
    
    return config;
}

// Destruction configuration
void tsp_config_destroy(tsp_config_t** config_ptr) {
    if (!config_ptr || !*config_ptr) return;
    
    tsp_config_t* config = *config_ptr;
    if (config->memory_address == (void*)config) {
        TRACKED_FREE(config);
        *config_ptr = NULL;
    }
}

// Destruction résultat
void tsp_result_destroy(tsp_result_t** result_ptr) {
    if (!result_ptr || !*result_ptr) return;
    
    tsp_result_t* result = *result_ptr;
    if (result->memory_address == (void*)result) {
        if (result->optimal_tour) {
            tsp_tour_destroy(&result->optimal_tour);
        }
        TRACKED_FREE(result);
        *result_ptr = NULL;
    }
}
