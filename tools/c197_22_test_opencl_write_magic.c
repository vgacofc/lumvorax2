/*
 * C197.22 - Test OpenCL write_magic
 * 
 * Programme OpenCL minimal qui exécute le kernel write_magic
 * pour capturer les vrais ioctl() via l'intercepteur
 */

#include <stdio.h>
#include <stdlib.h>
#include <CL/cl.h>

const char* kernel_source = 
"__kernel void write_magic(__global uint* output) {\n"
"    output[0] = 0x12345678;\n"
"}\n";

int main() {
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;
    cl_program program;
    cl_kernel kernel;
    cl_mem output_buffer;
    cl_int err;
    
    printf("=== OpenCL write_magic Test ===\n\n");
    
    // Get platform
    err = clGetPlatformIDs(1, &platform, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Failed to get platform: %d\n", err);
        return 1;
    }
    printf("✅ Platform obtained\n");
    
    // Get device
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Failed to get device: %d\n", err);
        return 1;
    }
    printf("✅ Device obtained\n");
    
    // Create context
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Failed to create context: %d\n", err);
        return 1;
    }
    printf("✅ Context created\n");
    
    // Create command queue
    queue = clCreateCommandQueue(context, device, 0, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Failed to create queue: %d\n", err);
        return 1;
    }
    printf("✅ Command queue created\n");
    
    // Create program
    program = clCreateProgramWithSource(context, 1, &kernel_source, NULL, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Failed to create program: %d\n", err);
        return 1;
    }
    printf("✅ Program created\n");
    
    // Build program
    printf("Building program...\n");
    err = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Failed to build program: %d\n", err);
        
        size_t log_size;
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        char* log = malloc(log_size);
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
        fprintf(stderr, "Build log:\n%s\n", log);
        free(log);
        return 1;
    }
    printf("✅ Program built\n");
    
    // Create kernel
    kernel = clCreateKernel(program, "write_magic", &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Failed to create kernel: %d\n", err);
        return 1;
    }
    printf("✅ Kernel created\n");
    
    // Create output buffer
    output_buffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(uint32_t) * 1024, NULL, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Failed to create buffer: %d\n", err);
        return 1;
    }
    printf("✅ Output buffer created\n");
    
    // Set kernel argument
    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &output_buffer);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Failed to set kernel arg: %d\n", err);
        return 1;
    }
    printf("✅ Kernel argument set\n");
    
    // Execute kernel
    printf("\n🔥 EXECUTING KERNEL (ioctl() will be captured) 🔥\n\n");
    size_t global_work_size = 1;
    err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_work_size, NULL, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Failed to enqueue kernel: %d\n", err);
        return 1;
    }
    printf("✅ Kernel enqueued\n");
    
    // Wait for completion
    err = clFinish(queue);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Failed to finish: %d\n", err);
        return 1;
    }
    printf("✅ Kernel completed\n");
    
    // Read result
    uint32_t result[1024] = {0};
    err = clEnqueueReadBuffer(queue, output_buffer, CL_TRUE, 0, sizeof(uint32_t) * 1024, result, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Failed to read buffer: %d\n", err);
        return 1;
    }
    printf("✅ Result read\n");
    
    // Validate
    printf("\nValidation:\n");
    printf("  result[0] = 0x%08X\n", result[0]);
    
    if (result[0] == 0x12345678) {
        printf("✅✅✅ SUCCESS: GPU wrote 0x12345678 ✅✅✅\n");
    } else {
        printf("❌ FAILED: Expected 0x12345678, got 0x%08X\n", result[0]);
    }
    
    // Cleanup
    clReleaseMemObject(output_buffer);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    
    printf("\n=== Check /tmp/opencl_ioctl_intercept.log for captured ioctl() ===\n");
    
    return 0;
}

// Made with Bob
