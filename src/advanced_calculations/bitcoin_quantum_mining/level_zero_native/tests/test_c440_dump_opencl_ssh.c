/*
 * TEST C440 - DUMP SSH OpenCL pour comparaison byte-par-byte
 * 
 * OBJECTIF: Capturer le SSH EXACT généré par OpenCL et le comparer avec notre SSH natif
 * 
 * STRATÉGIE:
 * 1. Créer kernel minimal OpenCL
 * 2. Configurer buffer output
 * 3. Appeler clSetKernelArg() pour que OpenCL génère le SSH
 * 4. DUMPER le SSH complet (Binding Table + Surface State)
 * 5. Comparer byte-par-byte avec notre SSH natif
 */

#define CL_TARGET_OPENCL_VERSION 120
#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

// Kernel minimal: écrit 0x12345678 dans output[0]
const char* kernel_source = 
"__kernel void test_write(__global uint* output) {\n"
"    if (get_global_id(0) == 0) {\n"
"        output[0] = 0x12345678;\n"
"    }\n"
"}\n";

// Fonction pour dumper mémoire en hexadécimal
void dump_hex(const char* label, const void* data, size_t size) {
    const uint8_t* bytes = (const uint8_t*)data;
    printf("\n[DUMP_%s] %zu bytes:\n", label, size);
    for (size_t i = 0; i < size; i += 16) {
        printf("  %04zx: ", i);
        for (size_t j = 0; j < 16 && i + j < size; j++) {
            printf("%02x ", bytes[i + j]);
        }
        printf("\n");
    }
}

// Fonction pour dumper en DWORDs (32-bit)
void dump_dwords(const char* label, const void* data, size_t size) {
    const uint32_t* dwords = (const uint32_t*)data;
    size_t count = size / 4;
    printf("\n[DUMP_%s] %zu DWORDs:\n", label, count);
    for (size_t i = 0; i < count; i++) {
        printf("  DW%02zu: 0x%08x", i, dwords[i]);
        
        // Annotations pour les champs connus
        if (i == 0 && label[0] == 'S') {  // Surface State DW0
            printf(" (Type, Format, VAlign, HAlign, Tile)");
        } else if (i == 1 && label[0] == 'S') {  // Surface State DW1
            printf(" (MOCS)");
        } else if (i == 2 && label[0] == 'S') {  // Surface State DW2
            printf(" (Width, Height)");
        } else if (i == 3 && label[0] == 'S') {  // Surface State DW3
            printf(" (Depth)");
        } else if (i == 6 && label[0] == 'S') {  // Surface State DW6
            printf(" (Coherency)");
        } else if (i == 8 && label[0] == 'S') {  // Surface State DW8
            printf(" (Base Address Low)");
        } else if (i == 9 && label[0] == 'S') {  // Surface State DW9
            printf(" (Base Address High)");
        } else if (label[0] == 'B') {  // Binding Table
            printf(" (BTI[%zu] → Surface State offset)", i);
        }
        
        printf("\n");
    }
}

int main() {
    printf("=== TEST C440: DUMP SSH OpenCL ===\n\n");
    
    cl_int err;
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;
    cl_program program;
    cl_kernel kernel;
    cl_mem output_buffer;
    
    // 1. Initialisation OpenCL
    err = clGetPlatformIDs(1, &platform, NULL);
    if (err != CL_SUCCESS) {
        printf("❌ clGetPlatformIDs failed: %d\n", err);
        return 1;
    }
    
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    if (err != CL_SUCCESS) {
        printf("❌ clGetDeviceIDs failed: %d\n", err);
        return 1;
    }
    
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    if (err != CL_SUCCESS) {
        printf("❌ clCreateContext failed: %d\n", err);
        return 1;
    }
    
    queue = clCreateCommandQueue(context, device, 0, &err);
    if (err != CL_SUCCESS) {
        printf("❌ clCreateCommandQueue failed: %d\n", err);
        return 1;
    }
    
    printf("✅ OpenCL initialisé\n");
    
    // 2. Compilation kernel
    program = clCreateProgramWithSource(context, 1, &kernel_source, NULL, &err);
    if (err != CL_SUCCESS) {
        printf("❌ clCreateProgramWithSource failed: %d\n", err);
        return 1;
    }
    
    err = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    if (err != CL_SUCCESS) {
        printf("❌ clBuildProgram failed: %d\n", err);
        size_t log_size;
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        char* log = (char*)malloc(log_size);
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
        printf("Build log:\n%s\n", log);
        free(log);
        return 1;
    }
    
    kernel = clCreateKernel(program, "test_write", &err);
    if (err != CL_SUCCESS) {
        printf("❌ clCreateKernel failed: %d\n", err);
        return 1;
    }
    
    printf("✅ Kernel compilé\n");
    
    // 3. Création buffer output
    size_t buffer_size = 1028;  // Même taille que notre test natif
    output_buffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, buffer_size, NULL, &err);
    if (err != CL_SUCCESS) {
        printf("❌ clCreateBuffer failed: %d\n", err);
        return 1;
    }
    
    printf("✅ Buffer créé: %zu bytes\n", buffer_size);
    
    // 4. Configuration argument kernel
    // C'EST ICI QUE OpenCL GÉNÈRE LE SSH!
    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &output_buffer);
    if (err != CL_SUCCESS) {
        printf("❌ clSetKernelArg failed: %d\n", err);
        return 1;
    }
    
    printf("✅ Argument kernel configuré\n");
    printf("\n🔍 OpenCL a maintenant généré le SSH interne!\n");
    
    // 5. TENTATIVE DE DUMP SSH
    // Malheureusement, OpenCL ne fournit PAS d'API pour accéder au SSH directement
    // Nous devons utiliser une approche indirecte
    
    printf("\n⚠️  OpenCL ne fournit pas d'API pour dumper le SSH directement\n");
    printf("⚠️  Nous devons analyser le binaire compilé pour extraire le SSH\n\n");
    
    // 6. Extraction binaire kernel (contient le SSH template)
    size_t binary_size;
    err = clGetProgramInfo(program, CL_PROGRAM_BINARY_SIZES, sizeof(size_t), &binary_size, NULL);
    if (err != CL_SUCCESS) {
        printf("❌ clGetProgramInfo(BINARY_SIZES) failed: %d\n", err);
        return 1;
    }
    
    unsigned char* binary = (unsigned char*)malloc(binary_size);
    unsigned char* binaries[] = { binary };
    err = clGetProgramInfo(program, CL_PROGRAM_BINARIES, sizeof(unsigned char*), binaries, NULL);
    if (err != CL_SUCCESS) {
        printf("❌ clGetProgramInfo(BINARIES) failed: %d\n", err);
        free(binary);
        return 1;
    }
    
    printf("✅ Binaire extrait: %zu bytes\n", binary_size);
    
    // 7. Sauvegarde binaire pour analyse
    FILE* f = fopen("tests/test_c440_opencl_binary.bin", "wb");
    if (f) {
        fwrite(binary, 1, binary_size, f);
        fclose(f);
        printf("✅ Binaire sauvegardé: tests/test_c440_opencl_binary.bin\n");
    }
    
    // 8. Recherche du SSH dans le binaire
    // Le SSH est typiquement dans une section spécifique du binaire ELF
    printf("\n🔍 Recherche du SSH dans le binaire...\n");
    
    // Recherche de patterns Surface State connus
    // DW0 Surface State: 0x0008FF80 (Type=BUFFER, Format=RAW, VAlign=4)
    uint32_t pattern_ss_dw0 = 0x0008FF80;
    
    for (size_t i = 0; i < binary_size - 64; i += 4) {
        uint32_t* dw = (uint32_t*)(binary + i);
        if (dw[0] == pattern_ss_dw0) {
            printf("\n✅ Surface State trouvé à offset 0x%zx!\n", i);
            dump_dwords("SURFACE_STATE_OPENCL", dw, 64);
            
            // Recherche Binding Table avant le Surface State
            if (i >= 64) {
                printf("\n🔍 Recherche Binding Table avant Surface State...\n");
                uint32_t* potential_bt = (uint32_t*)(binary + i - 64);
                // Le Binding Table contient des offsets vers Surface States
                // Typiquement des valeurs comme 64, 128, 192, etc.
                if (potential_bt[0] >= 64 && potential_bt[0] < 4096 && potential_bt[0] % 64 == 0) {
                    printf("✅ Binding Table potentiel trouvé à offset 0x%zx!\n", i - 64);
                    dump_dwords("BINDING_TABLE_OPENCL", potential_bt, 16);
                }
            }
            
            break;
        }
    }
    
    // 9. Exécution kernel pour validation
    size_t global_size = 256;
    err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_size, NULL, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        printf("❌ clEnqueueNDRangeKernel failed: %d\n", err);
    } else {
        printf("\n✅ Kernel exécuté\n");
    }
    
    clFinish(queue);
    
    // 10. Lecture résultat
    uint32_t result[4] = {0};
    err = clEnqueueReadBuffer(queue, output_buffer, CL_TRUE, 0, sizeof(result), result, 0, NULL, NULL);
    if (err == CL_SUCCESS) {
        printf("✅ Résultat: output[0] = 0x%08x (attendu: 0x12345678)\n", result[0]);
        if (result[0] == 0x12345678) {
            printf("✅ OpenCL fonctionne correctement!\n");
        }
    }
    
    // Cleanup
    free(binary);
    clReleaseMemObject(output_buffer);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    
    printf("\n=== CONCLUSION ===\n");
    printf("Le SSH OpenCL est dans le binaire compilé.\n");
    printf("Analysez tests/test_c440_opencl_binary.bin pour extraire le SSH exact.\n");
    printf("Comparez-le byte-par-byte avec notre SSH natif.\n");
    
    return 0;
}

// Made with Bob
