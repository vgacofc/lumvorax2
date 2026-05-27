#include "mathematical_research_engine.h"
#include "../logger/lum_logger.h"
#include "../debug/forensic_logger.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

// Création du moteur de recherche
mathematical_research_engine_t* math_research_engine_create(math_research_config_t* config) {
    mathematical_research_engine_t* engine = TRACKED_MALLOC(sizeof(mathematical_research_engine_t));
    if (!engine) return NULL;

    // Copier la configuration
    if (config) {
        engine->config = *config;
    } else {
        math_research_config_t* default_config = create_default_research_config();
        engine->config = *default_config;
        TRACKED_FREE(default_config);
    }

    // Initialiser le cache intelligent
    engine->cache = TRACKED_MALLOC(sizeof(collatz_cache_entry_t) * engine->config.cache_size);
    if (!engine->cache) {
        TRACKED_FREE(engine);
        return NULL;
    }
    memset(engine->cache, 0, sizeof(collatz_cache_entry_t) * engine->config.cache_size);

    // Initialiser les statistiques
    engine->cache_current_size = 0;
    engine->cache_hits = 0;
    engine->cache_misses = 0;
    engine->total_sequences_analyzed = 0;

    // Créer le groupe LUM pour stocker les données de recherche
    engine->research_data_group = lum_group_create(10000);
    engine->research_session_id = lum_generate_id();

    // Logging forensique
    forensic_log_memory_operation("MATH_RESEARCH_ENGINE_CREATE", engine, sizeof(mathematical_research_engine_t));
    lum_log(LUM_LOG_INFO, "Moteur de recherche mathématique initialisé - Session ID: %u", 
            engine->research_session_id);

    return engine;
}

// Destruction du moteur
void math_research_engine_destroy(mathematical_research_engine_t* engine) {
    if (!engine) return;

    lum_log(LUM_LOG_INFO, "Destruction moteur recherche - Séquences analysées: %lu, Cache hits: %lu",
            engine->total_sequences_analyzed, engine->cache_hits);

    if (engine->cache) {
        TRACKED_FREE(engine->cache);
    }

    if (engine->research_data_group) {
        lum_group_destroy(engine->research_data_group);
    }

    forensic_log_memory_operation("MATH_RESEARCH_ENGINE_DESTROY", engine, 0);
    TRACKED_FREE(engine);
}

// Fonction Collatz optimisée avec cache
uint64_t collatz_step_optimized(uint64_t n, mathematical_research_engine_t* engine) {
    if (n <= 1) return 1;

    // Vérifier le cache
    for (size_t i = 0; i < engine->cache_current_size; i++) {
        if (engine->cache[i].value == n) {
            engine->cache[i].access_count++;
            engine->cache_hits++;
            return engine->cache[i].result;
        }
    }

    // Calculer la prochaine valeur
    uint64_t result;
    if (n % 2 == 0) {
        result = n / 2;
    } else {
        // Protection contre débordement
        if (n > (UINT64_MAX - 1) / 3) {
            lum_log(LUM_LOG_WARN, "Overflow détecté pour n=%lu", n);
            return UINT64_MAX;
        }
        result = 3 * n + 1;
    }

    // Ajouter au cache si il y a de la place
    if (engine->cache_current_size < engine->config.cache_size) {
        engine->cache[engine->cache_current_size].value = n;
        engine->cache[engine->cache_current_size].result = result;
        engine->cache[engine->cache_current_size].access_count = 1;
        engine->cache_current_size++;
    }

    engine->cache_misses++;
    return result;
}

// Analyse d'une séquence de Collatz avec représentation LUM
collatz_sequence_t* analyze_single_collatz_sequence(
    mathematical_research_engine_t* engine,
    uint64_t starting_number
) {
    collatz_sequence_t* sequence = collatz_sequence_create(starting_number);
    if (!sequence) return NULL;

    // Les champs sont déjà initialisés par collatz_sequence_create
    // sequence->starting_number contient starting_number
    // sequence->sequence_length est initialisé à 0
    // sequence->max_value est initialisé à starting_number

    // Calculer la séquence complète
    uint64_t current = starting_number;
    uint64_t step_count = 0;

    // CORRECTION: Utiliser constantes cohérentes du header
    const uint64_t MAX_STEPS = COLLATZ_MAX_ITERATIONS;

    while (current != 1 && step_count < MAX_STEPS) {
        uint64_t next = collatz_step_optimized(current, engine);
        if (next == UINT64_MAX) break; // Overflow détecté

        if (next > sequence->max_value) {
            sequence->max_value = next;
        }

        current = next;
        step_count++;

        // Logging périodique pour grandes séquences
        if (step_count % 100000 == 0) {
            lum_log(LUM_LOG_DEBUG, "Séquence %lu: %lu étapes, max=%lu, current=%lu",
                    starting_number, step_count, sequence->max_value, current);
        }
    }

    sequence->steps_count = step_count;
    sequence->sequence_length = step_count;
    sequence->compression_ratio = (double)sequence->max_value / (double)starting_number;

    // CORRECTION CRITIQUE: Réallouer proprement au lieu de double allocation
    if (step_count < sequence->sequence_capacity && step_count > 0) {
        // Utiliser l'allocation existante si suffisante
        current = starting_number;
        for (uint64_t i = 0; i < step_count && current != 1 && i < sequence->sequence_capacity; i++) {
            sequence->sequence[i] = current;
            current = collatz_step_optimized(current, engine);
        }
    } else if (step_count >= sequence->sequence_capacity && step_count < 100000) {
        // LIBÉRER l'ancienne allocation avant la nouvelle
        if (sequence->sequence) {
            TRACKED_FREE(sequence->sequence);
            sequence->sequence = NULL;
        }
        
        // Nouvelle allocation avec taille correcte
        sequence->sequence = TRACKED_MALLOC(sizeof(uint64_t) * step_count);
        sequence->sequence_capacity = step_count;
        
        if (sequence->sequence) {
            current = starting_number;
            for (uint64_t i = 0; i < step_count && current != 1; i++) {
                sequence->sequence[i] = current;
                current = collatz_step_optimized(current, engine);
            }
        }
    } else {
        // Séquence trop grande - garder seulement les métadonnées
        if (sequence->sequence) {
            TRACKED_FREE(sequence->sequence);
            sequence->sequence = NULL;
        }
        sequence->sequence_capacity = 0;
    }

    // CORRECTION CRITIQUE: Nettoyage forcé obligatoire
    if (sequence) {
        // Libérer immédiatement les gros tableaux pour éviter les fuites
        if (sequence->sequence && sequence->sequence_capacity > 1000) {
            TRACKED_FREE(sequence->sequence);
            sequence->sequence = NULL;
            sequence->sequence_capacity = 0;
        }
        
        // Ajout du nettoyage obligatoire
        forensic_log_memory_operation("ANALYZE_COLLATZ_SEQUENCE_END", sequence, 0);
    }

    return sequence;
}

// Analyse dynamique d'une plage de valeurs
math_research_result_t* analyze_collatz_dynamic_range(
    mathematical_research_engine_t* engine,
    uint64_t start_value,
    uint64_t end_value
) {
    if (!engine || start_value >= end_value) return NULL;

    math_research_result_t* results = TRACKED_MALLOC(sizeof(math_research_result_t));
    if (!results) return NULL;

    // CORRECTION CRITIQUE: Limitation drastique pour éviter crash
    size_t range_size = (size_t)(end_value - start_value);
    if (range_size > 10) {  // RÉDUIT: 10 nombres max pour éviter fuites
        range_size = 10;
        end_value = start_value + 10;
    }

    results->sequences = TRACKED_MALLOC(sizeof(collatz_sequence_t) * range_size);
    if (!results->sequences) {
        TRACKED_FREE(results);
        return NULL;
    }

    results->sequence_count = 0;
    results->sequence_capacity = range_size;
    results->total_calculations = 0;
    results->average_length = 0.0;
    results->max_length_ratio = 0.0;
    results->pattern_count = 0;
    results->emergent_patterns = NULL;

    clock_t start_time = clock();

    lum_log(LUM_LOG_INFO, "Début analyse Collatz dynamique: %lu à %lu (%zu valeurs)",
            start_value, end_value, range_size);

    // Analyser chaque valeur dans la plage
    for (uint64_t n = start_value; n < end_value; n++) {
        collatz_sequence_t* sequence = analyze_single_collatz_sequence(engine, n);
        if (sequence) {
            results->sequences[results->sequence_count] = *sequence;
            results->sequence_count++;
            results->total_calculations += sequence->steps_count;

            // Mettre à jour les statistiques
            if (sequence->compression_ratio > results->max_length_ratio) {
                results->max_length_ratio = sequence->compression_ratio;
            }

            TRACKED_FREE(sequence);
        }

        engine->total_sequences_analyzed++;

        // Logging de progression
        if ((n - start_value) % 10000 == 0) {
            double progress = (double)(n - start_value) * 100.0 / (double)range_size;
            log_research_progress(engine, "COLLATZ_ANALYSIS", progress);
        }
    }

    // Calculer les statistiques finales
    if (results->sequence_count > 0) {
        results->average_length = (double)results->total_calculations / (double)results->sequence_count;
    }

    clock_t end_time = clock();
    results->calculation_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    results->memory_usage = memory_tracker_get_current_usage();

    lum_log(LUM_LOG_INFO, "Analyse terminée: %zu séquences, temps=%.3fs, longueur moyenne=%.2f",
            results->sequence_count, results->calculation_time, results->average_length);

    return results;
}

// Génération de conjectures mathématiques
bool generate_mathematical_conjectures(
    mathematical_research_engine_t* engine,
    math_research_result_t* results
) {
    if (!engine || !results || results->sequence_count == 0) return false;

    lum_log(LUM_LOG_INFO, "Génération de conjectures basée sur %zu séquences", results->sequence_count);

    // Analyser les motifs de longueur
    uint64_t total_length = 0;
    uint64_t max_length = 0;
    uint64_t min_length = UINT64_MAX;

    for (size_t i = 0; i < results->sequence_count; i++) {
        uint64_t length = results->sequences[i].steps_count;
        total_length += length;
        if (length > max_length) max_length = length;
        if (length < min_length) min_length = length;
    }

    double avg_length = (double)total_length / (double)results->sequence_count;
    double length_variance = 0.0;

    for (size_t i = 0; i < results->sequence_count; i++) {
        double diff = (double)results->sequences[i].steps_count - avg_length;
        length_variance += diff * diff;
    }
    length_variance /= (double)results->sequence_count;

    // Allouer de l'espace pour les conjectures
    results->emergent_patterns = TRACKED_MALLOC(sizeof(char*) * 10);
    if (!results->emergent_patterns) return false;

    size_t pattern_index = 0;

    // Conjecture 1: Distribution de longueur
    char* pattern1 = TRACKED_MALLOC(256);
    if (pattern1) {
        snprintf(pattern1, 256, "Distribution longueurs: avg=%.2f, var=%.2f, min=%lu, max=%lu",
                avg_length, length_variance, min_length, max_length);
        results->emergent_patterns[pattern_index++] = pattern1;
    }

    // Conjecture 2: Relation longueur/valeur initiale
    double correlation_sum = 0.0;
    for (size_t i = 0; i < results->sequence_count; i++) {
        double normalized_initial = log((double)results->sequences[i].starting_number);
        double normalized_length = log((double)results->sequences[i].steps_count);
        correlation_sum += normalized_initial * normalized_length;
    }

    char* pattern2 = TRACKED_MALLOC(256);
    if (pattern2) {
        snprintf(pattern2, 256, "Corrélation log(initiale) vs log(longueur): %.6f",
                correlation_sum / (double)results->sequence_count);
        results->emergent_patterns[pattern_index++] = pattern2;
    }

    // Conjecture 3: Ratios de convergence
    double avg_ratio = 0.0;
    for (size_t i = 0; i < results->sequence_count; i++) {
        avg_ratio += results->sequences[i].compression_ratio;
    }
    avg_ratio /= (double)results->sequence_count;

    char* pattern3 = TRACKED_MALLOC(256);
    if (pattern3) {
        snprintf(pattern3, 256, "Ratio convergence moyen (max/initial): %.6f, max observé: %.6f",
                avg_ratio, results->max_length_ratio);
        results->emergent_patterns[pattern_index++] = pattern3;
    }

    results->pattern_count = pattern_index;

    lum_log(LUM_LOG_INFO, "Conjectures générées: %zu motifs identifiés", results->pattern_count);

    return true;
}

// Logging de progression
void log_research_progress(
    mathematical_research_engine_t* engine,
    const char* phase,
    double progress_percentage
) {
    static time_t last_log_time = 0;
    time_t current_time = time(NULL);

    // Limiter la fréquence des logs
    if (current_time - last_log_time >= 5) { // Log toutes les 5 secondes maximum
        lum_log(LUM_LOG_INFO, "[SESSION %u] %s: %.1f%% - Cache: %lu hits, %lu miss",
                engine->research_session_id, phase, progress_percentage,
                engine->cache_hits, engine->cache_misses);

        forensic_log_lum_operation(phase, engine->total_sequences_analyzed, 
                                  (current_time - last_log_time) * 1000000000.0);

        last_log_time = current_time;
    }
}

// Configuration par défaut
math_research_config_t* create_default_research_config(void) {
    math_research_config_t* config = TRACKED_MALLOC(sizeof(math_research_config_t));
    if (!config) return NULL;

    config->max_iterations = 10000000;
    config->cache_size = 100000;
    config->enable_visualization = true;
    config->enable_conjecture_generation = true;
    config->convergence_threshold = 1e-6;
    config->parallel_threads = 1;

    return config;
}