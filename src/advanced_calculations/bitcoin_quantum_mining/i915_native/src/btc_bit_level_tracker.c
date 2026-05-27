/**
 * LumVorax C198 Phase 15U — Bit-Level Tracker Implementation
 * 
 * Traçabilité bit-level complète pour i915 DRM native
 * Format .lum : Header + Événements binaires
 * 
 * Objectif : Tracker TOUTES les opérations GPU/RAM au niveau bit
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>

#include "btc_bit_level_tracker.h"

// ═══════════════════════════════════════════════════════════════════════════
// FONCTIONS UTILITAIRES
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Obtenir timestamp actuel (nanosec depuis epoch)
 */
uint64_t btc_lum_get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

/**
 * Écrire données dans fichier .lum (avec vérification)
 */
static bool btc_lum_write_data(btc_lum_tracker *tracker, const void *data, size_t size) {
    if (!tracker || !tracker->is_open || tracker->fd < 0) {
        return false;
    }
    
    ssize_t written = write(tracker->fd, data, size);
    if (written != (ssize_t)size) {
        fprintf(stderr, "[LUM] ERROR: write() failed: %s\n", strerror(errno));
        return false;
    }
    
    return true;
}

/**
 * Logger événement générique
 */
static void btc_lum_log_event(btc_lum_tracker *tracker, btc_lum_event_type type,
                              const void *data, uint32_t data_size) {
    if (!tracker || !tracker->is_open) {
        return;
    }
    
    // Créer header événement
    btc_lum_event_header event_header = {
        .timestamp = btc_lum_get_timestamp_ns(),
        .event_type = (uint8_t)type,
        .reserved = {0, 0, 0},
        .data_size = data_size
    };
    
    // Écrire header événement
    if (!btc_lum_write_data(tracker, &event_header, sizeof(event_header))) {
        return;
    }
    
    // Écrire données événement (si présentes)
    if (data && data_size > 0) {
        if (!btc_lum_write_data(tracker, data, data_size)) {
            return;
        }
    }
    
    // Incrémenter compteur
    tracker->event_count++;
}

// ═══════════════════════════════════════════════════════════════════════════
// API PUBLIQUE — GESTION TRACKER
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Créer tracker et ouvrir fichier .lum
 */
btc_lum_tracker* btc_lum_create(const char *filepath) {
    if (!filepath) {
        fprintf(stderr, "[LUM] ERROR: filepath is NULL\n");
        return NULL;
    }
    
    // Allouer tracker
    btc_lum_tracker *tracker = (btc_lum_tracker*)calloc(1, sizeof(btc_lum_tracker));
    if (!tracker) {
        fprintf(stderr, "[LUM] ERROR: calloc() failed\n");
        return NULL;
    }
    
    // Copier chemin fichier
    strncpy(tracker->filepath, filepath, BTC_LUM_MAX_PATH - 1);
    tracker->filepath[BTC_LUM_MAX_PATH - 1] = '\0';
    
    // Ouvrir fichier .lum (création, troncature, écriture seule)
    tracker->fd = open(filepath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (tracker->fd < 0) {
        fprintf(stderr, "[LUM] ERROR: open(%s) failed: %s\n", filepath, strerror(errno));
        free(tracker);
        return NULL;
    }
    
    tracker->is_open = true;
    tracker->event_count = 0;
    tracker->start_time_ns = btc_lum_get_timestamp_ns();
    
    // Initialiser header
    tracker->header.magic = BTC_LUM_MAGIC;
    tracker->header.version = BTC_LUM_VERSION;
    tracker->header.timestamp_start = tracker->start_time_ns;
    tracker->header.timestamp_end = 0;  // Sera mis à jour à la fermeture
    strncpy(tracker->header.module, "i915_native", 16);
    strncpy(tracker->header.arch, "Gen9", 16);
    tracker->header.event_count = 0;  // Sera mis à jour à la fermeture
    tracker->header.reserved = 0;
    
    // Écrire header (sera réécrit à la fermeture avec valeurs finales)
    if (!btc_lum_write_data(tracker, &tracker->header, sizeof(tracker->header))) {
        close(tracker->fd);
        free(tracker);
        return NULL;
    }
    
    fprintf(stderr, "[LUM] Tracker créé: %s\n", filepath);
    return tracker;
}

/**
 * Fermer tracker et finaliser fichier .lum
 */
void btc_lum_close(btc_lum_tracker *tracker) {
    if (!tracker) {
        return;
    }
    
    if (tracker->is_open && tracker->fd >= 0) {
        // Mettre à jour header avec valeurs finales
        tracker->header.timestamp_end = btc_lum_get_timestamp_ns();
        tracker->header.event_count = tracker->event_count;
        
        // Réécrire header au début du fichier
        lseek(tracker->fd, 0, SEEK_SET);
        btc_lum_write_data(tracker, &tracker->header, sizeof(tracker->header));
        
        // Fermer fichier
        close(tracker->fd);
        tracker->is_open = false;
        
        uint64_t duration_ns = tracker->header.timestamp_end - tracker->header.timestamp_start;
        double duration_sec = (double)duration_ns / 1000000000.0;
        
        fprintf(stderr, "[LUM] Tracker fermé: %s\n", tracker->filepath);
        fprintf(stderr, "[LUM] Événements: %u\n", tracker->event_count);
        fprintf(stderr, "[LUM] Durée: %.6f sec\n", duration_sec);
    }
    
    free(tracker);
}

// ═══════════════════════════════════════════════════════════════════════════
// API PUBLIQUE — LOGGING ÉVÉNEMENTS
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Logger événement GEM_CREATE
 */
void btc_lum_log_gem_create(btc_lum_tracker *tracker, uint32_t gem_handle, 
                            uint64_t size, uint64_t gpu_addr) {
    btc_lum_gem_create_data data = {
        .gem_handle = gem_handle,
        .size = size,
        .gpu_addr = gpu_addr
    };
    btc_lum_log_event(tracker, BTC_LUM_EVENT_GEM_CREATE, &data, sizeof(data));
}

/**
 * Logger événement GEM_CLOSE
 */
void btc_lum_log_gem_close(btc_lum_tracker *tracker, uint32_t gem_handle) {
    btc_lum_gem_close_data data = {
        .gem_handle = gem_handle
    };
    btc_lum_log_event(tracker, BTC_LUM_EVENT_GEM_CLOSE, &data, sizeof(data));
}

/**
 * Logger événement GPU_MMAP
 */
void btc_lum_log_gpu_mmap(btc_lum_tracker *tracker, uint32_t gem_handle,
                          uint64_t cpu_addr, uint64_t size) {
    btc_lum_gpu_mmap_data data = {
        .gem_handle = gem_handle,
        .cpu_addr = cpu_addr,
        .size = size
    };
    btc_lum_log_event(tracker, BTC_LUM_EVENT_GPU_MMAP, &data, sizeof(data));
}

/**
 * Logger événement GPU_MUNMAP
 */
void btc_lum_log_gpu_munmap(btc_lum_tracker *tracker, uint64_t cpu_addr, 
                            uint64_t size) {
    btc_lum_gpu_munmap_data data = {
        .cpu_addr = cpu_addr,
        .size = size
    };
    btc_lum_log_event(tracker, BTC_LUM_EVENT_GPU_MUNMAP, &data, sizeof(data));
}

/**
 * Logger événement BATCH_WRITE
 */
void btc_lum_log_batch_write(btc_lum_tracker *tracker, uint32_t gem_handle,
                             uint64_t offset, const void *data, uint32_t size) {
    // Créer structure avec header + données
    btc_lum_batch_write_data batch_header = {
        .gem_handle = gem_handle,
        .offset = offset,
        .size = size
    };
    
    // Logger header événement
    btc_lum_event_header event_header = {
        .timestamp = btc_lum_get_timestamp_ns(),
        .event_type = (uint8_t)BTC_LUM_EVENT_BATCH_WRITE,
        .reserved = {0, 0, 0},
        .data_size = sizeof(batch_header) + size
    };
    
    if (!tracker || !tracker->is_open) {
        return;
    }
    
    // Écrire header événement
    btc_lum_write_data(tracker, &event_header, sizeof(event_header));
    
    // Écrire header batch
    btc_lum_write_data(tracker, &batch_header, sizeof(batch_header));
    
    // Écrire données batch
    if (data && size > 0) {
        btc_lum_write_data(tracker, data, size);
    }
    
    tracker->event_count++;
}

/**
 * Logger événement EXECBUFFER2
 */
void btc_lum_log_execbuffer2(btc_lum_tracker *tracker, uint32_t ctx_id,
                             uint32_t gem_handle, uint64_t batch_offset,
                             uint32_t batch_len) {
    btc_lum_execbuffer2_data data = {
        .ctx_id = ctx_id,
        .gem_handle = gem_handle,
        .batch_offset = batch_offset,
        .batch_len = batch_len
    };
    btc_lum_log_event(tracker, BTC_LUM_EVENT_EXECBUFFER2, &data, sizeof(data));
}

/**
 * Logger événement GPU_WAIT
 */
void btc_lum_log_gpu_wait(btc_lum_tracker *tracker, uint32_t ctx_id,
                          uint64_t timeout_ns, int32_t result) {
    btc_lum_gpu_wait_data data = {
        .ctx_id = ctx_id,
        .timeout_ns = timeout_ns,
        .result = result
    };
    btc_lum_log_event(tracker, BTC_LUM_EVENT_GPU_WAIT, &data, sizeof(data));
}

/**
 * Logger événement CTX_CREATE
 */
void btc_lum_log_ctx_create(btc_lum_tracker *tracker, uint32_t ctx_id) {
    btc_lum_ctx_create_data data = {
        .ctx_id = ctx_id
    };
    btc_lum_log_event(tracker, BTC_LUM_EVENT_CTX_CREATE, &data, sizeof(data));
}

/**
 * Logger événement CTX_DESTROY
 */
void btc_lum_log_ctx_destroy(btc_lum_tracker *tracker, uint32_t ctx_id) {
    btc_lum_ctx_destroy_data data = {
        .ctx_id = ctx_id
    };
    btc_lum_log_event(tracker, BTC_LUM_EVENT_CTX_DESTROY, &data, sizeof(data));
}

/**
 * Logger événement RAM_ALLOC
 */
void btc_lum_log_ram_alloc(btc_lum_tracker *tracker, uint64_t ptr, 
                           uint64_t size, const char *location) {
    btc_lum_ram_alloc_data data = {
        .ptr = ptr,
        .size = size
    };
    
    // Copier location (max 63 chars + '\0')
    if (location) {
        strncpy(data.location, location, 63);
        data.location[63] = '\0';
    } else {
        data.location[0] = '\0';
    }
    
    btc_lum_log_event(tracker, BTC_LUM_EVENT_RAM_ALLOC, &data, sizeof(data));
}

/**
 * Logger événement RAM_FREE
 */
void btc_lum_log_ram_free(btc_lum_tracker *tracker, uint64_t ptr) {
    btc_lum_ram_free_data data = {
        .ptr = ptr
    };
    btc_lum_log_event(tracker, BTC_LUM_EVENT_RAM_FREE, &data, sizeof(data));
}

/**
 * Logger événement IOCTL_CALL
 */
void btc_lum_log_ioctl_call(btc_lum_tracker *tracker, uint32_t request, 
                            int32_t fd) {
    btc_lum_ioctl_call_data data = {
        .request = request,
        .fd = fd
    };
    btc_lum_log_event(tracker, BTC_LUM_EVENT_IOCTL_CALL, &data, sizeof(data));
}

/**
 * Logger événement IOCTL_RESULT
 */
void btc_lum_log_ioctl_result(btc_lum_tracker *tracker, uint32_t request,
                              int32_t result, int32_t errno_val) {
    btc_lum_ioctl_result_data data = {
        .request = request,
        .result = result,
        .errno_val = errno_val
    };
    btc_lum_log_event(tracker, BTC_LUM_EVENT_IOCTL_RESULT, &data, sizeof(data));
}

/**
 * Logger événement ERROR
 */
void btc_lum_log_error(btc_lum_tracker *tracker, int32_t errno_val,
                       const char *message) {
    btc_lum_error_data data = {
        .errno_val = errno_val
    };
    
    // Copier message (max 127 chars + '\0')
    if (message) {
        strncpy(data.message, message, 127);
        data.message[127] = '\0';
    } else {
        data.message[0] = '\0';
    }
    
    btc_lum_log_event(tracker, BTC_LUM_EVENT_ERROR, &data, sizeof(data));
}

// Made with Bob
