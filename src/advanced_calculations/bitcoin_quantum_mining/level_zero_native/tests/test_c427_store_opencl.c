/*
 * TEST C427 #1 - VALIDATION STORE avec OpenCL
 * Objectif: Valider que le kernel écrit correctement output[0]=0x12345678
 * 
 * Si ce test RÉUSSIT: Le kernel fonctionne, problème dans infra i915 native
 * Si ce test ÉCHOUE: Le kernel lui-même a un problème
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
    
    FORENSIC_LOG("🚀 TEST C427 #1 - VALIDATION STORE DÉMARRAGE");
    
    // 1. Get platform
    err = clGetPlatformIDs(1, &platform, NULL);
    CHECK_CL(err, "clGetPlatformIDs");
    
    // 2. Get GPU device
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    CHECK_CL(err, "clGetDeviceIDs");
    
    char device_name[128];
    clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(device_name), device_name, NULL);
    FORENSIC_LOG("✅ Device: %s", device_name);
    
    // 3. Create context
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    CHECK_CL(err, "clCreateContext");
    
    // 4. Create queue
    queue = clCreateCommandQueue(context, device, 0, &err);
    CHECK_CL(err, "clCreateCommandQueue");
    
    // 5. Load kernel source
    FILE* f = fopen("tests/test_c427_store_validation.cl", "r");
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
    
    // 6. Create program
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
    
    // 7. Create kernel
    kernel = clCreateKernel(program, "test_store_validation", &err);
    CHECK_CL(err, "clCreateKernel");
    
    // 8. Create output buffer (257 uint32_t)
    size_t output_size = 257 * sizeof(uint32_t);
    cl_mem output_buf = clCreateBuffer(context, CL_MEM_WRITE_ONLY, output_size, NULL, &err);
    CHECK_CL(err, "clCreateBuffer");
    FORENSIC_LOG("✅ Output buffer créé: %zu bytes", output_size);
    
    // 9. Set kernel argument
    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &output_buf);
    CHECK_CL(err, "clSetKernelArg");
    
    // 10. Execute kernel (256 threads)
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
    
    // 11. Read results
    uint32_t output[257];
    memset(output, 0, sizeof(output));
    
    err = clEnqueueReadBuffer(queue, output_buf, CL_TRUE, 0, output_size, output, 0, NULL, NULL);
    CHECK_CL(err, "clEnqueueReadBuffer");
    
    FORENSIC_LOG("✅ Résultats lus");
    
    // 12. Validate results
    FORENSIC_LOG("🔍 VALIDATION RÉSULTATS");
    FORENSIC_LOG("📊 output[0] = 0x%08X (attendu: 0x12345678)", output[0]);
    
    int test_passed = 0;
    
    // TEST ABSOLU
    if (output[0] == 0x12345678) {
        FORENSIC_LOG("✅ TEST ABSOLU RÉUSSI: output[0] == 0x12345678");
        test_passed = 1;
        
        // TEST SECONDAIRE: Vérifier pattern gid
        int pattern_ok = 1;
        for (int i = 0; i < 256; i++) {
            if (output[i + 1] != (uint32_t)i) {
                FORENSIC_LOG("⚠️  output[%d] = %u (attendu: %d)", i+1, output[i+1], i);
                pattern_ok = 0;
                break;
            }
        }
        
        if (pattern_ok) {
            FORENSIC_LOG("✅ TEST SECONDAIRE RÉUSSI: get_global_id() fonctionnel");
        } else {
            FORENSIC_LOG("⚠️  TEST SECONDAIRE ÉCHOUÉ: get_global_id() incorrect");
        }
    } else {
        FORENSIC_LOG("❌ TEST ABSOLU ÉCHOUÉ: output[0] != 0x12345678");
        FORENSIC_LOG("🔍 Dump premiers 16 uint32_t:");
        for (int i = 0; i < 16; i++) {
            FORENSIC_LOG("   output[%d] = 0x%08X", i, output[i]);
        }
    }
    
    // Cleanup
    clReleaseMemObject(output_buf);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    free(source);
    
    if (test_passed) {
        FORENSIC_LOG("✅ TEST C427 #1 RÉUSSI - Kernel fonctionne avec OpenCL");
        FORENSIC_LOG("📝 CONCLUSION: Problème dans infrastructure i915 native, PAS dans kernel");
        return 0;
    } else {
        FORENSIC_LOG("❌ TEST C427 #1 ÉCHOUÉ - Kernel ne fonctionne pas");
        FORENSIC_LOG("📝 CONCLUSION: Problème dans le kernel lui-même");
        return 1;
    }
}

// Made with Bob
