/*
 * TEST C446 - VALIDATION WRITE-ONLY avec OpenCL
 * Objectif: Valider que le kernel write_test fonctionne avec OpenCL
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <CL/cl.h>
#include <time.h>

#define CHECK_CL(err, msg) if(err != CL_SUCCESS) { \
    fprintf(stderr, "❌ %s: error %d\n", msg, err); \
    exit(1); \
}

#define FORENSIC_LOG(fmt, ...) do { \
    struct timespec ts; \
    clock_gettime(CLOCK_MONOTONIC, &ts); \
    fprintf(stderr, "[%ld.%09ld] " fmt "\n", ts.tv_sec, ts.tv_nsec, ##__VA_ARGS__); \
} while(0)

int main() {
    cl_int err;
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;
    cl_program program;
    cl_kernel kernel;
    
    FORENSIC_LOG("🚀 TEST C446 - VALIDATION WRITE-ONLY DÉMARRAGE");
    
    err = clGetPlatformIDs(1, &platform, NULL);
    CHECK_CL(err, "clGetPlatformIDs");
    
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    CHECK_CL(err, "clGetDeviceIDs");
    
    char device_name[128];
    clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(device_name), device_name, NULL);
    FORENSIC_LOG("✅ Device: %s", device_name);
    
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    CHECK_CL(err, "clCreateContext");
    
    queue = clCreateCommandQueue(context, device, 0, &err);
    CHECK_CL(err, "clCreateCommandQueue");
    
    FILE* f = fopen("tests/test_c446_write_only.cl", "r");
    if (!f) {
        FORENSIC_LOG("❌ Cannot open kernel file");
        return 1;
    }
    fseek(f, 0, SEEK_END);
    size_t source_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* source = malloc(source_size + 1);
    fread(source, 1, source_size, f);
    source[source_size] = '\0';
    fclose(f);
    
    FORENSIC_LOG("✅ Kernel source chargé: %zu bytes", source_size);
    
    program = clCreateProgramWithSource(context, 1, (const char**)&source, &source_size, &err);
    CHECK_CL(err, "clCreateProgramWithSource");
    
    err = clBuildProgram(program, 1, &device, "-cl-std=CL2.0", NULL, NULL);
    if (err != CL_SUCCESS) {
        char log[4096];
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, sizeof(log), log, NULL);
        FORENSIC_LOG("❌ Build failed:\n%s", log);
        return 1;
    }
    FORENSIC_LOG("✅ Kernel compilé");
    
    kernel = clCreateKernel(program, "write_test", &err);
    CHECK_CL(err, "clCreateKernel");
    
    size_t output_size = 256 * sizeof(uint32_t);
    cl_mem output_buf = clCreateBuffer(context, CL_MEM_WRITE_ONLY, output_size, NULL, &err);
    CHECK_CL(err, "clCreateBuffer");
    FORENSIC_LOG("✅ Output buffer créé: %zu bytes", output_size);
    
    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &output_buf);
    CHECK_CL(err, "clSetKernelArg");
    
    size_t global_size = 256;
    FORENSIC_LOG("⚡ Dispatch kernel: %zu threads", global_size);
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_size, NULL, 0, NULL, NULL);
    CHECK_CL(err, "clEnqueueNDRangeKernel");
    
    err = clFinish(queue);
    CHECK_CL(err, "clFinish");
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    FORENSIC_LOG("✅ Kernel exécuté en %.6f secondes", elapsed);
    
    uint32_t output[256];
    memset(output, 0, sizeof(output));
    
    err = clEnqueueReadBuffer(queue, output_buf, CL_TRUE, 0, output_size, output, 0, NULL, NULL);
    CHECK_CL(err, "clEnqueueReadBuffer");
    
    FORENSIC_LOG("✅ Résultats lus");
    
    FORENSIC_LOG("🔍 VALIDATION RÉSULTATS");
    
    int test_passed = 1;
    int errors = 0;
    
    for (int i = 0; i < 256; i++) {
        if (output[i] != 0xAABBCCDD) {
            if (errors < 10) {
                FORENSIC_LOG("❌ output[%d] = 0x%08X (attendu: 0xAABBCCDD)", i, output[i]);
            }
            errors++;
            test_passed = 0;
        }
    }
    
    if (test_passed) {
        FORENSIC_LOG("✅ TEST C446 RÉUSSI - Tous les 256 threads ont écrit 0xAABBCCDD");
        FORENSIC_LOG("📝 CONCLUSION: Write path GPU → RAM fonctionne avec OpenCL");
    } else {
        FORENSIC_LOG("❌ TEST C446 ÉCHOUÉ - %d erreurs sur 256 threads", errors);
    }
    
    clReleaseMemObject(output_buf);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    free(source);
    
    return test_passed ? 0 : 1;
}

// Made with Bob
