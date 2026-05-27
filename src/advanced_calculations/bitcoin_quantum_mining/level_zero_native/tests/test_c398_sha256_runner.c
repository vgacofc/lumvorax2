// C398: SHA256 Minimal Test Runner
// Compile and test SHA256 OpenCL kernel

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <CL/cl.h>

#define CHECK_CL(err, msg) if (err != CL_SUCCESS) { \
    fprintf(stderr, "OpenCL Error %d: %s\n", err, msg); \
    exit(1); \
}

// Bitcoin genesis block header (for testing)
uint32_t genesis_header[20] = {
    0x00000001,  // version
    0x00000000, 0x00000000, 0x00000000, 0x00000000,  // prev_hash (8 uints)
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x3BA3EDFD, 0x7A7B12B2, 0x7AC72C3E, 0x67768F61,  // merkle_root (8 uints)
    0x7FC81BC3, 0x888A5132, 0x32A9C1E3, 0xD47DAC6D,
    0x29AB5F49,  // timestamp
    0xFFFF001D,  // bits (difficulty)
    0x1DAC2B7C   // nonce (will be overwritten)
};

int main() {
    cl_int err;
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;
    cl_program program;
    cl_kernel kernel;
    
    printf("=== C398 SHA256 Minimal Test ===\n\n");
    
    // Get platform
    err = clGetPlatformIDs(1, &platform, NULL);
    CHECK_CL(err, "clGetPlatformIDs");
    
    // Get device
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    CHECK_CL(err, "clGetDeviceIDs");
    
    char device_name[256];
    clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(device_name), device_name, NULL);
    printf("Device: %s\n", device_name);
    
    // Create context
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    CHECK_CL(err, "clCreateContext");
    
    // Create command queue
    queue = clCreateCommandQueue(context, device, 0, &err);
    CHECK_CL(err, "clCreateCommandQueue");
    
    // Read kernel source
    FILE* f = fopen("tests/test_c398_sha256_minimal.cl", "r");
    if (!f) {
        fprintf(stderr, "Failed to open kernel file\n");
        return 1;
    }
    fseek(f, 0, SEEK_END);
    size_t source_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* source = malloc(source_size + 1);
    fread(source, 1, source_size, f);
    source[source_size] = '\0';
    fclose(f);
    
    printf("Kernel source: %zu bytes\n", source_size);
    
    // Create program
    program = clCreateProgramWithSource(context, 1, (const char**)&source, &source_size, &err);
    CHECK_CL(err, "clCreateProgramWithSource");
    
    // Build program
    printf("Building kernel...\n");
    err = clBuildProgram(program, 1, &device, "-cl-std=CL1.2", NULL, NULL);
    if (err != CL_SUCCESS) {
        size_t log_size;
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        char* log = malloc(log_size);
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
        fprintf(stderr, "Build log:\n%s\n", log);
        free(log);
        CHECK_CL(err, "clBuildProgram");
    }
    printf("Build successful!\n");
    
    // Get binary
    size_t binary_size;
    clGetProgramInfo(program, CL_PROGRAM_BINARY_SIZES, sizeof(size_t), &binary_size, NULL);
    printf("Binary size: %zu bytes\n", binary_size);
    
    unsigned char* binary = malloc(binary_size);
    clGetProgramInfo(program, CL_PROGRAM_BINARIES, sizeof(unsigned char*), &binary, NULL);
    
    // Save binary
    FILE* bin_file = fopen("kernels/btc_sha256_c398.bin", "wb");
    fwrite(binary, 1, binary_size, bin_file);
    fclose(bin_file);
    printf("Binary saved to kernels/btc_sha256_c398.bin\n");
    
    // Create kernel
    kernel = clCreateKernel(program, "bitcoin_mine", &err);
    CHECK_CL(err, "clCreateKernel");
    
    // Test with small batch
    size_t num_threads = 256;
    uint32_t nonce_start = 0;
    
    // Create buffers
    cl_mem header_buf = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                       sizeof(genesis_header), genesis_header, &err);
    CHECK_CL(err, "clCreateBuffer header");
    
    cl_mem results_buf = clCreateBuffer(context, CL_MEM_WRITE_ONLY,
                                        num_threads * sizeof(uint32_t), NULL, &err);
    CHECK_CL(err, "clCreateBuffer results");
    
    // Set kernel arguments
    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &header_buf);
    CHECK_CL(err, "clSetKernelArg 0");
    err = clSetKernelArg(kernel, 1, sizeof(cl_mem), &results_buf);
    CHECK_CL(err, "clSetKernelArg 1");
    err = clSetKernelArg(kernel, 2, sizeof(uint32_t), &nonce_start);
    CHECK_CL(err, "clSetKernelArg 2");
    
    // Execute kernel
    printf("\nExecuting kernel with %zu threads...\n", num_threads);
    err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &num_threads, NULL, 0, NULL, NULL);
    CHECK_CL(err, "clEnqueueNDRangeKernel");
    
    // Read results
    uint32_t* results = malloc(num_threads * sizeof(uint32_t));
    err = clEnqueueReadBuffer(queue, results_buf, CL_TRUE, 0,
                              num_threads * sizeof(uint32_t), results, 0, NULL, NULL);
    CHECK_CL(err, "clEnqueueReadBuffer");
    
    // Analyze results
    printf("\nResults:\n");
    uint32_t best_result = 0;
    uint32_t best_nonce = 0;
    uint32_t best_zeros = 0;
    
    for (size_t i = 0; i < num_threads; i++) {
        uint32_t leading_zeros = results[i] >> 16;
        uint32_t nonce = results[i] & 0xFFFF;
        
        if (leading_zeros > best_zeros) {
            best_zeros = leading_zeros;
            best_nonce = nonce;
            best_result = results[i];
        }
        
        if (i < 10 || leading_zeros > 0) {
            printf("  Thread %3zu: nonce=%u leading_zeros=%u result=0x%08X\n",
                   i, nonce, leading_zeros, results[i]);
        }
    }
    
    printf("\nBest result:\n");
    printf("  Nonce: %u\n", best_nonce);
    printf("  Leading zeros: %u bits\n", best_zeros);
    printf("  Result: 0x%08X\n", best_result);
    
    if (best_zeros > 0) {
        printf("\n✅ SUCCESS: Found hash with %u leading zero bits!\n", best_zeros);
    } else {
        printf("\n⚠️  No leading zeros found (expected for genesis block with wrong nonce)\n");
    }
    
    // Cleanup
    free(results);
    free(binary);
    free(source);
    clReleaseMemObject(results_buf);
    clReleaseMemObject(header_buf);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    
    printf("\n=== Test Complete ===\n");
    return 0;
}

// Made with Bob
