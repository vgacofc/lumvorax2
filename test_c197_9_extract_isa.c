/*
 * CYCLE C197.9 - EXTRACTION ISA GEN9 OPENCL
 * ==========================================
 * 
 * OBJECTIF : Extraire ISA Gen9 réelle d'un kernel OpenCL compilé
 * 
 * MÉTHODE :
 * 1. Compiler kernel OpenCL minimal (output[0] = 0xDEADBEEF)
 * 2. Extraire binaire via clGetProgramInfo(CL_PROGRAM_BINARIES)
 * 3. Parser ELF container (section .text ou .kernel)
 * 4. Extraire ISA Gen9 brute (alignement 64 bytes)
 * 5. Sauvegarder ISA dans fichier binaire
 * 
 * VALIDATION :
 * - Binaire extrait doit être aligné 64 bytes
 * - Taille doit être multiple de 128 bits (16 bytes)
 * - Format ELF doit être valide
 * 
 * RÉFÉRENCE : C197.8 (kernel OpenCL fonctionnel)
 */

#define CL_TARGET_OPENCL_VERSION 300
#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <elf.h>

// Kernel OpenCL minimal (identique C197.8)
const char* kernel_source = 
"__kernel void test_write(__global uint* output) {\n"
"    output[0] = 0xDEADBEEF;\n"
"}\n";

// Structure pour stocker binaire extrait
typedef struct {
    unsigned char* data;
    size_t size;
    uint64_t timestamp_ns;
} extracted_binary_t;

// Fonction extraction binaire OpenCL
static int extract_opencl_binary(cl_program program, extracted_binary_t* binary) {
    cl_int err;
    
    // 1. Obtenir nombre de devices
    cl_uint num_devices = 0;
    err = clGetProgramInfo(program, CL_PROGRAM_NUM_DEVICES, sizeof(num_devices), &num_devices, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[ERROR] clGetProgramInfo(NUM_DEVICES) failed: %d\n", err);
        return -1;
    }
    printf("[INFO] Program has %u device(s)\n", num_devices);
    
    // 2. Obtenir tailles binaires
    size_t* binary_sizes = (size_t*)malloc(num_devices * sizeof(size_t));
    err = clGetProgramInfo(program, CL_PROGRAM_BINARY_SIZES, num_devices * sizeof(size_t), binary_sizes, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[ERROR] clGetProgramInfo(BINARY_SIZES) failed: %d\n", err);
        free(binary_sizes);
        return -1;
    }
    
    printf("[INFO] Binary sizes:\n");
    for (cl_uint i = 0; i < num_devices; i++) {
        printf("  Device %u: %zu bytes\n", i, binary_sizes[i]);
    }
    
    // 3. Allouer buffers pour binaires
    unsigned char** binaries = (unsigned char**)malloc(num_devices * sizeof(unsigned char*));
    for (cl_uint i = 0; i < num_devices; i++) {
        binaries[i] = (unsigned char*)malloc(binary_sizes[i]);
    }
    
    // 4. Extraire binaires
    err = clGetProgramInfo(program, CL_PROGRAM_BINARIES, num_devices * sizeof(unsigned char*), binaries, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[ERROR] clGetProgramInfo(BINARIES) failed: %d\n", err);
        for (cl_uint i = 0; i < num_devices; i++) free(binaries[i]);
        free(binaries);
        free(binary_sizes);
        return -1;
    }
    
    // 5. Copier premier binaire (device 0)
    binary->size = binary_sizes[0];
    binary->data = (unsigned char*)malloc(binary->size);
    memcpy(binary->data, binaries[0], binary->size);
    
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    binary->timestamp_ns = (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
    
    printf("[SUCCESS] Binary extracted: %zu bytes\n", binary->size);
    
    // Cleanup
    for (cl_uint i = 0; i < num_devices; i++) free(binaries[i]);
    free(binaries);
    free(binary_sizes);
    
    return 0;
}

// Fonction analyse ELF header
static int analyze_elf_header(const unsigned char* data, size_t size) {
    if (size < sizeof(Elf64_Ehdr)) {
        fprintf(stderr, "[ERROR] Binary too small for ELF header\n");
        return -1;
    }
    
    const Elf64_Ehdr* ehdr = (const Elf64_Ehdr*)data;
    
    // Vérifier magic ELF
    if (memcmp(ehdr->e_ident, ELFMAG, SELFMAG) != 0) {
        fprintf(stderr, "[WARNING] Not a valid ELF file (magic: 0x%02x%02x%02x%02x)\n",
                data[0], data[1], data[2], data[3]);
        return -1;
    }
    
    printf("[INFO] ELF Header:\n");
    printf("  Class: %s\n", ehdr->e_ident[EI_CLASS] == ELFCLASS64 ? "64-bit" : "32-bit");
    printf("  Data: %s\n", ehdr->e_ident[EI_DATA] == ELFDATA2LSB ? "Little-endian" : "Big-endian");
    printf("  Type: 0x%04x\n", ehdr->e_type);
    printf("  Machine: 0x%04x\n", ehdr->e_machine);
    printf("  Entry: 0x%016lx\n", ehdr->e_entry);
    printf("  Section headers: %u (offset: 0x%lx)\n", ehdr->e_shnum, ehdr->e_shoff);
    printf("  Program headers: %u (offset: 0x%lx)\n", ehdr->e_phnum, ehdr->e_phoff);
    
    return 0;
}

// Fonction extraction ISA Gen9 depuis ELF
static int extract_gen9_isa(const unsigned char* elf_data, size_t elf_size, 
                           unsigned char** isa_out, size_t* isa_size_out) {
    const Elf64_Ehdr* ehdr = (const Elf64_Ehdr*)elf_data;
    
    if (ehdr->e_shoff == 0 || ehdr->e_shnum == 0) {
        fprintf(stderr, "[ERROR] No section headers in ELF\n");
        return -1;
    }
    
    // Lire section headers
    const Elf64_Shdr* shdrs = (const Elf64_Shdr*)(elf_data + ehdr->e_shoff);
    
    // Trouver section string table
    const Elf64_Shdr* shstrtab = &shdrs[ehdr->e_shstrndx];
    const char* shstrtab_data = (const char*)(elf_data + shstrtab->sh_offset);
    
    printf("[INFO] Searching for ISA sections:\n");
    
    // Chercher sections contenant ISA (.text, .kernel, .intel_kernel)
    for (uint16_t i = 0; i < ehdr->e_shnum; i++) {
        const Elf64_Shdr* shdr = &shdrs[i];
        const char* name = shstrtab_data + shdr->sh_name;
        
        printf("  Section %u: %s (size: %lu, offset: 0x%lx, type: 0x%x)\n",
               i, name, shdr->sh_size, shdr->sh_offset, shdr->sh_type);
        
        // Chercher sections exécutables ou avec nom kernel
        if ((shdr->sh_flags & SHF_EXECINSTR) || 
            strstr(name, "text") || 
            strstr(name, "kernel") ||
            strstr(name, "intel")) {
            
            printf("[FOUND] Potential ISA section: %s (%lu bytes)\n", name, shdr->sh_size);
            
            // Extraire cette section
            *isa_size_out = shdr->sh_size;
            *isa_out = (unsigned char*)malloc(*isa_size_out);
            memcpy(*isa_out, elf_data + shdr->sh_offset, *isa_size_out);
            
            printf("[SUCCESS] ISA extracted from section '%s': %zu bytes\n", name, *isa_size_out);
            return 0;
        }
    }
    
    fprintf(stderr, "[ERROR] No ISA section found in ELF\n");
    return -1;
}

// Fonction sauvegarde binaire
static int save_binary(const char* filename, const unsigned char* data, size_t size) {
    FILE* f = fopen(filename, "wb");
    if (!f) {
        fprintf(stderr, "[ERROR] Cannot open %s for writing\n", filename);
        return -1;
    }
    
    size_t written = fwrite(data, 1, size, f);
    fclose(f);
    
    if (written != size) {
        fprintf(stderr, "[ERROR] Write failed: %zu/%zu bytes\n", written, size);
        return -1;
    }
    
    printf("[SUCCESS] Saved %zu bytes to %s\n", size, filename);
    return 0;
}

// Fonction dump hexadécimal
static void dump_hex(const unsigned char* data, size_t size, size_t max_lines) {
    printf("[HEX DUMP] First %zu bytes:\n", max_lines * 16);
    for (size_t i = 0; i < size && i < max_lines * 16; i += 16) {
        printf("  %04zx: ", i);
        for (size_t j = 0; j < 16 && i + j < size; j++) {
            printf("%02x ", data[i + j]);
        }
        printf("\n");
    }
}

int main(int argc, char** argv) {
    printf("=== CYCLE C197.9 - EXTRACTION ISA GEN9 OPENCL ===\n\n");
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    cl_int err;
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_program program;
    
    // 1. Initialisation OpenCL
    printf("[STEP 1] OpenCL initialization\n");
    err = clGetPlatformIDs(1, &platform, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[ERROR] clGetPlatformIDs failed: %d\n", err);
        return 1;
    }
    
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[ERROR] clGetDeviceIDs failed: %d\n", err);
        return 1;
    }
    
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[ERROR] clCreateContext failed: %d\n", err);
        return 1;
    }
    
    // 2. Compilation kernel
    printf("\n[STEP 2] Kernel compilation\n");
    size_t source_len = strlen(kernel_source);
    program = clCreateProgramWithSource(context, 1, &kernel_source, &source_len, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[ERROR] clCreateProgramWithSource failed: %d\n", err);
        return 1;
    }
    
    err = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[ERROR] clBuildProgram failed: %d\n", err);
        
        size_t log_size;
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        char* log = (char*)malloc(log_size);
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
        fprintf(stderr, "Build log:\n%s\n", log);
        free(log);
        return 1;
    }
    
    printf("[SUCCESS] Kernel compiled\n");
    
    // 3. Extraction binaire OpenCL
    printf("\n[STEP 3] Binary extraction\n");
    extracted_binary_t binary = {0};
    if (extract_opencl_binary(program, &binary) != 0) {
        fprintf(stderr, "[ERROR] Binary extraction failed\n");
        return 1;
    }
    
    // 4. Analyse ELF
    printf("\n[STEP 4] ELF analysis\n");
    analyze_elf_header(binary.data, binary.size);
    
    // 5. Extraction ISA Gen9
    printf("\n[STEP 5] ISA Gen9 extraction\n");
    unsigned char* isa_data = NULL;
    size_t isa_size = 0;
    
    if (extract_gen9_isa(binary.data, binary.size, &isa_data, &isa_size) != 0) {
        fprintf(stderr, "[ERROR] ISA extraction failed\n");
        free(binary.data);
        return 1;
    }
    
    // 6. Validation ISA
    printf("\n[STEP 6] ISA validation\n");
    printf("  ISA size: %zu bytes\n", isa_size);
    printf("  Alignment check (64 bytes): %s\n", (isa_size % 64 == 0) ? "OK" : "WARNING");
    printf("  Instruction alignment (16 bytes): %s\n", (isa_size % 16 == 0) ? "OK" : "WARNING");
    
    // 7. Dump hexadécimal
    printf("\n[STEP 7] ISA hexadecimal dump\n");
    dump_hex(isa_data, isa_size, 16);
    
    // 8. Sauvegarde fichiers
    printf("\n[STEP 8] Saving files\n");
    save_binary("c197_9_opencl_full_binary.bin", binary.data, binary.size);
    save_binary("c197_9_gen9_isa_extracted.bin", isa_data, isa_size);
    
    // 9. Métriques finales
    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed_ms = (end.tv_sec - start.tv_sec) * 1000.0 + 
                       (end.tv_nsec - start.tv_nsec) / 1000000.0;
    
    printf("\n=== RÉSULTATS C197.9 ===\n");
    printf("OpenCL binary size: %zu bytes\n", binary.size);
    printf("Gen9 ISA size: %zu bytes\n", isa_size);
    printf("Extraction time: %.3f ms\n", elapsed_ms);
    printf("Files saved:\n");
    printf("  - c197_9_opencl_full_binary.bin (%zu bytes)\n", binary.size);
    printf("  - c197_9_gen9_isa_extracted.bin (%zu bytes)\n", isa_size);
    
    // Cleanup
    free(isa_data);
    free(binary.data);
    clReleaseProgram(program);
    clReleaseContext(context);
    
    printf("\n[SUCCESS] ✓ ISA Gen9 extraction complete\n");
    return 0;
}

// Made with Bob
