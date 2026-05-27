/*
 * LUMVORAX - Test C506: Interception Batch Buffer OpenCL
 * 
 * Objectif: Intercepter ioctl(DRM_IOCTL_I915_GEM_EXECBUFFER2) pour capturer
 *           le batch buffer exact généré par OpenCL/Intel NEO
 * 
 * Utilisation: LD_PRELOAD=./libintercept_c506.so ./test_c427_store_opencl
 * 
 * Date: 2026-05-24
 * Cycle: C506
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <dlfcn.h>
#include <time.h>
#include <drm/i915_drm.h>

// Compteur appels EXECBUFFER2
static int execbuffer_count = 0;

// Pointeur vers ioctl original
static int (*original_ioctl)(int fd, unsigned long request, ...) = NULL;

// Fichier log forensique
static FILE *log_file = NULL;

// Timestamp nanoseconde
static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

// Dump buffer hexadécimal
static void dump_hex(FILE *f, const char *label, const void *data, size_t size) {
    const uint8_t *bytes = data;
    fprintf(f, "\n=== %s (%zu bytes) ===\n", label, size);
    
    for (size_t i = 0; i < size; i += 16) {
        fprintf(f, "%08zx: ", i);
        
        // Hex
        for (size_t j = 0; j < 16; j++) {
            if (i + j < size) {
                fprintf(f, "%02x ", bytes[i + j]);
            } else {
                fprintf(f, "   ");
            }
        }
        
        fprintf(f, " | ");
        
        // ASCII
        for (size_t j = 0; j < 16 && i + j < size; j++) {
            uint8_t c = bytes[i + j];
            fprintf(f, "%c", (c >= 32 && c < 127) ? c : '.');
        }
        
        fprintf(f, "\n");
    }
}

// Dump batch buffer complet
static void dump_batch_buffer(int fd, struct drm_i915_gem_execbuffer2 *exec) {
    uint64_t timestamp = get_timestamp_ns();
    execbuffer_count++;
    
    if (!log_file) {
        char filename[256];
        snprintf(filename, sizeof(filename), 
                 "logs/forensic/batch_intercept_c506_%d.log", getpid());
        log_file = fopen(filename, "w");
        if (!log_file) {
            log_file = stderr;
        }
    }
    
    fprintf(log_file, "\n");
    fprintf(log_file, "================================================================================\n");
    fprintf(log_file, "EXECBUFFER2 CALL #%d - Timestamp: %lu ns\n", execbuffer_count, timestamp);
    fprintf(log_file, "================================================================================\n");
    
    // Informations EXECBUFFER2
    fprintf(log_file, "\n--- EXECBUFFER2 Structure ---\n");
    fprintf(log_file, "buffers_ptr:       0x%016llx\n", exec->buffers_ptr);
    fprintf(log_file, "buffer_count:      %u\n", exec->buffer_count);
    fprintf(log_file, "batch_start_offset: %u\n", exec->batch_start_offset);
    fprintf(log_file, "batch_len:         %u bytes\n", exec->batch_len);
    fprintf(log_file, "flags:             0x%016llx\n", exec->flags);
    fprintf(log_file, "rsvd1:             0x%016llx\n", exec->rsvd1);
    fprintf(log_file, "rsvd2:             0x%016llx\n", exec->rsvd2);
    
    // Buffer objects
    if (exec->buffer_count > 0) {
        struct drm_i915_gem_exec_object2 *objects = 
            (struct drm_i915_gem_exec_object2 *)exec->buffers_ptr;
        
        fprintf(log_file, "\n--- Buffer Objects (%u) ---\n", exec->buffer_count);
        for (uint32_t i = 0; i < exec->buffer_count; i++) {
            fprintf(log_file, "\nBuffer #%u:\n", i);
            fprintf(log_file, "  handle:           %u\n", objects[i].handle);
            fprintf(log_file, "  relocation_count: %u\n", objects[i].relocation_count);
            fprintf(log_file, "  relocs_ptr:       0x%016llx\n", objects[i].relocs_ptr);
            fprintf(log_file, "  alignment:        %llu\n", objects[i].alignment);
            fprintf(log_file, "  offset:           0x%016llx\n", objects[i].offset);
            fprintf(log_file, "  flags:            0x%016llx\n", objects[i].flags);
            
            // Relocations
            if (objects[i].relocation_count > 0) {
                struct drm_i915_gem_relocation_entry *relocs = 
                    (struct drm_i915_gem_relocation_entry *)objects[i].relocs_ptr;
                
                fprintf(log_file, "\n  Relocations (%u):\n", objects[i].relocation_count);
                for (uint32_t j = 0; j < objects[i].relocation_count; j++) {
                    fprintf(log_file, "    Reloc #%u:\n", j);
                    fprintf(log_file, "      offset:        0x%08llx\n", (unsigned long long)relocs[j].offset);
                    fprintf(log_file, "      delta:         0x%08x\n", relocs[j].delta);
                    fprintf(log_file, "      target_handle: %u\n", relocs[j].target_handle);
                    fprintf(log_file, "      read_domains:  0x%08x\n", relocs[j].read_domains);
                    fprintf(log_file, "      write_domain:  0x%08x\n", relocs[j].write_domain);
                }
            }
            
            // Tenter de mapper et dumper le buffer
            if (i == exec->buffer_count - 1) {  // Dernier buffer = batch buffer
                struct drm_i915_gem_mmap mmap_arg = {
                    .handle = objects[i].handle,
                    .offset = 0,
                    .size = exec->batch_len,
                    .flags = 0
                };
                
                if (ioctl(fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_arg) == 0) {
                    void *mapped = (void *)mmap_arg.addr_ptr;
                    
                    fprintf(log_file, "\n--- BATCH BUFFER CONTENT ---\n");
                    fprintf(log_file, "Mapped address: %p\n", mapped);
                    fprintf(log_file, "Batch length:   %u bytes\n", exec->batch_len);
                    
                    // Dump hexadécimal complet
                    dump_hex(log_file, "BATCH BUFFER", mapped, exec->batch_len);
                    
                    // Décodage commandes GPU Gen9
                    fprintf(log_file, "\n--- GPU COMMANDS DECODE ---\n");
                    uint32_t *batch = (uint32_t *)mapped;
                    uint32_t offset = 0;
                    
                    while (offset < exec->batch_len / 4) {
                        uint32_t cmd = batch[offset];
                        uint32_t opcode = (cmd >> 23) & 0x1FF;
                        uint32_t subopcode = (cmd >> 16) & 0x7F;
                        uint32_t length = (cmd & 0xFF) + 2;
                        
                        fprintf(log_file, "[%04x] 0x%08x: ", offset * 4, cmd);
                        
                        // Décodage commandes principales
                        if ((cmd & 0xFFFF0000) == 0x69040000) {
                            fprintf(log_file, "PIPELINE_SELECT (length=%u)\n", length);
                        } else if ((cmd & 0xFFFF0000) == 0x61010000) {
                            fprintf(log_file, "STATE_BASE_ADDRESS (length=%u)\n", length);
                        } else if ((cmd & 0xFFFF0000) == 0x70000000) {
                            fprintf(log_file, "MEDIA_VFE_STATE (length=%u)\n", length);
                        } else if ((cmd & 0xFFFF0000) == 0x70020000) {
                            fprintf(log_file, "MEDIA_INTERFACE_DESCRIPTOR_LOAD (length=%u)\n", length);
                        } else if ((cmd & 0xFFFF0000) == 0x72050000) {
                            fprintf(log_file, "GPGPU_WALKER (length=%u)\n", length);
                        } else if ((cmd & 0xFFFF0000) == 0x7A000000) {
                            fprintf(log_file, "PIPE_CONTROL (length=%u)\n", length);
                        } else if (cmd == 0x05000000) {
                            fprintf(log_file, "MI_BATCH_BUFFER_END\n");
                            break;
                        } else {
                            fprintf(log_file, "UNKNOWN (opcode=0x%03x, subop=0x%02x, length=%u)\n",
                                   opcode, subopcode, length);
                        }
                        
                        // Dump paramètres commande
                        for (uint32_t k = 1; k < length && (offset + k) < exec->batch_len / 4; k++) {
                            fprintf(log_file, "       [%04x] 0x%08x\n", 
                                   (offset + k) * 4, batch[offset + k]);
                        }
                        
                        offset += length;
                    }
                    
                    munmap(mapped, exec->batch_len);
                } else {
                    fprintf(log_file, "\n!!! ERREUR: Impossible de mapper batch buffer (handle=%u)\n",
                           objects[i].handle);
                }
            }
        }
    }
    
    fprintf(log_file, "\n================================================================================\n");
    fflush(log_file);
}

// Wrapper ioctl
int ioctl(int fd, unsigned long request, ...) {
    va_list args;
    va_start(args, request);
    void *argp = va_arg(args, void*);
    va_end(args);
    
    // Charger ioctl original si nécessaire
    if (!original_ioctl) {
        original_ioctl = dlsym(RTLD_NEXT, "ioctl");
        if (!original_ioctl) {
            fprintf(stderr, "ERREUR: Impossible de charger ioctl original\n");
            return -1;
        }
    }
    
    // Intercepter EXECBUFFER2
    if (request == DRM_IOCTL_I915_GEM_EXECBUFFER2) {
        struct drm_i915_gem_execbuffer2 *exec = argp;
        
        // Dump batch buffer AVANT exécution
        dump_batch_buffer(fd, exec);
    }
    
    // Appeler ioctl original
    int result = original_ioctl(fd, request, argp);
    
    // Log résultat pour EXECBUFFER2
    if (request == DRM_IOCTL_I915_GEM_EXECBUFFER2) {
        if (log_file) {
            fprintf(log_file, "\nEXECBUFFER2 Result: %d (errno=%d)\n", result, result < 0 ? errno : 0);
            fflush(log_file);
        }
    }
    
    return result;
}

// Cleanup à la sortie
__attribute__((destructor))
static void cleanup(void) {
    if (log_file && log_file != stderr) {
        fprintf(log_file, "\n=== INTERCEPTION TERMINÉE ===\n");
        fprintf(log_file, "Total EXECBUFFER2 calls: %d\n", execbuffer_count);
        fclose(log_file);
    }
}

// Made with Bob
