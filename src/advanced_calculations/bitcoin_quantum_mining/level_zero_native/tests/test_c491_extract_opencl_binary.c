/*
 * LUMVORAX C491 - Extraction Binaire ISA Gen9 depuis OpenCL
 * 
 * Objectif: Extraire le kernel ISA Gen9 exact compilé par OpenCL
 *           pour l'utiliser dans notre infrastructure i915 native
 * 
 * Méthode: clGetProgramInfo(CL_PROGRAM_BINARIES) après compilation
 */

#define CL_TARGET_OPENCL_VERSION 120
#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Kernel minimal identique à celui qui fonctionne en OpenCL
const char *kernel_source = 
"__kernel void minimal_write(__global uint *output) {\n"
"    output[0] = 0xDEADBEEF;\n"
"    output[1] = 0xCAFEBABE;\n"
"    output[2] = 0x12345678;\n"
"    output[3] = 0xABCDEF00;\n"
"}\n";

void print_hex_dump(const unsigned char *data, size_t size, const char *filename) {
    FILE *f = fopen(filename, "w");
    if (!f) {
        perror("fopen");
        return;
    }
    
    fprintf(f, "// ISA Gen9 extrait depuis OpenCL - %zu bytes\n", size);
    fprintf(f, "// Date: %s\n", __DATE__);
    fprintf(f, "const unsigned char kernel_isa_gen9[] = {\n");
    
    for (size_t i = 0; i < size; i++) {
        if (i % 16 == 0) {
            fprintf(f, "    ");
        }
        fprintf(f, "0x%02x", data[i]);
        if (i < size - 1) {
            fprintf(f, ", ");
        }
        if ((i + 1) % 16 == 0) {
            fprintf(f, "\n");
        }
    }
    
    if (size % 16 != 0) {
        fprintf(f, "\n");
    }
    
    fprintf(f, "};\n");
    fprintf(f, "const size_t kernel_isa_gen9_size = %zu;\n", size);
    fclose(f);
    
    printf("✓ Kernel ISA Gen9 sauvegardé dans %s (%zu bytes)\n", filename, size);
}

void print_binary_dump(const unsigned char *data, size_t size, const char *filename) {
    FILE *f = fopen(filename, "wb");
    if (!f) {
        perror("fopen");
        return;
    }
    
    fwrite(data, 1, size, f);
    fclose(f);
    
    printf("✓ Binaire brut sauvegardé dans %s (%zu bytes)\n", filename, size);
}

int main() {
    printf("=== LUMVORAX C491 - Extraction ISA Gen9 depuis OpenCL ===\n\n");
    
    cl_int err;
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_program program;
    
    // 1. Obtenir plateforme et device
    err = clGetPlatformIDs(1, &platform, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "❌ clGetPlatformIDs failed: %d\n", err);
        return 1;
    }
    
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "❌ clGetDeviceIDs failed: %d\n", err);
        return 1;
    }
    
    // Afficher info device
    char device_name[256];
    clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(device_name), device_name, NULL);
    printf("✓ Device: %s\n", device_name);
    
    // 2. Créer contexte
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "❌ clCreateContext failed: %d\n", err);
        return 1;
    }
    printf("✓ Context créé\n");
    
    // 3. Créer et compiler programme
    program = clCreateProgramWithSource(context, 1, &kernel_source, NULL, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "❌ clCreateProgramWithSource failed: %d\n", err);
        return 1;
    }
    printf("✓ Programme créé\n");
    
    err = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "❌ clBuildProgram failed: %d\n", err);
        
        // Afficher log de compilation
        size_t log_size;
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        char *log = malloc(log_size);
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
        fprintf(stderr, "Build log:\n%s\n", log);
        free(log);
        return 1;
    }
    printf("✓ Programme compilé\n\n");
    
    // 4. EXTRACTION DU BINAIRE ISA Gen9
    printf("=== EXTRACTION BINAIRE ISA Gen9 ===\n\n");
    
    // Obtenir taille du binaire
    size_t binary_size;
    err = clGetProgramInfo(program, CL_PROGRAM_BINARY_SIZES, sizeof(size_t), &binary_size, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "❌ clGetProgramInfo(BINARY_SIZES) failed: %d\n", err);
        return 1;
    }
    printf("✓ Taille binaire: %zu bytes\n", binary_size);
    
    // Allouer buffer pour le binaire
    unsigned char *binary = malloc(binary_size);
    if (!binary) {
        fprintf(stderr, "❌ malloc failed\n");
        return 1;
    }
    
    // Extraire le binaire
    unsigned char *binaries[] = { binary };
    err = clGetProgramInfo(program, CL_PROGRAM_BINARIES, sizeof(unsigned char*), binaries, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "❌ clGetProgramInfo(BINARIES) failed: %d\n", err);
        free(binary);
        return 1;
    }
    printf("✓ Binaire extrait\n\n");
    
    // 5. ANALYSE DU BINAIRE
    printf("=== ANALYSE BINAIRE ===\n\n");
    
    // Afficher premiers 64 bytes en hexadécimal
    printf("Premiers 64 bytes:\n");
    for (size_t i = 0; i < 64 && i < binary_size; i++) {
        if (i % 16 == 0) printf("%04zx: ", i);
        printf("%02x ", binary[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }
    printf("\n\n");
    
    // Rechercher signature Intel Gen
    printf("Recherche signatures Intel Gen...\n");
    for (size_t i = 0; i < binary_size - 4; i++) {
        // Rechercher patterns communs
        if (binary[i] == 0x00 && binary[i+1] == 0x00 && 
            binary[i+2] == 0x00 && binary[i+3] == 0x00) {
            // Potentiel début de section
            if (i > 0 && i < binary_size - 320) {
                printf("  Offset 0x%04zx: Potentiel début section (4 zeros)\n", i);
            }
        }
    }
    printf("\n");
    
    // 6. SAUVEGARDER BINAIRE
    printf("=== SAUVEGARDE BINAIRE ===\n\n");
    
    // Sauvegarder en format C header
    print_hex_dump(binary, binary_size, 
                   "tests/test_c491_opencl_kernel.h");
    
    // Sauvegarder binaire brut
    print_binary_dump(binary, binary_size,
                      "tests/test_c491_opencl_kernel.bin");
    
    // Sauvegarder hexdump lisible
    char cmd[512];
    snprintf(cmd, sizeof(cmd), 
             "hexdump -C tests/test_c491_opencl_kernel.bin > tests/test_c491_opencl_kernel.hex");
    system(cmd);
    printf("✓ Hexdump sauvegardé dans tests/test_c491_opencl_kernel.hex\n\n");
    
    // 7. COMPARAISON AVEC KERNEL ACTUEL
    printf("=== COMPARAISON ===\n\n");
    printf("Kernel actuel (ocloc):  320 bytes\n");
    printf("Kernel OpenCL extrait:  %zu bytes\n", binary_size);
    printf("Différence:             %+zd bytes\n\n", (ssize_t)binary_size - 320);
    
    if (binary_size != 320) {
        printf("⚠️  TAILLES DIFFÉRENTES!\n");
        printf("    Le kernel OpenCL contient probablement des métadonnées\n");
        printf("    ou un format d'encapsulation différent.\n\n");
        printf("    Analyse du binaire nécessaire pour extraire l'ISA pur.\n");
    } else {
        printf("✓ Tailles identiques - Comparaison byte-à-byte possible\n");
    }
    
    // Cleanup
    free(binary);
    clReleaseProgram(program);
    clReleaseContext(context);
    
    printf("\n=== EXTRACTION TERMINÉE ===\n");
    printf("\nFichiers générés:\n");
    printf("  - tests/test_c491_opencl_kernel.h   (C header)\n");
    printf("  - tests/test_c491_opencl_kernel.bin (binaire brut)\n");
    printf("  - tests/test_c491_opencl_kernel.hex (hexdump)\n");
    
    return 0;
}

// Made with Bob
