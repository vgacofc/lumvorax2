// Test C541 - Comparaison directe avec OpenCL pour identifier différences
#define CL_TARGET_OPENCL_VERSION 120
#include <CL/cl.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

const char *kernel_source = 
"__kernel void test_write(__global uint *output) {\n"
"    output[0] = 0x12345678;\n"
"}\n";

int main() {
    printf("=== TEST C541 - COMPARAISON OPENCL ===\n\n");
    
    // Setup OpenCL
    cl_platform_id platform;
    clGetPlatformIDs(1, &platform, NULL);
    
    cl_device_id device;
    clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    
    cl_context context = clCreateContext(NULL, 1, &device, NULL, NULL, NULL);
    cl_command_queue queue = clCreateCommandQueue(context, device, 0, NULL);
    
    // Créer buffer output
    cl_mem output_buf = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 4096, NULL, NULL);
    
    // Initialiser à 0xFFFFFFFF
    uint32_t init_data[1024];
    memset(init_data, 0xFF, 4096);
    clEnqueueWriteBuffer(queue, output_buf, CL_TRUE, 0, 4096, init_data, 0, NULL, NULL);
    
    // Compiler kernel
    cl_program program = clCreateProgramWithSource(context, 1, &kernel_source, NULL, NULL);
    clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    cl_kernel kernel = clCreateKernel(program, "test_write", NULL);
    
    // Set argument
    clSetKernelArg(kernel, 0, sizeof(cl_mem), &output_buf);
    
    // Exécuter
    size_t global_size = 1;
    printf("Exécution kernel OpenCL...\n");
    clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_size, NULL, 0, NULL, NULL);
    clFinish(queue);
    
    // Lire résultat
    uint32_t result[2];
    clEnqueueReadBuffer(queue, output_buf, CL_TRUE, 0, 8, result, 0, NULL, NULL);
    
    printf("\n=== RÉSULTAT OPENCL ===\n");
    printf("output[0] = 0x%08x\n", result[0]);
    printf("output[1] = 0x%08x\n", result[1]);
    
    if (result[0] == 0x12345678) {
        printf("\n✓✓✓ OpenCL fonctionne correctement!\n");
    } else {
        printf("\n❌ OpenCL a échoué!\n");
    }
    
    // Cleanup
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseMemObject(output_buf);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    
    return (result[0] == 0x12345678) ? 0 : 1;
}

// Made with Bob
