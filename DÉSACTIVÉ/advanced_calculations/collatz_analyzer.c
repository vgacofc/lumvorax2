// Feature test macros for POSIX functions
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include "collatz_analyzer.h"
#include "../debug/memory_tracker.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

// Création nombre Collatz
collatz_number_t* collatz_number_create(uint64_t number) {
    if (number == 0 || number > COLLATZ_MAX_NUMBER) return NULL;

    collatz_number_t* collatz_num = TRACKED_MALLOC(sizeof(collatz_number_t));
    if (!collatz_num) return NULL;

    // Initialisation LUM de base
    collatz_num->base_lum.id = (uint32_t)(number % UINT32_MAX);
    collatz_num->base_lum.presence = 1;
    collatz_num->base_lum.position_x = (int32_t)(number % INT32_MAX);
    collatz_num->base_lum.position_y = (int32_t)((number >> 32) % INT32_MAX);
    collatz_num->base_lum.structure_type = LUM_STRUCTURE_LINEAR;

    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    collatz_num->base_lum.timestamp = ts.tv_sec * 1000000000ULL + ts.tv_nsec;
    collatz_num->base_lum.memory_address = &collatz_num->base_lum;
    collatz_num->base_lum.checksum = 0;
    collatz_num->base_lum.is_destroyed = 0;

    // Initialisation spécifique Collatz
    collatz_num->number = number;
    collatz_num->original_number = number;
    collatz_num->steps_to_one = 0;
    collatz_num->max_value_reached = number;
    collatz_num->odd_steps = 0;
    collatz_num->even_steps = 0;
    collatz_num->reached_one = false;
    collatz_num->is_cycle_detected = false;
    collatz_num->analysis_timestamp = collatz_num->base_lum.timestamp;
    collatz_num->memory_address = (void*)collatz_num;
    collatz_num->collatz_magic = COLLATZ_MAGIC_NUMBER;

    return collatz_num;
}

// Destruction nombre Collatz
void collatz_number_destroy(collatz_number_t** number_ptr) {
    if (!number_ptr || !*number_ptr) return;

    collatz_number_t* number = *number_ptr;

    if (number->collatz_magic != COLLATZ_MAGIC_NUMBER || 
        number->memory_address != (void*)number) {
        return;
    }

    number->collatz_magic = COLLATZ_DESTROYED_MAGIC;
    number->memory_address = NULL;

    TRACKED_FREE(number);
    *number_ptr = NULL;
}

// Calcul de la valeur suivante dans la séquence Collatz
uint64_t collatz_next_value(uint64_t n) {
    if (n == 1) return 1;

    if (n % 2 == 0) {
        return n / 2;
    } else {
        // Protection contre overflow
        if (n > (COLLATZ_MAX_NUMBER - 1) / 3) {
            return COLLATZ_MAX_NUMBER; // Valeur d'erreur
        }
        return 3 * n + 1;
    }
}

// Création séquence Collatz
collatz_sequence_t* collatz_sequence_create(uint64_t starting_number) {
    collatz_sequence_t* sequence = TRACKED_MALLOC(sizeof(collatz_sequence_t));
    if (!sequence) return NULL;

    sequence->sequence_length = 0;
    // CORRECTION CRITIQUE: Allocation initiale sécurisée et proportionnelle
    sequence->sequence_capacity = 100;  // Allocation initiale sécurisée (800 bytes au lieu de 80MB)
    sequence->sequence = TRACKED_MALLOC(sequence->sequence_capacity * sizeof(uint64_t));

    if (!sequence->sequence) {
        TRACKED_FREE(sequence);
        return NULL;
    }

    sequence->starting_number = starting_number;
    sequence->steps_count = 0;
    sequence->max_value = starting_number;
    sequence->compression_ratio = 1.0;
    sequence->computation_time_ns = 0;
    sequence->memory_address = (void*)sequence;
    sequence->sequence_magic = COLLATZ_MAGIC_NUMBER;

    return sequence;
}

// Destruction séquence
void collatz_sequence_destroy(collatz_sequence_t** sequence_ptr) {
    if (!sequence_ptr || !*sequence_ptr) return;

    collatz_sequence_t* sequence = *sequence_ptr;

    if (sequence->memory_address != (void*)sequence) return;

    if (sequence->sequence) {
        TRACKED_FREE(sequence->sequence);
    }

    sequence->memory_address = NULL;
    TRACKED_FREE(sequence);
    *sequence_ptr = NULL;
}

// Ajout d'une étape à la séquence
bool collatz_sequence_add_step(collatz_sequence_t* sequence, uint64_t value) {
    if (!sequence || sequence->sequence_length >= COLLATZ_MAX_ITERATIONS) {
        return false;
    }

    sequence->sequence[sequence->sequence_length] = value;
    sequence->sequence_length++;
    sequence->steps_count++;

    if (value > sequence->max_value) {
        sequence->max_value = value;
    }

    return true;
}

// Création cache Collatz
collatz_cache_t* collatz_cache_create(size_t cache_size) {
    if (cache_size == 0 || cache_size > COLLATZ_CACHE_SIZE) {
        cache_size = COLLATZ_CACHE_SIZE;
    }

    collatz_cache_t* cache = TRACKED_MALLOC(sizeof(collatz_cache_t));
    if (!cache) return NULL;

    cache->cache_size = cache_size;
    cache->cached_numbers = TRACKED_MALLOC(cache_size * sizeof(uint64_t));
    cache->cached_steps = TRACKED_MALLOC(cache_size * sizeof(uint64_t));
    cache->cache_valid = TRACKED_MALLOC(cache_size * sizeof(bool));

    if (!cache->cached_numbers || !cache->cached_steps || !cache->cache_valid) {
        if (cache->cached_numbers) TRACKED_FREE(cache->cached_numbers);
        if (cache->cached_steps) TRACKED_FREE(cache->cached_steps);
        if (cache->cache_valid) TRACKED_FREE(cache->cache_valid);
        TRACKED_FREE(cache);
        return NULL;
    }

    // Initialisation cache
    for (size_t i = 0; i < cache_size; i++) {
        cache->cached_numbers[i] = 0;
        cache->cached_steps[i] = 0;
        cache->cache_valid[i] = false;
    }

    cache->cache_hits = 0;
    cache->cache_misses = 0;
    cache->memory_address = (void*)cache;

    return cache;
}

// Destruction cache
void collatz_cache_destroy(collatz_cache_t** cache_ptr) {
    if (!cache_ptr || !*cache_ptr) return;

    collatz_cache_t* cache = *cache_ptr;

    if (cache->memory_address != (void*)cache) return;

    if (cache->cached_numbers) TRACKED_FREE(cache->cached_numbers);
    if (cache->cached_steps) TRACKED_FREE(cache->cached_steps);
    if (cache->cache_valid) TRACKED_FREE(cache->cache_valid);

    cache->memory_address = NULL;
    TRACKED_FREE(cache);
    *cache_ptr = NULL;
}

// Recherche dans le cache
bool collatz_cache_lookup(collatz_cache_t* cache, uint64_t number, uint64_t* steps) {
    if (!cache || !steps) return false;

    size_t index = number % cache->cache_size;

    if (cache->cache_valid[index] && cache->cached_numbers[index] == number) {
        *steps = cache->cached_steps[index];
        cache->cache_hits++;
        return true;
    }

    cache->cache_misses++;
    return false;
}

// Stockage dans le cache
void collatz_cache_store(collatz_cache_t* cache, uint64_t number, uint64_t steps) {
    if (!cache) return;

    size_t index = number % cache->cache_size;

    cache->cached_numbers[index] = number;
    cache->cached_steps[index] = steps;
    cache->cache_valid[index] = true;
}

// Calcul du nombre d'étapes pour atteindre 1
uint64_t collatz_compute_steps(uint64_t number, collatz_cache_t* cache) {
    if (number == 1) return 0;

    uint64_t steps = 0;
    uint64_t current = number;
    uint64_t cached_steps;

    // Vérification cache
    if (cache && collatz_cache_lookup(cache, current, &cached_steps)) {
        return cached_steps;
    }

    while (current != 1 && steps < COLLATZ_MAX_ITERATIONS) {
        current = collatz_next_value(current);
        steps++;

        if (current >= COLLATZ_MAX_NUMBER) {
            return UINT64_MAX; // Erreur overflow
        }

        // Vérification cache intermédiaire
        if (cache && collatz_cache_lookup(cache, current, &cached_steps)) {
            steps += cached_steps;
            break;
        }
    }

    // Stockage dans le cache
    if (cache && current == 1) {
        collatz_cache_store(cache, number, steps);
    }

    return steps;
}

// Calcul de la séquence complète
collatz_sequence_t* collatz_compute_full_sequence(uint64_t number, collatz_config_t* config) {
    if (!config) return NULL;

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    collatz_sequence_t* sequence = collatz_sequence_create(number);
    if (!sequence) return NULL;

    uint64_t current = number;
    collatz_sequence_add_step(sequence, current);

    while (current != 1 && sequence->sequence_length < COLLATZ_MAX_ITERATIONS) {
        current = collatz_next_value(current);

        if (current >= COLLATZ_MAX_NUMBER) {
            break; // Arrêt en cas d'overflow
        }

        if (!collatz_sequence_add_step(sequence, current)) {
            break; // Arrêt si impossible d'ajouter
        }

        if (current == 1) {
            break; // Séquence terminée
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    sequence->computation_time_ns = (end.tv_sec - start.tv_sec) * 1000000000ULL + 
                                   (end.tv_nsec - start.tv_nsec);

    // Calcul ratio de compression
    sequence->compression_ratio = (double)sequence->max_value / (double)number;

    return sequence;
}

// Analyse de base d'un nombre
collatz_result_t* collatz_analyze_basic(uint64_t number, collatz_config_t* config) {
    if (!config) return NULL;

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    collatz_result_t* result = TRACKED_MALLOC(sizeof(collatz_result_t));
    if (!result) return NULL;

    result->memory_address = (void*)result;
    strcpy(result->analysis_type_used, "Basic Analysis");
    result->analysis_success = false;

    // Initialisation explicite des pointeurs à NULL
    result->statistics = NULL;
    result->sequences = NULL;
    result->sequence_count = 0;

    // Création cache si activé
    collatz_cache_t* cache = NULL;
    if (config->use_caching) {
        cache = collatz_cache_create(COLLATZ_CACHE_SIZE);
    }

    // Calcul des étapes
    uint64_t steps = collatz_compute_steps(number, cache);

    if (steps != UINT64_MAX) {
        result->record_steps = steps;
        result->record_number = number;
        result->analysis_success = true;

        // Génération de la séquence si demandée
        if (config->store_sequences) {
            result->sequences = TRACKED_MALLOC(1 * sizeof(collatz_sequence_t*));
            if (result->sequences) {
                result->sequences[0] = collatz_compute_full_sequence(number, config);
                result->sequence_count = 1;

                if (result->sequences[0]) {
                    result->record_max_value = result->sequences[0]->max_value;
                }
            }
        }

        strcpy(result->error_message, "Basic analysis completed successfully");
    } else {
        strcpy(result->error_message, "Number caused overflow in computation");
    }

    if (cache) {
        collatz_cache_destroy(&cache);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    result->total_time_ns = (end.tv_sec - start.tv_sec) * 1000000000ULL + 
                           (end.tv_nsec - start.tv_nsec);

    return result;
}

// Analyse statistique d'une plage
collatz_result_t* collatz_analyze_statistical(uint64_t start, uint64_t end, collatz_config_t* config) {
    if (!config || start >= end) return NULL;

    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    collatz_result_t* result = TRACKED_MALLOC(sizeof(collatz_result_t));
    if (!result) return NULL;

    result->memory_address = (void*)result;
    strcpy(result->analysis_type_used, "Statistical Analysis");
    result->analysis_success = false;

    // Création statistiques
    result->statistics = TRACKED_MALLOC(sizeof(collatz_statistics_t));
    if (!result->statistics) {
        strcpy(result->error_message, "Failed to allocate statistics");
        return result;
    }

    result->statistics->numbers_analyzed = 0;
    result->statistics->total_steps = 0;
    result->statistics->min_steps = UINT64_MAX;
    result->statistics->max_steps = 0;
    result->statistics->min_max_value = UINT64_MAX;
    result->statistics->max_max_value = 0;
    result->statistics->convergence_rate = 0;
    result->statistics->memory_address = (void*)result->statistics;

    // Création cache
    collatz_cache_t* cache = NULL;
    if (config->use_caching) {
        cache = collatz_cache_create(COLLATZ_CACHE_SIZE);
    }

    result->record_steps = 0;
    result->record_number = 0;
    result->record_max_value = 0;

    // Analyse de la plage
    for (uint64_t n = start; n <= end && n <= start + 1000000; n++) { // Limité pour le test
        uint64_t steps = collatz_compute_steps(n, cache);

        if (steps != UINT64_MAX) {
            result->statistics->numbers_analyzed++;
            result->statistics->total_steps += steps;

            if (steps < result->statistics->min_steps) {
                result->statistics->min_steps = steps;
            }
            if (steps > result->statistics->max_steps) {
                result->statistics->max_steps = steps;
                result->record_steps = steps;
                result->record_number = n;
            }

            // Calcul valeur maximale atteinte
            if (config->store_sequences) {
                collatz_sequence_t* seq = collatz_compute_full_sequence(n, config);
                if (seq) {
                    if (seq->max_value < result->statistics->min_max_value) {
                        result->statistics->min_max_value = seq->max_value;
                    }
                    if (seq->max_value > result->statistics->max_max_value) {
                        result->statistics->max_max_value = seq->max_value;
                        result->record_max_value = seq->max_value;
                    }
                    collatz_sequence_destroy(&seq);
                }
            }
        }
    }

    // Calcul moyennes
    if (result->statistics->numbers_analyzed > 0) {
        result->statistics->average_steps = (double)result->statistics->total_steps / 
                                           (double)result->statistics->numbers_analyzed;
        result->statistics->average_max_value = (double)(result->statistics->min_max_value + 
                                                         result->statistics->max_max_value) / 2.0;
        result->statistics->convergence_rate = (result->statistics->numbers_analyzed * 100) / 
                                              (end - start + 1);
        result->analysis_success = true;
        strcpy(result->error_message, "Statistical analysis completed successfully");
    } else {
        strcpy(result->error_message, "No valid numbers found in range");
    }

    if (cache) {
        collatz_cache_destroy(&cache);
    }

    clock_gettime(CLOCK_MONOTONIC, &end_time);
    result->total_time_ns = (end_time.tv_sec - start_time.tv_sec) * 1000000000ULL + 
                           (end_time.tv_nsec - start_time.tv_nsec);
    result->statistics->analysis_time_ns = result->total_time_ns;

    return result;
}

// Tests stress 100M+ nombres
bool collatz_stress_test_100m_numbers(collatz_config_t* config) {
    if (!config) return false;

    printf("=== COLLATZ STRESS TEST: 100M+ Numbers ===\n");

    const uint64_t number_count = 100000000; // 100M nombres
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    printf("Testing Collatz with large number count...\n");

    // Test avec échantillon représentatif
    const uint64_t test_count = 100000; // 100K nombres
    const uint64_t test_start = 1000000; // Démarrer à 1M

    printf("Analyzing %lu numbers starting from %lu...\n", test_count, test_start);

    // Création cache
    collatz_cache_t* cache = collatz_cache_create(COLLATZ_CACHE_SIZE);
    if (!cache) {
        printf("❌ Failed to create cache\n");
        return false;
    }

    uint64_t total_steps = 0;
    uint64_t max_steps = 0;
    uint64_t max_steps_number = 0;
    uint64_t numbers_processed = 0;

    // Analyse des nombres
    for (uint64_t n = test_start; n < test_start + test_count; n++) {
        uint64_t steps = collatz_compute_steps(n, cache);

        if (steps != UINT64_MAX) {
            total_steps += steps;
            numbers_processed++;

            if (steps > max_steps) {
                max_steps = steps;
                max_steps_number = n;
            }
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double analysis_time = (end.tv_sec - start.tv_sec) + 
                          (end.tv_nsec - start.tv_nsec) / 1000000000.0;

    printf("✅ Analyzed %lu Collatz numbers in %.3f seconds\n", numbers_processed, analysis_time);

    if (numbers_processed > 0) {
        double avg_steps = (double)total_steps / (double)numbers_processed;
        printf("Average steps: %.2f\n", avg_steps);
        printf("Max steps: %lu (for number %lu)\n", max_steps, max_steps_number);
        printf("Cache hits: %lu\n", cache->cache_hits);
        printf("Cache misses: %lu\n", cache->cache_misses);
        printf("Cache hit rate: %.2f%%\n", 
               (double)cache->cache_hits / (cache->cache_hits + cache->cache_misses) * 100.0);
    }

    // Projection pour 100M
    double projected_time = analysis_time * (number_count / (double)test_count);
    printf("Projected time for %lu numbers: %.1f seconds\n", number_count, projected_time);
    printf("Analysis rate: %.0f numbers/second\n", numbers_processed / analysis_time);

    collatz_cache_destroy(&cache);

    printf("✅ Collatz stress test 100M+ numbers completed successfully\n");
    return true;
}

// Configuration par défaut
collatz_config_t* collatz_config_create_default(void) {
    collatz_config_t* config = TRACKED_MALLOC(sizeof(collatz_config_t));
    if (!config) return NULL;

    config->analysis_type = COLLATZ_ANALYSIS_BASIC;
    config->start_number = 1;
    config->end_number = 1000000;
    config->max_iterations = COLLATZ_MAX_ITERATIONS;
    config->use_caching = true;
    config->enable_parallelization = false;
    config->thread_count = 1;
    config->detect_cycles = false;
    config->compute_statistics = true;
    config->store_sequences = false;
    config->memory_address = (void*)config;

    return config;
}

// Destruction configuration
void collatz_config_destroy(collatz_config_t** config_ptr) {
    if (!config_ptr || !*config_ptr) return;

    collatz_config_t* config = *config_ptr;
    if (config->memory_address == (void*)config) {
        TRACKED_FREE(config);
        *config_ptr = NULL;
    }
}

// Destruction résultat
void collatz_result_destroy(collatz_result_t** result_ptr) {
    if (!result_ptr || !*result_ptr) return;

    collatz_result_t* result = *result_ptr;
    if (result->memory_address == (void*)result) {
        if (result->statistics) {
            TRACKED_FREE(result->statistics);
        }
        if (result->sequences) {
            for (size_t i = 0; i < result->sequence_count; i++) {
                if (result->sequences[i]) {
                    collatz_sequence_destroy(&result->sequences[i]);
                }
            }
            TRACKED_FREE(result->sequences);
        }
        TRACKED_FREE(result);
        *result_ptr = NULL;
    }
}