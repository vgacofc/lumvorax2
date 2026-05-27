/*
 * Programme OpenCL simple pour capturer un batch via LD_PRELOAD
 * Utilisé avec c197_35_replay_opencl_batch_exact.so
 */

#include <stdio.h>
#include <stdlib.h>
#include <CL/cl.h>

const char* kernel_source = 
"__kernel void test_write(__global uint* output) {\n"
"    int gid = get_global_id(0);\n"
"    output[gid] = 0x12345678;\n"
"}\n";

int main() {
    printf("=== Test OpenCL pour capture batch ===\n");
    
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;
    cl_program program;
    cl_kernel kernel;
    cl_mem buffer;
    cl_int err;
    
    // Platform
    err = clGetPlatformIDs(1, &platform, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "clGetPlatformIDs failed: %d\n", err);
        return 1;
    }
    
    // Device
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "clGetDeviceIDs failed: %d\n", err);
        return 1;
    }
    
    // Context
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "clCreateContext failed: %d\n", err);
        return 1;
    }
    
    // Queue
    queue = clCreateCommandQueue(context, device, 0, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "clCreateCommandQueue failed: %d\n", err);
        return 1;
    }
    
    // Program
    program = clCreateProgramWithSource(context, 1, &kernel_source, NULL, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "clCreateProgramWithSource failed: %d\n", err);
        return 1;
    }
    
    err = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "clBuildProgram failed: %d\n", err);
        return 1;
    }
    
    // Kernel
    kernel = clCreateKernel(program, "test_write", &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "clCreateKernel failed: %d\n", err);
        return 1;
    }
    
    // Buffer
    buffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 256 * sizeof(unsigned int), NULL, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "clCreateBuffer failed: %d\n", err);
        return 1;
    }
    
    // Set args
    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &buffer);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "clSetKernelArg failed: %d\n", err);
        return 1;
    }
    
    // Execute
    size_t global_size = 256;
    printf("Executing kernel (this will trigger batch capture)...\n");
    err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_size, NULL, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "clEnqueueNDRangeKernel failed: %d\n", err);
        return 1;
    }
    
    // Finish
    clFinish(queue);
    
    printf("✓ Kernel executed successfully\n");
    printf("✓ Batch should be captured in /tmp/c197_35_captured_batch.bin\n");
    
    // Cleanup
    clReleaseMemObject(buffer);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    
    return 0;
}

// Made with Bob
