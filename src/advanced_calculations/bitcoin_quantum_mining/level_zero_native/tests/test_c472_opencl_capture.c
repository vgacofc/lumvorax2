/*
 * TEST C472 - CAPTURE BATCH OPENCL AVEC AUBDUMP
 * 
 * Objectif: Reverse engineer le batch buffer OpenCL pour comprendre
 * la configuration GPGPU complète nécessaire
 * 
 * Programme OpenCL minimal qui exécute un kernel NOP
 * À exécuter avec: aubdump -c ./test_c472_opencl_capture
 */

#define CL_TARGET_OPENCL_VERSION 120
#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define FORENSIC_LOG(fmt, ...) do { \
    struct timespec ts; \
    clock_gettime(CLOCK_MONOTONIC, &ts); \
    fprintf(stderr, "[%ld.%09ld] " fmt "\n", ts.tv_sec, ts.tv_nsec, ##__VA_ARGS__); \
} while(0)

/* Kernel OpenCL minimal - NOP */
const char* kernel_source = 
"__kernel void nop_kernel(__global uint* output) {\n"
"    int gid = get_global_id(0);\n"
"    output[gid] = gid;\n"
"}\n";

int main(void) {
    FORENSIC_LOG("🚀 TEST C472 - CAPTURE BATCH OPENCL");
    FORENSIC_LOG("🎯 Objectif: Reverse engineer configuration GPGPU OpenCL");
    
    cl_int err;
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;
    cl_program program;
    cl_kernel kernel;
    cl_mem output_buffer;
    
    /* Obtenir plateforme OpenCL */
    err = clGetPlatformIDs(1, &platform, NULL);
    if (err != CL_SUCCESS) {
        FORENSIC_LOG("❌ clGetPlatformIDs failed: %d", err);
        return 1;
    }
    FORENSIC_LOG("✅ Plateforme OpenCL obtenue");
    
    /* Obtenir device GPU */
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    if (err != CL_SUCCESS) {
        FORENSIC_LOG("❌ clGetDeviceIDs failed: %d", err);
        return 1;
    }
    FORENSIC_LOG("✅ Device GPU obtenu");
    
    /* Créer contexte */
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    if (err != CL_SUCCESS) {
        FORENSIC_LOG("❌ clCreateContext failed: %d", err);
        return 1;
    }
    FORENSIC_LOG("✅ Contexte OpenCL créé");
    
    /* Créer command queue */
    queue = clCreateCommandQueue(context, device, 0, &err);
    if (err != CL_SUCCESS) {
        FORENSIC_LOG("❌ clCreateCommandQueue failed: %d", err);
        clReleaseContext(context);
        return 1;
    }
    FORENSIC_LOG("✅ Command queue créée");
    
    /* Créer programme */
    program = clCreateProgramWithSource(context, 1, &kernel_source, NULL, &err);
    if (err != CL_SUCCESS) {
        FORENSIC_LOG("❌ clCreateProgramWithSource failed: %d", err);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return 1;
    }
    FORENSIC_LOG("✅ Programme créé");
    
    /* Compiler programme */
    err = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    if (err != CL_SUCCESS) {
        FORENSIC_LOG("❌ clBuildProgram failed: %d", err);
        
        /* Afficher log de compilation */
        size_t log_size;
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        char* log = malloc(log_size);
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
        FORENSIC_LOG("Build log:\n%s", log);
        free(log);
        
        clReleaseProgram(program);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return 1;
    }
    FORENSIC_LOG("✅ Programme compilé");
    
    /* Créer kernel */
    kernel = clCreateKernel(program, "nop_kernel", &err);
    if (err != CL_SUCCESS) {
        FORENSIC_LOG("❌ clCreateKernel failed: %d", err);
        clReleaseProgram(program);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return 1;
    }
    FORENSIC_LOG("✅ Kernel créé");
    
    /* Créer buffer output */
    output_buffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 256 * sizeof(uint32_t), NULL, &err);
    if (err != CL_SUCCESS) {
        FORENSIC_LOG("❌ clCreateBuffer failed: %d", err);
        clReleaseKernel(kernel);
        clReleaseProgram(program);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return 1;
    }
    FORENSIC_LOG("✅ Buffer output créé (256 uint32)");
    
    /* Set kernel argument */
    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &output_buffer);
    if (err != CL_SUCCESS) {
        FORENSIC_LOG("❌ clSetKernelArg failed: %d", err);
        clReleaseMemObject(output_buffer);
        clReleaseKernel(kernel);
        clReleaseProgram(program);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return 1;
    }
    FORENSIC_LOG("✅ Kernel argument configuré");
    
    /* Exécuter kernel */
    size_t global_work_size = 256;
    size_t local_work_size = 16;
    
    FORENSIC_LOG("⚡ Exécution kernel (256 work-items, 16 local)...");
    FORENSIC_LOG("📊 AUBDUMP CAPTURE ICI - Batch buffer OpenCL généré");
    
    err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_work_size, &local_work_size, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        FORENSIC_LOG("❌ clEnqueueNDRangeKernel failed: %d", err);
        clReleaseMemObject(output_buffer);
        clReleaseKernel(kernel);
        clReleaseProgram(program);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return 1;
    }
    
    /* Attendre fin */
    err = clFinish(queue);
    if (err != CL_SUCCESS) {
        FORENSIC_LOG("❌ clFinish failed: %d", err);
        clReleaseMemObject(output_buffer);
        clReleaseKernel(kernel);
        clReleaseProgram(program);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return 1;
    }
    
    FORENSIC_LOG("✅ Kernel exécuté avec succès");
    FORENSIC_LOG("🎉 TEST C472 RÉUSSI - Batch OpenCL capturé par aubdump");
    FORENSIC_LOG("📊 Analyser: aubdump.trace et aubdump.aub");
    
    /* Cleanup */
    clReleaseMemObject(output_buffer);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    
    return 0;
}

// Made with Bob