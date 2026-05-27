/**
 * LumVorax C255v3 — i915 DRM Forensic Tracker BIT-LEVEL
 * 
 * OBJECTIF: Traçabilité TOTALE bit par bit, nanoseconde par nanoseconde
 * - Intercepter TOUS les ioctl i915 DRM
 * - Logger TOUS les paramètres (structures complètes)
 * - Dumper TOUS les buffers (contenu hexadécimal)
 * - Mesurer TOUS les timings (précision nanoseconde)
 * - Tracer TOUTES les dépendances (GEM fences, implicit sync)
 * 
 * GRANULARITÉ MAXIMALE:
 * - Chaque appel système (ioctl, mmap, munmap)
 * - Chaque buffer object (création, destruction, mapping)
 * - Chaque batch buffer (contenu complet)
 * - Chaque fence (création, attente, signalisation)
 * - Chaque contexte (création, destruction, switch)
 * 
 * AUCUNE LIMITE:
 * - Taille fichiers logs: illimitée
 * - Latence: acceptée (priorité = traçabilité)
 * - Verbosité: maximale
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/syscall.h>

#include <drm/i915_drm.h>
#include <xf86drm.h>

#include "btc_i915_drm_forensic_tracker.h"

/* ══════════════════════════════════════════════════════════════════════
 * SECTION 1: STRUCTURES FORENSIQUES
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * Événement forensique i915 DRM
 */
typedef struct {
    uint64_t timestamp_ns;          /* Timestamp nanoseconde */
    uint64_t thread_id;             /* ID thread Linux */
    uint64_t sequence_id;           /* Numéro séquence global */
    
    /* Type événement */
    enum {
        EVENT_IOCTL_BEFORE,         /* Avant appel ioctl */
        EVENT_IOCTL_AFTER,          /* Après appel ioctl */
        EVENT_MMAP_BEFORE,          /* Avant mmap */
        EVENT_MMAP_AFTER,           /* Après mmap */
        EVENT_MUNMAP,               /* munmap */
        EVENT_BUFFER_DUMP,          /* Dump contenu buffer */
        EVENT_FENCE_CREATE,         /* Création fence */
        EVENT_FENCE_WAIT,           /* Attente fence */
        EVENT_FENCE_SIGNAL,         /* Signalisation fence */
        EVENT_CONTEXT_SWITCH,       /* Switch contexte GPU */
        EVENT_BATCH_SUBMIT,         /* Soumission batch buffer */
        EVENT_GPU_HANG,             /* Détection GPU hang */
        EVENT_MEMORY_PRESSURE,      /* Pression mémoire */
    } event_type;
    
    /* Détails ioctl */
    unsigned long ioctl_request;    /* Numéro requête ioctl */
    const char* ioctl_name;         /* Nom ioctl (lisible) */
    int ioctl_result;               /* Code retour ioctl */
    int ioctl_errno;                /* errno si erreur */
    
    /* Détails buffer */
    uint32_t bo_handle;             /* Handle buffer object */
    uint64_t bo_size;               /* Taille buffer */
    void* bo_addr;                  /* Adresse mapping CPU */
    
    /* Détails contexte */
    uint32_t ctx_id;                /* ID contexte i915 */
    
    /* Données brutes (structures ioctl complètes) */
    void* raw_data;                 /* Pointeur données brutes */
    size_t raw_data_size;           /* Taille données */
    
    /* Dump hexadécimal buffer */
    uint8_t* buffer_dump;           /* Dump complet buffer */
    size_t buffer_dump_size;        /* Taille dump */
    
} i915_forensic_event_t;

/**
 * Tracker forensique global
 */
typedef struct {
    FILE* log_file;                 /* Fichier log principal */
    FILE* hex_dump_file;            /* Fichier dumps hexadécimaux */
    FILE* timeline_file;            /* Fichier timeline événements */
    
    pthread_mutex_t mutex;          /* Mutex thread-safe */
    
    uint64_t sequence_counter;      /* Compteur séquence global */
    uint64_t total_events;          /* Total événements tracés */
    uint64_t total_bytes_logged;    /* Total bytes loggés */
    
    /* Statistiques par type ioctl */
    uint64_t ioctl_counts[256];     /* Compteurs par ioctl */
    uint64_t ioctl_times_ns[256];   /* Temps cumulés par ioctl */
    
    /* Tracking buffer objects */
    struct {
        uint32_t handle;
        uint64_t size;
        void* addr;
        uint64_t create_ts;
        uint64_t last_access_ts;
        int ref_count;
    } bo_tracking[1024];            /* Max 1024 BOs simultanés */
    int bo_count;
    
    /* Tracking contextes */
    struct {
        uint32_t ctx_id;
        uint64_t create_ts;
        uint64_t usage_count;
        uint64_t total_exec_time_ns;
    } ctx_tracking[16];             /* Max 16 contextes */
    int ctx_count;
    
    /* Flags configuration */
    int enable_hex_dumps;           /* Activer dumps hexadécimaux */
    int enable_buffer_tracking;     /* Activer tracking buffers */
    int enable_timeline;            /* Activer timeline */
    int enable_full_structs;        /* Logger structures complètes */
    
} i915_forensic_tracker_t;

/* Tracker global */
static i915_forensic_tracker_t g_tracker = {0};
static int g_tracker_initialized = 0;

/* ══════════════════════════════════════════════════════════════════════
 * SECTION 2: UTILITAIRES TIMESTAMP
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * Obtenir timestamp nanoseconde (CLOCK_MONOTONIC_RAW)
 */
static inline uint64_t get_timestamp_ns_raw(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

/**
 * Obtenir ID thread Linux
 */
static inline uint64_t get_thread_id(void) {
    return (uint64_t)syscall(SYS_gettid);
}

/* ══════════════════════════════════════════════════════════════════════
 * SECTION 3: NOMS IOCTL LISIBLES
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * Convertir numéro ioctl en nom lisible
 */
static const char* ioctl_name(unsigned long request) {
    switch (request) {
        case DRM_IOCTL_I915_GEM_CREATE: return "GEM_CREATE";
        case DRM_IOCTL_I915_GEM_MMAP: return "GEM_MMAP";
        case DRM_IOCTL_I915_GEM_MMAP_OFFSET: return "GEM_MMAP_OFFSET";
        case DRM_IOCTL_I915_GEM_EXECBUFFER2: return "GEM_EXECBUFFER2";
        case DRM_IOCTL_I915_GEM_WAIT: return "GEM_WAIT";
        case DRM_IOCTL_I915_GEM_CONTEXT_CREATE: return "GEM_CONTEXT_CREATE";
        case DRM_IOCTL_I915_GEM_CONTEXT_DESTROY: return "GEM_CONTEXT_DESTROY";
        case DRM_IOCTL_GEM_CLOSE: return "GEM_CLOSE";
        case DRM_IOCTL_VERSION: return "VERSION";
        default: return "UNKNOWN";
    }
}

/* ══════════════════════════════════════════════════════════════════════
 * SECTION 4: DUMP HEXADÉCIMAL
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * Dump hexadécimal buffer (format: offset | hex | ascii)
 */
static void hex_dump(FILE* f, const void* data, size_t size, uint64_t base_offset) {
    const uint8_t* bytes = (const uint8_t*)data;
    
    for (size_t i = 0; i < size; i += 16) {
        /* Offset */
        fprintf(f, "%08lx | ", base_offset + i);
        
        /* Hexadécimal */
        for (size_t j = 0; j < 16; j++) {
            if (i + j < size) {
                fprintf(f, "%02x ", bytes[i + j]);
            } else {
                fprintf(f, "   ");
            }
        }
        
        fprintf(f, "| ");
        
        /* ASCII */
        for (size_t j = 0; j < 16 && i + j < size; j++) {
            uint8_t c = bytes[i + j];
            fprintf(f, "%c", (c >= 32 && c < 127) ? c : '.');
        }
        
        fprintf(f, "\n");
    }
}

/* ══════════════════════════════════════════════════════════════════════
 * SECTION 5: INITIALISATION/CLEANUP
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * Initialiser tracker forensique
 */
int i915_forensic_tracker_init(const char* log_dir) {
    if (g_tracker_initialized) {
        return 0;  /* Déjà initialisé */
    }
    
    memset(&g_tracker, 0, sizeof(g_tracker));
    
    /* Créer fichiers logs */
    char path[512];
    
    snprintf(path, sizeof(path), "%s/i915_forensic_main.log", log_dir);
    g_tracker.log_file = fopen(path, "w");
    if (!g_tracker.log_file) {
        fprintf(stderr, "[FORENSIC] ERREUR: Impossible créer %s\n", path);
        return -1;
    }
    
    snprintf(path, sizeof(path), "%s/i915_forensic_hexdump.log", log_dir);
    g_tracker.hex_dump_file = fopen(path, "w");
    
    snprintf(path, sizeof(path), "%s/i915_forensic_timeline.log", log_dir);
    g_tracker.timeline_file = fopen(path, "w");
    
    /* Initialiser mutex */
    pthread_mutex_init(&g_tracker.mutex, NULL);
    
    /* Configuration par défaut: TOUT activer */
    g_tracker.enable_hex_dumps = 1;
    g_tracker.enable_buffer_tracking = 1;
    g_tracker.enable_timeline = 1;
    g_tracker.enable_full_structs = 1;
    
    /* Header fichier principal */
    fprintf(g_tracker.log_file, "# LumVorax i915 DRM Forensic Tracker — BIT-LEVEL GRANULARITY\n");
    fprintf(g_tracker.log_file, "# Timestamp: %lu ns\n", get_timestamp_ns_raw());
    fprintf(g_tracker.log_file, "# Thread: %lu\n", get_thread_id());
    fprintf(g_tracker.log_file, "# Configuration: hex_dumps=%d buffer_tracking=%d timeline=%d full_structs=%d\n\n",
            g_tracker.enable_hex_dumps, g_tracker.enable_buffer_tracking,
            g_tracker.enable_timeline, g_tracker.enable_full_structs);
    fflush(g_tracker.log_file);
    
    g_tracker_initialized = 1;
    
    fprintf(stderr, "[FORENSIC] Tracker initialisé: %s\n", log_dir);
    return 0;
}

/**
 * Cleanup tracker
 */
void i915_forensic_tracker_cleanup(void) {
    if (!g_tracker_initialized) {
        return;
    }
    
    pthread_mutex_lock(&g_tracker.mutex);
    
    /* Statistiques finales */
    fprintf(g_tracker.log_file, "\n# === STATISTIQUES FINALES ===\n");
    fprintf(g_tracker.log_file, "# Total événements: %lu\n", g_tracker.total_events);
    fprintf(g_tracker.log_file, "# Total bytes loggés: %lu\n", g_tracker.total_bytes_logged);
    fprintf(g_tracker.log_file, "# Buffer objects créés: %d\n", g_tracker.bo_count);
    fprintf(g_tracker.log_file, "# Contextes créés: %d\n", g_tracker.ctx_count);
    
    fprintf(g_tracker.log_file, "\n# Statistiques ioctl:\n");
    for (int i = 0; i < 256; i++) {
        if (g_tracker.ioctl_counts[i] > 0) {
            fprintf(g_tracker.log_file, "# ioctl_%d: count=%lu time_total=%lu ns time_avg=%lu ns\n",
                    i, g_tracker.ioctl_counts[i], g_tracker.ioctl_times_ns[i],
                    g_tracker.ioctl_times_ns[i] / g_tracker.ioctl_counts[i]);
        }
    }
    
    /* Fermer fichiers */
    if (g_tracker.log_file) fclose(g_tracker.log_file);
    if (g_tracker.hex_dump_file) fclose(g_tracker.hex_dump_file);
    if (g_tracker.timeline_file) fclose(g_tracker.timeline_file);
    
    pthread_mutex_unlock(&g_tracker.mutex);
    pthread_mutex_destroy(&g_tracker.mutex);
    
    g_tracker_initialized = 0;
    
    fprintf(stderr, "[FORENSIC] Tracker nettoyé\n");
}

/* ══════════════════════════════════════════════════════════════════════
 * SECTION 6: LOGGING ÉVÉNEMENTS
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * Logger événement forensique
 */
static void log_event(i915_forensic_event_t* event) {
    if (!g_tracker_initialized || !g_tracker.log_file) {
        return;
    }
    
    pthread_mutex_lock(&g_tracker.mutex);
    
    /* Assigner numéro séquence */
    event->sequence_id = g_tracker.sequence_counter++;
    g_tracker.total_events++;
    
    /* Logger événement principal */
    fprintf(g_tracker.log_file, "[%020lu] SEQ=%lu TID=%lu TYPE=%d ",
            event->timestamp_ns, event->sequence_id, event->thread_id, event->event_type);
    
    switch (event->event_type) {
        case EVENT_IOCTL_BEFORE:
            fprintf(g_tracker.log_file, "IOCTL_BEFORE: %s (0x%lx)\n",
                    event->ioctl_name, event->ioctl_request);
            break;
            
        case EVENT_IOCTL_AFTER:
            fprintf(g_tracker.log_file, "IOCTL_AFTER: %s result=%d errno=%d\n",
                    event->ioctl_name, event->ioctl_result, event->ioctl_errno);
            break;
            
        case EVENT_BUFFER_DUMP:
            fprintf(g_tracker.log_file, "BUFFER_DUMP: bo=%u size=%lu addr=%p\n",
                    event->bo_handle, event->bo_size, event->bo_addr);
            break;
            
        default:
            fprintf(g_tracker.log_file, "\n");
            break;
    }
    
    /* Logger structure complète si activé */
    if (g_tracker.enable_full_structs && event->raw_data && event->raw_data_size > 0) {
        fprintf(g_tracker.log_file, "  RAW_STRUCT: size=%zu bytes\n", event->raw_data_size);
        hex_dump(g_tracker.log_file, event->raw_data, event->raw_data_size, 0);
        g_tracker.total_bytes_logged += event->raw_data_size;
    }
    
    /* Dump hexadécimal buffer si activé */
    if (g_tracker.enable_hex_dumps && event->buffer_dump && event->buffer_dump_size > 0 && g_tracker.hex_dump_file) {
        fprintf(g_tracker.hex_dump_file, "\n[%020lu] SEQ=%lu BUFFER_DUMP: bo=%u size=%lu\n",
                event->timestamp_ns, event->sequence_id, event->bo_handle, event->buffer_dump_size);
        hex_dump(g_tracker.hex_dump_file, event->buffer_dump, event->buffer_dump_size, 0);
        fflush(g_tracker.hex_dump_file);
        g_tracker.total_bytes_logged += event->buffer_dump_size;
    }
    
    /* Timeline si activé */
    if (g_tracker.enable_timeline && g_tracker.timeline_file) {
        fprintf(g_tracker.timeline_file, "%lu,%lu,%lu,%d,%s\n",
                event->timestamp_ns, event->sequence_id, event->thread_id,
                event->event_type, event->ioctl_name ? event->ioctl_name : "");
    }
    
    fflush(g_tracker.log_file);
    
    pthread_mutex_unlock(&g_tracker.mutex);
}

/* ══════════════════════════════════════════════════════════════════════
 * SECTION 7: WRAPPERS IOCTL FORENSIQUES
 * ══════════════════════════════════════════════════════════════════════ */

/**
 * Wrapper ioctl forensique (trace AVANT et APRÈS)
 */
int i915_forensic_ioctl_impl(int fd, unsigned long request, ...) {
    /* Désactiver macro temporairement pour appeler vrai ioctl */
    #undef ioctl
    
    /* Extraire argument variadic */
    va_list args;
    va_start(args, request);
    void* arg = va_arg(args, void*);
    va_end(args);
    
    if (!g_tracker_initialized) {
        return ioctl(fd, request, arg);
    }
    
    /* Événement AVANT */
    i915_forensic_event_t event_before = {
        .timestamp_ns = get_timestamp_ns_raw(),
        .thread_id = get_thread_id(),
        .event_type = EVENT_IOCTL_BEFORE,
        .ioctl_request = request,
        .ioctl_name = ioctl_name(request),
    };
    log_event(&event_before);
    
    /* Appel ioctl réel (vrai syscall système) */
    uint64_t ts_start = get_timestamp_ns_raw();
    int result = ioctl(fd, request, arg);  /* Vrai ioctl système */
    uint64_t ts_end = get_timestamp_ns_raw();
    int saved_errno = errno;
    
    /* Réactiver macro pour futurs appels */
    #define ioctl i915_forensic_ioctl_impl
    
    /* Événement APRÈS */
    i915_forensic_event_t event_after = {
        .timestamp_ns = ts_end,
        .thread_id = get_thread_id(),
        .event_type = EVENT_IOCTL_AFTER,
        .ioctl_request = request,
        .ioctl_name = ioctl_name(request),
        .ioctl_result = result,
        .ioctl_errno = (result < 0) ? saved_errno : 0,
    };
    
    /* Logger structure ioctl complète */
    if (g_tracker.enable_full_structs) {
        size_t struct_size = 0;
        
        /* Déterminer taille structure selon ioctl */
        switch (request) {
            case DRM_IOCTL_I915_GEM_CREATE:
                struct_size = sizeof(struct drm_i915_gem_create);
                break;
            case DRM_IOCTL_I915_GEM_EXECBUFFER2:
                struct_size = sizeof(struct drm_i915_gem_execbuffer2);
                break;
            case DRM_IOCTL_I915_GEM_WAIT:
                struct_size = sizeof(struct drm_i915_gem_wait);
                break;
            case DRM_IOCTL_I915_GEM_CONTEXT_CREATE:
                struct_size = sizeof(struct drm_i915_gem_context_create);
                break;
            default:
                struct_size = 256;  /* Taille par défaut */
                break;
        }
        
        event_after.raw_data = arg;
        event_after.raw_data_size = struct_size;
    }
    
    log_event(&event_after);
    
    /* Statistiques */
    pthread_mutex_lock(&g_tracker.mutex);
    uint8_t ioctl_idx = (request >> 8) & 0xFF;
    g_tracker.ioctl_counts[ioctl_idx]++;
    g_tracker.ioctl_times_ns[ioctl_idx] += (ts_end - ts_start);
    pthread_mutex_unlock(&g_tracker.mutex);
    
    errno = saved_errno;
    return result;
}

/**
 * Dump buffer object complet
 */
void i915_forensic_dump_buffer(uint32_t bo_handle, void* addr, size_t size) {
    if (!g_tracker_initialized || !g_tracker.enable_hex_dumps) {
        return;
    }
    
    i915_forensic_event_t event = {
        .timestamp_ns = get_timestamp_ns_raw(),
        .thread_id = get_thread_id(),
        .event_type = EVENT_BUFFER_DUMP,
        .bo_handle = bo_handle,
        .bo_size = size,
        .bo_addr = addr,
        .buffer_dump = (uint8_t*)addr,
        .buffer_dump_size = size,
    };
    
    log_event(&event);
}

// Made with Bob