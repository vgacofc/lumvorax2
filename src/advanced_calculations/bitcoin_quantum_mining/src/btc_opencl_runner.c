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
    if (g_initialized) return BTC_OCL_OK;

    g_batch_size = (batch_size > 0) ? batch_size : BTC_OCL_BATCH_DEFAULT;

    cl_uint num_platforms = 0;
    cl_platform_id platforms[8];
    cl_int err = clGetPlatformIDs(8, platforms, &num_platforms);
    if (err != CL_SUCCESS || num_platforms == 0) {
        fprintf(stderr, "[OCL] Aucune plateforme OpenCL détectée (err=%d)\n", err);
        return BTC_OCL_ERR_NO_DEVICE;
    }

    /* Chercher un GPU Intel (préféré) ou tout GPU disponible */
    cl_device_id best_device = 0;
    int found_gpu = 0;
    for (cl_uint p = 0; p < num_platforms && !found_gpu; p++) {
        char pname[256] = {0};
        clGetPlatformInfo(platforms[p], CL_PLATFORM_NAME, sizeof(pname), pname, NULL);

        cl_uint num_devs = 0;
        cl_device_id devs[8];
        err = clGetDeviceIDs(platforms[p], CL_DEVICE_TYPE_GPU, 8, devs, &num_devs);
        if (err != CL_SUCCESS || num_devs == 0) continue;

        for (cl_uint d = 0; d < num_devs; d++) {
            char dname[256] = {0};
            clGetDeviceInfo(devs[d], CL_DEVICE_NAME, sizeof(dname), dname, NULL);
            fprintf(stderr, "[OCL] GPU trouvé : %s (plateforme: %s)\n", dname, pname);

            /* Préférer Intel */
            if (strstr(dname, "Intel") || strstr(dname, "HD") || strstr(pname, "Intel")) {
                best_device = devs[d];
                found_gpu = 1;
                fprintf(stderr, "[OCL] → Sélectionné : %s\n", dname);
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

    /* Infos device */
    size_t max_wg = 0, max_items[3] = {0,0,0};
    cl_ulong gmem = 0;
    clGetDeviceInfo(g_device, CL_DEVICE_MAX_WORK_GROUP_SIZE,   sizeof(max_wg),    &max_wg,    NULL);
    clGetDeviceInfo(g_device, CL_DEVICE_MAX_WORK_ITEM_SIZES,   sizeof(max_items), max_items,  NULL);
    clGetDeviceInfo(g_device, CL_DEVICE_GLOBAL_MEM_SIZE,       sizeof(gmem),      &gmem,      NULL);
    fprintf(stderr, "[OCL] max_wg=%zu max_items=%zu gmem=%llu MB batch=%zu\n",
            max_wg, max_items[0], (unsigned long long)(gmem / 1024 / 1024), g_batch_size);

    /* Contexte + queue */
    g_ctx = clCreateContext(NULL, 1, &g_device, NULL, NULL, &err);
    if (err != CL_SUCCESS) { fprintf(stderr, "[OCL] clCreateContext err=%d\n", err); return BTC_OCL_ERR_BUILD; }

    g_queue = clCreateCommandQueue(g_ctx, g_device, 0, &err);
    if (err != CL_SUCCESS) { fprintf(stderr, "[OCL] clCreateCommandQueue err=%d\n", err); return BTC_OCL_ERR_BUILD; }

    /* Compilation du kernel */
    size_t src_len = 0;
    char* src = _load_kernel_source(KERNEL_PATH, &src_len);
    if (!src) return BTC_OCL_ERR_BUILD;

    g_program = clCreateProgramWithSource(g_ctx, 1, (const char**)&src, &src_len, &err);
    free(src);
    if (err != CL_SUCCESS) { fprintf(stderr, "[OCL] clCreateProgram err=%d\n", err); return BTC_OCL_ERR_BUILD; }

    /* Options : désactiver les extensions non supportées par UHD 620 */
    err = clBuildProgram(g_program, 1, &g_device,
                         "-cl-fast-relaxed-math -cl-mad-enable", NULL, NULL);
    if (err != CL_SUCCESS) {
        char build_log[4096] = {0};
        clGetProgramBuildInfo(g_program, g_device, CL_PROGRAM_BUILD_LOG,
                              sizeof(build_log), build_log, NULL);
        fprintf(stderr, "[OCL] Build ERREUR (err=%d):\n%s\n", err, build_log);
        return BTC_OCL_ERR_BUILD;
    }

    g_kernel    = clCreateKernel(g_program, "btc_double_hash",    &err);
    g_kernel_nl = clCreateKernel(g_program, "btc_count_leading",  &err);
    if (!g_kernel || !g_kernel_nl) {
        fprintf(stderr, "[OCL] clCreateKernel err=%d\n", err);
        return BTC_OCL_ERR_BUILD;
    }

    /* Allocation buffers GPU */
    g_buf_midstate = clCreateBuffer(g_ctx, CL_MEM_READ_ONLY,  8 * sizeof(uint32_t), NULL, &err);
    g_buf_tail     = clCreateBuffer(g_ctx, CL_MEM_READ_ONLY,  4 * sizeof(uint32_t), NULL, &err);
    g_buf_result_n = clCreateBuffer(g_ctx, CL_MEM_READ_WRITE, 1 * sizeof(uint32_t), NULL, &err);
    g_buf_result_h = clCreateBuffer(g_ctx, CL_MEM_WRITE_ONLY, 8 * sizeof(uint32_t), NULL, &err);
    g_buf_leading  = clCreateBuffer(g_ctx, CL_MEM_WRITE_ONLY,
                                    g_batch_size * sizeof(uint32_t), NULL, &err);
    if (!g_buf_midstate || !g_buf_tail || !g_buf_result_n ||
        !g_buf_result_h || !g_buf_leading) {
        fprintf(stderr, "[OCL] Allocation buffer GPU échouée (err=%d)\n", err);
        return BTC_OCL_ERR_BUILD;
    }

    g_initialized = 1;
    fprintf(stderr, "[OCL] ✅ OpenCL initialisé — batch=%zu — cible: 50-200 MH/s\n",
            g_batch_size);
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
        fprintf(stderr, "[OCL] NDRange err=%d\n", err);
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

    clFinish(g_queue);

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
            for (size_t i = 0; i < batch_size; i++)
                if (leading[i] > best) best = leading[i];
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
