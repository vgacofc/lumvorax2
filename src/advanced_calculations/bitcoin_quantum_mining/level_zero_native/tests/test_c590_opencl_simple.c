/*
 * TEST C590 - OpenCL Simple pour Capture Batch
 * ============================================
 * 
 * Programme OpenCL minimal pour capturer le batch buffer complet
 * avec test_c590_opencl_batch_capture.so
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <CL/cl.h>

// Kernel OpenCL simple qui écrit une valeur
const char* kernel_source = 
"__kernel void simple_write(__global uint* output) {\n"
"    int gid = get_global_id(0);\n"
"    output[gid] = 0x12345678 + gid;\n"
"}\n";

int main() {
    cl_int err;
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;
    cl_program program;
    cl_kernel kernel;
    cl_mem output_buffer;
    
    printf("🚀 TEST C590 - OpenCL Simple\n");
    printf("═══════════════════════════════════════\n\n");
    
    // Get platform
    err = clGetPlatformIDs(1, &platform, NULL);
    if (err != CL_SUCCESS) {
        printf("❌ Failed to get platform: %d\n", err);
        return 1;
    }
    printf("✅ Platform obtained\n");
    
    // Get device
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    if (err != CL_SUCCESS) {
        printf("❌ Failed to get device: %d\n", err);
        return 1;
    }
    printf("✅ GPU device obtained\n");
    
    // Create context
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    if (err != CL_SUCCESS) {
        printf("❌ Failed to create context: %d\n", err);
        return 1;
    }
    printf("✅ Context created\n");
    
    // Create command queue
    queue = clCreateCommandQueue(context, device, 0, &err);
    if (err != CL_SUCCESS) {
        printf("❌ Failed to create queue: %d\n", err);
        return 1;
    }
    printf("✅ Command queue created\n");
    
    // Create program
    program = clCreateProgramWithSource(context, 1, &kernel_source, NULL, &err);
    if (err != CL_SUCCESS) {
        printf("❌ Failed to create program: %d\n", err);
        return 1;
    }
    printf("✅ Program created\n");
    
    // Build program
    err = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    if (err != CL_SUCCESS) {
        printf("❌ Failed to build program: %d\n", err);
        
        // Get build log
        size_t log_size;
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        char* log = malloc(log_size);
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
        printf("Build log:\n%s\n", log);
        free(log);
        return 1;
    }
    printf("✅ Program built\n");
    
    // Create kernel
    kernel = clCreateKernel(program, "simple_write", &err);
    if (err != CL_SUCCESS) {
        printf("❌ Failed to create kernel: %d\n", err);
        return 1;
    }
    printf("✅ Kernel created\n");
    
    // Create output buffer
    output_buffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 256 * sizeof(unsigned int), NULL, &err);
    if (err != CL_SUCCESS) {
        printf("❌ Failed to create buffer: %d\n", err);
        return 1;
    }
    printf("✅ Output buffer created (256 uints)\n");
    
    // Set kernel argument
    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &output_buffer);
    if (err != CL_SUCCESS) {
        printf("❌ Failed to set kernel arg: %d\n", err);
        return 1;
    }
    printf("✅ Kernel argument set\n");
    
    // Execute kernel
    printf("\n🎯 Executing kernel (256 work items)...\n");
    size_t global_work_size = 256;
    size_t local_work_size = 16;
    
    err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_work_size, &local_work_size, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        printf("❌ Failed to enqueue kernel: %d\n", err);
        return 1;
    }
    
    // Wait for completion
    err = clFinish(queue);
    if (err != CL_SUCCESS) {
        printf("❌ Failed to finish: %d\n", err);
        return 1;
    }
    printf("✅ Kernel executed successfully\n");
    
    // Read results
    unsigned int output[256];
    err = clEnqueueReadBuffer(queue, output_buffer, CL_TRUE, 0, 256 * sizeof(unsigned int), output, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        printf("❌ Failed to read buffer: %d\n", err);
        return 1;
    }
    
    printf("\n📊 Results (first 4 values):\n");
    for (int i = 0; i < 4; i++) {
        printf("  output[%d] = 0x%08x\n", i, output[i]);
    }
    
    // Cleanup
    clReleaseMemObject(output_buffer);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    
    printf("\n✅ TEST C590 TERMINÉ\n");
    return 0;
}

// Made with Bob
