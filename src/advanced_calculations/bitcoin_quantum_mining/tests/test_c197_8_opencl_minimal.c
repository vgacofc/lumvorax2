/*
 * TEST C197.8 - Kernel OpenCL Minimal pour Reverse Engineering
 * 
 * Objectif: Créer un kernel OpenCL minimal fonctionnel pour capturer
 *           le batch buffer GPU via intel_aubdump
 * 
 * Kernel: out[0] = 0xDEADBEEF
 * 
 * Méthode:
 * 1. Compiler kernel OpenCL
 * 2. Exécuter avec intel_aubdump
 * 3. Analyser batch buffer capturé
 * 4. Extraire ISA Gen9 réelle
 * 5. Reproduire en DRM natif
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <CL/cl.h>

// Logging forensique
#define LOG(fmt, ...) do { \
    struct timespec ts; \
    clock_gettime(CLOCK_MONOTONIC, &ts); \
    printf("[%ld.%09ld] " fmt "\n", ts.tv_sec, ts.tv_nsec, ##__VA_ARGS__); \
} while(0)

// Kernel OpenCL minimal
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
    
    LOG("=== TEST C197.8 - Kernel OpenCL Minimal ===");
    LOG("Objectif: Capturer batch buffer GPU pour reverse engineering");
    
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
    
    // Afficher info device
    char device_name[256];
    clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(device_name), device_name, NULL);
    LOG("[OPENCL] Device: %s", device_name);
    
    cl_uint compute_units;
    clGetDeviceInfo(device, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(compute_units), &compute_units, NULL);
    LOG("[OPENCL] Compute Units: %u", compute_units);
    
    // Créer context
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    if (err != CL_SUCCESS) {
        LOG("[ERROR] clCreateContext échoué: %d", err);
        return 1;
    }
    LOG("[OPENCL] Context créé");
    
    // Créer command queue avec profiling
    cl_queue_properties props[] = {CL_QUEUE_PROPERTIES, CL_QUEUE_PROFILING_ENABLE, 0};
    queue = clCreateCommandQueueWithProperties(context, device, props, &err);
    if (err != CL_SUCCESS) {
        LOG("[ERROR] clCreateCommandQueue échoué: %d", err);
        clReleaseContext(context);
        return 1;
    }
    LOG("[OPENCL] Command queue créée (profiling enabled)");
    
    // Créer buffer output (4KB)
    output_buffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 4096, NULL, &err);
    if (err != CL_SUCCESS) {
        LOG("[ERROR] clCreateBuffer échoué: %d", err);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return 1;
    }
    LOG("[OPENCL] Output buffer créé (4096 bytes)");
    
    // Compiler kernel
    program = clCreateProgramWithSource(context, 1, &kernel_source, NULL, &err);
    if (err != CL_SUCCESS) {
        LOG("[ERROR] clCreateProgramWithSource échoué: %d", err);
        clReleaseMemObject(output_buffer);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return 1;
    }
    LOG("[OPENCL] Programme créé");
    
    err = clBuildProgram(program, 1, &device, "-cl-std=CL1.2", NULL, NULL);
    if (err != CL_SUCCESS) {
        LOG("[ERROR] clBuildProgram échoué: %d", err);
        
        // Afficher log de compilation
        size_t log_size;
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        char *log = (char*)malloc(log_size);
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
        LOG("[BUILD_LOG]\n%s", log);
        free(log);
        
        clReleaseProgram(program);
        clReleaseMemObject(output_buffer);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return 1;
    }
    LOG("[OPENCL] Programme compilé avec succès");
    
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
    
    // Configurer arguments kernel
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
    LOG("[OPENCL] Arguments kernel configurés");
    
    // Exécuter kernel
    size_t global_work_size = 1;
    size_t local_work_size = 1;
    
    cl_event event;
    struct timespec t_start, t_end;
    clock_gettime(CLOCK_MONOTONIC, &t_start);
    
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
    LOG("[OPENCL] Kernel dispatché (global_work_size=1, local_work_size=1)");
    
    // Attendre fin exécution
    err = clWaitForEvents(1, &event);
    if (err != CL_SUCCESS) {
        LOG("[ERROR] clWaitForEvents échoué: %d", err);
    }
    
    clock_gettime(CLOCK_MONOTONIC, &t_end);
    double exec_ms = (t_end.tv_sec - t_start.tv_sec) * 1000.0 + 
                     (t_end.tv_nsec - t_start.tv_nsec) / 1000000.0;
    LOG("[OPENCL] Kernel exécuté (%.3f ms)", exec_ms);
    
    // Profiling
    cl_ulong time_start, time_end;
    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START, sizeof(time_start), &time_start, NULL);
    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END, sizeof(time_end), &time_end, NULL);
    double kernel_time_ns = (double)(time_end - time_start);
    LOG("[PROFILING] Kernel execution: %.3f ns (%.6f ms)", kernel_time_ns, kernel_time_ns / 1000000.0);
    
    // Lire résultat
    uint32_t output[1024];
    memset(output, 0, sizeof(output));
    
    err = clEnqueueReadBuffer(queue, output_buffer, CL_TRUE, 0, sizeof(output), output, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        LOG("[ERROR] clEnqueueReadBuffer échoué: %d", err);
    } else {
        LOG("[OUTPUT] Lecture résultat GPU:");
        LOG("  output[0] = 0x%08X (attendu: 0xDEADBEEF)", output[0]);
        LOG("  output[1] = 0x%08X", output[1]);
        LOG("  output[2] = 0x%08X", output[2]);
        LOG("  output[3] = 0x%08X", output[3]);
        
        if (output[0] == 0xDEADBEEF) {
            LOG("[SUCCESS] ✓ Kernel OpenCL fonctionne ! output[0] = 0xDEADBEEF");
        } else {
            LOG("[FAILURE] ✗ Kernel OpenCL ne fonctionne pas");
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
