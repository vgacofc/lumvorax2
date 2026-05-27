/**
 * TEST C606 - VALIDATION LEVEL ZERO EU DISPATCH
 * 
 * Objectif: Prouver que Level Zero peut exécuter notre kernel simple
 *           et obtenir résultat ≠ 0xDEADBEEF
 * 
 * Stratégie:
 * 1. Utiliser infrastructure Level Zero existante (btc_levelzero_runner.c)
 * 2. Compiler kernel OpenCL C simple: output[0] = 0x12345678
 * 3. Exécuter sur GPU via Level Zero
 * 4. Vérifier résultat = 0x12345678 (SUCCÈS EU DISPATCH)
 * 
 * Si succès: Level Zero fonctionne → Intégration hybride possible
 * Si échec: Problème plus profond → Analyser logs Level Zero
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <CL/cl.h>
#include "../include/btc_levelzero_runner.h"

// Kernel OpenCL C ultra-simple
const char* KERNEL_SOURCE = 
"__kernel void test_simple(__global uint* output) {\n"
"    output[0] = 0x12345678;\n"
"}\n";

int main(void) {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║                                                            ║\n");
    printf("║  TEST C606 - VALIDATION LEVEL ZERO EU DISPATCH            ║\n");
    printf("║  Objectif: Prouver exécution GPU via Level Zero           ║\n");
    printf("║                                                            ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    printf("\n");

    // ═══════════════════════════════════════════════════════════════
    // ÉTAPE 1: COMPILATION KERNEL AVEC OPENCL
    // ═══════════════════════════════════════════════════════════════
    
    printf("[C606] Étape 1/5: Compilation kernel OpenCL...\n");
    
    cl_int err;
    cl_platform_id platform;
    cl_device_id device;
    cl_context cl_context;
    cl_program program;
    
    // Initialisation OpenCL
    err = clGetPlatformIDs(1, &platform, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[C606] ❌ ERREUR: clGetPlatformIDs = %d\n", err);
        return 1;
    }
    
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[C606] ❌ ERREUR: clGetDeviceIDs = %d\n", err);
        return 1;
    }
    
    cl_context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[C606] ❌ ERREUR: clCreateContext = %d\n", err);
        return 1;
    }
    
    // Compilation kernel
    size_t source_len = strlen(KERNEL_SOURCE);
    program = clCreateProgramWithSource(cl_context, 1, &KERNEL_SOURCE, &source_len, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[C606] ❌ ERREUR: clCreateProgramWithSource = %d\n", err);
        clReleaseContext(cl_context);
        return 1;
    }
    
    err = clBuildProgram(program, 1, &device, "-cl-std=CL2.0", NULL, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[C606] ❌ ERREUR: clBuildProgram = %d\n", err);
        
        // Afficher log compilation
        size_t log_size;
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        char* log = malloc(log_size);
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
        fprintf(stderr, "[C606] Build log:\n%s\n", log);
        free(log);
        
        clReleaseProgram(program);
        clReleaseContext(cl_context);
        return 1;
    }
    
    printf("[C606] ✅ Kernel compilé avec succès\n");
    
    // Extraire binaire Gen9 ISA
    size_t binary_size;
    err = clGetProgramInfo(program, CL_PROGRAM_BINARY_SIZES, sizeof(size_t), &binary_size, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[C606] ❌ ERREUR: clGetProgramInfo(BINARY_SIZES) = %d\n", err);
        clReleaseProgram(program);
        clReleaseContext(cl_context);
        return 1;
    }
    
    uint8_t* kernel_binary = malloc(binary_size);
    unsigned char* binaries[1] = { kernel_binary };
    
    err = clGetProgramInfo(program, CL_PROGRAM_BINARIES, sizeof(unsigned char*), binaries, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[C606] ❌ ERREUR: clGetProgramInfo(BINARIES) = %d\n", err);
        free(kernel_binary);
        clReleaseProgram(program);
        clReleaseContext(cl_context);
        return 1;
    }
    
    printf("[C606] ✅ Binaire Gen9 ISA extrait: %zu bytes\n", binary_size);
    
    // Cleanup OpenCL
    clReleaseProgram(program);
    clReleaseContext(cl_context);
    
    // ═══════════════════════════════════════════════════════════════
    // ÉTAPE 2: INITIALISATION LEVEL ZERO
    // ═══════════════════════════════════════════════════════════════
    
    printf("\n[C606] Étape 2/5: Initialisation Level Zero...\n");
    
    btc_l0_config_t config = {
        .batch_size = 1,
        .work_group_size = 1,
        .num_work_groups = 1,
        .enable_profiling = true,
        .enable_forensic_log = true,
        .forensic_log_path = "logs/forensic/test_c606_levelzero.lum",
        .timeout_ms = 5000
    };
    
    btc_l0_context_t* l0_ctx = btc_l0_init(&config);
    if (!l0_ctx) {
        fprintf(stderr, "[C606] ❌ ERREUR: btc_l0_init() échoué\n");
        free(kernel_binary);
        return 1;
    }
    
    printf("[C606] ✅ Level Zero initialisé\n");
    printf("[C606]   Device: %s\n", l0_ctx->device_name);
    printf("[C606]   Compute Units: %u\n", l0_ctx->max_compute_units);
    
    // ═══════════════════════════════════════════════════════════════
    // ÉTAPE 3: CRÉATION MODULE ET KERNEL LEVEL ZERO
    // ═══════════════════════════════════════════════════════════════
    
    printf("\n[C606] Étape 3/5: Création module Level Zero...\n");
    
    // Créer module depuis binaire Gen9 ISA
    ze_module_desc_t module_desc;
    memset(&module_desc, 0, sizeof(module_desc));
    module_desc.stype = ZE_STRUCTURE_TYPE_MODULE_DESC;
    module_desc.pNext = NULL;
    module_desc.format = ZE_MODULE_FORMAT_NATIVE;
    module_desc.inputSize = binary_size;
    module_desc.pInputModule = kernel_binary;
    module_desc.pBuildFlags = NULL;
    module_desc.pConstants = NULL;
    
    ze_module_handle_t module;
    ze_module_build_log_handle_t build_log;
    ze_result_t ze_ret = btc_l0_zeModuleCreate(
        (ze_context_handle_t)l0_ctx->ze_context,
        (ze_device_handle_t)l0_ctx->ze_device,
        &module_desc,
        &module,
        &build_log
    );
    
    if (ze_ret != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "[C606] ❌ ERREUR: zeModuleCreate = %d\n", ze_ret);
        
        // Afficher build log si disponible
        if (build_log) {
            size_t log_size = 0;
            btc_l0_zeModuleBuildLogGetString(build_log, &log_size, NULL);
            if (log_size > 0) {
                char* log = malloc(log_size);
                btc_l0_zeModuleBuildLogGetString(build_log, &log_size, log);
                fprintf(stderr, "[C606] Build log:\n%s\n", log);
                free(log);
            }
            btc_l0_zeModuleBuildLogDestroy(build_log);
        }
        
        free(kernel_binary);
        btc_l0_cleanup(l0_ctx);
        return 1;
    }
    
    printf("[C606] ✅ Module Level Zero créé\n");
    
    // Créer kernel
    ze_kernel_desc_t kernel_desc;
    memset(&kernel_desc, 0, sizeof(kernel_desc));
    kernel_desc.stype = ZE_STRUCTURE_TYPE_KERNEL_DESC;
    kernel_desc.pNext = NULL;
    kernel_desc.flags = 0;
    kernel_desc.pKernelName = "test_simple";
    
    ze_kernel_handle_t kernel;
    ze_ret = btc_l0_zeKernelCreate(module, &kernel_desc, &kernel);
    if (ze_ret != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "[C606] ❌ ERREUR: zeKernelCreate = %d\n", ze_ret);
        btc_l0_zeModuleDestroy(&module);
        free(kernel_binary);
        btc_l0_cleanup(l0_ctx);
        return 1;
    }
    
    printf("[C606] ✅ Kernel Level Zero créé\n");
    
    // ═══════════════════════════════════════════════════════════════
    // ÉTAPE 4: ALLOCATION BUFFER ET CONFIGURATION KERNEL
    // ═══════════════════════════════════════════════════════════════
    
    printf("\n[C606] Étape 4/5: Configuration kernel...\n");
    
    // Allouer buffer output
    btc_l0_buffer_t* output_buffer = btc_l0_alloc_buffer(l0_ctx, 4096, "output");
    if (!output_buffer) {
        fprintf(stderr, "[C606] ❌ ERREUR: Allocation buffer output échouée\n");
        btc_l0_zeKernelDestroy(&kernel);
        btc_l0_zeModuleDestroy(&module);
        free(kernel_binary);
        btc_l0_cleanup(l0_ctx);
        return 1;
    }
    
    printf("[C606] ✅ Buffer output alloué: %zu bytes\n", output_buffer->size);
    
    // Initialiser buffer avec 0xDEADBEEF
    uint32_t init_data[1024];
    for (int i = 0; i < 1024; i++) {
        init_data[i] = 0xDEADBEEF;
    }
    
    int ret = btc_l0_write_buffer(l0_ctx, output_buffer, init_data, sizeof(init_data));
    if (ret != BTC_L0_OK) {
        fprintf(stderr, "[C606] ❌ ERREUR: Initialisation buffer échouée\n");
        btc_l0_free_buffer(l0_ctx, output_buffer);
        btc_l0_zeKernelDestroy(&kernel);
        btc_l0_zeModuleDestroy(&module);
        free(kernel_binary);
        btc_l0_cleanup(l0_ctx);
        return 1;
    }
    
    printf("[C606] ✅ Buffer initialisé avec 0xDEADBEEF\n");
    
    // Configurer argument kernel
    ze_ret = btc_l0_zeKernelSetArgumentValue(kernel, 0, sizeof(void*), &output_buffer->ze_memory);
    if (ze_ret != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "[C606] ❌ ERREUR: zeKernelSetArgumentValue = %d\n", ze_ret);
        btc_l0_free_buffer(l0_ctx, output_buffer);
        btc_l0_zeKernelDestroy(&kernel);
        btc_l0_zeModuleDestroy(&module);
        free(kernel_binary);
        btc_l0_cleanup(l0_ctx);
        return 1;
    }
    
    // Configurer group size
    ze_ret = btc_l0_zeKernelSetGroupSize(kernel, 1, 1, 1);
    if (ze_ret != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "[C606] ❌ ERREUR: zeKernelSetGroupSize = %d\n", ze_ret);
        btc_l0_free_buffer(l0_ctx, output_buffer);
        btc_l0_zeKernelDestroy(&kernel);
        btc_l0_zeModuleDestroy(&module);
        free(kernel_binary);
        btc_l0_cleanup(l0_ctx);
        return 1;
    }
    
    printf("[C606] ✅ Kernel configuré (1x1x1 work-items)\n");
    
    // ═══════════════════════════════════════════════════════════════
    // ÉTAPE 5: EXÉCUTION KERNEL SUR GPU
    // ═══════════════════════════════════════════════════════════════
    
    printf("\n[C606] Étape 5/5: Exécution kernel sur GPU...\n");
    
    // Reset command list
    ze_ret = btc_l0_zeCommandListReset((ze_command_list_handle_t)l0_ctx->ze_command_list);
    if (ze_ret != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "[C606] ❌ ERREUR: zeCommandListReset = %d\n", ze_ret);
        btc_l0_free_buffer(l0_ctx, output_buffer);
        btc_l0_zeKernelDestroy(&kernel);
        btc_l0_zeModuleDestroy(&module);
        free(kernel_binary);
        btc_l0_cleanup(l0_ctx);
        return 1;
    }
    
    // Append kernel launch
    ze_group_count_t group_count = { 1, 1, 1 };
    ze_ret = btc_l0_zeCommandListAppendLaunchKernel(
        (ze_command_list_handle_t)l0_ctx->ze_command_list,
        kernel,
        &group_count,
        NULL, 0, NULL
    );
    
    if (ze_ret != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "[C606] ❌ ERREUR: zeCommandListAppendLaunchKernel = %d\n", ze_ret);
        btc_l0_free_buffer(l0_ctx, output_buffer);
        btc_l0_zeKernelDestroy(&kernel);
        btc_l0_zeModuleDestroy(&module);
        free(kernel_binary);
        btc_l0_cleanup(l0_ctx);
        return 1;
    }
    
    // Close command list
    ze_ret = btc_l0_zeCommandListClose((ze_command_list_handle_t)l0_ctx->ze_command_list);
    if (ze_ret != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "[C606] ❌ ERREUR: zeCommandListClose = %d\n", ze_ret);
        btc_l0_free_buffer(l0_ctx, output_buffer);
        btc_l0_zeKernelDestroy(&kernel);
        btc_l0_zeModuleDestroy(&module);
        free(kernel_binary);
        btc_l0_cleanup(l0_ctx);
        return 1;
    }
    
    // Execute command list
    ze_ret = btc_l0_zeCommandQueueExecuteCommandLists(
        (ze_command_queue_handle_t)l0_ctx->ze_command_queue,
        1,
        (ze_command_list_handle_t*)&l0_ctx->ze_command_list,
        NULL
    );
    
    if (ze_ret != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "[C606] ❌ ERREUR: zeCommandQueueExecuteCommandLists = %d\n", ze_ret);
        btc_l0_free_buffer(l0_ctx, output_buffer);
        btc_l0_zeKernelDestroy(&kernel);
        btc_l0_zeModuleDestroy(&module);
        free(kernel_binary);
        btc_l0_cleanup(l0_ctx);
        return 1;
    }
    
    // Synchronize
    uint64_t timeout_ns = 5000000000ULL; // 5 secondes
    ze_ret = btc_l0_zeCommandQueueSynchronize(
        (ze_command_queue_handle_t)l0_ctx->ze_command_queue,
        timeout_ns
    );
    
    if (ze_ret == ZE_RESULT_NOT_READY) {
        fprintf(stderr, "[C606] ❌ TIMEOUT: Kernel bloqué > 5 secondes\n");
        btc_l0_free_buffer(l0_ctx, output_buffer);
        btc_l0_zeKernelDestroy(&kernel);
        btc_l0_zeModuleDestroy(&module);
        free(kernel_binary);
        btc_l0_cleanup(l0_ctx);
        return 1;
    }
    
    if (ze_ret != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "[C606] ❌ ERREUR: zeCommandQueueSynchronize = %d\n", ze_ret);
        btc_l0_free_buffer(l0_ctx, output_buffer);
        btc_l0_zeKernelDestroy(&kernel);
        btc_l0_zeModuleDestroy(&module);
        free(kernel_binary);
        btc_l0_cleanup(l0_ctx);
        return 1;
    }
    
    printf("[C606] ✅ Kernel exécuté avec succès\n");
    
    // ═══════════════════════════════════════════════════════════════
    // VÉRIFICATION RÉSULTATS
    // ═══════════════════════════════════════════════════════════════
    
    printf("\n[C606] Lecture résultats...\n");
    
    uint32_t results[1024];
    ret = btc_l0_read_buffer(l0_ctx, output_buffer, results, sizeof(results));
    if (ret != BTC_L0_OK) {
        fprintf(stderr, "[C606] ❌ ERREUR: Lecture buffer échouée\n");
        btc_l0_free_buffer(l0_ctx, output_buffer);
        btc_l0_zeKernelDestroy(&kernel);
        btc_l0_zeModuleDestroy(&module);
        free(kernel_binary);
        btc_l0_cleanup(l0_ctx);
        return 1;
    }
    
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  RÉSULTATS TEST C606                                       ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    printf("Valeur attendue: 0x12345678\n");
    printf("Valeur obtenue:  0x%08X\n", results[0]);
    printf("\n");
    
    bool success = (results[0] == 0x12345678);
    
    if (success) {
        printf("✅ ✅ ✅ SUCCÈS EU DISPATCH LEVEL ZERO! ✅ ✅ ✅\n");
        printf("\n");
        printf("DÉCOUVERTE MAJEURE:\n");
        printf("- Level Zero peut exécuter kernel sur GPU\n");
        printf("- EUs sont correctement dispatchés\n");
        printf("- Résultats GPU valides obtenus\n");
        printf("- Intégration hybride Level Zero + DRM natif POSSIBLE\n");
        printf("\n");
        printf("PROCHAINE ÉTAPE C607:\n");
        printf("- Capturer batch buffers Level Zero (strace)\n");
        printf("- Comparer avec batch DRM natif\n");
        printf("- Identifier différences critiques\n");
        printf("- Corriger batch_generator_auto\n");
    } else {
        printf("❌ ÉCHEC: Résultat incorrect\n");
        printf("\n");
        printf("Analyse:\n");
        if (results[0] == 0xDEADBEEF) {
            printf("- Valeur initiale inchangée\n");
            printf("- Kernel non exécuté OU EUs non dispatchés\n");
            printf("- Problème plus profond que prévu\n");
        } else {
            printf("- Valeur modifiée mais incorrecte\n");
            printf("- Kernel partiellement exécuté?\n");
            printf("- Corruption mémoire?\n");
        }
    }
    
    printf("\n");
    
    // Cleanup
    btc_l0_free_buffer(l0_ctx, output_buffer);
    btc_l0_zeKernelDestroy(&kernel);
    btc_l0_zeModuleDestroy(&module);
    free(kernel_binary);
    btc_l0_cleanup(l0_ctx);
    
    printf("✅ TEST C606 TERMINÉ\n");
    printf("\n");
    
    return success ? 0 : 1;
}

// Made with Bob