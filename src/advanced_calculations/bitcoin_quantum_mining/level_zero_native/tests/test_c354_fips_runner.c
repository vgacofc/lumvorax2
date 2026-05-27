/*
 * Test C354 - FIPS SHA-256 Runner
 * Compile: gcc -o tests/test_c354_fips_runner tests/test_c354_fips_runner.c -lOpenCL -lssl -lcrypto
 * Run: ./tests/test_c354_fips_runner
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <CL/cl.h>
#include <openssl/sha.h>

#define CHECK_CL(err, msg) if(err != CL_SUCCESS) { \
    fprintf(stderr, "❌ %s: error %d\n", msg, err); \
    exit(1); \
}

int main() {
    cl_int err;
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;
    cl_program program;
    cl_kernel kernel;
    
    printf("╔═══════════════════════════════════════════════════╗\n");
    printf("║  Test C354 - FIPS SHA-256 \"abc\" Vector          ║\n");
    printf("╚═══════════════════════════════════════════════════╝\n\n");
    
    /* Test CPU (OpenSSL) */
    printf("═══ TEST CPU (OpenSSL) ═══\n");
    const unsigned char input[] = "abc";
    unsigned char cpu_hash[32];
    
    SHA256(input, 3, cpu_hash);
    
    printf("Input: \"abc\" (3 bytes)\n");
    printf("CPU hash: ");
    for(int i = 0; i < 32; i++) printf("%02x", cpu_hash[i]);
    printf("\n");
    printf("Expected: ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad\n");
    
    /* Verify CPU matches expected */
    const unsigned char expected[] = {
        0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea,
        0x41, 0x41, 0x40, 0xde, 0x5d, 0xae, 0x22, 0x23,
        0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17, 0x7a, 0x9c,
        0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad
    };
    
    if(memcmp(cpu_hash, expected, 32) == 0) {
        printf("✅ CPU matches FIPS expected\n\n");
    } else {
        printf("❌ CPU does NOT match FIPS expected!\n\n");
        return 1;
    }
    
    /* Setup OpenCL */
    printf("═══ SETUP GPU (OpenCL) ═══\n");
    
    err = clGetPlatformIDs(1, &platform, NULL);
    CHECK_CL(err, "clGetPlatformIDs");
    
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    CHECK_CL(err, "clGetDeviceIDs");
    
    char device_name[128];
    clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(device_name), device_name, NULL);
    printf("✅ Device: %s\n", device_name);
    
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    CHECK_CL(err, "clCreateContext");
    
    queue = clCreateCommandQueue(context, device, 0, &err);
    CHECK_CL(err, "clCreateCommandQueue");
    
    /* Load kernel */
    FILE* f = fopen("tests/test_c354_sha256_fips.cl", "r");
    if (!f) {
        fprintf(stderr, "❌ Cannot open kernel file\n");
        return 1;
    }
    
    fseek(f, 0, SEEK_END);
    size_t kernel_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char* kernel_source = malloc(kernel_size + 1);
    fread(kernel_source, 1, kernel_size, f);
    kernel_source[kernel_size] = '\0';
    fclose(f);
    
    printf("✅ Kernel loaded: %zu bytes\n", kernel_size);
    
    /* Compile kernel */
    program = clCreateProgramWithSource(context, 1, (const char**)&kernel_source, &kernel_size, &err);
    CHECK_CL(err, "clCreateProgramWithSource");
    
    err = clBuildProgram(program, 1, &device, "-cl-std=CL2.0", NULL, NULL);
    if (err != CL_SUCCESS) {
        char build_log[4096];
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, sizeof(build_log), build_log, NULL);
        fprintf(stderr, "❌ Build failed:\n%s\n", build_log);
        return 1;
    }
    printf("✅ Kernel compiled\n");
    
    kernel = clCreateKernel(program, "test_sha256_fips_abc", &err);
    CHECK_CL(err, "clCreateKernel");
    
    /* Create output buffer */
    uint32_t gpu_hash[8] = {0};
    cl_mem output_buf = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 32, NULL, &err);
    CHECK_CL(err, "clCreateBuffer");
    
    /* Set kernel arg */
    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &output_buf);
    CHECK_CL(err, "clSetKernelArg");
    
    /* Execute kernel */
    printf("\n═══ TEST GPU (Kernel C354) ═══\n");
    
    size_t global_work_size = 1;
    err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_work_size, NULL, 0, NULL, NULL);
    CHECK_CL(err, "clEnqueueNDRangeKernel");
    
    err = clFinish(queue);
    CHECK_CL(err, "clFinish");
    
    /* Read result */
    err = clEnqueueReadBuffer(queue, output_buf, CL_TRUE, 0, 32, gpu_hash, 0, NULL, NULL);
    CHECK_CL(err, "clEnqueueReadBuffer");
    
    printf("✅ GPU execution complete\n");
    printf("GPU hash: ");
    for(int i = 0; i < 8; i++) {
        /* Convert to big-endian for display */
        uint32_t val = gpu_hash[i];
        printf("%02x%02x%02x%02x", 
               (val >> 24) & 0xFF,
               (val >> 16) & 0xFF,
               (val >> 8) & 0xFF,
               val & 0xFF);
    }
    printf("\n");
    
    /* Compare CPU vs GPU */
    printf("\n═══ COMPARAISON CPU vs GPU ═══\n");
    
    /* Convert GPU hash to bytes (big-endian) */
    unsigned char gpu_hash_bytes[32];
    for(int i = 0; i < 8; i++) {
        uint32_t val = gpu_hash[i];
        gpu_hash_bytes[i*4 + 0] = (val >> 24) & 0xFF;
        gpu_hash_bytes[i*4 + 1] = (val >> 16) & 0xFF;
        gpu_hash_bytes[i*4 + 2] = (val >> 8) & 0xFF;
        gpu_hash_bytes[i*4 + 3] = val & 0xFF;
    }
    
    if(memcmp(cpu_hash, gpu_hash_bytes, 32) == 0) {
        printf("✅ SUCCESS: GPU matches CPU (FIPS correct)!\n");
        printf("\n🎉 SHA-256 kernel is CORRECT for FIPS test vector\n");
        printf("   → Problem is likely in Bitcoin-specific code\n");
        printf("   → Next: Test Bitcoin header format (Étape 2)\n");
        return 0;
    } else {
        printf("❌ FAILURE: GPU does NOT match CPU\n");
        printf("\n⚠️  SHA-256 kernel has fundamental bug\n");
        printf("   → Need to fix basic SHA-256 implementation\n");
        
        /* Show differences */
        printf("\nDifferences:\n");
        for(int i = 0; i < 32; i++) {
            if(cpu_hash[i] != gpu_hash_bytes[i]) {
                printf("  Byte %2d: CPU=%02x GPU=%02x\n", i, cpu_hash[i], gpu_hash_bytes[i]);
            }
        }
        return 1;
    }
    
    /* Cleanup */
    clReleaseMemObject(output_buf);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(device);
    free(kernel_source);
}

// Made with Bob
