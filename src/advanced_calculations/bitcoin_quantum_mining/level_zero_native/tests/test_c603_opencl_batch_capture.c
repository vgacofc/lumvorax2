/*
 * TEST C603 - CAPTURE BATCH OPENCL COMPLET
 * =========================================
 * 
 * Objectif: Capturer le batch buffer OpenCL COMPLET pour comparaison
 *           byte-by-byte avec notre batch natif i915
 * 
 * Méthode: Utiliser kernel manuel ultra-simple identique au test C602
 *          pour isoler les différences d'infrastructure uniquement
 * 
 * ROOT CAUSE #153: GPGPU_WALKER DW1-2 corrigé mais problème persiste
 * 
 * Prochaine étape: Identifier TOUTES les différences entre OpenCL et natif
 */

#define CL_TARGET_OPENCL_VERSION 120
#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <unistd.h>
#include <errno.h>

// Kernel OpenCL ultra-simple (identique au kernel manuel C602)
const char *kernel_source = 
"__kernel void test_simple(__global uint *output) {\n"
"    output[0] = 0x12345678;\n"
"}\n";

// Structure pour capturer les appels ioctl
typedef struct {
    uint32_t *batch_data;
    size_t batch_size;
    int captured;
} batch_capture_t;

void print_batch_hex(uint32_t *batch, size_t size_dwords) {
    printf("\n[BATCH OPENCL CAPTURE] Batch buffer (%zu DWords = %zu bytes):\n", 
           size_dwords, size_dwords * 4);
    
    for (size_t i = 0; i < size_dwords; i += 4) {
        printf("[BATCH]   [%3zu] ", i);
        for (size_t j = 0; j < 4 && (i + j) < size_dwords; j++) {
            printf("0x%08x ", batch[i + j]);
        }
        printf("\n");
    }
}

void decode_batch_commands(uint32_t *batch, size_t size_dwords) {
    printf("\n[BATCH DECODE] Analyse des commandes:\n");
    
    size_t idx = 0;
    int cmd_num = 1;
    
    while (idx < size_dwords) {
        uint32_t dw0 = batch[idx];
        
        // PIPELINE_SELECT
        if ((dw0 & 0xFFFF0000) == 0x69040000) {
            printf("[CMD %d] PIPELINE_SELECT @ DW%zu: 0x%08x\n", cmd_num++, idx, dw0);
            idx += 1;
        }
        // STATE_BASE_ADDRESS
        else if ((dw0 & 0xFFFF0000) == 0x61010000) {
            uint32_t length = (dw0 & 0xFF) + 2;
            printf("[CMD %d] STATE_BASE_ADDRESS @ DW%zu: %u DWords\n", cmd_num++, idx, length);
            
            // Afficher les adresses importantes
            if (idx + 5 < size_dwords) {
                printf("         Surface State Base:    0x%08x%08x\n", batch[idx+6], batch[idx+5]);
            }
            if (idx + 9 < size_dwords) {
                printf("         Dynamic State Base:    0x%08x%08x\n", batch[idx+10], batch[idx+9]);
            }
            if (idx + 13 < size_dwords) {
                printf("         Instruction Base:      0x%08x%08x\n", batch[idx+14], batch[idx+13]);
            }
            
            idx += length;
        }
        // MEDIA_VFE_STATE
        else if ((dw0 & 0xFFFF0000) == 0x70000000) {
            uint32_t length = (dw0 & 0xFF) + 2;
            printf("[CMD %d] MEDIA_VFE_STATE @ DW%zu: %u DWords\n", cmd_num++, idx, length);
            
            if (idx + 1 < size_dwords) {
                printf("         Scratch Space Base:    0x%08x%08x\n", batch[idx+2], batch[idx+1]);
            }
            
            idx += length;
        }
        // MEDIA_INTERFACE_DESCRIPTOR_LOAD
        else if ((dw0 & 0xFFFF0000) == 0x61020000) {
            uint32_t length = (dw0 & 0xFF) + 2;
            printf("[CMD %d] MEDIA_INTERFACE_DESCRIPTOR_LOAD @ DW%zu: %u DWords\n", cmd_num++, idx, length);
            
            if (idx + 1 < size_dwords) {
                printf("         IDRT Base Address:     0x%08x%08x\n", batch[idx+2], batch[idx+1]);
            }
            
            idx += length;
        }
        // GPGPU_WALKER
        else if ((dw0 & 0xFFFF0000) == 0x18800000) {
            uint32_t length = (dw0 & 0xFF) + 2;
            printf("[CMD %d] GPGPU_WALKER @ DW%zu: %u DWords\n", cmd_num++, idx, length);
            
            if (idx + 2 < size_dwords) {
                printf("         Interface Desc Offset: 0x%08x%08x\n", batch[idx+2], batch[idx+1]);
            }
            if (idx + 10 < size_dwords) {
                printf("         Thread Group X/Y/Z:    %u / %u / %u\n", 
                       batch[idx+8], batch[idx+9], batch[idx+10]);
            }
            
            idx += length;
        }
        // MI_BATCH_BUFFER_END
        else if ((dw0 & 0xFF000000) == 0x05000000) {
            printf("[CMD %d] MI_BATCH_BUFFER_END @ DW%zu: 0x%08x\n", cmd_num++, idx, dw0);
            idx += 1;
            break;
        }
        // Commande inconnue
        else {
            printf("[CMD %d] UNKNOWN @ DW%zu: 0x%08x\n", cmd_num++, idx, dw0);
            idx += 1;
        }
    }
}

int main() {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║                                                            ║\n");
    printf("║  TEST C603 - CAPTURE BATCH OPENCL COMPLET                 ║\n");
    printf("║  Comparaison byte-by-byte OpenCL vs i915 natif            ║\n");
    printf("║                                                            ║\n");
    printf("║  ROOT CAUSE #153: GPGPU_WALKER corrigé mais bug persiste  ║\n");
    printf("║  Objectif: Identifier TOUTES les différences              ║\n");
    printf("║                                                            ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");

    cl_int err;
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;
    cl_program program;
    cl_kernel kernel;
    cl_mem output_buffer;

    // Initialisation OpenCL
    printf("[OPENCL] Initialisation OpenCL...\n");
    
    err = clGetPlatformIDs(1, &platform, NULL);
    if (err != CL_SUCCESS) {
        printf("❌ Erreur clGetPlatformIDs: %d\n", err);
        return 1;
    }
    
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    if (err != CL_SUCCESS) {
        printf("❌ Erreur clGetDeviceIDs: %d\n", err);
        return 1;
    }
    
    // Afficher info device
    char device_name[256];
    clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(device_name), device_name, NULL);
    printf("✓ Device OpenCL: %s\n", device_name);
    
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    if (err != CL_SUCCESS) {
        printf("❌ Erreur clCreateContext: %d\n", err);
        return 1;
    }
    
    queue = clCreateCommandQueue(context, device, 0, &err);
    if (err != CL_SUCCESS) {
        printf("❌ Erreur clCreateCommandQueue: %d\n", err);
        return 1;
    }
    
    printf("✓ OpenCL initialisé\n\n");

    // Compilation kernel
    printf("[KERNEL] Compilation kernel ultra-simple...\n");
    
    program = clCreateProgramWithSource(context, 1, &kernel_source, NULL, &err);
    if (err != CL_SUCCESS) {
        printf("❌ Erreur clCreateProgramWithSource: %d\n", err);
        return 1;
    }
    
    err = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    if (err != CL_SUCCESS) {
        printf("❌ Erreur clBuildProgram: %d\n", err);
        
        // Afficher log de compilation
        size_t log_size;
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        char *log = malloc(log_size);
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
        printf("Build log:\n%s\n", log);
        free(log);
        
        return 1;
    }
    
    kernel = clCreateKernel(program, "test_simple", &err);
    if (err != CL_SUCCESS) {
        printf("❌ Erreur clCreateKernel: %d\n", err);
        return 1;
    }
    
    printf("✓ Kernel compilé: test_simple\n\n");

    // Allocation buffer output
    printf("[BUFFER] Allocation buffer output (4096 bytes)...\n");
    
    output_buffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 4096, NULL, &err);
    if (err != CL_SUCCESS) {
        printf("❌ Erreur clCreateBuffer: %d\n", err);
        return 1;
    }
    
    // Initialiser avec pattern 0xDEADBEEF
    uint32_t pattern = 0xDEADBEEF;
    err = clEnqueueFillBuffer(queue, output_buffer, &pattern, sizeof(uint32_t), 
                              0, 4096, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        printf("❌ Erreur clEnqueueFillBuffer: %d\n", err);
        return 1;
    }
    
    printf("✓ Buffer output alloué et initialisé (pattern 0xDEADBEEF)\n\n");

    // Configuration kernel
    printf("[KERNEL] Configuration arguments...\n");
    
    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &output_buffer);
    if (err != CL_SUCCESS) {
        printf("❌ Erreur clSetKernelArg: %d\n", err);
        return 1;
    }
    
    printf("✓ Argument 0: output buffer\n\n");

    // EXÉCUTION avec capture
    printf("[EXECUTION] Lancement kernel OpenCL...\n");
    printf("⚠️  NOTE: Pour capturer le batch, utiliser strace:\n");
    printf("    strace -e ioctl -s 4096 -o opencl_batch.strace ./test_c603_opencl_batch_capture\n\n");
    
    size_t global_work_size = 1;
    size_t local_work_size = 1;
    
    err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, 
                                 &global_work_size, &local_work_size,
                                 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        printf("❌ Erreur clEnqueueNDRangeKernel: %d\n", err);
        return 1;
    }
    
    err = clFinish(queue);
    if (err != CL_SUCCESS) {
        printf("❌ Erreur clFinish: %d\n", err);
        return 1;
    }
    
    printf("✓ Kernel exécuté\n\n");

    // Lecture résultats
    printf("[RESULTATS] Lecture buffer output...\n");
    
    uint32_t output[16];
    err = clEnqueueReadBuffer(queue, output_buffer, CL_TRUE, 0, 
                              sizeof(output), output, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        printf("❌ Erreur clEnqueueReadBuffer: %d\n", err);
        return 1;
    }
    
    printf("\nOutput buffer (16 premiers DWords):\n");
    for (int i = 0; i < 16; i++) {
        if (output[i] == 0x12345678) {
            printf("  output[%2d] = 0x%08x ✅ Valeur attendue!\n", i, output[i]);
        } else if (output[i] == 0xDEADBEEF) {
            printf("  output[%2d] = 0x%08x ⚠️  Inchangé (pattern initial)\n", i, output[i]);
        } else {
            printf("  output[%2d] = 0x%08x\n", i, output[i]);
        }
    }

    // Nettoyage
    clReleaseMemObject(output_buffer);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);

    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  INSTRUCTIONS POUR CAPTURE BATCH COMPLET                  ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");
    
    printf("Pour capturer le batch buffer OpenCL complet:\n\n");
    printf("1. Exécuter avec strace:\n");
    printf("   strace -e ioctl -s 8192 -xx -o logs/opencl_batch_c603.strace \\\n");
    printf("          ./test_c603_opencl_batch_capture\n\n");
    
    printf("2. Extraire les données EXECBUFFER2:\n");
    printf("   grep 'DRM_IOCTL_I915_GEM_EXECBUFFER2' logs/opencl_batch_c603.strace > \\\n");
    printf("        logs/opencl_execbuffer2_c603.txt\n\n");
    
    printf("3. Comparer avec notre batch natif:\n");
    printf("   diff -u logs/opencl_batch_c603.txt logs/execution_c603_*.log\n\n");

    if (output[0] == 0x12345678) {
        printf("✅ TEST C603 OPENCL RÉUSSI - Kernel fonctionne parfaitement\n\n");
        return 0;
    } else {
        printf("❌ TEST C603 OPENCL ÉCHOUÉ - Résultat inattendu\n\n");
        return 1;
    }
}

// Made with Bob
