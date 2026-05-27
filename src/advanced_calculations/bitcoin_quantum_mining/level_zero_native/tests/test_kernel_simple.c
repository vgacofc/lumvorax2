/*
 * LumVorax — Module 17 — Bitcoin Quantum Mining Engine
 * test_kernel_simple.c — Test Compilation et Exécution Kernel Level Zero
 *
 * CYCLE C198 — Phase 9 — Validation Kernel vector_add
 *
 * Test complet :
 * 1. Initialisation Level Zero
 * 2. Allocation buffers GPU (a, b, c)
 * 3. Compilation kernel OpenCL C → SPIR-V
 * 4. Configuration arguments
 * 5. Dispatch kernel sur GPU
 * 6. Validation résultats
 * 7. Traçabilité forensique complète
 */

#include "btc_levelzero_runner.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define TEST_SIZE 1024  /* 1K éléments */

int main(int argc, char** argv) {
    fprintf(stderr, "\n");
    fprintf(stderr, "═══════════════════════════════════════════════════════════════\n");
    fprintf(stderr, "  TEST KERNEL LEVEL ZERO — vector_add (C198 Phase 9)\n");
    fprintf(stderr, "═══════════════════════════════════════════════════════════════\n\n");
    
    int ret = 0;
    
    /* ═══════════════════════════════════════════════════════════════════
     * ÉTAPE 1 : Initialisation Level Zero
     * ═══════════════════════════════════════════════════════════════════ */
    
    fprintf(stderr, "[1/7] Initialisation Level Zero...\n");
    
    btc_l0_config_t config = {
        .batch_size = BTC_L0_BATCH_DEFAULT,
        .work_group_size = 256,
        .num_work_groups = 4,
        .enable_profiling = true,
        .enable_forensic_log = true,
        .timeout_ms = 5000
    };
    strncpy(config.forensic_log_path, "logs/forensic/test_kernel_c198.lum",
            sizeof(config.forensic_log_path) - 1);
    
    btc_l0_context_t* ctx = btc_l0_init(&config);
    if (!ctx) {
        fprintf(stderr, "❌ ERREUR : btc_l0_init() échoué\n");
        return 1;
    }
    
    fprintf(stderr, "✅ Contexte Level Zero initialisé\n\n");
    
    /* ═══════════════════════════════════════════════════════════════════
     * ÉTAPE 2 : Allocation buffers GPU
     * ═══════════════════════════════════════════════════════════════════ */
    
    fprintf(stderr, "[2/7] Allocation buffers GPU (%d éléments)...\n", TEST_SIZE);
    
    size_t buffer_size = TEST_SIZE * sizeof(float);
    
    /* Alloue buffers GPU */
    btc_l0_buffer_t* buf_a = btc_l0_alloc_buffer(ctx, buffer_size, "input_a");
    btc_l0_buffer_t* buf_b = btc_l0_alloc_buffer(ctx, buffer_size, "input_b");
    btc_l0_buffer_t* buf_c = btc_l0_alloc_buffer(ctx, buffer_size, "output_c");
    
    if (!buf_a || !buf_b || !buf_c) {
        fprintf(stderr, "❌ ERREUR : Allocation buffers échouée\n");
        btc_l0_cleanup(ctx);
        return 1;
    }
    
    fprintf(stderr, "✅ 3 buffers alloués (%zu bytes chacun)\n\n", buffer_size);
    
    /* ═══════════════════════════════════════════════════════════════════
     * ÉTAPE 3 : Préparation données host
     * ═══════════════════════════════════════════════════════════════════ */
    
    fprintf(stderr, "[3/7] Préparation données host...\n");
    
    float* host_a = malloc(buffer_size);
    float* host_b = malloc(buffer_size);
    float* host_c = malloc(buffer_size);
    
    if (!host_a || !host_b || !host_c) {
        fprintf(stderr, "❌ ERREUR : malloc() host échoué\n");
        btc_l0_free_buffer(ctx, buf_a);
        btc_l0_free_buffer(ctx, buf_b);
        btc_l0_free_buffer(ctx, buf_c);
        btc_l0_cleanup(ctx);
        return 1;
    }
    
    /* Initialise données */
    for (int i = 0; i < TEST_SIZE; i++) {
        host_a[i] = (float)i;
        host_b[i] = (float)(i * 2);
        host_c[i] = 0.0f;
    }
    
    fprintf(stderr, "✅ Données initialisées (a[i]=i, b[i]=i*2)\n\n");
    
    /* Upload vers GPU */
    fprintf(stderr, "[4/7] Upload données vers GPU...\n");
    
    if (btc_l0_write_buffer(ctx, buf_a, host_a, buffer_size) != BTC_L0_OK ||
        btc_l0_write_buffer(ctx, buf_b, host_b, buffer_size) != BTC_L0_OK) {
        fprintf(stderr, "❌ ERREUR : Upload GPU échoué\n");
        ret = 1;
        goto cleanup;
    }
    
    fprintf(stderr, "✅ Données uploadées sur GPU\n\n");
    
    /* ═══════════════════════════════════════════════════════════════════
     * ÉTAPE 5 : Compilation kernel
     * ═══════════════════════════════════════════════════════════════════ */
    
    fprintf(stderr, "[5/7] Compilation kernel OpenCL C...\n");
    
    ze_module_handle_t module = NULL;
    /* BUG #11 FIX: Utilise compilateur en ligne Intel (OpenCL C → SPIR-V → ISA) */
    if (btc_l0_compile_kernel(ctx, "kernels/test_add.cl", "-cl-std=CL2.0", &module) != BTC_L0_OK) {
        fprintf(stderr, "❌ ERREUR : Compilation kernel échouée\n");
        ret = 1;
        goto cleanup;
    }
    
    fprintf(stderr, "✅ Kernel compilé avec succès\n\n");
    
    /* Crée kernel */
    ze_kernel_handle_t kernel = NULL;
    if (btc_l0_create_kernel(ctx, module, "vector_add", &kernel) != BTC_L0_OK) {
        fprintf(stderr, "❌ ERREUR : Création kernel échouée\n");
        btc_l0_destroy_module(ctx, module);
        ret = 1;
        goto cleanup;
    }
    
    fprintf(stderr, "✅ Kernel 'vector_add' créé\n\n");
    
    /* ═══════════════════════════════════════════════════════════════════
     * ÉTAPE 6 : Configuration et exécution kernel
     * ═══════════════════════════════════════════════════════════════════ */
    
    fprintf(stderr, "[6/7] Configuration et dispatch kernel...\n");
    
    /* Configure arguments */
    uint32_t n = TEST_SIZE;
    
    /* Pour Level Zero, on passe directement le pointeur ze_memory */
    void* ptr_a = buf_a->ze_memory;
    void* ptr_b = buf_b->ze_memory;
    void* ptr_c = buf_c->ze_memory;
    
    btc_l0_set_kernel_arg(ctx, kernel, 0, sizeof(void*), &ptr_a);
    btc_l0_set_kernel_arg(ctx, kernel, 1, sizeof(void*), &ptr_b);
    btc_l0_set_kernel_arg(ctx, kernel, 2, sizeof(void*), &ptr_c);
    btc_l0_set_kernel_arg(ctx, kernel, 3, sizeof(uint32_t), &n);
    
    /* Configure work-group size (256 threads/group) */
    btc_l0_set_group_size(ctx, kernel, 256, 1, 1);
    
    /* Calcule nombre de groups */
    uint32_t num_groups = (TEST_SIZE + 255) / 256;
    
    /* Dispatch kernel */
    if (btc_l0_execute_kernel(ctx, kernel, num_groups, 1, 1) != BTC_L0_OK) {
        fprintf(stderr, "❌ ERREUR : Exécution kernel échouée\n");
        btc_l0_destroy_kernel(ctx, kernel);
        btc_l0_destroy_module(ctx, module);
        ret = 1;
        goto cleanup;
    }
    
    fprintf(stderr, "✅ Kernel exécuté (%u groups × 256 threads)\n\n", num_groups);
    
    /* ═══════════════════════════════════════════════════════════════════
     * ÉTAPE 7 : Validation résultats
     * ═══════════════════════════════════════════════════════════════════ */
    
    fprintf(stderr, "[7/7] Validation résultats...\n");
    
    /* Download résultats */
    if (btc_l0_read_buffer(ctx, buf_c, host_c, buffer_size) != BTC_L0_OK) {
        fprintf(stderr, "❌ ERREUR : Download GPU échoué\n");
        btc_l0_destroy_kernel(ctx, kernel);
        btc_l0_destroy_module(ctx, module);
        ret = 1;
        goto cleanup;
    }
    
    /* Vérifie résultats */
    int errors = 0;
    for (int i = 0; i < TEST_SIZE; i++) {
        float expected = host_a[i] + host_b[i];
        float actual = host_c[i];
        
        if (fabsf(actual - expected) > 0.001f) {
            if (errors < 10) {  /* Affiche max 10 erreurs */
                fprintf(stderr, "  ❌ c[%d] = %.2f (attendu %.2f)\n", i, actual, expected);
            }
            errors++;
        }
    }
    
    if (errors == 0) {
        fprintf(stderr, "✅ SUCCÈS : %d/%d éléments corrects (100%%)\n", TEST_SIZE, TEST_SIZE);
        ret = 0;
    } else {
        fprintf(stderr, "❌ ÉCHEC : %d/%d erreurs\n", errors, TEST_SIZE);
        ret = 1;
    }
    
    /* Cleanup kernel */
    btc_l0_destroy_kernel(ctx, kernel);
    btc_l0_destroy_module(ctx, module);
    
cleanup:
    /* ═══════════════════════════════════════════════════════════════════
     * CLEANUP
     * ═══════════════════════════════════════════════════════════════════ */
    
    fprintf(stderr, "\n[CLEANUP] Libération ressources...\n");
    
    free(host_a);
    free(host_b);
    free(host_c);
    
    btc_l0_free_buffer(ctx, buf_a);
    btc_l0_free_buffer(ctx, buf_b);
    btc_l0_free_buffer(ctx, buf_c);
    
    btc_l0_cleanup(ctx);
    
    fprintf(stderr, "✅ Cleanup terminé\n\n");
    
    /* ═══════════════════════════════════════════════════════════════════
     * RÉSUMÉ
     * ═══════════════════════════════════════════════════════════════════ */
    
    fprintf(stderr, "═══════════════════════════════════════════════════════════════\n");
    if (ret == 0) {
        fprintf(stderr, "  ✅ TEST RÉUSSI — Kernel Level Zero fonctionnel!\n");
    } else {
        fprintf(stderr, "  ❌ TEST ÉCHOUÉ — Voir logs ci-dessus\n");
    }
    fprintf(stderr, "═══════════════════════════════════════════════════════════════\n\n");
    
    fprintf(stderr, "📊 Logs forensiques : logs/forensic/test_kernel_c198.lum\n\n");
    
    return ret;
}

// Made with Bob
