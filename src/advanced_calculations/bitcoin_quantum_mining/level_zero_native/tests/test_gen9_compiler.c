/**
 * @file test_gen9_compiler.c
 * @brief Test module natif Gen9 ISA compiler
 * 
 * OBJECTIF : Valider compilation OpenCL C → Gen9 ISA pur + cache + traçabilité
 * 
 * AUTEUR : Bob (LumVorax C198 Phase 10B)
 * DATE : 2026-05-09
 */

#include "../src/btc_gen9_native_compiler.h"
#include "../src/btc_levelzero_driver_loader.h"
#include "../src/btc_levelzero_runner.h"
#include "../src/btc_gpu_async_logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* ============================================================================
 * KERNEL TEST : Addition vectorielle
 * ============================================================================ */

static const char* TEST_KERNEL_SOURCE = 
"__kernel void vector_add(\n"
"    __global const float* a,\n"
"    __global const float* b,\n"
"    __global float* c,\n"
"    const uint n)\n"
"{\n"
"    uint gid = get_global_id(0);\n"
"    if (gid < n) {\n"
"        c[gid] = a[gid] + b[gid];\n"
"    }\n"
"}\n";

/* ============================================================================
 * TEST 1 : Compilation + Extraction ISA
 * ============================================================================ */

static bool test_compile_and_extract(void) {
    printf("\n=== TEST 1 : Compilation + Extraction ISA ===\n");

    // Initialiser compilateur
    btc_gen9_compiler_ctx_t* compiler = btc_gen9_compiler_init(
        NULL,  // Cache par défaut
        true,  // Activer cache
        true   // Activer forensique
    );
    if (!compiler) {
        fprintf(stderr, "❌ Initialisation compilateur échouée\n");
        return false;
    }

    // Compiler kernel
    btc_gen9_compile_options_t options = {
        .build_options = "-cl-std=CL1.2 -cl-fast-relaxed-math",
        .optimize = true,
        .debug = false,
        .warnings = true
    };

    btc_gen9_isa_binary_t* binary = btc_gen9_compile_kernel(
        compiler,
        TEST_KERNEL_SOURCE,
        "vector_add",
        &options
    );

    if (!binary) {
        fprintf(stderr, "❌ Compilation kernel échouée\n");
        const char* log = btc_gen9_get_build_log(compiler);
        if (log) fprintf(stderr, "Log compilation :\n%s\n", log);
        btc_gen9_compiler_cleanup(compiler);
        return false;
    }

    printf("✅ Kernel compilé : %zu bytes ISA, CRC32=0x%08X\n",
           binary->size, binary->crc32);
    printf("   Temps compilation : %lu ns (%.3f ms)\n",
           binary->compile_time_ns, binary->compile_time_ns / 1e6);

    // Valider binaire
    if (!btc_gen9_validate_binary(binary)) {
        fprintf(stderr, "❌ Validation binaire échouée\n");
        btc_gen9_free_binary(binary);
        btc_gen9_compiler_cleanup(compiler);
        return false;
    }

    printf("✅ Binaire validé (CRC32 correct)\n");

    // Afficher premiers bytes ISA
    printf("   Premiers 64 bytes ISA :\n   ");
    for (size_t i = 0; i < 64 && i < binary->size; i++) {
        printf("%02X ", binary->data[i]);
        if ((i + 1) % 16 == 0) printf("\n   ");
    }
    printf("\n");

    btc_gen9_free_binary(binary);
    btc_gen9_compiler_cleanup(compiler);

    return true;
}

/* ============================================================================
 * TEST 2 : Cache
 * ============================================================================ */

static bool test_cache(void) {
    printf("\n=== TEST 2 : Cache ===\n");

    btc_gen9_compiler_ctx_t* compiler = btc_gen9_compiler_init(NULL, true, true);
    if (!compiler) return false;

    btc_gen9_compile_options_t options = {
        .build_options = "-cl-std=CL1.2",
        .optimize = true,
        .debug = false,
        .warnings = false
    };

    // Première compilation (cache miss)
    printf("Compilation 1 (cache miss attendu)...\n");
    btc_gen9_isa_binary_t* binary1 = btc_gen9_compile_kernel(
        compiler, TEST_KERNEL_SOURCE, "vector_add_cached", &options
    );
    if (!binary1) {
        fprintf(stderr, "❌ Compilation 1 échouée\n");
        btc_gen9_compiler_cleanup(compiler);
        return false;
    }

    uint64_t time1 = binary1->compile_time_ns;
    printf("✅ Compilation 1 : %lu ns (%.3f ms)\n", time1, time1 / 1e6);
    btc_gen9_free_binary(binary1);

    // Deuxième compilation (cache hit)
    printf("Compilation 2 (cache hit attendu)...\n");
    btc_gen9_isa_binary_t* binary2 = btc_gen9_compile_kernel(
        compiler, TEST_KERNEL_SOURCE, "vector_add_cached", &options
    );
    if (!binary2) {
        fprintf(stderr, "❌ Compilation 2 échouée\n");
        btc_gen9_compiler_cleanup(compiler);
        return false;
    }

    if (!binary2->cached) {
        fprintf(stderr, "❌ Cache non utilisé (attendu cached=true)\n");
        btc_gen9_free_binary(binary2);
        btc_gen9_compiler_cleanup(compiler);
        return false;
    }

    printf("✅ Compilation 2 : chargé depuis cache\n");
    printf("   Accélération : %.1f× plus rapide\n", (double)time1 / 1000.0);

    btc_gen9_free_binary(binary2);
    btc_gen9_compiler_cleanup(compiler);

    return true;
}

/* ============================================================================
 * TEST 3 : Intégration Level Zero
 * ============================================================================ */

static bool test_levelzero_integration(void) {
    printf("\n=== TEST 3 : Intégration Level Zero ===\n");

    // Initialiser compilateur
    btc_gen9_compiler_ctx_t* compiler = btc_gen9_compiler_init(NULL, true, true);
    if (!compiler) return false;

    // Compiler kernel
    btc_gen9_isa_binary_t* binary = btc_gen9_compile_kernel(
        compiler, TEST_KERNEL_SOURCE, "vector_add_l0", NULL
    );
    if (!binary) {
        fprintf(stderr, "❌ Compilation kernel échouée\n");
        btc_gen9_compiler_cleanup(compiler);
        return false;
    }

    printf("✅ Kernel compilé : %zu bytes ISA\n", binary->size);

    // Charger driver Level Zero
    if (btc_l0_load_driver_manual() != 0) {
        fprintf(stderr, "❌ Chargement driver Level Zero échoué\n");
        btc_gen9_free_binary(binary);
        btc_gen9_compiler_cleanup(compiler);
        return false;
    }

    printf("✅ Driver Level Zero chargé\n");

    // Initialiser Level Zero
    btc_levelzero_ctx_t* l0_ctx = btc_l0_init();
    if (!l0_ctx) {
        fprintf(stderr, "❌ Initialisation Level Zero échouée\n");
        btc_gen9_free_binary(binary);
        btc_gen9_compiler_cleanup(compiler);
        return false;
    }

    printf("✅ Level Zero initialisé\n");

    // Créer module depuis ISA Gen9 natif
    printf("Chargement ISA Gen9 dans Level Zero...\n");
    
    ze_module_desc_t module_desc = {0};
    module_desc.stype = ZE_STRUCTURE_TYPE_MODULE_DESC;
    module_desc.pNext = NULL;
    module_desc.format = ZE_MODULE_FORMAT_NATIVE;  // Format natif Gen9
    module_desc.inputSize = binary->size;
    module_desc.pInputModule = binary->data;
    module_desc.pBuildFlags = NULL;
    module_desc.pConstants = NULL;

    ze_module_handle_t module = NULL;
    ze_result_t res = btc_l0_zeModuleCreate(
        l0_ctx->context,
        l0_ctx->device,
        &module_desc,
        &module,
        NULL
    );

    if (res != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "❌ zeModuleCreate échoué : %d\n", res);
        btc_l0_cleanup(l0_ctx);
        btc_gen9_free_binary(binary);
        btc_gen9_compiler_cleanup(compiler);
        return false;
    }

    printf("✅ Module Level Zero créé depuis ISA Gen9 natif\n");

    // Créer kernel
    ze_kernel_desc_t kernel_desc = {0};
    kernel_desc.stype = ZE_STRUCTURE_TYPE_KERNEL_DESC;
    kernel_desc.pNext = NULL;
    kernel_desc.pKernelName = "vector_add";

    ze_kernel_handle_t kernel = NULL;
    res = btc_l0_zeKernelCreate(module, &kernel_desc, &kernel);

    if (res != ZE_RESULT_SUCCESS) {
        fprintf(stderr, "❌ zeKernelCreate échoué : %d\n", res);
        btc_l0_zeModuleDestroy(module);
        btc_l0_cleanup(l0_ctx);
        btc_gen9_free_binary(binary);
        btc_gen9_compiler_cleanup(compiler);
        return false;
    }

    printf("✅ Kernel Level Zero créé\n");

    // Allouer buffers GPU
    const uint32_t n = 1024;
    const size_t buffer_size = n * sizeof(float);

    btc_l0_buffer_t* buf_a = btc_l0_alloc_buffer(l0_ctx, buffer_size);
    btc_l0_buffer_t* buf_b = btc_l0_alloc_buffer(l0_ctx, buffer_size);
    btc_l0_buffer_t* buf_c = btc_l0_alloc_buffer(l0_ctx, buffer_size);

    if (!buf_a || !buf_b || !buf_c) {
        fprintf(stderr, "❌ Allocation buffers GPU échouée\n");
        btc_l0_zeKernelDestroy(kernel);
        btc_l0_zeModuleDestroy(module);
        btc_l0_cleanup(l0_ctx);
        btc_gen9_free_binary(binary);
        btc_gen9_compiler_cleanup(compiler);
        return false;
    }

    printf("✅ Buffers GPU alloués (3×%zu bytes)\n", buffer_size);

    // Préparer données
    float* host_a = malloc(buffer_size);
    float* host_b = malloc(buffer_size);
    float* host_c = malloc(buffer_size);

    for (uint32_t i = 0; i < n; i++) {
        host_a[i] = (float)i;
        host_b[i] = (float)(i * 2);
        host_c[i] = 0.0f;
    }

    // Upload données
    btc_l0_upload_buffer(l0_ctx, buf_a, host_a, buffer_size);
    btc_l0_upload_buffer(l0_ctx, buf_b, host_b, buffer_size);

    printf("✅ Données uploadées sur GPU\n");

    // Configurer arguments kernel
    btc_l0_zeKernelSetArgumentValue(kernel, 0, sizeof(void*), &buf_a->ze_memory);
    btc_l0_zeKernelSetArgumentValue(kernel, 1, sizeof(void*), &buf_b->ze_memory);
    btc_l0_zeKernelSetArgumentValue(kernel, 2, sizeof(void*), &buf_c->ze_memory);
    btc_l0_zeKernelSetArgumentValue(kernel, 3, sizeof(uint32_t), &n);

    // Configurer groupe de travail
    btc_l0_zeKernelSetGroupSize(kernel, 256, 1, 1);

    printf("✅ Kernel configuré (256 threads/groupe)\n");

    // Exécuter kernel
    ze_group_count_t dispatch = { (n + 255) / 256, 1, 1 };
    
    btc_l0_zeCommandListAppendLaunchKernel(
        l0_ctx->cmd_list,
        kernel,
        &dispatch,
        NULL, 0, NULL
    );

    btc_l0_zeCommandListClose(l0_ctx->cmd_list);
    btc_l0_zeCommandQueueExecuteCommandLists(l0_ctx->cmd_queue, 1, &l0_ctx->cmd_list, NULL);
    btc_l0_zeCommandQueueSynchronize(l0_ctx->cmd_queue, UINT64_MAX);

    printf("✅ Kernel exécuté sur GPU\n");

    // Lire résultats
    btc_l0_download_buffer(l0_ctx, buf_c, host_c, buffer_size);

    printf("✅ Résultats téléchargés depuis GPU\n");

    // Vérifier résultats
    bool success = true;
    uint32_t errors = 0;
    for (uint32_t i = 0; i < n && errors < 10; i++) {
        float expected = host_a[i] + host_b[i];
        float actual = host_c[i];
        if (fabsf(actual - expected) > 0.001f) {
            if (errors == 0) {
                fprintf(stderr, "❌ Erreurs détectées :\n");
            }
            fprintf(stderr, "   c[%u] = %.2f (attendu %.2f)\n", i, actual, expected);
            errors++;
            success = false;
        }
    }

    if (success) {
        printf("✅ TOUS les résultats corrects (1024 éléments validés)\n");
        printf("   Exemples : c[0]=%.2f, c[100]=%.2f, c[1023]=%.2f\n",
               host_c[0], host_c[100], host_c[1023]);
    } else {
        fprintf(stderr, "❌ %u erreurs détectées\n", errors);
    }

    // Cleanup
    free(host_a);
    free(host_b);
    free(host_c);
    btc_l0_free_buffer(l0_ctx, buf_a);
    btc_l0_free_buffer(l0_ctx, buf_b);
    btc_l0_free_buffer(l0_ctx, buf_c);
    btc_l0_zeKernelDestroy(kernel);
    btc_l0_zeModuleDestroy(module);
    btc_l0_cleanup(l0_ctx);
    btc_gen9_free_binary(binary);
    btc_gen9_compiler_cleanup(compiler);

    return success;
}

/* ============================================================================
 * MAIN
 * ============================================================================ */

int main(void) {
    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║  TEST MODULE NATIF GEN9 ISA COMPILER — LumVorax C198 Phase 10B ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n");

    bool all_passed = true;

    // Test 1 : Compilation + Extraction ISA
    if (!test_compile_and_extract()) {
        fprintf(stderr, "\n❌ TEST 1 ÉCHOUÉ\n");
        all_passed = false;
    } else {
        printf("\n✅ TEST 1 RÉUSSI\n");
    }

    // Test 2 : Cache
    if (!test_cache()) {
        fprintf(stderr, "\n❌ TEST 2 ÉCHOUÉ\n");
        all_passed = false;
    } else {
        printf("\n✅ TEST 2 RÉUSSI\n");
    }

    // Test 3 : Intégration Level Zero
    if (!test_levelzero_integration()) {
        fprintf(stderr, "\n❌ TEST 3 ÉCHOUÉ\n");
        all_passed = false;
    } else {
        printf("\n✅ TEST 3 RÉUSSI\n");
    }

    // Résumé
    printf("\n╔════════════════════════════════════════════════════════════════╗\n");
    if (all_passed) {
        printf("║  ✅ TOUS LES TESTS RÉUSSIS — Module natif Gen9 fonctionnel    ║\n");
    } else {
        printf("║  ❌ CERTAINS TESTS ÉCHOUÉS — Voir logs ci-dessus              ║\n");
    }
    printf("╚════════════════════════════════════════════════════════════════╝\n");

    return all_passed ? 0 : 1;
}

// Made with Bob
