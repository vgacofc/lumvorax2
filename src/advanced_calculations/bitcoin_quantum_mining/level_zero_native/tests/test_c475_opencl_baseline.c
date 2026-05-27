#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define FORENSIC_LOG(fmt, ...) do { \
    struct timespec ts; \
    clock_gettime(CLOCK_MONOTONIC, &ts); \
    fprintf(stderr, "[%ld.%09ld] " fmt "\n", ts.tv_sec, ts.tv_nsec, ##__VA_ARGS__); \
} while(0)

int main() {
    FORENSIC_LOG("🚀 TEST C475 BASELINE - Validation kernel minimal avec OpenCL");
    
    // Lire kernel source
    FILE* f = fopen("tests/test_c475_minimal_compute.cl", "r");
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* source = malloc(size + 1);
    fread(source, 1, size, f);
    source[size] = 0;
    fclose(f);
    
    // Setup OpenCL
    cl_platform_id platform;
    clGetPlatformIDs(1, &platform, NULL);
    cl_device_id device;
    clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    cl_context ctx = clCreateContext(NULL, 1, &device, NULL, NULL, NULL);
    cl_command_queue queue = clCreateCommandQueue(ctx, device, 0, NULL);
    
    // Compiler kernel
    cl_program prog = clCreateProgramWithSource(ctx, 1, (const char**)&source, &size, NULL);
    if (clBuildProgram(prog, 1, &device, NULL, NULL, NULL) != CL_SUCCESS) {
        FORENSIC_LOG("❌ Build failed");
        return 1;
    }
    FORENSIC_LOG("✅ Kernel compilé");
    
    // Créer kernel
    cl_kernel kernel = clCreateKernel(prog, "minimal_write", NULL);
    
    // Créer buffer output (4 uint32)
    cl_mem output_buf = clCreateBuffer(ctx, CL_MEM_WRITE_ONLY, 16, NULL, NULL);
    
    // Initialiser buffer à 0
    unsigned int zeros[4] = {0, 0, 0, 0};
    clEnqueueWriteBuffer(queue, output_buf, CL_TRUE, 0, 16, zeros, 0, NULL, NULL);
    FORENSIC_LOG("✅ Buffer output créé et initialisé à 0");
    
    // Set kernel arg
    clSetKernelArg(kernel, 0, sizeof(cl_mem), &output_buf);
    
    // Exécuter kernel (1 work-item)
    size_t global_size = 1;
    FORENSIC_LOG("⚡ Exécution kernel...");
    clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_size, NULL, 0, NULL, NULL);
    clFinish(queue);
    FORENSIC_LOG("✅ Kernel exécuté");
    
    // Lire résultat
    unsigned int output[4];
    clEnqueueReadBuffer(queue, output_buf, CL_TRUE, 0, 16, output, 0, NULL, NULL);
    
    // Vérifier résultat
    FORENSIC_LOG("📊 RÉSULTATS:");
    FORENSIC_LOG("  output[0] = 0x%08X (attendu: 0xDEADBEEF)", output[0]);
    FORENSIC_LOG("  output[1] = 0x%08X (attendu: 0xCAFEBABE)", output[1]);
    FORENSIC_LOG("  output[2] = 0x%08X (attendu: 0x12345678)", output[2]);
    FORENSIC_LOG("  output[3] = 0x%08X (attendu: 0xABCDEF00)", output[3]);
    
    int success = (output[0] == 0xDEADBEEF && 
                   output[1] == 0xCAFEBABE &&
                   output[2] == 0x12345678 &&
                   output[3] == 0xABCDEF00);
    
    if (success) {
        FORENSIC_LOG("🎉 TEST RÉUSSI - Kernel minimal fonctionne avec OpenCL");
    } else {
        FORENSIC_LOG("❌ TEST ÉCHOUÉ - Valeurs incorrectes");
    }
    
    // Cleanup
    clReleaseMemObject(output_buf);
    clReleaseKernel(kernel);
    clReleaseProgram(prog);
    clReleaseCommandQueue(queue);
    clReleaseContext(ctx);
    free(source);
    
    return success ? 0 : 1;
}
