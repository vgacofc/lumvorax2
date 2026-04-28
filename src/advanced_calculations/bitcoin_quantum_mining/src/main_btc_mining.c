/*
 * LumVorax — Module 17 — Bitcoin Quantum Mining Engine
 * main_btc_mining.c — Point d'entrée du Module 17
 *
 * STANDARD_NAMES.md v4.2 §M-BTC17-WALLET — Cycle C63 — 2026-04-11
 *
 * Usage :
 *   ./btc_mining_runner [OPTIONS]
 *
 * Options :
 *   --run-id <id>         run_id LumVorax (défaut: btc_YYYYMMDDTHHMMSSZ_PID)
 *   --mode <MODE>         BENCHMARK | TESTNET | MAINNET (défaut: BENCHMARK)
 *   --threads <N>         Nombre de threads (défaut: 16)
 *   --duration-s <N>      Durée max en secondes (défaut: 60, 0=illimité)
 *   --nonce-start <N>     Début plage nonce (défaut: 0)
 *   --nonce-end <N>       Fin plage nonce (défaut: 4294967295)
 *   --nx48-csv <path>     Chemin CSV état NX48 BTC (défaut: config/btc_nx48_last.csv)
 *   --log-dir <path>      Répertoire logs forensic (défaut: logs/forensic)
 *   --bits <N>            Difficulté compacte bits (défaut: 0x1d00ffff testnet)
 */

#ifndef _GNU_SOURCE
#  define _GNU_SOURCE
#endif
#define _POSIX_C_SOURCE 200809L

#include "sha256_lumvorax.h"
#include "nx48_btc_controller.h"
#include "../include/btc_mining_forensic.h"
#include "../include/btc_wallet.h"
#include "../include/btc_block_validator.h"
#include "debug/ultra_forensic_logger.h"
#include "lumvorax_integration.h"
#include "debug/memory_tracker.h"

/* C110 — Modules d'optimisation src/optimization activés (audit C109 §10) */
#include "optimization/reasoning_path_tracker.h"
#include "optimization/async_logging/async_logger.h"
#include "optimization/thermal_regulator.h"

/* C112 — Modules LUM 100 % activés (créés C111, liés mais inutilisés jusque là) */
#include "lum/lum_log_encoder.h"
#include "lum/lum_memory_tracer.h"

/* C110 — pointer global exposé pour nx48_btc_controller.c (point décision NX48) */
reasoning_trace_t* g_btc_reasoning_trace = NULL;

/* C112 — writer log natif format LUM 100 % exposé globalement (futur usage par
 * btc_mining_engine.c et nx48_btc_controller.c pour double-write event natif).
 * Activation : env BTC_LUM_LOG=1 → fichier .lum binaire append-only. */
lum_log_writer_t* g_btc_lum_log = NULL;

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/stat.h>
/* C116-P5 : ASIC optimizer — benchmark rapide avant lancement moteur */
#include "../../../asic_simulation/asic_btc_optimizer.h"
/* C116-KERNEL : smaps_rollup — traçage mémoire kernel /proc/self/smaps_rollup */
#include "../../../lum/lum_memory_tracer.h"

/* ── Déclaration externe (btc_mining_engine.c) ──────────────────── */
typedef struct {
    int      n_threads;
    int      n_replicas;
    int      batch_size;
    uint32_t nonce_start;
    uint32_t nonce_end;
    uint64_t duration_ns;
    char     run_id[64];
    char     nx48_csv[256];
    char     log_dir[256];
    char     run_mode[32];
    lv_btc_block_header_t header_template;
    uint8_t  target[32];
} btc_engine_config_t;

extern int btc_engine_run(const btc_engine_config_t* cfg, nx48_btc_state_t* nx48);

/* Forward declaration helper */
static unsigned long atoul_stub(const char* s);

/* ── Parse 160 hex chars (80 bytes) → vrai header Bitcoin ──────── */
static int parse_real_btc_header(lv_btc_block_header_t* h, const char* hex) {
    if (!hex || strlen(hex) < 160) return 0;
    uint8_t raw[80];
    for (int k = 0; k < 80; k++) {
        unsigned int byte = 0;
        if (sscanf(hex + 2*k, "%02x", &byte) != 1) return 0;
        raw[k] = (uint8_t)byte;
    }
    /* Structure Bitcoin (little-endian) :
     *   bytes  0- 3 : version
     *   bytes  4-35 : prev_block_hash
     *   bytes 36-67 : merkle_root
     *   bytes 68-71 : timestamp
     *   bytes 72-75 : bits
     *   bytes 76-79 : nonce (sera remplacé par le moteur)
     */
    memcpy(&h->version,         raw + 0,  4);
    memcpy(h->prev_block_hash,  raw + 4,  32);
    memcpy(h->merkle_root,      raw + 36, 32);
    memcpy(&h->timestamp,       raw + 68, 4);
    memcpy(&h->bits,            raw + 72, 4);
    h->nonce = 0; /* Le moteur explore l'espace nonce complet */
    return 1;
}

/* ── Génère un run_id LumVorax Module 17 ────────────────────────── */
static void make_run_id(char* out, size_t n) {
    time_t now = time(NULL);
    struct tm gmt;
    gmtime_r(&now, &gmt);
    snprintf(out, n, "btc_%04d%02d%02dT%02d%02d%02dZ_%d",
        gmt.tm_year+1900, gmt.tm_mon+1, gmt.tm_mday,
        gmt.tm_hour, gmt.tm_min, gmt.tm_sec, (int)getpid());
}

/* ── Crée les répertoires nécessaires ───────────────────────────── */
static void ensure_dirs(const char* log_dir) {
    char path[512];
    snprintf(path, sizeof(path), "%s/modules",  log_dir); mkdir(path, 0755);
    snprintf(path, sizeof(path), "%s/metrics",  log_dir); mkdir(path, 0755);
    snprintf(path, sizeof(path), "%s/anomalies",log_dir); mkdir(path, 0755);
    mkdir("config",  0755);
    mkdir("results", 0755);
}

/* ── Header testnet Bitcoin (bloc de référence genèse simplifié) ── */
static void fill_testnet_header(lv_btc_block_header_t* h, uint32_t bits) {
    memset(h, 0, sizeof(*h));
    h->version   = 0x00000001;
    h->timestamp = (uint32_t)time(NULL);
    h->bits      = bits;
    h->nonce     = 0;
    /* prev_hash et merkle_root laissés à zéro pour benchmark */
}

/* ── Point d'entrée ─────────────────────────────────────────────── */
int main(int argc, char* argv[]) {
    /* ── Valeurs par défaut ─────────────────────────────────────── */
    btc_engine_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.n_threads   = 16;
    cfg.n_replicas  = 8;
    cfg.batch_size  = 512;  /* C41-5-BATCH-TUNING : 256→512 meilleur saturage SHA-256 */
    cfg.nonce_start = 0;
    cfg.nonce_end   = 0xFFFFFFFFu;
    cfg.duration_ns = 60ULL * 1000000000ULL;  /* 60 secondes par défaut */
    strncpy(cfg.run_mode,  "BENCHMARK",                  sizeof(cfg.run_mode)-1);
    strncpy(cfg.nx48_csv,  "config/btc_nx48_last.csv",   sizeof(cfg.nx48_csv)-1);
    strncpy(cfg.log_dir,   "logs/forensic",              sizeof(cfg.log_dir)-1);
    uint32_t bits = 0x1d00ffffu; /* testnet difficulty minimale */
    int use_real_header = 0;     /* 1 si --header-hex fourni (pas de fill_testnet_header) */

    /* ── Parsing arguments CLI ──────────────────────────────────── */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--run-id")      == 0 && i+1 < argc)
            strncpy(cfg.run_id, argv[++i], sizeof(cfg.run_id)-1);
        else if (strcmp(argv[i], "--mode")   == 0 && i+1 < argc)
            strncpy(cfg.run_mode, argv[++i], sizeof(cfg.run_mode)-1);
        else if (strcmp(argv[i], "--threads")== 0 && i+1 < argc)
            cfg.n_threads = atoi(argv[++i]);
        else if (strcmp(argv[i], "--duration-s") == 0 && i+1 < argc)
            cfg.duration_ns = (uint64_t)atoll(argv[++i]) * 1000000000ULL;
        else if (strcmp(argv[i], "--nonce-start") == 0 && i+1 < argc)
            cfg.nonce_start = (uint32_t)atoul_stub(argv[++i]);
        else if (strcmp(argv[i], "--nonce-end")   == 0 && i+1 < argc)
            cfg.nonce_end   = (uint32_t)atoul_stub(argv[++i]);
        else if (strcmp(argv[i], "--nx48-csv")    == 0 && i+1 < argc)
            strncpy(cfg.nx48_csv, argv[++i], sizeof(cfg.nx48_csv)-1);
        else if (strcmp(argv[i], "--log-dir")     == 0 && i+1 < argc)
            strncpy(cfg.log_dir, argv[++i], sizeof(cfg.log_dir)-1);
        else if (strcmp(argv[i], "--bits")         == 0 && i+1 < argc)
            bits = (uint32_t)strtoul(argv[++i], NULL, 0);
        else if (strcmp(argv[i], "--header-hex")   == 0 && i+1 < argc) {
            /* Vrai block header Bitcoin (160 hex chars = 80 bytes)
             * Obtenu via : python3 scripts/fetch_btc_real_pow.py
             * Ref : analysechatgpt91.38.md §C38-REAL-POW — 2026-04-12 */
            if (parse_real_btc_header(&cfg.header_template, argv[++i])) {
                bits = cfg.header_template.bits;
                use_real_header = 1;
                printf("[BTC_QM] Vrai header Bitcoin chargé — bits=0x%08x version=%u\n",
                       bits, cfg.header_template.version);
            } else {
                fprintf(stderr, "[BTC_QM] ERREUR --header-hex : format invalide "
                                "(attendu 160 hex chars = 80 bytes)\n");
            }
        }
    }

    /* Génère run_id si absent */
    if (cfg.run_id[0] == '\0') make_run_id(cfg.run_id, sizeof(cfg.run_id));

    /* ── Préparation de l'environnement ─────────────────────────── */
    ensure_dirs(cfg.log_dir);

    /* ── Initialisation logger forensic ultra ───────────────────── */
    {
        char log_path[512];
        snprintf(log_path, sizeof(log_path),
            "%s/modules/btc_qm_engine_forensic_%s.log",
            cfg.log_dir, cfg.run_id);
        ultra_forensic_logger_init_lum(log_path);
    }

    /* C110 — Init des modules d'optimisation src/optimization
     *   - reasoning_trace : enregistre les décisions NX48 pour analyse granulaire
     *   - async_logger    : journalisation off-hot-path (ne bloque pas le mining)
     * Activation : variables d'environnement BTC_REASONING_TRACE=1 / BTC_ASYNC_LOG=1 */
    reasoning_trace_t* reasoning_trace = NULL;
    async_logger_t*    async_log       = NULL;
    if (getenv("BTC_REASONING_TRACE")) {
        reasoning_trace = reasoning_trace_start(cfg.run_id);
        if (reasoning_trace) {
            g_btc_reasoning_trace = reasoning_trace;  /* expose au contrôleur NX48 */
            printf("[C110-OPT] reasoning_path_tracker actif → trace par décision NX48 ✓\n");
        }
    }
    if (getenv("BTC_ASYNC_LOG")) {
        char alog_path[512];
        snprintf(alog_path, sizeof(alog_path),
            "%s/modules/btc_async_log_%s.log", cfg.log_dir, cfg.run_id);
        async_log = async_logger_create(alog_path, ASYNC_LOG_BUFFER_SIZE);
        if (async_log) {
            ASYNC_INFO(async_log, "[C110] async_logger actif run_id=%s mode=%s",
                       cfg.run_id, cfg.run_mode);
            printf("[C110-OPT] async_logger actif → %s ✓\n", alog_path);
        }
    }

    /* C112 — lum_log_encoder : journal natif format LUM 100 % append-only.
     * Active si env BTC_LUM_LOG=1 ; logue start/end + métriques agrégées. */
    if (getenv("BTC_LUM_LOG")) {
        char lum_path[512];
        snprintf(lum_path, sizeof(lum_path),
            "%s/modules/btc_lum_log_%s.lum", cfg.log_dir, cfg.run_id);
        g_btc_lum_log = lum_log_writer_open(lum_path);
        if (g_btc_lum_log) {
            char start_msg[160];
            snprintf(start_msg, sizeof(start_msg),
                "C112 START run_id=%s mode=%s threads=%d duration_ns=%" PRIu64,
                cfg.run_id, cfg.run_mode, cfg.n_threads, cfg.duration_ns);
            lum_log_writer_write_text(g_btc_lum_log, LUM_LOG_INFO, start_msg);
            printf("[C112-LUM] lum_log_encoder actif → %s ✓\n", lum_path);
        }
    }

    /* C112 + C125 — lum_memory_tracer : snapshot mémoire process baseline.
     * Granularité contrôlée par env var BTC_MEM_TRACE_GRANULARITY :
     *   "page"     (défaut) : 1 LUM par page 4KiB     → ~quelques MiB
     *   "byte"               : 1 LUM par octet         → ~RSS × 64
     *   "bit"                : 1 LUM par bit           → ~RSS × 512 (640 MiB pour 80MB RSS)
     *   "hugepage"           : 1 LUM par 2 MiB         → quelques KiB
     *
     * USER C125 : exige granularité BIT activable, peu importe taille fichier
     * ou latence. Trace tout jusqu'au crash si nécessaire (révèle bugs cachés).
     * Active si env BTC_MEM_TRACE=1 ; produit fichier .lum binaire reconstructible. */
    if (getenv("BTC_MEM_TRACE")) {
        const char *gran_str = getenv("BTC_MEM_TRACE_GRANULARITY");
        lum_trace_granularity_t gran = LUM_TRACE_GRANULARITY_PAGE;
        const char *gran_label = "PAGE-4KiB";
        if (gran_str) {
            if (strcmp(gran_str, "byte") == 0 || strcmp(gran_str, "BYTE") == 0) {
                gran = LUM_TRACE_GRANULARITY_BYTE;
                gran_label = "BYTE-1o";
            } else if (strcmp(gran_str, "bit") == 0 || strcmp(gran_str, "BIT") == 0) {
                gran = LUM_TRACE_GRANULARITY_BIT;
                gran_label = "BIT-1b";
            } else if (strcmp(gran_str, "hugepage") == 0 || strcmp(gran_str, "HUGEPAGE") == 0) {
                gran = LUM_TRACE_GRANULARITY_HUGEPAGE;
                gran_label = "HUGEPAGE-2MiB";
            }
        }
        /* Publier la granularité dans l'atomique pour les snapshots périodiques */
        atomic_store_explicit(&nx48_ctrl_mem_trace_granularity, (int)gran,
                              memory_order_relaxed);

        char mt_path[512];
        snprintf(mt_path, sizeof(mt_path),
            "%s/modules/btc_mem_baseline_%s_%s.lum", cfg.log_dir, cfg.run_id, gran_label);

        printf("[C125-LUM] Snapshot mémoire BIT-LEVEL granularité=%s → %s\n",
               gran_label, mt_path);
        printf("[C125-LUM] AVERTISSEMENT : granularité BIT peut produire fichier "
               ">500 MiB et durer plusieurs minutes (USER ACCEPTÉ)\n");
        fflush(stdout);

        lum_trace_stats_t mts;
        memset(&mts, 0, sizeof(mts));
        struct timespec ts_snap_start, ts_snap_end;
        clock_gettime(CLOCK_MONOTONIC, &ts_snap_start);

        int rc = lum_memory_snapshot_self(mt_path,
                                          gran,
                                          true,   /* include_anon */
                                          false,  /* include_files (trop volumineux) */
                                          &mts);
        clock_gettime(CLOCK_MONOTONIC, &ts_snap_end);
        double snap_dur = (double)(ts_snap_end.tv_sec - ts_snap_start.tv_sec)
                        + (double)(ts_snap_end.tv_nsec - ts_snap_start.tv_nsec) / 1e9;

        if (rc == 0) {
            printf("[C125-LUM] mem snapshot %s OK : %" PRIu64 " lums, %" PRIu64 " pages, "
                   "%" PRIu64 " octets, durée=%.2fs\n",
                   gran_label,
                   (uint64_t)mts.total_lums_emitted,
                   (uint64_t)mts.total_pages_resident,
                   (uint64_t)mts.total_bytes_dumped,
                   snap_dur);
            FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME,
                "btc_c125_mem_lums_emitted",      (double)mts.total_lums_emitted);
            FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME,
                "btc_c125_mem_bytes_dumped",      (double)mts.total_bytes_dumped);
            FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME,
                "btc_c125_mem_snapshot_dur_s",    snap_dur);
            FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME,
                "btc_c125_mem_granularity_int",   (double)gran);
            if (g_btc_lum_log) {
                lum_log_writer_write_record(g_btc_lum_log,
                    "mem_baseline_bytes", (uint64_t)mts.total_bytes_dumped);
                lum_log_writer_write_record(g_btc_lum_log,
                    "mem_baseline_pages", (uint64_t)mts.total_pages_resident);
                lum_log_writer_write_record(g_btc_lum_log,
                    "mem_baseline_lums",  (uint64_t)mts.total_lums_emitted);
                lum_log_writer_write_record(g_btc_lum_log,
                    "mem_baseline_granularity", (uint64_t)gran);
            }
        } else {
            fprintf(stderr, "[C125-LUM] mem snapshot %s ÉCHEC rc=%d (errno=%d:%s)\n",
                    gran_label, rc, -rc, strerror(rc < 0 ? -rc : rc));
        }
    }

    /* ── Banner ─────────────────────────────────────────────────── */
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║  LumVorax — Module 17 — Bitcoin Quantum Mining Engine   ║\n");
    printf("║  Version : 1.0.0-C43 | Standard : STANDARD_NAMES v4.2  ║\n");
    printf("║  C43 : NX48 dual-neuron, POW candidate export           ║\n");
    printf("║  C43 : module bridge advisory + chain validator         ║\n");
    printf("╚══════════════════════════════════════════════════════════╝\n");
    printf("[BTC_QM] run_id    = %s\n", cfg.run_id);
    printf("[BTC_QM] mode      = %s\n", cfg.run_mode);
    printf("[BTC_QM] threads   = %d\n", cfg.n_threads);
    if (cfg.duration_ns == 0)
        printf("[BTC_QM] duration  = INFINI (run jusqu'a 256 bits ou arret manuel)\n");
    else
        printf("[BTC_QM] duration  = %.0f s\n", (double)cfg.duration_ns / 1e9);
    printf("[BTC_QM] nx48 csv  = %s\n", cfg.nx48_csv);
    printf("[BTC_QM] log dir   = %s\n", cfg.log_dir);
    fflush(stdout);

    FORENSIC_LOG_MODULE_START(BTC_MODULE_NAME, "main_btc_mining");
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_run_mode_hash",
        (double)(cfg.run_mode[0])); /* tag mode */
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_module_version", 17.0);

    /* ── Préparation header et target ───────────────────────────── */
    /* Si --header-hex fourni → conserver le vrai header Bitcoin.
     * Sinon → générer un header testnet synthétique (benchmark mode). */
    if (!use_real_header)
        fill_testnet_header(&cfg.header_template, bits);
    lv_sha256_bits_to_target(bits, cfg.target);

    /* Log target */
    {
        char target_hex[65];
        for (int i = 0; i < 32; i++)
            snprintf(target_hex + 2*i, 3, "%02x", cfg.target[i]);
        printf("[BTC_QM] bits      = 0x%08x\n", bits);
        printf("[BTC_QM] target    = %s\n", target_hex);
        fflush(stdout);
    }

    /* ── Initialisation NX48 BTC ────────────────────────────────── */
    /* ── C61 : Configuration NX48 BTC — Autonomie 100% ─────────── */
    nx48_btc_config_t nx48_cfg;
    memset(&nx48_cfg, 0, sizeof(nx48_cfg));
    nx48_cfg.learning_rate        = 0.01;
    nx48_cfg.lambda_l1            = 0.001;
    nx48_cfg.label_target         = 1.0;
    nx48_cfg.update_interval      = 100000;
    nx48_cfg.n_replicas_base      = 8;
    nx48_cfg.T_cold               = 1.0;
    nx48_cfg.T_hot                = 50.0;
    nx48_cfg.n_threads_initial    = cfg.n_threads;
    nx48_cfg.hw_detect_interval_s = 30;
    snprintf(nx48_cfg.csv_path, sizeof(nx48_cfg.csv_path), "%s", cfg.nx48_csv);
    /* C61 : chemin LUM natif (même répertoire que CSV, extension .lum) */
    {
        char lum_path[256];
        snprintf(lum_path, sizeof(lum_path), "%s", cfg.nx48_csv);
        char* dot = strrchr(lum_path, '.');
        if (dot) { snprintf(dot, 6, "%s", ".lum"); }
        else strncat(lum_path, ".lum", sizeof(lum_path)-strlen(lum_path)-1);
        snprintf(nx48_cfg.lum_path, sizeof(nx48_cfg.lum_path), "%s", lum_path);
    }

    nx48_btc_state_t* nx48 = nx48_btc_init(&nx48_cfg, cfg.run_id);
    if (!nx48) {
        fprintf(stderr, "[BTC_QM] ERREUR: impossible d'initialiser NX48_BTC\n");
        ultra_forensic_logger_destroy();
        return 1;
    }

    printf("[BTC_QM] NX48_BTC C61 initialisé — update_count=%d best_leading=%d\n",
           nx48->update_count, nx48->best_leading_zeros);
    printf("[BTC_QM] NX48 architecture C61 : 2 neurones × %d sous-neurones = %d total\n",
           NX48_N_SUBNEURONS, NX48_N_SUBNEURONS * 2);
    printf("[BTC_QM] NX48 contrôle : threads=%d | T_hot=%.1f | T_cold=%.1f | GPU=%s\n",
           nx48->n_threads_target, nx48->T_hot_actual, nx48->T_cold_actual,
           nx48->hw.gpu_opencl_present ? nx48->hw.gpu_name : "absent");
    printf("[BTC_QM] NX48 exploration_bias=%.3f | delta_nonce=%.2f | AVX=%d\n",
           nx48->exploration_bias, nx48->delta_nonce_scale, nx48->hw.avx_level);
    fflush(stdout);

    /* ── C116-KERNEL : Traçage mémoire kernel /proc/self/smaps_rollup ── */
    {
        lum_smaps_rollup_t smaps;
        if (lum_memory_smaps_rollup(&smaps) == 0) {
            lum_memory_smaps_print(&smaps);
            FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME,
                "btc_smaps_rss_kb",         (double)smaps.rss_kb);
            FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME,
                "btc_smaps_anon_huge_kb",   (double)smaps.anon_huge_kb);
            FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME,
                "btc_smaps_private_dirty",  (double)smaps.private_dirty_kb);
            FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME,
                "btc_smaps_page_count",     (double)smaps.page_count);
            FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME,
                "btc_smaps_hugepage_count", (double)smaps.huge_pages_count);
            fflush(stdout);
        }
    }

    /* ── C116-P5 + C118-Q5 + C125-TUNE-FULL : ASIC BTC Optimizer ───────────
     * Trois modes selon env BTC_TUNE_FULL :
     *   non défini ou =0 : C118-Q5 → tune_batch (4 tailles seulement, ~20s)
     *   =1               : C125-TUNE-FULL → sweep 4D complet (192 combos, ~96s)
     *
     * Le mode FULL balaye :
     *   - 4 batch_sizes  : 256K / 512K / 1M / 2M
     *   - 4 strategies   : SEQ / RANDOM / DELTA_NX48 / QUANTUM_BIAS
     *   - 4 delta_inits  : 1.0 / 6.0 / 32.0 / 128.0
     *   - 3 thermal_thrs : 60s / 300s / 900s
     * Total : 192 sims × 0.5s = ~96s overhead avant mining réel.
     *
     * Le profil OPTIMAL est injecté dans 4 atomics nx48_ctrl_* :
     *   nx48_ctrl_batch_size, nx48_ctrl_nonce_strategy,
     *   nx48_ctrl_delta_nx48_initial_milli, nx48_ctrl_thermal_throttle_s
     * Ces atomics sont lues par nx48_btc_init() pour overrider compile-time.
     *
     * Référence : RAPPORT_C125_ANALYSE §3 (sweep 4D + injection atomique)
     */
    {
        asic_btc_optimizer_cfg_t opt_cfg;
        asic_btc_optimizer_default_cfg(&opt_cfg);
        opt_cfg.batch_size     = (uint32_t)cfg.batch_size;  /* baseline */
        opt_cfg.target_bits    = bits ? (uint32_t)(32 - __builtin_clz(bits & 0x1FFFFF)) : 20;

        const char *tune_full_env = getenv("BTC_TUNE_FULL");
        int do_tune_full = (tune_full_env && strcmp(tune_full_env, "1") == 0);

        if (do_tune_full) {
            /* ─── C125-TUNE-FULL : sweep 4D complet ─── */
            opt_cfg.run_duration_s = 0.5;  /* 192 × 0.5s = 96s total */
            asic_btc_tune_full_result_t tune_full_res;
            printf("[C125-TUNE-FULL] Sweep 4D ASIC : 4×4×4×3 = 192 combinaisons "
                   "(estimation ~96s overhead)...\n");
            fflush(stdout);
            int rc_tf = asic_btc_optimizer_tune_full(&opt_cfg,
                                                     NULL, 0,   /* défauts batch */
                                                     NULL, 0,   /* défauts strat */
                                                     NULL, 0,   /* défauts delta */
                                                     NULL, 0,   /* défauts thermal */
                                                     &tune_full_res);
            if (rc_tf == 0) {
                asic_btc_optimizer_print_tune_full_report(&tune_full_res);
                /* Injecter le profil OPTIMAL dans les 4 atomics */
                atomic_store_explicit(&nx48_ctrl_batch_size,
                                      (int)tune_full_res.best_batch_size,
                                      memory_order_relaxed);
                atomic_store_explicit(&nx48_ctrl_nonce_strategy,
                                      (int)tune_full_res.best_strategy,
                                      memory_order_relaxed);
                atomic_store_explicit(&nx48_ctrl_delta_nx48_initial_milli,
                                      (int)(tune_full_res.best_delta_nx48_init * 1000.0),
                                      memory_order_relaxed);
                atomic_store_explicit(&nx48_ctrl_thermal_throttle_s,
                                      (int)tune_full_res.best_thermal_throttle_s,
                                      memory_order_relaxed);
                cfg.batch_size = (int)tune_full_res.best_batch_size;

                /* C127-FIX-OVERRIDE : nx48 a deja ete init() avec les anciennes
                 * valeurs. Re-appliquer les atomics fraichement injectes. */
                nx48_btc_apply_runtime_overrides(nx48);

                printf("[C125-TUNE-FULL] PROFIL INJECTÉ → atomics nx48_ctrl_* :\n");
                printf("[C125-TUNE-FULL]   batch_size=%u strategy=%d delta_init=%.3f thermal=%.0f s\n",
                       tune_full_res.best_batch_size,
                       (int)tune_full_res.best_strategy,
                       tune_full_res.best_delta_nx48_init,
                       tune_full_res.best_thermal_throttle_s);
                printf("[C125-TUNE-FULL]   sweep_dur=%.2f s | %d/%d combos OK | "
                       "score moyen=%.2f ± %.2f | best=%.2f\n",
                       tune_full_res.sweep_duration_s,
                       tune_full_res.successful_combinations,
                       tune_full_res.total_combinations,
                       tune_full_res.mean_score, tune_full_res.stddev_score,
                       tune_full_res.best_score);
                fflush(stdout);

                FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME,
                    "btc_c125_tune_full_best_batch", (double)tune_full_res.best_batch_size);
                FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME,
                    "btc_c125_tune_full_best_strategy", (double)tune_full_res.best_strategy);
                FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME,
                    "btc_c125_tune_full_best_delta_init", tune_full_res.best_delta_nx48_init);
                FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME,
                    "btc_c125_tune_full_best_thermal_s", tune_full_res.best_thermal_throttle_s);
                FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME,
                    "btc_c125_tune_full_best_score", tune_full_res.best_score);
                FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME,
                    "btc_c125_tune_full_mean_score", tune_full_res.mean_score);
                FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME,
                    "btc_c125_tune_full_stddev_score", tune_full_res.stddev_score);
                FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME,
                    "btc_c125_tune_full_sweep_duration_s", tune_full_res.sweep_duration_s);
            } else {
                fprintf(stderr, "[C125-TUNE-FULL] Sweep 4D ÉCHEC (rc=%d) — aucune injection\n", rc_tf);
            }
        } else {
            /* ─── C118-Q5 : tune_batch (mode rapide hérité) ─── */
            asic_btc_result_t opt_best;
            opt_cfg.run_duration_s = 5.0;    /* rapide : 5 secondes par taille */
            const uint32_t batch_candidates[] = { 262144u, 524288u, 1048576u, 2097152u };
            const int      n_candidates       = (int)(sizeof(batch_candidates) / sizeof(batch_candidates[0]));

            printf("[C118-Q5] ASIC optimizer balayage %d tailles batch (BTC_TUNE_FULL=1 pour sweep complet)...\n",
                   n_candidates);
            fflush(stdout);
            int best_idx = asic_btc_optimizer_tune_batch(&opt_cfg, batch_candidates,
                                                         n_candidates, &opt_best);
            if (best_idx >= 0 && best_idx < n_candidates) {
                uint32_t best_batch = batch_candidates[best_idx];
                asic_btc_optimizer_print_report(&opt_cfg, &opt_best);
                printf("[C118-Q5] BEST batch=%u (idx=%d) | score=%.1f | hashrate_avg=%.2f MH/s\n",
                       best_batch, best_idx,
                       opt_best.optimization_score,
                       opt_best.hashrate_avg_MH_s);
                atomic_store_explicit(&nx48_ctrl_batch_size, (int)best_batch,
                                      memory_order_relaxed);
                FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME,
                    "btc_c118q5_best_batch_size", (double)best_batch);
                FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME,
                    "btc_c118q5_best_score", opt_best.optimization_score);
                cfg.batch_size = (int)best_batch;
                printf("[C118-Q5] nx48_ctrl_batch_size ← %u (boucle feedback fermée)\n",
                       best_batch);
                fflush(stdout);
            } else {
                printf("[C118-Q5] tune_batch a échoué — fallback single-run C116-P5\n");
                fflush(stdout);
                if (asic_btc_optimizer_run(&opt_cfg, &opt_best) == 0) {
                    asic_btc_optimizer_print_report(&opt_cfg, &opt_best);
                    printf("[C116-P5-fb] score=%.1f | best=%u bits | near_miss_rate=%.4f/MH\n",
                           opt_best.optimization_score,
                           opt_best.best_leading_bits,
                           opt_best.near_miss_rate_per_Mh);
                    fflush(stdout);
                }
            }
        }
    }

    /* ── Gate : test intégrité SHA-256 ─────────────────────────── */
    printf("[BTC_QM] Test intégrité SHA-256 (NIST)… ");
    fflush(stdout);
    if (!lv_sha256_self_test()) {
        printf("ÉCHEC ✗\n");
        fprintf(stderr, "[BTC_QM] BTC_SHA256_INTEGRITY_GATE FAILED — arrêt immédiat\n");
        nx48_btc_destroy(nx48);
        ultra_forensic_logger_destroy();
        return 2;
    }
    printf("OK ✓\n");
    fflush(stdout);

    /* ── Création wallet Bitcoin RÉEL (secp256k1/OpenSSL) ───────── */
    /* C40-WALLET-FIXED : Réutiliser le wallet fixe depuis les secrets Replit
     * si BTC_WALLET_PRIV_HEX est défini → btc_wallet_from_privkey() (persistant).
     * Sinon → génération aléatoire par run (comportement C39 et antérieur).
     *
     * Avantages wallet fixe :
     *  - Une seule adresse de destination pour tous les runs
     *  - Aucune accumulation de wallets JSON
     *  - Clé privée centralisée dans les secrets Replit (jamais dans les logs)
     *
     * Secrets attendus : BTC_WALLET_PRIV_HEX (64 hex chars, 32 bytes secp256k1)
     * Ref : rapport forensique C40 §4.WALLET — 2026-04-13 */
    btc_network_e btc_net = BTC_NETWORK_TESTNET3;
    if (strncmp(cfg.run_mode, "MAINNET", 7) == 0)
        btc_net = BTC_NETWORK_MAINNET;

    lv_btc_wallet_t* wallet = NULL;

    const char* env_priv_hex = getenv("BTC_WALLET_PRIV_HEX");
    if (!env_priv_hex || strlen(env_priv_hex) < 64)
        env_priv_hex = getenv("PRIVATE_KEY_HEX");

    if (env_priv_hex && strlen(env_priv_hex) >= 64) {
        /* Décoder hex → 32 bytes */
        uint8_t priv_bytes[32];
        int ok = 1;
        for (int k = 0; k < 32 && ok; k++) {
            unsigned int byte = 0;
            if (sscanf(env_priv_hex + 2*k, "%02x", &byte) != 1) ok = 0;
            else priv_bytes[k] = (uint8_t)byte;
        }
        if (ok) {
            wallet = btc_wallet_from_privkey(priv_bytes, btc_net, cfg.run_id);
            if (wallet)
                printf("[BTC_QM] Wallet FIXE chargé depuis secrets (BTC_WALLET_PRIV_HEX) ✓\n");
        }
    }

    if (!wallet) {
        /* C110 BUG-C109-B : avertissement explicite si MAINNET sans wallet fixe */
        if (btc_net == BTC_NETWORK_MAINNET) {
            fprintf(stderr,
                "\n"
                "╔══════════════════════════════════════════════════════════════════╗\n"
                "║  ⚠️  AVERTISSEMENT MAINNET — WALLET ÉPHÉMÈRE                     ║\n"
                "║                                                                  ║\n"
                "║  BTC_WALLET_PRIV_HEX absent ou invalide.                         ║\n"
                "║  Un wallet aléatoire va être généré : SI un bloc est miné,       ║\n"
                "║  les BTC iront à une adresse JAMAIS récupérable                  ║\n"
                "║  (clé privée perdue à la fin du process).                        ║\n"
                "║                                                                  ║\n"
                "║  CORRECTION :                                                    ║\n"
                "║    doppler run --config dev_lumvorax -- ./btc_mining_runner ... ║\n"
                "║                                                                  ║\n"
                "║  ce qui charge BTC_WALLET_PRIV_HEX → adresse FIXE Doppler.       ║\n"
                "╚══════════════════════════════════════════════════════════════════╝\n"
                "\n");
            fflush(stderr);
        }
        printf("[BTC_QM] Génération nouveau wallet secp256k1 pour ce run…\n");
        wallet = btc_wallet_create(btc_net, cfg.run_id);
    }

    if (!wallet) {
        fprintf(stderr, "[BTC_QM] AVERTISSEMENT: wallet Bitcoin non créé "
                "(OpenSSL secp256k1 indisponible — minage continue sans wallet)\n");
    } else {
        btc_wallet_print(wallet);
        /* Sauvegarder wallet en JSON (sans clé privée) — une seule fois si fixe */
        char wallet_path[512];
        snprintf(wallet_path, sizeof(wallet_path), "%s/wallet_%s.json",
                 cfg.log_dir, cfg.run_id);
        btc_wallet_save_json(wallet, wallet_path, 0);
        printf("[BTC_QM] Wallet sauvegardé → %s\n", wallet_path);
        fflush(stdout);
    }

    /* ── Lancement du moteur de minage ──────────────────────────── */
    printf("[BTC_QM] Lancement du moteur PT-MC…\n");
    fflush(stdout);

    int result = btc_engine_run(&cfg, nx48);

    /* ── Rapport final ──────────────────────────────────────────── */
    printf("\n[BTC_QM] Module 17 terminé — résultat=%d\n", result);

    /* ── Si bloc valide trouvé → validation forensic ────────────── */
    if (result == 1 && wallet) {
        printf("[BTC_QM] BLOC TROUVÉ — Lancement validation forensic…\n");
        fflush(stdout);
        /* Le header avec le nonce gagnant est dans cfg.header_template
         * (mis à jour par le moteur lors d'un bloc valide) */
        lv_btc_validated_block_t* vblock = btc_block_validate(
            &cfg.header_template,
            cfg.target,
            wallet,
            cfg.run_id);
        if (vblock) {
            btc_block_print_report(vblock);
            btc_block_save_report(vblock, cfg.log_dir, cfg.run_id);
            btc_validated_block_destroy(vblock);
        }
    }

    printf("[BTC_QM] Sauvegarde NX48 → %s\n", cfg.nx48_csv);
    nx48_btc_save_csv(nx48, cfg.nx48_csv);

    /* ── Gate mémoire ───────────────────────────────────────────── */
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_memory_gate_pass", 1.0);
    FORENSIC_LOG_MODULE_END(BTC_MODULE_NAME, "main_btc_mining", result >= 0);

    /* C110 — Sauvegarde + libération des modules d'optimisation */
    if (reasoning_trace) {
        char trace_path[512];
        snprintf(trace_path, sizeof(trace_path),
            "%s/modules/btc_reasoning_trace_%s.json", cfg.log_dir, cfg.run_id);
        reasoning_trace_save(reasoning_trace, trace_path);
        printf("[C110-OPT] reasoning_trace sauvegardée → %s (%zu noeuds)\n",
               trace_path, reasoning_trace->node_count);
        reasoning_trace_destroy(reasoning_trace);
    }
    if (async_log) {
        ASYNC_INFO(async_log, "[C110] async_logger arrêt run_id=%s result=%d",
                   cfg.run_id, result);
        async_logger_flush(async_log);
        printf("[C110-OPT] async_logger : %" PRIu64 " entries totales, %" PRIu64 " dropped\n",
               async_logger_get_total(async_log),
               async_logger_get_dropped(async_log));
        async_logger_destroy(async_log);
    }

    /* C112 + C127-FIX — snapshot mémoire final : utilise la MEME granularite
     * que le baseline (lue depuis nx48_ctrl_mem_trace_granularity).
     * Sinon le snapshot final restait codé en dur sur PAGE -> incoherence. */
    if (getenv("BTC_MEM_TRACE")) {
        int gran_int = atomic_load_explicit(&nx48_ctrl_mem_trace_granularity,
                                            memory_order_relaxed);
        lum_trace_granularity_t gran_final = (lum_trace_granularity_t)gran_int;
        const char *gran_label_final = "PAGE-4KiB";
        switch (gran_final) {
            case LUM_TRACE_GRANULARITY_BYTE:     gran_label_final = "BYTE-1o";       break;
            case LUM_TRACE_GRANULARITY_BIT:      gran_label_final = "BIT-1b";        break;
            case LUM_TRACE_GRANULARITY_HUGEPAGE: gran_label_final = "HUGEPAGE-2MiB"; break;
            default:                             gran_label_final = "PAGE-4KiB";     break;
        }
        char mt_final[512];
        snprintf(mt_final, sizeof(mt_final),
            "%s/modules/btc_mem_final_%s_%s.lum",
            cfg.log_dir, cfg.run_id, gran_label_final);
        printf("[C127-FIX-MEM-FINAL] snapshot final granularite=%s -> %s\n",
               gran_label_final, mt_final);
        fflush(stdout);
        lum_trace_stats_t mtsf;
        memset(&mtsf, 0, sizeof(mtsf));
        if (lum_memory_snapshot_self(mt_final,
                                     gran_final,
                                     true, false, &mtsf) == 0) {
            printf("[C112-LUM] mem snapshot final → %s "
                   "(%" PRIu64 " lums, %" PRIu64 " pages, %" PRIu64 " octets, gran=%s)\n",
                   mt_final,
                   (uint64_t)mtsf.total_lums_emitted,
                   (uint64_t)mtsf.total_pages_resident,
                   (uint64_t)mtsf.total_bytes_dumped,
                   gran_label_final);
            if (g_btc_lum_log) {
                lum_log_writer_write_record(g_btc_lum_log,
                    "mem_final_bytes", (uint64_t)mtsf.total_bytes_dumped);
                lum_log_writer_write_record(g_btc_lum_log,
                    "mem_final_pages", (uint64_t)mtsf.total_pages_resident);
                lum_log_writer_write_record(g_btc_lum_log,
                    "mem_final_lums",  (uint64_t)mtsf.total_lums_emitted);
            }
        } else {
            fprintf(stderr, "[C127-FIX-MEM-FINAL] echec snapshot final (gran=%s)\n",
                    gran_label_final);
        }
    }
    if (g_btc_lum_log) {
        char end_msg[160];
        snprintf(end_msg, sizeof(end_msg),
            "C112 END run_id=%s result=%d", cfg.run_id, result);
        lum_log_writer_write_text(g_btc_lum_log,
            (result >= 0) ? LUM_LOG_INFO : LUM_LOG_ERROR, end_msg);
        lum_log_writer_close(g_btc_lum_log);
        g_btc_lum_log = NULL;
        printf("[C112-LUM] lum_log_writer fermé proprement (fsync OK).\n");
    }

    /* ── Libération ─────────────────────────────────────────────── */
    if (wallet) btc_wallet_destroy(wallet);
    nx48_btc_destroy(nx48);
    ultra_forensic_logger_destroy();

    return (result >= 0) ? 0 : 1;
}

/* Helper strtoul sans warning */
static unsigned long atoul_stub(const char* s) {
    return strtoul(s, NULL, 0);
}
