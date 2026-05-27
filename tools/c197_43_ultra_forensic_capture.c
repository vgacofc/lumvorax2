/*
 * C197.43 - ULTRA FORENSIC CAPTURE - Traçabilité Bit-Level Complète
 * 
 * OBJECTIF : Capturer TOUTES les dépendances OpenCL→i915→GPU
 * - Tous les ioctls DRM (pas seulement EXECBUFFER2)
 * - Tous les GEM buffers créés/mappés/utilisés
 * - Contenu COMPLET de chaque buffer (pas juste metadata)
 * - Timeline nanoseconde de chaque opération
 * - État mémoire GPU avant/après
 * 
 * PRINCIPE : Ne rien ignorer, tout capturer, analyser offline
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <dlfcn.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <drm/i915_drm.h>
#include <drm/drm.h>

#define MAX_BUFFERS 256
#define MAX_EVENTS 10000
#define CAPTURE_DIR "/tmp/c197_43_forensic"

// Structure pour tracker un GEM buffer
typedef struct {
    uint32_t handle;
    uint64_t size;
    void* cpu_ptr;
    uint64_t gpu_offset;
    uint64_t flags;
    int active;
    char name[64];
} gem_buffer_t;

// Structure pour un événement forensique
typedef struct {
    uint64_t timestamp_ns;
    uint32_t ioctl_nr;
    char ioctl_name[64];
    int result;
    int errno_val;
    uint64_t data_size;
    char data_file[256];
} forensic_event_t;

// État global
static int (*real_ioctl)(int fd, unsigned long request, ...) = NULL;
static int (*real_mmap)(void *addr, size_t length, int prot, int flags, int fd, off_t offset) = NULL;
static gem_buffer_t buffers[MAX_BUFFERS] = {0};
static forensic_event_t events[MAX_EVENTS] = {0};
static int buffer_count = 0;
static int event_count = 0;
static int drm_fd = -1;
static FILE* log_file = NULL;

// Obtenir timestamp nanoseconde
static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

// Nom ioctl
static const char* ioctl_name(unsigned long request) {
    switch(request) {
        case DRM_IOCTL_VERSION: return "DRM_VERSION";
        case DRM_IOCTL_GET_MAGIC: return "DRM_GET_MAGIC";
        case DRM_IOCTL_I915_GETPARAM: return "I915_GETPARAM";
        case DRM_IOCTL_I915_GEM_CREATE: return "I915_GEM_CREATE";
        case DRM_IOCTL_I915_GEM_MMAP: return "I915_GEM_MMAP";
        case DRM_IOCTL_I915_GEM_MMAP_OFFSET: return "I915_GEM_MMAP_OFFSET";
        case DRM_IOCTL_I915_GEM_SET_DOMAIN: return "I915_GEM_SET_DOMAIN";
        case DRM_IOCTL_I915_GEM_EXECBUFFER2: return "I915_GEM_EXECBUFFER2";
        case DRM_IOCTL_I915_GEM_WAIT: return "I915_GEM_WAIT";
        case DRM_IOCTL_I915_GEM_CONTEXT_CREATE: return "I915_GEM_CONTEXT_CREATE";
        case DRM_IOCTL_I915_GEM_CONTEXT_DESTROY: return "I915_GEM_CONTEXT_DESTROY";
        case DRM_IOCTL_GEM_CLOSE: return "GEM_CLOSE";
        case DRM_IOCTL_I915_GEM_USERPTR: return "I915_GEM_USERPTR";
        case DRM_IOCTL_I915_GEM_SET_TILING: return "I915_GEM_SET_TILING";
        case DRM_IOCTL_I915_GEM_GET_TILING: return "I915_GEM_GET_TILING";
        default: return "UNKNOWN";
    }
}

// Sauvegarder données binaires
static void save_binary_data(const char* prefix, const void* data, size_t size) {
    if (!data || size == 0) return;
    
    char filename[512];
    snprintf(filename, sizeof(filename), "%s/%s_%d_%lu.bin", 
             CAPTURE_DIR, prefix, event_count, get_timestamp_ns());
    
    FILE* f = fopen(filename, "wb");
    if (f) {
        fwrite(data, 1, size, f);
        fclose(f);
        
        if (log_file) {
            fprintf(log_file, "  [DATA] Saved %zu bytes to %s\n", size, filename);
        }
    }
}

// Tracker buffer GEM
static void track_gem_buffer(uint32_t handle, uint64_t size, const char* name) {
    if (buffer_count >= MAX_BUFFERS) return;
    
    buffers[buffer_count].handle = handle;
    buffers[buffer_count].size = size;
    buffers[buffer_count].active = 1;
    strncpy(buffers[buffer_count].name, name, sizeof(buffers[buffer_count].name)-1);
    buffer_count++;
    
    if (log_file) {
        fprintf(log_file, "  [TRACK] Buffer handle=%u size=%lu name=%s\n", 
                handle, size, name);
    }
}

// Trouver buffer par handle
static gem_buffer_t* find_buffer(uint32_t handle) {
    for (int i = 0; i < buffer_count; i++) {
        if (buffers[i].handle == handle && buffers[i].active) {
            return &buffers[i];
        }
    }
    return NULL;
}

// Sauvegarder contenu buffer
static void save_buffer_content(uint32_t handle, const char* stage) {
    gem_buffer_t* buf = find_buffer(handle);
    if (!buf || !buf->cpu_ptr) return;
    
    char prefix[128];
    snprintf(prefix, sizeof(prefix), "buffer_%u_%s", handle, stage);
    save_binary_data(prefix, buf->cpu_ptr, buf->size);
}

// Init
__attribute__((constructor))
static void init_forensic(void) {
    real_ioctl = dlsym(RTLD_NEXT, "ioctl");
    real_mmap = dlsym(RTLD_NEXT, "mmap");
    
    if (!real_ioctl || !real_mmap) {
        fprintf(stderr, "[FORENSIC] Erreur dlsym\n");
        exit(1);
    }
    
    // Créer répertoire capture
    system("mkdir -p " CAPTURE_DIR);
    
    // Ouvrir log
    char logfile[512];
    snprintf(logfile, sizeof(logfile), "%s/forensic.log", CAPTURE_DIR);
    log_file = fopen(logfile, "w");
    
    if (log_file) {
        fprintf(log_file, "=== C197.43 ULTRA FORENSIC CAPTURE ===\n");
        fprintf(log_file, "Timestamp: %lu ns\n", get_timestamp_ns());
        fprintf(log_file, "PID: %d\n", getpid());
        fprintf(log_file, "========================================\n\n");
        fflush(log_file);
    }
    
    printf("[FORENSIC] Capture ultra-forensique activée\n");
    printf("[FORENSIC] Logs: %s\n", CAPTURE_DIR);
}

// Cleanup
__attribute__((destructor))
static void cleanup_forensic(void) {
    if (log_file) {
        fprintf(log_file, "\n=== FORENSIC SUMMARY ===\n");
        fprintf(log_file, "Total events: %d\n", event_count);
        fprintf(log_file, "Total buffers: %d\n", buffer_count);
        fprintf(log_file, "========================\n");
        fclose(log_file);
    }
    
    printf("[FORENSIC] Capture terminée: %d events, %d buffers\n", 
           event_count, buffer_count);
}

// Intercepter ioctl
int ioctl(int fd, unsigned long request, ...) {
    va_list args;
    va_start(args, request);
    void* argp = va_arg(args, void*);
    va_end(args);
    
    uint64_t ts_start = get_timestamp_ns();
    
    // Détecter DRM fd
    if (drm_fd == -1 && (request & 0xFF) == 'd') {
        drm_fd = fd;
    }
    
    // Log avant
    if (log_file && fd == drm_fd) {
        fprintf(log_file, "\n[%lu ns] IOCTL %s (0x%lx)\n", 
                ts_start, ioctl_name(request), request);
        fflush(log_file);
    }
    
    // Appeler ioctl réel
    int ret = real_ioctl(fd, request, argp);
    int err = errno;
    
    uint64_t ts_end = get_timestamp_ns();
    
    // Log après
    if (log_file && fd == drm_fd) {
        fprintf(log_file, "  Result: %d (errno=%d, duration=%lu ns)\n", 
                ret, err, ts_end - ts_start);
        fflush(log_file);
    }
    
    // Traiter selon type ioctl
    if (fd == drm_fd && ret == 0) {
        switch(request) {
            case DRM_IOCTL_I915_GEM_CREATE: {
                struct drm_i915_gem_create* create = argp;
                track_gem_buffer(create->handle, create->size, "GEM_CREATE");
                
                if (log_file) {
                    fprintf(log_file, "  handle=%u size=%llu\n",
                            (unsigned long long)create->handle, (unsigned long long)create->size);
                }
                break;
            }
            
            case DRM_IOCTL_I915_GEM_MMAP: {
                struct drm_i915_gem_mmap* mmap_arg = argp;
                gem_buffer_t* buf = find_buffer(mmap_arg->handle);
                if (buf) {
                    buf->cpu_ptr = (void*)(uintptr_t)mmap_arg->addr_ptr;
                    
                    if (log_file) {
                        fprintf(log_file, "  handle=%u ptr=%p\n", 
                                mmap_arg->handle, buf->cpu_ptr);
                    }
                }
                break;
            }
            
            case DRM_IOCTL_I915_GEM_EXECBUFFER2: {
                struct drm_i915_gem_execbuffer2* execbuf = argp;
                
                if (log_file) {
                    fprintf(log_file, "  buffer_count=%u batch_len=%u flags=0x%llx\n",
                            execbuf->buffer_count, execbuf->batch_len,
                            (unsigned long long)execbuf->flags);
                }
                
                // Sauvegarder exec_objects
                struct drm_i915_gem_exec_object2* objects = 
                    (struct drm_i915_gem_exec_object2*)execbuf->buffers_ptr;
                
                save_binary_data("exec_objects", objects, 
                                execbuf->buffer_count * sizeof(*objects));
                
                // Sauvegarder contenu de TOUS les buffers
                for (uint32_t i = 0; i < execbuf->buffer_count; i++) {
                    save_buffer_content(objects[i].handle, "pre_exec");
                    
                    if (log_file) {
                        fprintf(log_file, "  [%u] handle=%u offset=0x%llx flags=0x%llx\n",
                                i, objects[i].handle,
                                (unsigned long long)objects[i].offset,
                                (unsigned long long)objects[i].flags);
                    }
                }
                
                // Sauvegarder batch buffer (dernier objet)
                if (execbuf->buffer_count > 0) {
                    uint32_t batch_handle = objects[execbuf->buffer_count - 1].handle;
                    gem_buffer_t* batch_buf = find_buffer(batch_handle);
                    if (batch_buf && batch_buf->cpu_ptr) {
                        save_binary_data("batch_buffer", batch_buf->cpu_ptr, 
                                        execbuf->batch_len);
                    }
                }
                
                break;
            }
            
            case DRM_IOCTL_GEM_CLOSE: {
                struct drm_gem_close* close_arg = argp;
                gem_buffer_t* buf = find_buffer(close_arg->handle);
                if (buf) {
                    // Sauvegarder contenu final avant fermeture
                    save_buffer_content(close_arg->handle, "final");
                    buf->active = 0;
                }
                break;
            }
        }
    }
    
    // Enregistrer événement
    if (event_count < MAX_EVENTS && fd == drm_fd) {
        events[event_count].timestamp_ns = ts_start;
        events[event_count].ioctl_nr = request;
        strncpy(events[event_count].ioctl_name, ioctl_name(request), 
                sizeof(events[event_count].ioctl_name)-1);
        events[event_count].result = ret;
        events[event_count].errno_val = err;
        event_count++;
    }
    
    errno = err;
    return ret;
}

// Made with Bob
