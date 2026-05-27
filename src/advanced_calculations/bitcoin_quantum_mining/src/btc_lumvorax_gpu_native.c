/* ═══════════════════════════════════════════════════════════════════════════
   LUMVORAX GPU NATIVE - Implémentation
   ═══════════════════════════════════════════════════════════════════════════
   
   Module OpenCL natif LumVorax pour Bitcoin mining avec :
   - Events asynchrones (pas de clFinish bloquant)
   - Profiling hardware bit-level (clGetEventProfilingInfo)
   - Traçabilité JIT par JIT de toutes opérations GPU
   - CPU et GPU travaillent en parallèle
   
   CYCLE: C176
   DATE: 2026-05-04
   AUTEUR: Bob (LumVorax Core Team)
   
   ARCHITECTURE:
   - Tous les uploads/downloads utilisent CL_FALSE (non-bloquant)
   - Events OpenCL pour synchronisation fine
   - clGetEventProfilingInfo() pour timings hardware précis
   - Pas de clFinish() sauf dans lum_gpu_wait_batch()
   
   ═══════════════════════════════════════════════════════════════════════════ */

#include "btc_lumvorax_gpu_native.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <pthread.h>
#include <inttypes.h>

/* ═══════════════════════════════════════════════════════════════════════════
   ÉTAT GLOBAL GPU
   ═══════════════════════════════════════════════════════════════════════════ */

static int              g_initialized = 0;
static cl_platform_id   g_platform = NULL;
static cl_device_id     g_device = NULL;
static cl_context       g_ctx = NULL;
static cl_command_queue g_queue = NULL;
static cl_program       g_program = NULL;
static cl_kernel        g_kernel_hash = NULL;
static cl_kernel        g_kernel_leading = NULL;

/* C214-DOUBLE-BUFFERING: Buffers GPU dupliqués (2 jeux: A=0, B=1) */
static cl_mem           g_buf_midstate[2] = {NULL, NULL};
static cl_mem           g_buf_tail[2] = {NULL, NULL};
static cl_mem           g_buf_result_nonce[2] = {NULL, NULL};
static cl_mem           g_buf_result_hash[2] = {NULL, NULL};
static cl_mem           g_buf_leading[2] = {NULL, NULL};

/* Configuration */
static lum_gpu_config_t g_config;
static size_t           g_current_batch_size[2] = {0, 0};  /* C214: batch size par buffer */

/* Statistiques */
static lum_gpu_stats_t  g_stats;
static pthread_mutex_t  g_stats_mutex = PTHREAD_MUTEX_INITIALIZER;

/* C214-DOUBLE-BUFFERING: Events dupliqués (2 jeux: A=0, B=1) */
static cl_event         g_event_upload_midstate[2] = {NULL, NULL};
static cl_event         g_event_upload_tail[2] = {NULL, NULL};
static cl_event         g_event_upload_sentinel[2] = {NULL, NULL};
static cl_event         g_event_kernel[2] = {NULL, NULL};
static cl_event         g_event_kernel_leading[2] = {NULL, NULL};
static cl_event         g_event_download_nonce[2] = {NULL, NULL};
static cl_event         g_event_download_hash[2] = {NULL, NULL};
static cl_event         g_event_download_leading[2] = {NULL, NULL};

/* Logging bit-level */
static int              g_bit_level_logging = 0;
static FILE*            g_log_file = NULL;

/* C214-DOUBLE-BUFFERING: Buffers leading dupliqués pour lecture dans wait_batch */
static uint32_t*        g_leading_buffer[2] = {NULL, NULL};
static size_t           g_leading_size[2] = {0, 0};

/* ═══════════════════════════════════════════════════════════════════════════
   UTILITAIRES INTERNES
   ═══════════════════════════════════════════════════════════════════════════ */

/* Obtenir timestamp nanoseconde */
static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

/* Logger bit-level (si activé) */
static void log_bit_level(const char* format, ...) {
    if (!g_bit_level_logging || !g_log_file) return;
    
    va_list args;
    va_start(args, format);
    
    uint64_t ts = get_timestamp_ns();
    fprintf(g_log_file, "[C176-BIT-LEVEL][%lu] ", ts);
    vfprintf(g_log_file, format, args);
    fprintf(g_log_file, "\n");
    fflush(g_log_file);
    
    va_end(args);
}

/* Obtenir profiling info d'un event */
static int get_event_profiling(cl_event event, uint64_t* queued, uint64_t* submit,
                                uint64_t* start, uint64_t* end) {
    cl_int err;
    
    err = clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_QUEUED,
                                   sizeof(cl_ulong), queued, NULL);
    if (err != CL_SUCCESS) return -1;
    
    err = clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_SUBMIT,
                                   sizeof(cl_ulong), submit, NULL);
    if (err != CL_SUCCESS) return -1;
    
    err = clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START,
                                   sizeof(cl_ulong), start, NULL);
    if (err != CL_SUCCESS) return -1;
    
    err = clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END,
                                   sizeof(cl_ulong), end, NULL);
    if (err != CL_SUCCESS) return -1;
    
    return 0;
}

/* Libérer event si non-NULL */
static void release_event(cl_event* event) {
    if (*event) {
        clReleaseEvent(*event);
        *event = NULL;
    }
}

/* ═══════════════════════════════════════════════════════════════════════════
   INITIALISATION GPU
   ═══════════════════════════════════════════════════════════════════════════ */

int lum_gpu_init(const lum_gpu_config_t* config) {
    cl_int err;
    cl_uint num_platforms = 0;
    
    log_bit_level("=== LUM_GPU_INIT START ===");
    log_bit_level("batch_size=%zu, workgroup_size=%zu, target_bits=%u",
                  config->batch_size, config->workgroup_size, config->target_bits);
    
    if (g_initialized) {
        log_bit_level("Already initialized, cleanup first");
        lum_gpu_cleanup();
    }
    
    /* Copier config */
    memcpy(&g_config, config, sizeof(lum_gpu_config_t));
    /* C214: Initialiser batch_size pour les 2 buffers */
    g_current_batch_size[0] = config->batch_size;
    g_current_batch_size[1] = config->batch_size;
    
    /* Ouvrir fichier log si demandé */
    if (config->enable_bit_level_log) {
        char log_path[512];
        snprintf(log_path, sizeof(log_path), "%s/lum_gpu_native_c176.log",
                 config->log_dir[0] ? config->log_dir : ".");
        g_log_file = fopen(log_path, "w");
        if (g_log_file) {
            g_bit_level_logging = 1;
            log_bit_level("Bit-level logging enabled: %s", log_path);
        }
    }
    
    /* 1. Obtenir platform */
    log_bit_level("Step 1: Get OpenCL platform");
    err = clGetPlatformIDs(1, &g_platform, &num_platforms);
    if (err != CL_SUCCESS || num_platforms == 0) {
        log_bit_level("ERROR: No OpenCL platform found (err=%d)", err);
        return LUM_GPU_ERR_NO_PLATFORM;
    }
    log_bit_level("Platform found: %p", (void*)g_platform);
    
    /* 2. Obtenir device GPU */
    log_bit_level("Step 2: Get GPU device");
    err = clGetDeviceIDs(g_platform, CL_DEVICE_TYPE_GPU, 1, &g_device, NULL);
    if (err != CL_SUCCESS) {
        log_bit_level("ERROR: No GPU device found (err=%d)", err);
        return LUM_GPU_ERR_NO_DEVICE;
    }
    
    /* Log device info */
    char device_name[256];
    clGetDeviceInfo(g_device, CL_DEVICE_NAME, sizeof(device_name), device_name, NULL);
    log_bit_level("Device found: %s", device_name);
    
    cl_ulong global_mem, local_mem;
    clGetDeviceInfo(g_device, CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(cl_ulong), &global_mem, NULL);
    clGetDeviceInfo(g_device, CL_DEVICE_LOCAL_MEM_SIZE, sizeof(cl_ulong), &local_mem, NULL);
    log_bit_level("Global memory: %lu MB, Local memory: %lu KB",
                  global_mem / (1024*1024), local_mem / 1024);
    
    /* 3. Créer context */
    log_bit_level("Step 3: Create OpenCL context");
    g_ctx = clCreateContext(NULL, 1, &g_device, NULL, NULL, &err);
    if (err != CL_SUCCESS) {
        log_bit_level("ERROR: Failed to create context (err=%d)", err);
        return LUM_GPU_ERR_CONTEXT;
    }
    log_bit_level("Context created: %p", (void*)g_ctx);
    
    /* 4. Créer command queue avec profiling */
    log_bit_level("Step 4: Create command queue with profiling");
    cl_command_queue_properties props = CL_QUEUE_PROFILING_ENABLE;
    g_queue = clCreateCommandQueue(g_ctx, g_device, props, &err);
    if (err != CL_SUCCESS) {
        log_bit_level("ERROR: Failed to create queue (err=%d)", err);
        return LUM_GPU_ERR_QUEUE;
    }
    log_bit_level("Queue created: %p (profiling enabled)", (void*)g_queue);
    
    /* 5. Charger et compiler kernel */
    log_bit_level("Step 5: Load and compile kernel");
    FILE* f = fopen("src/btc_sha256.cl", "r");
    if (!f) {
        log_bit_level("ERROR: Cannot open btc_sha256.cl");
        return LUM_GPU_ERR_PROGRAM;
    }
    
    fseek(f, 0, SEEK_END);
    size_t kernel_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char* kernel_source = (char*)malloc(kernel_size + 1);
    size_t read_bytes = fread(kernel_source, 1, kernel_size, f);
    if (read_bytes != kernel_size) {
        log_bit_level("[C177-WARN] Kernel read incomplete: %zu/%zu bytes\n", read_bytes, kernel_size);
    }
    kernel_source[kernel_size] = '\0';
    fclose(f);
    
    log_bit_level("Kernel source loaded: %zu bytes", kernel_size);
    
    g_program = clCreateProgramWithSource(g_ctx, 1, (const char**)&kernel_source,
                                          &kernel_size, &err);
    free(kernel_source);
    
    if (err != CL_SUCCESS) {
        log_bit_level("ERROR: Failed to create program (err=%d)", err);
        return LUM_GPU_ERR_PROGRAM;
    }
    
    /* Compiler avec optimisations */
    log_bit_level("Compiling kernel with optimizations...");
    err = clBuildProgram(g_program, 1, &g_device,
                         "-cl-fast-relaxed-math -cl-mad-enable", NULL, NULL);
    
    if (err != CL_SUCCESS) {
        log_bit_level("ERROR: Kernel compilation failed (err=%d)", err);
        
        /* Log build errors */
        char build_log[4096];
        clGetProgramBuildInfo(g_program, g_device, CL_PROGRAM_BUILD_LOG,
                             sizeof(build_log), build_log, NULL);
        log_bit_level("Build log:\n%s", build_log);
        
        return LUM_GPU_ERR_PROGRAM;
    }
    log_bit_level("Kernel compiled successfully");
    
    /* 6. Créer kernels */
    log_bit_level("Step 6: Create kernel objects");
    g_kernel_hash = clCreateKernel(g_program, "btc_double_hash", &err);
    if (err != CL_SUCCESS) {
        log_bit_level("ERROR: Failed to create btc_double_hash kernel (err=%d)", err);
        return LUM_GPU_ERR_KERNEL;
    }
    log_bit_level("Kernel btc_double_hash created: %p", (void*)g_kernel_hash);
    
    g_kernel_leading = clCreateKernel(g_program, "btc_count_leading", &err);
    if (err != CL_SUCCESS) {
        log_bit_level("ERROR: Failed to create btc_count_leading kernel (err=%d)", err);
        return LUM_GPU_ERR_KERNEL;
    }
    log_bit_level("Kernel btc_count_leading created: %p", (void*)g_kernel_leading);
    
    /* C214-DOUBLE-BUFFERING: Créer 2 jeux complets de buffers GPU (A=0, B=1) */
    log_bit_level("Step 7: Create GPU buffers (batch_size=%zu, DOUBLE-BUFFERED)", config->batch_size);
    
    for (int buf_idx = 0; buf_idx < 2; buf_idx++) {
        log_bit_level("Creating buffer set %d (A=0, B=1)...", buf_idx);
        
        g_buf_midstate[buf_idx] = clCreateBuffer(g_ctx, CL_MEM_READ_ONLY,
                                        8 * sizeof(uint32_t), NULL, &err);
        if (err != CL_SUCCESS) {
            log_bit_level("ERROR: Failed to create midstate buffer %d (err=%d)", buf_idx, err);
            return LUM_GPU_ERR_BUFFER;
        }
        log_bit_level("Buffer midstate[%d]: %p (32 bytes)", buf_idx, (void*)g_buf_midstate[buf_idx]);
        
        g_buf_tail[buf_idx] = clCreateBuffer(g_ctx, CL_MEM_READ_ONLY,
                                    4 * sizeof(uint32_t), NULL, &err);
        if (err != CL_SUCCESS) {
            log_bit_level("ERROR: Failed to create tail buffer %d (err=%d)", buf_idx, err);
            return LUM_GPU_ERR_BUFFER;
        }
        log_bit_level("Buffer tail[%d]: %p (16 bytes)", buf_idx, (void*)g_buf_tail[buf_idx]);
        
        g_buf_result_nonce[buf_idx] = clCreateBuffer(g_ctx, CL_MEM_WRITE_ONLY,
                                            sizeof(uint32_t), NULL, &err);
        if (err != CL_SUCCESS) {
            log_bit_level("ERROR: Failed to create result_nonce buffer %d (err=%d)", buf_idx, err);
            return LUM_GPU_ERR_BUFFER;
        }
        log_bit_level("Buffer result_nonce[%d]: %p (4 bytes)", buf_idx, (void*)g_buf_result_nonce[buf_idx]);
        
        g_buf_result_hash[buf_idx] = clCreateBuffer(g_ctx, CL_MEM_WRITE_ONLY,
                                           8 * sizeof(uint32_t), NULL, &err);
        if (err != CL_SUCCESS) {
            log_bit_level("ERROR: Failed to create result_hash buffer %d (err=%d)", buf_idx, err);
            return LUM_GPU_ERR_BUFFER;
        }
        log_bit_level("Buffer result_hash[%d]: %p (32 bytes)", buf_idx, (void*)g_buf_result_hash[buf_idx]);
        
        g_buf_leading[buf_idx] = clCreateBuffer(g_ctx, CL_MEM_WRITE_ONLY,
                                       config->batch_size * sizeof(uint32_t), NULL, &err);
        if (err != CL_SUCCESS) {
            log_bit_level("ERROR: Failed to create leading buffer %d (err=%d)", buf_idx, err);
            return LUM_GPU_ERR_BUFFER;
        }
        log_bit_level("Buffer leading[%d]: %p (%zu bytes)", buf_idx,
                      (void*)g_buf_leading[buf_idx], config->batch_size * sizeof(uint32_t));
    }
    
    log_bit_level("Double-buffering complete: 2 × 5 buffers = 10 buffers total");
    
    /* 8. Warm-up JIT compiler */
    log_bit_level("Step 8: Warm-up JIT compiler");
    uint32_t dummy_midstate[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };
    uint32_t dummy_tail[4] = { 0, 0, 0, 0 };
    uint32_t dummy_nonce = 0xFFFFFFFFu;
    uint32_t dummy_hash[8] = {0};
    uint32_t dummy_bits = 0;
    
    uint64_t warmup_start = get_timestamp_ns();
    lum_gpu_mine_batch_async(0, dummy_midstate, dummy_tail, 0, 1024, 32,
                             &dummy_nonce, dummy_hash, &dummy_bits);
    lum_gpu_wait_batch(0, &dummy_bits);
    uint64_t warmup_end = get_timestamp_ns();
    
    log_bit_level("Warm-up completed in %.3f ms",
                  (warmup_end - warmup_start) / 1000000.0);
    
    /* Reset stats */
    memset(&g_stats, 0, sizeof(lum_gpu_stats_t));
    
    g_initialized = 1;
    log_bit_level("=== LUM_GPU_INIT SUCCESS ===");
    
    return LUM_GPU_OK;
}

/* ═══════════════════════════════════════════════════════════════════════════
   MINING ASYNCHRONE
   ═══════════════════════════════════════════════════════════════════════════ */

int lum_gpu_mine_batch_async(
    int             buffer_index,     /* C214: 0 ou 1 pour double-buffering */
    const uint32_t  midstate[8],
    const uint32_t  tail[4],
    uint32_t        nonce_start,
    size_t          batch_size,
    uint32_t        target_bits,
    uint32_t*       out_nonce,
    uint32_t        out_hash[8],
    uint32_t*       out_best_bits
) {
    if (!g_initialized) return LUM_GPU_ERR_NOT_INIT;
    if (buffer_index < 0 || buffer_index > 1) return LUM_GPU_ERR_RUNTIME;
    
    int idx = buffer_index;  /* Raccourci pour lisibilité */
    
    cl_int err;
    uint64_t batch_start_ns = get_timestamp_ns();
    
    log_bit_level("=== MINE_BATCH_ASYNC START (buffer=%d) ===", idx);
    log_bit_level("nonce_start=%u, batch_size=%zu, target_bits=%u",
                  nonce_start, batch_size, target_bits);
    
    /* C214: Libérer events précédents de CE buffer uniquement */
    release_event(&g_event_upload_midstate[idx]);
    release_event(&g_event_upload_tail[idx]);
    release_event(&g_event_upload_sentinel[idx]);
    release_event(&g_event_kernel[idx]);
    release_event(&g_event_kernel_leading[idx]);
    release_event(&g_event_download_nonce[idx]);
    release_event(&g_event_download_hash[idx]);
    release_event(&g_event_download_leading[idx]);
    
    /* C214: Réallouer buffer leading[idx] si batch_size a changé */
    if (batch_size != g_current_batch_size[idx]) {
        log_bit_level("Buffer[%d]: Batch size changed: %zu -> %zu, reallocating leading buffer",
                      idx, g_current_batch_size[idx], batch_size);
        if (g_buf_leading[idx]) {
            clReleaseMemObject(g_buf_leading[idx]);
        }
        g_buf_leading[idx] = clCreateBuffer(g_ctx, CL_MEM_WRITE_ONLY,
                                            batch_size * sizeof(uint32_t), NULL, &err);
        if (err != CL_SUCCESS) {
            log_bit_level("ERROR: Failed to reallocate leading buffer[%d] (err=%d)", idx, err);
            return LUM_GPU_ERR_BUFFER;
        }
        g_current_batch_size[idx] = batch_size;
        g_leading_size[idx] = batch_size;
    }
    
    /* 1. Upload midstate (ASYNCHRONE avec event) */
    log_bit_level("Upload midstate[%d] (32 bytes, async)", idx);
    err = clEnqueueWriteBuffer(g_queue, g_buf_midstate[idx], CL_FALSE, 0,
                               8 * sizeof(uint32_t), midstate,
                               0, NULL, &g_event_upload_midstate[idx]);
    if (err != CL_SUCCESS) {
        log_bit_level("ERROR: Upload midstate[%d] failed (err=%d)", idx, err);
        return LUM_GPU_ERR_RUNTIME;
    }
    
    /* 2. Upload tail (ASYNCHRONE avec event) */
    log_bit_level("Upload tail[%d] (16 bytes, async)", idx);
    err = clEnqueueWriteBuffer(g_queue, g_buf_tail[idx], CL_FALSE, 0,
                               4 * sizeof(uint32_t), tail,
                               0, NULL, &g_event_upload_tail[idx]);
    if (err != CL_SUCCESS) {
        log_bit_level("ERROR: Upload tail[%d] failed (err=%d)", idx, err);
        return LUM_GPU_ERR_RUNTIME;
    }
    
    /* 3. Upload sentinel (ASYNCHRONE avec event) */
    uint32_t sentinel = 0xFFFFFFFFu;
    log_bit_level("Upload sentinel[%d] (4 bytes, async)", idx);
    err = clEnqueueWriteBuffer(g_queue, g_buf_result_nonce[idx], CL_FALSE, 0,
                               sizeof(uint32_t), &sentinel,
                               0, NULL, &g_event_upload_sentinel[idx]);
    if (err != CL_SUCCESS) {
        log_bit_level("ERROR: Upload sentinel[%d] failed (err=%d)", idx, err);
        return LUM_GPU_ERR_RUNTIME;
    }
    
    /* 4. Set kernel arguments */
    log_bit_level("Set kernel arguments (buffer=%d)", idx);
    clSetKernelArg(g_kernel_hash, 0, sizeof(cl_mem), &g_buf_midstate[idx]);
    clSetKernelArg(g_kernel_hash, 1, sizeof(cl_mem), &g_buf_tail[idx]);
    clSetKernelArg(g_kernel_hash, 2, sizeof(uint32_t), &nonce_start);
    clSetKernelArg(g_kernel_hash, 3, sizeof(uint32_t), &target_bits);
    clSetKernelArg(g_kernel_hash, 4, sizeof(cl_mem), &g_buf_result_nonce[idx]);
    clSetKernelArg(g_kernel_hash, 5, sizeof(cl_mem), &g_buf_result_hash[idx]);
    
    /* 5. Launch kernel (ASYNCHRONE, attend les 3 uploads de CE buffer) */
    size_t global_size = batch_size;
    size_t workgroup_size = g_config.workgroup_size;
    
    log_bit_level("Launch kernel[%d]: global=%zu, workgroup=%zu", idx, global_size, workgroup_size);
    
    cl_event upload_events[3] = {
        g_event_upload_midstate[idx],
        g_event_upload_tail[idx],
        g_event_upload_sentinel[idx]
    };
    
    err = clEnqueueNDRangeKernel(g_queue, g_kernel_hash, 1, NULL,
                                 &global_size, &workgroup_size,
                                 3, upload_events, &g_event_kernel[idx]);
    if (err != CL_SUCCESS) {
        log_bit_level("ERROR: Kernel[%d] launch failed (err=%d)", idx, err);
        return LUM_GPU_ERR_RUNTIME;
    }
    
    /* 6. Launch leading zeros kernel (ASYNCHRONE, attend kernel principal) */
    if (out_best_bits) {
        log_bit_level("Launch btc_count_leading kernel[%d]", idx);
        clSetKernelArg(g_kernel_leading, 0, sizeof(cl_mem), &g_buf_midstate[idx]);
        clSetKernelArg(g_kernel_leading, 1, sizeof(cl_mem), &g_buf_tail[idx]);
        clSetKernelArg(g_kernel_leading, 2, sizeof(uint32_t), &nonce_start);
        clSetKernelArg(g_kernel_leading, 3, sizeof(cl_mem), &g_buf_leading[idx]);
        
        /* C213-BUG-FIX: Capturer event kernel leading pour synchronisation correcte */
        clEnqueueNDRangeKernel(g_queue, g_kernel_leading, 1, NULL,
                               &global_size, &workgroup_size,
                               1, &g_event_kernel[idx], &g_event_kernel_leading[idx]);
    }
    
    /* 7. Download result nonce (ASYNCHRONE, attend kernel) */
    log_bit_level("Download result_nonce[%d] (4 bytes, async)", idx);
    err = clEnqueueReadBuffer(g_queue, g_buf_result_nonce[idx], CL_FALSE, 0,
                              sizeof(uint32_t), out_nonce,
                              1, &g_event_kernel[idx], &g_event_download_nonce[idx]);
    if (err != CL_SUCCESS) {
        log_bit_level("ERROR: Download nonce[%d] failed (err=%d)", idx, err);
        return LUM_GPU_ERR_RUNTIME;
    }
    
    /* 8. Download result hash (ASYNCHRONE, attend kernel) */
    log_bit_level("Download result_hash[%d] (32 bytes, async)", idx);
    err = clEnqueueReadBuffer(g_queue, g_buf_result_hash[idx], CL_FALSE, 0,
                              8 * sizeof(uint32_t), out_hash,
                              1, &g_event_kernel[idx], &g_event_download_hash[idx]);
    if (err != CL_SUCCESS) {
        log_bit_level("ERROR: Download hash[%d] failed (err=%d)", idx, err);
        return LUM_GPU_ERR_RUNTIME;
    }
    
    /* 9. Download leading zeros (ASYNCHRONE, attend kernel leading) */
    if (out_best_bits) {
        log_bit_level("Download leading zeros[%d] (%zu bytes, async)",
                      idx, batch_size * sizeof(uint32_t));
        
        /* Allouer buffer temporaire (sera lu dans wait_batch) */
        uint32_t* leading = (uint32_t*)malloc(batch_size * sizeof(uint32_t));
        if (!leading) {
            log_bit_level("ERROR: Failed to allocate leading buffer[%d]", idx);
            return LUM_GPU_ERR_RUNTIME;
        }
        
        /* C213-BUG-FIX: Attendre g_event_kernel_leading[idx] au lieu de g_event_kernel[idx] */
        cl_event wait_event = g_event_kernel_leading[idx] ? g_event_kernel_leading[idx] : g_event_kernel[idx];
        err = clEnqueueReadBuffer(g_queue, g_buf_leading[idx], CL_FALSE, 0,
                                  batch_size * sizeof(uint32_t), leading,
                                  1, &wait_event, &g_event_download_leading[idx]);
        if (err != CL_SUCCESS) {
            log_bit_level("ERROR: Download leading[%d] failed (err=%d)", idx, err);
            free(leading);
            return LUM_GPU_ERR_RUNTIME;
        }
        
        /* C213-BUG-FIX: Stocker pointeur pour wait_batch */
        if (g_leading_buffer[idx]) free(g_leading_buffer[idx]);
        g_leading_buffer[idx] = leading;
        g_leading_size[idx] = batch_size;
    }
    
    uint64_t batch_queued_ns = get_timestamp_ns();
    log_bit_level("All operations queued in %.3f ms (buffer=%d)",
                  (batch_queued_ns - batch_start_ns) / 1000000.0, idx);
    log_bit_level("=== MINE_BATCH_ASYNC QUEUED (CPU FREE, buffer=%d) ===", idx);
    
    /* CPU est maintenant LIBRE pendant que GPU travaille ! */
    
    return LUM_GPU_OK;
}

/* ═══════════════════════════════════════════════════════════════════════════
   C214-DOUBLE-BUFFERING: ATTENDRE FIN BATCH AVEC BUFFER INDEX
   ═══════════════════════════════════════════════════════════════════════════ */

int lum_gpu_wait_batch(int buffer_index, uint32_t* out_best_bits) {
    if (!g_initialized) return LUM_GPU_ERR_NOT_INIT;
    if (buffer_index < 0 || buffer_index > 1) return LUM_GPU_ERR_RUNTIME;
    
    int idx = buffer_index;
    
    log_bit_level("=== WAIT_BATCH START (buffer=%d) ===", idx);
    uint64_t wait_start_ns = get_timestamp_ns();
    
    /* Attendre UNIQUEMENT le dernier event (download_leading ou download_hash) */
    cl_event wait_event = g_event_download_leading[idx] ? g_event_download_leading[idx]
                                                         : g_event_download_hash[idx];
    
    if (!wait_event) {
        log_bit_level("WARNING: No event to wait for (buffer=%d)", idx);
        return LUM_GPU_OK;
    }
    
    log_bit_level("Waiting for event[%d]: %p", idx, (void*)wait_event);
    clWaitForEvents(1, &wait_event);
    
    uint64_t wait_end_ns = get_timestamp_ns();
    uint64_t cpu_wait_ns = wait_end_ns - wait_start_ns;
    
    log_bit_level("CPU wait completed in %.3f ms (buffer=%d)", cpu_wait_ns / 1000000.0, idx);
    
    /* C213-BUG-FIX: Lire buffer leading et calculer best_leading_zeros_gpu */
    if (out_best_bits && g_leading_buffer[idx] && g_leading_size[idx] > 0) {
        uint32_t best_lz = 0;
        for (size_t i = 0; i < g_leading_size[idx]; i++) {
            if (g_leading_buffer[idx][i] > best_lz) {
                best_lz = g_leading_buffer[idx][i];
            }
        }
        *out_best_bits = best_lz;
        g_stats.best_leading_zeros_gpu = best_lz;
        log_bit_level("Best leading zeros GPU[%d]: %u bits (scanned %zu nonces)",
                      idx, best_lz, g_leading_size[idx]);
        
        /* Libérer buffer après lecture */
        free(g_leading_buffer[idx]);
        g_leading_buffer[idx] = NULL;
        g_leading_size[idx] = 0;
    }
    
    /* Extraire profiling info de tous les events */
    if (g_config.enable_profiling) {
        log_bit_level("=== PROFILING INFO (buffer=%d) ===", idx);
        
        uint64_t q, s, start, end;
        
        /* C219-PIPELINE-METRICS : Variables pour calculer métriques pipeline */
        uint64_t kernel_queued = 0, kernel_submit = 0, kernel_start = 0, kernel_end = 0;
        
        /* Upload midstate */
        if (g_event_upload_midstate[idx] &&
            get_event_profiling(g_event_upload_midstate[idx], &q, &s, &start, &end) == 0) {
            g_stats.upload_midstate_ns = end - start;
            log_bit_level("Upload midstate[%d]: %.3f us", idx, g_stats.upload_midstate_ns / 1000.0);
        }
        
        /* Upload tail */
        if (g_event_upload_tail[idx] &&
            get_event_profiling(g_event_upload_tail[idx], &q, &s, &start, &end) == 0) {
            g_stats.upload_tail_ns = end - start;
            log_bit_level("Upload tail[%d]: %.3f us", idx, g_stats.upload_tail_ns / 1000.0);
        }
        
        /* Upload sentinel */
        if (g_event_upload_sentinel[idx] &&
            get_event_profiling(g_event_upload_sentinel[idx], &q, &s, &start, &end) == 0) {
            g_stats.upload_sentinel_ns = end - start;
            log_bit_level("Upload sentinel[%d]: %.3f us", idx, g_stats.upload_sentinel_ns / 1000.0);
        }
        
        /* C219-PIPELINE-METRICS : Kernel execution avec timestamps complets */
        if (g_event_kernel[idx] &&
            get_event_profiling(g_event_kernel[idx], &q, &s, &start, &end) == 0) {
            g_stats.kernel_exec_ns = end - start;
            kernel_queued = q;
            kernel_submit = s;
            kernel_start = start;
            kernel_end = end;
            log_bit_level("Kernel execution[%d]: %.3f ms", idx, g_stats.kernel_exec_ns / 1000000.0);
            log_bit_level("  queued→submit: %.3f us", (kernel_submit - kernel_queued) / 1000.0);
            log_bit_level("  submit→start:  %.3f us", (kernel_start - kernel_submit) / 1000.0);
            log_bit_level("  start→end:     %.3f ms", (kernel_end - kernel_start) / 1000000.0);
        }
        
        /* Download nonce */
        if (g_event_download_nonce[idx] &&
            get_event_profiling(g_event_download_nonce[idx], &q, &s, &start, &end) == 0) {
            g_stats.download_result_ns = end - start;
            log_bit_level("Download nonce[%d]: %.3f us", idx, g_stats.download_result_ns / 1000.0);
        }
        
        /* Download hash */
        if (g_event_download_hash[idx] &&
            get_event_profiling(g_event_download_hash[idx], &q, &s, &start, &end) == 0) {
            g_stats.download_hash_ns = end - start;
            log_bit_level("Download hash[%d]: %.3f us", idx, g_stats.download_hash_ns / 1000.0);
        }
        
        /* Download leading */
        if (g_event_download_leading[idx] &&
            get_event_profiling(g_event_download_leading[idx], &q, &s, &start, &end) == 0) {
            g_stats.download_leading_ns = end - start;
            log_bit_level("Download leading[%d]: %.3f ms", idx, g_stats.download_leading_ns / 1000000.0);
        }
        
        /* C219-PIPELINE-METRICS : Calculer métriques pipeline OpenCL */
        if (kernel_queued > 0 && kernel_end > kernel_queued) {
            g_stats.gpu_queue_wait_ns = kernel_submit - kernel_queued;
            g_stats.gpu_transfer_ns = kernel_start - kernel_submit;
            g_stats.gpu_total_pipeline_ns = kernel_end - kernel_queued;
            
            log_bit_level("=== C219-PIPELINE-METRICS ===");
            log_bit_level("Queue wait:      %.3f us", g_stats.gpu_queue_wait_ns / 1000.0);
            log_bit_level("Transfer/setup:  %.3f us", g_stats.gpu_transfer_ns / 1000.0);
            log_bit_level("Total pipeline:  %.3f ms", g_stats.gpu_total_pipeline_ns / 1000000.0);
        }
        
        /* Totaux */
        g_stats.total_gpu_time_ns = g_stats.upload_midstate_ns +
                                    g_stats.upload_tail_ns +
                                    g_stats.upload_sentinel_ns +
                                    g_stats.kernel_exec_ns +
                                    g_stats.download_result_ns +
                                    g_stats.download_hash_ns +
                                    g_stats.download_leading_ns;
        
        g_stats.total_cpu_wait_ns = cpu_wait_ns;
        
        log_bit_level("Total GPU time: %.3f ms", g_stats.total_gpu_time_ns / 1000000.0);
        log_bit_level("Total CPU wait: %.3f ms", g_stats.total_cpu_wait_ns / 1000000.0);
        
        /* Calculer performance */
        if (g_stats.kernel_exec_ns > 0) {
            double kernel_sec = g_stats.kernel_exec_ns / 1000000000.0;
            g_stats.current_mhs = (g_current_batch_size[idx] / kernel_sec) / 1000000.0;
            log_bit_level("Current hashrate[%d]: %.3f MH/s", idx, g_stats.current_mhs);
        }
        
        /* C219-FIX : Utiliser gpu_total_pipeline_ns au lieu de kernel_exec_ns
         * AVANT C219 : kernel_exec_ns mesure seulement temps kernel pur (start→end)
         *              → ignore queue wait, transfer, setup → métrique "GPU actif 12.9%" fausse
         * APRES C219 : gpu_total_pipeline_ns mesure temps pipeline complet (queued→end)
         *              → inclut TOUT le temps GPU occupé → métrique réelle attendue >80% */
        if (cpu_wait_ns > 1000000) {  /* > 1ms */
            /* C219-PIPELINE-METRICS : Utiliser pipeline complet si disponible */
            uint64_t gpu_time_for_util = (g_stats.gpu_total_pipeline_ns > 0)
                                         ? g_stats.gpu_total_pipeline_ns
                                         : g_stats.kernel_exec_ns;
            
            g_stats.gpu_utilization_pct = (double)gpu_time_for_util /
                                          (double)cpu_wait_ns * 100.0;
            
            /* Plafonner à 100% pour éviter valeurs aberrantes */
            if (g_stats.gpu_utilization_pct > 100.0) {
                g_stats.gpu_utilization_pct = 100.0;
            }
            g_stats.cpu_free_pct = 100.0 - g_stats.gpu_utilization_pct;
            
            log_bit_level("GPU utilization[%d]: %.1f%% (pipeline=%s), CPU free: %.1f%%",
                          idx, g_stats.gpu_utilization_pct,
                          (g_stats.gpu_total_pipeline_ns > 0) ? "FULL" : "kernel_only",
                          g_stats.cpu_free_pct);
        } else {
            /* cpu_wait_ns trop petit → valeur invalide */
            g_stats.gpu_utilization_pct = 0.0;
            g_stats.cpu_free_pct = 100.0;
            log_bit_level("GPU utilization[%d]: INVALID (cpu_wait_ns=%"PRIu64" ns < 1ms)",
                          idx, cpu_wait_ns);
        }
    }
    
    /* Mettre à jour compteurs */
    pthread_mutex_lock(&g_stats_mutex);
    g_stats.total_batches++;
    g_stats.total_hashes += g_current_batch_size[idx];
    
    if (g_stats.current_mhs > g_stats.peak_mhs) {
        g_stats.peak_mhs = g_stats.current_mhs;
    }
    
    if (g_stats.total_batches > 0) {
        g_stats.average_mhs = (g_stats.total_hashes / 1000000.0) /
                              (g_stats.total_gpu_time_ns / 1000000000.0);
    }
    pthread_mutex_unlock(&g_stats_mutex);
    
    log_bit_level("=== WAIT_BATCH COMPLETE (buffer=%d) ===", idx);
    
    return LUM_GPU_OK;
}

/* ═══════════════════════════════════════════════════════════════════════════
   STATISTIQUES
   ═══════════════════════════════════════════════════════════════════════════ */

int lum_gpu_get_stats(lum_gpu_stats_t* stats) {
    if (!g_initialized) return LUM_GPU_ERR_NOT_INIT;
    
    pthread_mutex_lock(&g_stats_mutex);
    memcpy(stats, &g_stats, sizeof(lum_gpu_stats_t));
    pthread_mutex_unlock(&g_stats_mutex);
    
    return LUM_GPU_OK;
}

void lum_gpu_reset_stats(void) {
    pthread_mutex_lock(&g_stats_mutex);
    memset(&g_stats, 0, sizeof(lum_gpu_stats_t));
    pthread_mutex_unlock(&g_stats_mutex);
}

/* ═══════════════════════════════════════════════════════════════════════════
   BENCHMARK
   ═══════════════════════════════════════════════════════════════════════════ */

double lum_gpu_benchmark_mhs(size_t batch_size, double duration_sec) {
    if (!g_initialized) return 0.0;
    
    log_bit_level("=== BENCHMARK START ===");
    log_bit_level("batch_size=%zu, duration=%.1f sec", batch_size, duration_sec);
    
    uint32_t midstate[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };
    uint32_t tail[4] = { 0x61736400, 0x3b9ac9ff, 0x17a950d2, 0x00000000 };
    
    lum_gpu_reset_stats();
    
    uint64_t start_ns = get_timestamp_ns();
    uint64_t end_ns = start_ns + (uint64_t)(duration_sec * 1000000000.0);
    uint64_t total_hashes = 0;
    uint32_t nonce = 0;
    
    while (get_timestamp_ns() < end_ns) {
        uint32_t out_nonce = 0xFFFFFFFFu;
        uint32_t out_hash[8] = {0};
        uint32_t out_bits = 0;
        
        lum_gpu_mine_batch_async(0, midstate, tail, nonce, batch_size, 32,
                                 &out_nonce, out_hash, &out_bits);
        lum_gpu_wait_batch(0, &out_bits);
        
        total_hashes += batch_size;
        nonce += batch_size;
    }
    
    uint64_t actual_ns = get_timestamp_ns() - start_ns;
    double actual_sec = actual_ns / 1000000000.0;
    double mhs = (total_hashes / actual_sec) / 1000000.0;
    
    log_bit_level("Benchmark complete: %.3f MH/s (%.1f sec, %lu hashes)",
                  mhs, actual_sec, total_hashes);
    log_bit_level("=== BENCHMARK END ===");
    
    return mhs;
}

/* ═══════════════════════════════════════════════════════════════════════════
   CLEANUP
   ═══════════════════════════════════════════════════════════════════════════ */

void lum_gpu_cleanup(void) {
    if (!g_initialized) return;
    
    log_bit_level("=== LUM_GPU_CLEANUP START (C214: double-buffering) ===");
    
    /* C214: Libérer events des 2 buffers */
    for (int idx = 0; idx < 2; idx++) {
        log_bit_level("Releasing events for buffer[%d]", idx);
        release_event(&g_event_upload_midstate[idx]);
        release_event(&g_event_upload_tail[idx]);
        release_event(&g_event_upload_sentinel[idx]);
        release_event(&g_event_kernel[idx]);
        release_event(&g_event_kernel_leading[idx]);
        release_event(&g_event_download_nonce[idx]);
        release_event(&g_event_download_hash[idx]);
        release_event(&g_event_download_leading[idx]);
    }
    
    /* C214: Libérer buffers des 2 jeux */
    for (int idx = 0; idx < 2; idx++) {
        log_bit_level("Releasing GPU buffers for buffer[%d]", idx);
        if (g_buf_midstate[idx]) clReleaseMemObject(g_buf_midstate[idx]);
        if (g_buf_tail[idx]) clReleaseMemObject(g_buf_tail[idx]);
        if (g_buf_result_nonce[idx]) clReleaseMemObject(g_buf_result_nonce[idx]);
        if (g_buf_result_hash[idx]) clReleaseMemObject(g_buf_result_hash[idx]);
        if (g_buf_leading[idx]) clReleaseMemObject(g_buf_leading[idx]);
        
        /* Libérer buffer leading CPU */
        if (g_leading_buffer[idx]) {
            free(g_leading_buffer[idx]);
            g_leading_buffer[idx] = NULL;
        }
    }
    
    /* Libérer kernels */
    if (g_kernel_hash) clReleaseKernel(g_kernel_hash);
    if (g_kernel_leading) clReleaseKernel(g_kernel_leading);
    
    /* Libérer program */
    if (g_program) clReleaseProgram(g_program);
    
    /* Libérer queue */
    if (g_queue) clReleaseCommandQueue(g_queue);
    
    /* Libérer context */
    if (g_ctx) clReleaseContext(g_ctx);
    
    /* Fermer log */
    if (g_log_file) {
        fclose(g_log_file);
        g_log_file = NULL;
    }
    
    g_initialized = 0;
    log_bit_level("=== LUM_GPU_CLEANUP COMPLETE ===");
}

/* ═══════════════════════════════════════════════════════════════════════════
   PROFILING BIT-LEVEL
   ═══════════════════════════════════════════════════════════════════════════ */

void lum_gpu_set_bit_level_logging(int enable) {
    g_bit_level_logging = enable;
}

int lum_gpu_get_last_event_info(uint64_t* queued_ns, uint64_t* submit_ns,
                                 uint64_t* start_ns, uint64_t* end_ns) {
    /* C221-FIX : g_event_kernel est un tableau statique, vérifier si initialisé */
    if (!g_ctx) return -1;  /* Vérifier contexte au lieu de l'adresse du tableau */
    return get_event_profiling(*g_event_kernel, queued_ns, submit_ns, start_ns, end_ns);
}

void lum_gpu_dump_state(const char* filename) {
    FILE* f = fopen(filename, "w");
    if (!f) return;
    
    fprintf(f, "=== LUMVORAX GPU NATIVE STATE DUMP ===\n\n");
    fprintf(f, "Initialized: %d\n", g_initialized);
    fprintf(f, "Platform: %p\n", (void*)g_platform);
    fprintf(f, "Device: %p\n", (void*)g_device);
    fprintf(f, "Context: %p\n", (void*)g_ctx);
    fprintf(f, "Queue: %p\n", (void*)g_queue);
    fprintf(f, "Program: %p\n", (void*)g_program);
    fprintf(f, "Kernel hash: %p\n", (void*)g_kernel_hash);
    fprintf(f, "Kernel leading: %p\n", (void*)g_kernel_leading);
    fprintf(f, "\nBuffers:\n");
    fprintf(f, "  midstate: %p\n", (void*)g_buf_midstate);
    fprintf(f, "  tail: %p\n", (void*)g_buf_tail);
    fprintf(f, "  result_nonce: %p\n", (void*)g_buf_result_nonce);
    fprintf(f, "  result_hash: %p\n", (void*)g_buf_result_hash);
    fprintf(f, "  leading: %p\n", (void*)g_buf_leading);
    fprintf(f, "\nConfiguration:\n");
    fprintf(f, "  batch_size: %zu\n", g_config.batch_size);
    fprintf(f, "  workgroup_size: %zu\n", g_config.workgroup_size);
    fprintf(f, "  target_bits: %u\n", g_config.target_bits);
    fprintf(f, "  profiling: %d\n", g_config.enable_profiling);
    fprintf(f, "  bit_level_log: %d\n", g_config.enable_bit_level_log);
    fprintf(f, "\nStatistics:\n");
    fprintf(f, "  total_batches: %lu\n", g_stats.total_batches);
    fprintf(f, "  total_hashes: %lu\n", g_stats.total_hashes);
    fprintf(f, "  current_mhs: %.3f\n", g_stats.current_mhs);
    fprintf(f, "  average_mhs: %.3f\n", g_stats.average_mhs);
    fprintf(f, "  peak_mhs: %.3f\n", g_stats.peak_mhs);
    fprintf(f, "  gpu_utilization: %.1f%%\n", g_stats.gpu_utilization_pct);
    fprintf(f, "  cpu_free: %.1f%%\n", g_stats.cpu_free_pct);
    
    fclose(f);
}

// Made with Bob
