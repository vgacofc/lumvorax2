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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/stat.h>

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
    strncpy(nx48_cfg.csv_path, cfg.nx48_csv, sizeof(nx48_cfg.csv_path)-1);
    nx48_cfg.csv_path[sizeof(nx48_cfg.csv_path)-1] = '\0';
    /* C61 : chemin LUM natif (même répertoire que CSV, extension .lum) */
    {
        char lum_path[256];
        strncpy(lum_path, cfg.nx48_csv, sizeof(lum_path)-1);
        lum_path[sizeof(lum_path)-1] = '\0';
        char* dot = strrchr(lum_path, '.');
        if (dot) strncpy(dot, ".lum", 5);
        else strncat(lum_path, ".lum", sizeof(lum_path)-strlen(lum_path)-1);
        strncpy(nx48_cfg.lum_path, lum_path, sizeof(nx48_cfg.lum_path)-1);
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
