/*
 * TEST C449 - DUMP SSH OPENCL pour comparaison
 * Objectif: Capturer la configuration Surface State d'OpenCL
 */

#define CL_TARGET_OPENCL_VERSION 120
#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#define FORENSIC_LOG(fmt, ...) do { \
    struct timespec ts; \
    clock_gettime(CLOCK_MONOTONIC, &ts); \
    fprintf(stderr, "[%ld.%09ld] " fmt "\n", ts.tv_sec, ts.tv_nsec, ##__VA_ARGS__); \
} while(0)

const char* kernel_source = 
"__kernel void write_test(__global uint* output)\n"
"{\n"
"    int gid = get_global_id(0);\n"
"    output[gid] = 0xAABBCCDD;\n"
"}\n";

int main() {
    FORENSIC_LOG("🚀 TEST C449 - DUMP SSH OPENCL");
    
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;
    cl_program program;
    cl_kernel kernel;
    cl_mem output_buffer;
    cl_int err;
    
    // Get platform
    err = clGetPlatformIDs(1, &platform, NULL);
    if (err != CL_SUCCESS) {
        FORENSIC_LOG("❌ clGetPlatformIDs failed: %d", err);
        return 1;
    }
    
    // Get device
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    if (err != CL_SUCCESS) {
        FORENSIC_LOG("❌ clGetDeviceIDs failed: %d", err);
        return 1;
    }
    
    char device_name[256];
    clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(device_name), device_name, NULL);
    FORENSIC_LOG("✅ Device: %s", device_name);
    
    // Create context
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    if (err != CL_SUCCESS) {
        FORENSIC_LOG("❌ clCreateContext failed: %d", err);
        return 1;
    }
    
    // Create queue
    queue = clCreateCommandQueue(context, device, 0, &err);
    if (err != CL_SUCCESS) {
        FORENSIC_LOG("❌ clCreateCommandQueue failed: %d", err);
        return 1;
    }
    
    // Create program
    program = clCreateProgramWithSource(context, 1, &kernel_source, NULL, &err);
    if (err != CL_SUCCESS) {
        FORENSIC_LOG("❌ clCreateProgramWithSource failed: %d", err);
        return 1;
    }
    
    // Build program
    err = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    if (err != CL_SUCCESS) {
        FORENSIC_LOG("❌ clBuildProgram failed: %d", err);
        size_t log_size;
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        char* log = malloc(log_size);
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
        FORENSIC_LOG("Build log:\n%s", log);
        free(log);
        return 1;
    }
    
    FORENSIC_LOG("✅ Program built successfully");
    
    // Create kernel
    kernel = clCreateKernel(program, "write_test", &err);
    if (err != CL_SUCCESS) {
        FORENSIC_LOG("❌ clCreateKernel failed: %d", err);
        return 1;
    }
    
    // Create buffer
    output_buffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 256 * sizeof(uint32_t), NULL, &err);
    if (err != CL_SUCCESS) {
        FORENSIC_LOG("❌ clCreateBuffer failed: %d", err);
        return 1;
    }
    
    // Set kernel arg
    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &output_buffer);
    if (err != CL_SUCCESS) {
        FORENSIC_LOG("❌ clSetKernelArg failed: %d", err);
        return 1;
    }
    
    FORENSIC_LOG("✅ Kernel configured");
    
    // Execute kernel
    size_t global_size = 256;
    err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_size, NULL, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        FORENSIC_LOG("❌ clEnqueueNDRangeKernel failed: %d", err);
        return 1;
    }
    
    // Wait
    clFinish(queue);
    FORENSIC_LOG("✅ Kernel executed");
    
    // Read results
    uint32_t output[256];
    err = clEnqueueReadBuffer(queue, output_buffer, CL_TRUE, 0, 256 * sizeof(uint32_t), output, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        FORENSIC_LOG("❌ clEnqueueReadBuffer failed: %d", err);
        return 1;
    }
    
    // Verify
    FORENSIC_LOG("📊 Vérification résultats:");
    int success = 1;
    for (int i = 0; i < 256; i++) {
        if (output[i] != 0xAABBCCDD) {
            FORENSIC_LOG("❌ output[%d] = 0x%08X (attendu: 0xAABBCCDD)", i, output[i]);
            success = 0;
            break;
        }
    }
    
    if (success) {
        FORENSIC_LOG("✅ TEST RÉUSSI: Tous les threads ont écrit 0xAABBCCDD");
    } else {
        FORENSIC_LOG("❌ TEST ÉCHOUÉ");
    }
    
    // Cleanup
    clReleaseMemObject(output_buffer);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    
    return success ? 0 : 1;
}

// Made with Bob
