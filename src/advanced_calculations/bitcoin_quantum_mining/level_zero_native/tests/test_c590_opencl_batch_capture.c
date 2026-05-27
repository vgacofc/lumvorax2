/*
 * TEST C590 - CAPTURE BATCH OPENCL COMPLET
 * ========================================
 *
 * OBJECTIF: Capturer le batch buffer OpenCL complet (pas seulement exec_objects)
 *           pour analyser MEDIA_VFE_STATE bit-à-bit vs i915 natif
 *
 * BASE: C585 i915_spy.c (LD_PRELOAD ioctl interceptor)
 * AJOUT: Capture complète batch buffer + decode MEDIA_VFE_STATE
 *
 * ROOT CAUSE #137: Kernel pas exécuté par GPU
 * HYPOTHÈSE #138: MEDIA_VFE_STATE configuration invalide (40% probabilité)
 *
 * PLAN:
 * 1. Intercepter DRM_IOCTL_I915_GEM_EXECBUFFER2
 * 2. Mapper tous les buffers (pas seulement exec_objects)
 * 3. Extraire batch buffer complet
 * 4. Decoder MEDIA_VFE_STATE (9 DWords)
 * 5. Comparer avec MEDIA_VFE_STATE i915 natif (C584/C589)
 *
 * COMPILATION:
 * gcc -shared -fPIC -o test_c590_opencl_batch_capture.so test_c590_opencl_batch_capture.c -ldl -ldrm
 *
 * UTILISATION:
 * LD_PRELOAD=./test_c590_opencl_batch_capture.so ./test_c585_opencl_capture
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <errno.h>
#include <stdint.h>
#include <stdarg.h>
#include <time.h>

// DRM headers
#include <drm/drm.h>
#include <drm/i915_drm.h>

// Timing utilities
static uint64_t get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

// Global state
static int (*real_ioctl)(int fd, unsigned long request, ...) = NULL;
static int drm_fd = -1;
static int capture_count = 0;

// Initialize real ioctl
static void init_real_ioctl(void) {
    if (!real_ioctl) {
        real_ioctl = dlsym(RTLD_NEXT, "ioctl");
        if (!real_ioctl) {
            fprintf(stderr, "❌ Failed to find real ioctl: %s\n", dlerror());
            exit(1);
        }
    }
}

// Check if fd is DRM device
static int is_drm_fd(int fd) {
    struct drm_version version = {0};
    if (real_ioctl(fd, DRM_IOCTL_VERSION, &version) == 0) {
        if (drm_fd == -1) {
            drm_fd = fd;
            printf("🔍 DRM device detected: fd=%d\n", fd);
        }
        return 1;
    }
    return 0;
}

// Map GEM buffer for reading
static void* map_gem_buffer(int fd, uint32_t handle, uint64_t size) {
    struct drm_i915_gem_mmap_gtt mmap_arg = {
        .handle = handle,
    };
    
    if (real_ioctl(fd, DRM_IOCTL_I915_GEM_MMAP_GTT, &mmap_arg) != 0) {
        printf("    ❌ Failed to get GTT offset for handle %u: %s\n", handle, strerror(errno));
        return NULL;
    }
    
    void* ptr = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, mmap_arg.offset);
    if (ptr == MAP_FAILED) {
        printf("    ❌ Failed to mmap buffer handle %u: %s\n", handle, strerror(errno));
        return NULL;
    }
    
    return ptr;
}

// Decode MEDIA_VFE_STATE (9 DWords starting at specified offset)
static void decode_media_vfe_state(const uint32_t* batch, int offset, const char* source) {
    printf("\n  🔍 MEDIA_VFE_STATE Analysis (%s):\n", source);
    printf("    Offset: DWord %d (0x%x bytes)\n", offset, offset * 4);
    
    // Verify opcode
    uint32_t dw0 = batch[offset];
    if ((dw0 & 0xFFFF0000) != 0x70000000) {
        printf("    ❌ Invalid MEDIA_VFE_STATE opcode: 0x%08x (expected 0x7000xxxx)\n", dw0);
        return;
    }
    
    uint32_t length = (dw0 & 0xFF) + 1;
    printf("    ✅ Valid MEDIA_VFE_STATE, Length: %u DWords\n", length);
    
    if (length != 8) {
        printf("    ⚠️  Unexpected length: %u (expected 8)\n", length);
    }
    
    // Decode each DWord
    printf("    DWord 0: 0x%08x (Opcode + Length)\n", batch[offset + 0]);
    printf("    DWord 1: 0x%08x (Scratch Space Base Pointer Low)\n", batch[offset + 1]);
    printf("    DWord 2: 0x%08x (Scratch Space Base Pointer High)\n", batch[offset + 2]);
    
    uint32_t dw3 = batch[offset + 3];
    uint32_t max_threads = dw3 & 0xFFFF;
    uint32_t num_urb_entries = (dw3 >> 16) & 0xFF;
    uint32_t reset_gateway = (dw3 >> 24) & 0x1;
    uint32_t bypass_gateway = (dw3 >> 25) & 0x1;
    
    printf("    DWord 3: 0x%08x\n", dw3);
    printf("      Max Threads: %u\n", max_threads);
    printf("      Num URB Entries: %u\n", num_urb_entries);
    printf("      Reset Gateway Timer: %u\n", reset_gateway);
    printf("      Bypass Gateway Control: %u\n", bypass_gateway);
    
    uint32_t dw4 = batch[offset + 4];
    printf("    DWord 4: 0x%08x (Reserved)\n", dw4);
    
    uint32_t dw5 = batch[offset + 5];
    uint32_t urb_entry_size = dw5 & 0xFFFF;
    printf("    DWord 5: 0x%08x\n", dw5);
    printf("      URB Entry Allocation Size: %u (x64 bytes = %u bytes)\n", 
           urb_entry_size, urb_entry_size * 64);
    
    uint32_t dw6 = batch[offset + 6];
    uint32_t curbe_size = dw6 & 0xFFFF;
    printf("    DWord 6: 0x%08x\n", dw6);
    printf("      CURBE Allocation Size: %u (x32 bytes = %u bytes)\n", 
           curbe_size, curbe_size * 32);
    
    uint32_t dw7 = batch[offset + 7];
    printf("    DWord 7: 0x%08x (Scoreboard Mask)\n", dw7);
    
    uint32_t dw8 = batch[offset + 8];
    uint32_t scoreboard_type = dw8 & 0x1;
    printf("    DWord 8: 0x%08x\n", dw8);
    printf("      Scoreboard Type: %u (%s)\n", 
           scoreboard_type, scoreboard_type ? "Stalling" : "Non-stalling");
}

// Find MEDIA_VFE_STATE in batch buffer
static int find_media_vfe_state(const uint32_t* batch, int batch_size_dwords) {
    for (int i = 0; i < batch_size_dwords - 8; i++) {
        uint32_t dw = batch[i];
        // MEDIA_VFE_STATE opcode: 0x70000007 (length=7, so 8 DWords total)
        if ((dw & 0xFFFF0000) == 0x70000000) {
            uint32_t length = (dw & 0xFF) + 1;
            if (length == 8) {
                return i;
            }
        }
    }
    return -1;
}

// Dump batch buffer with command decode
static void dump_batch_buffer(const uint32_t* batch, int batch_size_dwords, const char* filename) {
    FILE* f = fopen(filename, "w");
    if (!f) {
        printf("    ❌ Failed to create %s: %s\n", filename, strerror(errno));
        return;
    }
    
    fprintf(f, "# OpenCL Batch Buffer Dump - C590\n");
    fprintf(f, "# Size: %d DWords (%d bytes)\n", batch_size_dwords, batch_size_dwords * 4);
    fprintf(f, "# Timestamp: %lu\n\n", time(NULL));
    
    for (int i = 0; i < batch_size_dwords; i++) {
        uint32_t dw = batch[i];
        fprintf(f, "[%3d] 0x%08x", i, dw);
        
        // Basic command decode
        if ((dw & 0xE0000000) == 0x60000000) {
            fprintf(f, "  # STATE command");
        } else if ((dw & 0xE0000000) == 0x70000000) {
            fprintf(f, "  # MEDIA command");
            if ((dw & 0xFFFF0000) == 0x70000000) {
                uint32_t length = (dw & 0xFF) + 1;
                if (length == 8) {
                    fprintf(f, " - MEDIA_VFE_STATE");
                }
            } else if ((dw & 0xFFFF0000) == 0x70020000) {
                fprintf(f, " - MEDIA_INTERFACE_DESCRIPTOR_LOAD");
            }
        } else if ((dw & 0xE0000000) == 0x20000000) {
            fprintf(f, "  # BLT command");
        } else if ((dw & 0xE0000000) == 0x00000000) {
            fprintf(f, "  # MI command");
            if (dw == 0x0A000000) {
                fprintf(f, " - MI_BATCH_BUFFER_END");
            }
        } else if ((dw & 0xE0000000) == 0x78000000) {
            fprintf(f, "  # 3D command");
            if ((dw & 0xFFFF0000) == 0x78050000) {
                fprintf(f, " - GPGPU_WALKER");
            }
        } else if ((dw & 0xE0000000) == 0x7A000000) {
            fprintf(f, "  # PIPE_CONTROL");
        }
        
        fprintf(f, "\n");
    }
    
    fclose(f);
    printf("    ✅ Batch buffer dumped to %s\n", filename);
}

// Process EXECBUFFER2 ioctl
static void process_execbuffer2(int fd, struct drm_i915_gem_execbuffer2* eb) {
    capture_count++;
    uint64_t start_time = get_time_ns();
    
    printf("\n🚀 EXECBUFFER2 #%d CAPTURE (C590)\n", capture_count);
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("Buffer count: %u\n", eb->buffer_count);
    printf("Batch start offset: 0x%x\n", eb->batch_start_offset);
    printf("Batch length: %u bytes (%u DWords)\n", eb->batch_len, eb->batch_len / 4);
    printf("Flags: 0x%llx\n", eb->flags);
    printf("Context: %llu\n", eb->rsvd1);
    
    // Get exec_objects array
    struct drm_i915_gem_exec_object2* exec_objects =
        (struct drm_i915_gem_exec_object2*)eb->buffers_ptr;
    
    // Find batch buffer (last one, largest handle)
    uint32_t batch_handle = 0;
    uint64_t batch_size = eb->batch_len;
    
    for (uint32_t i = 0; i < eb->buffer_count; i++) {
        printf("Buffer[%u]: handle=%u, offset=0x%llx, flags=0x%llx\n",
               i, exec_objects[i].handle,
               exec_objects[i].offset, exec_objects[i].flags);
        
        // Batch buffer is typically the last one
        if (i == eb->buffer_count - 1) {
            batch_handle = exec_objects[i].handle;
        }
    }
    
    if (batch_handle == 0) {
        printf("❌ No batch buffer found\n");
        return;
    }
    
    printf("\n📦 Mapping batch buffer (handle=%u, size=%lu)...\n", batch_handle, batch_size);
    
    // Map batch buffer
    void* batch_ptr = map_gem_buffer(fd, batch_handle, batch_size);
    if (!batch_ptr) {
        return;
    }
    
    uint32_t* batch = (uint32_t*)batch_ptr;
    int batch_size_dwords = eb->batch_len / 4;
    
    printf("✅ Batch buffer mapped: %p\n", batch_ptr);
    printf("Analyzing %d DWords...\n", batch_size_dwords);
    
    // Find MEDIA_VFE_STATE
    int vfe_offset = find_media_vfe_state(batch, batch_size_dwords);
    if (vfe_offset >= 0) {
        printf("\n🎯 MEDIA_VFE_STATE found at DWord %d!\n", vfe_offset);
        decode_media_vfe_state(batch, vfe_offset, "OpenCL");
        
        // Compare with i915 native (C584/C589)
        printf("\n📊 COMPARISON WITH i915 NATIVE:\n");
        printf("  i915 C584/C589 MEDIA_VFE_STATE (DWord 25-32):\n");
        printf("    DWord 0: 0x70000007 (Opcode + Length=7)\n");
        printf("    DWord 1: 0x00000000 (Scratch Space Base Low)\n");
        printf("    DWord 2: 0x00000000 (Scratch Space Base High)\n");
        printf("    DWord 3: 0x00820000 (Max Threads=130, URB Entries=2)\n");
        printf("    DWord 4: 0x00000000 (Reserved)\n");
        printf("    DWord 5: 0x00000000 (URB Entry Size=0)\n");
        printf("    DWord 6: 0x00000000 (CURBE Size=0)\n");
        printf("    DWord 7: 0x00000000 (Scoreboard Mask)\n");
        printf("    DWord 8: 0x00000000 (Scoreboard Type=Non-stalling)\n");
        
        // Highlight differences
        uint32_t opencl_dw3 = batch[vfe_offset + 3];
        if (opencl_dw3 != 0x00820000) {
            printf("\n🔴 CRITICAL DIFFERENCE FOUND!\n");
            printf("  OpenCL DWord 3: 0x%08x\n", opencl_dw3);
            printf("  i915   DWord 3: 0x00820000\n");
            printf("  This could be ROOT CAUSE #138!\n");
        }
        
        uint32_t opencl_dw5 = batch[vfe_offset + 5];
        if (opencl_dw5 != 0x00000000) {
            printf("\n🔴 URB ENTRY SIZE DIFFERENCE!\n");
            printf("  OpenCL URB Size: 0x%08x\n", opencl_dw5);
            printf("  i915   URB Size: 0x00000000\n");
        }
        
        uint32_t opencl_dw6 = batch[vfe_offset + 6];
        if (opencl_dw6 != 0x00000000) {
            printf("\n🔴 CURBE SIZE DIFFERENCE!\n");
            printf("  OpenCL CURBE Size: 0x%08x\n", opencl_dw6);
            printf("  i915   CURBE Size: 0x00000000\n");
        }
        
    } else {
        printf("❌ MEDIA_VFE_STATE not found in batch buffer\n");
    }
    
    // Dump complete batch buffer
    char filename[256];
    snprintf(filename, sizeof(filename), 
             "../logs/opencl_batch_c590_capture_%d.txt", capture_count);
    dump_batch_buffer(batch, batch_size_dwords, filename);
    
    // Unmap buffer
    munmap(batch_ptr, batch_size);
    
    uint64_t end_time = get_time_ns();
    printf("\n✅ Capture completed in %.3f ms\n", (end_time - start_time) / 1000000.0);
    printf("═══════════════════════════════════════════════════════════════\n");
}

// Intercepted ioctl function
int ioctl(int fd, unsigned long request, ...) {
    va_list args;
    va_start(args, request);
    void* arg = va_arg(args, void*);
    va_end(args);
    
    init_real_ioctl();
    
    // Check for DRM EXECBUFFER2
    if (request == DRM_IOCTL_I915_GEM_EXECBUFFER2 && is_drm_fd(fd)) {
        struct drm_i915_gem_execbuffer2* eb = (struct drm_i915_gem_execbuffer2*)arg;
        
        // Process before calling real ioctl
        process_execbuffer2(fd, eb);
    }
    
    // Call real ioctl
    return real_ioctl(fd, request, arg);
}

// Constructor - called when library is loaded
__attribute__((constructor))
static void init_spy(void) {
    printf("🔍 C590 OpenCL Batch Capture initialized\n");
    printf("Intercepting DRM_IOCTL_I915_GEM_EXECBUFFER2...\n");
}

// Destructor - called when library is unloaded
__attribute__((destructor))
static void cleanup_spy(void) {
    printf("🔍 C590 OpenCL Batch Capture finished\n");
    printf("Total EXECBUFFER2 calls captured: %d\n", capture_count);
}

// Made with Bob
