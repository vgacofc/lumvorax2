/*
 * LumVorax C198 — Compilateur OpenCL → Binaire Gen9 ISA
 * Utilise OpenCL runtime pour générer binaire natif Gen9
 */
#define CL_TARGET_OPENCL_VERSION 120
#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <input.cl> <output.bin>\n", argv[0]);
        return 1;
    }
    
    const char* input_file = argv[1];
    const char* output_file = argv[2];
    
    printf("[C198-COMPILER] Compilation OpenCL → Gen9 ISA\n");
    printf("[C198-COMPILER] Input: %s\n", input_file);
    printf("[C198-COMPILER] Output: %s\n", output_file);
    
    // 1. Lecture source OpenCL
    FILE* f = fopen(input_file, "rb");
    if (!f) {
        fprintf(stderr, "❌ Erreur lecture %s\n", input_file);
        return 1;
    }
    fseek(f, 0, SEEK_END);
    size_t src_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* src = malloc(src_size + 1);
    fread(src, 1, src_size, f);
    src[src_size] = '\0';
    fclose(f);
    printf("[C198-COMPILER] Source lu: %zu bytes\n", src_size);
    
    // 2. Initialisation OpenCL
    cl_platform_id platform;
    cl_device_id device;
    cl_int err;
    
    err = clGetPlatformIDs(1, &platform, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "❌ clGetPlatformIDs: %d\n", err);
        return 1;
    }
    
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "❌ clGetDeviceIDs: %d\n", err);
        return 1;
    }
    
    char device_name[256];
    clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(device_name), device_name, NULL);
    printf("[C198-COMPILER] Device: %s\n", device_name);
    
    // 3. Création contexte + programme
    cl_context ctx = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "❌ clCreateContext: %d\n", err);
        return 1;
    }
    
    cl_program program = clCreateProgramWithSource(ctx, 1, (const char**)&src, &src_size, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "❌ clCreateProgramWithSource: %d\n", err);
        return 1;
    }
    
    // 4. Compilation avec options Gen9
    const char* options = "-cl-std=CL1.2 -cl-kernel-arg-info";
    err = clBuildProgram(program, 1, &device, options, NULL, NULL);
    
    // Affichage build log
    size_t log_size;
    clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
    if (log_size > 2) {
        char* log = malloc(log_size);
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
        printf("[C198-COMPILER] Build log:\n%s\n", log);
        free(log);
    }
    
    if (err != CL_SUCCESS) {
        fprintf(stderr, "❌ clBuildProgram: %d\n", err);
        return 1;
    }
    
    printf("[C198-COMPILER] ✅ Compilation réussie\n");
    
    // 5. Extraction binaire Gen9
    size_t binary_size;
    err = clGetProgramInfo(program, CL_PROGRAM_BINARY_SIZES, sizeof(size_t), &binary_size, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "❌ clGetProgramInfo(BINARY_SIZES): %d\n", err);
        return 1;
    }
    
    printf("[C198-COMPILER] Binaire Gen9: %zu bytes\n", binary_size);
    
    unsigned char* binary = malloc(binary_size);
    unsigned char* binaries[1] = { binary };
    err = clGetProgramInfo(program, CL_PROGRAM_BINARIES, sizeof(unsigned char*), binaries, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "❌ clGetProgramInfo(BINARIES): %d\n", err);
        return 1;
    }
    
    // 6. Sauvegarde binaire
    FILE* out = fopen(output_file, "wb");
    if (!out) {
        fprintf(stderr, "❌ Erreur écriture %s\n", output_file);
        return 1;
    }
    fwrite(binary, 1, binary_size, out);
    fclose(out);
    
    printf("[C198-COMPILER] ✅ Binaire Gen9 sauvegardé: %s (%zu bytes)\n", output_file, binary_size);
    
    // Cleanup
    free(binary);
    free(src);
    clReleaseProgram(program);
    clReleaseContext(ctx);
    
    return 0;
}
