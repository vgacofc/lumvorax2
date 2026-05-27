// TEST C537 - DUMP COMPLET DE TOUS LES BUFFERS OPENCL
// Objectif: Capturer IDRT, Surface State, Binding Table, CURBE byte-par-byte
#define CL_TARGET_OPENCL_VERSION 120
#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <drm/i915_drm.h>
#include <xf86drm.h>

// Structure pour capturer les buffers
typedef struct {
    uint32_t handle;
    uint64_t offset;
    uint64_t size;
    void *cpu_addr;
    char name[64];
} BufferInfo;

#define MAX_BUFFERS 100
BufferInfo captured_buffers[MAX_BUFFERS];
int num_buffers = 0;

// Hook pour capturer les gem_create
static int drm_fd = -1;

void dump_buffer_hex(const char *name, const void *data, size_t size) {
    printf("\n=== %s (%zu bytes) ===\n", name, size);
    const uint8_t *bytes = (const uint8_t *)data;
    
    for (size_t i = 0; i < size; i += 16) {
        printf("  %04zx: ", i);
        
        // Hex
        for (size_t j = 0; j < 16 && i + j < size; j++) {
            printf("%02x ", bytes[i + j]);
        }
        
        // Padding
        for (size_t j = size - i; j < 16; j++) {
            printf("   ");
        }
        
        // ASCII
        printf(" |");
        for (size_t j = 0; j < 16 && i + j < size; j++) {
            uint8_t c = bytes[i + j];
            printf("%c", (c >= 32 && c < 127) ? c : '.');
        }
        printf("|\n");
    }
}

void save_buffer_to_file(const char *filename, const void *data, size_t size) {
    FILE *f = fopen(filename, "wb");
    if (f) {
        fwrite(data, 1, size, f);
        fclose(f);
        printf("✓ Sauvegardé: %s (%zu bytes)\n", filename, size);
    }
}

void analyze_idrt(const uint8_t *data, size_t size) {
    printf("\n=== ANALYSE IDRT (Interface Descriptor) ===\n");
    
    if (size < 32) {
        printf("❌ Taille insuffisante pour IDRT\n");
        return;
    }
    
    const uint32_t *dwords = (const uint32_t *)data;
    
    printf("DW0 (Kernel Start Pointer LOW):  0x%08x\n", dwords[0]);
    printf("DW1 (Kernel Start Pointer HIGH): 0x%08x\n", dwords[1]);
    printf("  → Kernel GTT Address: 0x%016lx\n", 
           ((uint64_t)dwords[1] << 32) | dwords[0]);
    
    printf("DW2 (Reserved):                   0x%08x\n", dwords[2]);
    
    printf("DW3 (Sampler/BTI):                0x%08x\n", dwords[3]);
    uint32_t sampler_count = (dwords[3] >> 2) & 0x7;
    uint32_t sampler_offset = (dwords[3] >> 5) & 0x1F;
    uint32_t bti_offset = (dwords[3] >> 16) & 0x1F;
    uint32_t bti_count = (dwords[3] >> 21) & 0x1F;
    printf("  → Sampler Count: %u\n", sampler_count);
    printf("  → Sampler Offset: 0x%x\n", sampler_offset);
    printf("  → BTI Offset: 0x%x\n", bti_offset);
    printf("  → BTI Count: %u\n", bti_count);
    
    printf("DW4 (Constant URB Entry):         0x%08x\n", dwords[4]);
    uint32_t curbe_read_offset = dwords[4] & 0xFFFF;
    uint32_t curbe_read_length = (dwords[4] >> 16) & 0xFFFF;
    printf("  → CURBE Read Offset: 0x%x\n", curbe_read_offset);
    printf("  → CURBE Read Length: %u\n", curbe_read_length);
    
    printf("DW5 (Barrier/SLM):                0x%08x\n", dwords[5]);
    uint32_t barrier_enable = (dwords[5] >> 21) & 0x1;
    uint32_t slm_size = (dwords[5] >> 16) & 0x1F;
    uint32_t num_threads = dwords[5] & 0x3FF;
    printf("  → Barrier Enable: %u\n", barrier_enable);
    printf("  → SLM Size: %u\n", slm_size);
    printf("  → Number of Threads: %u\n", num_threads);
    
    printf("DW6 (Cross-Thread Constant):      0x%08x\n", dwords[6]);
    printf("DW7 (Reserved):                   0x%08x\n", dwords[7]);
}

void analyze_surface_state(const uint8_t *data, size_t size) {
    printf("\n=== ANALYSE SURFACE STATE ===\n");
    
    if (size < 64) {
        printf("❌ Taille insuffisante pour Surface State\n");
        return;
    }
    
    const uint32_t *dwords = (const uint32_t *)data;
    
    printf("DW0 (Surface Type/Format):        0x%08x\n", dwords[0]);
    uint32_t surface_type = dwords[0] & 0x7;
    printf("  → Surface Type: %u (0=1D, 1=2D, 4=BUFFER)\n", surface_type);
    
    printf("DW1 (Base Address LOW):           0x%08x\n", dwords[1]);
    printf("DW2 (Base Address HIGH):          0x%08x\n", dwords[2]);
    printf("  → Buffer GTT Address: 0x%016lx\n", 
           ((uint64_t)dwords[2] << 32) | dwords[1]);
    
    printf("DW3 (Width):                      0x%08x\n", dwords[3]);
    uint32_t width = (dwords[3] & 0x7F) + 1;
    printf("  → Width: %u bytes\n", width);
    
    printf("DW4 (Height/Depth):               0x%08x\n", dwords[4]);
    printf("DW5 (Surface Pitch):              0x%08x\n", dwords[5]);
    uint32_t pitch = (dwords[5] & 0x3FFFF) + 1;
    printf("  → Pitch: %u bytes\n", pitch);
    
    printf("DW6-DW15: ");
    for (int i = 6; i < 16; i++) {
        printf("0x%08x ", dwords[i]);
        if ((i - 5) % 4 == 0) printf("\n          ");
    }
    printf("\n");
}

void analyze_binding_table(const uint8_t *data, size_t size) {
    printf("\n=== ANALYSE BINDING TABLE ===\n");
    
    const uint32_t *entries = (const uint32_t *)data;
    size_t num_entries = size / 4;
    
    printf("Nombre d'entrées: %zu\n", num_entries);
    
    for (size_t i = 0; i < num_entries && i < 16; i++) {
        printf("BTI[%zu]: 0x%08x (offset vers Surface State)\n", i, entries[i]);
    }
}

int main(void) {
    printf("=== TEST C537 - DUMP COMPLET BUFFERS OPENCL ===\n\n");
    
    // Ouvrir DRM pour monitoring
    drm_fd = open("/dev/dri/card0", O_RDWR);
    if (drm_fd < 0) {
        perror("open /dev/dri/card0");
        return 1;
    }
    
    // Lire le kernel source
    FILE *f = fopen("tests/test_c532_opencl_compare.cl", "r");
    if (!f) {
        perror("fopen kernel");
        return 1;
    }
    
    fseek(f, 0, SEEK_END);
    size_t kernel_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char *kernel_source = malloc(kernel_size + 1);
    fread(kernel_source, 1, kernel_size, f);
    kernel_source[kernel_size] = '\0';
    fclose(f);
    
    printf("Kernel source chargé: %zu bytes\n\n", kernel_size);
    
    // Initialiser OpenCL
    cl_platform_id platform;
    clGetPlatformIDs(1, &platform, NULL);
    
    cl_device_id device;
    clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    
    cl_context context = clCreateContext(NULL, 1, &device, NULL, NULL, NULL);
    cl_command_queue queue = clCreateCommandQueue(context, device, 0, NULL);
    
    // Compiler le kernel
    printf("Compilation du kernel...\n");
    cl_program program = clCreateProgramWithSource(context, 1, 
                                                   (const char **)&kernel_source, 
                                                   &kernel_size, NULL);
    
    cl_int err = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    
    if (err != CL_SUCCESS) {
        printf("❌ Erreur compilation: %d\n", err);
        return 1;
    }
    
    printf("✓ Compilation réussie\n\n");
    
    cl_kernel kernel = clCreateKernel(program, "minimal_write", NULL);
    cl_mem output_buf = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 4096, NULL, NULL);
    
    // Initialiser output
    uint32_t init_val = 0xFFFFFFFF;
    clEnqueueFillBuffer(queue, output_buf, &init_val, sizeof(uint32_t), 
                       0, 4096, 0, NULL, NULL);
    
    clSetKernelArg(kernel, 0, sizeof(cl_mem), &output_buf);
    
    // AVANT l'exécution, scanner tous les GEM objects
    printf("=== SCAN GEM OBJECTS AVANT EXÉCUTION ===\n");
    
    // Lister tous les handles via /sys/kernel/debug/dri/0/i915_gem_objects
    system("sudo cat /sys/kernel/debug/dri/0/i915_gem_objects > /tmp/gem_objects_c537_before.txt");
    
    // Exécuter le kernel
    printf("\n=== EXÉCUTION KERNEL ===\n");
    size_t global_work_size = 1;
    clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_work_size, NULL, 
                          0, NULL, NULL);
    
    // APRÈS l'exécution, scanner à nouveau
    printf("\n=== SCAN GEM OBJECTS APRÈS EXÉCUTION ===\n");
    system("sudo cat /sys/kernel/debug/dri/0/i915_gem_objects > /tmp/gem_objects_c537_after.txt");
    
    // Lire le résultat
    uint32_t result[4] = {0};
    clEnqueueReadBuffer(queue, output_buf, CL_TRUE, 0, sizeof(result), 
                       result, 0, NULL, NULL);
    
    printf("\nRésultat OpenCL:\n");
    printf("  output[0] = 0x%08x\n", result[0]);
    
    if (result[0] == 0x12345678) {
        printf("✓ OpenCL fonctionne correctement!\n");
    } else {
        printf("❌ OpenCL a échoué\n");
    }
    
    // Analyser les différences dans les GEM objects
    printf("\n=== ANALYSE DIFFÉRENCES GEM OBJECTS ===\n");
    system("diff /tmp/gem_objects_c537_before.txt /tmp/gem_objects_c537_after.txt > /tmp/gem_diff_c537.txt");
    
    FILE *diff = fopen("/tmp/gem_diff_c537.txt", "r");
    if (diff) {
        char line[1024];
        int count = 0;
        while (fgets(line, sizeof(line), diff) && count < 50) {
            printf("%s", line);
            count++;
        }
        fclose(diff);
    }
    
    // Maintenant, essayer de dumper les buffers via i915_gem_objects
    printf("\n=== TENTATIVE DUMP BUFFERS VIA DEBUGFS ===\n");
    
    // Parser le fichier gem_objects pour trouver les handles
    FILE *gem_file = fopen("/tmp/gem_objects_c537_after.txt", "r");
    if (gem_file) {
        char line[1024];
        int buffer_idx = 0;
        
        while (fgets(line, sizeof(line), gem_file) && buffer_idx < 20) {
            uint32_t handle;
            uint64_t size;
            
            // Format: handle size ...
            if (sscanf(line, "%u %lu", &handle, &size) == 2) {
                if (size > 0 && size < 1024*1024) {  // Filtrer tailles raisonnables
                    printf("\nBuffer[%d]: handle=%u size=%lu\n", 
                           buffer_idx, handle, size);
                    
                    // Essayer de mapper via mmap
                    struct drm_i915_gem_mmap mmap_arg = {
                        .handle = handle,
                        .offset = 0,
                        .size = size,
                        .flags = 0
                    };
                    
                    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_arg) == 0) {
                        void *addr = (void *)(uintptr_t)mmap_arg.addr_ptr;
                        
                        printf("✓ Mappé à %p\n", addr);
                        
                        // Sauvegarder
                        char filename[256];
                        snprintf(filename, sizeof(filename), 
                                "/tmp/buffer_c537_%d_h%u_s%lu.bin", 
                                buffer_idx, handle, size);
                        save_buffer_to_file(filename, addr, size);
                        
                        // Analyser selon la taille
                        if (size == 32) {
                            printf("→ Taille 32 bytes: probablement IDRT\n");
                            analyze_idrt(addr, size);
                        } else if (size == 64) {
                            printf("→ Taille 64 bytes: probablement Surface State\n");
                            analyze_surface_state(addr, size);
                        } else if (size <= 256 && size % 4 == 0) {
                            printf("→ Taille %lu bytes: possiblement Binding Table\n", size);
                            analyze_binding_table(addr, size);
                        }
                        
                        dump_buffer_hex(filename, addr, size > 256 ? 256 : size);
                        
                        munmap(addr, size);
                        buffer_idx++;
                    }
                }
            }
        }
        fclose(gem_file);
        
        printf("\n✓ %d buffers analysés\n", buffer_idx);
    }
    
    // Cleanup
    clReleaseMemObject(output_buf);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    free(kernel_source);
    close(drm_fd);
    
    printf("\n=== FICHIERS GÉNÉRÉS ===\n");
    printf("/tmp/gem_objects_c537_before.txt\n");
    printf("/tmp/gem_objects_c537_after.txt\n");
    printf("/tmp/gem_diff_c537.txt\n");
    printf("/tmp/buffer_c537_*.bin\n");
    
    return 0;
}

// Made with Bob