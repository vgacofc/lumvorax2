/**
 * LumVorax C198 Phase 15Y-F20 — Programme OpenCL Simple pour Interception
 * 
 * Programme minimal OpenCL qui exécute un kernel vide
 * pour capturer les batch buffers générés par le driver Intel NEO
 */

#include <stdio.h>
#include <stdlib.h>
#include <CL/cl.h>

const char *kernel_source = 
"__kernel void dummy_kernel(__global int *data) {\n"
"    int gid = get_global_id(0);\n"
"    data[gid] = gid * 2;\n"
"}\n";

int main(void) {
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;
    cl_program program;
    cl_kernel kernel;
    cl_mem buffer;
    cl_int err;
    
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  LumVorax C198 — OpenCL Simple Test                      ║\n");
    printf("║  Exécution 10 dispatches pour capture batch buffers      ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");
    
    /* Get platform */
    err = clGetPlatformIDs(1, &platform, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Error: clGetPlatformIDs failed\n");
        return 1;
    }
    
    /* Get device */
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Error: clGetDeviceIDs failed, trying CPU\n");
        err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 1, &device, NULL);
        if (err != CL_SUCCESS) {
            fprintf(stderr, "Error: No OpenCL device found\n");
            return 1;
        }
    }
    
    char device_name[256];
    clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(device_name), device_name, NULL);
    printf("Device: %s\n\n", device_name);
    
    /* Create context */
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Error: clCreateContext failed\n");
        return 1;
    }
    
    /* Create command queue */
    queue = clCreateCommandQueue(context, device, 0, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Error: clCreateCommandQueue failed\n");
        return 1;
    }
    
    /* Create program */
    program = clCreateProgramWithSource(context, 1, &kernel_source, NULL, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Error: clCreateProgramWithSource failed\n");
        return 1;
    }
    
    /* Build program */
    err = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Error: clBuildProgram failed\n");
        char build_log[4096];
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG,
                              sizeof(build_log), build_log, NULL);
        fprintf(stderr, "Build log:\n%s\n", build_log);
        return 1;
    }
    
    /* Create kernel */
    kernel = clCreateKernel(program, "dummy_kernel", &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Error: clCreateKernel failed\n");
        return 1;
    }
    
    /* Create buffer */
    buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, 1024 * sizeof(int), NULL, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Error: clCreateBuffer failed\n");
        return 1;
    }
    
    /* Set kernel argument */
    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &buffer);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Error: clSetKernelArg failed\n");
        return 1;
    }
    
    /* Execute kernel 10 times */
    size_t global_size = 1024;
    size_t local_size = 64;
    
    printf("Executing 10 kernel dispatches...\n\n");
    
    for (int i = 0; i < 10; i++) {
        err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_size, &local_size, 0, NULL, NULL);
        if (err != CL_SUCCESS) {
            fprintf(stderr, "Error: clEnqueueNDRangeKernel #%d failed (err=%d)\n", i+1, err);
            return 1;
        }
        printf("  Dispatch #%d: OK\n", i+1);
    }
    
    /* Wait for completion */
    clFinish(queue);
    
    printf("\n✅ All 10 dispatches completed successfully\n");
    printf("Check opencl_batch_dump.txt for captured batch buffers\n\n");
    
    /* Cleanup */
    clReleaseMemObject(buffer);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    
    return 0;
}

// Made with Bob
