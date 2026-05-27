
#include "hostinger_resource_limiter.h"
#include "../debug/memory_tracker.h"
#include "../logger/lum_logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// CORRECTION RAPPORT 117: Limites configurables dynamiques
typedef struct {
    uint32_t max_cpu_cores;
    uint32_t max_ram_gb;
    uint32_t max_storage_gb;
    uint32_t max_concurrent_lums;
    const char* server_identifier;
} resource_limits_config_t;

static resource_limits_config_t current_limits = {
    .max_cpu_cores = 2,
    .max_ram_gb = 6,
    .max_storage_gb = 90,
    .max_concurrent_lums = 1000000,
    .server_identifier = "generic"
};

bool hostinger_set_resource_limits(uint32_t cpu, uint32_t ram_gb, uint32_t storage_gb, uint32_t max_lums) {
    current_limits.max_cpu_cores = cpu;
    current_limits.max_ram_gb = ram_gb;
    current_limits.max_storage_gb = storage_gb;
    current_limits.max_concurrent_lums = max_lums;
    return true;
}

// Structure hostinger_resource_monitor_t déjà définie dans hostinger_resource_limiter.h

static hostinger_resource_monitor_t* global_monitor = NULL;

bool hostinger_check_cpu_availability(void) {
    if (!global_monitor) return false;
    
    if (global_monitor->active_threads >= current_limits.max_cpu_cores) {
        // CORRECTION RAPPORT 117: Logging système au lieu de printf hardcodé
        if (lum_get_global_logger()) {
            char log_msg[256];
            snprintf(log_msg, sizeof(log_msg), "❌ CPU limité: %u/%u threads actifs",
                     (uint32_t)global_monitor->active_threads, current_limits.max_cpu_cores);
            lum_log_message(lum_get_global_logger(), LUM_LOG_WARN, log_msg);
        }
        return false;
    }
    
    // CORRECTION RAPPORT 117: Logging système
    if (lum_get_global_logger()) {
        char log_msg[256];
        snprintf(log_msg, sizeof(log_msg), "✅ CPU disponible: %u/%u threads",
                 (uint32_t)global_monitor->active_threads, current_limits.max_cpu_cores);
        lum_log_message(lum_get_global_logger(), LUM_LOG_DEBUG, log_msg);
    }
    return true;
}

bool hostinger_check_ram_availability(size_t required_mb) {
    if (!global_monitor) return false;
    
    size_t max_ram_mb = current_limits.max_ram_gb * 1024;
    size_t total_needed = global_monitor->current_ram_usage_mb + required_mb;
    
    if (total_needed > max_ram_mb) {
        // CORRECTION RAPPORT 117: Logging système
        if (lum_get_global_logger()) {
            char log_msg[256];
            snprintf(log_msg, sizeof(log_msg), "❌ RAM insuffisante: %zu MB + %zu MB > %zu MB max",
                     global_monitor->current_ram_usage_mb, required_mb, max_ram_mb);
            lum_log_message(lum_get_global_logger(), LUM_LOG_WARN, log_msg);
        }
        return false;
    }
    
    // CORRECTION RAPPORT 117: Logging système
    if (lum_get_global_logger()) {
        char log_msg[256];
        snprintf(log_msg, sizeof(log_msg), "✅ RAM disponible: %zu MB libres",
                 max_ram_mb - total_needed);
        lum_log_message(lum_get_global_logger(), LUM_LOG_DEBUG, log_msg);
    }
    return true;
}

bool hostinger_check_lum_processing_limit(size_t lum_count) {
    if (!global_monitor) return false;
    
    if (lum_count > current_limits.max_concurrent_lums) {
        // CORRECTION RAPPORT 117: Logging système
        if (lum_get_global_logger()) {
            char log_msg[256];
            snprintf(log_msg, sizeof(log_msg), "❌ Trop de LUMs: %zu > %u max autorisés",
                     lum_count, current_limits.max_concurrent_lums);
            lum_log_message(lum_get_global_logger(), LUM_LOG_WARN, log_msg);
        }
        // CORRECTION RAPPORT 117: Message dynamique
        if (lum_get_global_logger()) {
            char log_msg[256];
            snprintf(log_msg, sizeof(log_msg), "Limitation serveur %uCPU/%uGB RAM",
                     current_limits.max_cpu_cores, current_limits.max_ram_gb);
            lum_log_message(lum_get_global_logger(), LUM_LOG_INFO, log_msg);
        }
        return false;
    }
    
    // CORRECTION RAPPORT 117: Logging système
    if (lum_get_global_logger()) {
        char log_msg[256];
        snprintf(log_msg, sizeof(log_msg), "✅ LUMs autorisés: %zu/%u",
                 lum_count, current_limits.max_concurrent_lums);
        lum_log_message(lum_get_global_logger(), LUM_LOG_DEBUG, log_msg);
    }
    return true;
}

hostinger_resource_monitor_t* hostinger_resource_monitor_create(void) {
    if (global_monitor) return global_monitor;
    
    global_monitor = TRACKED_MALLOC(sizeof(hostinger_resource_monitor_t));
    if (!global_monitor) return NULL;
    
    memset(global_monitor, 0, sizeof(hostinger_resource_monitor_t));
    global_monitor->memory_address = global_monitor;
    global_monitor->magic_number = 0x484F5354;  // "HOST"
    global_monitor->resource_check_enabled = true;
    
    printf("[HOSTINGER_LIMITER] Monitor créé - Serveur 72.60.185.90\n");
    printf("[HOSTINGER_LIMITER] Limites: CPU=2cores, RAM=6GB, LUMs=1M max\n");
    
    return global_monitor;
}

void hostinger_resource_monitor_destroy(void) {
    if (global_monitor && global_monitor->memory_address == global_monitor) {
        TRACKED_FREE(global_monitor);
        global_monitor = NULL;
        printf("[HOSTINGER_LIMITER] Monitor détruit\n");
    }
}
