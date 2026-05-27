/*
 * EXTRACTEUR ISA KERNEL OPENCL
 * Compile test_c427_store_validation.cl et extrait l'ISA Gen9
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <CL/cl.h>

int main() {
    cl_int err;
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_program program;
    
    printf("=== EXTRACTION ISA KERNEL OPENCL ===\n\n");
    
    // 1. Get platform
    err = clGetPlatformIDs(1, &platform, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "❌ clGetPlatformIDs: %d\n", err);
        return 1;
    }
    
    // 2. Get GPU device
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "❌ clGetDeviceIDs: %d\n", err);
        return 1;
    }
    
    char device_name[128];
    clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(device_name), device_name, NULL);
    printf("✓ Device: %s\n", device_name);
    
    // 3. Create context
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "❌ clCreateContext: %d\n", err);
        return 1;
    }
    
    // 4. Load kernel source
    FILE* f = fopen("tests/test_c427_store_validation.cl", "r");
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
    
    printf("✓ Kernel source chargé: %zu bytes\n", source_size);
    
    // 5. Create program
    program = clCreateProgramWithSource(context, 1, (const char**)&source, &source_size, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "❌ clCreateProgramWithSource: %d\n", err);
        return 1;
    }
    
    // 6. Build program
    err = clBuildProgram(program, 1, &device, "-cl-std=CL2.0", NULL, NULL);
    if (err != CL_SUCCESS) {
        char log[4096];
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, sizeof(log), log, NULL);
        fprintf(stderr, "❌ Build failed:\n%s\n", log);
        return 1;
    }
    printf("✓ Kernel compilé\n");
    
    // 7. Get binary size
    size_t binary_size;
    err = clGetProgramInfo(program, CL_PROGRAM_BINARY_SIZES, sizeof(size_t), &binary_size, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "❌ clGetProgramInfo BINARY_SIZES: %d\n", err);
        return 1;
    }
    
    printf("✓ Binary size: %zu bytes\n", binary_size);
    
    // 8. Get binary
    unsigned char* binary = malloc(binary_size);
    unsigned char* binaries[1] = { binary };
    
    err = clGetProgramInfo(program, CL_PROGRAM_BINARIES, sizeof(unsigned char*), binaries, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "❌ clGetProgramInfo BINARIES: %d\n", err);
        return 1;
    }
    
    printf("✓ Binary extrait\n");
    
    // 9. Write to header file
    FILE* out = fopen("tests/test_c505_extracted_kernel.h", "w");
    if (!out) {
        fprintf(stderr, "❌ Cannot create output file\n");
        return 1;
    }
    
    fprintf(out, "// ISA Gen9 extrait depuis OpenCL - test_c427_store_validation.cl\n");
    fprintf(out, "// Date: May 24 2026\n");
    fprintf(out, "// Kernel: output[0] = 0x12345678\n");
    fprintf(out, "const unsigned char kernel_isa_c505[] = {\n");
    
    for (size_t i = 0; i < binary_size; i++) {
        if (i % 16 == 0) fprintf(out, "    ");
        fprintf(out, "0x%02x", binary[i]);
        if (i < binary_size - 1) fprintf(out, ", ");
        if (i % 16 == 15) fprintf(out, "\n");
    }
    
    if (binary_size % 16 != 0) fprintf(out, "\n");
    fprintf(out, "};\n");
    fprintf(out, "const size_t kernel_isa_c505_size = %zu;\n", binary_size);
    
    fclose(out);
    
    printf("✓ Header file créé: tests/test_c505_extracted_kernel.h\n");
    printf("\n🎉 EXTRACTION RÉUSSIE!\n");
    printf("   Kernel ISA: %zu bytes\n", binary_size);
    printf("   Fichier: test_c505_extracted_kernel.h\n");
    
    // Cleanup
    free(binary);
    free(source);
    clReleaseProgram(program);
    clReleaseContext(context);
    
    return 0;
}

// Made with Bob