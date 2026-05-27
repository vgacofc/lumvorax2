/**
 * C417 - DUMP BATCH BUFFER OPENCL FONCTIONNEL
 * 
 * Stratégie: Utiliser OpenCL pour exécuter le kernel simple,
 * puis dumper TOUS les buffers GPU via /sys/kernel/debug/dri/1/i915_gem_objects
 * 
 * Cela nous permettra de voir EXACTEMENT comment OpenCL configure:
 * - Le batch buffer
 * - Les Surface States
 * - L'Interface Descriptor
 * - Le CURBE
 * 
 * Comparaison directe avec notre batch manuel C415
 */

#define CL_TARGET_OPENCL_VERSION 120
#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#define NUM_THREADS 256

const char *kernel_source = 
"__kernel void simple_write(__global uint *output) {\n"
"    uint tid = get_global_id(0);\n"
"    output[tid] = (4 << 16) | tid;\n"
"}\n";

static void dump_i915_objects(void) {
    printf("\n=== DUMP i915 GEM OBJECTS ===\n");
    system("sudo cat /sys/kernel/debug/dri/1/i915_gem_objects 2>/dev/null | head -100");
    printf("=== END DUMP ===\n\n");
}

static void dump_buffer_via_debugfs(uint32_t handle) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), 
        "sudo cat /sys/kernel/debug/dri/1/i915_gem_gtt 2>/dev/null | grep -A 20 'handle %u'", 
        handle);
    printf("\n=== Buffer handle=%u ===\n", handle);
    system(cmd);
    printf("=== END ===\n\n");
}

int main(void) {
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════════╗\n");
    printf("║  C417 - DUMP BATCH BUFFER OPENCL                             ║\n");
    printf("║  Analyse du batch buffer généré par OpenCL                   ║\n");
    printf("╚═══════════════════════════════════════════════════════════════╝\n\n");
    
    cl_platform_id platform;
    clGetPlatformIDs(1, &platform, NULL);
    
    cl_device_id device;
    clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    
    char device_name[256];
    clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(device_name), device_name, NULL);
    printf("Device: %s\n\n", device_name);
    
    cl_context context = clCreateContext(NULL, 1, &device, NULL, NULL, NULL);
    cl_command_queue queue = clCreateCommandQueue(context, device, 0, NULL);
    
    printf("=== AVANT EXÉCUTION ===\n");
    dump_i915_objects();
    
    // Créer le buffer output
    cl_mem output_buf = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 
                                       NUM_THREADS * sizeof(uint32_t), NULL, NULL);
    
    // Initialiser avec pattern 0xCC
    uint32_t init_pattern[NUM_THREADS];
    memset(init_pattern, 0xCC, sizeof(init_pattern));
    clEnqueueWriteBuffer(queue, output_buf, CL_TRUE, 0, sizeof(init_pattern), 
                         init_pattern, 0, NULL, NULL);
    
    // Compiler le kernel
    cl_program program = clCreateProgramWithSource(context, 1, &kernel_source, NULL, NULL);
    cl_int ret = clBuildProgram(program, 1, &device, "-cl-std=CL1.2", NULL, NULL);
    
    if (ret != CL_SUCCESS) {
        printf("ERROR: Build failed: %d\n", ret);
        size_t log_size;
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        char *log = malloc(log_size);
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
        printf("%s\n", log);
        free(log);
        return 1;
    }
    
    cl_kernel kernel = clCreateKernel(program, "simple_write", NULL);
    clSetKernelArg(kernel, 0, sizeof(cl_mem), &output_buf);
    
    printf("=== JUSTE AVANT clEnqueueNDRangeKernel ===\n");
    printf("Pause de 2 secondes pour dump...\n");
    sleep(2);
    dump_i915_objects();
    
    // Exécuter le kernel
    size_t global_size = NUM_THREADS;
    size_t local_size = 16;  // SIMD16
    
    printf("=== EXÉCUTION KERNEL ===\n");
    printf("global_size=%zu, local_size=%zu\n", global_size, local_size);
    
    ret = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_size, &local_size, 
                                 0, NULL, NULL);
    if (ret != CL_SUCCESS) {
        printf("ERROR: Kernel execution failed: %d\n", ret);
        return 1;
    }
    
    clFinish(queue);
    
    printf("=== APRÈS EXÉCUTION ===\n");
    dump_i915_objects();
    
    // Lire les résultats
    uint32_t results[NUM_THREADS];
    clEnqueueReadBuffer(queue, output_buf, CL_TRUE, 0, sizeof(results), 
                        results, 0, NULL, NULL);
    
    // Vérifier les résultats
    int valid = 0;
    for (int i = 0; i < NUM_THREADS; i++) {
        uint32_t expected = (4 << 16) | i;
        if (results[i] == expected) {
            valid++;
        }
    }
    
    printf("\n=== RÉSULTATS ===\n");
    printf("Valid: %d / %d\n", valid, NUM_THREADS);
    
    if (valid > 0) {
        printf("\n✅ OpenCL fonctionne! Premiers résultats:\n");
        for (int i = 0; i < 16; i++) {
            printf("  output[%d] = 0x%08x\n", i, results[i]);
        }
    } else {
        printf("\n❌ OpenCL a échoué!\n");
    }
    
    // Dump détaillé des buffers
    printf("\n=== ANALYSE DÉTAILLÉE ===\n");
    printf("Pour analyser les batch buffers, utiliser:\n");
    printf("  sudo cat /sys/kernel/debug/dri/1/i915_gem_objects\n");
    printf("  sudo intel_gpu_top\n");
    printf("  sudo cat /sys/kernel/debug/dri/1/i915_capabilities\n");
    
    clReleaseMemObject(output_buf);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════════╗\n");
    printf("║  PROCHAINE ÉTAPE:                                            ║\n");
    printf("║  Comparer les dumps avec notre batch buffer C415             ║\n");
    printf("║  Identifier les différences critiques                        ║\n");
    printf("╚═══════════════════════════════════════════════════════════════╝\n\n");
    
    return (valid > 0) ? 0 : 1;
}

// Made with Bob
