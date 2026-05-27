/*
 * EXTRACT KERNEL ISA - Outil Intel Beignet
 * 
 * Source: /tmp/beignet/utests/load_program_from_gen_bin.cpp lignes 40-47
 * 
 * Compile un kernel OpenCL et extrait le binaire ISA Gen9
 * pour utilisation dans i915 DRM natif
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <CL/cl.h>

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <kernel.cl>\n", argv[0]);
        return 1;
    }
    
    const char* kernel_file = argv[1];
    
    // Lire kernel source
    FILE* f = fopen(kernel_file, "r");
    if (!f) {
        perror("fopen");
        return 1;
    }
    
    fseek(f, 0, SEEK_END);
    size_t src_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char* src = malloc(src_size + 1);
    fread(src, 1, src_size, f);
    src[src_size] = '\0';
    fclose(f);
    
    printf("✓ Kernel source lu: %zu bytes\n", src_size);
    
    // Init OpenCL
    cl_platform_id platform;
    cl_device_id device;
    cl_context ctx;
    cl_program program;
    cl_int err;
    
    err = clGetPlatformIDs(1, &platform, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "clGetPlatformIDs failed: %d\n", err);
        return 1;
    }
    
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "clGetDeviceIDs failed: %d\n", err);
        return 1;
    }
    
    ctx = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "clCreateContext failed: %d\n", err);
        return 1;
    }
    
    printf("✓ OpenCL initialisé\n");
    
    // Créer et compiler program (CODE INTEL BEIGNET)
    program = clCreateProgramWithSource(ctx, 1, (const char**)&src, &src_size, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "clCreateProgramWithSource failed: %d\n", err);
        return 1;
    }
    
    err = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "clBuildProgram failed: %d\n", err);
        
        // Afficher build log
        size_t log_size;
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        char* log = malloc(log_size);
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
        fprintf(stderr, "Build log:\n%s\n", log);
        free(log);
        return 1;
    }
    
    printf("✓ Kernel compilé\n");
    
    // Extraire binaire ISA (CODE INTEL BEIGNET lignes 40-47)
    size_t binary_size;
    err = clGetProgramInfo(program, CL_PROGRAM_BINARY_SIZES, sizeof(binary_size), &binary_size, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "clGetProgramInfo(BINARY_SIZES) failed: %d\n", err);
        return 1;
    }
    
    printf("✓ Taille binaire ISA: %zu bytes\n", binary_size);
    
    // Allouer buffer pour binaire
    unsigned char* binary = malloc(binary_size);
    if (!binary) {
        fprintf(stderr, "malloc failed\n");
        return 1;
    }
    
    // Récupérer binaire (CODE INTEL BEIGNET ligne 46)
    err = clGetProgramInfo(program, CL_PROGRAM_BINARIES, sizeof(&binary), &binary, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "clGetProgramInfo(BINARIES) failed: %d\n", err);
        return 1;
    }
    
    printf("✓ Binaire ISA extrait\n");
    
    // Sauvegarder binaire
    char output_file[256];
    snprintf(output_file, sizeof(output_file), "%s.bin", kernel_file);
    
    FILE* out = fopen(output_file, "wb");
    if (!out) {
        perror("fopen output");
        return 1;
    }
    
    fwrite(binary, 1, binary_size, out);
    fclose(out);
    
    printf("✓ Binaire sauvegardé: %s\n", output_file);
    
    // Afficher premiers bytes (debug)
    printf("\nPremiers 64 bytes (hex):\n");
    for (size_t i = 0; i < 64 && i < binary_size; i++) {
        printf("%02x ", binary[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }
    printf("\n");
    
    // Générer header C
    snprintf(output_file, sizeof(output_file), "%s.h", kernel_file);
    out = fopen(output_file, "w");
    if (out) {
        fprintf(out, "/* Auto-generated from %s */\n", kernel_file);
        fprintf(out, "#ifndef KERNEL_ISA_H\n");
        fprintf(out, "#define KERNEL_ISA_H\n\n");
        fprintf(out, "static const unsigned char kernel_isa[] = {\n");
        
        for (size_t i = 0; i < binary_size; i++) {
            if (i % 12 == 0) fprintf(out, "    ");
            fprintf(out, "0x%02x", binary[i]);
            if (i < binary_size - 1) fprintf(out, ",");
            if ((i + 1) % 12 == 0) fprintf(out, "\n");
        }
        
        fprintf(out, "\n};\n");
        fprintf(out, "static const size_t kernel_isa_size = %zu;\n\n", binary_size);
        fprintf(out, "#endif /* KERNEL_ISA_H */\n");
        fclose(out);
        
        printf("✓ Header C généré: %s\n", output_file);
    }
    
    // Cleanup
    free(binary);
    free(src);
    clReleaseProgram(program);
    clReleaseContext(ctx);
    
    printf("\n🎉 SUCCÈS! ISA Gen9 extrait et prêt pour i915 natif\n");
    
    return 0;
}

// Made with Bob
