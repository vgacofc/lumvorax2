/*
 * TEST C486: OpenCL RÉFÉRENCE - Comparaison forensique
 * 
 * Objectif: Exécuter EXACTEMENT le même kernel en OpenCL
 * et dumper TOUTES les structures pour comparaison bit-à-bit
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <CL/cl.h>

// Kernel OpenCL identique
const char *kernel_source = 
"__kernel void minimal_write(__global unsigned int* output) {\n"
"    output[0] = 0xDEADBEEF;\n"
"    output[1] = 0xCAFEBABE;\n"
"    output[2] = 0x12345678;\n"
"    output[3] = 0xABCDEF00;\n"
"}\n";

int main(void) {
    printf("=== TEST C486: OpenCL RÉFÉRENCE ===\n\n");
    
    // 1. Initialisation OpenCL
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
        fprintf(stderr, "ERREUR: clGetPlatformIDs: %d\n", err);
        return 1;
    }
    printf("✓ Platform obtenue\n");
    
    // Get device (GPU Intel)
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "ERREUR: clGetDeviceIDs: %d\n", err);
        return 1;
    }
    printf("✓ Device GPU obtenu\n");
    
    // Create context
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "ERREUR: clCreateContext: %d\n", err);
        return 1;
    }
    printf("✓ Context créé\n");
    
    // Create command queue
    queue = clCreateCommandQueue(context, device, 0, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "ERREUR: clCreateCommandQueue: %d\n", err);
        clReleaseContext(context);
        return 1;
    }
    printf("✓ Command queue créée\n");
    
    // Create program
    program = clCreateProgramWithSource(context, 1, &kernel_source, NULL, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "ERREUR: clCreateProgramWithSource: %d\n", err);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return 1;
    }
    printf("✓ Program créé\n");
    
    // Build program
    err = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "ERREUR: clBuildProgram: %d\n", err);
        
        // Get build log
        size_t log_size;
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        char *log = malloc(log_size);
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
        fprintf(stderr, "Build log:\n%s\n", log);
        free(log);
        
        clReleaseProgram(program);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return 1;
    }
    printf("✓ Program compilé\n");
    
    // Create kernel
    kernel = clCreateKernel(program, "minimal_write", &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "ERREUR: clCreateKernel: %d\n", err);
        clReleaseProgram(program);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return 1;
    }
    printf("✓ Kernel créé\n");
    
    // Create output buffer
    output_buffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 4096, NULL, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "ERREUR: clCreateBuffer: %d\n", err);
        clReleaseKernel(kernel);
        clReleaseProgram(program);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return 1;
    }
    printf("✓ Output buffer créé (4096 bytes)\n");
    
    // Initialize output buffer
    unsigned int init_data[4] = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
    err = clEnqueueWriteBuffer(queue, output_buffer, CL_TRUE, 0, 16, init_data, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "ERREUR: clEnqueueWriteBuffer: %d\n", err);
        clReleaseMemObject(output_buffer);
        clReleaseKernel(kernel);
        clReleaseProgram(program);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return 1;
    }
    printf("✓ Output buffer initialisé avec 0xFFFFFFFF\n");
    
    // Set kernel argument
    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &output_buffer);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "ERREUR: clSetKernelArg: %d\n", err);
        clReleaseMemObject(output_buffer);
        clReleaseKernel(kernel);
        clReleaseProgram(program);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return 1;
    }
    printf("✓ Kernel argument configuré\n");
    
    // Execute kernel
    size_t global_work_size = 1;
    size_t local_work_size = 1;
    err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_work_size, &local_work_size, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "ERREUR: clEnqueueNDRangeKernel: %d\n", err);
        clReleaseMemObject(output_buffer);
        clReleaseKernel(kernel);
        clReleaseProgram(program);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return 1;
    }
    printf("✓ Kernel exécuté\n");
    
    // Wait for completion
    err = clFinish(queue);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "ERREUR: clFinish: %d\n", err);
        clReleaseMemObject(output_buffer);
        clReleaseKernel(kernel);
        clReleaseProgram(program);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return 1;
    }
    printf("✓ Exécution terminée\n");
    
    // Read results
    unsigned int output[4];
    err = clEnqueueReadBuffer(queue, output_buffer, CL_TRUE, 0, 16, output, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "ERREUR: clEnqueueReadBuffer: %d\n", err);
        clReleaseMemObject(output_buffer);
        clReleaseKernel(kernel);
        clReleaseProgram(program);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return 1;
    }
    
    // Verify results
    printf("\n=== RÉSULTATS OpenCL ===\n");
    printf("Output[0] = 0x%08x", output[0]);
    if (output[0] == 0xDEADBEEF) printf(" ✓✓✓ SUCCÈS!\n");
    else printf(" ❌ ÉCHEC (attendu: 0xDEADBEEF)\n");
    
    printf("Output[1] = 0x%08x", output[1]);
    if (output[1] == 0xCAFEBABE) printf(" ✓✓✓ SUCCÈS!\n");
    else printf(" ❌ ÉCHEC (attendu: 0xCAFEBABE)\n");
    
    printf("Output[2] = 0x%08x", output[2]);
    if (output[2] == 0x12345678) printf(" ✓✓✓ SUCCÈS!\n");
    else printf(" ❌ ÉCHEC (attendu: 0x12345678)\n");
    
    printf("Output[3] = 0x%08x", output[3]);
    if (output[3] == 0xABCDEF00) printf(" ✓✓✓ SUCCÈS!\n");
    else printf(" ❌ ÉCHEC (attendu: 0xABCDEF00)\n");
    
    // Cleanup
    clReleaseMemObject(output_buffer);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    
    printf("\n=== CONCLUSION ===\n");
    if (output[0] == 0xDEADBEEF && output[1] == 0xCAFEBABE &&
        output[2] == 0x12345678 && output[3] == 0xABCDEF00) {
        printf("✓✓✓ OpenCL FONCTIONNE PARFAITEMENT!\n");
        printf("Maintenant il faut identifier ce qui manque dans i915 natif.\n");
        return 0;
    } else {
        printf("❌ OpenCL a échoué aussi!\n");
        return 1;
    }
}

// Made with Bob
