/*
 * Test NEO Wrapper - Projet A - Phase 15Y-F26-5
 * Test complet avec dispatch kernel réel
 * 
 * OBJECTIF: Valider wrapper NEO avec:
 * 1. Initialisation OpenCL
 * 2. Compilation kernel
 * 3. Allocation buffers
 * 4. Dispatch kernel
 * 5. Vérification résultats
 * 6. Cleanup complet
 */

#include "btc_neo_wrapper.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <CL/cl.h>

// Kernel OpenCL simple pour test (addition vectorielle)
const char *test_kernel_source = 
"__kernel void vector_add(__global const float *a, __global const float *b, __global float *c, const unsigned int n) {\n"
"    int id = get_global_id(0);\n"
"    if (id < n) {\n"
"        c[id] = a[id] + b[id];\n"
"    }\n"
"}\n";

int main(void) {
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  Test NEO Wrapper — Dispatch Kernel Complet\n");
    printf("  Phase 15Y-F26-5 — Projet A\n");
    printf("═══════════════════════════════════════════════════════════\n\n");

    // Test 1: Initialisation
    printf("[TEST 1/6] Initialisation NEO Wrapper...\n");
    btc_neo_wrapper_t *wrapper = NULL;
    int ret = btc_neo_wrapper_init(&wrapper);
    if (ret != 0 || !wrapper) {
        fprintf(stderr, "❌ Erreur initialisation (ret=%d)\n", ret);
        return 1;
    }
    printf("✅ Initialisation réussie\n");
    cl_context ctx = btc_neo_wrapper_get_context(wrapper);
    cl_command_queue queue = btc_neo_wrapper_get_queue(wrapper);
    printf("   Context: %p\n", (void*)ctx);
    printf("   Queue: %p\n\n", (void*)queue);

    // Test 2: Compilation kernel
    printf("[TEST 2/6] Compilation kernel OpenCL...\n");
    ret = btc_neo_wrapper_load_kernel(wrapper, test_kernel_source, "vector_add");
    if (ret != 0) {
        fprintf(stderr, "❌ Erreur compilation kernel (ret=%d)\n", ret);
        btc_neo_wrapper_cleanup(wrapper);
        return 1;
    }
    printf("✅ Kernel compilé avec succès\n");
    cl_kernel kernel = btc_neo_wrapper_get_kernel(wrapper);
    printf("   Kernel: %p\n\n", (void*)kernel);

    // Test 3: Allocation buffers et préparation données
    printf("[TEST 3/6] Allocation buffers et préparation données...\n");
    const size_t n = 1024;
    const size_t bytes = n * sizeof(float);
    
    // Données host
    float *h_a = (float*)malloc(bytes);
    float *h_b = (float*)malloc(bytes);
    float *h_c = (float*)malloc(bytes);
    
    if (!h_a || !h_b || !h_c) {
        fprintf(stderr, "❌ Erreur allocation mémoire host\n");
        btc_neo_wrapper_cleanup(wrapper);
        return 1;
    }
    
    // Initialisation données
    for (size_t i = 0; i < n; i++) {
        h_a[i] = (float)i;
        h_b[i] = (float)(i * 2);
        h_c[i] = 0.0f;
    }
    
    // Création buffers OpenCL
    cl_int err;
    cl_context ctx2 = btc_neo_wrapper_get_context(wrapper);
    cl_mem d_a = clCreateBuffer(ctx2, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, bytes, h_a, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "❌ Erreur création buffer A (err=%d)\n", err);
        free(h_a); free(h_b); free(h_c);
        btc_neo_wrapper_cleanup(wrapper);
        return 1;
    }
    
    cl_mem d_b = clCreateBuffer(ctx2, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, bytes, h_b, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "❌ Erreur création buffer B (err=%d)\n", err);
        clReleaseMemObject(d_a);
        free(h_a); free(h_b); free(h_c);
        btc_neo_wrapper_cleanup(wrapper);
        return 1;
    }
    
    cl_mem d_c = clCreateBuffer(ctx2, CL_MEM_WRITE_ONLY, bytes, NULL, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "❌ Erreur création buffer C (err=%d)\n", err);
        clReleaseMemObject(d_a);
        clReleaseMemObject(d_b);
        free(h_a); free(h_b); free(h_c);
        btc_neo_wrapper_cleanup(wrapper);
        return 1;
    }
    
    printf("✅ Buffers alloués\n");
    printf("   Buffer A: %p (%zu bytes)\n", (void*)d_a, bytes);
    printf("   Buffer B: %p (%zu bytes)\n", (void*)d_b, bytes);
    printf("   Buffer C: %p (%zu bytes)\n\n", (void*)d_c, bytes);

    // Test 4: Configuration arguments
    printf("[TEST 4/6] Configuration arguments kernel...\n");
    
    cl_kernel kernel2 = btc_neo_wrapper_get_kernel(wrapper);
    err = clSetKernelArg(kernel2, 0, sizeof(cl_mem), &d_a);
    err |= clSetKernelArg(kernel2, 1, sizeof(cl_mem), &d_b);
    err |= clSetKernelArg(kernel2, 2, sizeof(cl_mem), &d_c);
    err |= clSetKernelArg(kernel2, 3, sizeof(unsigned int), &n);
    
    if (err != CL_SUCCESS) {
        fprintf(stderr, "❌ Erreur configuration arguments (err=%d)\n", err);
        clReleaseMemObject(d_a);
        clReleaseMemObject(d_b);
        clReleaseMemObject(d_c);
        free(h_a); free(h_b); free(h_c);
        btc_neo_wrapper_cleanup(wrapper);
        return 1;
    }
    
    printf("✅ Arguments configurés\n");
    printf("   Arg 0: buffer A (input)\n");
    printf("   Arg 1: buffer B (input)\n");
    printf("   Arg 2: buffer C (output)\n");
    printf("   Arg 3: n=%zu (size)\n\n", n);

    // Test 5: Dispatch kernel
    printf("[TEST 5/6] Dispatch kernel...\n");
    
    size_t global_size = n;
    size_t local_size = 256;
    cl_mem buffers[3] = {d_a, d_b, d_c};
    ret = btc_neo_wrapper_dispatch(wrapper, global_size, local_size, buffers, 3);
    if (ret != 0) {
        fprintf(stderr, "❌ Erreur dispatch kernel (ret=%d)\n", ret);
        clReleaseMemObject(d_a);
        clReleaseMemObject(d_b);
        clReleaseMemObject(d_c);
        free(h_a); free(h_b); free(h_c);
        btc_neo_wrapper_cleanup(wrapper);
        return 1;
    }
    
    printf("✅ Kernel dispatché\n");
    printf("   Global size: %zu\n", global_size);
    printf("   Local size: %zu\n", local_size);
    printf("   Work groups: %zu\n", global_size / local_size);
    
    btc_neo_metrics_t metrics;
    btc_neo_wrapper_get_metrics(wrapper, &metrics);
    printf("   Dispatch count: %lu\n", metrics.dispatch_count);
    printf("   Total time: %.3f ms\n", metrics.total_time_ns / 1000000.0);
    printf("   Bytes transferred: %lu\n\n", metrics.bytes_transferred);

    // Test 6: Lecture résultats et vérification
    printf("[TEST 6/6] Lecture résultats et vérification...\n");
    
    cl_command_queue queue2 = btc_neo_wrapper_get_queue(wrapper);
    err = clEnqueueReadBuffer(queue2, d_c, CL_TRUE, 0, bytes, h_c, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "❌ Erreur lecture buffer C (err=%d)\n", err);
        clReleaseMemObject(d_a);
        clReleaseMemObject(d_b);
        clReleaseMemObject(d_c);
        free(h_a); free(h_b); free(h_c);
        btc_neo_wrapper_cleanup(wrapper);
        return 1;
    }
    
    // Vérification résultats
    int errors = 0;
    for (size_t i = 0; i < n && errors < 10; i++) {
        float expected = h_a[i] + h_b[i];
        if (h_c[i] != expected) {
            fprintf(stderr, "❌ Erreur résultat[%zu]: attendu %.1f, obtenu %.1f\n", i, expected, h_c[i]);
            errors++;
        }
    }
    
    if (errors == 0) {
        printf("✅ Tous les résultats sont corrects (%zu éléments vérifiés)\n", n);
        printf("   c[0] = %.1f (attendu: %.1f)\n", h_c[0], h_a[0] + h_b[0]);
        printf("   c[100] = %.1f (attendu: %.1f)\n", h_c[100], h_a[100] + h_b[100]);
        printf("   c[1023] = %.1f (attendu: %.1f)\n\n", h_c[1023], h_a[1023] + h_b[1023]);
    } else {
        fprintf(stderr, "❌ %d erreurs détectées\n\n", errors);
    }

    // Cleanup
    printf("[CLEANUP] Libération ressources...\n");
    clReleaseMemObject(d_a);
    clReleaseMemObject(d_b);
    clReleaseMemObject(d_c);
    free(h_a);
    free(h_b);
    free(h_c);
    btc_neo_wrapper_cleanup(wrapper);
    printf("✅ Cleanup terminé\n\n");

    // Résultat final
    printf("═══════════════════════════════════════════════════════════\n");
    if (errors == 0) {
        printf("  RÉSULTAT : ✅ TOUS LES TESTS RÉUSSIS\n");
        printf("  - Initialisation OpenCL: OK\n");
        printf("  - Compilation kernel: OK\n");
        printf("  - Allocation buffers: OK\n");
        printf("  - Dispatch kernel: OK\n");
        printf("  - Vérification résultats: OK\n");
        printf("  - Cleanup: OK\n");
    } else {
        printf("  RÉSULTAT : ❌ ÉCHEC (%d erreurs)\n", errors);
    }
    printf("═══════════════════════════════════════════════════════════\n");

    return (errors == 0) ? 0 : 1;
}

// Made with Bob - Phase 15Y-F26-5
