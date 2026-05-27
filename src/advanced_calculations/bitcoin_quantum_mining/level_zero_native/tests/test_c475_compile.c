#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    // Lire le kernel source
    FILE* f = fopen("tests/test_c475_minimal_compute.cl", "r");
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* source = malloc(size + 1);
    fread(source, 1, size, f);
    source[size] = 0;
    fclose(f);
    
    // Setup OpenCL
    cl_platform_id platform;
    clGetPlatformIDs(1, &platform, NULL);
    cl_device_id device;
    clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    cl_context ctx = clCreateContext(NULL, 1, &device, NULL, NULL, NULL);
    
    // Compiler le kernel
    cl_program prog = clCreateProgramWithSource(ctx, 1, (const char**)&source, &size, NULL);
    cl_int err = clBuildProgram(prog, 1, &device, "-save-temps=/tmp/c475", NULL, NULL);
    
    if (err != CL_SUCCESS) {
        size_t log_size;
        clGetProgramBuildInfo(prog, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        char* log = malloc(log_size);
        clGetProgramBuildInfo(prog, device, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
        printf("Build error:\n%s\n", log);
        return 1;
    }
    
    // Extraire le binaire
    size_t bin_size;
    clGetProgramInfo(prog, CL_PROGRAM_BINARY_SIZES, sizeof(size_t), &bin_size, NULL);
    unsigned char* binary = malloc(bin_size);
    clGetProgramInfo(prog, CL_PROGRAM_BINARIES, sizeof(unsigned char*), &binary, NULL);
    
    // Sauvegarder
    FILE* out = fopen("tests/test_c475_minimal_compute.bin", "wb");
    fwrite(binary, 1, bin_size, out);
    fclose(out);
    
    printf("✅ Kernel compilé: %zu bytes\n", bin_size);
    printf("✅ Binaire sauvegardé: tests/test_c475_minimal_compute.bin\n");
    
    free(binary);
    free(source);
    clReleaseProgram(prog);
    clReleaseContext(ctx);
    return 0;
}
