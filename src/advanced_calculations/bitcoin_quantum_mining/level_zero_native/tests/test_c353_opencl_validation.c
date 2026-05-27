/*
 * Test C353 - Validation SHA-256 CPU vs GPU (OpenCL)
 * Compile: gcc -o tests/test_c353_opencl_validation tests/test_c353_opencl_validation.c -lOpenCL -lssl -lcrypto
 * Run: ./tests/test_c353_opencl_validation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <CL/cl.h>
#include <openssl/sha.h>

#define CHECK_CL(err, msg) if(err != CL_SUCCESS) { \
    fprintf(stderr, "❌ %s: error %d\n", msg, err); \
    exit(1); \
}

/* Compter leading zeros dans hash */
uint32_t count_leading_zeros_cpu(const uint8_t* hash) {
    uint32_t zeros = 0;
    for (int i = 0; i < 32; i++) {
        if (hash[i] == 0) {
            zeros += 8;
        } else {
            uint8_t byte = hash[i];
            while ((byte & 0x80) == 0) {
                zeros++;
                byte <<= 1;
            }
            break;
        }
    }
    return zeros;
}

/* SHA-256 double avec OpenSSL */
void cpu_sha256_double(const uint8_t* data, size_t len, uint8_t* hash) {
    uint8_t temp[32];
    SHA256(data, len, temp);
    SHA256(temp, 32, hash);
}

int main() {
    cl_int err;
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;
    cl_program program;
    cl_kernel kernel;
    
    printf("╔═══════════════════════════════════════════════════╗\n");
    printf("║  Test C353 - Validation SHA-256 CPU vs GPU       ║\n");
    printf("╚═══════════════════════════════════════════════════╝\n\n");
    
    /* Bitcoin block header test */
    uint8_t header[80] = {
        0x01, 0x00, 0x00, 0x00, /* version */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* prev block */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* merkle root */
        0x00, 0x00, 0x00, 0x00, /* timestamp */
        0xff, 0xff, 0x00, 0x1d, /* bits */
        0x00, 0x00, 0x00, 0x00  /* nonce (will be modified) */
    };
    
    /* Test CPU avec 6 nonces */
    printf("═══ TEST CPU (OpenSSL) ═══\n");
    for (uint32_t nonce = 0; nonce < 6; nonce++) {
        /* Mettre nonce dans header (little-endian) */
        header[76] = nonce & 0xFF;
        header[77] = (nonce >> 8) & 0xFF;
        header[78] = (nonce >> 16) & 0xFF;
        header[79] = (nonce >> 24) & 0xFF;
        
        /* Calculer hash */
        uint8_t hash[32];
        cpu_sha256_double(header, 80, hash);
        
        /* Compter zeros */
        uint32_t zeros = count_leading_zeros_cpu(hash);
        
        printf("  nonce=%u → %u leading zeros\n", nonce, zeros);
        
        /* Afficher hash si zeros > 0 */
        if (zeros > 0) {
            printf("    hash: ");
            for (int i = 0; i < 32; i++) printf("%02x", hash[i]);
            printf("\n");
        }
    }
    
    /* Setup OpenCL */
    printf("\n═══ SETUP GPU (OpenCL) ═══\n");
    
    err = clGetPlatformIDs(1, &platform, NULL);
    CHECK_CL(err, "clGetPlatformIDs");
    
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    CHECK_CL(err, "clGetDeviceIDs");
    
    char device_name[128];
    clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(device_name), device_name, NULL);
    printf("✅ Device: %s\n", device_name);
    
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    CHECK_CL(err, "clCreateContext");
    
    queue = clCreateCommandQueue(context, device, 0, &err);
    CHECK_CL(err, "clCreateCommandQueue");
    
    /* Charger kernel C353 */
    FILE* f = fopen("kernels/btc_sha256_c353_fixed.cl", "r");
    if (!f) {
        fprintf(stderr, "❌ Cannot open kernel file\n");
        return 1;
    }
    
    fseek(f, 0, SEEK_END);
    size_t kernel_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char* kernel_source = malloc(kernel_size + 1);
    fread(kernel_source, 1, kernel_size, f);
    kernel_source[kernel_size] = '\0';
    fclose(f);
    
    printf("✅ Kernel loaded: %zu bytes\n", kernel_size);
    
    /* Compiler kernel */
    program = clCreateProgramWithSource(context, 1, (const char**)&kernel_source, &kernel_size, &err);
    CHECK_CL(err, "clCreateProgramWithSource");
    
    err = clBuildProgram(program, 1, &device, "-cl-std=CL2.0", NULL, NULL);
    if (err != CL_SUCCESS) {
        char build_log[4096];
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, sizeof(build_log), build_log, NULL);
        fprintf(stderr, "❌ Build failed:\n%s\n", build_log);
        return 1;
    }
    printf("✅ Kernel compiled\n");
    
    kernel = clCreateKernel(program, "btc_sha256_mining_gen9", &err);
    CHECK_CL(err, "clCreateKernel");
    
    /* Créer buffers */
    uint32_t header_uint[20];
    memcpy(header_uint, header, 80);
    
    cl_mem input_buf = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                      80, header_uint, &err);
    CHECK_CL(err, "clCreateBuffer input");
    
    /* Target Bitcoin (difficulté minimale) */
    uint32_t target[8] = {
        0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
        0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00FFFFFF
    };
    
    cl_mem target_buf = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                       32, target, &err);
    CHECK_CL(err, "clCreateBuffer target");
    
    uint32_t output[16] = {0};
    cl_mem output_buf = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
                                       sizeof(output), output, &err);
    CHECK_CL(err, "clCreateBuffer output");
    
    /* Configurer kernel */
    uint32_t start_nonce = 0;
    uint32_t batch_size = 6;
    
    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &input_buf);
    CHECK_CL(err, "clSetKernelArg 0");
    
    err = clSetKernelArg(kernel, 1, sizeof(cl_mem), &target_buf);
    CHECK_CL(err, "clSetKernelArg 1");
    
    err = clSetKernelArg(kernel, 2, sizeof(uint32_t), &start_nonce);
    CHECK_CL(err, "clSetKernelArg 2");
    
    err = clSetKernelArg(kernel, 3, sizeof(cl_mem), &output_buf);
    CHECK_CL(err, "clSetKernelArg 3");
    
    /* Exécuter kernel */
    printf("\n═══ TEST GPU (Kernel C353) ═══\n");
    
    size_t global_work_size = 64;  /* Doit être multiple de 64 */
    size_t local_work_size = 64;   /* reqd_work_group_size(64,1,1) */
    
    err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_work_size, &local_work_size, 0, NULL, NULL);
    CHECK_CL(err, "clEnqueueNDRangeKernel");
    
    err = clFinish(queue);
    CHECK_CL(err, "clFinish");
    
    /* Lire résultats */
    err = clEnqueueReadBuffer(queue, output_buf, CL_TRUE, 0, sizeof(output), output, 0, NULL, NULL);
    CHECK_CL(err, "clEnqueueReadBuffer");
    
    printf("✅ GPU execution complete\n");
    printf("  output[0] (best_nonce) = %u\n", output[0]);
    printf("  output[1] (best_zeros) = %u\n", output[1]);
    
    /* Afficher hash complet */
    printf("  output[2-9] (hash) = ");
    for (int i = 2; i < 10; i++) {
        printf("%08x ", output[i]);
    }
    printf("\n");
    
    /* Comparaison */
    printf("\n═══ COMPARAISON CPU vs GPU ═══\n");
    
    /* Recalculer CPU pour nonce trouvé par GPU */
    uint32_t gpu_nonce = output[0];
    uint32_t gpu_zeros = output[1];
    
    header[76] = gpu_nonce & 0xFF;
    header[77] = (gpu_nonce >> 8) & 0xFF;
    header[78] = (gpu_nonce >> 16) & 0xFF;
    header[79] = (gpu_nonce >> 24) & 0xFF;
    
    uint8_t cpu_hash[32];
    cpu_sha256_double(header, 80, cpu_hash);
    uint32_t cpu_zeros = count_leading_zeros_cpu(cpu_hash);
    
    printf("  CPU: nonce=%u → %u zeros\n", gpu_nonce, cpu_zeros);
    printf("    hash: ");
    for (int i = 0; i < 32; i++) printf("%02x", cpu_hash[i]);
    printf("\n");
    
    printf("  GPU: nonce=%u → %u zeros\n", gpu_nonce, gpu_zeros);
    
    /* Vérifier aussi nonce=4 qui devrait avoir 6 zeros */
    printf("\n═══ VÉRIFICATION NONCE=4 (devrait avoir 6 zeros) ═══\n");
    header[76] = 4;
    header[77] = 0;
    header[78] = 0;
    header[79] = 0;
    
    cpu_sha256_double(header, 80, cpu_hash);
    cpu_zeros = count_leading_zeros_cpu(cpu_hash);
    
    printf("  CPU: nonce=4 → %u zeros\n", cpu_zeros);
    printf("    hash: ");
    for (int i = 0; i < 32; i++) printf("%02x", cpu_hash[i]);
    printf("\n");
    
    /* Vérifier si GPU a trouvé le meilleur (nonce=4 avec 6 zeros) */
    if (gpu_nonce == 4 && gpu_zeros == 6) {
        printf("\n✅ SUCCESS: GPU a trouvé le meilleur nonce (4 avec 6 zeros)!\n");
    } else if (gpu_zeros >= 6) {
        printf("\n✅ PARTIAL SUCCESS: GPU a trouvé %u zeros (attendu: 6)\n", gpu_zeros);
    } else {
        printf("\n❌ FAILURE: GPU n'a pas trouvé le meilleur nonce\n");
        printf("   Attendu: nonce=4 avec 6 zeros\n");
        printf("   Trouvé:  nonce=%u avec %u zeros\n", gpu_nonce, gpu_zeros);
    }
    
    /* Cleanup */
    clReleaseMemObject(input_buf);
    clReleaseMemObject(target_buf);
    clReleaseMemObject(output_buf);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    free(kernel_source);
    
    return (cpu_zeros == gpu_zeros) ? 0 : 1;
}

// Made with Bob
