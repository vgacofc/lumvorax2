// TEST C532 - Compiler kernel OpenCL et extraire ISA pour comparaison
#define CL_TARGET_OPENCL_VERSION 120
#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    printf("=== TEST C532 - COMPILATION OPENCL + EXTRACTION ISA ===\n\n");
    
    // Lire le kernel source
    FILE *f = fopen("tests/test_c532_opencl_compare.cl", "r");
    if (!f) {
        perror("fopen kernel");
        return 1;
    }
    
    fseek(f, 0, SEEK_END);
    size_t kernel_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char *kernel_source = malloc(kernel_size + 1);
    fread(kernel_source, 1, kernel_size, f);
    kernel_source[kernel_size] = '\0';
    fclose(f);
    
    printf("Kernel source chargé: %zu bytes\n\n", kernel_size);
    
    // Initialiser OpenCL
    cl_platform_id platform;
    clGetPlatformIDs(1, &platform, NULL);
    
    cl_device_id device;
    clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    
    cl_context context = clCreateContext(NULL, 1, &device, NULL, NULL, NULL);
    cl_command_queue queue = clCreateCommandQueue(context, device, 0, NULL);
    
    // Compiler le kernel
    printf("Compilation du kernel...\n");
    cl_program program = clCreateProgramWithSource(context, 1, (const char **)&kernel_source, &kernel_size, NULL);
    
    cl_int err = clBuildProgram(program, 1, &device, "-save-temps=/tmp/opencl_build", NULL, NULL);
    
    if (err != CL_SUCCESS) {
        printf("❌ Erreur compilation: %d\n", err);
        
        size_t log_size;
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        char *log = malloc(log_size);
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
        printf("Build log:\n%s\n", log);
        free(log);
        return 1;
    }
    
    printf("✓ Compilation réussie\n\n");
    
    // Extraire le binaire
    size_t binary_size;
    clGetProgramInfo(program, CL_PROGRAM_BINARY_SIZES, sizeof(size_t), &binary_size, NULL);
    
    printf("Taille binaire: %zu bytes\n", binary_size);
    
    unsigned char *binary = malloc(binary_size);
    unsigned char *binaries[] = {binary};
    clGetProgramInfo(program, CL_PROGRAM_BINARIES, sizeof(unsigned char *), binaries, NULL);
    
    // Sauvegarder le binaire
    FILE *out = fopen("/tmp/opencl_kernel_c532.bin", "wb");
    fwrite(binary, 1, binary_size, out);
    fclose(out);
    
    printf("✓ Binaire sauvegardé: /tmp/opencl_kernel_c532.bin\n\n");
    
    // Afficher les premiers bytes
    printf("Premiers 128 bytes du binaire:\n");
    for (size_t i = 0; i < 128 && i < binary_size; i += 16) {
        printf("  %04zx: ", i);
        for (size_t j = 0; j < 16 && i + j < binary_size; j++) {
            printf("%02x ", binary[i + j]);
        }
        printf("\n");
    }
    
    // Tester l'exécution
    printf("\n=== TEST EXÉCUTION ===\n");
    
    cl_kernel kernel = clCreateKernel(program, "minimal_write", NULL);
    
    cl_mem output_buf = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 4096, NULL, NULL);
    
    // Initialiser output
    uint32_t init_val = 0xFFFFFFFF;
    clEnqueueFillBuffer(queue, output_buf, &init_val, sizeof(uint32_t), 0, 4096, 0, NULL, NULL);
    
    clSetKernelArg(kernel, 0, sizeof(cl_mem), &output_buf);
    
    size_t global_work_size = 1;
    clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_work_size, NULL, 0, NULL, NULL);
    
    uint32_t result[4] = {0};
    clEnqueueReadBuffer(queue, output_buf, CL_TRUE, 0, sizeof(result), result, 0, NULL, NULL);
    
    printf("Résultat OpenCL:\n");
    printf("  output[0] = 0x%08x\n", result[0]);
    printf("  output[1] = 0x%08x\n", result[1]);
    
    if (result[0] == 0x12345678) {
        printf("\n✓✓✓ OpenCL fonctionne correctement!\n");
    } else {
        printf("\n❌ OpenCL a échoué\n");
    }
    
    // Cleanup
    clReleaseMemObject(output_buf);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    free(kernel_source);
    free(binary);
    
    return 0;
}

// Made with Bob
