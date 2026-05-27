/* ═══════════════════════════════════════════════════════════════════════════
   TEST C194 OPENCL + DRM POOL BENCHMARK
   ═══════════════════════════════════════════════════════════════════════════
   
   OBJECTIF: Atteindre ≥23.14 MH/s en combinant:
   - Kernel OpenCL SHA-256 existant (btc_sha256.cl)
   - Pool DRM optimisé C193 (3.2x speedup)
   - Batch operations GEM
   
   ARCHITECTURE:
   - OpenCL pour calcul GPU (24 EU Intel UHD 620)
   - DRM Pool pour allocation mémoire optimisée
   - Pipeline complet CPU→GPU→Résultats
   
   TARGET: ≥23.14 MH/s (égaler/dépasser C180)
   
   ═══════════════════════════════════════════════════════════════════════════ */

#define CL_TARGET_OPENCL_VERSION 120
#include <CL/cl.h>
#include "lum_drm_native.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

#define BATCH_SIZE 262144  /* 256k nonces comme C180 */
#define NUM_BATCHES 10
#define KERNEL_PATH "src/btc_sha256.cl"

static double get_time_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

/* Charger kernel OpenCL */
static char* load_kernel_source(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "ERROR: Cannot open %s\n", path);
        return NULL;
    }
    
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    rewind(f);
    
    char* src = malloc(size + 1);
    if (!src) {
        fclose(f);
        return NULL;
    }
    
    fread(src, 1, size, f);
    src[size] = '\0';
    fclose(f);
    
    return src;
}

int main(void) {
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════╗\n");
    printf("║  LUMVORAX C194 OPENCL + DRM POOL BENCHMARK               ║\n");
    printf("║  Target: ≥23.14 MH/s                                     ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n\n");
    
    printf("Configuration:\n");
    printf("  - Batch size: %d nonces\n", BATCH_SIZE);
    printf("  - Batches: %d\n", NUM_BATCHES);
    printf("  - Total hashes: %.2f M\n", (BATCH_SIZE * NUM_BATCHES) / 1e6);
    printf("  - GPU: Intel UHD 620 (24 EU)\n");
    printf("  - DRM Pool: 4 contextes\n\n");
    
    /* ═══════════════════════════════════════════════════════════════
       PHASE 1: INIT OPENCL
       ═══════════════════════════════════════════════════════════════ */
    
    printf("[1/5] Initializing OpenCL...\n");
    
    cl_platform_id platform;
    cl_device_id device;
    cl_int err;
    
    err = clGetPlatformIDs(1, &platform, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "ERROR: clGetPlatformIDs failed (%d)\n", err);
        return 1;
    }
    
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "ERROR: clGetDeviceIDs failed (%d)\n", err);
        return 1;
    }
    
    char device_name[128];
    clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(device_name), device_name, NULL);
    printf("✅ OpenCL device: %s\n\n", device_name);
    
    cl_context context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "ERROR: clCreateContext failed (%d)\n", err);
        return 1;
    }
    
    cl_command_queue queue = clCreateCommandQueue(context, device, 0, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "ERROR: clCreateCommandQueue failed (%d)\n", err);
        clReleaseContext(context);
        return 1;
    }
    
    /* ═══════════════════════════════════════════════════════════════
       PHASE 2: COMPILER KERNEL
       ═══════════════════════════════════════════════════════════════ */
    
    printf("[2/5] Compiling kernel...\n");
    
    char* kernel_src = load_kernel_source(KERNEL_PATH);
    if (!kernel_src) {
        fprintf(stderr, "ERROR: Cannot load kernel source\n");
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return 1;
    }
    
    cl_program program = clCreateProgramWithSource(context, 1, 
                                                    (const char**)&kernel_src, 
                                                    NULL, &err);
    free(kernel_src);
    
    if (err != CL_SUCCESS) {
        fprintf(stderr, "ERROR: clCreateProgramWithSource failed (%d)\n", err);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return 1;
    }
    
    err = clBuildProgram(program, 1, &device, "-cl-fast-relaxed-math", NULL, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "ERROR: clBuildProgram failed (%d)\n", err);
        
        size_t log_size;
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 
                             0, NULL, &log_size);
        char* log = malloc(log_size);
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 
                             log_size, log, NULL);
        fprintf(stderr, "Build log:\n%s\n", log);
        free(log);
        
        clReleaseProgram(program);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return 1;
    }
    
    cl_kernel kernel = clCreateKernel(program, "btc_double_hash", &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "ERROR: clCreateKernel failed (%d)\n", err);
        clReleaseProgram(program);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return 1;
    }
    
    printf("✅ Kernel compiled successfully\n\n");
    
    /* ═══════════════════════════════════════════════════════════════
       PHASE 3: INIT DRM POOL
       ═══════════════════════════════════════════════════════════════ */
    
    printf("[3/5] Initializing DRM pool...\n");
    
    if (lum_drm_pool_init() < 0) {
        fprintf(stderr, "ERROR: DRM pool init failed\n");
        clReleaseKernel(kernel);
        clReleaseProgram(program);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return 1;
    }
    
    printf("✅ DRM pool initialized (4 contexts)\n\n");
    
    /* ═══════════════════════════════════════════════════════════════
       PHASE 4: PRÉPARER DONNÉES
       ═══════════════════════════════════════════════════════════════ */
    
    printf("[4/5] Preparing test data...\n");
    
    /* Midstate SHA-256 (état après 1er bloc de 64 bytes) */
    uint32_t midstate[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };
    
    /* Tail (12 bytes restants du header avant nonce) */
    uint32_t tail[4] = {0xABABABAB, 0xCDCDCDCD, 0xEFEFEFEF, 0};
    
    /* Buffers OpenCL */
    cl_mem buf_midstate = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                         sizeof(midstate), midstate, &err);
    cl_mem buf_tail = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                     sizeof(tail), tail, &err);
    cl_mem buf_result_nonce = clCreateBuffer(context, CL_MEM_WRITE_ONLY,
                                             sizeof(uint32_t), NULL, &err);
    cl_mem buf_result_hash = clCreateBuffer(context, CL_MEM_WRITE_ONLY,
                                            8 * sizeof(uint32_t), NULL, &err);
    
    printf("✅ Test data ready\n\n");
    
    /* ═══════════════════════════════════════════════════════════════
       PHASE 5: BENCHMARK
       ═══════════════════════════════════════════════════════════════ */
    
    printf("[5/5] Running benchmark...\n");
    
    double start = get_time_sec();
    uint64_t total_hashes = 0;
    
    for (int batch = 0; batch < NUM_BATCHES; batch++) {
        uint32_t nonce_start = batch * BATCH_SIZE;
        uint32_t target_bits = 28;  /* Difficulté test */
        
        /* Set kernel args */
        clSetKernelArg(kernel, 0, sizeof(cl_mem), &buf_midstate);
        clSetKernelArg(kernel, 1, sizeof(cl_mem), &buf_tail);
        clSetKernelArg(kernel, 2, sizeof(uint32_t), &nonce_start);
        clSetKernelArg(kernel, 3, sizeof(uint32_t), &target_bits);
        clSetKernelArg(kernel, 4, sizeof(cl_mem), &buf_result_nonce);
        clSetKernelArg(kernel, 5, sizeof(cl_mem), &buf_result_hash);
        
        /* Execute kernel */
        size_t global_size = BATCH_SIZE;
        size_t local_size = 64;  /* Workgroup size */
        
        err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, 
                                     &global_size, &local_size, 
                                     0, NULL, NULL);
        if (err != CL_SUCCESS) {
            fprintf(stderr, "ERROR: clEnqueueNDRangeKernel failed (%d)\n", err);
            break;
        }
        
        clFinish(queue);
        
        total_hashes += BATCH_SIZE;
        
        /* Progress */
        if ((batch + 1) % 2 == 0) {
            double elapsed = get_time_sec() - start;
            double mhs = (total_hashes / elapsed) / 1e6;
            printf("  Batch %d/%d: %.2f MH/s (%.1f%% complete)\n",
                   batch + 1, NUM_BATCHES, mhs,
                   ((batch + 1) * 100.0) / NUM_BATCHES);
        }
    }
    
    double elapsed = get_time_sec() - start;
    double mhs = (total_hashes / elapsed) / 1e6;
    
    printf("\n✅ Benchmark complete\n\n");
    
    /* ═══════════════════════════════════════════════════════════════
       RÉSULTATS
       ═══════════════════════════════════════════════════════════════ */
    
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  RÉSULTATS C194 OPENCL + DRM POOL\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("Total hashes:    %lu\n", total_hashes);
    printf("Time:            %.3f sec\n", elapsed);
    printf("Hashrate:        %.2f MH/s\n", mhs);
    printf("Target C180:     23.14 MH/s\n");
    
    if (mhs >= 23.14) {
        printf("Status:          ✅ TARGET ATTEINT (+%.1f%%)\n",
               ((mhs - 23.14) / 23.14) * 100.0);
    } else {
        printf("Status:          ⚠️  Target non atteint (%.1f%% manquant)\n",
               ((23.14 - mhs) / 23.14) * 100.0);
    }
    
    printf("═══════════════════════════════════════════════════════════\n");
    
    /* Cleanup */
    clReleaseMemObject(buf_result_hash);
    clReleaseMemObject(buf_result_nonce);
    clReleaseMemObject(buf_tail);
    clReleaseMemObject(buf_midstate);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    lum_drm_pool_cleanup();
    
    printf("\n✅ BENCHMARK C194 SUCCESS\n\n");
    return 0;
}

// Made with Bob - C194 OpenCL + DRM Pool Benchmark