/*
 * Test Runner C293 - Vérification Atomiques Gen9
 * Compile: gcc -o test_atomics test_c293_atomics_runner.c -lOpenCL
 * Run: ./test_atomics
 */

#include <stdio.h>
#include <stdlib.h>
#include <CL/cl.h>

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
    
    printf("╔════════════════════════════════════════╗\n");
    printf("║  Test Atomiques Gen9 Skylake C293     ║\n");
    printf("╚════════════════════════════════════════╝\n\n");
    
    // 1. Get platform
    err = clGetPlatformIDs(1, &platform, NULL);
    CHECK_CL(err, "clGetPlatformIDs");
    
    // 2. Get GPU device
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    CHECK_CL(err, "clGetDeviceIDs");
    
    char device_name[128];
    clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(device_name), device_name, NULL);
    printf("✅ Device: %s\n\n", device_name);
    
    // 3. Create context
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    CHECK_CL(err, "clCreateContext");
    
    // 4. Create queue
    queue = clCreateCommandQueue(context, device, 0, &err);
    CHECK_CL(err, "clCreateCommandQueue");
    
    // 5. Load kernel source
    FILE* f = fopen("tests/test_gen9_atomics.cl", "r");
    if (!f) {
        fprintf(stderr, "❌ Cannot open kernel file\n");
        return 1;
    }
    fseek(f, 0, SEEK_END);
    size_t source_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* source = malloc(source_size + 1);
    fread(source, 1, source_size, f);
    source[source_size] = '\0';
    fclose(f);
    
    // 6. Create program
    program = clCreateProgramWithSource(context, 1, (const char**)&source, &source_size, &err);
    CHECK_CL(err, "clCreateProgramWithSource");
    
    err = clBuildProgram(program, 1, &device, "-cl-std=CL2.0", NULL, NULL);
    if (err != CL_SUCCESS) {
        char log[4096];
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, sizeof(log), log, NULL);
        fprintf(stderr, "❌ Build failed:\n%s\n", log);
        return 1;
    }
    printf("✅ Kernel compiled\n\n");
    
    // TEST 1: Atomic sur local memory
    printf("═══ TEST 1: atomic_max() sur LOCAL memory ═══\n");
    cl_kernel kernel_local = clCreateKernel(program, "test_atomic_local", &err);
    CHECK_CL(err, "clCreateKernel(test_atomic_local)");
    
    cl_mem buf_local = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(cl_uint) * 10, NULL, &err);
    CHECK_CL(err, "clCreateBuffer");
    
    cl_uint test_value = 100;
    clSetKernelArg(kernel_local, 0, sizeof(cl_mem), &buf_local);
    clSetKernelArg(kernel_local, 1, sizeof(cl_uint), &test_value);
    
    size_t global_size = 256;  // 256 work-items
    size_t local_size = 256;   // 1 workgroup
    err = clEnqueueNDRangeKernel(queue, kernel_local, 1, NULL, &global_size, &local_size, 0, NULL, NULL);
    CHECK_CL(err, "clEnqueueNDRangeKernel");
    
    cl_uint result_local[10];
    err = clEnqueueReadBuffer(queue, buf_local, CL_TRUE, 0, sizeof(result_local), result_local, 0, NULL, NULL);
    CHECK_CL(err, "clEnqueueReadBuffer");
    
    printf("Valeur attendue: %u (100 + 255)\n", test_value + 255);
    printf("Valeur obtenue:  %u\n", result_local[0]);
    if (result_local[0] == test_value + 255) {
        printf("✅ atomic_max() LOCAL fonctionne!\n\n");
    } else {
        printf("❌ atomic_max() LOCAL ne fonctionne PAS!\n\n");
    }
    
    // TEST 2: Atomic sur global memory
    printf("═══ TEST 2: atomic_max() sur GLOBAL memory ═══\n");
    cl_kernel kernel_global = clCreateKernel(program, "test_atomic_global", &err);
    CHECK_CL(err, "clCreateKernel(test_atomic_global)");
    
    cl_mem buf_global = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(cl_uint), NULL, &err);
    CHECK_CL(err, "clCreateBuffer");
    
    cl_uint zero = 0;
    clEnqueueWriteBuffer(queue, buf_global, CL_TRUE, 0, sizeof(cl_uint), &zero, 0, NULL, NULL);
    
    clSetKernelArg(kernel_global, 0, sizeof(cl_mem), &buf_global);
    clSetKernelArg(kernel_global, 1, sizeof(cl_uint), &test_value);
    
    global_size = 256;
    err = clEnqueueNDRangeKernel(queue, kernel_global, 1, NULL, &global_size, NULL, 0, NULL, NULL);
    CHECK_CL(err, "clEnqueueNDRangeKernel");
    
    cl_uint result_global;
    err = clEnqueueReadBuffer(queue, buf_global, CL_TRUE, 0, sizeof(cl_uint), &result_global, 0, NULL, NULL);
    CHECK_CL(err, "clEnqueueReadBuffer");
    
    printf("Valeur attendue: %u (100 + 255)\n", test_value + 255);
    printf("Valeur obtenue:  %u\n", result_global);
    if (result_global == test_value + 255) {
        printf("✅ atomic_max() GLOBAL fonctionne!\n\n");
    } else {
        printf("❌ atomic_max() GLOBAL ne fonctionne PAS!\n\n");
    }
    
    // TEST 3: clz() builtin
    printf("═══ TEST 3: clz() builtin ═══\n");
    cl_kernel kernel_clz = clCreateKernel(program, "test_clz_builtin", &err);
    CHECK_CL(err, "clCreateKernel(test_clz_builtin)");
    
    cl_uint test_values[] = {
        0x00000001,  // 31 leading zeros
        0x00000010,  // 27 leading zeros
        0x00001000,  // 19 leading zeros
        0x80000000,  // 0 leading zeros
        0xFFFFFFFF   // 0 leading zeros
    };
    
    cl_mem buf_input = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, 
                                      sizeof(test_values), test_values, &err);
    CHECK_CL(err, "clCreateBuffer(input)");
    
    cl_mem buf_output = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(test_values), NULL, &err);
    CHECK_CL(err, "clCreateBuffer(output)");
    
    clSetKernelArg(kernel_clz, 0, sizeof(cl_mem), &buf_input);
    clSetKernelArg(kernel_clz, 1, sizeof(cl_mem), &buf_output);
    
    global_size = 5;
    err = clEnqueueNDRangeKernel(queue, kernel_clz, 1, NULL, &global_size, NULL, 0, NULL, NULL);
    CHECK_CL(err, "clEnqueueNDRangeKernel");
    
    cl_uint results_clz[5];
    err = clEnqueueReadBuffer(queue, buf_output, CL_TRUE, 0, sizeof(results_clz), results_clz, 0, NULL, NULL);
    CHECK_CL(err, "clEnqueueReadBuffer");
    
    int clz_ok = 1;
    cl_uint expected[] = {31, 27, 19, 0, 0};
    for (int i = 0; i < 5; i++) {
        printf("0x%08X → %u leading zeros (attendu: %u) %s\n", 
               test_values[i], results_clz[i], expected[i],
               results_clz[i] == expected[i] ? "✅" : "❌");
        if (results_clz[i] != expected[i]) clz_ok = 0;
    }
    
    if (clz_ok) {
        printf("✅ clz() builtin fonctionne!\n\n");
    } else {
        printf("❌ clz() builtin ne fonctionne PAS correctement!\n\n");
    }
    
    // Cleanup
    clReleaseMemObject(buf_local);
    clReleaseMemObject(buf_global);
    clReleaseMemObject(buf_input);
    clReleaseMemObject(buf_output);
    clReleaseKernel(kernel_local);
    clReleaseKernel(kernel_global);
    clReleaseKernel(kernel_clz);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    free(source);
    
    printf("╔════════════════════════════════════════╗\n");
    printf("║  Tests terminés                        ║\n");
    printf("╚════════════════════════════════════════╝\n");
    
    return 0;
}

// Made with Bob
