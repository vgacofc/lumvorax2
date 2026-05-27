/*
 * CYCLE C197.10 - CAPTURE COMMAND STREAM COMPLET OPENCL
 * =======================================================
 * 
 * OBJECTIF : Capturer command stream GPU complet généré par OpenCL
 * 
 * APPROCHE SIMPLIFIÉE :
 * 1. Compiler et exécuter kernel OpenCL minimal
 * 2. Accéder aux GEM objects via DRM après dispatch
 * 3. Dumper tous les buffers GPU actifs
 * 4. Analyser contenu pour identifier command stream
 * 
 * NOTE : Capture complète nécessite accès root ou Intel NEO debug
 * Cette version capture ce qui est accessible sans root
 */

#define CL_TARGET_OPENCL_VERSION 300
#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>
#include <libdrm/drm.h>
#include <libdrm/i915_drm.h>

// Kernel OpenCL minimal
const char* kernel_source = 
"__kernel void test_write(__global uint* output) {\n"
"    output[0] = 0xDEADBEEF;\n"
"}\n";

// Fonction dump buffer GPU
static int dump_gpu_buffer(int drm_fd, uint32_t handle, const char* name, FILE* report) {
    fprintf(report, "\n## Buffer %s (handle=%u)\n\n", name, handle);
    
    // Tenter de mapper buffer
    struct drm_i915_gem_mmap mmap_arg = {
        .handle = handle,
        .offset = 0,
        .size = 4096,
        .flags = 0
    };
    
    if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_MMAP, &mmap_arg) != 0) {
        fprintf(report, "**Status** : Cannot map (errno=%d)\n", errno);
        return -1;
    }
    
    unsigned char* ptr = (unsigned char*)mmap_arg.addr_ptr;
    
    // Dump premiers 512 bytes
    fprintf(report, "**Size** : %llu bytes\n\n", mmap_arg.size);
    fprintf(report, "**Content** (first 512 bytes):\n\n```\n");
    
    for (size_t i = 0; i < 512 && i < mmap_arg.size; i += 16) {
        fprintf(report, "%04zx: ", i);
        for (size_t j = 0; j < 16 && i + j < 512; j++) {
            fprintf(report, "%02x ", ptr[i + j]);
        }
        fprintf(report, "\n");
    }
    fprintf(report, "```\n");
    
    // Sauvegarder binaire complet
    char filename[256];
    snprintf(filename, sizeof(filename), "c197_10_buffer_%s_h%u.bin", name, handle);
    FILE* f = fopen(filename, "wb");
    if (f) {
        fwrite(ptr, 1, mmap_arg.size, f);
        fclose(f);
        fprintf(report, "\n**Binary saved** : `%s`\n", filename);
    }
    
    munmap(ptr, mmap_arg.size);
    return 0;
}

// Fonction scan GEM handles
static void scan_gem_handles(int drm_fd, FILE* report) {
    fprintf(report, "\n# GEM Objects Scan\n\n");
    fprintf(report, "Scanning handles 1-50...\n\n");
    
    int found = 0;
    for (uint32_t handle = 1; handle <= 50; handle++) {
        // Tester si handle existe via GEM_BUSY
        struct drm_i915_gem_busy busy = {
            .handle = handle
        };
        
        if (ioctl(drm_fd, DRM_IOCTL_I915_GEM_BUSY, &busy) == 0) {
            fprintf(report, "### Handle %u : ACTIVE (busy=0x%x)\n", handle, busy.busy);
            dump_gpu_buffer(drm_fd, handle, "active", report);
            found++;
        }
    }
    
    fprintf(report, "\n**Total active handles found** : %d\n", found);
}

int main(int argc, char** argv) {
    printf("=== CYCLE C197.10 - CAPTURE COMMAND STREAM OPENCL ===\n\n");
    
    // Ouvrir fichier rapport
    FILE* report = fopen("c197_10_capture_report.md", "w");
    if (!report) {
        fprintf(stderr, "[ERROR] Cannot create report file\n");
        return 1;
    }
    
    fprintf(report, "# RAPPORT C197.10 - CAPTURE COMMAND STREAM OPENCL\n\n");
    fprintf(report, "**Date** : 2026-05-05\n");
    fprintf(report, "**Objectif** : Capturer command stream GPU complet\n\n");
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    cl_int err;
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_program program;
    cl_kernel kernel;
    cl_command_queue queue;
    cl_mem output_buffer;
    
    // 1. Ouvrir DRM
    printf("[STEP 1] Opening DRM device\n");
    fprintf(report, "## Step 1 : DRM Initialization\n\n");
    
    int drm_fd = open("/dev/dri/renderD128", O_RDWR);
    if (drm_fd < 0) {
        fprintf(stderr, "[ERROR] Cannot open DRM device\n");
        fprintf(report, "**ERROR** : Cannot open /dev/dri/renderD128\n");
        fclose(report);
        return 1;
    }
    
    printf("[SUCCESS] DRM opened (fd=%d)\n", drm_fd);
    fprintf(report, "**DRM fd** : %d\n\n", drm_fd);
    
    // 2. Initialisation OpenCL
    printf("\n[STEP 2] OpenCL initialization\n");
    fprintf(report, "## Step 2 : OpenCL Initialization\n\n");
    
    err = clGetPlatformIDs(1, &platform, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[ERROR] clGetPlatformIDs failed: %d\n", err);
        fprintf(report, "**ERROR** : clGetPlatformIDs failed (%d)\n", err);
        fclose(report);
        return 1;
    }
    
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[ERROR] clGetDeviceIDs failed: %d\n", err);
        fprintf(report, "**ERROR** : clGetDeviceIDs failed (%d)\n", err);
        fclose(report);
        return 1;
    }
    
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[ERROR] clCreateContext failed: %d\n", err);
        fprintf(report, "**ERROR** : clCreateContext failed (%d)\n", err);
        fclose(report);
        return 1;
    }
    
    queue = clCreateCommandQueue(context, device, 0, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[ERROR] clCreateCommandQueue failed: %d\n", err);
        fprintf(report, "**ERROR** : clCreateCommandQueue failed (%d)\n", err);
        fclose(report);
        return 1;
    }
    
    printf("[SUCCESS] OpenCL initialized\n");
    fprintf(report, "**Status** : OpenCL initialized successfully\n\n");
    
    // 3. Compilation kernel
    printf("\n[STEP 3] Kernel compilation\n");
    fprintf(report, "## Step 3 : Kernel Compilation\n\n");
    
    size_t source_len = strlen(kernel_source);
    program = clCreateProgramWithSource(context, 1, &kernel_source, &source_len, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[ERROR] clCreateProgramWithSource failed: %d\n", err);
        fprintf(report, "**ERROR** : clCreateProgramWithSource failed (%d)\n", err);
        fclose(report);
        return 1;
    }
    
    err = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[ERROR] clBuildProgram failed: %d\n", err);
        fprintf(report, "**ERROR** : clBuildProgram failed (%d)\n", err);
        fclose(report);
        return 1;
    }
    
    kernel = clCreateKernel(program, "test_write", &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[ERROR] clCreateKernel failed: %d\n", err);
        fprintf(report, "**ERROR** : clCreateKernel failed (%d)\n", err);
        fclose(report);
        return 1;
    }
    
    printf("[SUCCESS] Kernel compiled\n");
    fprintf(report, "**Status** : Kernel compiled successfully\n\n");
    
    // 4. Créer buffer output
    printf("\n[STEP 4] Creating output buffer\n");
    fprintf(report, "## Step 4 : Output Buffer Creation\n\n");
    
    output_buffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 32, NULL, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[ERROR] clCreateBuffer failed: %d\n", err);
        fprintf(report, "**ERROR** : clCreateBuffer failed (%d)\n", err);
        fclose(report);
        return 1;
    }
    
    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &output_buffer);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[ERROR] clSetKernelArg failed: %d\n", err);
        fprintf(report, "**ERROR** : clSetKernelArg failed (%d)\n", err);
        fclose(report);
        return 1;
    }
    
    printf("[SUCCESS] Output buffer created\n");
    fprintf(report, "**Status** : Output buffer created (32 bytes)\n\n");
    
    // 5. Scanner GEM objects AVANT dispatch
    printf("\n[STEP 5] Scanning GEM objects BEFORE dispatch\n");
    fprintf(report, "## Step 5 : GEM Objects BEFORE Dispatch\n\n");
    scan_gem_handles(drm_fd, report);
    
    // 6. Exécuter kernel
    printf("\n[STEP 6] Executing kernel\n");
    fprintf(report, "\n## Step 6 : Kernel Execution\n\n");
    
    size_t global_work_size = 1;
    err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_work_size, NULL, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[ERROR] clEnqueueNDRangeKernel failed: %d\n", err);
        fprintf(report, "**ERROR** : clEnqueueNDRangeKernel failed (%d)\n", err);
        fclose(report);
        return 1;
    }
    
    clFinish(queue);
    
    printf("[SUCCESS] Kernel executed\n");
    fprintf(report, "**Status** : Kernel executed successfully\n\n");
    
    // 7. Scanner GEM objects APRÈS dispatch
    printf("\n[STEP 7] Scanning GEM objects AFTER dispatch\n");
    fprintf(report, "## Step 7 : GEM Objects AFTER Dispatch\n\n");
    scan_gem_handles(drm_fd, report);
    
    // 8. Vérifier résultat
    printf("\n[STEP 8] Verifying result\n");
    fprintf(report, "\n## Step 8 : Result Verification\n\n");
    
    uint32_t output[8] = {0};
    err = clEnqueueReadBuffer(queue, output_buffer, CL_TRUE, 0, 32, output, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[ERROR] clEnqueueReadBuffer failed: %d\n", err);
        fprintf(report, "**ERROR** : clEnqueueReadBuffer failed (%d)\n", err);
    } else {
        printf("  output[0] = 0x%08x (expected: 0xDEADBEEF)\n", output[0]);
        fprintf(report, "**output[0]** : 0x%08x (expected: 0xDEADBEEF)\n\n", output[0]);
        
        if (output[0] == 0xDEADBEEF) {
            printf("[SUCCESS] ✓ Kernel output correct\n");
            fprintf(report, "**Status** : ✅ Kernel output correct\n\n");
        } else {
            printf("[FAILURE] ✗ Kernel output incorrect\n");
            fprintf(report, "**Status** : ❌ Kernel output incorrect\n\n");
        }
    }
    
    // 9. Métriques finales
    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed_ms = (end.tv_sec - start.tv_sec) * 1000.0 +
                       (end.tv_nsec - start.tv_nsec) / 1000000.0;
    
    printf("\n=== RÉSULTATS C197.10 ===\n");
    printf("Total time: %.3f ms\n", elapsed_ms);
    printf("Report saved: c197_10_capture_report.md\n");
    
    fprintf(report, "## Summary\n\n");
    fprintf(report, "**Total time** : %.3f ms\n", elapsed_ms);
    fprintf(report, "**Status** : Capture completed\n\n");
    fprintf(report, "### Files Generated\n\n");
    fprintf(report, "- `c197_10_capture_report.md` (this file)\n");
    fprintf(report, "- `c197_10_buffer_active_h*.bin` (GEM buffers dumps)\n\n");
    fprintf(report, "### Next Steps\n\n");
    fprintf(report, "1. Analyze captured buffers for command stream patterns\n");
    fprintf(report, "2. Identify MEDIA_VFE_STATE, GPGPU_WALKER commands\n");
    fprintf(report, "3. Extract surface states and binding table\n");
    fprintf(report, "4. Reconstruct complete GPU context for DRM injection\n");
    
    // Cleanup
    clReleaseMemObject(output_buffer);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    close(drm_fd);
    fclose(report);
    
    printf("\n[SUCCESS] ✓ Capture complete\n");
    return 0;
}

// Made with Bob
