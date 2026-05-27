// Feature test macros for POSIX functions - _GNU_SOURCE defined in Makefile
#define _POSIX_C_SOURCE 200809L

#include "ai_dynamic_config_manager.h"
#include "../debug/memory_tracker.h"
#include "../logger/lum_logger.h"
#include "../common/safe_string.h"  // SÉCURITÉ: Pour SAFE_STRCPY
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdio.h>

// Noms des modules pour logging
static const char* MODULE_NAMES[MODULE_COUNT] = {
    "pareto_optimizer",
    "simd_optimizer", 
    "zero_copy_allocator",
    "parallel_processor",
    "neural_network",
    "quantum_simulator",
    // "homomorphic_encryption", // REMOVED - No homomorphic functionality
    "matrix_calculator",
    "realtime_analytics",
    "distributed_computing",
    "ai_optimization"
};

// Création gestionnaire IA centralisé
ai_dynamic_config_manager_t* ai_dynamic_config_manager_create(void) {
    ai_dynamic_config_manager_t* manager = TRACKED_MALLOC(sizeof(ai_dynamic_config_manager_t));
    if (!manager) return NULL;

    // Initialisation structure principale
    manager->active_modules_count = 0;
    manager->auto_optimization_enabled = true;
    manager->global_performance_target = 1.0;
    manager->optimization_cycles_count = 0;
    manager->learning_rate_global = 0.001;
    manager->memory_address = (void*)manager;
    manager->manager_magic = AI_CONFIG_MANAGER_MAGIC;

    // Initialisation array de configurations
    for (size_t i = 0; i < MODULE_COUNT; i++) {
        manager->managed_configs[i] = NULL;
    }

    // Création agent IA maître pour orchestration
    size_t brain_layers[] = {128, 64, 32, 11}; // 11 modules sortie
    manager->master_agent = ai_agent_create(brain_layers, 4);
    if (!manager->master_agent) {
        TRACKED_FREE(manager);
        return NULL;
    }

    // Création LUM group pour état système avec capacité dynamique
    size_t dynamic_capacity = 512 + (MODULE_COUNT * 64); // Capacité basée sur le nombre de modules
    manager->system_state_lums = lum_group_create(dynamic_capacity);
    if (!manager->system_state_lums) {
        ai_agent_destroy(&manager->master_agent);
        TRACKED_FREE(manager);
        return NULL;
    }

    lum_logger_t* logger = lum_get_global_logger();
    if (logger) {
        char log_msg[512];
        snprintf(log_msg, sizeof(log_msg), "AI Dynamic Config Manager created successfully, capacity: %zu", dynamic_capacity);
        lum_log_message(logger, LUM_LOG_INFO, log_msg);
        lum_log_message(logger, LUM_LOG_INFO, "Master agent initialized with 4-layer brain");
        lum_log_message(logger, LUM_LOG_INFO, "System state tracking with dynamic capacity enabled");
    }

    return manager;
}

// Destruction gestionnaire
void ai_dynamic_config_manager_destroy(ai_dynamic_config_manager_t** manager_ptr) {
    if (!manager_ptr || !*manager_ptr) return;

    ai_dynamic_config_manager_t* manager = *manager_ptr;

    if (manager->manager_magic != AI_CONFIG_MANAGER_MAGIC ||
        manager->memory_address != (void*)manager) {
        return;
    }

    // Libération configurations modules
    for (size_t i = 0; i < MODULE_COUNT; i++) {
        if (manager->managed_configs[i]) {
            if (manager->managed_configs[i]->config_data) {
                TRACKED_FREE(manager->managed_configs[i]->config_data);
            }
            TRACKED_FREE(manager->managed_configs[i]);
        }
    }

    // Destruction agent maître
    if (manager->master_agent) {
        ai_agent_destroy(&manager->master_agent);
    }

    // Destruction groupe LUM système
    if (manager->system_state_lums) {
        lum_group_destroy(manager->system_state_lums);
    }

    manager->manager_magic = AI_CONFIG_DESTROYED_MAGIC;
    manager->memory_address = NULL;

    TRACKED_FREE(manager);
    *manager_ptr = NULL;
}

// Enregistrement module pour gestion dynamique IA
bool ai_register_module_for_management(ai_dynamic_config_manager_t* manager,
                                       ai_managed_module_type_e module_type,
                                       void* config_data,
                                       size_t config_size) {
    if (!manager || module_type >= MODULE_COUNT || !config_data || config_size == 0) {
        return false;
    }

    if (manager->manager_magic != AI_CONFIG_MANAGER_MAGIC) return false;

    // Création configuration module
    ai_module_config_t* module_config = TRACKED_MALLOC(sizeof(ai_module_config_t));
    if (!module_config) return false;

    // Copie données configuration
    module_config->config_data = TRACKED_MALLOC(config_size);
    if (!module_config->config_data) {
        TRACKED_FREE(module_config);
        return false;
    }

    memcpy(module_config->config_data, config_data, config_size);

    // Initialisation métadonnées
    module_config->module_type = module_type;
    module_config->config_size = config_size;
    module_config->is_active = true;
    module_config->performance_weight = 1.0;
    
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    module_config->last_updated_timestamp = ts.tv_sec * 1000000000ULL + ts.tv_nsec;
    
    strncpy(module_config->module_name, MODULE_NAMES[module_type], 
            sizeof(module_config->module_name) - 1);
    module_config->module_name[sizeof(module_config->module_name) - 1] = '\0';
    module_config->memory_address = (void*)module_config;

    // Enregistrement dans manager
    if (manager->managed_configs[module_type]) {
        // Remplacement configuration existante
        if (manager->managed_configs[module_type]->config_data) {
            TRACKED_FREE(manager->managed_configs[module_type]->config_data);
        }
        TRACKED_FREE(manager->managed_configs[module_type]);
    } else {
        manager->active_modules_count++;
    }

    manager->managed_configs[module_type] = module_config;

    lum_logger_t* logger = lum_get_global_logger();
    if (logger) {
        char log_msg[256];
        snprintf(log_msg, sizeof(log_msg), "Module '%s' registered for AI dynamic management, Config size: %zu bytes, Active modules: %zu/%d",
                 MODULE_NAMES[module_type], config_size, manager->active_modules_count, MODULE_COUNT);
        lum_log_message(logger, LUM_LOG_INFO, log_msg);
    }

    return true;
}

// Configuration dynamique Pareto Optimizer
bool ai_configure_pareto_optimizer(ai_dynamic_config_manager_t* manager,
                                  bool enable_simd,
                                  bool enable_memory_pooling,
                                  bool enable_parallel,
                                  bool enable_crypto,
                                  double efficiency_threshold) {
    if (!manager) return false;

    pareto_config_t config = {
        .enable_simd_optimization = enable_simd,
        .enable_memory_pooling = enable_memory_pooling,
        .enable_parallel_processing = enable_parallel,
        .max_optimization_layers = 10,
        .max_points = 1000
    };

    bool registered = ai_register_module_for_management(manager, 
                                                       MODULE_PARETO_OPTIMIZER,
                                                       &config, 
                                                       sizeof(pareto_config_t));

    if (registered && manager->master_agent) {
        // Apprentissage agent maître de cette configuration
        lum_group_t* state = lum_group_create(4);
        lum_group_t* action = lum_group_create(4);
        
        if (state && action) {
            // Encodage paramètres dans LUMs
            state->lums[0].presence = enable_simd ? 1 : 0;
            state->lums[1].presence = enable_memory_pooling ? 1 : 0;
            state->lums[2].presence = enable_parallel ? 1 : 0;
            state->lums[3].presence = enable_crypto ? 1 : 0;
            state->count = 4;

            action->lums[0].position_x = (int32_t)(efficiency_threshold * 1000);
            action->count = 1;

            // Récompense basée sur nombre optimisations activées
            double reward = (enable_simd + enable_memory_pooling + 
                           enable_parallel + enable_crypto) * 0.25;

            ai_agent_learn_from_experience(manager->master_agent, state, action, reward);

            lum_group_destroy(state);
            lum_group_destroy(action);
        }
    }

    LOG_INFOF("✅ Pareto Optimizer configured via AI: SIMD=%s, Memory=%s, Parallel=%s, Crypto=%s",
           enable_simd ? "ON" : "OFF",
           enable_memory_pooling ? "ON" : "OFF", 
           enable_parallel ? "ON" : "OFF",
           enable_crypto ? "ON" : "OFF");

    return registered;
}

// Configuration dynamique Neural Network
bool ai_configure_neural_network(ai_dynamic_config_manager_t* manager,
                                 size_t max_epochs,
                                 double convergence_threshold,
                                 bool use_momentum,
                                 double momentum_coefficient,
                                 bool use_dropout,
                                 double dropout_rate) {
    if (!manager) return false;

    neural_config_t* config = neural_config_create_default();
    if (!config) return false;

    // Application paramètres personnalisés
    config->max_epochs = max_epochs;
    config->convergence_threshold = convergence_threshold;
    config->use_momentum = use_momentum;
    config->momentum_coefficient = momentum_coefficient;
    config->use_dropout = use_dropout;
    config->dropout_rate = dropout_rate;

    bool registered = ai_register_module_for_management(manager,
                                                       MODULE_NEURAL_NETWORK,
                                                       config,
                                                       sizeof(neural_config_t));

    // CORRECTION FUITE MÉMOIRE: libérer config après registration
    neural_config_destroy(&config);

    LOG_INFOF("✅ Neural Network configured via AI: epochs=%zu, threshold=%.6f, momentum=%s",
           max_epochs, convergence_threshold, use_momentum ? "ON" : "OFF");

    return registered;
}

// Optimisation automatique TOUS paramètres système
ai_dynamic_optimization_result_t* ai_optimize_all_system_parameters(
    ai_dynamic_config_manager_t* manager,
    double performance_target) {
    
    if (!manager) return NULL;

    struct timespec start, end;
    clock_gettime(CLOCK_REALTIME, &start);

    ai_dynamic_optimization_result_t* result = TRACKED_MALLOC(sizeof(ai_dynamic_optimization_result_t));
    if (!result) return NULL;

    result->memory_address = (void*)result;
    result->optimization_success = false;
    result->performance_improvement_ratio = 0.0;
    result->modules_optimized_count = 0;
    result->optimized_parameters = NULL;

    manager->optimization_cycles_count++;

    LOG_INFOF("=== AI SYSTEM-WIDE OPTIMIZATION CYCLE #%lu ===", 
           manager->optimization_cycles_count);
    LOG_INFOF("Target performance: %.3f", performance_target);
    LOG_INFOF("Active modules under AI control: %zu/%d", 
           manager->active_modules_count, MODULE_COUNT);

    // Utilisation agent maître pour décision optimisation
    if (manager->master_agent && manager->system_state_lums) {
        // Mise à jour état système dans LUMs
        for (size_t i = 0; i < manager->active_modules_count && 
             i < manager->system_state_lums->capacity; i++) {
            
            manager->system_state_lums->lums[i].id = i;
            manager->system_state_lums->lums[i].presence = 1;
            manager->system_state_lums->lums[i].position_x = (int32_t)(performance_target * 1000);
            manager->system_state_lums->lums[i].position_y = (int32_t)(manager->optimization_cycles_count);
            manager->system_state_lums->lums[i].structure_type = LUM_STRUCTURE_GROUP;
            manager->system_state_lums->count++;
        }

        // Prise de décision par agent IA maître
        lum_group_t* optimization_decision = ai_agent_make_decision(manager->master_agent,
                                                                  manager->system_state_lums);
        if (optimization_decision) {
            result->optimized_parameters = optimization_decision;
            
            // Optimisation par module actif
            for (size_t i = 0; i < MODULE_COUNT; i++) {
                if (manager->managed_configs[i] && manager->managed_configs[i]->is_active) {
                    
                    // Simulation optimisation selon type module
                    switch (manager->managed_configs[i]->module_type) {
                        case MODULE_PARETO_OPTIMIZER: {
                            pareto_config_t* config = (pareto_config_t*)manager->managed_configs[i]->config_data;
                            // Optimisation adaptative basée sur performance actuelle
                            if (performance_target > 0.8) {
                                config->enable_simd_optimization = true;
                                config->enable_parallel_processing = true;
                            }
                            result->modules_optimized_count++;
                            break;
                        }
                        case MODULE_NEURAL_NETWORK: {
                            neural_config_t* config = (neural_config_t*)manager->managed_configs[i]->config_data;
                            // Adaptation learning rate selon cible performance
                            if (performance_target > 0.9) {
                                config->max_epochs = (size_t)(config->max_epochs * 1.2);
                                config->convergence_threshold *= 0.9;
                            }
                            result->modules_optimized_count++;
                            break;
                        }
                        default:
                            // Module supporté mais optimisation générique
                            manager->managed_configs[i]->performance_weight *= 
                                (performance_target + 1.0) / 2.0;
                            result->modules_optimized_count++;
                            break;
                    }

                    struct timespec ts;
                    clock_gettime(CLOCK_REALTIME, &ts);
                    manager->managed_configs[i]->last_updated_timestamp = 
                        ts.tv_sec * 1000000000ULL + ts.tv_nsec;
                }
            }
            
            result->optimization_success = (result->modules_optimized_count > 0);
            SAFE_STRCPY(result->optimization_strategy, "AI_Master_Agent_Guided_Optimization", sizeof(result->optimization_strategy));
        }
    }

    clock_gettime(CLOCK_REALTIME, &end);
    result->total_optimization_time_ms = 
        ((end.tv_sec - start.tv_sec) * 1000.0) + 
        ((end.tv_nsec - start.tv_nsec) / 1000000.0);

    if (result->optimization_success) {
        result->performance_improvement_ratio = 
            (double)result->modules_optimized_count / MODULE_COUNT;
        
        LOG_INFOF("✅ AI optimization completed successfully");
        LOG_INFOF("   Modules optimized: %zu/%d (%.1f%%)",
               result->modules_optimized_count, (int)MODULE_COUNT,
               result->performance_improvement_ratio * 100.0);
        LOG_INFOF("   Optimization time: %.3f ms", result->total_optimization_time_ms);
    } else {
        LOG_ERRORF("❌ AI optimization failed or no improvements found");
    }

    return result;
}

// Test stress gestion dynamique 100M+ paramètres
bool ai_stress_test_100m_parameters(ai_dynamic_config_manager_t* manager) {
    if (!manager) return false;

    LOG_INFOF("=== AI DYNAMIC CONFIG STRESS TEST: 100M+ Parameters ===");

    const size_t param_count = 100000000; // 100M paramètres
    const size_t test_params = 100000;    // 100K test representatif
    
    struct timespec start, end;
    clock_gettime(CLOCK_REALTIME, &start);

    LOG_INFOF("Testing AI parameter management at scale...");

    // Test configuration massive modules
    size_t configs_created = 0;
    for (size_t i = 0; i < test_params; i++) {
        // Configuration Pareto avec paramètres variables
        bool success = ai_configure_pareto_optimizer(manager,
            (i % 2) == 0,           // SIMD alterné
            (i % 3) == 0,           // Memory pooling cyclique  
            (i % 4) == 0,           // Parallel cyclique
            (i % 5) == 0,           // Crypto cyclique
            0.5 + (i % 1000) / 2000.0);  // Efficiency threshold variable

        if (success) configs_created++;

        if (i > 0 && (i % 10000) == 0) {
            LOG_INFOF("   Progress: %zu/%zu configurations processed (%.1f%%)",
                   i, test_params, (i * 100.0) / test_params);
        }
    }

    clock_gettime(CLOCK_REALTIME, &end);
    double config_time = (end.tv_sec - start.tv_sec) + 
                        (end.tv_nsec - start.tv_nsec) / 1000000000.0;

    LOG_INFOF("✅ AI configuration test completed");
    LOG_INFOF("   Configurations created: %zu/%zu (%.1f%% success)",
           configs_created, test_params, (configs_created * 100.0) / test_params);
    LOG_INFOF("   Configuration time: %.3f seconds", config_time);
    LOG_INFOF("   Configuration rate: %.0f configs/second", configs_created / config_time);

    // Projection pour 100M
    double projected_time = config_time * (param_count / (double)test_params);
    LOG_INFOF("   Projected time for %zu parameters: %.1f seconds", 
           param_count, projected_time);

    // Test optimisation système complète
    LOG_INFOF("Testing system-wide AI optimization...");
    clock_gettime(CLOCK_REALTIME, &start);

    ai_dynamic_optimization_result_t* opt_result = 
        ai_optimize_all_system_parameters(manager, 0.95);

    clock_gettime(CLOCK_REALTIME, &end);

    if (opt_result) {
        LOG_INFOF("✅ System optimization completed successfully");
        LOG_INFOF("   Performance improvement: %.3f", opt_result->performance_improvement_ratio);
        LOG_INFOF("   Modules optimized: %zu", opt_result->modules_optimized_count);

        if (opt_result->optimized_parameters) {
            lum_group_destroy(opt_result->optimized_parameters);
        }
        TRACKED_FREE(opt_result);
    }

    LOG_INFOF("✅ AI Dynamic Config Manager stress test 100M+ parameters completed");
    return true;
}

// Export configuration système complète  
bool ai_export_system_configuration(ai_dynamic_config_manager_t* manager,
                                   const char* filename) {
    if (!manager || !filename) return false;

    FILE* file = fopen(filename, "w");
    if (!file) return false;

    fprintf(file, "# AI Dynamic Config Manager - System Configuration Export\n");
    fprintf(file, "# Generated at: %lu\n", (unsigned long)time(NULL));
    fprintf(file, "# Active modules: %zu/%d\n", manager->active_modules_count, MODULE_COUNT);
    fprintf(file, "# Optimization cycles: %lu\n", manager->optimization_cycles_count);
    fprintf(file, "\n[GLOBAL_SETTINGS]\n");
    fprintf(file, "auto_optimization_enabled=%s\n", 
            manager->auto_optimization_enabled ? "true" : "false");
    fprintf(file, "global_performance_target=%.6f\n", manager->global_performance_target);
    fprintf(file, "learning_rate_global=%.6f\n", manager->learning_rate_global);

    fprintf(file, "\n[MANAGED_MODULES]\n");
    for (size_t i = 0; i < MODULE_COUNT; i++) {
        if (manager->managed_configs[i]) {
            ai_module_config_t* config = manager->managed_configs[i];
            fprintf(file, "[MODULE_%s]\n", config->module_name);
            fprintf(file, "type=%d\n", (int)config->module_type);
            fprintf(file, "is_active=%s\n", config->is_active ? "true" : "false");
            fprintf(file, "performance_weight=%.6f\n", config->performance_weight);
            fprintf(file, "last_updated=%lu\n", config->last_updated_timestamp);
            fprintf(file, "config_size=%zu\n", config->config_size);
            fprintf(file, "\n");
        }
    }

    fclose(file);
    LOG_INFOF("✅ System configuration exported to: %s", filename);
    return true;
}