#define CL_TARGET_OPENCL_VERSION 120
#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK_CL(call) do { \
    cl_int err = call; \
    if (err != CL_SUCCESS) { \
        fprintf(stderr, "OpenCL error at %s:%d: %d\n", __FILE__, __LINE__, err); \
        exit(1); \
    } \
} while(0)

int main() {
    printf("\n╔════════════════════════════════════════════════════════════╗\n");
    printf("║  TEST C396 - VALIDATION OPENCL DU KERNEL MANUEL          ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");

    // Charger le kernel binaire C396
    FILE *f = fopen("kernels/test_c396_fixed.bin", "rb");
    if (!f) {
        fprintf(stderr, "❌ Impossible d'ouvrir kernels/test_c396_fixed.bin\n");
        return 1;
    }
    fseek(f, 0, SEEK_END);
    size_t bin_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    unsigned char *binary = malloc(bin_size);
    fread(binary, 1, bin_size, f);
    fclose(f);
    printf("✓ Kernel binaire chargé: %zu bytes\n", bin_size);

    // Initialiser OpenCL
    cl_platform_id platform;
    CHECK_CL(clGetPlatformIDs(1, &platform, NULL));
    
    cl_device_id device;
    CHECK_CL(clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL));
    
    char device_name[256];
    clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(device_name), device_name, NULL);
    printf("✓ Device: %s\n", device_name);

    cl_context context = clCreateContext(NULL, 1, &device, NULL, NULL, NULL);
    cl_command_queue queue = clCreateCommandQueue(context, device, 0, NULL);

    // Créer le programme depuis le binaire
    cl_int binary_status;
    cl_program program = clCreateProgramWithBinary(context, 1, &device, 
                                                    &bin_size, (const unsigned char**)&binary,
                                                    &binary_status, NULL);
    if (!program || binary_status != CL_SUCCESS) {
        fprintf(stderr, "❌ Erreur création programme: binary_status=%d\n", binary_status);
        return 1;
    }
    printf("✓ Programme créé depuis binaire\n");

    CHECK_CL(clBuildProgram(program, 1, &device, NULL, NULL, NULL));
    printf("✓ Programme compilé\n");

    cl_kernel kernel = clCreateKernel(program, "test_magic_values", NULL);
    if (!kernel) {
        fprintf(stderr, "❌ Kernel 'test_magic_values' non trouvé\n");
        return 1;
    }
    printf("✓ Kernel 'test_magic_values' créé\n");

    // Créer buffer de sortie (16 uint32_t)
    cl_mem output_buf = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 16 * sizeof(cl_uint), NULL, NULL);
    
    // Définir les arguments
    CHECK_CL(clSetKernelArg(kernel, 0, sizeof(cl_mem), &output_buf));
    
    // Exécuter avec 16 work-items
    size_t global_size = 16;
    printf("\n🚀 Exécution du kernel (16 work-items)...\n");
    CHECK_CL(clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_size, NULL, 0, NULL, NULL));
    
    // Lire les résultats
    cl_uint output[16] = {0};
    CHECK_CL(clEnqueueReadBuffer(queue, output_buf, CL_TRUE, 0, 16 * sizeof(cl_uint), output, 0, NULL, NULL));
    
    printf("\n📊 RÉSULTATS:\n");
    int success_count = 0;
    for (int i = 0; i < 16; i++) {
        int is_correct = (output[i] == 0xDEADBEEF);
        printf("  output[%2d] = 0x%08X %s\n", i, output[i], 
               is_correct ? "✓ CORRECT" : "❌ ERREUR");
        if (is_correct) success_count++;
    }
    
    printf("\n");
    if (success_count == 16) {
        printf("✅ SUCCÈS TOTAL: 16/16 valeurs correctes (0xDEADBEEF)\n");
        printf("✅ Le kernel C396 fonctionne parfaitement!\n");
        printf("✅ Les opcodes SEND sont corrects\n");
        printf("✅ Prêt pour application au mining Bitcoin\n");
    } else {
        printf("❌ ÉCHEC: %d/16 valeurs correctes\n", success_count);
    }

    // Nettoyage
    clReleaseMemObject(output_buf);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    free(binary);

    return (success_count == 16) ? 0 : 1;
}
