/**
 * @file compile_kernel_to_spirv.c
 * @brief Outil de compilation OpenCL C → SPIR-V via API OpenCL
 * 
 * Compile un kernel OpenCL C en binaire SPIR-V en utilisant l'API OpenCL,
 * puis extrait le binaire compilé pour utilisation avec Level Zero.
 * 
 * Approche:
 * 1. Charge kernel OpenCL C depuis fichier
 * 2. Compile via clBuildProgram()
 * 3. Extrait binaire via clGetProgramInfo(CL_PROGRAM_BINARIES)
 * 4. Sauvegarde binaire SPIR-V dans fichier .spv
 * 
 * Usage: ./compile_kernel_to_spirv input.cl output.spv
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <CL/cl.h>

#define MAX_PLATFORMS 10
#define MAX_DEVICES 10
#define MAX_SOURCE_SIZE (1024 * 1024)  // 1MB max

/**
 * Lit le contenu d'un fichier dans un buffer
 */
char* read_file(const char* filename, size_t* size) {
    FILE* fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "❌ Erreur: Impossible d'ouvrir %s\n", filename);
        return NULL;
    }
    
    fseek(fp, 0, SEEK_END);
    *size = ftell(fp);
    rewind(fp);
    
    char* buffer = (char*)malloc(*size + 1);
    if (!buffer) {
        fprintf(stderr, "❌ Erreur: Allocation mémoire échouée\n");
        fclose(fp);
        return NULL;
    }
    
    size_t read = fread(buffer, 1, *size, fp);
    buffer[read] = '\0';
    *size = read;
    
    fclose(fp);
    return buffer;
}

/**
 * Écrit un buffer dans un fichier
 */
int write_file(const char* filename, const unsigned char* data, size_t size) {
    FILE* fp = fopen(filename, "wb");
    if (!fp) {
        fprintf(stderr, "❌ Erreur: Impossible de créer %s\n", filename);
        return -1;
    }
    
    size_t written = fwrite(data, 1, size, fp);
    fclose(fp);
    
    if (written != size) {
        fprintf(stderr, "❌ Erreur: Écriture incomplète (%zu/%zu bytes)\n", written, size);
        return -1;
    }
    
    return 0;
}

int main(int argc, char** argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <input.cl> <output.spv>\n", argv[0]);
        fprintf(stderr, "Exemple: %s kernels/test_add.cl kernels/test_add.spv\n", argv[0]);
        return 1;
    }
    
    const char* input_file = argv[1];
    const char* output_file = argv[2];
    
    printf("🔧 Compilation OpenCL C → SPIR-V\n");
    printf("   Input:  %s\n", input_file);
    printf("   Output: %s\n\n", output_file);
    
    // Étape 1: Lire le source OpenCL C
    size_t source_size;
    char* source = read_file(input_file, &source_size);
    if (!source) {
        return 1;
    }
    printf("✅ Source chargé: %zu bytes\n", source_size);
    
    // Étape 2: Initialiser OpenCL
    cl_platform_id platforms[MAX_PLATFORMS];
    cl_uint num_platforms;
    cl_int err = clGetPlatformIDs(MAX_PLATFORMS, platforms, &num_platforms);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "❌ Erreur clGetPlatformIDs: %d\n", err);
        free(source);
        return 1;
    }
    printf("✅ Platforms trouvées: %u\n", num_platforms);
    
    // Étape 3: Trouver un device GPU Intel
    cl_device_id device = NULL;
    for (cl_uint i = 0; i < num_platforms; i++) {
        cl_device_id devices[MAX_DEVICES];
        cl_uint num_devices;
        err = clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_GPU, MAX_DEVICES, devices, &num_devices);
        if (err == CL_SUCCESS && num_devices > 0) {
            // Vérifier si c'est un device Intel
            char vendor[256];
            clGetDeviceInfo(devices[0], CL_DEVICE_VENDOR, sizeof(vendor), vendor, NULL);
            if (strstr(vendor, "Intel")) {
                device = devices[0];
                printf("✅ Device Intel trouvé: %s\n", vendor);
                break;
            }
        }
    }
    
    if (!device) {
        fprintf(stderr, "❌ Erreur: Aucun device GPU Intel trouvé\n");
        free(source);
        return 1;
    }
    
    // Étape 4: Créer contexte
    cl_context context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "❌ Erreur clCreateContext: %d\n", err);
        free(source);
        return 1;
    }
    printf("✅ Contexte créé\n");
    
    // Étape 5: Créer programme depuis source
    cl_program program = clCreateProgramWithSource(context, 1, (const char**)&source, &source_size, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "❌ Erreur clCreateProgramWithSource: %d\n", err);
        clReleaseContext(context);
        free(source);
        return 1;
    }
    printf("✅ Programme créé\n");
    
    // Étape 6: Compiler le programme
    printf("⏳ Compilation en cours...\n");
    err = clBuildProgram(program, 1, &device, "-cl-std=CL2.0", NULL, NULL);
    
    // Récupérer le log de compilation
    size_t log_size;
    clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
    if (log_size > 1) {
        char* log = (char*)malloc(log_size);
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
        printf("📋 Build log:\n%s\n", log);
        free(log);
    }
    
    if (err != CL_SUCCESS) {
        fprintf(stderr, "❌ Erreur clBuildProgram: %d\n", err);
        clReleaseProgram(program);
        clReleaseContext(context);
        free(source);
        return 1;
    }
    printf("✅ Compilation réussie\n");
    
    // Étape 7: Extraire le binaire compilé
    size_t binary_size;
    err = clGetProgramInfo(program, CL_PROGRAM_BINARY_SIZES, sizeof(size_t), &binary_size, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "❌ Erreur clGetProgramInfo(BINARY_SIZES): %d\n", err);
        clReleaseProgram(program);
        clReleaseContext(context);
        free(source);
        return 1;
    }
    printf("✅ Taille binaire: %zu bytes\n", binary_size);
    
    unsigned char* binary = (unsigned char*)malloc(binary_size);
    if (!binary) {
        fprintf(stderr, "❌ Erreur: Allocation mémoire binaire échouée\n");
        clReleaseProgram(program);
        clReleaseContext(context);
        free(source);
        return 1;
    }
    
    unsigned char* binaries[1] = { binary };
    err = clGetProgramInfo(program, CL_PROGRAM_BINARIES, sizeof(unsigned char*), binaries, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "❌ Erreur clGetProgramInfo(BINARIES): %d\n", err);
        free(binary);
        clReleaseProgram(program);
        clReleaseContext(context);
        free(source);
        return 1;
    }
    printf("✅ Binaire extrait\n");
    
    // Étape 8: Vérifier le magic number SPIR-V
    if (binary_size >= 4) {
        uint32_t magic = *(uint32_t*)binary;
        printf("📊 Magic number: 0x%08X ", magic);
        if (magic == 0x07230203) {
            printf("(SPIR-V ✓)\n");
        } else if (magic == 0x464C457F) {
            printf("(ELF - binaire natif)\n");
        } else {
            printf("(Format inconnu)\n");
        }
    }
    
    // Étape 9: Sauvegarder le binaire
    if (write_file(output_file, binary, binary_size) != 0) {
        free(binary);
        clReleaseProgram(program);
        clReleaseContext(context);
        free(source);
        return 1;
    }
    printf("✅ Binaire sauvegardé: %s (%zu bytes)\n", output_file, binary_size);
    
    // Cleanup
    free(binary);
    clReleaseProgram(program);
    clReleaseContext(context);
    free(source);
    
    printf("\n🎉 Compilation terminée avec succès!\n");
    return 0;
}

// Made with Bob
