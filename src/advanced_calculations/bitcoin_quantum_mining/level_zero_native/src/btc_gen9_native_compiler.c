/**
 * @file btc_gen9_native_compiler.c
 * @brief Implémentation module natif LumVorax pour compilation Gen9 ISA
 * 
 * AUTEUR : Bob (LumVorax C198 Phase 10B)
 * DATE : 2026-05-09
 */

#define _POSIX_C_SOURCE 199309L

#include "btc_gen9_native_compiler.h"
// Logger non nécessaire pour le compilateur Gen9
// #include "btc_gpu_async_logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

/* ============================================================================
 * CONSTANTES
 * ============================================================================ */

#define GEN9_ISA_MAGIC_BYTES 0x464C457F  /**< Magic ELF (0x7F 'E' 'L' 'F') */
#define GEN9_ISA_MIN_SIZE 64             /**< Taille minimale ISA Gen9 */
#define GEN9_ISA_ALIGNMENT 64            /**< Alignement ISA Gen9 */
#define GEN9_CACHE_VERSION 1             /**< Version format cache */

#define MAX_BUILD_LOG_SIZE (64 * 1024)   /**< Taille max log compilation */
#define MAX_KERNEL_NAME 256              /**< Taille max nom kernel */

/* ============================================================================
 * STRUCTURES INTERNES
 * ============================================================================ */

/**
 * @brief Header ELF64
 */
typedef struct {
    uint32_t magic;         /**< 0x7F 'E' 'L' 'F' */
    uint8_t class;          /**< 1=32bit, 2=64bit */
    uint8_t data;           /**< 1=little endian, 2=big endian */
    uint8_t version;        /**< 1=current */
    uint8_t osabi;          /**< OS ABI */
    uint8_t abiversion;     /**< ABI version */
    uint8_t pad[7];         /**< Padding */
    uint16_t type;          /**< Object file type */
    uint16_t machine;       /**< Architecture */
    uint32_t version2;      /**< Object file version */
    uint64_t entry;         /**< Entry point */
    uint64_t phoff;         /**< Program header offset */
    uint64_t shoff;         /**< Section header offset */
    uint32_t flags;         /**< Processor flags */
    uint16_t ehsize;        /**< ELF header size */
    uint16_t phentsize;     /**< Program header entry size */
    uint16_t phnum;         /**< Program header count */
    uint16_t shentsize;     /**< Section header entry size */
    uint16_t shnum;         /**< Section header count */
    uint16_t shstrndx;      /**< Section name string table index */
} __attribute__((packed)) elf64_header_t;

/**
 * @brief Section header ELF64
 */
typedef struct {
    uint32_t name;          /**< Section name (string table index) */
    uint32_t type;          /**< Section type */
    uint64_t flags;         /**< Section flags */
    uint64_t addr;          /**< Section virtual address */
    uint64_t offset;        /**< Section file offset */
    uint64_t size;          /**< Section size */
    uint32_t link;          /**< Link to another section */
    uint32_t info;          /**< Additional section info */
    uint64_t addralign;     /**< Section alignment */
    uint64_t entsize;       /**< Entry size if section holds table */
} __attribute__((packed)) elf64_section_t;

/**
 * @brief Header cache Gen9
 */
typedef struct {
    uint32_t magic;         /**< 'LUM9' */
    uint32_t version;       /**< Version format cache */
    uint8_t source_hash[32]; /**< SHA-256 du source */
    uint32_t isa_crc32;     /**< CRC32 de l'ISA */
    uint64_t compile_time;  /**< Timestamp compilation */
    uint32_t isa_size;      /**< Taille ISA */
    char kernel_name[MAX_KERNEL_NAME]; /**< Nom kernel */
} __attribute__((packed)) gen9_cache_header_t;

/* ============================================================================
 * VARIABLES GLOBALES
 * ============================================================================ */

static char g_build_log[MAX_BUILD_LOG_SIZE];  /**< Log compilation */

/* ============================================================================
 * UTILITAIRES CRC32
 * ============================================================================ */

/**
 * @brief Table CRC32 (polynôme IEEE 802.3)
 */
static const uint32_t crc32_table[256] = {
    0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F,
    0xE963A535, 0x9E6495A3, 0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
    0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91, 0x1DB71064, 0x6AB020F2,
    0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
    0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9,
    0xFA0F3D63, 0x8D080DF5, 0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
    0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B, 0x35B5A8FA, 0x42B2986C,
    0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
    0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423,
    0xCFBA9599, 0xB8BDA50F, 0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
    0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D, 0x76DC4190, 0x01DB7106,
    0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
    0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D,
    0x91646C97, 0xE6635C01, 0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
    0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457, 0x65B0D9C6, 0x12B7E950,
    0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
    0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7,
    0xA4D1C46D, 0xD3D6F4FB, 0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
    0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9, 0x5005713C, 0x270241AA,
    0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
    0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81,
    0xB7BD5C3B, 0xC0BA6CAD, 0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
    0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683, 0xE3630B12, 0x94643B84,
    0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
    0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB,
    0x196C3671, 0x6E6B06E7, 0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
    0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5, 0xD6D6A3E8, 0xA1D1937E,
    0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
    0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55,
    0x316E8EEF, 0x4669BE79, 0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
    0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F, 0xC5BA3BBE, 0xB2BD0B28,
    0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
    0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F,
    0x72076785, 0x05005713, 0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
    0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21, 0x86D3D2D4, 0xF1D4E242,
    0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
    0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69,
    0x616BFFD3, 0x166CCF45, 0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
    0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB, 0xAED16A4A, 0xD9D65ADC,
    0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
    0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693,
    0x54DE5729, 0x23D967BF, 0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
    0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
};

uint32_t btc_gen9_crc32(const uint8_t* data, size_t size) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < size; i++) {
        crc = crc32_table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFF;
}

/* ============================================================================
 * UTILITAIRES SHA-256
 * ============================================================================ */

void btc_gen9_hash_source(const char* source, uint8_t hash[32]) {
    // TODO: Implémenter SHA-256 (pour l'instant CRC32 étendu)
    uint32_t crc = btc_gen9_crc32((const uint8_t*)source, strlen(source));
    memset(hash, 0, 32);
    memcpy(hash, &crc, sizeof(crc));
}

/* ============================================================================
 * EXTRACTION ISA GEN9 PUR
 * ============================================================================ */

uint8_t* btc_gen9_extract_pure_isa(
    const uint8_t* elf_binary,
    size_t elf_size,
    size_t* isa_size
) {
    if (!elf_binary || !isa_size || elf_size < sizeof(elf64_header_t)) {
        fprintf(stderr, "[GEN9-COMPILER] ERREUR : Paramètres invalides\n");
        return NULL;
    }

    // Vérifier magic ELF
    const elf64_header_t* elf_hdr = (const elf64_header_t*)elf_binary;
    if (elf_hdr->magic != GEN9_ISA_MAGIC_BYTES) {
        fprintf(stderr, "[GEN9-COMPILER] ERREUR : Magic ELF invalide (0x%08X)\n", 
                elf_hdr->magic);
        return NULL;
    }

    // Vérifier ELF64
    if (elf_hdr->class != 2) {
        fprintf(stderr, "[GEN9-COMPILER] ERREUR : Pas un ELF64\n");
        return NULL;
    }

    // Localiser section headers
    if (elf_hdr->shoff == 0 || elf_hdr->shnum == 0) {
        fprintf(stderr, "[GEN9-COMPILER] ERREUR : Pas de section headers\n");
        return NULL;
    }

    const uint8_t* sections_base = elf_binary + elf_hdr->shoff;
    
    // Chercher section .text ou section avec code Gen9
    // STRATÉGIE : Prendre la plus grande section exécutable
    size_t max_section_size = 0;
    const elf64_section_t* max_section = NULL;

    for (uint16_t i = 0; i < elf_hdr->shnum; i++) {
        const elf64_section_t* section = 
            (const elf64_section_t*)(sections_base + i * elf_hdr->shentsize);
        
        // Section exécutable (SHF_EXECINSTR = 0x4)
        if ((section->flags & 0x4) && section->size > max_section_size) {
            max_section_size = section->size;
            max_section = section;
        }
    }

    if (!max_section || max_section_size == 0) {
        fprintf(stderr, "[GEN9-COMPILER] ERREUR : Aucune section exécutable trouvée\n");
        return NULL;
    }

    // Extraire ISA
    if (max_section->offset + max_section_size > elf_size) {
        fprintf(stderr, "[GEN9-COMPILER] ERREUR : Section hors limites\n");
        return NULL;
    }

    uint8_t* isa_data = malloc(max_section_size);
    if (!isa_data) {
        fprintf(stderr, "[GEN9-COMPILER] ERREUR : Allocation mémoire échouée\n");
        return NULL;
    }

    memcpy(isa_data, elf_binary + max_section->offset, max_section_size);
    *isa_size = max_section_size;

    printf("[GEN9-COMPILER] ISA Gen9 extrait : %zu bytes (section offset=0x%lX)\n",
           max_section_size, max_section->offset);

    return isa_data;
}

/* ============================================================================
 * INITIALISATION / CLEANUP
 * ============================================================================ */

btc_gen9_compiler_ctx_t* btc_gen9_compiler_init(
    const char* cache_dir,
    bool enable_cache,
    bool enable_forensic
) {
    btc_gen9_compiler_ctx_t* ctx = calloc(1, sizeof(btc_gen9_compiler_ctx_t));
    if (!ctx) {
        fprintf(stderr, "[GEN9-COMPILER] ERREUR : Allocation contexte échouée\n");
        return NULL;
    }

    // Configuration cache
    if (cache_dir) {
        snprintf(ctx->cache_dir, sizeof(ctx->cache_dir), "%s", cache_dir);
    } else {
        const char* home = getenv("HOME");
        snprintf(ctx->cache_dir, sizeof(ctx->cache_dir), 
                 "%s/.lumvorax/gen9_cache", home ? home : "/tmp");
    }
    ctx->enable_cache = enable_cache;

    // Créer répertoire cache
    if (enable_cache) {
        mkdir(ctx->cache_dir, 0755);
    }

    // Configuration forensique
    ctx->enable_forensic = enable_forensic;
    if (enable_forensic) {
        char log_path[1024];
        snprintf(log_path, sizeof(log_path), "%s/gen9_compiler.log", ctx->cache_dir);
        ctx->forensic_log = fopen(log_path, "a");
        if (!ctx->forensic_log) {
            fprintf(stderr, "[GEN9-COMPILER] AVERTISSEMENT : Impossible d'ouvrir log forensique\n");
        }
    }

    // Initialiser OpenCL
    cl_platform_id platform;
    cl_device_id device;
    cl_int err;

    err = clGetPlatformIDs(1, &platform, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[GEN9-COMPILER] ERREUR : clGetPlatformIDs = %d\n", err);
        free(ctx);
        return NULL;
    }

    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[GEN9-COMPILER] ERREUR : clGetDeviceIDs = %d\n", err);
        free(ctx);
        return NULL;
    }

    cl_context cl_ctx = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[GEN9-COMPILER] ERREUR : clCreateContext = %d\n", err);
        free(ctx);
        return NULL;
    }

    cl_command_queue queue = clCreateCommandQueue(cl_ctx, device, 0, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[GEN9-COMPILER] ERREUR : clCreateCommandQueue = %d\n", err);
        clReleaseContext(cl_ctx);
        free(ctx);
        return NULL;
    }

    ctx->cl_context = cl_ctx;
    ctx->cl_device = device;
    ctx->cl_queue = queue;

    printf("[GEN9-COMPILER] Compilateur initialisé (cache=%s, forensic=%s)\n",
           enable_cache ? "ON" : "OFF",
           enable_forensic ? "ON" : "OFF");

    return ctx;
}

void btc_gen9_compiler_cleanup(btc_gen9_compiler_ctx_t* ctx) {
    if (!ctx) return;

    if (ctx->cl_queue) clReleaseCommandQueue(ctx->cl_queue);
    if (ctx->cl_context) clReleaseContext(ctx->cl_context);
    if (ctx->forensic_log) fclose(ctx->forensic_log);

    free(ctx);
}

/* ============================================================================
 * COMPILATION
 * ============================================================================ */

btc_gen9_isa_binary_t* btc_gen9_compile_kernel(
    btc_gen9_compiler_ctx_t* ctx,
    const char* source,
    const char* kernel_name,
    const btc_gen9_compile_options_t* options
) {
    if (!ctx || !source || !kernel_name) {
        fprintf(stderr, "[GEN9-COMPILER] ERREUR : Paramètres invalides\n");
        return NULL;
    }

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    // Calculer hash source
    uint8_t source_hash[32];
    btc_gen9_hash_source(source, source_hash);

    // Vérifier cache
    if (ctx->enable_cache) {
        btc_gen9_isa_binary_t* cached = btc_gen9_load_from_cache(ctx, kernel_name, source_hash);
        if (cached) {
            printf("[GEN9-COMPILER] Kernel '%s' chargé depuis cache\n", kernel_name);
            return cached;
        }
    }

    // Compiler via OpenCL
    cl_int err;
    size_t source_len = strlen(source);
    cl_program program = clCreateProgramWithSource(
        ctx->cl_context, 1, &source, &source_len, &err
    );
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[GEN9-COMPILER] ERREUR : clCreateProgramWithSource = %d\n", err);
        return NULL;
    }

    // Options compilation
    const char* build_opts = "-cl-std=CL1.2";
    if (options && options->build_options) {
        build_opts = options->build_options;
    }

    err = clBuildProgram(program, 1, &ctx->cl_device, build_opts, NULL, NULL);
    if (err != CL_SUCCESS) {
        // Récupérer log compilation
        clGetProgramBuildInfo(program, ctx->cl_device, CL_PROGRAM_BUILD_LOG,
                             MAX_BUILD_LOG_SIZE, g_build_log, NULL);
        fprintf(stderr, "[GEN9-COMPILER] ERREUR : clBuildProgram = %d\n%s\n", 
                err, g_build_log);
        clReleaseProgram(program);
        return NULL;
    }

    // Extraire binaire ELF+SPIR-V
    size_t binary_size;
    err = clGetProgramInfo(program, CL_PROGRAM_BINARY_SIZES, 
                          sizeof(size_t), &binary_size, NULL);
    if (err != CL_SUCCESS || binary_size == 0) {
        fprintf(stderr, "[GEN9-COMPILER] ERREUR : clGetProgramInfo (size) = %d\n", err);
        clReleaseProgram(program);
        return NULL;
    }

    unsigned char* elf_binary = malloc(binary_size);
    if (!elf_binary) {
        fprintf(stderr, "[GEN9-COMPILER] ERREUR : Allocation binaire échouée\n");
        clReleaseProgram(program);
        return NULL;
    }

    unsigned char* binaries[1] = { elf_binary };
    err = clGetProgramInfo(program, CL_PROGRAM_BINARIES, 
                          sizeof(unsigned char*), binaries, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[GEN9-COMPILER] ERREUR : clGetProgramInfo (binary) = %d\n", err);
        free(elf_binary);
        clReleaseProgram(program);
        return NULL;
    }

    clReleaseProgram(program);

    printf("[GEN9-COMPILER] Binaire ELF+SPIR-V compilé : %zu bytes\n", binary_size);

    // Extraire ISA Gen9 pur
    size_t isa_size;
    uint8_t* isa_data = btc_gen9_extract_pure_isa(elf_binary, binary_size, &isa_size);
    free(elf_binary);

    if (!isa_data) {
        fprintf(stderr, "[GEN9-COMPILER] ERREUR : Extraction ISA échouée\n");
        return NULL;
    }

    // Créer structure binaire
    btc_gen9_isa_binary_t* binary = calloc(1, sizeof(btc_gen9_isa_binary_t));
    if (!binary) {
        fprintf(stderr, "[GEN9-COMPILER] ERREUR : Allocation structure échouée\n");
        free(isa_data);
        return NULL;
    }

    binary->data = isa_data;
    binary->size = isa_size;
    binary->crc32 = btc_gen9_crc32(isa_data, isa_size);
    snprintf(binary->kernel_name, sizeof(binary->kernel_name), "%s", kernel_name);
    binary->cached = false;

    clock_gettime(CLOCK_MONOTONIC, &end);
    binary->compile_time_ns = (end.tv_sec - start.tv_sec) * 1000000000ULL +
                              (end.tv_nsec - start.tv_nsec);

    printf("[GEN9-COMPILER] Kernel '%s' compilé : %zu bytes ISA, CRC32=0x%08X, temps=%lu ns\n",
           kernel_name, isa_size, binary->crc32, binary->compile_time_ns);

    // Sauvegarder dans cache
    if (ctx->enable_cache) {
        btc_gen9_save_to_cache(ctx, binary, source_hash);
    }

    return binary;
}

btc_gen9_isa_binary_t* btc_gen9_compile_file(
    btc_gen9_compiler_ctx_t* ctx,
    const char* source_file,
    const char* kernel_name,
    const btc_gen9_compile_options_t* options
) {
    FILE* f = fopen(source_file, "r");
    if (!f) {
        fprintf(stderr, "[GEN9-COMPILER] ERREUR : Impossible d'ouvrir '%s'\n", source_file);
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* source = malloc(size + 1);
    if (!source) {
        fclose(f);
        return NULL;
    }

    fread(source, 1, size, f);
    source[size] = '\0';
    fclose(f);

    btc_gen9_isa_binary_t* binary = btc_gen9_compile_kernel(ctx, source, kernel_name, options);
    free(source);

    return binary;
}

/* ============================================================================
 * CACHE
 * ============================================================================ */

btc_gen9_isa_binary_t* btc_gen9_load_from_cache(
    btc_gen9_compiler_ctx_t* ctx,
    const char* kernel_name,
    const uint8_t source_hash[32]
) {
    char cache_path[1024];
    snprintf(cache_path, sizeof(cache_path), "%s/%s.gen9", 
             ctx->cache_dir, kernel_name);

    FILE* f = fopen(cache_path, "rb");
    if (!f) return NULL;

    // Lire header
    gen9_cache_header_t header;
    if (fread(&header, sizeof(header), 1, f) != 1) {
        fclose(f);
        return NULL;
    }

    // Vérifier magic + version
    if (header.magic != 0x394D554C || header.version != GEN9_CACHE_VERSION) {
        fclose(f);
        return NULL;
    }

    // Vérifier hash source
    if (memcmp(header.source_hash, source_hash, 32) != 0) {
        fclose(f);
        return NULL;
    }

    // Lire ISA
    uint8_t* isa_data = malloc(header.isa_size);
    if (!isa_data) {
        fclose(f);
        return NULL;
    }

    if (fread(isa_data, 1, header.isa_size, f) != header.isa_size) {
        free(isa_data);
        fclose(f);
        return NULL;
    }

    fclose(f);

    // Vérifier CRC32
    uint32_t crc = btc_gen9_crc32(isa_data, header.isa_size);
    if (crc != header.isa_crc32) {
        free(isa_data);
        return NULL;
    }

    // Créer structure
    btc_gen9_isa_binary_t* binary = calloc(1, sizeof(btc_gen9_isa_binary_t));
    if (!binary) {
        free(isa_data);
        return NULL;
    }

    binary->data = isa_data;
    binary->size = header.isa_size;
    binary->crc32 = header.isa_crc32;
    snprintf(binary->kernel_name, sizeof(binary->kernel_name), "%s", header.kernel_name);
    binary->compile_time_ns = 0;
    binary->cached = true;

    return binary;
}

bool btc_gen9_save_to_cache(
    btc_gen9_compiler_ctx_t* ctx,
    const btc_gen9_isa_binary_t* binary,
    const uint8_t source_hash[32]
) {
    char cache_path[1024];
    snprintf(cache_path, sizeof(cache_path), "%s/%s.gen9", 
             ctx->cache_dir, binary->kernel_name);

    FILE* f = fopen(cache_path, "wb");
    if (!f) return false;

    // Écrire header
    gen9_cache_header_t header = {
        .magic = 0x394D554C,  // 'LUM9'
        .version = GEN9_CACHE_VERSION,
        .isa_crc32 = binary->crc32,
        .compile_time = time(NULL),
        .isa_size = binary->size
    };
    memcpy(header.source_hash, source_hash, 32);
    snprintf(header.kernel_name, sizeof(header.kernel_name), "%s", binary->kernel_name);

    fwrite(&header, sizeof(header), 1, f);
    fwrite(binary->data, 1, binary->size, f);
    fclose(f);

    printf("[GEN9-COMPILER] Kernel '%s' sauvegardé dans cache\n", binary->kernel_name);
    return true;
}

/* ============================================================================
 * VALIDATION
 * ============================================================================ */

bool btc_gen9_validate_binary(const btc_gen9_isa_binary_t* binary) {
    if (!binary || !binary->data) return false;
    if (binary->size < GEN9_ISA_MIN_SIZE) return false;
    if (binary->size % GEN9_ISA_ALIGNMENT != 0) return false;

    uint32_t crc = btc_gen9_crc32(binary->data, binary->size);
    return (crc == binary->crc32);
}

void btc_gen9_free_binary(btc_gen9_isa_binary_t* binary) {
    if (!binary) return;
    if (binary->data) free(binary->data);
    free(binary);
}

const char* btc_gen9_get_build_log(btc_gen9_compiler_ctx_t* ctx) {
    (void)ctx;
    return g_build_log;
}

bool btc_gen9_disassemble(
    const btc_gen9_isa_binary_t* binary,
    const char* output_file
) {
    // TODO: Implémenter désassemblage avec intel-gpu-tools
    (void)binary;
    (void)output_file;
    fprintf(stderr, "[GEN9-COMPILER] Désassemblage non implémenté\n");
    return false;
}

// Made with Bob
