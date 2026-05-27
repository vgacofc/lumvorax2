/*
 * C490: Extraction Kernel ISA Gen9 EXACT de Beignet OpenCL
 * Objectif: Obtenir le bytecode Gen9 qui fonctionne parfaitement
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <CL/cl.h>

// Kernel OpenCL qui fonctionne
const char* kernel_source = 
"__kernel void minimal_write(__global uint* output) {\n"
"    output[0] = 0xDEADBEEF;\n"
"    output[1] = 0xCAFEBABE;\n"
"    output[2] = 0x12345678;\n"
"    output[3] = 0xABCDEF00;\n"
"}\n";

int main(void)
{
    printf("=== C490: EXTRACTION KERNEL BEIGNET ===\n\n");
    
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;
    cl_program program;
    cl_kernel kernel;
    cl_int err;
    
    // 1. Obtenir platform
    err = clGetPlatformIDs(1, &platform, NULL);
    if (err != CL_SUCCESS) {
        printf("❌ Erreur clGetPlatformIDs: %d\n", err);
        return 1;
    }
    printf("✓ Platform obtenue\n");
    
    // 2. Obtenir device GPU
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    if (err != CL_SUCCESS) {
        printf("❌ Erreur clGetDeviceIDs: %d\n", err);
        return 1;
    }
    printf("✓ Device GPU obtenu\n");
    
    // 3. Créer context
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    if (err != CL_SUCCESS) {
        printf("❌ Erreur clCreateContext: %d\n", err);
        return 1;
    }
    printf("✓ Context créé\n");
    
    // 4. Créer command queue
    queue = clCreateCommandQueue(context, device, 0, &err);
    if (err != CL_SUCCESS) {
        printf("❌ Erreur clCreateCommandQueue: %d\n", err);
        return 1;
    }
    printf("✓ Command queue créée\n");
    
    // 5. Créer program
    program = clCreateProgramWithSource(context, 1, &kernel_source, NULL, &err);
    if (err != CL_SUCCESS) {
        printf("❌ Erreur clCreateProgramWithSource: %d\n", err);
        return 1;
    }
    printf("✓ Program créé\n");
    
    // 6. Compiler program
    err = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    if (err != CL_SUCCESS) {
        printf("❌ Erreur clBuildProgram: %d\n", err);
        
        // Afficher log de compilation
        size_t log_size;
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        char* log = malloc(log_size);
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
        printf("Build log:\n%s\n", log);
        free(log);
        return 1;
    }
    printf("✓ Program compilé\n");
    
    // 7. Extraire binaires (kernel ISA Gen9)
    size_t binary_size;
    err = clGetProgramInfo(program, CL_PROGRAM_BINARY_SIZES, sizeof(size_t), &binary_size, NULL);
    if (err != CL_SUCCESS) {
        printf("❌ Erreur clGetProgramInfo (sizes): %d\n", err);
        return 1;
    }
    printf("✓ Taille binaire: %zu bytes\n", binary_size);
    
    unsigned char* binary = malloc(binary_size);
    unsigned char** binaries = &binary;
    err = clGetProgramInfo(program, CL_PROGRAM_BINARIES, sizeof(unsigned char*), binaries, NULL);
    if (err != CL_SUCCESS) {
        printf("❌ Erreur clGetProgramInfo (binaries): %d\n", err);
        return 1;
    }
    printf("✓ Binaire extrait\n\n");
    
    // 8. Sauvegarder binaire dans fichier
    FILE* f = fopen("tests/test_c490_beignet_kernel.bin", "wb");
    if (!f) {
        printf("❌ Erreur ouverture fichier\n");
        return 1;
    }
    fwrite(binary, 1, binary_size, f);
    fclose(f);
    printf("✓ Binaire sauvegardé: tests/test_c490_beignet_kernel.bin\n");
    
    // 9. Afficher dump hex du binaire
    printf("\n=== DUMP HEX BINAIRE (premiers 512 bytes) ===\n");
    size_t dump_size = (binary_size < 512) ? binary_size : 512;
    for (size_t i = 0; i < dump_size; i++) {
        if (i % 16 == 0) printf("%04zx: ", i);
        printf("%02x ", binary[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }
    printf("\n");
    
    // 10. Chercher section .text (kernel ISA Gen9 pur)
    printf("\n=== RECHERCHE SECTION .text ===\n");
    
    // Format ELF: chercher magic bytes Gen9
    // Les kernels Gen9 commencent souvent par des patterns spécifiques
    for (size_t i = 0; i < binary_size - 16; i++) {
        // Chercher pattern typique Gen9: send instructions (0x01, 0x00, 0x60, 0x00)
        if (binary[i] == 0x01 && binary[i+1] == 0x00 && 
            binary[i+2] == 0x60 && binary[i+3] == 0x00) {
            printf("✓ Pattern Gen9 trouvé à offset 0x%04zx\n", i);
            printf("  Dump 64 bytes:\n  ");
            for (size_t j = 0; j < 64 && (i+j) < binary_size; j++) {
                printf("%02x ", binary[i+j]);
                if ((j + 1) % 16 == 0) printf("\n  ");
            }
            printf("\n");
        }
    }
    
    // Cleanup
    free(binary);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    
    printf("\n=== SUCCÈS C490 ===\n");
    printf("Binaire Beignet extrait avec succès!\n");
    printf("Prochaine étape: Analyser le binaire pour extraire section .text\n");
    
    return 0;
}

// Made with Bob
