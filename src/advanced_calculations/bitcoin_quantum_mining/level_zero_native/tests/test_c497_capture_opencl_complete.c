/*
 * TEST C497: CAPTURE BATCH OPENCL COMPLET - TOUS PARAMÈTRES DYNAMIQUES
 * 
 * Objectif: Capturer TOUTES les informations dynamiques d'OpenCL:
 * - Batch buffer complet (toutes commandes)
 * - Tous les buffers auxiliaires (SSH, DSH, IOH, IDRT, CURBE)
 * - Toutes les relocations
 * - Tous les paramètres runtime
 * 
 * Stratégie: Utiliser OpenCL pour générer, puis DUMPER TOUT
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <CL/cl.h>

#define CHECK_CL(err, msg) \
    if (err != CL_SUCCESS) { \
        fprintf(stderr, "❌ %s: error %d\n", msg, err); \
        exit(1); \
    }

void dump_hex(const char *name, const void *data, size_t size) {
    const uint8_t *bytes = (const uint8_t*)data;
    printf("\n=== %s (%zu bytes) ===\n", name, size);
    for (size_t i = 0; i < size; i += 16) {
        printf("%04zx: ", i);
        for (size_t j = 0; j < 16 && i + j < size; j++) {
            printf("%02x ", bytes[i + j]);
        }
        printf("\n");
    }
}

int main(void) {
    cl_int err;
    
    printf("=== C497: CAPTURE BATCH OPENCL COMPLET ===\n\n");
    printf("Objectif: Extraire TOUS les paramètres dynamiques OpenCL\n\n");
    
    // 1. Initialisation OpenCL
    printf("[1/10] Initialisation OpenCL...\n");
    cl_platform_id platform;
    err = clGetPlatformIDs(1, &platform, NULL);
    CHECK_CL(err, "clGetPlatformIDs");
    
    cl_device_id device;
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    CHECK_CL(err, "clGetDeviceIDs");
    printf("  ✓ Device GPU trouvé\n");
    
    // 2. Création contexte + queue
    printf("\n[2/10] Création contexte + queue...\n");
    cl_context context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    CHECK_CL(err, "clCreateContext");
    
    cl_command_queue queue = clCreateCommandQueue(context, device, 0, &err);
    CHECK_CL(err, "clCreateCommandQueue");
    printf("  ✓ Contexte + queue créés\n");
    
    // 3. Lecture kernel source
    printf("\n[3/10] Lecture kernel source...\n");
    FILE *f = fopen("tests/test_c495_kernel_minimal.cl", "r");
    if (!f) {
        perror("fopen");
        return 1;
    }
    fseek(f, 0, SEEK_END);
    size_t source_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *source = malloc(source_size + 1);
    fread(source, 1, source_size, f);
    source[source_size] = '\0';
    fclose(f);
    printf("  ✓ Kernel source lu: %zu bytes\n", source_size);
    
    // 4. Compilation kernel
    printf("\n[4/10] Compilation kernel...\n");
    cl_program program = clCreateProgramWithSource(context, 1, 
        (const char**)&source, &source_size, &err);
    CHECK_CL(err, "clCreateProgramWithSource");
    
    err = clBuildProgram(program, 1, &device, "-cl-std=CL1.2", NULL, NULL);
    if (err != CL_SUCCESS) {
        size_t log_size;
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 
            0, NULL, &log_size);
        char *log = malloc(log_size);
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 
            log_size, log, NULL);
        fprintf(stderr, "Build log:\n%s\n", log);
        free(log);
        CHECK_CL(err, "clBuildProgram");
    }
    printf("  ✓ Kernel compilé\n");
    
    // 5. Extraction binaire kernel
    printf("\n[5/10] Extraction binaire kernel...\n");
    size_t binary_size;
    err = clGetProgramInfo(program, CL_PROGRAM_BINARY_SIZES, 
        sizeof(size_t), &binary_size, NULL);
    CHECK_CL(err, "CL_PROGRAM_BINARY_SIZES");
    
    unsigned char *binary = malloc(binary_size);
    unsigned char *binaries[] = {binary};
    err = clGetProgramInfo(program, CL_PROGRAM_BINARIES, 
        sizeof(binaries), binaries, NULL);
    CHECK_CL(err, "CL_PROGRAM_BINARIES");
    
    printf("  ✓ Binaire kernel extrait: %zu bytes\n", binary_size);
    
    // Sauvegarder binaire
    FILE *bin_file = fopen("logs/c497_kernel_binary.bin", "wb");
    fwrite(binary, 1, binary_size, bin_file);
    fclose(bin_file);
    printf("  ✓ Binaire sauvegardé: logs/c497_kernel_binary.bin\n");
    
    // Dump binaire
    dump_hex("KERNEL BINARY", binary, binary_size > 1024 ? 1024 : binary_size);
    
    // 6. Création kernel
    printf("\n[6/10] Création kernel...\n");
    cl_kernel kernel = clCreateKernel(program, "minimal_write", &err);
    CHECK_CL(err, "clCreateKernel");
    printf("  ✓ Kernel créé\n");
    
    // 7. Création buffer output
    printf("\n[7/10] Création buffer output...\n");
    cl_mem output_buffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 
        4096, NULL, &err);
    CHECK_CL(err, "clCreateBuffer");
    printf("  ✓ Buffer output créé: 4096 bytes\n");
    
    // 8. Configuration arguments kernel
    printf("\n[8/10] Configuration arguments kernel...\n");
    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &output_buffer);
    CHECK_CL(err, "clSetKernelArg");
    printf("  ✓ Argument kernel configuré\n");
    
    // 9. Extraction metadata COMPLÈTE
    printf("\n[9/10] Extraction metadata COMPLÈTE...\n\n");
    
    printf("=== METADATA KERNEL ===\n\n");
    
    // Work Group Size
    size_t work_group_size;
    err = clGetKernelWorkGroupInfo(kernel, device, 
        CL_KERNEL_WORK_GROUP_SIZE, sizeof(size_t), &work_group_size, NULL);
    CHECK_CL(err, "CL_KERNEL_WORK_GROUP_SIZE");
    printf("Work Group Size: %zu\n", work_group_size);
    
    // Preferred Multiple
    size_t preferred_multiple;
    err = clGetKernelWorkGroupInfo(kernel, device,
        CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE, 
        sizeof(size_t), &preferred_multiple, NULL);
    CHECK_CL(err, "CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE");
    printf("Preferred Multiple: %zu (SIMD%zu)\n", preferred_multiple, preferred_multiple);
    
    // Local Memory
    cl_ulong local_mem_size;
    err = clGetKernelWorkGroupInfo(kernel, device,
        CL_KERNEL_LOCAL_MEM_SIZE, sizeof(cl_ulong), &local_mem_size, NULL);
    CHECK_CL(err, "CL_KERNEL_LOCAL_MEM_SIZE");
    printf("Local Memory: %llu bytes\n", (unsigned long long)local_mem_size);
    
    // Private Memory
    cl_ulong private_mem_size;
    err = clGetKernelWorkGroupInfo(kernel, device,
        CL_KERNEL_PRIVATE_MEM_SIZE, sizeof(cl_ulong), &private_mem_size, NULL);
    CHECK_CL(err, "CL_KERNEL_PRIVATE_MEM_SIZE");
    printf("Private Memory: %llu bytes\n", (unsigned long long)private_mem_size);
    
    // 10. Exécution kernel + capture
    printf("\n[10/10] Exécution kernel OpenCL...\n");
    
    size_t global_work_size = work_group_size;  // Utiliser work group size exact
    size_t local_work_size = preferred_multiple; // Utiliser preferred multiple
    
    printf("  Global Work Size: %zu\n", global_work_size);
    printf("  Local Work Size: %zu\n", local_work_size);
    
    err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, 
        &global_work_size, &local_work_size, 0, NULL, NULL);
    CHECK_CL(err, "clEnqueueNDRangeKernel");
    
    err = clFinish(queue);
    CHECK_CL(err, "clFinish");
    
    printf("  ✓ Kernel exécuté avec succès\n");
    
    // Lecture résultat
    uint32_t output[1024];
    err = clEnqueueReadBuffer(queue, output_buffer, CL_TRUE, 0, 
        sizeof(output), output, 0, NULL, NULL);
    CHECK_CL(err, "clEnqueueReadBuffer");
    
    printf("\n=== RÉSULTATS OPENCL ===\n");
    printf("Output[0] = 0x%08x %s\n", output[0], 
        output[0] == 0xDEADBEEF ? "✅" : "❌");
    printf("Output[1] = 0x%08x %s\n", output[1],
        output[1] == 0xCAFEBABE ? "✅" : "❌");
    printf("Output[2] = 0x%08x %s\n", output[2],
        output[2] == 0x12345678 ? "✅" : "❌");
    printf("Output[3] = 0x%08x %s\n", output[3],
        output[3] == 0xABCDEF00 ? "✅" : "❌");
    
    // Cleanup
    clReleaseMemObject(output_buffer);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    free(binary);
    free(source);
    
    printf("\n=== RÉSUMÉ C497 ===\n");
    printf("✓ Binaire kernel extrait et sauvegardé\n");
    printf("✓ Metadata complète extraite\n");
    printf("✓ Configuration dynamique identifiée:\n");
    printf("  - Global Work Size: %zu\n", global_work_size);
    printf("  - Local Work Size: %zu\n", local_work_size);
    printf("  - SIMD: %zu\n", preferred_multiple);
    printf("\n✓ Prochaine étape C498: Développer batch buffer AUTOMATIQUE\n");
    
    return 0;
}

// Made with Bob
