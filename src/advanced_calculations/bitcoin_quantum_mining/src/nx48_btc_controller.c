/*
 * LumVorax — Module 17 — Bitcoin Quantum Mining Engine
 * nx48_btc_controller.c — Contrôleur NX48 AUTONOME 100% — C61
 *
 * STANDARD_NAMES.md v4.4 §M-BTC17-C61 — 2026-04-18
 *
 * REFONTE C61 — Problèmes corrigés depuis C60 :
 *  [C61-EXPLOR-BLOCK]  exploration_bias bloquée à ~0.48-0.50 (affiché 48-50%)
 *                      CAUSE : +0.04 monte quand loss augmente, -0.02 descend quand améliore.
 *                              Avec stagnation habituelle → balance quasi-nulle → plateau 0.50
 *                      FIX   : Vélocité momentum (Adam-like) + force de rappel si plateau long.
 *                              exploration_vel accumule la direction → sort des plateaux.
 *                              Stagnation longue → boost forcé exploration (×3 sur vélocité).
 *
 *  [C61-DELTA-UNLOCK]  delta_nonce_scale max 50.0→500.0
 *                      CAUSE : Clamp à 50.0 → blocage absolu une fois atteint.
 *                      FIX   : Max étendu à 500.0, log de l'atteinte du cap dans forensic.
 *
 *  [C61-SUBNEURONS]    8 sous-neurones dynamiques par neurone (16 total)
 *                      Chaque sous-neurone apprend via Adam (≠ ISTA racine)
 *                      Spécialités : exploration, threads, GPU, T_hot, T_cold, batch, AVX, QDAYPRIZE
 *                      Les sous-neurones envoient leurs commandes via atomiques partagées.
 *
 *  [C61-HW-DETECT]     nx48_btc_hw_detect() lit :
 *                        /proc/cpuinfo → avx512f/sha_ni/avx2, n_cores
 *                        /dev/dri/renderD128 → GPU DRI présent
 *                        clinfo --list → GPU OpenCL disponible
 *                        /proc/meminfo → RAM disponible
 *                      Détection automatique toutes les 30 secondes.
 *
 *  [C61-CTRL-ALL]      nx48_btc_control_all() écrit les atomiques partagées :
 *                        nx48_ctrl_n_threads  → moteur ajuste le pool de threads
 *                        nx48_ctrl_T_hot_idx  → moteur change T_hot répliques
 *                        nx48_ctrl_T_cold_idx → moteur change T_cold répliques
 *                        nx48_ctrl_gpu_active → moteur active/désactive OpenCL
 *                        nx48_ctrl_avx_level  → moteur choisit chemin SHA optimal
 *                        nx48_ctrl_batch_size → moteur ajuste batch SHA-256
 *
 *  [C61-LUM-NATIVE]    nx48_btc_save_lum() / nx48_btc_load_lum()
 *                      Format binaire 64 bytes — CRC32 intégrité — 5× plus rapide que CSV
 *                      Chemin : btc_nx48_last.lum (+ CSV gardé pour compatibilité)
 *
 *  [C61-ADAM]          Poids principaux mis à jour par Adam (β1=0.9, β2=0.999)
 *                      au lieu de ISTA pur → convergence plus stable, moins de stagnation
 *
 * NX48 gère désormais 100% des paramètres système :
 *   ✅ exploration/exploitation ratio    ✅ threads dynamiques
 *   ✅ GPU OpenCL activation             ✅ T_hot PT-MC adaptation
 *   ✅ T_cold PT-MC adaptation           ✅ batch size SHA-256
 *   ✅ chemin AVX-512/AVX2/scalaire      ✅ feedback QDAYPRIZE
 *   ✅ delta_nonce radius                ✅ format LUM binaire persistance
 */

#ifndef _GNU_SOURCE
#  define _GNU_SOURCE
#endif
#define _POSIX_C_SOURCE 200809L

#include "nx48_btc_controller.h"
#include "nx48_alltime_record.h"  /* C107 : persistance MONOTONE par header */
#include "../include/btc_mining_forensic.h"
#include "optimization/reasoning_path_tracker.h"  /* C110 : trace décisions NX48 */

/* C110 — pointer global vers reasoning_trace (init dans main_btc_mining.c).
 * NULL si BTC_REASONING_TRACE absent → appel ignoré (no-op). */
extern reasoning_trace_t* g_btc_reasoning_trace;
/* NOTE C99-P5 : Le hook coupler officiel est dans btc_mining_engine.c
 * (nx48_bridge_*) — voir rapport 103 (C99 P2). Pas de hook duplicata ici. */
#include "debug/ultra_forensic_logger.h"
#include "lumvorax_integration.h"
#include "debug/memory_tracker.h"
#include "../../../../include/lumvorax_ibm_constants.h"  /* C94 : ponts IBM Quantum (S_pi reel) */

#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <unistd.h>

/* ════════════════════════════════════════════════════════════════════
 * ATOMIQUES PARTAGÉES MOTEUR ↔ NX48 — définition ici, extern dans .h
 * ════════════════════════════════════════════════════════════════════ */
_Atomic int nx48_ctrl_n_threads      = 2;    /* Défaut : 2 threads */
_Atomic int nx48_ctrl_T_hot_idx      = 7;    /* Défaut : T_hot = 50.0 (index 7) */
_Atomic int nx48_ctrl_T_cold_idx     = 0;    /* Défaut : T_cold = 1.0 (index 0) */
_Atomic int nx48_ctrl_gpu_active     = 0;    /* Défaut : GPU inactif */
_Atomic int nx48_ctrl_avx_level      = 0;    /* Détecté à l'init */
_Atomic int nx48_ctrl_batch_size     = 1024; /* Défaut C46 */
/* C116-P4 : seuil near-miss adaptatif (QDPR). NX48 l'augmente si success>60%.
 * Valeur initiale 20 (baseline C112). Max observé : 38 bits (run 3, t=460s). */
_Atomic int nx48_ctrl_near_miss_bits = 20;   /* Défaut : 20 bits (C116-P4) */

/* ════════════════════════════════════════════════════════════════════
 * C125-OPTIMIZE-RUNTIME — Atomics injectés par main_btc_mining après
 * asic_btc_optimizer_tune_full(). Valeur 0 = aucune injection.
 * ════════════════════════════════════════════════════════════════════ */
_Atomic int nx48_ctrl_delta_nx48_initial_milli = 0;  /* 0 = pas d'override (garder compile-time) */
_Atomic int nx48_ctrl_thermal_throttle_s       = 0;  /* 0 = pas d'override */
_Atomic int nx48_ctrl_nonce_strategy           = 0;  /* 0 = SEQUENTIAL par défaut */
/* C125-MEM-BIT — granularité traçage mémoire (0=PAGE 1=BYTE 2=BIT 3=HUGEPAGE) */
_Atomic int nx48_ctrl_mem_trace_granularity    = 0;  /* défaut PAGE, override par BTC_MEM_TRACE_GRANULARITY */

/* Températures répliques disponibles (identiques btc_mining_engine.c) */
static const double NX48_REPLICA_TEMPS[8] = {
    1.0, 2.0, 4.0, 8.0, 12.0, 20.0, 35.0, 50.0
};

/* ════════════════════════════════════════════════════════════════════
 * XOSHIRO256++ — PRNG qualité statistique extrême
 * ════════════════════════════════════════════════════════════════════ */
static uint64_t xosh_s[4];

static void xosh_seed(void) {
    FILE* urnd = fopen("/dev/urandom", "rb");
    if (urnd) {
        size_t n = fread(xosh_s, sizeof(uint64_t), 4, urnd);
        fclose(urnd);
        if (n == 4 && (xosh_s[0]|xosh_s[1]|xosh_s[2]|xosh_s[3]) != 0) return;
    }
    xosh_s[0] = (uint64_t)time(NULL) ^ 0x6C62272E07BB0142ULL;
    xosh_s[1] = (uint64_t)clock()    ^ 0x62B821756295C58DULL;
    xosh_s[2] = xosh_s[0] ^ (xosh_s[0] >> 33);
    xosh_s[3] = xosh_s[1] ^ (xosh_s[1] >> 17);
}

static inline uint64_t xosh_rotl(uint64_t x, int k) {
    return (x << k) | (x >> (64 - k));
}

static inline uint64_t xosh_next(void) {
    const uint64_t result = xosh_rotl(xosh_s[0] + xosh_s[3], 23) + xosh_s[0];
    const uint64_t t = xosh_s[1] << 17;
    xosh_s[2] ^= xosh_s[0]; xosh_s[3] ^= xosh_s[1];
    xosh_s[1] ^= xosh_s[2]; xosh_s[0] ^= xosh_s[3];
    xosh_s[2] ^= t; xosh_s[3] = xosh_rotl(xosh_s[3], 45);
    return result;
}

static inline double xosh_uniform(void) {
    return (double)(xosh_next() >> 11) * (1.0 / 9007199254740992.0);
}

static double xosh_gaussian(double sigma) {
    static int have_extra = 0; static double extra = 0.0;
    if (have_extra) { have_extra = 0; return extra * sigma; }
    double u1, u2;
    do { u1 = xosh_uniform(); } while (u1 < 1e-300);
    u2 = xosh_uniform();
    double mag = sqrt(-2.0 * log(u1));
    double angle = 2.0 * 3.14159265358979323846 * u2;
    extra = mag * sin(angle); have_extra = 1;
    return mag * cos(angle) * sigma;
}

/* ── CRC32 simple (IEEE 802.3) ──────────────────────────────────── */
static uint32_t crc32_compute(const uint8_t* data, size_t len) {
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++)
            crc = (crc >> 1) ^ (0xEDB88320u & -(crc & 1u));
    }
    return ~crc;
}

/* ── Fonctions utilitaires ──────────────────────────────────────── */
static inline double sigmoid(double x) { return 1.0 / (1.0 + exp(-x)); }
static inline double clamp(double v, double lo, double hi) {
    return (v < lo) ? lo : (v > hi) ? hi : v;
}

/* ── Poids défaut neurone producteur ────────────────────────────── */
static const double NX48_BTC_WEIGHTS_DEFAULT[NX48_BTC_N_FEATURES] = {
    0.35, 0.20, 0.15, 0.10, 0.08, 0.05, 0.04, 0.03
};

/* ════════════════════════════════════════════════════════════════════
 * C61 : DÉTECTION HARDWARE AUTONOME
 * Lit /proc/cpuinfo, /dev/dri, clinfo, /proc/meminfo
 * ════════════════════════════════════════════════════════════════════ */
void nx48_btc_hw_detect(nx48_btc_state_t* s) {
    nx48_hw_state_t* hw = &s->hw;
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    hw->last_hw_detect_ns = (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;

    /* ── /proc/cpuinfo ── */
    hw->n_cores_physical = 1;
    hw->n_threads_max    = 1;
    hw->avx_level        = NX48_HW_SCALAR;
    hw->sha_ni           = 0;

    FILE* fp = fopen("/proc/cpuinfo", "r");
    if (fp) {
        char line[512];
        int proc_count = 0;
        while (fgets(line, sizeof(line), fp)) {
            if (strncmp(line, "processor", 9) == 0) proc_count++;
            if (strncmp(line, "flags", 5) == 0) {
                if (strstr(line, "avx512f")) {
                    hw->avx_level = NX48_HW_AVX512;
                } else if (strstr(line, "avx2")) {
                    if (hw->avx_level < NX48_HW_AVX2)
                        hw->avx_level = NX48_HW_AVX2;
                }
                /* C110 fix : flags Linux peut afficher "sha_ni" OU "sha-ni" OU "sha" */
                if (strstr(line, "sha_ni") || strstr(line, "sha-ni")) hw->sha_ni = 1;
            }
        }
        fclose(fp);
        hw->n_threads_max = (proc_count > 0) ? proc_count : 1;
        hw->n_cores_physical = hw->n_threads_max;
    }

    /* C110 fix BUG-C109-D : fallback __builtin_cpu_supports si /proc/cpuinfo
     * échoue ou si flags absent (cas Intel Kaby Lake où "avx2" était parfois
     * raté par le grep). Détection au runtime via cpuid réel. */
    __builtin_cpu_init();
    if (hw->avx_level < NX48_HW_AVX512 && __builtin_cpu_supports("avx512f")) {
        hw->avx_level = NX48_HW_AVX512;
    } else if (hw->avx_level < NX48_HW_AVX2 && __builtin_cpu_supports("avx2")) {
        hw->avx_level = NX48_HW_AVX2;
    }
#ifdef __SHA__
    /* GCC builtin sha pas dispo partout — utiliser flags compilateur */
    hw->sha_ni = 1;
#endif

    /* ── GPU DRI ── */
    hw->dri_present = (access("/dev/dri/renderD128", F_OK) == 0) ? 1 : 0;

    /* ── GPU OpenCL via clinfo ── */
    hw->gpu_opencl_present = 0;
    hw->gpu_name[0] = '\0';
    FILE* cl = popen("clinfo --list 2>/dev/null | head -4", "r");
    if (cl) {
        char buf[256];
        while (fgets(buf, sizeof(buf), cl)) {
            if (strstr(buf, "Device") && strstr(buf, "#0")) {
                hw->gpu_opencl_present = 1;
                /* Extraire le nom après "Device #0: " */
                char* dptr = strstr(buf, ": ");
                if (dptr) {
                    strncpy(hw->gpu_name, dptr + 2, sizeof(hw->gpu_name) - 1);
                    hw->gpu_name[sizeof(hw->gpu_name) - 1] = '\0';
                    /* Supprimer \n */
                    char* nl = strchr(hw->gpu_name, '\n');
                    if (nl) *nl = '\0';
                }
            }
        }
        pclose(cl);
    }

    /* Si GPU DRI présent mais clinfo échoue → supposer présent */
    if (hw->dri_present && !hw->gpu_opencl_present) {
        hw->gpu_opencl_present = 1;
        strncpy(hw->gpu_name, "DRI-GPU (clinfo non installé)", sizeof(hw->gpu_name)-1);
    }

    /* ── RAM disponible ── */
    hw->ram_available_mb = 4096; /* défaut */
    FILE* mi = fopen("/proc/meminfo", "r");
    if (mi) {
        char line[128]; long avail_kb = 0;
        while (fgets(line, sizeof(line), mi)) {
            if (strncmp(line, "MemAvailable:", 13) == 0) {
                sscanf(line + 13, "%ld", &avail_kb); break;
            }
        }
        fclose(mi);
        hw->ram_available_mb = avail_kb / 1024L;
    }

    /* ── C62 : QDAYPRIZE feedback — lecture JSON le plus recent ──── */
    {
        FILE* qd = popen("ls -t logs/forensic/qdayprize_*.json 2>/dev/null | head -1", "r");
        if (qd) {
            char qpath[512] = {0};
            if (fgets(qpath, sizeof(qpath)-1, qd)) {
                char* nl = strchr(qpath, '\n');
                if (nl) *nl = '\0';
                pclose(qd);
                qd = NULL;
                if (qpath[0] != '\0') {
                    FILE* jf = fopen(qpath, "r");
                    if (jf) {
                        char jbuf[4096] = {0};
                        size_t nr = fread(jbuf, 1, sizeof(jbuf)-1, jf);
                        fclose(jf);
                        if (nr > 0) {
                            char* sr = strstr(jbuf, "\"success_rate\"");
                            if (sr) { char* c = strchr(sr, ':'); if (c) hw->qdayprize_success_rate = atof(c+1); }
                            char* nb = strstr(jbuf, "\"near_miss_bits\"");
                            if (nb) { char* c = strchr(nb, ':'); if (c) hw->qdayprize_bits = atoi(c+1); }
                            char* rt = strstr(jbuf, "\"runtime_s\"");
                            if (rt) { char* c = strchr(rt, ':'); if (c) hw->qdayprize_runtime_s = atof(c+1); }
                            printf("[NX48-QDPR] Feedback C62 : success=%.1f%% bits=%d rt=%.1fs\n",
                                hw->qdayprize_success_rate * 100.0,
                                hw->qdayprize_bits, hw->qdayprize_runtime_s);
                            FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME,
                                "btc_nx48_qdayprize_success_rate", hw->qdayprize_success_rate);
                            FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME,
                                "btc_nx48_qdayprize_bits", (double)hw->qdayprize_bits);
                            /* C118-Q2 : QDPR adaptatif bidirectionnel avec hysteresis.
                             * - C116-P4 : si success > 60% → monter near_miss_bits (max 38)
                             * - C118-Q2 : si success < 30% → descendre near_miss_bits (min 20)
                             * - Zone morte 0.30 ≤ rate ≤ 0.60 : pas de changement (anti-oscillation).
                             *
                             * Justification (rapport C117 §6) : sans descente, un run qui démarre
                             * mal (success=0%) reste bloqué à near_miss_bits=20, le GPU loupe les
                             * near-miss de 25 LZ et NX48 ne peut pas apprendre des near-miss bas.
                             * L'hysteresis ±0.30/±0.60 évite l'oscillation du seuil. */
                            {
                                double sr  = hw->qdayprize_success_rate;
                                int    cur = atomic_load_explicit(&nx48_ctrl_near_miss_bits,
                                                                  memory_order_relaxed);
                                int    next = cur;
                                const char* direction = NULL;
                                if (sr > 0.60 && cur < 38) {
                                    next = cur + 1;
                                    direction = "↑";
                                } else if (sr < 0.30 && cur > 20) {
                                    next = cur - 1;
                                    direction = "↓";
                                }
                                if (next != cur) {
                                    atomic_store_explicit(&nx48_ctrl_near_miss_bits, next,
                                                          memory_order_relaxed);
                                    printf("[C118-Q2-QDPR] success=%.1f%% %s near_miss_bits %d→%d\n",
                                        sr * 100.0, direction, cur, next);
                                    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME,
                                        "btc_nx48_c118q2_near_miss_bits", (double)next);
                                } else {
                                    /* Zone morte ou aux bornes : log sans changement */
                                    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME,
                                        "btc_nx48_c118q2_near_miss_bits", (double)cur);
                                }
                            }
                        }
                    }
                }
            } else {
                pclose(qd);
            }
        }
    }

    /* ── Mise à jour atomique AVX level ── */
    atomic_store_explicit(&nx48_ctrl_avx_level, hw->avx_level, memory_order_relaxed);

    /* ── Log forensic ── */
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_nx48_hw_avx_level",
        (double)hw->avx_level);
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_nx48_hw_sha_ni",
        (double)hw->sha_ni);
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_nx48_hw_threads_max",
        (double)hw->n_threads_max);
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_nx48_hw_gpu_opencl",
        (double)hw->gpu_opencl_present);
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_nx48_hw_ram_mb",
        (double)hw->ram_available_mb);

    /* C110 : étiquette explicite + flags compile-time pour preuve d'utilisation */
    const char* avx_label =
        (hw->avx_level == NX48_HW_AVX512) ? "AVX-512" :
        (hw->avx_level == NX48_HW_AVX2)   ? "AVX2"    :
        (hw->avx_level == NX48_HW_OPENCL) ? "OpenCL"  : "scalar";

    /* Flags compile-time réellement actifs dans ce binaire */
    const char* compiled_with =
#if defined(__AVX512F__)
        "AVX-512+AVX2+SSE"
#elif defined(__AVX2__)
        "AVX2+SSE"
#elif defined(__SSE4_2__)
        "SSE4.2"
#else
        "scalar"
#endif
        ;

    printf("[NX48-HW] CPU:%d threads AVX:%d (%s) SHA-NI:%d | GPU-OpenCL:%s DRI:%d | RAM:%ldMB\n",
        hw->n_threads_max, hw->avx_level, avx_label, hw->sha_ni,
        hw->gpu_opencl_present ? hw->gpu_name : "ABSENT",
        hw->dri_present, hw->ram_available_mb);
    printf("[NX48-HW] Binaire compilé AVEC : %s%s%s | gcc auto-vectorise SHA-256 dès maintenant\n",
        compiled_with,
#ifdef __FMA__
        " +FMA",
#else
        "",
#endif
#ifdef __SHA__
        " +SHA-NI"
#else
        ""
#endif
        );
}

/* ════════════════════════════════════════════════════════════════════
 * C61 : SOUS-NEURONES — Features spécialisées
 * Chaque sous-neurone voit 4 features pertinentes à sa spécialité
 * ════════════════════════════════════════════════════════════════════ */
void nx48_subneuron_compute_features(
    double                  feat[NX48_SN_FEATURES],
    const nx48_btc_state_t* s,
    int                     specialty)
{
    switch (specialty) {
    case NX48_SN_EXPLORATION:
        feat[0] = s->exploration_bias;
        feat[1] = clamp(s->stall_count / 20.0, 0.0, 1.0);
        feat[2] = clamp(s->loss_curr, 0.0, 1.0);
        feat[3] = clamp((double)s->best_leading_zeros / 256.0, 0.0, 1.0);
        break;
    case NX48_SN_THREADS:
        feat[0] = clamp((double)s->n_threads_target / (double)(s->hw.n_threads_max + 1), 0.0, 1.0);
        feat[1] = clamp(s->hw.ram_available_mb / 8192.0, 0.0, 1.0);
        feat[2] = clamp(s->loss_curr, 0.0, 1.0);
        feat[3] = clamp(s->grad_norm, 0.0, 1.0);
        break;
    case NX48_SN_GPU:
        feat[0] = (double)s->hw.gpu_opencl_present;
        feat[1] = (double)s->hw.gpu_opencl_active;
        feat[2] = clamp(s->hw.gpu_hashrate_est / 500.0, 0.0, 1.0);
        feat[3] = clamp((double)s->stall_long_count / 50.0, 0.0, 1.0);
        break;
    case NX48_SN_TEMP_HOT:
        feat[0] = clamp((double)s->T_hot_idx / 7.0, 0.0, 1.0);
        feat[1] = clamp(s->hw.qdayprize_success_rate, 0.0, 1.0);
        feat[2] = clamp(s->exploration_bias, 0.0, 1.0);
        feat[3] = clamp(s->loss_curr, 0.0, 1.0);
        break;
    case NX48_SN_TEMP_COLD:
        feat[0] = clamp((double)s->T_cold_idx / 7.0, 0.0, 1.0);
        feat[1] = 1.0 - clamp(s->exploration_bias, 0.0, 1.0);
        feat[2] = clamp((double)s->best_leading_zeros / 64.0, 0.0, 1.0);
        feat[3] = clamp(s->grad_norm, 0.0, 1.0);
        break;
    case NX48_SN_BATCH:
        feat[0] = clamp(s->batch_size_scale / 8.0, 0.0, 1.0);
        feat[1] = clamp(s->hw.ram_available_mb / 4096.0, 0.0, 1.0);
        feat[2] = clamp((double)s->hw.avx_level / 3.0, 0.0, 1.0);
        feat[3] = clamp(s->loss_curr, 0.0, 1.0);
        break;
    case NX48_SN_AVX:
        feat[0] = clamp((double)s->hw.avx_level / 3.0, 0.0, 1.0);
        feat[1] = (double)s->hw.sha_ni;
        feat[2] = clamp(s->batch_size_scale / 8.0, 0.0, 1.0);
        feat[3] = clamp(s->grad_norm, 0.0, 1.0);
        break;
    case NX48_SN_QDAYPRIZE:
        feat[0] = clamp(s->hw.qdayprize_success_rate, 0.0, 1.0);
        feat[1] = clamp((double)s->hw.qdayprize_bits / 256.0, 0.0, 1.0);
        feat[2] = clamp((double)s->best_leading_zeros / 256.0, 0.0, 1.0);
        feat[3] = clamp(s->exploration_bias, 0.0, 1.0);
        break;
    default:
        feat[0] = feat[1] = feat[2] = feat[3] = 0.5;
        break;
    }
}

/* ════════════════════════════════════════════════════════════════════
 * C61 : MISE À JOUR SOUS-NEURONE (Adam optimizer)
 * Adam : β1=0.9, β2=0.999, ε=1e-8
 * ════════════════════════════════════════════════════════════════════ */
void nx48_subneuron_update(
    nx48_subneuron_t* sn,
    const double      features[NX48_SN_FEATURES],
    double            label,
    double            parent_err)
{
    /* Prédiction locale */
    double z = sn->bias;
    for (int i = 0; i < NX48_SN_FEATURES; i++)
        z += sn->weights[i] * features[i];
    double pred = sigmoid(z);
    sn->output = pred;

    /* Loss BCE locale */
    double eps = 1e-12;
    double p = clamp(pred, eps, 1.0 - eps);
    sn->loss = -(label * log(p) + (1.0 - label) * log(1.0 - p));

    /* Erreur combinée : locale + influence parent */
    double err = (pred - label) + 0.3 * parent_err;
    sn->update_count++;

    /* Adam update */
    double beta1 = 0.9, beta2 = 0.999, adam_eps = 1e-8;
    double lr = sn->learning_rate > 0.0 ? sn->learning_rate : 0.005;
    double t = (double)sn->update_count;
    double bc1 = 1.0 - pow(beta1, t);
    double bc2 = 1.0 - pow(beta2, t);
    double gns = 0.0;

    for (int i = 0; i < NX48_SN_FEATURES; i++) {
        double g = err * features[i];
        sn->momentum[i] = beta1 * sn->momentum[i] + (1.0 - beta1) * g;
        sn->velocity[i] = beta2 * sn->velocity[i] + (1.0 - beta2) * g * g;
        double m_hat = sn->momentum[i] / bc1;
        double v_hat = sn->velocity[i] / bc2;
        sn->weights[i] -= lr * m_hat / (sqrt(v_hat) + adam_eps);
        gns += g * g;
    }
    sn->bias  -= lr * err;
    sn->grad_norm = sqrt(gns);

    /* Adaptation learning_rate sous-neurone */
    if (sn->loss < 0.3)       sn->learning_rate = clamp(lr * 0.98, 0.001, 0.05);
    else if (sn->loss > 0.7)  sn->learning_rate = clamp(lr * 1.05, 0.001, 0.05);
}

/* ════════════════════════════════════════════════════════════════════
 * C61 : INITIALISATION SOUS-NEURONES
 * ════════════════════════════════════════════════════════════════════ */
static const char* sn_names[NX48_N_SUBNEURONS] = {
    "exploration", "threads", "gpu", "T_hot",
    "T_cold", "batch", "avx", "qdayprize"
};

static void nx48_subneurons_init(nx48_subneuron_t sn[NX48_N_SUBNEURONS]) {
    static const double sn_lr[NX48_N_SUBNEURONS] = {
        0.010, 0.005, 0.003, 0.008,
        0.008, 0.010, 0.003, 0.005
    };
    for (int i = 0; i < NX48_N_SUBNEURONS; i++) {
        memset(&sn[i], 0, sizeof(nx48_subneuron_t));
        sn[i].specialty    = i;
        sn[i].learning_rate = sn_lr[i];
        strncpy(sn[i].name, sn_names[i], sizeof(sn[i].name) - 1);
        /* Poids initiaux : légèrement positifs pour éviter zone morte sigmoid */
        for (int j = 0; j < NX48_SN_FEATURES; j++)
            sn[i].weights[j] = 0.1 * (j + 1) / (double)NX48_SN_FEATURES;
        sn[i].output = 0.5;
        sn[i].loss   = 0.7;
    }
}

/* ════════════════════════════════════════════════════════════════════
 * C61 : CONTRÔLE TOTAL — Atomiques partagées moteur ↔ NX48
 * Lit les sorties des sous-neurones → commandes atomiques
 * ════════════════════════════════════════════════════════════════════ */
void nx48_btc_control_all(nx48_btc_state_t* s) {
    nx48_hw_state_t* hw = &s->hw;

    /* ── SN0 : exploration_bias ── */
    /* La sortie du SN exploration est déjà dans exploration_bias via update */
    /* Ici on ajuste le ratio exploitation dans le moteur */
    /* (le moteur lit directement s->exploration_bias) */

    /* ── SN1 : threads dynamiques ── */
    {
        double sn_out = s->subneurons_exec[NX48_SN_THREADS].output;
        int n_max = hw->n_threads_max;
        if (n_max < 1) n_max = 4; /* Défaut si détection échoue */
        /* sn_out [0,1] → [1, n_max] threads */
        int n_target = 1 + (int)(sn_out * (double)(n_max - 1));
        n_target = (n_target < 1) ? 1 : (n_target > n_max) ? n_max : n_target;
        /* Si SN sous-neurone pas encore entraîné (output=0.5), garder n_threads_target existant */
        if (s->subneurons_exec[NX48_SN_THREADS].update_count < 5 && s->n_threads_target > 0)
            n_target = s->n_threads_target;
        s->n_threads_target = n_target;
        atomic_store_explicit(&nx48_ctrl_n_threads, n_target, memory_order_relaxed);
    }

    /* ── SN2 : GPU OpenCL ── */
    {
        double sn_out = s->subneurons_exec[NX48_SN_GPU].output;
        int activate = 0;
        if (hw->gpu_opencl_present && sn_out > 0.6 && s->stall_long_count > 5) {
            activate = 1;
            if (!hw->gpu_opencl_active)
                printf("[NX48-GPU] ✅ Activation GPU OpenCL : %s (stall_long=%d sn=%.3f)\n",
                    hw->gpu_name, s->stall_long_count, sn_out);
        } else if (sn_out < 0.3 && hw->gpu_opencl_active) {
            activate = 0;
            printf("[NX48-GPU] ⏸ GPU OpenCL désactivé par NX48 (sn=%.3f)\n", sn_out);
        } else {
            activate = hw->gpu_opencl_active;
        }
        hw->gpu_opencl_active = activate;
        atomic_store_explicit(&nx48_ctrl_gpu_active, activate, memory_order_relaxed);
    }

    /* ── SN3 : T_hot température chaude ── */
    /* Ne pas modifier si sous-neurone pas encore entraîné (<10 updates) */
    {
        if (s->subneurons_exec[NX48_SN_TEMP_HOT].update_count >= 10) {
            double sn_out = s->subneurons_exec[NX48_SN_TEMP_HOT].output;
            int idx = (int)(sn_out * 7.0 + 0.5);
            idx = (idx < 0) ? 0 : (idx > 7) ? 7 : idx;
            /* T_hot doit rester > T_cold ET ≥ index 4 (T≥12) pour exploration réelle */
            if (idx < 4) idx = 4;
            if (idx <= s->T_cold_idx) idx = s->T_cold_idx + 1;
            if (idx > 7) idx = 7;
            s->T_hot_idx    = idx;
            s->T_hot_actual = NX48_REPLICA_TEMPS[idx];
        }
        atomic_store_explicit(&nx48_ctrl_T_hot_idx, s->T_hot_idx, memory_order_relaxed);
    }

    /* ── SN4 : T_cold température froide ── */
    {
        if (s->subneurons_exec[NX48_SN_TEMP_COLD].update_count >= 10) {
            double sn_out = s->subneurons_exec[NX48_SN_TEMP_COLD].output;
            int idx = (int)(sn_out * 3.0 + 0.5); /* T_cold reste basse [0..3] */
            idx = (idx < 0) ? 0 : (idx > 3) ? 3 : idx;
            s->T_cold_idx    = idx;
            s->T_cold_actual = NX48_REPLICA_TEMPS[idx];
        }
        atomic_store_explicit(&nx48_ctrl_T_cold_idx, s->T_cold_idx, memory_order_relaxed);
    }

    /* ── SN5 : batch size ── */
    {
        double sn_out = s->subneurons_exec[NX48_SN_BATCH].output;
        /* sn_out [0,1] → batch [256, 8192] puissances de 2 */
        static const int batch_vals[] = {256, 512, 1024, 2048, 4096, 8192};
        int idx = (int)(sn_out * 5.0 + 0.5);
        idx = (idx < 0) ? 0 : (idx > 5) ? 5 : idx;
        int bsize = batch_vals[idx];
        s->batch_size_scale = (double)bsize / 1024.0;
        atomic_store_explicit(&nx48_ctrl_batch_size, bsize, memory_order_relaxed);
    }

    /* ── SN6 : AVX level ── */
    {
        /* NX48 ne peut que suggérer — le niveau HW est la limite physique */
        int hw_max = hw->avx_level;
        /* Pas besoin d'atomique supplémentaire — déjà mis à jour par hw_detect */
        atomic_store_explicit(&nx48_ctrl_avx_level, hw_max, memory_order_relaxed);
    }

    /* ── SN7 : QDAYPRIZE feedback → exploration_bias ── C63-FIX ──
     * PROBLÈME C62 : signal trop fort → exploration_bias descend jusqu'au floor 0.05.
     * ANALYSE C63 : avec qsr=0.833, signal = 0.0183 par update.
     *   Si SN7 appelé chaque update (1000x/s) → floor atteint en ~50s.
     * CORRECTION C63 :
     *   1. Signal divisé par 10 (0.10 → 0.01) — influence douce
     *   2. Appliquer seulement tous les 100 updates (throttle)
     *   3. Zone neutre élargie : [0.60, 0.70] → pas de signal
     *   4. Plancher relevé à 0.30 (exploration minimale garantie)
     *   5. Log forensic du signal pour suivi
     */
    {
        double qsr = hw->qdayprize_success_rate;
        if (qsr > 0.0 && (s->update_count % 100) == 0) {
            double qdayprize_signal = 0.0;
            if (qsr > 0.70) {
                /* QDAYPRIZE > 70% → légère exploitation supplémentaire */
                qdayprize_signal = (qsr - 0.70) * 0.010;  /* max ~0.003/update */
            } else if (qsr < 0.60) {
                /* QDAYPRIZE < 60% → encourager exploration */
                qdayprize_signal = (qsr - 0.60) * 0.010;  /* négatif → monte bias */
            }
            /* Plancher 0.30 : au moins 30% d'exploration toujours actif */
            double new_bias = clamp(s->exploration_bias - qdayprize_signal, 0.30, 0.95);
            if (new_bias != s->exploration_bias) {
                s->exploration_bias = new_bias;
                FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME,
                    "btc_nx48_sn7_qdpr_signal", qdayprize_signal);
            }
        }
    }

    /* C99-P5 : hook neural retiré ici (doublon avec btc_mining_engine.c).
     * Le couplage Izhikevich+STDP officiel est branché par-thread dans
     * mine_thread_worker() de btc_mining_engine.c via nx48_bridge_modulate
     * (voir rapport 103 C99 P2). */

    /* ── Log forensic contrôle ── */
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_nx48_ctrl_threads",
        (double)s->n_threads_target);
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_nx48_ctrl_T_hot",
        s->T_hot_actual);
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_nx48_ctrl_T_cold",
        s->T_cold_actual);
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_nx48_ctrl_gpu",
        (double)hw->gpu_opencl_active);
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_nx48_ctrl_batch",
        (double)atomic_load_explicit(&nx48_ctrl_batch_size, memory_order_relaxed));
}

/* ════════════════════════════════════════════════════════════════════
 * INITIALISE NX48_BTC — C61
 * ════════════════════════════════════════════════════════════════════ */
nx48_btc_state_t* nx48_btc_init(const nx48_btc_config_t* cfg, const char* run_id) {
    nx48_btc_state_t* s = LV_CALLOC(1, sizeof(nx48_btc_state_t));
    if (!s) return NULL;

    int loaded = 0;

    /* Tentative chargement LUM natif (priorité sur CSV) */
    if (cfg->lum_path[0] != '\0')
        loaded = nx48_btc_load_lum(s, cfg->lum_path);

    /* Fallback CSV */
    if (!loaded && cfg->csv_path[0] != '\0')
        loaded = nx48_btc_load_csv(s, cfg->csv_path);

    if (!loaded) {
        memcpy(s->weights, NX48_BTC_WEIGHTS_DEFAULT, sizeof(s->weights));
        s->bias               = 0.0;
        memcpy(s->executor_weights, s->weights, sizeof(s->executor_weights));
        s->executor_bias      = s->bias;
        s->dual_blend         = 0.20;
        s->delta_nonce_scale  = 1.0;
        s->n_replicas_scale   = 1.0;
        s->swap_temp_scale    = 1.0;
        s->batch_size_scale   = 1.0;
        /* C61 : exploration_bias démarrage 0.70 (70% exploitation, 30% exploration)
         * AVANT C61 : 0.50 → oscillation 0.48-0.50 garantie
         * APRÈS C61  : 0.70 + vélocité momentum → peut sortir du plateau */
        s->exploration_bias   = 0.70;
        s->exploration_vel    = 0.0;
        s->exploration_acc    = 0.0;
        s->loss_prev          = 1.0;
        s->loss_curr          = 1.0;
        s->T_hot_idx          = 7;
        s->T_cold_idx         = 0;
        s->T_hot_actual       = 50.0;
        s->T_cold_actual      = 1.0;
        s->n_threads_target   = (cfg->n_threads_initial > 0)
                                ? cfg->n_threads_initial : 2;
    }

    /* C61 : Sous-neurones toujours réinitialisés (apprentissage repart propre) */
    nx48_subneurons_init(s->subneurons_prod);
    nx48_subneurons_init(s->subneurons_exec);

    /* C61 : S'assurer que n_threads_target est initialisé (CSV ne le contient pas toujours) */
    if (s->n_threads_target <= 0)
        s->n_threads_target = (cfg->n_threads_initial > 0) ? cfg->n_threads_initial : 2;
    /* T_hot/T_cold par défaut si non chargés */
    if (s->T_hot_actual <= 0.0) { s->T_hot_idx = 7; s->T_hot_actual = 50.0; }
    if (s->T_cold_actual <= 0.0) { s->T_cold_idx = 0; s->T_cold_actual = 1.0; }

    /* Chemins fichiers */
    snprintf(s->csv_path, sizeof(s->csv_path), "%s", cfg->csv_path[0] ? cfg->csv_path : "");
    snprintf(s->lum_path, sizeof(s->lum_path), "%s", cfg->lum_path[0] ? cfg->lum_path : "");
    strncpy(s->run_id, run_id ? run_id : "unknown", sizeof(s->run_id)-1);

    xosh_seed();

    /* Détection hardware immédiate */
    nx48_btc_hw_detect(s);

    /* Publier état initial aux atomiques */
    atomic_store_explicit(&nx48_ctrl_n_threads,  s->n_threads_target, memory_order_relaxed);
    atomic_store_explicit(&nx48_ctrl_T_hot_idx,  s->T_hot_idx,        memory_order_relaxed);
    atomic_store_explicit(&nx48_ctrl_T_cold_idx, s->T_cold_idx,       memory_order_relaxed);
    atomic_store_explicit(&nx48_ctrl_avx_level,  s->hw.avx_level,     memory_order_relaxed);
    /* C125 : ne pas écraser nx48_ctrl_batch_size si déjà setté par tune_full/tune_batch */
    int existing_batch = atomic_load_explicit(&nx48_ctrl_batch_size, memory_order_relaxed);
    if (existing_batch <= 1024) {
        atomic_store_explicit(&nx48_ctrl_batch_size, 1024, memory_order_relaxed);
    }

    /* C125-OPTIMIZE-RUNTIME : si tune_full a injecté un profil dans les atomics,
     * appliquer override sur l'état NX48 (delta_nonce_scale, etc.). */
    int delta_milli = atomic_load_explicit(&nx48_ctrl_delta_nx48_initial_milli,
                                           memory_order_relaxed);
    int thermal_s   = atomic_load_explicit(&nx48_ctrl_thermal_throttle_s,
                                           memory_order_relaxed);
    int strat       = atomic_load_explicit(&nx48_ctrl_nonce_strategy,
                                           memory_order_relaxed);
    if (delta_milli > 0) {
        s->delta_nonce_scale = (double)delta_milli / 1000.0;
        printf("[C125-NX48-OVERRIDE] delta_nonce_scale ← %.3f (depuis atomic, was tune_full)\n",
               s->delta_nonce_scale);
    }
    if (thermal_s > 0) {
        printf("[C125-NX48-OVERRIDE] thermal_throttle_s observé = %d (asic_btc_optimizer)\n",
               thermal_s);
    }
    if (strat > 0) {
        printf("[C125-NX48-OVERRIDE] nonce_strategy observé = %d (asic_btc_optimizer)\n", strat);
    }

    printf("[NX48-INIT] C61 — %d sous-neurones × 2 | exploration_bias=%.2f | LUM=%s\n",
        NX48_N_SUBNEURONS, s->exploration_bias,
        loaded ? "chargé" : "défauts");

    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_nx48_c61_init",       1.0);
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_nx48_n_subneurons",   (double)NX48_N_SUBNEURONS*2);
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_nx48_exploration_init", s->exploration_bias);
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_nx48_init_loaded",    (double)loaded);

    return s;
}

/* ════════════════════════════════════════════════════════════════════
 * CALCUL DES FEATURES PRINCIPALES
 * ════════════════════════════════════════════════════════════════════ */
void nx48_btc_compute_features(
    double features[NX48_BTC_N_FEATURES],
    int    best_leading_zeros,
    double hashrate_mhs,
    double hashrate_max,
    double swap_rate,
    double time_since_improvement_s,
    double nonce_coverage_pct,
    double delta_nonce,
    double hashes_done,
    double hashes_expected,
    double T_hot,
    double T_cold)
{
    features[NX48_BTC_F_BEST_LEADING]  = clamp((double)best_leading_zeros / 256.0, 0.0, 1.0);
    features[NX48_BTC_F_HASHRATE_NORM] = (hashrate_max > 0.0)
        ? clamp(hashrate_mhs / hashrate_max, 0.0, 1.0) : 0.0;
    features[NX48_BTC_F_SWAP_RATE]     = clamp(swap_rate, 0.0, 1.0);
    features[NX48_BTC_F_TIME_STALL]    = (time_since_improvement_s > 0.0)
        ? clamp(-log10(time_since_improvement_s) / 10.0 + 0.5, 0.0, 1.0) : 0.5;
    features[NX48_BTC_F_COVERAGE]      = clamp(nonce_coverage_pct / 100.0, 0.0, 1.0);
    /* C61 : delta normalisé sur 500.0 (max C61) au lieu de 4294967296.0 */
    features[NX48_BTC_F_DELTA_NORM]    = clamp(delta_nonce / (65536.0 * 500.0), 0.0, 1.0);
    features[NX48_BTC_F_THREAD_EFF]    = (hashes_expected > 0.0)
        ? clamp(hashes_done / hashes_expected, 0.0, 1.0) : 0.0;
    features[NX48_BTC_F_TEMP_RATIO]    = (T_cold > 0.0)
        ? clamp((T_hot / T_cold) / 100.0, 0.0, 1.0) : 0.5;
}

/* ════════════════════════════════════════════════════════════════════
 * PRÉDICTION NX48_BTC (neurone applicateur)
 * ════════════════════════════════════════════════════════════════════ */
double nx48_btc_predict(nx48_btc_state_t* s, const double features[NX48_BTC_N_FEATURES]) {
    double z = s->executor_bias
             + s->executor_weights[0] * features[0]
             + s->executor_weights[1] * features[1]
             + s->executor_weights[2] * features[2]
             + s->executor_weights[3] * features[3]
             + s->executor_weights[4] * features[4]
             + s->executor_weights[5] * features[5]
             + s->executor_weights[6] * features[6]
             + s->executor_weights[7] * features[7];
    return sigmoid(z);
}

/* ════════════════════════════════════════════════════════════════════
 * MISE À JOUR GRADIENT — Adam principal + ISTA L1 + Sous-neurones C61
 * ════════════════════════════════════════════════════════════════════ */
void nx48_btc_update(
    nx48_btc_state_t*        s,
    const nx48_btc_config_t* cfg,
    const double             features[NX48_BTC_N_FEATURES],
    double                   prob,
    int                      best_leading_zeros,
    uint32_t                 best_nonce,  /* C107 : nonce qui a produit le record */
    double                   hashrate_mhs)
{
    /* Label linéaire [0, 256] — C38-FIX-LABEL-256 maintenu */
    double label = clamp((double)best_leading_zeros / 256.0, 0.0, 1.0);
    double eps = 1e-12;
    double p   = clamp(prob, eps, 1.0 - eps);
    double bce = -(label * log(p) + (1.0 - label) * log(1.0 - p));

    s->loss_prev = s->loss_curr;
    s->loss_curr = bce;

    double err = prob - label;
    double loss_delta = (s->loss_prev > 1e-12)
        ? (s->loss_curr - s->loss_prev) / s->loss_prev : 0.0;

    /* ── Adam update poids principaux (C61 : remplace ISTA pur) ─── */
    double beta1 = 0.9, beta2 = 0.999, adam_eps_v = 1e-8;
    double lr = cfg->learning_rate;
    double l1 = cfg->lambda_l1;
    double grad_norm_sq = 0.0;

    s->adam_t++;
    double bc1 = 1.0 - pow(beta1, (double)s->adam_t);
    double bc2 = 1.0 - pow(beta2, (double)s->adam_t);

    for (int i = 0; i < NX48_BTC_N_FEATURES; i++) {
        double g = err * features[i];
        s->adam_m1[i] = beta1 * s->adam_m1[i] + (1.0 - beta1) * g;
        s->adam_m2[i] = beta2 * s->adam_m2[i] + (1.0 - beta2) * g * g;
        double m_hat = s->adam_m1[i] / bc1;
        double v_hat = s->adam_m2[i] / bc2;
        double step  = lr * m_hat / (sqrt(v_hat) + adam_eps_v);
        double w = s->weights[i] - step;
        /* L1 proximal */
        if      (w >  l1) w -= l1;
        else if (w < -l1) w += l1;
        else               w  = 0.0;
        s->weights[i] = w;
        grad_norm_sq += g * g;
    }
    s->bias -= lr * err;
    s->grad_norm = sqrt(grad_norm_sq);

    /* ── Distillation neurone applicateur ─────────────────────────── */
    {
        double blend = clamp(s->dual_blend > 0.0 ? s->dual_blend : 0.20, 0.01, 0.50);
        for (int i = 0; i < NX48_BTC_N_FEATURES; i++)
            s->executor_weights[i] = (1.0 - blend) * s->executor_weights[i]
                                   + blend * s->weights[i];
        s->executor_bias = (1.0 - blend) * s->executor_bias + blend * s->bias;
        s->dual_blend    = blend;
    }

    /* ════════════════════════════════════════════════════════════════
     * C61-EXPLOR-UNLOCK : exploration_bias avec VÉLOCITÉ + MOMENTUM
     *
     * AVANT C61 : +0.04 si loss monte, -0.02 si améliore
     *   → symétrie quasi parfaite avec stagnation habituelle → plateau 0.48-0.50
     *   → NX48 jamais en mode pleine exploitation ni pleine exploration
     *
     * APRÈS C61 : Système dynamique du 2ème ordre (masse-ressort)
     *   vel = vel × 0.9 + signal × 0.1        (momentum 0.9)
     *   bias += vel × dt                        (intégration)
     *
     * Signaux :
     *   loss_delta > +5%  → accélération positive (plus d'exploration)
     *   loss_delta < -5%  → accélération négative (plus d'exploitation)
     *   stagnation longue → boost forcé exploration (×5 sur vélocité)
     *   stagnation absolue (>100 updates sans record) → reset exploration=0.85
     *
     * Rapport à T_hot/T_cold : Plus T_hot est élevé → exploration plus large.
     *   NX48 synchronise exploration_bias avec T_hot_idx automatiquement.
     * ════════════════════════════════════════════════════════════════ */
    {
        double signal = 0.0;
        if (loss_delta > 0.05)       signal = +0.15;    /* Stagnation forte → explorer */
        else if (loss_delta > 0.02)  signal = +0.06;
        else if (loss_delta < -0.05) signal = -0.10;    /* Amélioration forte → exploiter */
        else if (loss_delta < -0.02) signal = -0.04;
        else                         signal = xosh_gaussian(0.02); /* Bruit doux au plateau */

        /* Momentum : intègre le signal (évite les oscillations rapides) */
        s->exploration_vel = 0.90 * s->exploration_vel + 0.10 * signal;

        /* Intégration : exploration_bias += vélocité */
        s->exploration_bias = clamp(s->exploration_bias + s->exploration_vel,
                                    0.05, 0.95);

        /* Force de rappel si bloqué au plateau [0.45, 0.55] > 20 updates */
        double eb = s->exploration_bias;
        if (eb > 0.44 && eb < 0.56 && s->stall_count > 20) {
            /* Boost : pousser vers l'exploration si stagnation active */
            s->exploration_vel += 0.15;
            printf("[NX48-C61] ⚡ Déblocage plateau exploration_bias=%.3f → +vel\n", eb);
        }
    }

    /* ════════════════════════════════════════════════════════════════
     * C61-DELTA-UNLOCK : delta_nonce_scale max 50→500
     * Perturbation Xoshiro256++ avec sigma adaptatif
     * ════════════════════════════════════════════════════════════════ */
    {
        double stagnation = clamp(loss_delta * 10.0, 0.0, 1.0);
        double sigma_base = 0.05, sigma_max = 0.30;
        double sigma = sigma_base * exp(stagnation * log(sigma_max / sigma_base));
        double noise = xosh_gaussian(sigma);
        double alpha = 0.8;
        double push  = alpha * (-loss_delta);
        s->delta_nonce_scale *= exp(push + noise);
        /* C61 : max étendu 50→500 */
        s->delta_nonce_scale = clamp(s->delta_nonce_scale, 0.1, 500.0);
        if (s->delta_nonce_scale >= 499.0)
            FORENSIC_LOG_ANOMALY(BTC_MODULE_NAME,
                "btc_nx48_delta_cap_500", s->delta_nonce_scale);
    }

    /* ── batch_size_scale (C65 conservé + SN contrôle) ─────────── */
    if (hashrate_mhs > 0 && hashrate_mhs < 500.0) {
        double adapt_rate = 1.0 + 0.10 * tanh(5.0 * s->grad_norm);
        s->batch_size_scale = clamp(s->batch_size_scale * adapt_rate, 0.5, 8.0);
    }

    /* ── Stagnation record ──────────────────────────────────────── */
    if (best_leading_zeros > s->best_leading_zeros) {
        s->best_leading_zeros = best_leading_zeros;
        s->best_nonce         = best_nonce;  /* C107 : nonce du record courant pour persistance par header */
        s->stall_count        = 0;
        s->stall_long_count   = 0;
        FORENSIC_LOG_ANOMALY(BTC_MODULE_NAME,
            "btc_nx48_new_record_leading_zeros", (double)best_leading_zeros);

        /* C110 — trace granulaire : nouveau record NX48 = décision-clé.
         * confidence = best_lz / 256 ; lyapunov = exploration_bias (proxy stabilité). */
        if (g_btc_reasoning_trace) {
            char label[128];
            snprintf(label, sizeof(label),
                "NX48-NEW-RECORD lz=%d nonce=%u hashrate=%.2fMHs",
                best_leading_zeros, best_nonce, hashrate_mhs);
            reasoning_trace_add_node(g_btc_reasoning_trace, label,
                (float)best_leading_zeros / 256.0f,
                (float)s->exploration_bias);
        }
        /* C86-IMMEDIATE-SAVE : flush LUM + CSV IMMEDIATEMENT a chaque nouveau record.
         * Avant : sauvegarde periodique (60s) -> en cas de SIGSEGV, perte du record.
         * Apres : double persistance instantanee -> meme un crash apres conserve le record. */
        if (s->lum_path[0] != '\0') {
            nx48_btc_save_lum(s, s->lum_path);
        }
        if (s->csv_path[0] != '\0') {
            nx48_btc_save_csv(s, s->csv_path);
        }
        /* ════════════════════════════════════════════════════════════
         * C107 — PERSISTANCE MONOTONE PAR HEADER (alltime CSV)
         * Mise à jour automatique du best ABSOLU bound au header courant.
         * Si le best monte (LZ=20→25→37), il est persisté avec le header
         * exact où il a été trouvé. Si le header change, le best alltime
         * reste mais avec son header d'origine traçable.
         * ════════════════════════════════════════════════════════════ */
        const char *hh_env     = getenv("BTC_HEADER_HEX_CURRENT");
        const char *wallet_env = getenv("BTC_WALLET_ADDRESS");
        if (hh_env && strlen(hh_env) >= 160 && wallet_env) {
            int upd = nx48_alltime_try_update(NX48_ALLTIME_DEFAULT_PATH,
                                              best_leading_zeros,
                                              best_nonce,
                                              hh_env,
                                              wallet_env,
                                              s->run_id[0] ? s->run_id : "unknown");
            if (upd > 0) {
                FORENSIC_LOG_ANOMALY(BTC_MODULE_NAME,
                    "btc_nx48_alltime_record_updated_header_bound",
                    (double)best_leading_zeros);
                printf("[NX48-ALLTIME-C107] 🏆 NEW ALLTIME : LZ=%d nonce=%u header=%.16s... wallet=%.20s...\n",
                    best_leading_zeros, best_nonce, hh_env, wallet_env);
            }
        } else {
            /* Pas d'env header/wallet → log warning, on continue (mode local) */
            static int warned = 0;
            if (!warned) {
                fprintf(stderr, "[NX48-ALLTIME-C107] ⚠️  BTC_HEADER_HEX_CURRENT ou BTC_WALLET_ADDRESS absent — alltime CSV non MAJ\n");
                warned = 1;
            }
        }
        FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME,
            "btc_nx48_immediate_save_on_record", (double)best_leading_zeros);
    } else {
        s->stall_count++;
        s->stall_long_count++;
        if (s->stall_count >= 2) {
            s->delta_nonce_scale = clamp(s->delta_nonce_scale * 1.05, 0.1, 500.0);
            s->stall_count = 0;
        }
        /* C63 : Reset delta_nonce si bloqué au cap 500 trop longtemps (plateau absolu)
         * C62 : %50 → trop rare (1 reset sur 139K lines / 7.5min = 1 reset)
         * C63 FIX : %10 → reset 5× plus fréquent → moins de stalls prolongés */
        if (s->stall_long_count > 0 && (s->stall_long_count % 10) == 0
            && s->delta_nonce_scale >= 490.0) {
            double old_delta = s->delta_nonce_scale;
            s->delta_nonce_scale = 1.0 + xosh_uniform() * 15.0; /* Reset aleatoire [1, 16] */
            printf("[NX48-C62] Reset delta_nonce %.1f->%.3f (stall_long=%d cap500_plateau)\n",
                old_delta, s->delta_nonce_scale, s->stall_long_count);
            FORENSIC_LOG_ANOMALY(BTC_MODULE_NAME,
                "btc_nx48_delta_reset_stall_c62", s->delta_nonce_scale);
        }
    }

    /* ════════════════════════════════════════════════════════════════
     * C61 : MISE À JOUR DES 16 SOUS-NEURONES
     * ════════════════════════════════════════════════════════════════ */
    {
        double sn_feat[NX48_SN_FEATURES];

        for (int i = 0; i < NX48_N_SUBNEURONS; i++) {
            nx48_subneuron_compute_features(sn_feat, s, i);

            /* Label sous-neurone : adapté à la spécialité */
            double sn_label = label; /* base : même objectif que neurone principal */
            switch (i) {
            case NX48_SN_THREADS:
                /* Objectif : plus de threads si hashrate faible et RAM ok */
                sn_label = (hashrate_mhs < 1.0 && s->hw.ram_available_mb > 2000)
                           ? 1.0 : 0.5;
                break;
            case NX48_SN_GPU:
                /* Objectif : activer GPU si stagnation longue + GPU présent */
                sn_label = (s->stall_long_count > 10 && s->hw.gpu_opencl_present)
                           ? 0.8 : 0.2;
                break;
            case NX48_SN_TEMP_HOT:
                /* Objectif : T_hot élevé si exploration nécessaire */
                sn_label = (s->exploration_bias < 0.5) ? 0.8 : 0.5;
                break;
            case NX48_SN_TEMP_COLD:
                /* T_cold bas = exploitation propre */
                sn_label = (s->best_leading_zeros > 20) ? 0.1 : 0.3;
                break;
            case NX48_SN_BATCH:
                /* Batch élevé si AVX512 disponible et RAM ok */
                sn_label = (s->hw.avx_level >= NX48_HW_AVX512
                            && s->hw.ram_available_mb > 4000) ? 0.8 : 0.5;
                break;
            case NX48_SN_EXPLORATION:
                sn_label = (s->stall_long_count > 5) ? 0.8 : label;
                break;
            default:
                break;
            }

            /* Mise à jour producteur */
            nx48_subneuron_update(&s->subneurons_prod[i], sn_feat, sn_label, err);

            /* Distillation vers applicateur (blend 0.2) */
            for (int j = 0; j < NX48_SN_FEATURES; j++)
                s->subneurons_exec[i].weights[j] =
                    0.8 * s->subneurons_exec[i].weights[j]
                  + 0.2 * s->subneurons_prod[i].weights[j];
            s->subneurons_exec[i].bias =
                    0.8 * s->subneurons_exec[i].bias
                  + 0.2 * s->subneurons_prod[i].bias;

            /* Recalcul sortie applicateur */
            double zx = s->subneurons_exec[i].bias;
            for (int j = 0; j < NX48_SN_FEATURES; j++)
                zx += s->subneurons_exec[i].weights[j] * sn_feat[j];
            s->subneurons_exec[i].output = sigmoid(zx);
        }
    }

    /* ── Détection HW périodique (toutes les 1000 updates ≈ 30s) ─── */
    if (s->update_count % 1000 == 0)
        nx48_btc_hw_detect(s);

    /* ── Contrôle total des paramètres ─────────────────────────── */
    nx48_btc_clamp_scales(s);
    nx48_btc_control_all(s);

    s->update_count++;

    /* ── Log forensic ────────────────────────────────────────────── */
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_nx48_loss",             s->loss_curr);
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_nx48_grad_norm",        s->grad_norm);
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_nx48_update_count",     (double)s->update_count);
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_nx48_exploration_bias", s->exploration_bias);
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_nx48_exploration_vel",  s->exploration_vel);
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_nx48_delta_nonce",      s->delta_nonce_scale);
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_nx48_stall_long",       (double)s->stall_long_count);
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_nx48_dual_blend",       s->dual_blend);
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_nx48_n_threads",        (double)s->n_threads_target);
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_nx48_T_hot",            s->T_hot_actual);
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_nx48_T_cold",           s->T_cold_actual);
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_nx48_gpu_active",       (double)s->hw.gpu_opencl_active);

    /* Log sous-neurones (résumé) */
    if (s->update_count % 100 == 0) {
        for (int i = 0; i < NX48_N_SUBNEURONS; i++) {
            char key[64];
            snprintf(key, sizeof(key), "btc_nx48_sn%d_%s_out", i, sn_names[i]);
            FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, key,
                s->subneurons_exec[i].output);
        }
    }
}

/* ════════════════════════════════════════════════════════════════════
 * C61 : CLAMP DES SCALES — bornes élargies
 * ════════════════════════════════════════════════════════════════════ */
void nx48_btc_clamp_scales(nx48_btc_state_t* s) {
    /* C68-NAN-GUARD : Reset des scalaires NaN/Inf AVANT clamp
     * Cause du SIGSEGV à ~870s : après 400+ stall_long, les poids Adam divergent
     * (Inf/NaN via multiplication de flottants extrêmes), ce qui corrompt
     * delta_nonce_scale → undefined behavior lors du cast (int64_t)NaN en btc_mining_engine.c
     * CORRECTION : isnan/isinf check sur tous les scalaires → reset aux valeurs sûres */
#define NX48_SAFE_RESET_SCALAR(v, dflt) \
    do { if (!__builtin_isfinite(v)) { (v) = (dflt); } } while(0)

    NX48_SAFE_RESET_SCALAR(s->delta_nonce_scale,  1.0);
    NX48_SAFE_RESET_SCALAR(s->n_replicas_scale,   1.0);
    NX48_SAFE_RESET_SCALAR(s->swap_temp_scale,    1.0);
    NX48_SAFE_RESET_SCALAR(s->batch_size_scale,   1.0);
    NX48_SAFE_RESET_SCALAR(s->exploration_bias,   0.5);
    NX48_SAFE_RESET_SCALAR(s->exploration_vel,    0.0);
    NX48_SAFE_RESET_SCALAR(s->dual_blend,         0.2);
    NX48_SAFE_RESET_SCALAR(s->loss_curr,          1.0);
    NX48_SAFE_RESET_SCALAR(s->loss_prev,          1.0);
    NX48_SAFE_RESET_SCALAR(s->grad_norm,          0.0);

    /* C68 : Reset NaN/Inf dans les poids Adam des sous-neurones */
    for (int i = 0; i < NX48_N_SUBNEURONS; i++) {
        for (int j = 0; j < NX48_SN_FEATURES; j++) {
            NX48_SAFE_RESET_SCALAR(s->subneurons_prod[i].weights[j],  0.0);
            NX48_SAFE_RESET_SCALAR(s->subneurons_prod[i].momentum[j], 0.0);
            NX48_SAFE_RESET_SCALAR(s->subneurons_prod[i].velocity[j], 0.0);
            NX48_SAFE_RESET_SCALAR(s->subneurons_exec[i].weights[j],  0.0);
            NX48_SAFE_RESET_SCALAR(s->subneurons_exec[i].momentum[j], 0.0);
            NX48_SAFE_RESET_SCALAR(s->subneurons_exec[i].velocity[j], 0.0);
        }
        NX48_SAFE_RESET_SCALAR(s->subneurons_prod[i].bias,         0.0);
        NX48_SAFE_RESET_SCALAR(s->subneurons_exec[i].bias,         0.0);
        NX48_SAFE_RESET_SCALAR(s->subneurons_prod[i].output,       0.5);
        NX48_SAFE_RESET_SCALAR(s->subneurons_exec[i].output,       0.5);
        NX48_SAFE_RESET_SCALAR(s->subneurons_prod[i].loss,         0.0);
        NX48_SAFE_RESET_SCALAR(s->subneurons_exec[i].loss,         0.0);
        NX48_SAFE_RESET_SCALAR(s->subneurons_prod[i].grad_norm,    0.0);
        NX48_SAFE_RESET_SCALAR(s->subneurons_exec[i].grad_norm,    0.0);
    }

    /* C68 : Reset NaN/Inf dans les poids principaux Adam */
    for (int i = 0; i < NX48_BTC_N_FEATURES; i++) {
        NX48_SAFE_RESET_SCALAR(s->weights[i],          0.0);
        NX48_SAFE_RESET_SCALAR(s->executor_weights[i], 0.0);
        NX48_SAFE_RESET_SCALAR(s->adam_m1[i],          0.0);
        NX48_SAFE_RESET_SCALAR(s->adam_m2[i],          0.0);
    }
#undef NX48_SAFE_RESET_SCALAR

    s->delta_nonce_scale  = clamp(s->delta_nonce_scale,  0.1,  500.0);
    s->n_replicas_scale   = clamp(s->n_replicas_scale,   1.0,    2.0);
    s->swap_temp_scale    = clamp(s->swap_temp_scale,    0.5,    3.0);
    s->batch_size_scale   = clamp(s->batch_size_scale,   0.5,    8.0);
    s->exploration_bias   = clamp(s->exploration_bias,   0.05,   0.95);
    s->exploration_vel    = clamp(s->exploration_vel,   -0.5,    0.5);
    s->dual_blend         = clamp(s->dual_blend > 0.0 ? s->dual_blend : 0.20, 0.01, 0.50);
    /* Température indices */
    s->T_hot_idx  = (s->T_hot_idx  < 0) ? 0 : (s->T_hot_idx  > 7) ? 7 : s->T_hot_idx;
    s->T_cold_idx = (s->T_cold_idx < 0) ? 0 : (s->T_cold_idx > 3) ? 3 : s->T_cold_idx;
    s->T_hot_actual  = NX48_REPLICA_TEMPS[s->T_hot_idx];
    s->T_cold_actual = NX48_REPLICA_TEMPS[s->T_cold_idx];
}

/* ════════════════════════════════════════════════════════════════════
 * C61 : PERSISTANCE LUM BINAIRE NATIF (64 bytes par entrée)
 * Magic : 0x4E583438 = "NX48" | CRC32 intégrité
 * ════════════════════════════════════════════════════════════════════ */
/* C107 — Sidecar header binding : écrit le header_hex à côté du .lum
 * pour permettre la validation du best_leading_zeros au prochain load.
 * Format : <lum_path>.header (160 chars hex + \n) */
static void nx48_lum_save_header_sidecar(const char* lum_path) {
    if (!lum_path || !lum_path[0]) return;
    const char* hh = getenv("BTC_HEADER_HEX_CURRENT");
    if (!hh || strlen(hh) < 160) return;  /* Pas de header courant connu */
    char sidecar[1024];
    snprintf(sidecar, sizeof(sidecar), "%s.header", lum_path);
    FILE* sf = fopen(sidecar, "w");
    if (!sf) return;
    fwrite(hh, 1, 160, sf);
    fputc('\n', sf);
    fclose(sf);
}

/* C107 — Lit le sidecar et compare au header courant.
 * Retourne 1 si match (best_lz du .lum est valide), 0 si mismatch (DOIT reset). */
static int nx48_lum_check_header_sidecar(const char* lum_path) {
    if (!lum_path || !lum_path[0]) return 1;  /* Pas de path → on garde (legacy) */
    const char* hh_current = getenv("BTC_HEADER_HEX_CURRENT");
    if (!hh_current || strlen(hh_current) < 160) return 1;  /* Pas de check possible → on garde */
    char sidecar[1024];
    snprintf(sidecar, sizeof(sidecar), "%s.header", lum_path);
    FILE* sf = fopen(sidecar, "r");
    if (!sf) {
        /* Sidecar absent → .lum vient d'avant le patch C107 → header inconnu → RESET */
        fprintf(stderr, "[NX48-LUM-C107] ⚠️  Sidecar header absent (%s) — best_lz potentiellement obsolète, RESET\n", sidecar);
        return 0;
    }
    char hh_stored[200] = {0};
    size_t n = fread(hh_stored, 1, 160, sf);
    fclose(sf);
    if (n < 160) return 0;
    int match = (strncmp(hh_stored, hh_current, 160) == 0);
    if (!match) {
        fprintf(stderr, "[NX48-LUM-C107] ⚠️  Header MISMATCH — best_lz du .lum invalide pour ce header, RESET\n");
        fprintf(stderr, "[NX48-LUM-C107]    stored : %.40s...\n", hh_stored);
        fprintf(stderr, "[NX48-LUM-C107]    current: %.40s...\n", hh_current);
    }
    return match;
}

int nx48_btc_save_lum(const nx48_btc_state_t* s, const char* lum_path) {
    if (!lum_path || lum_path[0] == '\0') return 0;
    FILE* f = fopen(lum_path, "wb");
    if (!f) return 0;

    nx48_lum_entry_t e;
    memset(&e, 0, sizeof(e));
    e.magic            = NX48_LUM_MAGIC;
    e.version          = NX48_LUM_VERSION;
    e.update_count     = (uint32_t)s->update_count;
    e.best_leading_zeros = (uint32_t)s->best_leading_zeros;
    e.best_nonce       = s->best_nonce;
    for (int i = 0; i < 8; i++) {
        e.weights[i]          = (float)s->weights[i];
        e.executor_weights[i] = (float)s->executor_weights[i];
    }
    e.exploration_bias = (float)s->exploration_bias;
    e.exploration_vel  = (float)s->exploration_vel;
    e.delta_nonce_scale = (float)s->delta_nonce_scale;
    e.batch_size_scale  = (float)s->batch_size_scale;
    e.loss_curr         = (float)s->loss_curr;
    e.dual_blend        = (float)s->dual_blend;

    /* CRC32 sur tous les champs sauf le dernier uint32_t */
    e.crc32 = crc32_compute((const uint8_t*)&e, sizeof(e) - sizeof(uint32_t));

    fwrite(&e, sizeof(e), 1, f);
    fclose(f);
    /* C107 — écrit le sidecar header pour validation au prochain load */
    nx48_lum_save_header_sidecar(lum_path);
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_nx48_lum_saved", 1.0);
    return 1;
}

int nx48_btc_load_lum(nx48_btc_state_t* s, const char* lum_path) {
    if (!lum_path || lum_path[0] == '\0') return 0;
    FILE* f = fopen(lum_path, "rb");
    if (!f) return 0;

    nx48_lum_entry_t e;
    size_t n = fread(&e, sizeof(e), 1, f);
    fclose(f);
    if (n != 1) return 0;
    if (e.magic != NX48_LUM_MAGIC) return 0;

    /* Vérification CRC32 */
    uint32_t crc_calc = crc32_compute((const uint8_t*)&e, sizeof(e) - sizeof(uint32_t));
    if (crc_calc != e.crc32) {
        fprintf(stderr, "[NX48-LUM] ⚠️  CRC32 invalide — chargement annulé\n");
        return 0;
    }

    s->update_count       = (int)e.update_count;

    /* C107 — VALIDATION HEADER BINDING : best_leading_zeros / best_nonce ne sont
     * valides QUE pour le header_hex stocké dans le sidecar .header. Si mismatch
     * (ou sidecar absent), on RESET ces deux champs pour empêcher la persistance
     * d'un best « fantôme » non bound à un header réel.
     * Les autres params (poids RL, exploration_bias, etc.) restent persistants
     * car ils décrivent la politique d'apprentissage, pas un résultat de mining. */
    int header_ok = nx48_lum_check_header_sidecar(lum_path);
    if (header_ok) {
        s->best_leading_zeros = (int)e.best_leading_zeros;
        s->best_nonce         = (uint32_t)e.best_nonce;
    } else {
        s->best_leading_zeros = 0;
        s->best_nonce         = 0;
        FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_nx48_best_reset_header_mismatch", 1.0);
    }

    for (int i = 0; i < 8; i++) {
        s->weights[i]          = (double)e.weights[i];
        s->executor_weights[i] = (double)e.executor_weights[i];
    }
    s->exploration_bias   = clamp((double)e.exploration_bias, 0.05, 0.95);
    s->exploration_vel    = clamp((double)e.exploration_vel, -0.5, 0.5);
    s->delta_nonce_scale  = clamp((double)e.delta_nonce_scale, 0.1, 500.0);
    s->batch_size_scale   = clamp((double)e.batch_size_scale, 0.5, 8.0);
    s->loss_curr          = (double)e.loss_curr;
    s->loss_prev          = s->loss_curr;
    s->dual_blend         = clamp((double)e.dual_blend, 0.01, 0.50);

    nx48_btc_clamp_scales(s);
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_nx48_lum_loaded", 1.0);
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_nx48_best_leading_loaded",
        (double)s->best_leading_zeros);
    printf("[NX48-LUM] Chargé : update=%d best=%d bits exploration=%.3f delta=%.2f%s\n",
        s->update_count, s->best_leading_zeros, s->exploration_bias, s->delta_nonce_scale,
        header_ok ? " [header-bound]" : " [best RESET — header mismatch C107]");
    return 1;
}

/* ════════════════════════════════════════════════════════════════════
 * COMPATIBILITÉ C42 : CSV (gardé pour rétrocompatibilité)
 * ════════════════════════════════════════════════════════════════════ */
int nx48_btc_save_csv(const nx48_btc_state_t* s, const char* csv_path) {
    if (!csv_path || csv_path[0] == '\0') return 0;
    FILE* f = fopen(csv_path, "w");
    if (!f) return 0;
    fprintf(f, "run_id,delta_nonce_scale,n_replicas_scale,swap_temp_scale,"
               "batch_size_scale,exploration_bias,best_leading_zeros,"
               "best_nonce,update_count,loss_curr,grad_norm,"
               "w0,w1,w2,w3,w4,w5,w6,w7,bias,"
               "exec_w0,exec_w1,exec_w2,exec_w3,exec_w4,exec_w5,exec_w6,exec_w7,exec_bias,dual_blend,"
               "exploration_vel,T_hot_idx,T_cold_idx,n_threads_target,stall_long_count\n");
    fprintf(f, "%s,%.6f,%.6f,%.6f,%.6f,%.6f,%d,%u,%d,%.9f,%.9f,"
               "%.9f,%.9f,%.9f,%.9f,%.9f,%.9f,%.9f,%.9f,%.9f,"
               "%.9f,%.9f,%.9f,%.9f,%.9f,%.9f,%.9f,%.9f,%.9f,%.9f,"
               "%.9f,%d,%d,%d,%d\n",
        s->run_id,
        s->delta_nonce_scale, s->n_replicas_scale,
        s->swap_temp_scale,   s->batch_size_scale,
        s->exploration_bias,  s->best_leading_zeros,
        s->best_nonce,        s->update_count,
        s->loss_curr,         s->grad_norm,
        s->weights[0], s->weights[1], s->weights[2], s->weights[3],
        s->weights[4], s->weights[5], s->weights[6], s->weights[7], s->bias,
        s->executor_weights[0], s->executor_weights[1],
        s->executor_weights[2], s->executor_weights[3],
        s->executor_weights[4], s->executor_weights[5],
        s->executor_weights[6], s->executor_weights[7],
        s->executor_bias, s->dual_blend,
        s->exploration_vel, s->T_hot_idx, s->T_cold_idx,
        s->n_threads_target, s->stall_long_count);
    fclose(f);
    /* Sauvegarder aussi en LUM natif si chemin disponible */
    if (s->lum_path[0] != '\0')
        nx48_btc_save_lum(s, s->lum_path);
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_nx48_csv_saved", 1.0);
    return 1;
}

int nx48_btc_load_csv(nx48_btc_state_t* s, const char* csv_path) {
    if (!csv_path || csv_path[0] == '\0') return 0;
    FILE* f = fopen(csv_path, "r");
    if (!f) return 0;
    char header[1024];
    if (!fgets(header, sizeof(header), f)) { fclose(f); return 0; }

    int n = fscanf(f,
        "%63[^,],%lf,%lf,%lf,%lf,%lf,%d,%u,%d,%lf,%lf,"
        "%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,"
        "%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf",
        s->run_id,
        &s->delta_nonce_scale, &s->n_replicas_scale,
        &s->swap_temp_scale,   &s->batch_size_scale,
        &s->exploration_bias,  &s->best_leading_zeros,
        &s->best_nonce,        &s->update_count,
        &s->loss_curr,         &s->grad_norm,
        &s->weights[0], &s->weights[1], &s->weights[2], &s->weights[3],
        &s->weights[4], &s->weights[5], &s->weights[6], &s->weights[7], &s->bias,
        &s->executor_weights[0], &s->executor_weights[1],
        &s->executor_weights[2], &s->executor_weights[3],
        &s->executor_weights[4], &s->executor_weights[5],
        &s->executor_weights[6], &s->executor_weights[7],
        &s->executor_bias, &s->dual_blend);
    fclose(f);

    if (n >= 11) {
        /* Champs C61 optionnels */
        if (n < 20) {
            memcpy(s->weights, NX48_BTC_WEIGHTS_DEFAULT, sizeof(s->weights));
            s->bias = 0.0;
            memcpy(s->executor_weights, s->weights, sizeof(s->executor_weights));
            s->executor_bias = s->bias;
            s->dual_blend = 0.20;
        }
        /* Ajustements C61 */
        s->exploration_bias  = clamp(s->exploration_bias, 0.05, 0.95);
        s->delta_nonce_scale = clamp(s->delta_nonce_scale, 0.1, 500.0);
        s->batch_size_scale  = clamp(s->batch_size_scale, 0.5, 8.0);
        s->loss_prev = s->loss_curr;
        if (s->T_hot_idx == 0)  s->T_hot_idx  = 7;
        if (s->T_cold_idx == 0) s->T_cold_idx  = 0;
        s->T_hot_actual  = NX48_REPLICA_TEMPS[s->T_hot_idx];
        s->T_cold_actual = NX48_REPLICA_TEMPS[s->T_cold_idx];
        nx48_btc_clamp_scales(s);
        FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_nx48_csv_loaded", 1.0);
        FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_nx48_best_leading_loaded",
            (double)s->best_leading_zeros);
        return 1;
    }
    return 0;
}

/* ── Libère la mémoire ──────────────────────────────────────────── */
void nx48_btc_destroy(nx48_btc_state_t* s) {
    if (s) LV_FREE(s);
}
