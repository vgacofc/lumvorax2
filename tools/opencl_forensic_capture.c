/**
 * @file opencl_forensic_capture.c
 * @brief Capture forensique COMPLÈTE d'une exécution OpenCL réussie
 * 
 * OBJECTIF : Instrumenter OpenCL avec traçabilité bit-level pour capturer
 *            EXACTEMENT ce qui se passe quand le kernel fonctionne, puis
 *            reproduire la même chose dans Level Zero.
 * 
 * CAPTURE :
 * - Binaire kernel EXACT (ISA Gen9)
 * - Arguments kernel (valeurs, tailles, alignement)
 * - Configuration dispatch (work-groups, threads)
 * - Données GPU (avant/après exécution)
 * - Timings nanoseconde
 * - Tous les appels OpenCL
 * 
 * AUTEUR : Bob (LumVorax C198 Phase 10B)
 * DATE : 2026-05-09
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

/* ============================================================================
 * KERNEL TEST
 * ============================================================================ */

static const char* KERNEL_SOURCE = 
"__kernel void vector_add(\n"
"    __global const float* a,\n"
"    __global const float* b,\n"
"    __global float* c,\n"
"    const uint n)\n"
"{\n"
"    uint gid = get_global_id(0);\n"
"    if (gid < n) {\n"
"        c[gid] = a[gid] + b[gid];\n"
"    }\n"
"}\n";

/* ============================================================================
 * UTILITAIRES
 * ============================================================================ */

static uint64_t get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

static void dump_hex(const char* label, const void* data, size_t size) {
    const uint8_t* bytes = (const uint8_t*)data;
    printf("[FORENSIC] %s (%zu bytes) :\n", label, size);
    for (size_t i = 0; i < size; i++) {
        if (i % 16 == 0) printf("  %04zX: ", i);
        printf("%02X ", bytes[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }
    if (size % 16 != 0) printf("\n");
}

static void save_binary(const char* filename, const void* data, size_t size) {
    FILE* f = fopen(filename, "wb");
    if (!f) {
        fprintf(stderr, "ERREUR : Impossible de créer '%s'\n", filename);
        return;
    }
    fwrite(data, 1, size, f);
    fclose(f);
    printf("[FORENSIC] Binaire sauvegardé : %s (%zu bytes)\n", filename, size);
}

/* ============================================================================
 * CAPTURE FORENSIQUE OPENCL
 * ============================================================================ */

int main(void) {
    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║  CAPTURE FORENSIQUE OPENCL — LumVorax C198 Phase 10B          ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n\n");

    cl_int err;
    uint64_t t_start, t_end;

    /* ========================================================================
     * ÉTAPE 1 : Initialisation OpenCL
     * ======================================================================== */

    printf("=== ÉTAPE 1 : Initialisation OpenCL ===\n");

    t_start = get_time_ns();

    // Platform
    cl_platform_id platform;
    err = clGetPlatformIDs(1, &platform, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "ERREUR : clGetPlatformIDs = %d\n", err);
        return 1;
    }

    char platform_name[256];
    clGetPlatformInfo(platform, CL_PLATFORM_NAME, sizeof(platform_name), platform_name, NULL);
    printf("[FORENSIC] Platform : %s\n", platform_name);

    // Device
    cl_device_id device;
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "ERREUR : clGetDeviceIDs = %d\n", err);
        return 1;
    }

    char device_name[256];
    cl_uint compute_units;
    cl_ulong global_mem_size;
    clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(device_name), device_name, NULL);
    clGetDeviceInfo(device, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(compute_units), &compute_units, NULL);
    clGetDeviceInfo(device, CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(global_mem_size), &global_mem_size, NULL);

    printf("[FORENSIC] Device : %s\n", device_name);
    printf("[FORENSIC] Compute Units : %u\n", compute_units);
    printf("[FORENSIC] Global Memory : %lu MB\n", global_mem_size / (1024 * 1024));

    // Context
    cl_context context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "ERREUR : clCreateContext = %d\n", err);
        return 1;
    }

    // Queue
    cl_command_queue queue = clCreateCommandQueue(context, device, 0, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "ERREUR : clCreateCommandQueue = %d\n", err);
        return 1;
    }

    t_end = get_time_ns();
    printf("[FORENSIC] Initialisation : %lu ns (%.3f ms)\n\n", t_end - t_start, (t_end - t_start) / 1e6);

    /* ========================================================================
     * ÉTAPE 2 : Compilation kernel
     * ======================================================================== */

    printf("=== ÉTAPE 2 : Compilation kernel ===\n");

    t_start = get_time_ns();

    size_t source_len = strlen(KERNEL_SOURCE);
    cl_program program = clCreateProgramWithSource(context, 1, &KERNEL_SOURCE, &source_len, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "ERREUR : clCreateProgramWithSource = %d\n", err);
        return 1;
    }

    const char* build_options = "-cl-std=CL1.2 -cl-fast-relaxed-math";
    printf("[FORENSIC] Options compilation : %s\n", build_options);

    err = clBuildProgram(program, 1, &device, build_options, NULL, NULL);
    if (err != CL_SUCCESS) {
        char build_log[4096];
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, sizeof(build_log), build_log, NULL);
        fprintf(stderr, "ERREUR : clBuildProgram = %d\n%s\n", err, build_log);
        return 1;
    }

    t_end = get_time_ns();
    printf("[FORENSIC] Compilation : %lu ns (%.3f ms)\n", t_end - t_start, (t_end - t_start) / 1e6);

    /* ========================================================================
     * ÉTAPE 3 : Extraction binaire Gen9 ISA
     * ======================================================================== */

    printf("\n=== ÉTAPE 3 : Extraction binaire Gen9 ISA ===\n");

    size_t binary_size;
    err = clGetProgramInfo(program, CL_PROGRAM_BINARY_SIZES, sizeof(size_t), &binary_size, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "ERREUR : clGetProgramInfo (size) = %d\n", err);
        return 1;
    }

    printf("[FORENSIC] Taille binaire : %zu bytes\n", binary_size);

    unsigned char* binary = malloc(binary_size);
    if (!binary) {
        fprintf(stderr, "ERREUR : Allocation binaire échouée\n");
        return 1;
    }

    unsigned char* binaries[1] = { binary };
    err = clGetProgramInfo(program, CL_PROGRAM_BINARIES, sizeof(unsigned char*), binaries, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "ERREUR : clGetProgramInfo (binary) = %d\n", err);
        return 1;
    }

    // Sauvegarder binaire complet
    save_binary("opencl_vector_add_gen9_FULL.bin", binary, binary_size);

    // Afficher header
    dump_hex("Header binaire (premiers 256 bytes)", binary, binary_size < 256 ? binary_size : 256);

    /* ========================================================================
     * ÉTAPE 4 : Création kernel
     * ======================================================================== */

    printf("\n=== ÉTAPE 4 : Création kernel ===\n");

    cl_kernel kernel = clCreateKernel(program, "vector_add", &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "ERREUR : clCreateKernel = %d\n", err);
        return 1;
    }

    printf("[FORENSIC] Kernel créé : vector_add\n");

    // Informations kernel
    char kernel_name[256];
    cl_uint num_args;
    clGetKernelInfo(kernel, CL_KERNEL_FUNCTION_NAME, sizeof(kernel_name), kernel_name, NULL);
    clGetKernelInfo(kernel, CL_KERNEL_NUM_ARGS, sizeof(num_args), &num_args, NULL);

    printf("[FORENSIC] Nom kernel : %s\n", kernel_name);
    printf("[FORENSIC] Nombre arguments : %u\n", num_args);

    /* ========================================================================
     * ÉTAPE 5 : Allocation buffers GPU
     * ======================================================================== */

    printf("\n=== ÉTAPE 5 : Allocation buffers GPU ===\n");

    const uint32_t n = 1024;
    const size_t buffer_size = n * sizeof(float);

    printf("[FORENSIC] Taille buffers : %zu bytes (%u éléments)\n", buffer_size, n);

    cl_mem buf_a = clCreateBuffer(context, CL_MEM_READ_ONLY, buffer_size, NULL, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "ERREUR : clCreateBuffer (a) = %d\n", err);
        return 1;
    }

    cl_mem buf_b = clCreateBuffer(context, CL_MEM_READ_ONLY, buffer_size, NULL, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "ERREUR : clCreateBuffer (b) = %d\n", err);
        return 1;
    }

    cl_mem buf_c = clCreateBuffer(context, CL_MEM_WRITE_ONLY, buffer_size, NULL, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "ERREUR : clCreateBuffer (c) = %d\n", err);
        return 1;
    }

    printf("[FORENSIC] Buffers GPU alloués : 3×%zu bytes\n", buffer_size);

    /* ========================================================================
     * ÉTAPE 6 : Préparation données
     * ======================================================================== */

    printf("\n=== ÉTAPE 6 : Préparation données ===\n");

    float* host_a = malloc(buffer_size);
    float* host_b = malloc(buffer_size);
    float* host_c = malloc(buffer_size);

    for (uint32_t i = 0; i < n; i++) {
        host_a[i] = (float)i;
        host_b[i] = (float)(i * 2);
        host_c[i] = 0.0f;
    }

    printf("[FORENSIC] Données préparées : a[i]=i, b[i]=i*2\n");
    printf("[FORENSIC] Exemples : a[0]=%.2f, a[100]=%.2f, a[1023]=%.2f\n",
           host_a[0], host_a[100], host_a[1023]);
    printf("[FORENSIC] Exemples : b[0]=%.2f, b[100]=%.2f, b[1023]=%.2f\n",
           host_b[0], host_b[100], host_b[1023]);

    // Sauvegarder données input
    save_binary("opencl_input_a.bin", host_a, buffer_size);
    save_binary("opencl_input_b.bin", host_b, buffer_size);

    /* ========================================================================
     * ÉTAPE 7 : Upload données GPU
     * ======================================================================== */

    printf("\n=== ÉTAPE 7 : Upload données GPU ===\n");

    t_start = get_time_ns();

    err = clEnqueueWriteBuffer(queue, buf_a, CL_TRUE, 0, buffer_size, host_a, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "ERREUR : clEnqueueWriteBuffer (a) = %d\n", err);
        return 1;
    }

    err = clEnqueueWriteBuffer(queue, buf_b, CL_TRUE, 0, buffer_size, host_b, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "ERREUR : clEnqueueWriteBuffer (b) = %d\n", err);
        return 1;
    }

    t_end = get_time_ns();
    double upload_time_ms = (t_end - t_start) / 1e6;
    double upload_gbps = (2.0 * buffer_size) / ((t_end - t_start) / 1e9) / 1e9;

    printf("[FORENSIC] Upload GPU : %lu ns (%.3f ms)\n", t_end - t_start, upload_time_ms);
    printf("[FORENSIC] Débit upload : %.2f GB/s\n", upload_gbps);

    /* ========================================================================
     * ÉTAPE 8 : Configuration arguments kernel
     * ======================================================================== */

    printf("\n=== ÉTAPE 8 : Configuration arguments kernel ===\n");

    printf("[FORENSIC] Argument 0 : buf_a (cl_mem, %zu bytes)\n", sizeof(cl_mem));
    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &buf_a);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "ERREUR : clSetKernelArg(0) = %d\n", err);
        return 1;
    }
    printf("[FORENSIC]   → Valeur : %p\n", (void*)buf_a);

    printf("[FORENSIC] Argument 1 : buf_b (cl_mem, %zu bytes)\n", sizeof(cl_mem));
    err = clSetKernelArg(kernel, 1, sizeof(cl_mem), &buf_b);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "ERREUR : clSetKernelArg(1) = %d\n", err);
        return 1;
    }
    printf("[FORENSIC]   → Valeur : %p\n", (void*)buf_b);

    printf("[FORENSIC] Argument 2 : buf_c (cl_mem, %zu bytes)\n", sizeof(cl_mem));
    err = clSetKernelArg(kernel, 2, sizeof(cl_mem), &buf_c);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "ERREUR : clSetKernelArg(2) = %d\n", err);
        return 1;
    }
    printf("[FORENSIC]   → Valeur : %p\n", (void*)buf_c);

    printf("[FORENSIC] Argument 3 : n (uint32_t, %zu bytes)\n", sizeof(uint32_t));
    err = clSetKernelArg(kernel, 3, sizeof(uint32_t), &n);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "ERREUR : clSetKernelArg(3) = %d\n", err);
        return 1;
    }
    printf("[FORENSIC]   → Valeur : %u\n", n);

    /* ========================================================================
     * ÉTAPE 9 : Exécution kernel GPU
     * ======================================================================== */

    printf("\n=== ÉTAPE 9 : Exécution kernel GPU ===\n");

    size_t global_work_size = n;
    size_t local_work_size = 256;

    printf("[FORENSIC] Global work size : %zu\n", global_work_size);
    printf("[FORENSIC] Local work size : %zu\n", local_work_size);
    printf("[FORENSIC] Nombre work-groups : %zu\n", global_work_size / local_work_size);

    t_start = get_time_ns();

    err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_work_size, &local_work_size, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "ERREUR : clEnqueueNDRangeKernel = %d\n", err);
        return 1;
    }

    clFinish(queue);

    t_end = get_time_ns();
    double kernel_time_us = (t_end - t_start) / 1e3;

    printf("[FORENSIC] Exécution kernel : %lu ns (%.3f µs)\n", t_end - t_start, kernel_time_us);

    /* ========================================================================
     * ÉTAPE 10 : Lecture résultats GPU
     * ======================================================================== */

    printf("\n=== ÉTAPE 10 : Lecture résultats GPU ===\n");

    t_start = get_time_ns();

    err = clEnqueueReadBuffer(queue, buf_c, CL_TRUE, 0, buffer_size, host_c, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "ERREUR : clEnqueueReadBuffer = %d\n", err);
        return 1;
    }

    t_end = get_time_ns();
    double download_time_ms = (t_end - t_start) / 1e6;
    double download_gbps = buffer_size / ((t_end - t_start) / 1e9) / 1e9;

    printf("[FORENSIC] Download GPU : %lu ns (%.3f ms)\n", t_end - t_start, download_time_ms);
    printf("[FORENSIC] Débit download : %.2f GB/s\n", download_gbps);

    // Sauvegarder résultats
    save_binary("opencl_output_c.bin", host_c, buffer_size);

    /* ========================================================================
     * ÉTAPE 11 : Vérification résultats
     * ======================================================================== */

    printf("\n=== ÉTAPE 11 : Vérification résultats ===\n");

    bool success = true;
    uint32_t errors = 0;

    for (uint32_t i = 0; i < n && errors < 10; i++) {
        float expected = host_a[i] + host_b[i];
        float actual = host_c[i];
        if (fabsf(actual - expected) > 0.001f) {
            if (errors == 0) {
                fprintf(stderr, "[FORENSIC] ❌ ERREURS DÉTECTÉES :\n");
            }
            fprintf(stderr, "  c[%u] = %.2f (attendu %.2f)\n", i, actual, expected);
            errors++;
            success = false;
        }
    }

    if (success) {
        printf("[FORENSIC] ✅ TOUS les résultats corrects (%u éléments validés)\n", n);
        printf("[FORENSIC] Exemples : c[0]=%.2f, c[100]=%.2f, c[1023]=%.2f\n",
               host_c[0], host_c[100], host_c[1023]);
    } else {
        fprintf(stderr, "[FORENSIC] ❌ %u erreurs détectées\n", errors);
    }

    /* ========================================================================
     * ÉTAPE 12 : Cleanup
     * ======================================================================== */

    printf("\n=== ÉTAPE 12 : Cleanup ===\n");

    free(host_a);
    free(host_b);
    free(host_c);
    free(binary);
    clReleaseMemObject(buf_a);
    clReleaseMemObject(buf_b);
    clReleaseMemObject(buf_c);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);

    printf("[FORENSIC] Cleanup terminé\n");

    /* ========================================================================
     * RÉSUMÉ
     * ======================================================================== */

    printf("\n╔════════════════════════════════════════════════════════════════╗\n");
    printf("║  CAPTURE FORENSIQUE OPENCL TERMINÉE                            ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n");

    printf("\nFichiers générés :\n");
    printf("  - opencl_vector_add_gen9_FULL.bin (%zu bytes)\n", binary_size);
    printf("  - opencl_input_a.bin (%zu bytes)\n", buffer_size);
    printf("  - opencl_input_b.bin (%zu bytes)\n", buffer_size);
    printf("  - opencl_output_c.bin (%zu bytes)\n", buffer_size);

    printf("\nProchaine étape : Reproduire EXACTEMENT dans Level Zero\n");

    return success ? 0 : 1;
}

// Made with Bob
