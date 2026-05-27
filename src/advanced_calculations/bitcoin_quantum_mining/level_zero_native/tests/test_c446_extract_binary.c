/*
 * TEST C446 - EXTRACTION BINAIRE ISA
 * Objectif: Extraire le binaire Gen9 ISA du kernel OpenCL compilé
 */

#include <stdio.h>
#include <stdlib.h>
#include <CL/cl.h>

#define CHECK_CL(err, msg) if(err != CL_SUCCESS) { \
    fprintf(stderr, "❌ %s: error %d\n", msg, err); \
    exit(1); \
}

int main() {
    cl_int err;
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_program program;
    
    printf("🚀 Extraction binaire ISA kernel write_test\n");
    
    err = clGetPlatformIDs(1, &platform, NULL);
    CHECK_CL(err, "clGetPlatformIDs");
    
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    CHECK_CL(err, "clGetDeviceIDs");
    
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    CHECK_CL(err, "clCreateContext");
    
    FILE* f = fopen("tests/test_c446_write_only.cl", "r");
    if (!f) {
        fprintf(stderr, "❌ Cannot open kernel file\n");
        return 1;
    }
    fseek(f, 0, SEEK_END);
    size_t source_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* source = malloc(source_size + 1);
    fread(source, 1, source_size, f);
    source[source_size] = '\0';
    fclose(f);
    
    program = clCreateProgramWithSource(context, 1, (const char**)&source, &source_size, &err);
    CHECK_CL(err, "clCreateProgramWithSource");
    
    err = clBuildProgram(program, 1, &device, "-cl-std=CL2.0", NULL, NULL);
    if (err != CL_SUCCESS) {
        char log[4096];
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, sizeof(log), log, NULL);
        fprintf(stderr, "❌ Build failed:\n%s\n", log);
        return 1;
    }
    printf("✅ Kernel compilé\n");
    
    // Get binary size
    size_t binary_size;
    err = clGetProgramInfo(program, CL_PROGRAM_BINARY_SIZES, sizeof(size_t), &binary_size, NULL);
    CHECK_CL(err, "clGetProgramInfo BINARY_SIZES");
    
    printf("✅ Binary size: %zu bytes\n", binary_size);
    
    // Get binary
    unsigned char* binary = malloc(binary_size);
    unsigned char* binaries[1] = { binary };
    err = clGetProgramInfo(program, CL_PROGRAM_BINARIES, sizeof(unsigned char*), binaries, NULL);
    CHECK_CL(err, "clGetProgramInfo BINARIES");
    
    // Save binary
    FILE* out = fopen("tests/test_c446_write_only.bin", "wb");
    if (!out) {
        fprintf(stderr, "❌ Cannot create output file\n");
        return 1;
    }
    fwrite(binary, 1, binary_size, out);
    fclose(out);
    
    printf("✅ Binary saved to: tests/test_c446_write_only.bin\n");
    
    free(binary);
    free(source);
    clReleaseProgram(program);
    clReleaseContext(context);
    
    return 0;
}

// Made with Bob
