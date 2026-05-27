/*
 * TEST C434 - EXTRACTION BINAIRE GEN9 ISA depuis OpenCL
 * Objectif: Extraire le binaire Gen9 ISA compilé par OpenCL pour le comparer avec le binaire actuel
 * 
 * Ce programme:
 * 1. Compile le kernel source avec OpenCL
 * 2. Extrait le binaire Gen9 ISA généré
 * 3. Sauvegarde dans test_c434_opencl_extracted.bin
 * 4. Affiche les informations de compilation
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
    cl_program program;
    
    FORENSIC_LOG("🚀 TEST C434 - EXTRACTION BINAIRE GEN9 ISA");
    
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
    
    // 4. Load kernel source
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
    
    // 5. Create program
    program = clCreateProgramWithSource(context, 1, (const char**)&source, &source_size, &err);
    CHECK_CL(err, "clCreateProgramWithSource");
    
    // 6. Build program
    FORENSIC_LOG("⚙️  Compilation kernel avec OpenCL...");
    err = clBuildProgram(program, 1, &device, "-cl-std=CL2.0", NULL, NULL);
    if (err != CL_SUCCESS) {
        char log[4096];
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, sizeof(log), log, NULL);
        FORENSIC_LOG("❌ Build failed:\n%s", log);
        return 1;
    }
    FORENSIC_LOG("✅ Kernel compilé avec succès");
    
    // 7. Get binary size
    size_t binary_size;
    err = clGetProgramInfo(program, CL_PROGRAM_BINARY_SIZES, sizeof(size_t), &binary_size, NULL);
    CHECK_CL(err, "clGetProgramInfo(BINARY_SIZES)");
    
    FORENSIC_LOG("📊 Taille binaire Gen9 ISA: %zu bytes", binary_size);
    
    // 8. Allocate buffer for binary
    unsigned char* binary = malloc(binary_size);
    if (!binary) {
        FORENSIC_LOG("❌ Allocation mémoire échouée");
        return 1;
    }
    
    // 9. Get binary
    unsigned char* binaries[1] = { binary };
    err = clGetProgramInfo(program, CL_PROGRAM_BINARIES, sizeof(unsigned char*), binaries, NULL);
    CHECK_CL(err, "clGetProgramInfo(BINARIES)");
    
    FORENSIC_LOG("✅ Binaire Gen9 ISA extrait");
    
    // 10. Save binary to file
    FILE* out = fopen("tests/test_c434_opencl_extracted.bin", "wb");
    if (!out) {
        FORENSIC_LOG("❌ Cannot create output file");
        free(binary);
        return 1;
    }
    
    size_t written = fwrite(binary, 1, binary_size, out);
    fclose(out);
    
    if (written != binary_size) {
        FORENSIC_LOG("❌ Écriture incomplète: %zu/%zu bytes", written, binary_size);
        free(binary);
        return 1;
    }
    
    FORENSIC_LOG("✅ Binaire sauvegardé: tests/test_c434_opencl_extracted.bin");
    
    // 11. Display binary info
    FORENSIC_LOG("🔍 ANALYSE BINAIRE:");
    FORENSIC_LOG("   Taille: %zu bytes", binary_size);
    FORENSIC_LOG("   Magic: 0x%02X%02X%02X%02X", binary[0], binary[1], binary[2], binary[3]);
    
    // 12. Display first 64 bytes (hex dump)
    FORENSIC_LOG("📝 Premiers 64 bytes (hex):");
    for (size_t i = 0; i < 64 && i < binary_size; i += 16) {
        fprintf(stderr, "   %04zx: ", i);
        for (size_t j = 0; j < 16 && (i + j) < binary_size; j++) {
            fprintf(stderr, "%02x ", binary[i + j]);
        }
        fprintf(stderr, "\n");
    }
    
    // 13. Compare with existing binary
    FILE* old = fopen("tests/test_c427_store_validation.bin.bin", "rb");
    if (old) {
        fseek(old, 0, SEEK_END);
        size_t old_size = ftell(old);
        fseek(old, 0, SEEK_SET);
        
        FORENSIC_LOG("🔍 COMPARAISON avec binaire actuel:");
        FORENSIC_LOG("   Ancien: %zu bytes", old_size);
        FORENSIC_LOG("   Nouveau: %zu bytes", binary_size);
        
        if (old_size == binary_size) {
            unsigned char* old_binary = malloc(old_size);
            fread(old_binary, 1, old_size, old);
            
            int differences = 0;
            for (size_t i = 0; i < old_size; i++) {
                if (old_binary[i] != binary[i]) {
                    differences++;
                    if (differences <= 10) {
                        FORENSIC_LOG("   Diff @0x%04zx: 0x%02x → 0x%02x", i, old_binary[i], binary[i]);
                    }
                }
            }
            
            if (differences == 0) {
                FORENSIC_LOG("✅ Binaires IDENTIQUES");
            } else {
                FORENSIC_LOG("⚠️  %d différences trouvées", differences);
            }
            
            free(old_binary);
        } else {
            FORENSIC_LOG("⚠️  Tailles différentes!");
        }
        
        fclose(old);
    } else {
        FORENSIC_LOG("⚠️  Binaire actuel non trouvé pour comparaison");
    }
    
    // 14. Cleanup
    free(binary);
    free(source);
    clReleaseProgram(program);
    clReleaseContext(context);
    
    FORENSIC_LOG("✅ TEST C434 TERMINÉ");
    FORENSIC_LOG("📝 Prochaine étape: ./test_c433_dispatch_runner tests/test_c434_opencl_extracted.bin");
    
    return 0;
}

// Made with Bob
