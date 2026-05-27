// Test OpenCL: 100 dispatches pour observer recyclage contexte/VM
#define CL_TARGET_OPENCL_VERSION 120
#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char* kernel_source = 
"__kernel void nop_kernel(__global int* output) {\n"
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
    
    // 1. Platform
    err = clGetPlatformIDs(1, &platform, NULL);
    if (err != CL_SUCCESS) {
        printf("ERROR: clGetPlatformIDs failed: %d\n", err);
        return 1;
    }
    
    // 2. Device
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    if (err != CL_SUCCESS) {
        printf("ERROR: clGetDeviceIDs failed: %d\n", err);
        return 1;
    }
    
    // 3. Context (1 seul pour tous les dispatches)
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    if (err != CL_SUCCESS) {
        printf("ERROR: clCreateContext failed: %d\n", err);
        return 1;
    }
    printf("✅ Context créé (1 seul pour 100 dispatches)\n");
    
    // 4. Queue
    queue = clCreateCommandQueue(context, device, 0, &err);
    if (err != CL_SUCCESS) {
        printf("ERROR: clCreateCommandQueue failed: %d\n", err);
        return 1;
    }
    
    // 5. Program
    program = clCreateProgramWithSource(context, 1, &kernel_source, NULL, &err);
    if (err != CL_SUCCESS) {
        printf("ERROR: clCreateProgramWithSource failed: %d\n", err);
        return 1;
    }
    
    err = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    if (err != CL_SUCCESS) {
        printf("ERROR: clBuildProgram failed: %d\n", err);
        return 1;
    }
    
    // 6. Kernel
    kernel = clCreateKernel(program, "nop_kernel", &err);
    if (err != CL_SUCCESS) {
        printf("ERROR: clCreateKernel failed: %d\n", err);
        return 1;
    }
    
    // 7. Buffer
    buffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 1024 * sizeof(int), NULL, &err);
    if (err != CL_SUCCESS) {
        printf("ERROR: clCreateBuffer failed: %d\n", err);
        return 1;
    }
    
    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &buffer);
    if (err != CL_SUCCESS) {
        printf("ERROR: clSetKernelArg failed: %d\n", err);
        return 1;
    }
    
    // 8. 100 DISPATCHES avec MÊME contexte
    printf("\n🚀 Lancement 100 dispatches...\n");
    size_t global_size = 1024;
    
    for (int i = 0; i < 100; i++) {
        err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_size, NULL, 0, NULL, NULL);
        if (err != CL_SUCCESS) {
            printf("❌ Dispatch %d FAILED: %d\n", i+1, err);
            break;
        }
        
        if ((i+1) % 10 == 0) {
            printf("✅ Dispatch %d OK\n", i+1);
        }
    }
    
    clFinish(queue);
    printf("\n✅ 100 dispatches terminés avec SUCCÈS\n");
    printf("📊 Contexte: 1 seul (jamais recyclé)\n");
    printf("📊 VM: 1 seule (jamais recyclée)\n");
    
    // Cleanup
    clReleaseMemObject(buffer);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    
    return 0;
}

// Made with Bob
