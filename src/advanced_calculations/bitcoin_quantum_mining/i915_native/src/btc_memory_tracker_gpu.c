/**
 * @file btc_memory_tracker_gpu.c
 * @brief Implémentation Memory Tracker RAM+GPU
 * 
 * PHASE 15W: Tracking complet allocations mémoire avec détection fuites
 * 
 * @author LumVorax Team
 * @date 2026-05-11
 */

#define _GNU_SOURCE
#include "btc_memory_tracker_gpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

/* ============================================================================
 * CONSTANTES
 * ============================================================================ */

#define BTC_MEM_DEFAULT_MAX_ENTRIES 10000
#define BTC_MEM_HASH_SIZE 1024

/* ============================================================================
 * STRUCTURES INTERNES
 * ============================================================================ */

/**
 * @brief Nœud de hash table pour recherche rapide
 */
typedef struct btc_mem_node {
    btc_mem_entry_t entry;
    struct btc_mem_node* next;
} btc_mem_node_t;

/**
 * @brief Structure tracker mémoire
 */
struct btc_mem_tracker {
    btc_mem_node_t* hash_table[BTC_MEM_HASH_SIZE];
    btc_mem_stats_t stats;
    size_t max_entries;
    size_t current_entries;
    pthread_mutex_t mutex;
    bool initialized;
};

/* ============================================================================
 * FONCTIONS UTILITAIRES
 * ============================================================================ */

/**
 * @brief Obtenir timestamp nanosec
 */
static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

/**
 * @brief Hash fonction pour pointeur
 */
static size_t hash_ptr(const void* ptr) {
    uintptr_t val = (uintptr_t)ptr;
    return (val ^ (val >> 16)) % BTC_MEM_HASH_SIZE;
}

/**
 * @brief Hash fonction pour GEM handle
 */
static size_t hash_gem(uint32_t handle) {
    return handle % BTC_MEM_HASH_SIZE;
}

/**
 * @brief Trouver entrée par pointeur
 */
static btc_mem_node_t* find_entry_by_ptr(
    btc_mem_tracker_t* tracker,
    const void* ptr
) {
    size_t idx = hash_ptr(ptr);
    btc_mem_node_t* node = tracker->hash_table[idx];
    
    while (node) {
        if (node->entry.ptr == ptr && !node->entry.freed) {
            return node;
        }
        node = node->next;
    }
    
    return NULL;
}

/**
 * @brief Trouver entrée par GEM handle
 */
static btc_mem_node_t* find_entry_by_gem(
    btc_mem_tracker_t* tracker,
    uint32_t gem_handle
) {
    size_t idx = hash_gem(gem_handle);
    btc_mem_node_t* node = tracker->hash_table[idx];
    
    while (node) {
        if (node->entry.gem_handle == gem_handle && !node->entry.freed) {
            return node;
        }
        node = node->next;
    }
    
    return NULL;
}

/**
 * @brief Ajouter entrée dans hash table
 */
static bool add_entry(
    btc_mem_tracker_t* tracker,
    const btc_mem_entry_t* entry
) {
    if (tracker->current_entries >= tracker->max_entries) {
        fprintf(stderr, "[MEM] Max entries reached (%zu)\n", tracker->max_entries);
        return false;
    }
    
    btc_mem_node_t* node = malloc(sizeof(btc_mem_node_t));
    if (!node) {
        return false;
    }
    
    memcpy(&node->entry, entry, sizeof(btc_mem_entry_t));
    
    size_t idx;
    if (entry->type == BTC_MEM_TYPE_GPU_GEM) {
        idx = hash_gem(entry->gem_handle);
    } else {
        idx = hash_ptr(entry->ptr);
    }
    
    node->next = tracker->hash_table[idx];
    tracker->hash_table[idx] = node;
    tracker->current_entries++;
    
    return true;
}

/* ============================================================================
 * API PUBLIQUE
 * ============================================================================ */

btc_mem_tracker_t* btc_mem_tracker_create(size_t max_entries) {
    btc_mem_tracker_t* tracker = calloc(1, sizeof(btc_mem_tracker_t));
    if (!tracker) {
        return NULL;
    }
    
    tracker->max_entries = (max_entries > 0) ? max_entries : BTC_MEM_DEFAULT_MAX_ENTRIES;
    
    if (pthread_mutex_init(&tracker->mutex, NULL) != 0) {
        free(tracker);
        return NULL;
    }
    
    tracker->initialized = true;
    
    fprintf(stderr, "[MEM] Tracker créé (max_entries=%zu)\n", tracker->max_entries);
    
    return tracker;
}

void btc_mem_tracker_destroy(btc_mem_tracker_t* tracker) {
    if (!tracker || !tracker->initialized) {
        return;
    }
    
    pthread_mutex_lock(&tracker->mutex);
    
    for (size_t i = 0; i < BTC_MEM_HASH_SIZE; i++) {
        btc_mem_node_t* node = tracker->hash_table[i];
        while (node) {
            btc_mem_node_t* next = node->next;
            free(node);
            node = next;
        }
    }
    
    tracker->initialized = false;
    pthread_mutex_unlock(&tracker->mutex);
    pthread_mutex_destroy(&tracker->mutex);
    
    free(tracker);
    
    fprintf(stderr, "[MEM] Tracker détruit\n");
}

bool btc_mem_track_alloc_ram(
    btc_mem_tracker_t* tracker,
    void* ptr,
    size_t size,
    btc_mem_type_t type,
    const char* file,
    int line
) {
    if (!tracker || !tracker->initialized || !ptr) {
        return false;
    }
    
    pthread_mutex_lock(&tracker->mutex);
    
    btc_mem_entry_t entry = {
        .ptr = ptr,
        .size = size,
        .type = type,
        .timestamp_ns = get_timestamp_ns(),
        .gem_handle = 0,
        .file = file,
        .line = line,
        .freed = false
    };
    
    bool success = add_entry(tracker, &entry);
    
    if (success) {
        tracker->stats.ram_alloc_count++;
        tracker->stats.ram_current_bytes += size;
        tracker->stats.ram_total_bytes += size;
        
        if (tracker->stats.ram_current_bytes > tracker->stats.ram_peak_bytes) {
            tracker->stats.ram_peak_bytes = tracker->stats.ram_current_bytes;
        }
    }
    
    pthread_mutex_unlock(&tracker->mutex);
    
    return success;
}

bool btc_mem_track_free_ram(
    btc_mem_tracker_t* tracker,
    void* ptr,
    const char* file,
    int line
) {
    if (!tracker || !tracker->initialized || !ptr) {
        return false;
    }
    
    pthread_mutex_lock(&tracker->mutex);
    
    btc_mem_node_t* node = find_entry_by_ptr(tracker, ptr);
    
    if (!node) {
        fprintf(stderr, "[MEM] WARNING: free() unknown pointer %p at %s:%d\n", 
                ptr, file, line);
        pthread_mutex_unlock(&tracker->mutex);
        return false;
    }
    
    if (node->entry.freed) {
        fprintf(stderr, "[MEM] ERROR: Double free() %p at %s:%d (first at %s:%d)\n",
                ptr, file, line, node->entry.file, node->entry.line);
        pthread_mutex_unlock(&tracker->mutex);
        return false;
    }
    
    node->entry.freed = true;
    tracker->stats.ram_free_count++;
    tracker->stats.ram_current_bytes -= node->entry.size;
    
    pthread_mutex_unlock(&tracker->mutex);
    
    return true;
}

bool btc_mem_track_alloc_gpu(
    btc_mem_tracker_t* tracker,
    uint32_t gem_handle,
    size_t size,
    const char* file,
    int line
) {
    if (!tracker || !tracker->initialized || gem_handle == 0) {
        return false;
    }
    
    pthread_mutex_lock(&tracker->mutex);
    
    btc_mem_entry_t entry = {
        .ptr = NULL,
        .size = size,
        .type = BTC_MEM_TYPE_GPU_GEM,
        .timestamp_ns = get_timestamp_ns(),
        .gem_handle = gem_handle,
        .file = file,
        .line = line,
        .freed = false
    };
    
    bool success = add_entry(tracker, &entry);
    
    if (success) {
        tracker->stats.gpu_alloc_count++;
        tracker->stats.gpu_current_bytes += size;
        tracker->stats.gpu_total_bytes += size;
        
        if (tracker->stats.gpu_current_bytes > tracker->stats.gpu_peak_bytes) {
            tracker->stats.gpu_peak_bytes = tracker->stats.gpu_current_bytes;
        }
    }
    
    pthread_mutex_unlock(&tracker->mutex);
    
    return success;
}

bool btc_mem_track_free_gpu(
    btc_mem_tracker_t* tracker,
    uint32_t gem_handle,
    const char* file,
    int line
) {
    if (!tracker || !tracker->initialized || gem_handle == 0) {
        return false;
    }
    
    pthread_mutex_lock(&tracker->mutex);
    
    btc_mem_node_t* node = find_entry_by_gem(tracker, gem_handle);
    
    if (!node) {
        fprintf(stderr, "[MEM] WARNING: GEM_CLOSE unknown handle %u at %s:%d\n",
                gem_handle, file, line);
        pthread_mutex_unlock(&tracker->mutex);
        return false;
    }
    
    if (node->entry.freed) {
        fprintf(stderr, "[MEM] ERROR: Double GEM_CLOSE handle %u at %s:%d (first at %s:%d)\n",
                gem_handle, file, line, node->entry.file, node->entry.line);
        pthread_mutex_unlock(&tracker->mutex);
        return false;
    }
    
    node->entry.freed = true;
    tracker->stats.gpu_free_count++;
    tracker->stats.gpu_current_bytes -= node->entry.size;
    
    pthread_mutex_unlock(&tracker->mutex);
    
    return true;
}

bool btc_mem_track_mmap_gpu(
    btc_mem_tracker_t* tracker,
    void* ptr,
    uint32_t gem_handle,
    size_t size,
    const char* file,
    int line
) {
    if (!tracker || !tracker->initialized || !ptr) {
        return false;
    }
    
    pthread_mutex_lock(&tracker->mutex);
    
    btc_mem_entry_t entry = {
        .ptr = ptr,
        .size = size,
        .type = BTC_MEM_TYPE_GPU_MMAP,
        .timestamp_ns = get_timestamp_ns(),
        .gem_handle = gem_handle,
        .file = file,
        .line = line,
        .freed = false
    };
    
    bool success = add_entry(tracker, &entry);
    
    pthread_mutex_unlock(&tracker->mutex);
    
    return success;
}

bool btc_mem_track_munmap_gpu(
    btc_mem_tracker_t* tracker,
    void* ptr,
    const char* file,
    int line
) {
    if (!tracker || !tracker->initialized || !ptr) {
        return false;
    }
    
    pthread_mutex_lock(&tracker->mutex);
    
    btc_mem_node_t* node = find_entry_by_ptr(tracker, ptr);
    
    if (!node) {
        fprintf(stderr, "[MEM] WARNING: munmap() unknown pointer %p at %s:%d\n",
                ptr, file, line);
        pthread_mutex_unlock(&tracker->mutex);
        return false;
    }
    
    if (node->entry.freed) {
        fprintf(stderr, "[MEM] ERROR: Double munmap() %p at %s:%d (first at %s:%d)\n",
                ptr, file, line, node->entry.file, node->entry.line);
        pthread_mutex_unlock(&tracker->mutex);
        return false;
    }
    
    node->entry.freed = true;
    
    pthread_mutex_unlock(&tracker->mutex);
    
    return true;
}

bool btc_mem_get_stats(
    btc_mem_tracker_t* tracker,
    btc_mem_stats_t* stats
) {
    if (!tracker || !tracker->initialized || !stats) {
        return false;
    }
    
    pthread_mutex_lock(&tracker->mutex);
    memcpy(stats, &tracker->stats, sizeof(btc_mem_stats_t));
    pthread_mutex_unlock(&tracker->mutex);
    
    return true;
}

uint64_t btc_mem_detect_leaks(btc_mem_tracker_t* tracker) {
    if (!tracker || !tracker->initialized) {
        return 0;
    }
    
    pthread_mutex_lock(&tracker->mutex);
    
    tracker->stats.ram_leaks_count = 0;
    tracker->stats.ram_leaks_bytes = 0;
    tracker->stats.gpu_leaks_count = 0;
    tracker->stats.gpu_leaks_bytes = 0;
    
    for (size_t i = 0; i < BTC_MEM_HASH_SIZE; i++) {
        btc_mem_node_t* node = tracker->hash_table[i];
        while (node) {
            if (!node->entry.freed) {
                if (node->entry.type == BTC_MEM_TYPE_GPU_GEM) {
                    tracker->stats.gpu_leaks_count++;
                    tracker->stats.gpu_leaks_bytes += node->entry.size;
                } else if (node->entry.type != BTC_MEM_TYPE_GPU_MMAP) {
                    tracker->stats.ram_leaks_count++;
                    tracker->stats.ram_leaks_bytes += node->entry.size;
                }
            }
            node = node->next;
        }
    }
    
    uint64_t total_leaks = tracker->stats.ram_leaks_count + tracker->stats.gpu_leaks_count;
    
    pthread_mutex_unlock(&tracker->mutex);
    
    return total_leaks;
}

void btc_mem_print_leaks(btc_mem_tracker_t* tracker, size_t max_display) {
    if (!tracker || !tracker->initialized) {
        return;
    }
    
    uint64_t total_leaks = btc_mem_detect_leaks(tracker);
    
    if (total_leaks == 0) {
        fprintf(stderr, "\n[MEM] ✅ Aucune fuite mémoire détectée\n");
        return;
    }
    
    fprintf(stderr, "\n[MEM] ❌ FUITES MÉMOIRE DÉTECTÉES: %lu\n", total_leaks);
    fprintf(stderr, "═══════════════════════════════════════════════════════════\n");
    
    pthread_mutex_lock(&tracker->mutex);
    
    size_t displayed = 0;
    
    for (size_t i = 0; i < BTC_MEM_HASH_SIZE && (max_display == 0 || displayed < max_display); i++) {
        btc_mem_node_t* node = tracker->hash_table[i];
        while (node && (max_display == 0 || displayed < max_display)) {
            if (!node->entry.freed) {
                const char* type_str;
                switch (node->entry.type) {
                    case BTC_MEM_TYPE_RAM_MALLOC: type_str = "malloc"; break;
                    case BTC_MEM_TYPE_RAM_CALLOC: type_str = "calloc"; break;
                    case BTC_MEM_TYPE_RAM_REALLOC: type_str = "realloc"; break;
                    case BTC_MEM_TYPE_GPU_GEM: type_str = "GEM"; break;
                    case BTC_MEM_TYPE_GPU_MMAP: type_str = "mmap"; break;
                    default: type_str = "unknown"; break;
                }
                
                if (node->entry.type == BTC_MEM_TYPE_GPU_GEM) {
                    fprintf(stderr, "[%zu] %s: handle=%u, size=%zu bytes at %s:%d\n",
                            displayed + 1, type_str, node->entry.gem_handle,
                            node->entry.size, node->entry.file, node->entry.line);
                } else if (node->entry.type != BTC_MEM_TYPE_GPU_MMAP) {
                    fprintf(stderr, "[%zu] %s: ptr=%p, size=%zu bytes at %s:%d\n",
                            displayed + 1, type_str, node->entry.ptr,
                            node->entry.size, node->entry.file, node->entry.line);
                }
                
                displayed++;
            }
            node = node->next;
        }
    }
    
    pthread_mutex_unlock(&tracker->mutex);
    
    fprintf(stderr, "═══════════════════════════════════════════════════════════\n");
}

void btc_mem_print_stats(btc_mem_tracker_t* tracker) {
    if (!tracker || !tracker->initialized) {
        return;
    }
    
    btc_mem_stats_t stats;
    btc_mem_get_stats(tracker, &stats);
    
    fprintf(stderr, "\n[MEM] STATISTIQUES MÉMOIRE\n");
    fprintf(stderr, "═══════════════════════════════════════════════════════════\n");
    fprintf(stderr, "RAM:\n");
    fprintf(stderr, "  Allocations:  %lu\n", stats.ram_alloc_count);
    fprintf(stderr, "  Libérations:  %lu\n", stats.ram_free_count);
    fprintf(stderr, "  Actuelle:     %lu bytes (%.2f MB)\n", 
            stats.ram_current_bytes, stats.ram_current_bytes / 1024.0 / 1024.0);
    fprintf(stderr, "  Pic:          %lu bytes (%.2f MB)\n",
            stats.ram_peak_bytes, stats.ram_peak_bytes / 1024.0 / 1024.0);
    fprintf(stderr, "  Totale:       %lu bytes (%.2f MB)\n",
            stats.ram_total_bytes, stats.ram_total_bytes / 1024.0 / 1024.0);
    fprintf(stderr, "\nGPU:\n");
    fprintf(stderr, "  Allocations:  %lu\n", stats.gpu_alloc_count);
    fprintf(stderr, "  Libérations:  %lu\n", stats.gpu_free_count);
    fprintf(stderr, "  Actuelle:     %lu bytes (%.2f MB)\n",
            stats.gpu_current_bytes, stats.gpu_current_bytes / 1024.0 / 1024.0);
    fprintf(stderr, "  Pic:          %lu bytes (%.2f MB)\n",
            stats.gpu_peak_bytes, stats.gpu_peak_bytes / 1024.0 / 1024.0);
    fprintf(stderr, "  Totale:       %lu bytes (%.2f MB)\n",
            stats.gpu_total_bytes, stats.gpu_total_bytes / 1024.0 / 1024.0);
    fprintf(stderr, "\nFuites:\n");
    fprintf(stderr, "  RAM:          %lu (%lu bytes)\n",
            stats.ram_leaks_count, stats.ram_leaks_bytes);
    fprintf(stderr, "  GPU:          %lu (%lu bytes)\n",
            stats.gpu_leaks_count, stats.gpu_leaks_bytes);
    fprintf(stderr, "═══════════════════════════════════════════════════════════\n");
}

bool btc_mem_export_lum(
    btc_mem_tracker_t* tracker,
    const char* lum_path
) {
    if (!tracker || !tracker->initialized || !lum_path) {
        return false;
    }
    
    fprintf(stderr, "[MEM] Export .lum non implémenté (TODO Phase 15X)\n");
    return false;
}

void btc_mem_reset(btc_mem_tracker_t* tracker) {
    if (!tracker || !tracker->initialized) {
        return;
    }
    
    pthread_mutex_lock(&tracker->mutex);
    
    for (size_t i = 0; i < BTC_MEM_HASH_SIZE; i++) {
        btc_mem_node_t* node = tracker->hash_table[i];
        while (node) {
            btc_mem_node_t* next = node->next;
            free(node);
            node = next;
        }
        tracker->hash_table[i] = NULL;
    }
    
    memset(&tracker->stats, 0, sizeof(btc_mem_stats_t));
    tracker->current_entries = 0;
    
    pthread_mutex_unlock(&tracker->mutex);
    
    fprintf(stderr, "[MEM] Tracker réinitialisé\n");
}

// Made with Bob
