/**
 * TEST C620 - Validation SHA-256 avec Test Vectors Bitcoin
 * 
 * Objectif: Valider que le kernel SHA-256 calcule correctement
 * Méthode: Utiliser test vectors Bitcoin connus (Genesis Block)
 * 
 * Test Vector: Bitcoin Genesis Block
 * - Block header connu (80 bytes)
 * - Hash attendu connu (32 bytes)
 * - Si kernel correct: hash calculé == hash attendu
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <CL/cl.h>

// Test Vector: Bitcoin Genesis Block Header
// Source: https://en.bitcoin.it/wiki/Genesis_block
uint8_t genesis_header[80] = {
    // Version (4 bytes, little-endian)
    0x01, 0x00, 0x00, 0x00,
    
    // Previous block hash (32 bytes, all zeros for genesis)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    
    // Merkle root (32 bytes)
    0x3b, 0xa3, 0xed, 0xfd, 0x7a, 0x7b, 0x12, 0xb2,
    0x7a, 0xc7, 0x2c, 0x3e, 0x67, 0x76, 0x8f, 0x61,
    0x7f, 0xc8, 0x1b, 0xc3, 0x88, 0x8a, 0x51, 0x32,
    0x3a, 0x9f, 0xb8, 0xaa, 0x4b, 0x1e, 0x5e, 0x4a,
    
    // Timestamp (4 bytes): 2009-01-03 18:15:05 GMT
    0x29, 0xab, 0x5f, 0x49,
    
    // Bits (difficulty target, 4 bytes)
    0xff, 0xff, 0x00, 0x1d,
    
    // Nonce (4 bytes)
    0x1d, 0xac, 0x2b, 0x7c
};

// Expected hash (double SHA-256 of genesis header)
// Note: Bitcoin uses double SHA-256 and stores in little-endian
uint8_t expected_hash[32] = {
    0x6f, 0xe2, 0x8c, 0x0a, 0xb6, 0xf1, 0xb3, 0x72,
    0xc1, 0xa6, 0xa2, 0x46, 0xae, 0x63, 0xf7, 0x4f,
    0x93, 0x1e, 0x83, 0x65, 0xe1, 0x5a, 0x08, 0x9c,
    0x68, 0xd6, 0x19, 0x00, 0x00, 0x00, 0x00, 0x00
};

void print_hex(const char* label, uint8_t* data, size_t len) {
    printf("%s: ", label);
    for (size_t i = 0; i < len; i++) {
        printf("%02x", data[i]);
        if ((i + 1) % 8 == 0 && i + 1 < len) printf(" ");
    }
    printf("\n");
}

int main(int argc, char** argv) {
    printf("\n╔════════════════════════════════════════════════════════════╗\n");
    printf("║  TEST C620 - VALIDATION SHA-256 KERNEL                    ║\n");
    printf("║  Test Vector: Bitcoin Genesis Block                       ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");
    
    // Afficher test vector
    printf("[TEST VECTOR] Bitcoin Genesis Block\n");
    print_hex("Header (80 bytes)", genesis_header, 80);
    print_hex("Expected Hash", expected_hash, 32);
    printf("\n");
    
    // Initialiser OpenCL
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;
    cl_program program;
    cl_kernel kernel;
    cl_int err;
    
    // Get platform
    err = clGetPlatformIDs(1, &platform, NULL);
    if (err != CL_SUCCESS) {
        printf("❌ ÉCHEC: clGetPlatformIDs (err=%d)\n", err);
        return 1;
    }
    
    // Get device
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    if (err != CL_SUCCESS) {
        printf("❌ ÉCHEC: clGetDeviceIDs (err=%d)\n", err);
        return 1;
    }
    
    // Create context
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    if (err != CL_SUCCESS) {
        printf("❌ ÉCHEC: clCreateContext (err=%d)\n", err);
        return 1;
    }
    
    // Create command queue
    queue = clCreateCommandQueue(context, device, 0, &err);
    if (err != CL_SUCCESS) {
        printf("❌ ÉCHEC: clCreateCommandQueue (err=%d)\n", err);
        return 1;
    }
    
    printf("✅ OpenCL initialisé\n\n");
    
    // Charger kernel depuis fichier
    const char* kernel_path = "kernels/btc_sha256_gen9.cl";
    FILE* fp = fopen(kernel_path, "r");
    if (!fp) {
        printf("❌ ÉCHEC: Impossible d'ouvrir %s\n", kernel_path);
        printf("ℹ️  Essayez: kernels/btc_sha256_c294_final.cl\n");
        return 1;
    }
    
    fseek(fp, 0, SEEK_END);
    size_t kernel_size = ftell(fp);
    rewind(fp);
    
    char* kernel_source = (char*)malloc(kernel_size + 1);
    fread(kernel_source, 1, kernel_size, fp);
    kernel_source[kernel_size] = '\0';
    fclose(fp);
    
    printf("✅ Kernel chargé: %s (%zu bytes)\n", kernel_path, kernel_size);
    
    // Créer program
    program = clCreateProgramWithSource(context, 1, (const char**)&kernel_source, &kernel_size, &err);
    if (err != CL_SUCCESS) {
        printf("❌ ÉCHEC: clCreateProgramWithSource (err=%d)\n", err);
        free(kernel_source);
        return 1;
    }
    
    // Compiler program
    err = clBuildProgram(program, 1, &device, "-cl-std=CL1.2", NULL, NULL);
    if (err != CL_SUCCESS) {
        printf("❌ ÉCHEC: clBuildProgram (err=%d)\n", err);
        
        // Afficher log de compilation
        size_t log_size;
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        char* log = (char*)malloc(log_size);
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
        printf("\n[BUILD LOG]\n%s\n", log);
        free(log);
        
        free(kernel_source);
        return 1;
    }
    
    printf("✅ Kernel compilé\n");
    free(kernel_source);
    
    // Créer kernel
    kernel = clCreateKernel(program, "btc_sha256_mining_gen9", &err);
    if (err != CL_SUCCESS) {
        printf("❌ ÉCHEC: clCreateKernel (err=%d)\n", err);
        printf("ℹ️  Essayez un autre nom de kernel\n");
        return 1;
    }
    
    printf("✅ Kernel créé: btc_sha256_mining_gen9\n\n");
    
    // Créer buffers
    cl_mem input_buffer = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                         80, genesis_header, &err);
    if (err != CL_SUCCESS) {
        printf("❌ ÉCHEC: clCreateBuffer input (err=%d)\n", err);
        return 1;
    }
    
    uint8_t output_hash[32] = {0};
    cl_mem output_buffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY,
                                          32, NULL, &err);
    if (err != CL_SUCCESS) {
        printf("❌ ÉCHEC: clCreateBuffer output (err=%d)\n", err);
        return 1;
    }
    
    printf("✅ Buffers créés\n");
    
    // Set kernel arguments
    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &input_buffer);
    err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &output_buffer);
    if (err != CL_SUCCESS) {
        printf("❌ ÉCHEC: clSetKernelArg (err=%d)\n", err);
        return 1;
    }
    
    // Exécuter kernel
    size_t global_work_size = 1;
    err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_work_size, NULL, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        printf("❌ ÉCHEC: clEnqueueNDRangeKernel (err=%d)\n", err);
        return 1;
    }
    
    printf("✅ Kernel exécuté\n");
    
    // Lire résultat
    err = clEnqueueReadBuffer(queue, output_buffer, CL_TRUE, 0, 32, output_hash, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        printf("❌ ÉCHEC: clEnqueueReadBuffer (err=%d)\n", err);
        return 1;
    }
    
    printf("✅ Résultat lu\n\n");
    
    // Comparer résultats
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  RÉSULTATS VALIDATION                                      ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    
    print_hex("Hash attendu ", expected_hash, 32);
    print_hex("Hash calculé ", output_hash, 32);
    
    int match = memcmp(expected_hash, output_hash, 32) == 0;
    
    printf("\n");
    if (match) {
        printf("✅ SUCCESS: Kernel SHA-256 CORRECT!\n");
        printf("✅ Le kernel calcule correctement les hashes Bitcoin\n");
        printf("\n[CONCLUSION]\n");
        printf("Le problème '0 nonces trouvés' n'est PAS dû au kernel SHA-256.\n");
        printf("Causes possibles:\n");
        printf("  1. Target difficulty trop strict\n");
        printf("  2. Nonce range trop petit\n");
        printf("  3. Output buffer non lu correctement\n");
        printf("  4. Critère validation nonce incorrect\n");
        return 0;
    } else {
        printf("❌ FAIL: Kernel SHA-256 INCORRECT!\n");
        printf("❌ Le kernel ne calcule PAS correctement les hashes\n");
        printf("\n[ANALYSE DIFFÉRENCES]\n");
        
        int diff_count = 0;
        for (int i = 0; i < 32; i++) {
            if (expected_hash[i] != output_hash[i]) {
                printf("  Byte %2d: attendu=0x%02x, calculé=0x%02x (diff=%d)\n",
                       i, expected_hash[i], output_hash[i],
                       (int)output_hash[i] - (int)expected_hash[i]);
                diff_count++;
            }
        }
        
        printf("\n[CONCLUSION]\n");
        printf("ROOT CAUSE: Kernel SHA-256 incorrect (%d bytes différents)\n", diff_count);
        printf("Actions requises:\n");
        printf("  1. Vérifier implémentation SHA-256 (rounds, constants)\n");
        printf("  2. Vérifier endianness (big-endian vs little-endian)\n");
        printf("  3. Vérifier padding message\n");
        printf("  4. Comparer avec implémentation référence\n");
        return 1;
    }
    
    // Cleanup
    clReleaseMemObject(input_buffer);
    clReleaseMemObject(output_buffer);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
}

// Made with Bob
