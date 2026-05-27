
#ifndef MATHEMATICAL_RESEARCH_ENGINE_H
#define MATHEMATICAL_RESEARCH_ENGINE_H

#include "../lum/lum_core.h"
#include "../debug/memory_tracker.h"
#include <stdint.h>
#include <stdbool.h>

// Configuration du moteur de recherche mathématique
typedef struct {
    uint64_t max_iterations;
    uint64_t cache_size;
    bool enable_visualization;
    bool enable_conjecture_generation;
    double convergence_threshold;
    uint32_t parallel_threads;
} math_research_config_t;

// Utiliser la définition de collatz_sequence_t depuis collatz_analyzer.h
#include "collatz_analyzer.h"

// Résultats de recherche dynamique
typedef struct {
    collatz_sequence_t* sequences;
    size_t sequence_count;
    size_t sequence_capacity;
    
    // Statistiques dynamiques
    uint64_t total_calculations;
    double average_length;
    double max_length_ratio;
    
    // Conjectures émergentes
    char** emergent_patterns;
    size_t pattern_count;
    
    // Métriques de performance
    double calculation_time;
    size_t memory_usage;
} math_research_result_t;

// Cache intelligent pour optimisation
typedef struct {
    uint64_t value;
    uint64_t result;
    uint64_t access_count;
} collatz_cache_entry_t;

// Moteur de recherche principal
typedef struct {
    math_research_config_t config;
    collatz_cache_entry_t* cache;
    size_t cache_current_size;
    
    // Statistiques en temps réel
    uint64_t cache_hits;
    uint64_t cache_misses;
    uint64_t total_sequences_analyzed;
    
    // Intégration LUM/VORAX
    lum_group_t* research_data_group;
    uint32_t research_session_id;
} mathematical_research_engine_t;

// Fonctions principales du moteur
mathematical_research_engine_t* math_research_engine_create(math_research_config_t* config);
void math_research_engine_destroy(mathematical_research_engine_t* engine);

// Analyse de séquence individuelle Collatz avec LUM
collatz_sequence_t* analyze_single_collatz_sequence(
    mathematical_research_engine_t* engine,
    uint64_t initial_value
);

// Analyse dynamique de Collatz
math_research_result_t* analyze_collatz_dynamic_range(
    mathematical_research_engine_t* engine,
    uint64_t start_value,
    uint64_t end_value
);

// Génération d'hypothèses basée sur les données
bool generate_mathematical_conjectures(
    mathematical_research_engine_t* engine,
    math_research_result_t* results
);

// Visualisation et export des données
bool export_research_data_to_lum_format(
    mathematical_research_engine_t* engine,
    math_research_result_t* results,
    const char* output_path
);

// Analyse de motifs récursifs
bool detect_recursive_patterns(
    mathematical_research_engine_t* engine,
    collatz_sequence_t* sequences,
    size_t count
);

// Optimisations algorithmiques
bool optimize_cache_strategy(mathematical_research_engine_t* engine);
uint64_t collatz_step_optimized(uint64_t n, mathematical_research_engine_t* engine);

// Intégration avec le système de logging forensique
void log_research_progress(
    mathematical_research_engine_t* engine,
    const char* phase,
    double progress_percentage
);

// Analyse comparative avec autres systèmes dynamiques
bool compare_with_other_discrete_systems(
    mathematical_research_engine_t* engine,
    math_research_result_t* collatz_results
);

// Configuration par défaut
math_research_config_t* create_default_research_config(void);

#endif // MATHEMATICAL_RESEARCH_ENGINE_H
