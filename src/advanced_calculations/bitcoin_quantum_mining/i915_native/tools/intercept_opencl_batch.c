/**
 * LumVorax C198 Phase 15Y-F20 — INTERCEPTEUR BATCH BUFFERS OPENCL
 * 
 * OBJECTIF: Capturer les VRAIS batch buffers générés par OpenCL
 * MÉTHODE: LD_PRELOAD pour intercepter ioctl(DRM_IOCTL_I915_GEM_EXECBUFFER2)
 * 
 * STRATÉGIE:
 * 1. Intercepter tous les appels ioctl vers i915
 * 2. Détecter DRM_IOCTL_I915_GEM_EXECBUFFER2
 * 3. Dumper le contenu du batch buffer
 * 4. Analyser la structure exacte utilisée par OpenCL
 * 
 * Build:
 *   gcc -shared -fPIC -o intercept_opencl_batch.so intercept_opencl_batch.c -ldl
 * 
 * Usage:
 *   LD_PRELOAD=./intercept_opencl_batch.so <programme_opencl>
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <dlfcn.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <i915_drm.h>

#define COLOR_CYAN    "\033[36m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_RESET   "\033[0m"

static int (*real_ioctl)(int fd, unsigned long request, ...) = NULL;
static int intercept_count = 0;
static FILE *dump_file = NULL;

__attribute__((constructor))
static void init_intercept(void) {
    real_ioctl = dlsym(RTLD_NEXT, "ioctl");
    if (!real_ioctl) {
        fprintf(stderr, "ERREUR: dlsym(ioctl) failed\n");
        exit(1);
    }
    
    dump_file = fopen("opencl_batch_dump.txt", "w");
    if (!dump_file) {
        fprintf(stderr, "ERREUR: Cannot create dump file\n");
        exit(1);
    }
    
    fprintf(stderr, COLOR_CYAN "\n");
    fprintf(stderr, "╔════════════════════════════════════════════════════════════╗\n");
    fprintf(stderr, "║  LumVorax C198 — OpenCL Batch Buffer Interceptor         ║\n");
    fprintf(stderr, "║  Capturing all i915 EXECBUFFER2 calls                     ║\n");
    fprintf(stderr, "╚════════════════════════════════════════════════════════════╝\n");
    fprintf(stderr, COLOR_RESET "\n");
    
    fprintf(dump_file, "LumVorax C198 — OpenCL Batch Buffer Dump\n");
    fprintf(dump_file, "==========================================\n\n");
    fflush(dump_file);
}

__attribute__((destructor))
static void cleanup_intercept(void) {
    if (dump_file) {
        fprintf(dump_file, "\n\nTotal EXECBUFFER2 calls intercepted: %d\n", intercept_count);
        fclose(dump_file);
    }
    
    fprintf(stderr, COLOR_GREEN "\n");
    fprintf(stderr, "╔════════════════════════════════════════════════════════════╗\n");
    fprintf(stderr, "║  Interception complete: %d EXECBUFFER2 calls              ║\n", intercept_count);
    fprintf(stderr, "║  Dump saved to: opencl_batch_dump.txt                     ║\n");
    fprintf(stderr, "╚════════════════════════════════════════════════════════════╝\n");
    fprintf(stderr, COLOR_RESET "\n");
}

static void dump_batch_buffer(int fd, struct drm_i915_gem_execbuffer2 *execbuf) {
    intercept_count++;
    
    fprintf(dump_file, "\n");
    fprintf(dump_file, "═══════════════════════════════════════════════════════════\n");
    fprintf(dump_file, "EXECBUFFER2 Call #%d\n", intercept_count);
    fprintf(dump_file, "═══════════════════════════════════════════════════════════\n");
    fprintf(dump_file, "buffer_count:        %u\n", execbuf->buffer_count);
    fprintf(dump_file, "batch_start_offset:  0x%x\n", execbuf->batch_start_offset);
    fprintf(dump_file, "batch_len:           %u bytes\n", execbuf->batch_len);
    fprintf(dump_file, "flags:               0x%llx\n", execbuf->flags);
    fprintf(dump_file, "rsvd1 (ctx_id):      %llu\n", execbuf->rsvd1);
    fprintf(dump_file, "\n");
    
    /* Analyser exec_objects */
    struct drm_i915_gem_exec_object2 *objects = 
        (struct drm_i915_gem_exec_object2 *)(uintptr_t)execbuf->buffers_ptr;
    
    for (uint32_t i = 0; i < execbuf->buffer_count; i++) {
        fprintf(dump_file, "Object[%u]:\n", i);
        fprintf(dump_file, "  handle:            %u\n", objects[i].handle);
        fprintf(dump_file, "  relocation_count:  %u\n", objects[i].relocation_count);
        fprintf(dump_file, "  offset:            0x%llx\n", objects[i].offset);
        fprintf(dump_file, "  flags:             0x%llx\n", objects[i].flags);
        fprintf(dump_file, "\n");
        
        /* Si c'est le batch buffer (dernier objet généralement) */
        if (i == execbuf->buffer_count - 1) {
            /* Essayer de mapper et dumper le contenu */
            struct drm_i915_gem_mmap mmap_arg = {
                .handle = objects[i].handle,
                .offset = 0,
                .size = 4096,  /* Taille arbitraire */
                .flags = 0
            };
            
            if (real_ioctl(fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_arg) == 0) {
                uint32_t *batch = (uint32_t *)(uintptr_t)mmap_arg.addr_ptr;
                
                fprintf(dump_file, "Batch Buffer Content (first 64 DWORDs):\n");
                fprintf(dump_file, "───────────────────────────────────────────────────────\n");
                
                for (int j = 0; j < 64 && j * 4 < execbuf->batch_len; j++) {
                    fprintf(dump_file, "  [%3d] 0x%08x", j, batch[j]);
                    
                    /* Décoder quelques commandes connues */
                    if ((batch[j] & 0xFF000000) == 0x7A000000) {
                        fprintf(dump_file, "  ; PIPE_CONTROL");
                    } else if ((batch[j] & 0xFF000000) == 0x0A000000) {
                        fprintf(dump_file, "  ; MI_BATCH_BUFFER_END");
                    } else if ((batch[j] & 0xFF000000) == 0x00000000) {
                        fprintf(dump_file, "  ; MI_NOOP");
                    } else if ((batch[j] & 0xFF000000) == 0x01000000) {
                        fprintf(dump_file, "  ; STATE_BASE_ADDRESS");
                    } else if ((batch[j] & 0xFF000000) == 0x70000000) {
                        fprintf(dump_file, "  ; MEDIA_VFE_STATE");
                    } else if ((batch[j] & 0xFF000000) == 0x71000000) {
                        fprintf(dump_file, "  ; MEDIA_INTERFACE_DESCRIPTOR_LOAD");
                    } else if ((batch[j] & 0xFF000000) == 0x72000000) {
                        fprintf(dump_file, "  ; GPGPU_WALKER");
                    }
                    
                    fprintf(dump_file, "\n");
                }
                
                fprintf(dump_file, "───────────────────────────────────────────────────────\n");
                
                munmap(batch, 4096);
            } else {
                fprintf(dump_file, "  (Could not map batch buffer for reading)\n");
            }
        }
        
        /* Dumper relocations si présentes */
        if (objects[i].relocation_count > 0) {
            struct drm_i915_gem_relocation_entry *relocs =
                (struct drm_i915_gem_relocation_entry *)(uintptr_t)objects[i].relocs_ptr;
            
            fprintf(dump_file, "  Relocations:\n");
            for (uint32_t r = 0; r < objects[i].relocation_count && r < 10; r++) {
                fprintf(dump_file, "    [%u] offset=0x%x target_handle=%u delta=0x%x\n",
                        r, (uint32_t)relocs[r].offset, relocs[r].target_handle, (uint32_t)relocs[r].delta);
            }
            if (objects[i].relocation_count > 10) {
                fprintf(dump_file, "    ... (%u more relocations)\n",
                        objects[i].relocation_count - 10);
            }
        }
    }
    
    fprintf(dump_file, "\n");
    fflush(dump_file);
    
    /* Log console */
    fprintf(stderr, COLOR_YELLOW "[INTERCEPT #%d] EXECBUFFER2: batch_len=%u bytes, %u objects\n" COLOR_RESET,
            intercept_count, execbuf->batch_len, execbuf->buffer_count);
}

int ioctl(int fd, unsigned long request, ...) {
    va_list args;
    va_start(args, request);
    void *argp = va_arg(args, void *);
    va_end(args);
    
    /* Intercepter DRM_IOCTL_I915_GEM_EXECBUFFER2 */
    if (request == DRM_IOCTL_I915_GEM_EXECBUFFER2) {
        struct drm_i915_gem_execbuffer2 *execbuf = (struct drm_i915_gem_execbuffer2 *)argp;
        dump_batch_buffer(fd, execbuf);
    }
    
    /* Appeler le vrai ioctl */
    return real_ioctl(fd, request, argp);
}

// Made with Bob
