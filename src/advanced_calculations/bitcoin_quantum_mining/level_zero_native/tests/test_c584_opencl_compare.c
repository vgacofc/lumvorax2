/* Test C584 - Comparaison OpenCL vs i915 pour identifier ROOT CAUSE kernel ISA */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <CL/cl.h>

const char *kernel_source = 
"__kernel void write_constant(__global uint *output) {\n"
"    output[0] = 0x12345678;\n"
"}\n";

int main(void) {
    printf("🔬 TEST C584 - COMPARAISON OPENCL vs i915\n");
    printf("═══════════════════════════════════════════════════════════\n\n");
    
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;
    cl_program program;
    cl_kernel kernel;
    cl_mem output_buffer;
    cl_int err;
    
    // 1. Setup OpenCL
    printf("[ 10%%] Setting up OpenCL...\n");
    err = clGetPlatformIDs(1, &platform, NULL);
    if (err != CL_SUCCESS) {
        printf("❌ clGetPlatformIDs failed: %d\n", err);
        return 1;
    }
    
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    if (err != CL_SUCCESS) {
        printf("❌ clGetDeviceIDs failed: %d\n", err);
        return 1;
    }
    
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    if (err != CL_SUCCESS) {
        printf("❌ clCreateContext failed: %d\n", err);
        return 1;
    }
    
    queue = clCreateCommandQueue(context, device, 0, &err);
    if (err != CL_SUCCESS) {
        printf("❌ clCreateCommandQueue failed: %d\n", err);
        return 1;
    }
    printf("✅ OpenCL setup complete\n\n");
    
    // 2. Compile kernel
    printf("[ 30%%] Compiling kernel...\n");
    program = clCreateProgramWithSource(context, 1, &kernel_source, NULL, &err);
    if (err != CL_SUCCESS) {
        printf("❌ clCreateProgramWithSource failed: %d\n", err);
        return 1;
    }
    
    err = clBuildProgram(program, 1, &device, "-save-temps=/tmp/opencl_c584", NULL, NULL);
    if (err != CL_SUCCESS) {
        printf("❌ clBuildProgram failed: %d\n", err);
        size_t log_size;
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        char *log = malloc(log_size);
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
        printf("Build log:\n%s\n", log);
        free(log);
        return 1;
    }
    printf("✅ Kernel compiled (binaries saved to /tmp/opencl_c584*)\n\n");
    
    // 3. Get binary
    printf("[ 50%%] Extracting kernel binary...\n");
    size_t binary_size;
    err = clGetProgramInfo(program, CL_PROGRAM_BINARY_SIZES, sizeof(size_t), &binary_size, NULL);
    if (err != CL_SUCCESS) {
        printf("❌ clGetProgramInfo (sizes) failed: %d\n", err);
        return 1;
    }
    
    unsigned char *binary = malloc(binary_size);
    unsigned char *binaries[1] = { binary };
    err = clGetProgramInfo(program, CL_PROGRAM_BINARIES, sizeof(unsigned char *), binaries, NULL);
    if (err != CL_SUCCESS) {
        printf("❌ clGetProgramInfo (binaries) failed: %d\n", err);
        return 1;
    }
    
    // Save binary to file
    FILE *f = fopen("/tmp/opencl_c584_kernel.bin", "wb");
    if (f) {
        fwrite(binary, 1, binary_size, f);
        fclose(f);
        printf("✅ Binary saved: /tmp/opencl_c584_kernel.bin (%zu bytes)\n", binary_size);
    }
    
    // Dump first 128 bytes as hex
    printf("\n📊 First 128 bytes of OpenCL kernel ISA:\n");
    for (size_t i = 0; i < 128 && i < binary_size; i++) {
        if (i % 8 == 0) printf("    ");
        printf("0x%02x, ", binary[i]);
        if ((i + 1) % 8 == 0) printf("\n");
    }
    printf("\n");
    
    // 4. Create kernel and buffers
    printf("[ 70%%] Creating kernel and buffers...\n");
    kernel = clCreateKernel(program, "write_constant", &err);
    if (err != CL_SUCCESS) {
        printf("❌ clCreateKernel failed: %d\n", err);
        return 1;
    }
    
    output_buffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(unsigned int), NULL, &err);
    if (err != CL_SUCCESS) {
        printf("❌ clCreateBuffer failed: %d\n", err);
        return 1;
    }
    
    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &output_buffer);
    if (err != CL_SUCCESS) {
        printf("❌ clSetKernelArg failed: %d\n", err);
        return 1;
    }
    printf("✅ Kernel and buffers ready\n\n");
    
    // 5. Execute kernel
    printf("[ 90%%] Executing kernel...\n");
    size_t global_work_size = 1;
    err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_work_size, NULL, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        printf("❌ clEnqueueNDRangeKernel failed: %d\n", err);
        return 1;
    }
    
    clFinish(queue);
    printf("✅ Kernel executed\n\n");
    
    // 6. Read result
    printf("[100%%] Reading result...\n");
    unsigned int result = 0;
    err = clEnqueueReadBuffer(queue, output_buffer, CL_TRUE, 0, sizeof(unsigned int), &result, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        printf("❌ clEnqueueReadBuffer failed: %d\n", err);
        return 1;
    }
    
    printf("✅ Result: 0x%08x\n\n", result);
    
    // 7. Validate
    if (result == 0x12345678) {
        printf("╔════════════════════════════════════════════════════════════╗\n");
        printf("║  ✅ OPENCL TEST RÉUSSI                                    ║\n");
        printf("║  Result: 0x12345678 (attendu: 0x12345678)                ║\n");
        printf("║                                                            ║\n");
        printf("║  📝 ACTION: Comparer kernel ISA OpenCL vs i915            ║\n");
        printf("║  - OpenCL binary: /tmp/opencl_c584_kernel.bin            ║\n");
        printf("║  - i915 kernel: test_c584_relocation.c lignes 61-78      ║\n");
        printf("╚════════════════════════════════════════════════════════════╝\n");
    } else {
        printf("╔════════════════════════════════════════════════════════════╗\n");
        printf("║  ⚠️  RÉSULTAT INATTENDU                                   ║\n");
        printf("║  Expected: 0x12345678                                     ║\n");
        printf("║  Got:      0x%08x                                     ║\n", result);
        printf("╚════════════════════════════════════════════════════════════╝\n");
    }
    
    // Cleanup
    clReleaseMemObject(output_buffer);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    free(binary);
    
    return (result == 0x12345678) ? 0 : 1;
}

// Made with Bob
