/**
 * @file c198_compile_minimal_opencl.c
 * @brief Compilation kernel minimal avec OpenCL → Gen9 ISA
 * 
 * OBJECTIF : Compiler kernel test_minimal en Gen9 ISA pour test Level Zero
 * 
 * AUTEUR : Bob (LumVorax C198 Phase 10C)
 * DATE : 2026-05-09
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <CL/cl.h>

const char* KERNEL_SOURCE = 
"__kernel void test_minimal(__global float* output) {\n"
"    int gid = get_global_id(0);\n"
"    output[gid] = 42.0f;\n"
"}\n";

int main(void) {
    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║  C198 COMPILATION KERNEL MINIMAL — OpenCL → Gen9 ISA          ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n\n");

    cl_int err;
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_program program;

    /* ========================================================================
     * ÉTAPE 1 : Initialisation OpenCL
     * ======================================================================== */

    printf("=== ÉTAPE 1 : Initialisation OpenCL ===\n");

    err = clGetPlatformIDs(1, &platform, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[CL] ERREUR : clGetPlatformIDs = %d\n", err);
        return 1;
    }

    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[CL] ERREUR : clGetDeviceIDs = %d\n", err);
        return 1;
    }

    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[CL] ERREUR : clCreateContext = %d\n", err);
        return 1;
    }

    printf("[CL] ✅ OpenCL initialisé\n");

    /* ========================================================================
     * ÉTAPE 2 : Compilation kernel
     * ======================================================================== */

    printf("\n=== ÉTAPE 2 : Compilation kernel ===\n");

    size_t source_len = strlen(KERNEL_SOURCE);
    program = clCreateProgramWithSource(context, 1, &KERNEL_SOURCE, &source_len, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[CL] ERREUR : clCreateProgramWithSource = %d\n", err);
        return 1;
    }

    err = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[CL] ERREUR : clBuildProgram = %d\n", err);
        
        size_t log_size;
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        char* log = malloc(log_size);
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
        fprintf(stderr, "[CL] Build log :\n%s\n", log);
        free(log);
        return 1;
    }

    printf("[CL] ✅ Kernel compilé\n");

    /* ========================================================================
     * ÉTAPE 3 : Extraction binaire Gen9 ISA
     * ======================================================================== */

    printf("\n=== ÉTAPE 3 : Extraction binaire Gen9 ISA ===\n");

    size_t binary_size;
    err = clGetProgramInfo(program, CL_PROGRAM_BINARY_SIZES, sizeof(size_t), &binary_size, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[CL] ERREUR : clGetProgramInfo(BINARY_SIZES) = %d\n", err);
        return 1;
    }

    printf("[CL] Taille binaire : %zu bytes\n", binary_size);

    unsigned char* binary = malloc(binary_size);
    unsigned char* binaries[1] = { binary };

    err = clGetProgramInfo(program, CL_PROGRAM_BINARIES, sizeof(unsigned char*), binaries, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[CL] ERREUR : clGetProgramInfo(BINARIES) = %d\n", err);
        return 1;
    }

    printf("[CL] ✅ Binaire extrait\n");

    /* ========================================================================
     * ÉTAPE 4 : Sauvegarde binaire
     * ======================================================================== */

    printf("\n=== ÉTAPE 4 : Sauvegarde binaire ===\n");

    const char* output_path = "opencl_test_minimal_gen9.bin";
    FILE* f = fopen(output_path, "wb");
    if (!f) {
        fprintf(stderr, "[CL] ERREUR : Impossible de créer '%s'\n", output_path);
        return 1;
    }

    fwrite(binary, 1, binary_size, f);
    fclose(f);

    printf("[CL] ✅ Binaire sauvegardé : %s (%zu bytes)\n", output_path, binary_size);

    /* ========================================================================
     * ÉTAPE 5 : Cleanup
     * ======================================================================== */

    printf("\n=== ÉTAPE 5 : Cleanup ===\n");

    free(binary);
    clReleaseProgram(program);
    clReleaseContext(context);

    printf("[CL] ✅ Cleanup terminé\n");

    printf("\n╔════════════════════════════════════════════════════════════════╗\n");
    printf("║  COMPILATION TERMINÉE — Binaire prêt pour Level Zero          ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n");

    return 0;
}

// Made with Bob
