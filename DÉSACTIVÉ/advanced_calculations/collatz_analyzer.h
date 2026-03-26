
#ifndef COLLATZ_ANALYZER_H
#define COLLATZ_ANALYZER_H

#include "../lum/lum_core.h"
#include "../debug/memory_tracker.h"
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

// Constantes Collatz (CORRECTION CRITIQUE - valeurs cohérentes et sécurisées)
#define COLLATZ_MAX_ITERATIONS 10000       // Compromis sécurité/performance
#define COLLATZ_MAX_NUMBER 1000000ULL      // Augmenté mais sécurisé
#define COLLATZ_MAX_SEQUENCE_LENGTH 10000  // NOUVELLE: limite séquence
#define COLLATZ_MAGIC_NUMBER 0x434F4C5A    // Conforme STANDARD_NAMES.md
#define COLLATZ_DESTROYED_MAGIC 0xDEADBEEF // Conforme STANDARD_NAMES.md  
#define COLLATZ_CACHE_SIZE 10000           // Augmenté mais raisonnable

// Types d'analyses Collatz
typedef enum {
    COLLATZ_ANALYSIS_BASIC,
    COLLATZ_ANALYSIS_STATISTICAL,
    COLLATZ_ANALYSIS_PATTERN_DETECTION,
    COLLATZ_ANALYSIS_PARALLEL_BATCH,
    COLLATZ_ANALYSIS_CONVERGENCE_STUDY,
    COLLATZ_ANALYSIS_RECORD_BREAKING
} collatz_analysis_e;

// Nombre Collatz basé sur LUM
typedef struct {
    lum_t base_lum;              // LUM de base
    uint64_t number;             // Nombre analysé
    uint64_t original_number;    // Nombre de départ
    uint64_t steps_to_one;       // Nombre d'étapes pour atteindre 1
    uint64_t max_value_reached;  // Valeur maximale atteinte
    uint64_t odd_steps;          // Nombre d'étapes impaires
    uint64_t even_steps;         // Nombre d'étapes paires
    bool reached_one;            // A atteint 1
    bool is_cycle_detected;      // Cycle détecté
    uint64_t analysis_timestamp; // Timestamp d'analyse
    void* memory_address;        // Protection double-free
    uint32_t collatz_magic;      // Magic number protection
} collatz_number_t;

// Séquence Collatz
typedef struct {
    size_t sequence_length;
    size_t sequence_capacity;  // Nouvelle capacité allouée
    uint64_t* sequence;
    uint64_t starting_number;
    uint64_t steps_count;
    uint64_t max_value;
    double compression_ratio;
    uint64_t computation_time_ns;
    void* memory_address;
    uint32_t sequence_magic;
} collatz_sequence_t;

// Statistiques Collatz
typedef struct {
    uint64_t numbers_analyzed;
    uint64_t total_steps;
    uint64_t min_steps;
    uint64_t max_steps;
    double average_steps;
    uint64_t min_max_value;
    uint64_t max_max_value;
    double average_max_value;
    uint64_t convergence_rate;
    uint64_t analysis_time_ns;
    void* memory_address;
} collatz_statistics_t;

// Cache Collatz pour mémorisation
typedef struct {
    size_t cache_size;
    uint64_t* cached_numbers;
    uint64_t* cached_steps;
    bool* cache_valid;
    uint64_t cache_hits;
    uint64_t cache_misses;
    void* memory_address;
} collatz_cache_t;

// Configuration Collatz
typedef struct {
    collatz_analysis_e analysis_type;
    uint64_t start_number;
    uint64_t end_number;
    uint64_t max_iterations;
    bool use_caching;
    bool enable_parallelization;
    size_t thread_count;
    bool detect_cycles;
    bool compute_statistics;
    bool store_sequences;
    void* memory_address;
} collatz_config_t;

// Résultat d'analyse Collatz
typedef struct {
    collatz_statistics_t* statistics;
    collatz_sequence_t** sequences;
    size_t sequence_count;
    uint64_t record_steps;
    uint64_t record_number;
    uint64_t record_max_value;
    bool analysis_success;
    uint64_t total_time_ns;
    char analysis_type_used[64];
    char error_message[256];
    void* memory_address;
} collatz_result_t;

// Fonctions principales Collatz
collatz_number_t* collatz_number_create(uint64_t number);
void collatz_number_destroy(collatz_number_t** number_ptr);

collatz_sequence_t* collatz_sequence_create(uint64_t starting_number);
void collatz_sequence_destroy(collatz_sequence_t** sequence_ptr);
bool collatz_sequence_add_step(collatz_sequence_t* sequence, uint64_t value);

collatz_cache_t* collatz_cache_create(size_t cache_size);
void collatz_cache_destroy(collatz_cache_t** cache_ptr);
bool collatz_cache_lookup(collatz_cache_t* cache, uint64_t number, uint64_t* steps);
void collatz_cache_store(collatz_cache_t* cache, uint64_t number, uint64_t steps);

// Fonctions de calcul Collatz
uint64_t collatz_next_value(uint64_t n);
uint64_t collatz_compute_steps(uint64_t number, collatz_cache_t* cache);
collatz_sequence_t* collatz_compute_full_sequence(uint64_t number, collatz_config_t* config);

// Analyses Collatz
collatz_result_t* collatz_analyze_basic(uint64_t number, collatz_config_t* config);
collatz_result_t* collatz_analyze_range(uint64_t start, uint64_t end, collatz_config_t* config);
collatz_result_t* collatz_analyze_statistical(uint64_t start, uint64_t end, collatz_config_t* config);
collatz_result_t* collatz_analyze_parallel_batch(uint64_t* numbers, size_t count, collatz_config_t* config);

// Tests stress
bool collatz_stress_test_100m_numbers(collatz_config_t* config);

// Fonctions utilitaires
collatz_statistics_t* collatz_statistics_create(void);
void collatz_statistics_destroy(collatz_statistics_t** stats_ptr);
void collatz_statistics_update(collatz_statistics_t* stats, collatz_number_t* number);

// Configuration et nettoyage
collatz_config_t* collatz_config_create_default(void);
void collatz_config_destroy(collatz_config_t** config_ptr);
void collatz_result_destroy(collatz_result_t** result_ptr);

#endif // COLLATZ_ANALYZER_H
