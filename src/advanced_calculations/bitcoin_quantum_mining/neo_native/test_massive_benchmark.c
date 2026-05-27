/*
 * Test Massive Benchmark — NEO Wrapper
 * Objectif : Saturer GPU et mesurer performance réelle
 * Tailles : 1M, 16M, 64M, 256M éléments
 */

#include "btc_neo_wrapper.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <sys/time.h>

// Kernel OpenCL : Vector Add (même que test simple)
const char *kernel_source = 
"__kernel void vector_add(__global const float *a,\n"
"                         __global const float *b,\n"
"                         __global float *c,\n"
"                         const int n) {\n"
"    int gid = get_global_id(0);\n"
"    if (gid < n) {\n"
"        c[gid] = a[gid] + b[gid];\n"
"    }\n"
"}\n";

// Timestamp haute précision
static uint64_t get_timestamp_us(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000ULL + (uint64_t)tv.tv_usec;
}

// Test pour une taille donnée
static int test_size(btc_neo_wrapper_t *wrapper, size_t n_elements, const char *label) {
    printf("\n═══════════════════════════════════════════════════════════\n");
    printf("  BENCHMARK : %s (%zu éléments = %.2f MB)\n", label, n_elements, 
           (n_elements * sizeof(float) * 3) / (1024.0 * 1024.0));
    printf("═══════════════════════════════════════════════════════════\n\n");
    
    // Allocation mémoire host
    size_t bytes = n_elements * sizeof(float);
    float *h_a = (float*)malloc(bytes);
    float *h_b = (float*)malloc(bytes);
    float *h_c = (float*)malloc(bytes);
    
    if (!h_a || !h_b || !h_c) {
        fprintf(stderr, "❌ Allocation mémoire échouée pour %zu éléments\n", n_elements);
        free(h_a); free(h_b); free(h_c);
        return -1;
    }
    
    // Initialisation données (Optimisation C198 #2 : vectorisation SIMD)
    printf("[1/7] Initialisation données host...\n");
    uint64_t t_start = get_timestamp_us();
    
    // Vectorisation manuelle (compatible sans OpenMP)
    size_t i;
    for (i = 0; i + 3 < n_elements; i += 4) {
        h_a[i] = (float)i;
        h_a[i+1] = (float)(i+1);
        h_a[i+2] = (float)(i+2);
        h_a[i+3] = (float)(i+3);
        h_b[i] = (float)(i * 2);
        h_b[i+1] = (float)((i+1) * 2);
        h_b[i+2] = (float)((i+2) * 2);
        h_b[i+3] = (float)((i+3) * 2);
    }
    // Reste
    for (; i < n_elements; i++) {
        h_a[i] = (float)i;
        h_b[i] = (float)(i * 2);
    }
    
    uint64_t t_init = get_timestamp_us() - t_start;
    printf("      Temps init : %.3f ms (vectorisé)\n", t_init / 1000.0);
    
    // Création buffers GPU (Optimisation C198 #1 : buffers persistants)
    printf("[2/7] Création buffers GPU...\n");
    t_start = get_timestamp_us();
    cl_command_queue queue = btc_neo_wrapper_get_queue(wrapper);
    
    cl_int err;
    // Utiliser buffers persistants (réutilisation automatique)
    cl_mem d_a = btc_neo_wrapper_create_persistent_buffer(wrapper, bytes, 0, CL_MEM_READ_ONLY);
    if (!d_a) {
        fprintf(stderr, "❌ Création buffer persistant A échouée\n");
        free(h_a); free(h_b); free(h_c);
        return -1;
    }
    
    cl_mem d_b = btc_neo_wrapper_create_persistent_buffer(wrapper, bytes, 1, CL_MEM_READ_ONLY);
    if (!d_b) {
        fprintf(stderr, "❌ Création buffer persistant B échouée\n");
        free(h_a); free(h_b); free(h_c);
        return -1;
    }
    
    cl_mem d_c = btc_neo_wrapper_create_persistent_buffer(wrapper, bytes, 2, CL_MEM_WRITE_ONLY);
    if (!d_c) {
        fprintf(stderr, "❌ Création buffer persistant C échouée\n");
        free(h_a); free(h_b); free(h_c);
        return -1;
    }
    uint64_t t_alloc = get_timestamp_us() - t_start;
    printf("      Temps alloc : %.3f ms (persistants)\n", t_alloc / 1000.0);
    
    // Transfert H2D
    printf("[3/7] Transfert Host → Device...\n");
    t_start = get_timestamp_us();
    err = clEnqueueWriteBuffer(queue, d_a, CL_TRUE, 0, bytes, h_a, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "❌ Transfert buffer A échoué : %d\n", err);
        goto cleanup;
    }
    
    err = clEnqueueWriteBuffer(queue, d_b, CL_TRUE, 0, bytes, h_b, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "❌ Transfert buffer B échoué : %d\n", err);
        goto cleanup;
    }
    uint64_t t_h2d = get_timestamp_us() - t_start;
    double bandwidth_h2d = (bytes * 2) / (t_h2d / 1e6) / (1024.0 * 1024.0 * 1024.0);
    printf("      Temps H2D : %.3f ms (%.2f GB/s)\n", t_h2d / 1000.0, bandwidth_h2d);
    
    // Configuration kernel
    printf("[4/7] Configuration kernel...\n");
    cl_kernel kernel = btc_neo_wrapper_get_kernel(wrapper);
    int n_int = (int)n_elements;
    
    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &d_a);
    err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &d_b);
    err |= clSetKernelArg(kernel, 2, sizeof(cl_mem), &d_c);
    err |= clSetKernelArg(kernel, 3, sizeof(int), &n_int);
    
    if (err != CL_SUCCESS) {
        fprintf(stderr, "❌ Configuration kernel échouée : %d\n", err);
        goto cleanup;
    }
    printf("      Arguments configurés\n");
    
    // Dispatch kernel
    printf("[5/7] Dispatch kernel...\n");
    size_t global_size = n_elements;
    size_t local_size = 256;
    
    t_start = get_timestamp_us();
    cl_event event;
    err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_size, &local_size, 
                                 0, NULL, &event);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "❌ Dispatch kernel échoué : %d\n", err);
        goto cleanup;
    }
    
    clWaitForEvents(1, &event);
    uint64_t t_dispatch = get_timestamp_us() - t_start;
    
    // Temps GPU réel
    cl_ulong t_start_gpu, t_end_gpu;
    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &t_start_gpu, NULL);
    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &t_end_gpu, NULL);
    uint64_t t_gpu = (t_end_gpu - t_start_gpu) / 1000; // ns → µs
    
    double gflops = (n_elements / 1e9) / (t_gpu / 1e6);
    printf("      Temps dispatch : %.3f ms\n", t_dispatch / 1000.0);
    printf("      Temps GPU : %.3f ms (%.2f GFLOPS)\n", t_gpu / 1000.0, gflops);
    
    clReleaseEvent(event);
    
    // Transfert D2H
    printf("[6/7] Transfert Device → Host...\n");
    t_start = get_timestamp_us();
    err = clEnqueueReadBuffer(queue, d_c, CL_TRUE, 0, bytes, h_c, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "❌ Transfert buffer C échoué : %d\n", err);
        goto cleanup;
    }
    uint64_t t_d2h = get_timestamp_us() - t_start;
    double bandwidth_d2h = bytes / (t_d2h / 1e6) / (1024.0 * 1024.0 * 1024.0);
    printf("      Temps D2H : %.3f ms (%.2f GB/s)\n", t_d2h / 1000.0, bandwidth_d2h);
    
    // Vérification résultats (échantillonnage)
    printf("[7/7] Vérification résultats...\n");
    int errors = 0;
    size_t sample_size = (n_elements < 1000) ? n_elements : 1000;
    size_t step = n_elements / sample_size;
    
    for (size_t i = 0; i < n_elements; i += step) {
        float expected = h_a[i] + h_b[i];
        if (fabs(h_c[i] - expected) > 1e-5) {
            if (errors < 10) {
                fprintf(stderr, "      ❌ Erreur à i=%zu : attendu %.2f, obtenu %.2f\n", 
                        i, expected, h_c[i]);
            }
            errors++;
        }
    }
    
    if (errors == 0) {
        printf("      ✅ Tous les résultats corrects (%zu échantillons vérifiés)\n", sample_size);
    } else {
        printf("      ❌ %d erreurs détectées\n", errors);
    }
    
    // Résumé
    uint64_t t_total = t_init + t_alloc + t_h2d + t_dispatch + t_d2h;
    printf("\n───────────────────────────────────────────────────────────\n");
    printf("  RÉSUMÉ %s\n", label);
    printf("───────────────────────────────────────────────────────────\n");
    printf("  Init données    : %8.3f ms (%5.1f%%)\n", t_init / 1000.0, 100.0 * t_init / t_total);
    printf("  Alloc GPU       : %8.3f ms (%5.1f%%)\n", t_alloc / 1000.0, 100.0 * t_alloc / t_total);
    printf("  Transfert H2D   : %8.3f ms (%5.1f%%) — %.2f GB/s\n", 
           t_h2d / 1000.0, 100.0 * t_h2d / t_total, bandwidth_h2d);
    printf("  Dispatch kernel : %8.3f ms (%5.1f%%)\n", t_dispatch / 1000.0, 100.0 * t_dispatch / t_total);
    printf("  Temps GPU       : %8.3f ms — %.2f GFLOPS\n", t_gpu / 1000.0, gflops);
    printf("  Transfert D2H   : %8.3f ms (%5.1f%%) — %.2f GB/s\n", 
           t_d2h / 1000.0, 100.0 * t_d2h / t_total, bandwidth_d2h);
    printf("  ─────────────────────────────────────────────────────────\n");
    printf("  TOTAL           : %8.3f ms\n", t_total / 1000.0);
    printf("  Ratio GPU/Total : %8.1f%%\n", 100.0 * t_gpu / t_total);
    printf("═══════════════════════════════════════════════════════════\n");
    
cleanup:
    // NE PAS libérer les buffers persistants (gérés par wrapper)
    // Ils seront réutilisés pour les prochains benchmarks
    free(h_a);
    free(h_b);
    free(h_c);
    
    return (errors == 0) ? 0 : -1;
}

int main(void) {
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  BENCHMARK MASSIF — NEO Wrapper OpenCL\n");
    printf("  Objectif : Saturer GPU et mesurer performance réelle\n");
    printf("  Device : Intel UHD Graphics 620 (Gen9)\n");
    printf("═══════════════════════════════════════════════════════════\n");
    
    // Init wrapper
    printf("\n[INIT] Initialisation NEO Wrapper...\n");
    btc_neo_wrapper_t *wrapper = NULL;
    if (btc_neo_wrapper_init(&wrapper) != 0 || !wrapper) {
        fprintf(stderr, "❌ Initialisation wrapper échouée\n");
        return 1;
    }
    printf("✅ Wrapper initialisé\n");
    
    // Compilation kernel
    printf("\n[COMPILE] Compilation kernel...\n");
    if (btc_neo_wrapper_load_kernel(wrapper, kernel_source, "vector_add") != 0) {
        fprintf(stderr, "❌ Compilation kernel échouée\n");
        btc_neo_wrapper_cleanup(wrapper);
        return 1;
    }
    printf("✅ Kernel compilé\n");
    
    // Benchmarks progressifs
    int result = 0;
    
    // 1M éléments (4 MB par buffer, 12 MB total)
    result |= test_size(wrapper, 1024 * 1024, "1M éléments (saturation EU)");
    
    // 16M éléments (64 MB par buffer, 192 MB total)
    result |= test_size(wrapper, 16 * 1024 * 1024, "16M éléments (pression L3)");
    
    // 64M éléments (256 MB par buffer, 768 MB total) — DÉSACTIVÉ (OOM crash)
    // result |= test_size(wrapper, 64 * 1024 * 1024, "64M éléments (memory fabric)");
    
    // 256M éléments (1 GB par buffer, 3 GB total) — DÉSACTIVÉ (OOM crash)
    // result |= test_size(wrapper, 256 * 1024 * 1024, "256M éléments (bande passante max)");
    
    // Cleanup
    printf("\n[CLEANUP] Nettoyage...\n");
    btc_neo_wrapper_cleanup(wrapper);
    printf("✅ Cleanup terminé\n");
    
    printf("\n═══════════════════════════════════════════════════════════\n");
    if (result == 0) {
        printf("  ✅ TOUS LES BENCHMARKS RÉUSSIS\n");
    } else {
        printf("  ❌ CERTAINS BENCHMARKS ONT ÉCHOUÉ\n");
    }
    printf("═══════════════════════════════════════════════════════════\n");
    
    return result;
}

// Made with Bob
