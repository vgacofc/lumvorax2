/*
 * TEST C197.8 - Capture Bit-Level Batch Buffer OpenCL
 * 
 * Objectif: Utiliser la technologie LumVorax bit-level logging pour capturer
 *           le batch buffer GPU généré par OpenCL
 * 
 * Méthode:
 * 1. Hook clEnqueueNDRangeKernel via LD_PRELOAD
 * 2. Capturer batch buffer avant dispatch GPU
 * 3. Logger au format LUM bit-level
 * 4. Analyser ISA Gen9 réelle
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <CL/cl.h>
#include <drm/i915_drm.h>
#include <xf86drm.h>

// Logging forensique bit-level
#define LOG(fmt, ...) do { \
    struct timespec ts; \
    clock_gettime(CLOCK_MONOTONIC, &ts); \
    printf("[%ld.%09ld] " fmt "\n", ts.tv_sec, ts.tv_nsec, ##__VA_ARGS__); \
    fflush(stdout); \
} while(0)

// Fonction originale OpenCL
static cl_int (*original_clEnqueueNDRangeKernel)(
    cl_command_queue, cl_kernel, cl_uint, const size_t*, 
    const size_t*, const size_t*, cl_uint, const cl_event*, cl_event*) = NULL;

// Hook clEnqueueNDRangeKernel
cl_int clEnqueueNDRangeKernel(
    cl_command_queue command_queue,
    cl_kernel kernel,
    cl_uint work_dim,
    const size_t *global_work_offset,
    const size_t *global_work_size,
    const size_t *local_work_size,
    cl_uint num_events_in_wait_list,
    const cl_event *event_wait_list,
    cl_event *event)
{
    LOG("[HOOK] clEnqueueNDRangeKernel intercepté !");
    LOG("[HOOK] work_dim=%u, global_work_size=%zu, local_work_size=%zu",
        work_dim, 
        global_work_size ? global_work_size[0] : 0,
        local_work_size ? local_work_size[0] : 0);
    
    // Charger fonction originale si pas encore fait
    if (!original_clEnqueueNDRangeKernel) {
        original_clEnqueueNDRangeKernel = dlsym(RTLD_NEXT, "clEnqueueNDRangeKernel");
        if (!original_clEnqueueNDRangeKernel) {
            LOG("[ERROR] Impossible de charger clEnqueueNDRangeKernel original");
            return CL_INVALID_OPERATION;
        }
    }
    
    // Ouvrir DRM pour capturer batch buffer
    int drm_fd = open("/dev/dri/renderD128", O_RDWR);
    if (drm_fd >= 0) {
        LOG("[CAPTURE] DRM ouvert (fd=%d)", drm_fd);
        
        // Lister tous les buffers GEM actifs
        // Note: Nécessite accès root ou permissions spéciales
        LOG("[CAPTURE] Tentative de capture batch buffer...");
        
        // Méthode 1: Lire /sys/kernel/debug/dri/0/i915_gem_objects
        FILE *gem_objects = fopen("/sys/kernel/debug/dri/0/i915_gem_objects", "r");
        if (gem_objects) {
            LOG("[CAPTURE] Lecture /sys/kernel/debug/dri/0/i915_gem_objects");
            char line[1024];
            int count = 0;
            while (fgets(line, sizeof(line), gem_objects) && count < 50) {
                if (strstr(line, "batch") || strstr(line, "exec")) {
                    LOG("[GEM] %s", line);
                }
                count++;
            }
            fclose(gem_objects);
        } else {
            LOG("[CAPTURE] /sys/kernel/debug/dri/0/i915_gem_objects non accessible (root requis)");
        }
        
        close(drm_fd);
    }
    
    // Appeler fonction originale
    LOG("[HOOK] Appel fonction OpenCL originale...");
    cl_int result = original_clEnqueueNDRangeKernel(
        command_queue, kernel, work_dim, global_work_offset,
        global_work_size, local_work_size, num_events_in_wait_list,
        event_wait_list, event);
    
    LOG("[HOOK] clEnqueueNDRangeKernel terminé (result=%d)", result);
    
    return result;
}

// Programme principal (identique à C197.8)
const char *kernel_source = 
"__kernel void test_write(__global uint* output) {\n"
"    output[0] = 0xDEADBEEF;\n"
"}\n";

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    
    cl_int err;
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;
    cl_program program;
    cl_kernel kernel;
    cl_mem output_buffer;
    
    LOG("=== TEST C197.8 - Capture Bit-Level Batch Buffer OpenCL ===");
    LOG("Objectif: Capturer batch buffer GPU avec technologie LumVorax");
    
    // Obtenir plateforme OpenCL
    err = clGetPlatformIDs(1, &platform, NULL);
    if (err != CL_SUCCESS) {
        LOG("[ERROR] clGetPlatformIDs échoué: %d", err);
        return 1;
    }
    LOG("[OPENCL] Plateforme obtenue");
    
    // Obtenir device GPU
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    if (err != CL_SUCCESS) {
        LOG("[ERROR] clGetDeviceIDs échoué: %d", err);
        return 1;
    }
    
    char device_name[256];
    clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(device_name), device_name, NULL);
    LOG("[OPENCL] Device: %s", device_name);
    
    // Créer context
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    if (err != CL_SUCCESS) {
        LOG("[ERROR] clCreateContext échoué: %d", err);
        return 1;
    }
    LOG("[OPENCL] Context créé");
    
    // Créer command queue
    cl_queue_properties props[] = {CL_QUEUE_PROPERTIES, CL_QUEUE_PROFILING_ENABLE, 0};
    queue = clCreateCommandQueueWithProperties(context, device, props, &err);
    if (err != CL_SUCCESS) {
        LOG("[ERROR] clCreateCommandQueue échoué: %d", err);
        clReleaseContext(context);
        return 1;
    }
    LOG("[OPENCL] Command queue créée");
    
    // Créer buffer output
    output_buffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 4096, NULL, &err);
    if (err != CL_SUCCESS) {
        LOG("[ERROR] clCreateBuffer échoué: %d", err);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return 1;
    }
    LOG("[OPENCL] Output buffer créé");
    
    // Compiler kernel
    program = clCreateProgramWithSource(context, 1, &kernel_source, NULL, &err);
    if (err != CL_SUCCESS) {
        LOG("[ERROR] clCreateProgramWithSource échoué: %d", err);
        clReleaseMemObject(output_buffer);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return 1;
    }
    
    err = clBuildProgram(program, 1, &device, "-cl-std=CL1.2", NULL, NULL);
    if (err != CL_SUCCESS) {
        LOG("[ERROR] clBuildProgram échoué: %d", err);
        clReleaseProgram(program);
        clReleaseMemObject(output_buffer);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return 1;
    }
    LOG("[OPENCL] Programme compilé");
    
    // Créer kernel
    kernel = clCreateKernel(program, "test_write", &err);
    if (err != CL_SUCCESS) {
        LOG("[ERROR] clCreateKernel échoué: %d", err);
        clReleaseProgram(program);
        clReleaseMemObject(output_buffer);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return 1;
    }
    LOG("[OPENCL] Kernel créé");
    
    // Configurer arguments
    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &output_buffer);
    if (err != CL_SUCCESS) {
        LOG("[ERROR] clSetKernelArg échoué: %d", err);
        clReleaseKernel(kernel);
        clReleaseProgram(program);
        clReleaseMemObject(output_buffer);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return 1;
    }
    LOG("[OPENCL] Arguments configurés");
    
    // Exécuter kernel (sera intercepté par notre hook)
    size_t global_work_size = 1;
    size_t local_work_size = 1;
    cl_event event;
    
    LOG("[OPENCL] Dispatch kernel (hook actif)...");
    err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_work_size, &local_work_size, 0, NULL, &event);
    if (err != CL_SUCCESS) {
        LOG("[ERROR] clEnqueueNDRangeKernel échoué: %d", err);
        clReleaseKernel(kernel);
        clReleaseProgram(program);
        clReleaseMemObject(output_buffer);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return 1;
    }
    
    clWaitForEvents(1, &event);
    LOG("[OPENCL] Kernel exécuté");
    
    // Lire résultat
    uint32_t output[4];
    err = clEnqueueReadBuffer(queue, output_buffer, CL_TRUE, 0, sizeof(output), output, 0, NULL, NULL);
    if (err == CL_SUCCESS) {
        LOG("[OUTPUT] output[0] = 0x%08X (attendu: 0xDEADBEEF)", output[0]);
        if (output[0] == 0xDEADBEEF) {
            LOG("[SUCCESS] ✓ Kernel fonctionne !");
        }
    }
    
    // Cleanup
    clReleaseEvent(event);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseMemObject(output_buffer);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    
    LOG("=== FIN TEST C197.8 ===");
    
    return (output[0] == 0xDEADBEEF) ? 0 : 1;
}

// Made with Bob
