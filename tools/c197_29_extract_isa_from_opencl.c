/*
 * C197.29 - Extraction ISA Réel depuis OpenCL
 * 
 * OBJECTIF: Extraire ISA Gen9 compilé depuis cache IGC OpenCL
 * 
 * STRATÉGIE:
 * 1. Compiler kernel OpenCL simple (write test)
 * 2. Intercepter sortie IGC (cache ~/.cache/intel-opencl/)
 * 3. Extraire binary ISA Gen9
 * 4. Sauvegarder pour réutilisation
 * 
 * KERNEL CIBLE:
 * __kernel void write_test(__global uint* output) {
 *     output[0] = 0x12345678;
 * }
 */

#define CL_TARGET_OPENCL_VERSION 120
#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>
#include <unistd.h>

// Kernel OpenCL source
const char* kernel_source = 
"__kernel void write_test(__global uint* output) {\n"
"    output[0] = 0x12345678;\n"
"}\n";

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

void save_binary_to_file(const char* filename, const void* data, size_t size) {
    FILE* f = fopen(filename, "wb");
    if (!f) {
        perror("fopen");
        return;
    }
    fwrite(data, 1, size, f);
    fclose(f);
    printf("✅ Binary sauvegardé: %s (%zu bytes)\n", filename, size);
}

void dump_hex(const uint8_t* data, size_t size, size_t max_lines) {
    for (size_t i = 0; i < size && i < max_lines * 16; i++) {
        if (i % 16 == 0) printf("\n  [%04zx] ", i);
        printf("%02x ", data[i]);
    }
    printf("\n");
}

// ============================================================================
// MAIN PROGRAM
// ============================================================================

int main() {
    printf("=== C197.29 - Extraction ISA depuis OpenCL ===\n\n");
    
    cl_int err;
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;
    cl_program program;
    cl_kernel kernel;
    
    // 1. Get platform
    printf("[1/10] Récupération platform OpenCL...\n");
    err = clGetPlatformIDs(1, &platform, NULL);
    if (err != CL_SUCCESS) {
        printf("❌ clGetPlatformIDs failed: %d\n", err);
        return 1;
    }
    
    char platform_name[256];
    clGetPlatformInfo(platform, CL_PLATFORM_NAME, sizeof(platform_name), platform_name, NULL);
    printf("✅ Platform: %s\n", platform_name);
    
    // 2. Get device
    printf("\n[2/10] Récupération device GPU...\n");
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    if (err != CL_SUCCESS) {
        printf("❌ clGetDeviceIDs failed: %d\n", err);
        return 1;
    }
    
    char device_name[256];
    clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(device_name), device_name, NULL);
    printf("✅ Device: %s\n", device_name);
    
    // 3. Create context
    printf("\n[3/10] Création context...\n");
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    if (err != CL_SUCCESS) {
        printf("❌ clCreateContext failed: %d\n", err);
        return 1;
    }
    printf("✅ Context créé\n");
    
    // 4. Create command queue
    printf("\n[4/10] Création command queue...\n");
    queue = clCreateCommandQueue(context, device, 0, &err);
    if (err != CL_SUCCESS) {
        printf("❌ clCreateCommandQueue failed: %d\n", err);
        return 1;
    }
    printf("✅ Command queue créée\n");
    
    // 5. Create program from source
    printf("\n[5/10] Création program depuis source...\n");
    program = clCreateProgramWithSource(context, 1, &kernel_source, NULL, &err);
    if (err != CL_SUCCESS) {
        printf("❌ clCreateProgramWithSource failed: %d\n", err);
        return 1;
    }
    printf("✅ Program créé\n");
    
    // 6. Build program (IGC compilation)
    printf("\n[6/10] Compilation program (IGC)...\n");
    err = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    if (err != CL_SUCCESS) {
        printf("❌ clBuildProgram failed: %d\n", err);
        
        // Get build log
        size_t log_size;
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        char* log = malloc(log_size);
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
        printf("Build log:\n%s\n", log);
        free(log);
        return 1;
    }
    printf("✅ Program compilé (IGC)\n");
    
    // 7. Get binary size
    printf("\n[7/10] Récupération taille binary...\n");
    size_t binary_size;
    err = clGetProgramInfo(program, CL_PROGRAM_BINARY_SIZES, sizeof(size_t), &binary_size, NULL);
    if (err != CL_SUCCESS) {
        printf("❌ clGetProgramInfo (BINARY_SIZES) failed: %d\n", err);
        return 1;
    }
    printf("✅ Binary size: %zu bytes\n", binary_size);
    
    // 8. Get binary
    printf("\n[8/10] Extraction binary ISA...\n");
    uint8_t* binary = malloc(binary_size);
    uint8_t* binaries[] = { binary };
    err = clGetProgramInfo(program, CL_PROGRAM_BINARIES, sizeof(binaries), binaries, NULL);
    if (err != CL_SUCCESS) {
        printf("❌ clGetProgramInfo (BINARIES) failed: %d\n", err);
        free(binary);
        return 1;
    }
    printf("✅ Binary ISA extrait\n");
    
    // 9. Analyze binary
    printf("\n[9/10] Analyse binary...\n");
    printf("Binary header (premiers 256 bytes):");
    dump_hex(binary, binary_size, 16);
    
    // Check for ELF header
    if (binary_size >= 4 && binary[0] == 0x7F && binary[1] == 'E' && 
        binary[2] == 'L' && binary[3] == 'F') {
        printf("\n✅ Format: ELF binary\n");
    } else {
        printf("\n⚠️  Format: Non-ELF (raw ISA ou autre)\n");
    }
    
    // 10. Save binary
    printf("\n[10/10] Sauvegarde binary...\n");
    save_binary_to_file("/tmp/gen9_kernel_write_test.bin", binary, binary_size);
    
    // Also save as C array
    FILE* f = fopen("/tmp/gen9_kernel_write_test.h", "w");
    if (f) {
        fprintf(f, "// Gen9 ISA - Kernel write_test\n");
        fprintf(f, "// Extrait depuis OpenCL/IGC\n");
        fprintf(f, "// Size: %zu bytes\n\n", binary_size);
        fprintf(f, "uint8_t gen9_kernel_write_test[] = {\n");
        for (size_t i = 0; i < binary_size; i++) {
            if (i % 16 == 0) fprintf(f, "    ");
            fprintf(f, "0x%02x", binary[i]);
            if (i < binary_size - 1) fprintf(f, ",");
            if (i % 16 == 15) fprintf(f, "\n");
            else if (i < binary_size - 1) fprintf(f, " ");
        }
        fprintf(f, "\n};\n");
        fclose(f);
        printf("✅ C array sauvegardé: /tmp/gen9_kernel_write_test.h\n");
    }
    
    printf("\n=== RÉSULTATS ===\n");
    printf("✅ ISA Gen9 extrait avec succès\n");
    printf("📊 Taille: %zu bytes\n", binary_size);
    printf("📁 Fichiers:\n");
    printf("   - /tmp/gen9_kernel_write_test.bin (binary brut)\n");
    printf("   - /tmp/gen9_kernel_write_test.h (C array)\n");
    
    printf("\n=== ANALYSE CACHE IGC ===\n");
    printf("Cache IGC typiquement dans:\n");
    printf("  ~/.cache/intel-opencl/\n");
    printf("  ~/.cache/mesa_shader_cache/\n");
    
    // Check cache directories
    char cache_path[512];
    snprintf(cache_path, sizeof(cache_path), "%s/.cache/intel-opencl", getenv("HOME"));
    struct stat st;
    if (stat(cache_path, &st) == 0) {
        printf("✅ Cache Intel OpenCL trouvé: %s\n", cache_path);
    } else {
        printf("⚠️  Cache Intel OpenCL non trouvé\n");
    }
    
    printf("\n=== PROCHAINES ÉTAPES ===\n");
    printf("C197.30: Intégrer ISA dans command stream C197.28\n");
    printf("C197.31: Tester exécution GPU complète\n");
    printf("C197.32: Valider output (0x12345678)\n");
    
    // Cleanup
    free(binary);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    
    return 0;
}

// Made with Bob
