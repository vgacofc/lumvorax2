#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_program program;
    cl_int err;
    
    // Get platform
    clGetPlatformIDs(1, &platform, NULL);
    
    // Get device
    clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    
    // Create context
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    
    // Read kernel source
    FILE *f = fopen("tests/test_c580_minimal_store.cl", "r");
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *source = malloc(size + 1);
    fread(source, 1, size, f);
    source[size] = 0;
    fclose(f);
    
    // Create program
    program = clCreateProgramWithSource(context, 1, (const char**)&source, &size, &err);
    
    // Build program
    err = clBuildProgram(program, 1, &device, "-cl-std=CL2.0", NULL, NULL);
    if (err != CL_SUCCESS) {
        size_t log_size;
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        char *log = malloc(log_size);
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
        printf("Build error:\n%s\n", log);
        return 1;
    }
    
    // Get binary
    size_t binary_size;
    clGetProgramInfo(program, CL_PROGRAM_BINARY_SIZES, sizeof(size_t), &binary_size, NULL);
    unsigned char *binary = malloc(binary_size);
    clGetProgramInfo(program, CL_PROGRAM_BINARIES, sizeof(unsigned char*), &binary, NULL);
    
    // Save binary
    FILE *out = fopen("tests/test_c580_minimal_store.bin", "wb");
    fwrite(binary, 1, binary_size, out);
    fclose(out);
    
    printf("✅ Kernel compiled: %zu bytes\n", binary_size);
    printf("✅ Saved to: tests/test_c580_minimal_store.bin\n");
    
    return 0;
}
