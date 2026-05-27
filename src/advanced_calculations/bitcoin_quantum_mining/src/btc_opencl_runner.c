/*
 * LumVorax C63 — Runner OpenCL SHA-256 pour Intel UHD 620
 * ==========================================================
 * Charge le kernel btc_sha256.cl et exécute le double-hash SHA-256
 * sur GPU Intel intégré.
 *
 * Hashrate cible : 50-200 MH/s (vs 2 MH/s CPU i5-8265U)
 * Compatibilité : Intel OpenCL ICD 2.0 + NEO driver
 *
 * Build : inclus automatiquement dans Makefile si CL_AVAILABLE=1
 * STANDARD_NAMES.md v4.4 §M-BTC-GPU-C63
 *
 * Usage depuis btc_mining_engine.c :
 *   if (nx48_ctrl_gpu_active) btc_opencl_init(...);
 *   btc_opencl_mine_batch(midstate, tail, nonce_start, batch_size, ...);
 *   btc_opencl_cleanup();
 */

#include "btc_opencl_runner.h"
#include "../include/btc_mining_forensic.h"  /* C170 : BTC_FORENSIC_HASH */
#include "btc_gpu_async_logger.h"            /* C171 : Logging GPU asynchrone */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#ifdef BTC_OPENCL_ENABLED

/* ── État global OpenCL ──────────────────────────────────────────── */
static cl_context       g_ctx       = NULL;
static cl_command_queue g_queue     = NULL;
static cl_program       g_program   = NULL;
static cl_kernel        g_kernel    = NULL;
static cl_kernel        g_kernel_nl = NULL;  /* btc_count_leading */
static cl_device_id     g_device    = 0;
static int              g_initialized = 0;

/* ── Buffers GPU ─────────────────────────────────────────────────── */
static cl_mem g_buf_midstate    = NULL;
static cl_mem g_buf_tail        = NULL;
static cl_mem g_buf_result_n    = NULL;
static cl_mem g_buf_result_h    = NULL;
static cl_mem g_buf_leading     = NULL;
static size_t g_batch_size      = BTC_OCL_BATCH_DEFAULT;

/* ── Chemin kernel (relatif à BTC_DIR) ──────────────────────────── */
#define KERNEL_PATH "src/btc_sha256.cl"

/* ── Lecture fichier .cl ─────────────────────────────────────────── */
static char* _load_kernel_source(const char* path, size_t* len_out) {
    FILE* f = fopen(path, "rb");
    if (!f) {
        /* Essai depuis REPO_ROOT/src/advanced_calculations/bitcoin_quantum_mining/ */
        char alt[512];
        const char* repo = getenv("REPO_ROOT");
        if (!repo) repo = ".";
        snprintf(alt, sizeof(alt),
            "%s/src/advanced_calculations/bitcoin_quantum_mining/src/btc_sha256.cl", repo);
        f = fopen(alt, "rb");
        if (!f) {
            fprintf(stderr, "[OCL] ERREUR : kernel non trouvé : %s\n", path);
            return NULL;
        }
    }
    fseek(f, 0, SEEK_END);
    size_t sz = (size_t)ftell(f);
    rewind(f);
    char* src = (char*)malloc(sz + 1);
    if (!src) { fclose(f); return NULL; }
    if (fread(src, 1, sz, f) != sz) { free(src); fclose(f); return NULL; }
    src[sz] = '\0';
    fclose(f);
    if (len_out) *len_out = sz;
    return src;
}

/* ── Initialisation OpenCL ──────────────────────────────────────── */
int btc_opencl_init(size_t batch_size) {
    /* C174-BIT-LEVEL : Logging forensique COMPLET du début à la fin */
    fprintf(stderr, "\n[C174-BIT-LEVEL] ========================================\n");
    fprintf(stderr, "[C174-BIT-LEVEL] btc_opencl_init() START\n");
    fprintf(stderr, "[C174-BIT-LEVEL] batch_size=%zu | g_initialized=%d\n", batch_size, g_initialized);
    fflush(stderr);
    
    if (g_initialized) {
        fprintf(stderr, "[C174-BIT-LEVEL] Already initialized, returning BTC_OCL_OK\n");
        fprintf(stderr, "[C174-BIT-LEVEL] ========================================\n\n");
        fflush(stderr);
        return BTC_OCL_OK;
    }

    g_batch_size = (batch_size > 0) ? batch_size : BTC_OCL_BATCH_DEFAULT;
    fprintf(stderr, "[C174-BIT-LEVEL] g_batch_size set to %zu\n", g_batch_size);
    fflush(stderr);

    cl_uint num_platforms = 0;
    cl_platform_id platforms[8];
    fprintf(stderr, "[C174-BIT-LEVEL] Calling clGetPlatformIDs...\n");
    fflush(stderr);
    cl_int err = clGetPlatformIDs(8, platforms, &num_platforms);
    fprintf(stderr, "[C174-BIT-LEVEL] clGetPlatformIDs returned: err=%d, num_platforms=%u\n", err, num_platforms);
    fflush(stderr);
    if (err != CL_SUCCESS || num_platforms == 0) {
        fprintf(stderr, "[OCL] Aucune plateforme OpenCL détectée (err=%d)\n", err);
        fprintf(stderr, "[C174-BIT-LEVEL] FAILED: No OpenCL platforms\n");
        fprintf(stderr, "[C174-BIT-LEVEL] ========================================\n\n");
        fflush(stderr);
        return BTC_OCL_ERR_NO_DEVICE;
    }

    /* Chercher un GPU Intel (préféré) ou tout GPU disponible */
    fprintf(stderr, "[C174-BIT-LEVEL] Searching for GPU devices...\n");
    fflush(stderr);
    cl_device_id best_device = 0;
    int found_gpu = 0;
    for (cl_uint p = 0; p < num_platforms && !found_gpu; p++) {
        char pname[256] = {0};
        clGetPlatformInfo(platforms[p], CL_PLATFORM_NAME, sizeof(pname), pname, NULL);
        fprintf(stderr, "[C174-BIT-LEVEL] Platform %u: %s\n", p, pname);
        fflush(stderr);

        cl_uint num_devs = 0;
        cl_device_id devs[8];
        err = clGetDeviceIDs(platforms[p], CL_DEVICE_TYPE_GPU, 8, devs, &num_devs);
        fprintf(stderr, "[C174-BIT-LEVEL] clGetDeviceIDs(GPU) returned: err=%d, num_devs=%u\n", err, num_devs);
        fflush(stderr);
        if (err != CL_SUCCESS || num_devs == 0) continue;

        for (cl_uint d = 0; d < num_devs; d++) {
            char dname[256] = {0};
            clGetDeviceInfo(devs[d], CL_DEVICE_NAME, sizeof(dname), dname, NULL);
            fprintf(stderr, "[OCL] GPU trouvé : %s (plateforme: %s)\n", dname, pname);
            fprintf(stderr, "[C174-BIT-LEVEL] Device %u: %s\n", d, dname);
            fflush(stderr);

            /* Préférer Intel */
            if (strstr(dname, "Intel") || strstr(dname, "HD") || strstr(pname, "Intel")) {
                best_device = devs[d];
                found_gpu = 1;
                fprintf(stderr, "[OCL] → Sélectionné : %s\n", dname);
                fprintf(stderr, "[C174-BIT-LEVEL] Selected Intel GPU: %s\n", dname);
                fflush(stderr);
                break;
            }
            if (!best_device) best_device = devs[d];
        }
    }

    if (!best_device) {
        /* Fallback : CPU OpenCL */
        for (cl_uint p = 0; p < num_platforms; p++) {
            cl_uint num_devs = 0;
            cl_device_id devs[4];
            err = clGetDeviceIDs(platforms[p], CL_DEVICE_TYPE_CPU, 4, devs, &num_devs);
            if (err == CL_SUCCESS && num_devs > 0) {
                best_device = devs[0];
                char dname[256] = {0};
                clGetDeviceInfo(devs[0], CL_DEVICE_NAME, sizeof(dname), dname, NULL);
                fprintf(stderr, "[OCL] Fallback CPU OpenCL : %s\n", dname);
                break;
            }
        }
    }

    if (!best_device) {
        fprintf(stderr, "[OCL] Aucun device OpenCL disponible\n");
        return BTC_OCL_ERR_NO_DEVICE;
    }
    g_device = best_device;
    fprintf(stderr, "[C174-BIT-LEVEL] g_device set successfully\n");
    fflush(stderr);

    /* Infos device */
    fprintf(stderr, "[C174-BIT-LEVEL] Querying device capabilities...\n");
    fflush(stderr);
    size_t max_wg = 0, max_items[3] = {0,0,0};
    cl_ulong gmem = 0;
    clGetDeviceInfo(g_device, CL_DEVICE_MAX_WORK_GROUP_SIZE,   sizeof(max_wg),    &max_wg,    NULL);
    clGetDeviceInfo(g_device, CL_DEVICE_MAX_WORK_ITEM_SIZES,   sizeof(max_items), max_items,  NULL);
    clGetDeviceInfo(g_device, CL_DEVICE_GLOBAL_MEM_SIZE,       sizeof(gmem),      &gmem,      NULL);
    fprintf(stderr, "[OCL] max_wg=%zu max_items=%zu gmem=%llu MB batch=%zu\n",
            max_wg, max_items[0], (unsigned long long)(gmem / 1024 / 1024), g_batch_size);
    fprintf(stderr, "[C174-BIT-LEVEL] Device capabilities: max_wg=%zu, gmem=%llu MB\n",
            max_wg, (unsigned long long)(gmem / 1024 / 1024));
    fflush(stderr);

    /* Contexte + queue */
    fprintf(stderr, "[C174-BIT-LEVEL] Creating OpenCL context...\n");
    fflush(stderr);
    g_ctx = clCreateContext(NULL, 1, &g_device, NULL, NULL, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[OCL] clCreateContext err=%d\n", err);
        fprintf(stderr, "[C174-BIT-LEVEL] FAILED: clCreateContext err=%d\n", err);
        fflush(stderr);
        return BTC_OCL_ERR_BUILD;
    }
    fprintf(stderr, "[C174-BIT-LEVEL] Context created successfully\n");
    fflush(stderr);

    fprintf(stderr, "[C174-BIT-LEVEL] Creating command queue...\n");
    fflush(stderr);
    g_queue = clCreateCommandQueue(g_ctx, g_device, 0, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[OCL] clCreateCommandQueue err=%d\n", err);
        fprintf(stderr, "[C174-BIT-LEVEL] FAILED: clCreateCommandQueue err=%d\n", err);
        fflush(stderr);
        return BTC_OCL_ERR_BUILD;
    }
    fprintf(stderr, "[C174-BIT-LEVEL] Command queue created successfully\n");
    fflush(stderr);

    /* Compilation du kernel */
    fprintf(stderr, "[C174-BIT-LEVEL] Loading kernel source from %s...\n", KERNEL_PATH);
    fflush(stderr);
    size_t src_len = 0;
    char* src = _load_kernel_source(KERNEL_PATH, &src_len);
    if (!src) {
        fprintf(stderr, "[C174-BIT-LEVEL] FAILED: Could not load kernel source\n");
        fflush(stderr);
        return BTC_OCL_ERR_BUILD;
    }
    fprintf(stderr, "[C174-BIT-LEVEL] Kernel source loaded: %zu bytes\n", src_len);
    fflush(stderr);

    fprintf(stderr, "[C174-BIT-LEVEL] Creating program from source...\n");
    fflush(stderr);
    g_program = clCreateProgramWithSource(g_ctx, 1, (const char**)&src, &src_len, &err);
    free(src);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[OCL] clCreateProgram err=%d\n", err);
        fprintf(stderr, "[C174-BIT-LEVEL] FAILED: clCreateProgram err=%d\n", err);
        fflush(stderr);
        return BTC_OCL_ERR_BUILD;
    }
    fprintf(stderr, "[C174-BIT-LEVEL] Program created successfully\n");
    fflush(stderr);

    /* Options : désactiver les extensions non supportées par UHD 620 */
    fprintf(stderr, "[C174-BIT-LEVEL] Building program (this may take 30-60s for JIT compilation)...\n");
    fflush(stderr);
    struct timespec build_start, build_end;
    clock_gettime(CLOCK_MONOTONIC, &build_start);
    err = clBuildProgram(g_program, 1, &g_device,
                         "-cl-fast-relaxed-math -cl-mad-enable", NULL, NULL);
    clock_gettime(CLOCK_MONOTONIC, &build_end);
    double build_time = (build_end.tv_sec - build_start.tv_sec) +
                        (build_end.tv_nsec - build_start.tv_nsec) * 1e-9;
    fprintf(stderr, "[C174-BIT-LEVEL] clBuildProgram completed in %.2f seconds (err=%d)\n", build_time, err);
    fflush(stderr);
    if (err != CL_SUCCESS) {
        char build_log[4096] = {0};
        clGetProgramBuildInfo(g_program, g_device, CL_PROGRAM_BUILD_LOG,
                              sizeof(build_log), build_log, NULL);
        fprintf(stderr, "[OCL] Build ERREUR (err=%d):\n%s\n", err, build_log);
        fprintf(stderr, "[C174-BIT-LEVEL] FAILED: Build error\n%s\n", build_log);
        fflush(stderr);
        return BTC_OCL_ERR_BUILD;
    }
    fprintf(stderr, "[C174-BIT-LEVEL] Program built successfully\n");
    fflush(stderr);

    fprintf(stderr, "[C174-BIT-LEVEL] Creating kernels...\n");
    fflush(stderr);
    g_kernel    = clCreateKernel(g_program, "btc_double_hash",    &err);
    g_kernel_nl = clCreateKernel(g_program, "btc_count_leading",  &err);
    if (!g_kernel || !g_kernel_nl) {
        fprintf(stderr, "[OCL] clCreateKernel err=%d\n", err);
        fprintf(stderr, "[C174-BIT-LEVEL] FAILED: clCreateKernel err=%d\n", err);
        fflush(stderr);
        return BTC_OCL_ERR_BUILD;
    }
    fprintf(stderr, "[C174-BIT-LEVEL] Kernels created successfully\n");
    fflush(stderr);

    /* Allocation buffers GPU — C210 : Mémoire non-cacheable pour réduire contention PCIe */
    fprintf(stderr, "[C210-OPTIMIZATION] Allocating GPU buffers with CL_MEM_ALLOC_HOST_PTR (non-cacheable)...\n");
    fprintf(stderr, "[C210-OPTIMIZATION] Total buffer size: %zu bytes\n",
            (8 + 4 + 1 + 8) * sizeof(uint32_t) + g_batch_size * sizeof(uint32_t));
    fprintf(stderr, "[C210-OPTIMIZATION] Expected benefit: -83%% outliers, +4.4%% hashrate\n");
    fflush(stderr);
    
    /* CL_MEM_ALLOC_HOST_PTR : Force allocation dans mémoire non-cacheable
     * Bénéfice : Réduit contention PCIe CPU-GPU (outliers SHA-256 80-181µs)
     * Référence : RAPPORT_C209_ANALYSE_FORENSIQUE_MANUELLE_COMPLETE.md §Optimisation #1
     */
    g_buf_midstate = clCreateBuffer(g_ctx,
                                    CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
                                    8 * sizeof(uint32_t), NULL, &err);
    g_buf_tail     = clCreateBuffer(g_ctx,
                                    CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
                                    4 * sizeof(uint32_t), NULL, &err);
    g_buf_result_n = clCreateBuffer(g_ctx,
                                    CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR,
                                    1 * sizeof(uint32_t), NULL, &err);
    g_buf_result_h = clCreateBuffer(g_ctx,
                                    CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR,
                                    8 * sizeof(uint32_t), NULL, &err);
    g_buf_leading  = clCreateBuffer(g_ctx,
                                    CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR,
                                    g_batch_size * sizeof(uint32_t), NULL, &err);
    if (!g_buf_midstate || !g_buf_tail || !g_buf_result_n ||
        !g_buf_result_h || !g_buf_leading) {
        fprintf(stderr, "[OCL] Allocation buffer GPU échouée (err=%d)\n", err);
        fprintf(stderr, "[C174-BIT-LEVEL] FAILED: Buffer allocation err=%d\n", err);
        fflush(stderr);
        return BTC_OCL_ERR_BUILD;
    }
    fprintf(stderr, "[C174-BIT-LEVEL] All GPU buffers allocated successfully\n");
    fflush(stderr);

    g_initialized = 1;
    fprintf(stderr, "[OCL] ✅ OpenCL initialisé — batch=%zu — cible: 50-200 MH/s\n",
            g_batch_size);
    fprintf(stderr, "[C174-BIT-LEVEL] g_initialized set to 1\n");
    fflush(stderr);

    /* ── C116-P1 : GPU JIT warm-up — 1 batch factice pour forcer la compilation JIT ──
     * Pattern P1 identifié logs Ubuntu C112 : hashrate froid 3.66 MH/s → chaud 9.80 MH/s
     * (facteur 2.68×). Le warm-up garantit que le batch RÉEL de benchmark n'est
     * pas perturbé par la latence de compilation JIT du driver Intel NEO. */
    {
        fprintf(stderr, "[C116-WARMUP] Batch factice JIT en cours (élimination cold-start)...\n");
        fprintf(stderr, "[C174-BIT-LEVEL] Starting JIT warm-up batch...\n");
        fflush(stderr);
        uint32_t wm[8] = {
            0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
            0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
        };
        uint32_t wt[4] = { 0x00000000, 0x3b9ac9ff, 0x17a950d2, 0x00000000 };
        uint32_t wn = 0xFFFFFFFFu, wh[8] = {0}, wb = 0;
        struct timespec tw0, tw1;
        clock_gettime(CLOCK_MONOTONIC, &tw0);
        fprintf(stderr, "[C174-BIT-LEVEL] Calling btc_opencl_mine_batch for warm-up...\n");
        fflush(stderr);
        btc_opencl_mine_batch(wm, wt, 0, g_batch_size, 32, &wn, wh, &wb);
        clock_gettime(CLOCK_MONOTONIC, &tw1);
        double tw = (tw1.tv_sec - tw0.tv_sec) + (tw1.tv_nsec - tw0.tv_nsec) * 1e-9;
        double mhs_cold = (tw > 0) ? ((double)g_batch_size / tw / 1e6) : 0.0;
        fprintf(stderr, "[C116-WARMUP] ✅ JIT warm-up terminé : %.2f MH/s froid "
                "(prochain batch = GPU chaud)\n", mhs_cold);
        fprintf(stderr, "[C174-BIT-LEVEL] Warm-up completed in %.2f seconds (%.2f MH/s)\n", tw, mhs_cold);
        fflush(stderr);
    }

    fprintf(stderr, "[C174-BIT-LEVEL] btc_opencl_init() COMPLETE - returning BTC_OCL_OK\n");
    fprintf(stderr, "[C174-BIT-LEVEL] ========================================\n\n");
    fflush(stderr);
    return BTC_OCL_OK;
}

/* ── Mine un batch de nonces sur GPU ─────────────────────────────── */
int btc_opencl_mine_batch(
    const uint32_t* midstate,     /* 8 uint32 midstate */
    const uint32_t* tail,         /* 4 uint32 tail header */
    uint32_t        nonce_start,
    size_t          batch_size,
    uint32_t        target_bits,
    uint32_t*       out_nonce,    /* nonce trouvé, ou 0xFFFFFFFF */
    uint32_t*       out_hash,     /* hash[8] si trouvé */
    uint32_t*       out_best_bits /* meilleur near-miss de ce batch */
) {
    if (!g_initialized) return BTC_OCL_ERR_NO_DEVICE;
    if (batch_size == 0) batch_size = g_batch_size;

    cl_int err;
    *out_nonce = 0xFFFFFFFFu;
    uint32_t sentinel = 0xFFFFFFFFu;

    /* Upload midstate + tail */
    clEnqueueWriteBuffer(g_queue, g_buf_midstate, CL_FALSE, 0,
                         8*sizeof(uint32_t), midstate, 0, NULL, NULL);
    clEnqueueWriteBuffer(g_queue, g_buf_tail,     CL_FALSE, 0,
                         4*sizeof(uint32_t), tail,     0, NULL, NULL);
    clEnqueueWriteBuffer(g_queue, g_buf_result_n, CL_FALSE, 0,
                         sizeof(uint32_t), &sentinel, 0, NULL, NULL);
    clFinish(g_queue);

    /* Arguments kernel btc_double_hash */
    clSetKernelArg(g_kernel, 0, sizeof(cl_mem),  &g_buf_midstate);
    clSetKernelArg(g_kernel, 1, sizeof(cl_mem),  &g_buf_tail);
    clSetKernelArg(g_kernel, 2, sizeof(uint32_t), &nonce_start);
    clSetKernelArg(g_kernel, 3, sizeof(uint32_t), &target_bits);
    clSetKernelArg(g_kernel, 4, sizeof(cl_mem),  &g_buf_result_n);
    clSetKernelArg(g_kernel, 5, sizeof(cl_mem),  &g_buf_result_h);

    /* Lancement NDRange */
    size_t wg_size = 64;
    size_t global_size = ((batch_size + wg_size - 1) / wg_size) * wg_size;
    err = clEnqueueNDRangeKernel(g_queue, g_kernel, 1, NULL,
                                  &global_size, &wg_size, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        /* C172-FIX-OPENCL-LOGGING : Messages détaillés pour chaque erreur OpenCL
         * AVANT C172 : Message générique "NDRange err=%d"
         * APRÈS C172 : Diagnostic précis selon code erreur (CL_INVALID_WORK_GROUP_SIZE, etc.)
         * Impact : Identification immédiate de la cause (batch trop grand, GPU overload, etc.) */
        fprintf(stderr, "[OCL-C172] ❌ clEnqueueNDRangeKernel ÉCHEC : err=%d\n", err);
        switch (err) {
        case -54: /* CL_INVALID_WORK_GROUP_SIZE */
            fprintf(stderr, "[OCL-C172]    └─ CL_INVALID_WORK_GROUP_SIZE (-54)\n");
            fprintf(stderr, "[OCL-C172]    └─ Batch %zu dépasse capacité GPU (max_wg=%d)\n",
                    batch_size, 256);
            fprintf(stderr, "[OCL-C172]    └─ Solution : Réduire batch_size_scale NX48 à ≤1.0\n");
            break;
        case -52: /* CL_INVALID_KERNEL_ARGS */
            fprintf(stderr, "[OCL-C172]    └─ CL_INVALID_KERNEL_ARGS (-52)\n");
            fprintf(stderr, "[OCL-C172]    └─ Arguments kernel invalides ou manquants\n");
            break;
        case -5:  /* CL_OUT_OF_RESOURCES */
            fprintf(stderr, "[OCL-C172]    └─ CL_OUT_OF_RESOURCES (-5)\n");
            fprintf(stderr, "[OCL-C172]    └─ GPU surchargé : mémoire ou compute units épuisés\n");
            break;
        case -6:  /* CL_OUT_OF_HOST_MEMORY */
            fprintf(stderr, "[OCL-C172]    └─ CL_OUT_OF_HOST_MEMORY (-6)\n");
            fprintf(stderr, "[OCL-C172]    └─ RAM système insuffisante pour buffers GPU\n");
            break;
        case -11: /* CL_BUILD_PROGRAM_FAILURE */
            fprintf(stderr, "[OCL-C172]    └─ CL_BUILD_PROGRAM_FAILURE (-11)\n");
            fprintf(stderr, "[OCL-C172]    └─ Kernel non compilé ou erreur JIT\n");
            break;
        default:
            fprintf(stderr, "[OCL-C172]    └─ Code erreur OpenCL inconnu : %d\n", err);
            fprintf(stderr, "[OCL-C172]    └─ Consulter cl.h pour diagnostic\n");
            break;
        }
        fprintf(stderr, "[OCL-C172]    └─ global_size=%zu | wg_size=%zu | batch=%zu\n",
                global_size, wg_size, batch_size);
        return BTC_OCL_ERR_RUNTIME;
    }

    /* Lancer aussi btc_count_leading pour near-miss tracking */
    if (out_best_bits) {
        /* Réallouer si batch_size a changé */
        if (batch_size > g_batch_size) {
            clReleaseMemObject(g_buf_leading);
            g_buf_leading = clCreateBuffer(g_ctx, CL_MEM_WRITE_ONLY,
                                           batch_size*sizeof(uint32_t), NULL, &err);
            g_batch_size = batch_size;
        }
        clSetKernelArg(g_kernel_nl, 0, sizeof(cl_mem),   &g_buf_midstate);
        clSetKernelArg(g_kernel_nl, 1, sizeof(cl_mem),   &g_buf_tail);
        clSetKernelArg(g_kernel_nl, 2, sizeof(uint32_t),  &nonce_start);
        clSetKernelArg(g_kernel_nl, 3, sizeof(cl_mem),   &g_buf_leading);
        clEnqueueNDRangeKernel(g_queue, g_kernel_nl, 1, NULL,
                                &global_size, &wg_size, 0, NULL, NULL);
    }

    /* C171-AC-01-FIX : Timer GPU pour mesurer durée kernel */
    struct timespec ts_gpu_start, ts_gpu_end;
    clock_gettime(CLOCK_MONOTONIC, &ts_gpu_start);
    
    clFinish(g_queue);
    
    clock_gettime(CLOCK_MONOTONIC, &ts_gpu_end);
    uint64_t gpu_batch_elapsed_ns = (ts_gpu_end.tv_sec - ts_gpu_start.tv_sec) * 1000000000ULL
                                   + (ts_gpu_end.tv_nsec - ts_gpu_start.tv_nsec);

    /* Lecture résultat */
    clEnqueueReadBuffer(g_queue, g_buf_result_n, CL_TRUE, 0,
                        sizeof(uint32_t), out_nonce, 0, NULL, NULL);
    if (*out_nonce != 0xFFFFFFFFu && out_hash) {
        clEnqueueReadBuffer(g_queue, g_buf_result_h, CL_TRUE, 0,
                            8*sizeof(uint32_t), out_hash, 0, NULL, NULL);
    }

    /* Calcul best bits du batch */
    if (out_best_bits) {
        uint32_t* leading = (uint32_t*)malloc(batch_size * sizeof(uint32_t));
        if (leading) {
            clEnqueueReadBuffer(g_queue, g_buf_leading, CL_TRUE, 0,
                                batch_size*sizeof(uint32_t), leading, 0, NULL, NULL);
            uint32_t best = 0;
            
            /* C170-GPU-BIT-LEVEL : Log CHAQUE nonce GPU avec leading_zeros
             * AVANT C170 : Aucun log des 262k nonces GPU individuels
             * APRÈS C170 : Chaque nonce GPU loggé avec BTC_FORENSIC_HASH
             * Impact : +262k nonces/batch, ~786k lignes CSV, -5% hashrate GPU
             * Justification : Traçabilité totale GPU demandée par utilisateur */
            /* C171-AC-01-FIX + C171-AC-02-FIX : Timer GPU + Logging asynchrone
             * Calcul elapsed_ns par nonce = durée_batch / batch_size
             * Log via thread dédié (non-bloquant) */
            uint64_t elapsed_per_nonce_ns = gpu_batch_elapsed_ns / batch_size;
            
            for (size_t i = 0; i < batch_size; i++) {
                if (leading[i] > best) best = leading[i];
                
                // Log asynchrone (non-bloquant si buffer non plein)
                uint32_t nonce = nonce_start + (uint32_t)i;
                btc_gpu_async_log_hash(nonce, leading[i], elapsed_per_nonce_ns);
            }
            
            *out_best_bits = best;
            free(leading);
        } else {
            *out_best_bits = 0;
        }
    }

    return BTC_OCL_OK;
}

/* ── Benchmark rapide : estime le hashrate ──────────────────────── */
double btc_opencl_benchmark_mhs(size_t batch_size) {
    if (!g_initialized) return 0.0;

    uint32_t midstate[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };
    uint32_t tail[4]  = { 0x61736400, 0x3b9ac9ff, 0x17a950d2, 0x00000000 };
    uint32_t out_nonce = 0xFFFFFFFFu;
    uint32_t out_hash[8] = {0};
    uint32_t out_bits = 0;

    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    btc_opencl_mine_batch(midstate, tail, 0, batch_size, 32,
                          &out_nonce, out_hash, &out_bits);

    clock_gettime(CLOCK_MONOTONIC, &t1);
    double elapsed = (t1.tv_sec - t0.tv_sec) +
                     (t1.tv_nsec - t0.tv_nsec) * 1e-9;
    double mhs = (elapsed > 0) ? ((double)batch_size / elapsed / 1e6) : 0.0;
    fprintf(stderr, "[OCL] Benchmark : %.2f MH/s (batch=%zu, t=%.3fs)\n",
            mhs, batch_size, elapsed);
    return mhs;
}

/* ── Nettoyage ──────────────────────────────────────────────────── */
void btc_opencl_cleanup(void) {
    if (!g_initialized) return;
    if (g_buf_midstate) clReleaseMemObject(g_buf_midstate);
    if (g_buf_tail)     clReleaseMemObject(g_buf_tail);
    if (g_buf_result_n) clReleaseMemObject(g_buf_result_n);
    if (g_buf_result_h) clReleaseMemObject(g_buf_result_h);
    if (g_buf_leading)  clReleaseMemObject(g_buf_leading);
    if (g_kernel)       clReleaseKernel(g_kernel);
    if (g_kernel_nl)    clReleaseKernel(g_kernel_nl);
    if (g_program)      clReleaseProgram(g_program);
    if (g_queue)        clReleaseCommandQueue(g_queue);
    if (g_ctx)          clReleaseContext(g_ctx);
    g_initialized = 0;
    fprintf(stderr, "[OCL] Ressources libérées\n");
}

#else /* BTC_OPENCL_ENABLED non défini */

int    btc_opencl_init(size_t b)             { (void)b;  return BTC_OCL_ERR_NO_DEVICE; }
int    btc_opencl_mine_batch(const uint32_t* ms, const uint32_t* tl,
    uint32_t ns, size_t bs, uint32_t tb,
    uint32_t* on, uint32_t* oh, uint32_t* ob) {
    (void)ms;(void)tl;(void)ns;(void)bs;(void)tb;(void)on;(void)oh;(void)ob;
    return BTC_OCL_ERR_NO_DEVICE;
}
double btc_opencl_benchmark_mhs(size_t b)    { (void)b;  return 0.0; }
void   btc_opencl_cleanup(void)              {}

#endif
