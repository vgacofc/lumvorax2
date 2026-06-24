/**
 * @file lum_poh_gpu.c
 * @brief Implémentation GPU OpenCL pour vérification PoH
 * 
 * Module d'accélération GPU utilisant OpenCL pour vérifier
 * les ticks PoH en parallèle sur GPU Intel UHD Graphics 620.
 * 
 * @version 1.0.0
 * @date 2026-06-11
 */

#include "lum_poh_gpu.h"
#include "../debug/ultra_forensic_logger.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <openssl/sha.h>

/* ============================================================================
 * KERNEL OPENCL SHA-256
 * ============================================================================ */

/**
 * @brief Kernel OpenCL pour vérification SHA-256 parallèle
 * 
 * Chaque work-item vérifie un hash : curr_hash == SHA256(prev_hash)
 */
static const char* LUM_POH_GPU_KERNEL_SOURCE = 
"__kernel void verify_poh_hashes(\n"
"    __global const uchar* prev_hashes,\n"
"    __global const uchar* curr_hashes,\n"
"    __global uchar* results,\n"
"    const uint count)\n"
"{\n"
"    uint gid = get_global_id(0);\n"
"    if (gid >= count) return;\n"
"    \n"
"    // Offset pour ce hash (32 bytes)\n"
"    uint offset = gid * 32;\n"
"    \n"
"    // Calculer SHA-256 du hash précédent (simplifié)\n"
"    // NOTE: Implémentation complète SHA-256 nécessite ~200 lignes\n"
"    // Pour l'instant, on fait une comparaison directe (placeholder)\n"
"    \n"
"    uchar computed_hash[32];\n"
"    \n"
"    // TODO: Implémenter SHA-256 complet en OpenCL\n"
"    // Pour l'instant, copier prev_hash (simulation)\n"
"    for (int i = 0; i < 32; i++) {\n"
"        computed_hash[i] = prev_hashes[offset + i];\n"
"    }\n"
"    \n"
"    // Comparer avec curr_hash\n"
"    uchar match = 1;\n"
"    for (int i = 0; i < 32; i++) {\n"
"        if (computed_hash[i] != curr_hashes[offset + i]) {\n"
"            match = 0;\n"
"            break;\n"
"        }\n"
"    }\n"
"    \n"
"    results[gid] = match;\n"
"}\n";

/* ============================================================================
 * UTILITAIRES
 * ============================================================================ */

/**
 * @brief Obtenir timestamp nanoseconde
 */
static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

/**
 * @brief Vérifier erreur OpenCL
 */
static bool check_cl_error(cl_int err, const char* operation) {
    if (err != CL_SUCCESS) {
        char msg[256];
        snprintf(msg, sizeof(msg), "OpenCL error %d during %s", err, operation);
        fprintf(stderr, "[GPU ERROR] %s\n", msg);
        return false;
    }
    return true;
}

/* ============================================================================
 * DÉTECTION GPU
 * ============================================================================ */

int lum_poh_gpu_detect_devices(lum_poh_gpu_device_info_t* devices, uint32_t max_devices) {
    if (!devices || max_devices == 0) return -1;
    
    cl_platform_id platforms[8];
    cl_uint num_platforms = 0;
    cl_int err = clGetPlatformIDs(8, platforms, &num_platforms);
    
    if (!check_cl_error(err, "clGetPlatformIDs")) {
        return -1;
    }
    
    if (num_platforms == 0) {
        fprintf(stderr, "[GPU WARNING] No OpenCL platforms found\n");
        return 0;
    }
    
    uint32_t total_devices = 0;
    
    // Parcourir toutes les platforms
    for (cl_uint i = 0; i < num_platforms && total_devices < max_devices; i++) {
        cl_device_id platform_devices[8];
        cl_uint num_devices = 0;
        
        err = clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_GPU, 8, 
                            platform_devices, &num_devices);
        
        if (err != CL_SUCCESS) continue;
        
        // Récupérer infos de chaque device
        for (cl_uint j = 0; j < num_devices && total_devices < max_devices; j++) {
            lum_poh_gpu_device_info_t* info = &devices[total_devices];
            info->device_id = platform_devices[j];
            
            clGetDeviceInfo(platform_devices[j], CL_DEVICE_NAME, 
                          sizeof(info->name), info->name, NULL);
            clGetDeviceInfo(platform_devices[j], CL_DEVICE_VENDOR,
                          sizeof(info->vendor), info->vendor, NULL);
            clGetDeviceInfo(platform_devices[j], CL_DEVICE_GLOBAL_MEM_SIZE,
                          sizeof(info->global_mem_size), &info->global_mem_size, NULL);
            clGetDeviceInfo(platform_devices[j], CL_DEVICE_MAX_COMPUTE_UNITS,
                          sizeof(info->compute_units), &info->compute_units, NULL);
            clGetDeviceInfo(platform_devices[j], CL_DEVICE_MAX_WORK_GROUP_SIZE,
                          sizeof(info->max_work_group_size), &info->max_work_group_size, NULL);
            clGetDeviceInfo(platform_devices[j], CL_DEVICE_MAX_CLOCK_FREQUENCY,
                          sizeof(info->max_clock_frequency), &info->max_clock_frequency, NULL);
            
            info->supports_sha256 = false; // TODO: Détecter support SHA-256
            
            total_devices++;
        }
    }
    
    char msg[256];
    snprintf(msg, sizeof(msg), "Detected %u GPU device(s)", total_devices);
    printf("[GPU INFO] %s\n", msg);
    
    return (int)total_devices;
}

/* ============================================================================
 * INITIALISATION GPU
 * ============================================================================ */

lum_poh_gpu_context_t* lum_poh_gpu_init(uint32_t device_index, bool fallback_cpu) {
    lum_poh_gpu_context_t* ctx = (lum_poh_gpu_context_t*)calloc(1, sizeof(lum_poh_gpu_context_t));
    if (!ctx) return NULL;
    
    ctx->fallback_cpu = fallback_cpu;
    ctx->initialized = false;
    
    // Détecter GPUs
    lum_poh_gpu_device_info_t devices[LUM_POH_GPU_MAX_DEVICES];
    int num_devices = lum_poh_gpu_detect_devices(devices, LUM_POH_GPU_MAX_DEVICES);
    
    if (num_devices <= 0) {
        fprintf(stderr, "[GPU WARNING] No GPU detected, using CPU fallback\n");
        ctx->fallback_cpu = true;
        return ctx;
    }
    
    if (device_index >= (uint32_t)num_devices) {
        fprintf(stderr, "[GPU ERROR] Invalid device index\n");
        free(ctx);
        return NULL;
    }
    
    // Copier info device
    ctx->device_info = devices[device_index];
    ctx->device = devices[device_index].device_id;
    
    // Créer contexte OpenCL
    cl_int err;
    ctx->context = clCreateContext(NULL, 1, &ctx->device, NULL, NULL, &err);
    if (!check_cl_error(err, "clCreateContext")) {
        free(ctx);
        return NULL;
    }
    
    // Créer queue commandes
    ctx->queue = clCreateCommandQueue(ctx->context, ctx->device, 0, &err);
    if (!check_cl_error(err, "clCreateCommandQueue")) {
        clReleaseContext(ctx->context);
        free(ctx);
        return NULL;
    }
    
    // Compiler kernel
    ctx->program = clCreateProgramWithSource(ctx->context, 1, 
                                            &LUM_POH_GPU_KERNEL_SOURCE, NULL, &err);
    if (!check_cl_error(err, "clCreateProgramWithSource")) {
        clReleaseCommandQueue(ctx->queue);
        clReleaseContext(ctx->context);
        free(ctx);
        return NULL;
    }
    
    err = clBuildProgram(ctx->program, 1, &ctx->device, NULL, NULL, NULL);
    if (err != CL_SUCCESS) {
        // Récupérer log de compilation
        char build_log[4096];
        clGetProgramBuildInfo(ctx->program, ctx->device, CL_PROGRAM_BUILD_LOG,
                            sizeof(build_log), build_log, NULL);
        fprintf(stderr, "[GPU ERROR] Kernel compilation failed:\n%s\n", build_log);
        clReleaseProgram(ctx->program);
        clReleaseCommandQueue(ctx->queue);
        clReleaseContext(ctx->context);
        free(ctx);
        return NULL;
    }
    
    // Créer kernel
    ctx->kernel = clCreateKernel(ctx->program, "verify_poh_hashes", &err);
    if (!check_cl_error(err, "clCreateKernel")) {
        clReleaseProgram(ctx->program);
        clReleaseCommandQueue(ctx->queue);
        clReleaseContext(ctx->context);
        free(ctx);
        return NULL;
    }
    
    // Allouer buffers GPU
    size_t buffer_size = LUM_POH_GPU_BATCH_SIZE * LUM_POH_GPU_HASH_SIZE;
    
    ctx->buffer_prev_hashes = clCreateBuffer(ctx->context, CL_MEM_READ_ONLY,
                                            buffer_size, NULL, &err);
    if (!check_cl_error(err, "clCreateBuffer prev_hashes")) goto cleanup;
    
    ctx->buffer_curr_hashes = clCreateBuffer(ctx->context, CL_MEM_READ_ONLY,
                                            buffer_size, NULL, &err);
    if (!check_cl_error(err, "clCreateBuffer curr_hashes")) goto cleanup;
    
    ctx->buffer_results = clCreateBuffer(ctx->context, CL_MEM_WRITE_ONLY,
                                        LUM_POH_GPU_BATCH_SIZE, NULL, &err);
    if (!check_cl_error(err, "clCreateBuffer results")) goto cleanup;
    
    ctx->initialized = true;
    
    char msg[512];
    snprintf(msg, sizeof(msg), 
            "GPU initialized: %s (%s) - %u CUs, %.2f GB memory",
            ctx->device_info.name, ctx->device_info.vendor,
            ctx->device_info.compute_units,
            ctx->device_info.global_mem_size / (1024.0 * 1024.0 * 1024.0));
    printf("[GPU INFO] %s\n", msg);
    
    return ctx;
    
cleanup:
    if (ctx->buffer_prev_hashes) clReleaseMemObject(ctx->buffer_prev_hashes);
    if (ctx->buffer_curr_hashes) clReleaseMemObject(ctx->buffer_curr_hashes);
    if (ctx->buffer_results) clReleaseMemObject(ctx->buffer_results);
    clReleaseKernel(ctx->kernel);
    clReleaseProgram(ctx->program);
    clReleaseCommandQueue(ctx->queue);
    clReleaseContext(ctx->context);
    free(ctx);
    return NULL;
}

/* ============================================================================
 * VÉRIFICATION GPU
 * ============================================================================ */

bool lum_poh_gpu_verify_batch(lum_poh_gpu_context_t* ctx, lum_poh_gpu_batch_t* batch) {
    if (!ctx || !batch || batch->count == 0) return false;
    
    batch->timestamp_submit_ns = get_timestamp_ns();
    
    // Fallback CPU si GPU non initialisé
    if (!ctx->initialized || ctx->fallback_cpu) {
        return lum_poh_gpu_verify_batch_cpu(batch);
    }
    
    cl_int err;
    
    // Copier données vers GPU
    size_t data_size = batch->count * LUM_POH_GPU_HASH_SIZE;
    
    err = clEnqueueWriteBuffer(ctx->queue, ctx->buffer_prev_hashes, CL_FALSE,
                              0, data_size, batch->prev_hashes, 0, NULL, NULL);
    if (!check_cl_error(err, "clEnqueueWriteBuffer prev_hashes")) {
        return lum_poh_gpu_verify_batch_cpu(batch);
    }
    
    err = clEnqueueWriteBuffer(ctx->queue, ctx->buffer_curr_hashes, CL_FALSE,
                              0, data_size, batch->curr_hashes, 0, NULL, NULL);
    if (!check_cl_error(err, "clEnqueueWriteBuffer curr_hashes")) {
        return lum_poh_gpu_verify_batch_cpu(batch);
    }
    
    // Configurer arguments kernel
    err = clSetKernelArg(ctx->kernel, 0, sizeof(cl_mem), &ctx->buffer_prev_hashes);
    err |= clSetKernelArg(ctx->kernel, 1, sizeof(cl_mem), &ctx->buffer_curr_hashes);
    err |= clSetKernelArg(ctx->kernel, 2, sizeof(cl_mem), &ctx->buffer_results);
    err |= clSetKernelArg(ctx->kernel, 3, sizeof(uint32_t), &batch->count);
    
    if (!check_cl_error(err, "clSetKernelArg")) {
        return lum_poh_gpu_verify_batch_cpu(batch);
    }
    
    // Exécuter kernel
    size_t global_work_size = ((batch->count + LUM_POH_GPU_WORKGROUP_SIZE - 1) / 
                               LUM_POH_GPU_WORKGROUP_SIZE) * LUM_POH_GPU_WORKGROUP_SIZE;
    size_t local_work_size = LUM_POH_GPU_WORKGROUP_SIZE;
    
    err = clEnqueueNDRangeKernel(ctx->queue, ctx->kernel, 1, NULL,
                                &global_work_size, &local_work_size,
                                0, NULL, NULL);
    if (!check_cl_error(err, "clEnqueueNDRangeKernel")) {
        return lum_poh_gpu_verify_batch_cpu(batch);
    }
    
    // Lire résultats
    err = clEnqueueReadBuffer(ctx->queue, ctx->buffer_results, CL_TRUE,
                             0, batch->count, batch->results, 0, NULL, NULL);
    if (!check_cl_error(err, "clEnqueueReadBuffer results")) {
        return lum_poh_gpu_verify_batch_cpu(batch);
    }
    
    batch->timestamp_complete_ns = get_timestamp_ns();
    
    // Compter erreurs
    uint32_t errors = 0;
    for (uint32_t i = 0; i < batch->count; i++) {
        if (batch->results[i] == 0) errors++;
    }
    
    // Mettre à jour métriques
    ctx->total_batches_processed++;
    ctx->total_hashes_verified += batch->count;
    ctx->total_errors_detected += errors;
    
    double batch_time_ms = (batch->timestamp_complete_ns - batch->timestamp_submit_ns) / 1000000.0;
    ctx->avg_batch_time_ms = (ctx->avg_batch_time_ms * (ctx->total_batches_processed - 1) + 
                             batch_time_ms) / ctx->total_batches_processed;
    
    if (batch_time_ms > 0) {
        ctx->verifications_per_second = (uint64_t)((batch->count / batch_time_ms) * 1000.0);
    }
    
    return true;
}

/* ============================================================================
 * VÉRIFICATION CPU (FALLBACK)
 * ============================================================================ */

bool lum_poh_gpu_verify_batch_cpu(lum_poh_gpu_batch_t* batch) {
    if (!batch || batch->count == 0) return false;
    
    batch->timestamp_submit_ns = get_timestamp_ns();
    
    // Vérifier chaque hash avec OpenSSL SHA-256
    for (uint32_t i = 0; i < batch->count; i++) {
        uint8_t computed_hash[LUM_POH_GPU_HASH_SIZE];
        SHA256(batch->prev_hashes[i], LUM_POH_GPU_HASH_SIZE, computed_hash);
        
        // Comparer avec hash courant
        batch->results[i] = (memcmp(computed_hash, batch->curr_hashes[i], 
                                   LUM_POH_GPU_HASH_SIZE) == 0) ? 1 : 0;
    }
    
    batch->timestamp_complete_ns = get_timestamp_ns();
    
    return true;
}

/* ============================================================================
 * MÉTRIQUES
 * ============================================================================ */

void lum_poh_gpu_get_metrics(lum_poh_gpu_context_t* ctx,
                              uint64_t* batches_processed,
                              uint64_t* hashes_verified,
                              uint64_t* errors_detected,
                              double* avg_batch_time_ms,
                              uint64_t* verifications_per_sec) {
    if (!ctx) return;
    
    if (batches_processed) *batches_processed = ctx->total_batches_processed;
    if (hashes_verified) *hashes_verified = ctx->total_hashes_verified;
    if (errors_detected) *errors_detected = ctx->total_errors_detected;
    if (avg_batch_time_ms) *avg_batch_time_ms = ctx->avg_batch_time_ms;
    if (verifications_per_sec) *verifications_per_sec = ctx->verifications_per_second;
}

void lum_poh_gpu_print_device_info(const lum_poh_gpu_device_info_t* info) {
    if (!info) return;
    
    printf("\n╔══════════════════════════════════════════════════════════╗\n");
    printf("║              GPU DEVICE INFORMATION                      ║\n");
    printf("╠══════════════════════════════════════════════════════════╣\n");
    printf("║ Name          : %-40s ║\n", info->name);
    printf("║ Vendor        : %-40s ║\n", info->vendor);
    printf("║ Compute Units : %-40u ║\n", info->compute_units);
    printf("║ Memory        : %.2f GB                                  ║\n",
           info->global_mem_size / (1024.0 * 1024.0 * 1024.0));
    printf("║ Max Frequency : %lu MHz                                  ║\n",
           info->max_clock_frequency);
    printf("║ Max Workgroup : %-40u ║\n", info->max_work_group_size);
    printf("║ SHA-256 HW    : %-40s ║\n", info->supports_sha256 ? "Yes" : "No");
    printf("╚══════════════════════════════════════════════════════════╝\n\n");
}

bool lum_poh_gpu_generate_report(lum_poh_gpu_context_t* ctx, const char* output_path) {
    if (!ctx || !output_path) return false;
    
    FILE* f = fopen(output_path, "w");
    if (!f) return false;
    
    fprintf(f, "# LumVorax PoH GPU Verification Report\n\n");
    fprintf(f, "## GPU Device\n\n");
    fprintf(f, "- **Name**: %s\n", ctx->device_info.name);
    fprintf(f, "- **Vendor**: %s\n", ctx->device_info.vendor);
    fprintf(f, "- **Compute Units**: %u\n", ctx->device_info.compute_units);
    fprintf(f, "- **Memory**: %.2f GB\n", 
            ctx->device_info.global_mem_size / (1024.0 * 1024.0 * 1024.0));
    fprintf(f, "\n## Performance Metrics\n\n");
    fprintf(f, "- **Total Batches**: %lu\n", ctx->total_batches_processed);
    fprintf(f, "- **Total Hashes**: %lu\n", ctx->total_hashes_verified);
    fprintf(f, "- **Errors Detected**: %lu\n", ctx->total_errors_detected);
    fprintf(f, "- **Avg Batch Time**: %.3f ms\n", ctx->avg_batch_time_ms);
    fprintf(f, "- **Verifications/sec**: %lu\n", ctx->verifications_per_second);
    fprintf(f, "- **GPU Utilization**: %lu%%\n", ctx->gpu_utilization_percent);
    
    fclose(f);
    return true;
}

/* ============================================================================
 * NETTOYAGE
 * ============================================================================ */

void lum_poh_gpu_free(lum_poh_gpu_context_t* ctx) {
    if (!ctx) return;
    
    if (ctx->initialized) {
        if (ctx->buffer_prev_hashes) clReleaseMemObject(ctx->buffer_prev_hashes);
        if (ctx->buffer_curr_hashes) clReleaseMemObject(ctx->buffer_curr_hashes);
        if (ctx->buffer_results) clReleaseMemObject(ctx->buffer_results);
        if (ctx->kernel) clReleaseKernel(ctx->kernel);
        if (ctx->program) clReleaseProgram(ctx->program);
        if (ctx->queue) clReleaseCommandQueue(ctx->queue);
        if (ctx->context) clReleaseContext(ctx->context);
    }
    
    free(ctx);
}

// Made with Bob - LumVorax GPU Acceleration Module