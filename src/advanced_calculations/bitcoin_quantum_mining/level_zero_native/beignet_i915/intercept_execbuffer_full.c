#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <i915_drm.h>

static int exec_count = 0;
static FILE *log_file = NULL;

static void init_log(void) {
    if (!log_file) {
        char filename[256];
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        snprintf(filename, sizeof(filename), 
                 "logs/forensic/opencl_execbuffer_sequence_%04d%02d%02d_%02d%02d%02d.log",
                 t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
                 t->tm_hour, t->tm_min, t->tm_sec);
        log_file = fopen(filename, "w");
        if (log_file) {
            fprintf(log_file, "╔════════════════════════════════════════════════════════════════╗\n");
            fprintf(log_file, "║  CAPTURE SÉQUENCE COMPLÈTE EXECBUFFER2 OPENCL                ║\n");
            fprintf(log_file, "╚════════════════════════════════════════════════════════════════╝\n\n");
        }
    }
}

int ioctl(int fd, unsigned long request, ...) {
    static int (*real_ioctl)(int, unsigned long, ...) = NULL;
    if (!real_ioctl) {
        real_ioctl = dlsym(RTLD_NEXT, "ioctl");
    }
    
    va_list args;
    va_start(args, request);
    void *argp = va_arg(args, void*);
    va_end(args);
    
    // Intercepter EXECBUFFER2
    if (request == DRM_IOCTL_I915_GEM_EXECBUFFER2) {
        init_log();
        exec_count++;
        
        struct drm_i915_gem_execbuffer2 *exec = (struct drm_i915_gem_execbuffer2*)argp;
        
        fprintf(log_file, "\n═══════════════════════════════════════════════════════════════\n");
        fprintf(log_file, "EXECBUFFER2 #%d\n", exec_count);
        fprintf(log_file, "═══════════════════════════════════════════════════════════════\n");
        fprintf(log_file, "  buffer_count:    %u\n", exec->buffer_count);
        fprintf(log_file, "  batch_start:     0x%x\n", exec->batch_start_offset);
        fprintf(log_file, "  batch_len:       %u bytes\n", exec->batch_len);
        fprintf(log_file, "  flags:           0x%llx\n", (unsigned long long)exec->flags);
        fprintf(log_file, "  rsvd1 (ctx_id):  %llu\n", (unsigned long long)exec->rsvd1);
        fprintf(log_file, "  rsvd2:           %llu\n", (unsigned long long)exec->rsvd2);
        
        // Analyser les buffer objects
        struct drm_i915_gem_exec_object2 *objects = 
            (struct drm_i915_gem_exec_object2*)exec->buffers_ptr;
        
        fprintf(log_file, "\n  Buffer Objects (%u):\n", exec->buffer_count);
        for (uint32_t i = 0; i < exec->buffer_count; i++) {
            fprintf(log_file, "    [%u] handle=%u, reloc_count=%u, offset=0x%llx, flags=0x%llx\n",
                    i, objects[i].handle, (uint32_t)objects[i].relocation_count,
                    (unsigned long long)objects[i].offset, (unsigned long long)objects[i].flags);
            
            // Dump relocations si présentes
            if (objects[i].relocation_count > 0) {
                struct drm_i915_gem_relocation_entry *relocs = 
                    (struct drm_i915_gem_relocation_entry*)objects[i].relocs_ptr;
                fprintf(log_file, "      Relocations:\n");
                for (uint64_t j = 0; j < objects[i].relocation_count; j++) {
                    fprintf(log_file, "        [%lu] offset=0x%llx, target_handle=%u, delta=0x%x, read_domains=0x%x, write_domain=0x%x\n",
                            (unsigned long)j, (unsigned long long)relocs[j].offset, 
                            relocs[j].target_handle, relocs[j].delta, 
                            relocs[j].read_domains, relocs[j].write_domain);
                }
            }
        }
        
        fflush(log_file);
    }
    
    int ret = real_ioctl(fd, request, argp);
    
    if (request == DRM_IOCTL_I915_GEM_EXECBUFFER2 && log_file) {
        fprintf(log_file, "  → Résultat: %d\n", ret);
        fflush(log_file);
    }
    
    return ret;
}

__attribute__((destructor))
static void cleanup(void) {
    if (log_file) {
        fprintf(log_file, "\n═══════════════════════════════════════════════════════════════\n");
        fprintf(log_file, "TOTAL EXECBUFFER2: %d\n", exec_count);
        fprintf(log_file, "═══════════════════════════════════════════════════════════════\n");
        fclose(log_file);
    }
}
