// Test C585: Capture batch OpenCL COMPLET avec instrumentation i915
// Objectif: Voir EXACTEMENT ce qu'OpenCL envoie au GPU
#define CL_TARGET_OPENCL_VERSION 300
#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <i915_drm.h>
#include <xf86drm.h>

// Kernel OpenCL COMPLET - Minage Bitcoin SHA256d
const char* kernel_source = 
"__kernel void bitcoin_mine(\n"
"    __global uint* output,\n"
"    uint target_bits,\n"
"    uint nonce_start)\n"
"{\n"
"    uint gid = get_global_id(0);\n"
"    uint nonce = nonce_start + gid;\n"
"    \n"
"    // Block header Bitcoin (80 bytes)\n"
"    uint header[20];\n"
"    header[0] = 0x01000000;  // Version\n"
"    header[1] = 0x00000000;  // Previous block hash (partie 1)\n"
"    header[2] = 0x00000000;\n"
"    header[3] = 0x00000000;\n"
"    header[4] = 0x00000000;\n"
"    header[5] = 0x00000000;\n"
"    header[6] = 0x00000000;\n"
"    header[7] = 0x00000000;\n"
"    header[8] = 0x00000000;  // Previous block hash (partie 2)\n"
"    header[9] = 0x00000000;  // Merkle root (partie 1)\n"
"    header[10] = 0x00000000;\n"
"    header[11] = 0x00000000;\n"
"    header[12] = 0x00000000;\n"
"    header[13] = 0x00000000;\n"
"    header[14] = 0x00000000;\n"
"    header[15] = 0x00000000;\n"
"    header[16] = 0x00000000; // Merkle root (partie 2)\n"
"    header[17] = 0x00000000; // Timestamp\n"
"    header[18] = target_bits; // Bits (difficulty)\n"
"    header[19] = nonce;      // Nonce\n"
"    \n"
"    // SHA256 constants\n"
"    uint K[64] = {\n"
"        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,\n"
"        0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,\n"
"        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,\n"
"        0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,\n"
"        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,\n"
"        0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,\n"
"        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,\n"
"        0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,\n"
"        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,\n"
"        0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,\n"
"        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,\n"
"        0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,\n"
"        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,\n"
"        0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,\n"
"        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,\n"
"        0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2\n"
"    };\n"
"    \n"
"    // SHA256 initial hash values\n"
"    uint H[8] = {\n"
"        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,\n"
"        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19\n"
"    };\n"
"    \n"
"    // Premier SHA256 (simplifié pour test)\n"
"    uint a = H[0];\n"
"    uint b = H[1];\n"
"    uint c = H[2];\n"
"    uint d = H[3];\n"
"    uint e = H[4];\n"
"    uint f = H[5];\n"
"    uint g = H[6];\n"
"    uint h = H[7];\n"
"    \n"
"    // Quelques rounds SHA256 (simplifié)\n"
"    for(int i = 0; i < 16; i++) {\n"
"        uint S1 = rotate(e, 26U) ^ rotate(e, 21U) ^ rotate(e, 7U);\n"
"        uint ch = (e & f) ^ ((~e) & g);\n"
"        uint temp1 = h + S1 + ch + K[i] + header[i];\n"
"        uint S0 = rotate(a, 30U) ^ rotate(a, 19U) ^ rotate(a, 10U);\n"
"        uint maj = (a & b) ^ (a & c) ^ (b & c);\n"
"        uint temp2 = S0 + maj;\n"
"        \n"
"        h = g;\n"
"        g = f;\n"
"        f = e;\n"
"        e = d + temp1;\n"
"        d = c;\n"
"        c = b;\n"
"        b = a;\n"
"        a = temp1 + temp2;\n"
"    }\n"
"    \n"
"    // Check si hash < target (simplifié)\n"
"    uint hash_result = a + H[0];\n"
"    \n"
"    // Si leading zeros suffisants, écrire le nonce\n"
"    if((hash_result & 0xFF000000) == 0) {\n"
"        atomic_min(output, nonce);\n"
"    }\n"
"}\n";

int main() {
    printf("=== TEST C585: CAPTURE BATCH OPENCL COMPLET ===\n\n");
    
    struct timeval tv_start, tv_end;
    gettimeofday(&tv_start, NULL);
    
    // 1. Init OpenCL
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;
    cl_program program;
    cl_kernel kernel;
    cl_int err;
    
    err = clGetPlatformIDs(1, &platform, NULL);
    if(err != CL_SUCCESS) {
        printf("❌ clGetPlatformIDs failed: %d\n", err);
        return 1;
    }
    
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    if(err != CL_SUCCESS) {
        printf("❌ clGetDeviceIDs failed: %d\n", err);
        return 1;
    }
    
    // Device info
    char device_name[256];
    clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(device_name), device_name, NULL);
    printf("📊 Device: %s\n", device_name);
    
    cl_uint compute_units;
    clGetDeviceInfo(device, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(compute_units), &compute_units, NULL);
    printf("📊 Compute Units: %u\n", compute_units);
    
    size_t max_work_group_size;
    clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(max_work_group_size), &max_work_group_size, NULL);
    printf("📊 Max Work Group Size: %zu\n\n", max_work_group_size);
    
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    if(err != CL_SUCCESS) {
        printf("❌ clCreateContext failed: %d\n", err);
        return 1;
    }
    
    queue = clCreateCommandQueueWithProperties(context, device, NULL, &err);
    if(err != CL_SUCCESS) {
        printf("❌ clCreateCommandQueue failed: %d\n", err);
        return 1;
    }
    
    // 2. Compile kernel
    printf("🔧 Compiling kernel...\n");
    program = clCreateProgramWithSource(context, 1, &kernel_source, NULL, &err);
    if(err != CL_SUCCESS) {
        printf("❌ clCreateProgramWithSource failed: %d\n", err);
        return 1;
    }
    
    err = clBuildProgram(program, 1, &device, "-cl-std=CL3.0", NULL, NULL);
    if(err != CL_SUCCESS) {
        printf("❌ clBuildProgram failed: %d\n", err);
        
        size_t log_size;
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        char* log = malloc(log_size);
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
        printf("Build log:\n%s\n", log);
        free(log);
        return 1;
    }
    printf("✅ Kernel compiled\n\n");
    
    kernel = clCreateKernel(program, "bitcoin_mine", &err);
    if(err != CL_SUCCESS) {
        printf("❌ clCreateKernel failed: %d\n", err);
        return 1;
    }
    
    // 3. Create buffers
    cl_mem output_buffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(cl_uint), NULL, &err);
    if(err != CL_SUCCESS) {
        printf("❌ clCreateBuffer failed: %d\n", err);
        return 1;
    }
    
    cl_uint initial_value = 0xFFFFFFFF;
    err = clEnqueueWriteBuffer(queue, output_buffer, CL_TRUE, 0, sizeof(cl_uint), &initial_value, 0, NULL, NULL);
    
    // 4. Set kernel args
    cl_uint target_bits = 0x1d00ffff;  // Difficulté Bitcoin
    cl_uint nonce_start = 0;
    
    clSetKernelArg(kernel, 0, sizeof(cl_mem), &output_buffer);
    clSetKernelArg(kernel, 1, sizeof(cl_uint), &target_bits);
    clSetKernelArg(kernel, 2, sizeof(cl_uint), &nonce_start);
    
    // 5. Execute kernel - PETIT pour capturer batch complet
    size_t global_work_size = 256;  // Petit pour analyse
    size_t local_work_size = 16;    // Petit work group
    
    printf("🚀 Executing kernel (global=%zu, local=%zu)...\n", global_work_size, local_work_size);
    printf("⏱️  Capture i915 batch pendant exécution...\n\n");
    
    err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_work_size, &local_work_size, 0, NULL, NULL);
    if(err != CL_SUCCESS) {
        printf("❌ clEnqueueNDRangeKernel failed: %d\n", err);
        return 1;
    }
    
    clFinish(queue);
    
    // 6. Read result
    cl_uint result;
    err = clEnqueueReadBuffer(queue, output_buffer, CL_TRUE, 0, sizeof(cl_uint), &result, 0, NULL, NULL);
    
    gettimeofday(&tv_end, NULL);
    double elapsed = (tv_end.tv_sec - tv_start.tv_sec) + (tv_end.tv_usec - tv_start.tv_usec) / 1000000.0;
    
    printf("✅ Kernel executed successfully\n");
    printf("📊 Result: 0x%08X\n", result);
    printf("⏱️  Total time: %.6f seconds\n\n", elapsed);
    
    // 7. Cleanup
    clReleaseMemObject(output_buffer);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    
    printf("=== INSTRUCTIONS POUR CAPTURER BATCH ===\n");
    printf("1. Installer: sudo apt-get install intel-gpu-tools\n");
    printf("2. Pendant exécution, dans autre terminal:\n");
    printf("   sudo intel_gpu_top\n");
    printf("   sudo cat /sys/kernel/debug/dri/1/i915_gem_objects\n");
    printf("3. Pour capturer batch complet:\n");
    printf("   echo 1 | sudo tee /sys/module/i915/parameters/enable_cmd_parser\n");
    printf("   Puis relancer ce test\n\n");
    
    return 0;
}
