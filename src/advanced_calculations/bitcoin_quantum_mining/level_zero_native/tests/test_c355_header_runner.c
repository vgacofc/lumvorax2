/*
 * Test C355 - Bitcoin Header Validation Runner
 * Compare CPU (OpenSSL) vs GPU on SAME header with nonce=32
 * 
 * This will identify if the problem is:
 * - Endianness (little vs big)
 * - Double SHA-256 chaining
 * - Header format
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

void print_hash(const char* label, const unsigned char* hash, int len) {
    printf("%s: ", label);
    for(int i = 0; i < len; i++) {
        printf("%02x", hash[i]);
    }
    printf("\n");
}

int main() {
    printf("╔═══════════════════════════════════════════════════════════╗\n");
    printf("║  Test C355 - Bitcoin Header Validation (nonce=32)       ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n\n");
    
    /* Bitcoin block header (80 bytes) - SAME as C353 */
    unsigned char header[80];
    memset(header, 0, 80);
    
    /* Version: 0x20000000 (little-endian) */
    header[0] = 0x00; header[1] = 0x00; header[2] = 0x00; header[3] = 0x20;
    
    /* Previous block hash: all zeros */
    /* Merkle root: all zeros */
    /* Timestamp: 0x00000000 */
    
    /* Bits (difficulty): 0x1d00ffff (little-endian) */
    header[72] = 0xff; header[73] = 0xff; header[74] = 0x00; header[75] = 0x1d;
    
    /* Nonce: 32 (little-endian) */
    uint32_t test_nonce = 32;
    header[76] = test_nonce & 0xFF;
    header[77] = (test_nonce >> 8) & 0xFF;
    header[78] = (test_nonce >> 16) & 0xFF;
    header[79] = (test_nonce >> 24) & 0xFF;
    
    printf("═══ HEADER CONFIGURATION ═══\n");
    printf("Version: 0x20000000\n");
    printf("Prev block: all zeros\n");
    printf("Merkle root: all zeros\n");
    printf("Timestamp: 0x00000000\n");
    printf("Bits: 0x1d00ffff\n");
    printf("Nonce: %u (0x%08x)\n\n", test_nonce, test_nonce);
    
    /* CPU: Double SHA-256 */
    printf("═══ CPU CALCULATION (OpenSSL) ═══\n");
    
    unsigned char hash1_cpu[32];
    unsigned char hash2_cpu[32];
    
    /* First SHA-256 */
    SHA256(header, 80, hash1_cpu);
    print_hash("CPU hash1", hash1_cpu, 32);
    
    /* Second SHA-256 */
    SHA256(hash1_cpu, 32, hash2_cpu);
    print_hash("CPU hash2", hash2_cpu, 32);
    
    /* Count leading zeros */
    int cpu_leading_zeros = 0;
    for(int i = 31; i >= 0; i--) {  /* Bitcoin uses little-endian for difficulty */
        if(hash2_cpu[i] == 0) {
            cpu_leading_zeros += 8;
        } else {
            unsigned char byte = hash2_cpu[i];
            while((byte & 0x80) == 0 && byte != 0) {
                cpu_leading_zeros++;
                byte <<= 1;
            }
            break;
        }
    }
    printf("CPU leading zeros: %d bits\n\n", cpu_leading_zeros);
    
    /* GPU: Same calculation via OpenCL */
    printf("═══ GPU CALCULATION (OpenCL) ═══\n");
    
    cl_int err;
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;
    cl_program program;
    cl_kernel kernel;
    
    err = clGetPlatformIDs(1, &platform, NULL);
    CHECK_CL(err, "clGetPlatformIDs");
    
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    CHECK_CL(err, "clGetDeviceIDs");
    
    char device_name[128];
    clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(device_name), device_name, NULL);
    printf("Device: %s\n", device_name);
    
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    CHECK_CL(err, "clCreateContext");
    
    queue = clCreateCommandQueue(context, device, 0, &err);
    CHECK_CL(err, "clCreateCommandQueue");
    
    /* Load kernel */
    FILE* f = fopen("tests/test_c355_bitcoin_header_validation.cl", "r");
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
    
    printf("Kernel loaded: %zu bytes\n", kernel_size);
    
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
    printf("Kernel compiled\n");
    
    kernel = clCreateKernel(program, "test_bitcoin_header_c355", &err);
    CHECK_CL(err, "clCreateKernel");
    
    /* Create output buffer: [0-7]=hash1, [8-15]=hash2, [16]=nonce */
    uint32_t gpu_output[17] = {0};
    cl_mem output_buf = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(gpu_output), NULL, &err);
    CHECK_CL(err, "clCreateBuffer");
    
    /* Set kernel args */
    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &output_buf);
    CHECK_CL(err, "clSetKernelArg[0]");
    
    err = clSetKernelArg(kernel, 1, sizeof(uint32_t), &test_nonce);
    CHECK_CL(err, "clSetKernelArg[1]");
    
    /* Execute kernel */
    size_t global_work_size = 1;
    err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_work_size, NULL, 0, NULL, NULL);
    CHECK_CL(err, "clEnqueueNDRangeKernel");
    
    err = clFinish(queue);
    CHECK_CL(err, "clFinish");
    
    /* Read result */
    err = clEnqueueReadBuffer(queue, output_buf, CL_TRUE, 0, sizeof(gpu_output), gpu_output, 0, NULL, NULL);
    CHECK_CL(err, "clEnqueueReadBuffer");
    
    printf("GPU execution complete\n\n");
    
    /* Convert GPU hash1 to bytes (big-endian) */
    unsigned char hash1_gpu[32];
    for(int i = 0; i < 8; i++) {
        uint32_t val = gpu_output[i];
        hash1_gpu[i*4 + 0] = (val >> 24) & 0xFF;
        hash1_gpu[i*4 + 1] = (val >> 16) & 0xFF;
        hash1_gpu[i*4 + 2] = (val >> 8) & 0xFF;
        hash1_gpu[i*4 + 3] = val & 0xFF;
    }
    print_hash("GPU hash1", hash1_gpu, 32);
    
    /* Convert GPU hash2 to bytes (big-endian) */
    unsigned char hash2_gpu[32];
    for(int i = 0; i < 8; i++) {
        uint32_t val = gpu_output[8 + i];
        hash2_gpu[i*4 + 0] = (val >> 24) & 0xFF;
        hash2_gpu[i*4 + 1] = (val >> 16) & 0xFF;
        hash2_gpu[i*4 + 2] = (val >> 8) & 0xFF;
        hash2_gpu[i*4 + 3] = val & 0xFF;
    }
    print_hash("GPU hash2", hash2_gpu, 32);
    
    printf("GPU nonce tested: %u\n\n", gpu_output[16]);
    
    /* Compare results */
    printf("═══ COMPARISON CPU vs GPU ═══\n");
    
    int hash1_match = (memcmp(hash1_cpu, hash1_gpu, 32) == 0);
    int hash2_match = (memcmp(hash2_cpu, hash2_gpu, 32) == 0);
    
    printf("Hash1 match: %s\n", hash1_match ? "✅ YES" : "❌ NO");
    printf("Hash2 match: %s\n\n", hash2_match ? "✅ YES" : "❌ NO");
    
    if(!hash1_match) {
        printf("⚠️  HASH1 MISMATCH - Differences:\n");
        for(int i = 0; i < 32; i++) {
            if(hash1_cpu[i] != hash1_gpu[i]) {
                printf("  Byte %2d: CPU=%02x GPU=%02x\n", i, hash1_cpu[i], hash1_gpu[i]);
            }
        }
        printf("\n");
    }
    
    if(!hash2_match) {
        printf("⚠️  HASH2 MISMATCH - Differences:\n");
        for(int i = 0; i < 32; i++) {
            if(hash2_cpu[i] != hash2_gpu[i]) {
                printf("  Byte %2d: CPU=%02x GPU=%02x\n", i, hash2_cpu[i], hash2_gpu[i]);
            }
        }
        printf("\n");
    }
    
    /* Diagnosis */
    printf("═══ DIAGNOSIS ═══\n");
    
    if(hash1_match && hash2_match) {
        printf("✅ SUCCESS: GPU calculates Bitcoin double SHA-256 correctly!\n");
        printf("   → Problem must be in:\n");
        printf("     - Output buffer logic (condition if)\n");
        printf("     - Target bits too high\n");
        printf("     - Leading zeros calculation\n");
    } else if(hash1_match && !hash2_match) {
        printf("⚠️  First SHA-256 OK, but second SHA-256 FAILS\n");
        printf("   → Problem in double hash chaining:\n");
        printf("     - Hash1 not passed correctly to hash2\n");
        printf("     - Padding of hash1 (32 bytes) incorrect\n");
        printf("     - Endianness issue in second pass\n");
    } else if(!hash1_match) {
        printf("❌ First SHA-256 already FAILS\n");
        printf("   → Problem in header format:\n");
        printf("     - Endianness of nonce (little vs big)\n");
        printf("     - Header padding incorrect\n");
        printf("     - Input data not read correctly\n");
    }
    
    /* Cleanup */
    clReleaseMemObject(output_buf);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    free(kernel_source);
    
    return (hash1_match && hash2_match) ? 0 : 1;
}

// Made with Bob
