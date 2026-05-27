#define CL_TARGET_OPENCL_VERSION 120
#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;
    cl_program program;
    cl_kernel kernel;
    cl_mem output_buf;
    cl_int err;
    
    // Get platform/device
    clGetPlatformIDs(1, &platform, NULL);
    clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    
    // Create context/queue
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    queue = clCreateCommandQueue(context, device, 0, &err);
    
    // Load kernel source
    FILE *f = fopen("tests/test_c601_opencl_reference.cl", "r");
    fseek(f, 0, SEEK_END);
    size_t src_size = ftell(f);
    rewind(f);
    char *src = malloc(src_size + 1);
    fread(src, 1, src_size, f);
    src[src_size] = 0;
    fclose(f);
    
    // Compile
    program = clCreateProgramWithSource(context, 1, (const char**)&src, &src_size, &err);
    err = clBuildProgram(program, 1, &device, "", NULL, NULL);
    
    if (err != CL_SUCCESS) {
        size_t log_size;
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        char *log = malloc(log_size);
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
        printf("Build error:\n%s\n", log);
        return 1;
    }
    
    // Get binary
    size_t bin_size;
    clGetProgramInfo(program, CL_PROGRAM_BINARY_SIZES, sizeof(size_t), &bin_size, NULL);
    unsigned char *binary = malloc(bin_size);
    clGetProgramInfo(program, CL_PROGRAM_BINARIES, sizeof(unsigned char*), &binary, NULL);
    
    // Save binary
    FILE *out = fopen("tests/test_c601_opencl_kernel.bin", "wb");
    fwrite(binary, 1, bin_size, out);
    fclose(out);
    
    printf("✓ Kernel OpenCL compilé: %zu bytes\n", bin_size);
    printf("✓ Binaire sauvegardé: tests/test_c601_opencl_kernel.bin\n");
    
    // Create kernel
    kernel = clCreateKernel(program, "test_write", &err);
    
    // Create output buffer
    output_buf = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 4096, NULL, &err);
    
    // Set args
    clSetKernelArg(kernel, 0, sizeof(cl_mem), &output_buf);
    
    // Execute
    size_t global_size = 1;
    clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_size, NULL, 0, NULL, NULL);
    clFinish(queue);
    
    // Read result
    uint32_t result[16];
    clEnqueueReadBuffer(queue, output_buf, CL_TRUE, 0, 64, result, 0, NULL, NULL);
    
    printf("\n✓ Résultat OpenCL:\n");
    printf("  output[0] = 0x%08x %s\n", result[0], 
           result[0] == 0x12345678 ? "✅ CORRECT" : "❌ INCORRECT");
    
    // Cleanup
    clReleaseMemObject(output_buf);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    free(src);
    free(binary);
    
    return result[0] == 0x12345678 ? 0 : 1;
}
