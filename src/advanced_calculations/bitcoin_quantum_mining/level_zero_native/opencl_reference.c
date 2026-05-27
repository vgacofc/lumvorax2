/*
 * Programme OpenCL minimal pour capturer batch GPU réel
 * Objectif : Reverse engineer états GPU manquants
 */

#include <stdio.h>
#include <stdlib.h>
#include <CL/cl.h>

const char *kernel_source = 
"__kernel void nop_kernel(__global int *output) {\n"
"    int gid = get_global_id(0);\n"
"    output[gid] = gid;\n"
"}\n";

int main() {
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;
    cl_program program;
    cl_kernel kernel;
    cl_mem buffer;
    cl_int err;
    
    // Get platform
    err = clGetPlatformIDs(1, &platform, NULL);
    if (err != CL_SUCCESS) {
        printf("Error getting platform: %d\n", err);
        return 1;
    }
    
    // Get device
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    if (err != CL_SUCCESS) {
        printf("Error getting device: %d\n", err);
        return 1;
    }
    
    // Create context
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    if (err != CL_SUCCESS) {
        printf("Error creating context: %d\n", err);
        return 1;
    }
    
    // Create queue
    queue = clCreateCommandQueue(context, device, 0, &err);
    if (err != CL_SUCCESS) {
        printf("Error creating queue: %d\n", err);
        return 1;
    }
    
    // Create program
    program = clCreateProgramWithSource(context, 1, &kernel_source, NULL, &err);
    if (err != CL_SUCCESS) {
        printf("Error creating program: %d\n", err);
        return 1;
    }
    
    // Build program
    err = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    if (err != CL_SUCCESS) {
        printf("Error building program: %d\n", err);
        return 1;
    }
    
    // Create kernel
    kernel = clCreateKernel(program, "nop_kernel", &err);
    if (err != CL_SUCCESS) {
        printf("Error creating kernel: %d\n", err);
        return 1;
    }
    
    // Create buffer
    buffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 256 * sizeof(int), NULL, &err);
    if (err != CL_SUCCESS) {
        printf("Error creating buffer: %d\n", err);
        return 1;
    }
    
    // Set kernel arg
    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &buffer);
    if (err != CL_SUCCESS) {
        printf("Error setting kernel arg: %d\n", err);
        return 1;
    }
    
    // Execute kernel
    size_t global_size = 256;
    printf("Executing OpenCL kernel...\n");
    err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_size, NULL, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        printf("Error executing kernel: %d\n", err);
        return 1;
    }
    
    // Wait
    clFinish(queue);
    printf("OpenCL kernel executed successfully!\n");
    
    // Cleanup
    clReleaseMemObject(buffer);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    
    return 0;
}

// Made with Bob
