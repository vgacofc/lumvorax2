/*
 * LumVorax — Module 18 — QDAYPRIZE Quantum Attack Simulator
 * main_qdayprize.c — Point d'entrée standalone
 *
 * STANDARD_NAMES.md v4.3 §M-QDPR18-C55 — Cycle C55 — 2026-04-17
 *
 * Usage :
 *   ./qdayprize_runner [--sim-bits N] [--duration S] [--threads T] [--run-id ID]
 *   ./qdayprize_runner --sim-bits 20 --duration 300 --run-id qdpr_c55_20260417
 *
 * En parallèle du BTC mining (depuis tools/qdayprize_run.sh) :
 *   bash tools/qdayprize_run.sh &
 *   bash tools/btc_run_ubuntu.sh
 */

#include "qdayprize_engine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <pthread.h>

/* ── Banner ──────────────────────────────────────────────────────── */
static void print_banner(const qdpr_config_t* cfg) {
    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║  LumVorax — Module 18 — QDAYPRIZE Quantum Attack        ║\n");
    printf("║  Version : %s | Cycle : %s              ║\n", QDPR_VERSION, QDPR_CYCLE);
    printf("║  Simulation classique d'attaque quantique ECDLP         ║\n");
    printf("║  Cible : secp256k1 (Bitcoin) — 6 924 807 BTC à risque  ║\n");
    printf("╚══════════════════════════════════════════════════════════╝\n");
    printf("[QDPR] run_id    = %s\n", cfg->run_id);
    printf("[QDPR] sim_bits  = %d bits\n", cfg->sim_bits);
    printf("[QDPR] duration  = %s\n", cfg->duration_s == 0 ? "illimitée" : "limitée");
    printf("[QDPR] threads   = %d\n", cfg->n_threads);
    printf("[QDPR] log_dir   = %s\n", cfg->log_dir);
    printf("[QDPR] THÉORIE : secp256k1 nécessite %d qubits logiques [Roetteler 2017]\n",
           QDPR_QUBITS_FORMULA(SECP256K1_BITS));
    printf("[QDPR] THÉORIE : Surface Code d=%d → %d qubits physiques\n",
           QDPR_QEC_DISTANCE,
           QDPR_QUBITS_FORMULA(SECP256K1_BITS) * (2 * QDPR_QEC_DISTANCE * QDPR_QEC_DISTANCE + 1));
    printf("[QDPR] SIMULATION : %d bits → validation oracle BSGS + QFT classique\n", cfg->sim_bits);
    printf("[QDPR] ============================================\n");
}

/* ── Parsing arguments ligne de commande ─────────────────────────── */
static void parse_args(int argc, char** argv, qdpr_config_t* cfg) {
    /* Valeurs par défaut */
    cfg->sim_bits    = QDPR_SIM_BITS_DEFAULT;
    cfg->n_threads   = 1;
    cfg->duration_s  = 0;
    cfg->verbose     = 1;
    cfg->parallel_btc = 0;
    snprintf(cfg->log_dir, sizeof(cfg->log_dir)-1, "logs/forensic");

    /* Générer run_id */
    time_t t = time(NULL);
    struct tm* tm_info = gmtime(&t);
    snprintf(cfg->run_id, sizeof(cfg->run_id)-1,
             "qdpr_c55_%04d%02d%02dT%02d%02d%02dZ",
             tm_info->tm_year+1900, tm_info->tm_mon+1, tm_info->tm_mday,
             tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--sim-bits") == 0 && i+1 < argc)
            cfg->sim_bits = atoi(argv[++i]);
        else if (strcmp(argv[i], "--duration") == 0 && i+1 < argc)
            cfg->duration_s = (uint64_t)atoi(argv[++i]);
        else if (strcmp(argv[i], "--threads") == 0 && i+1 < argc)
            cfg->n_threads = atoi(argv[++i]);
        else if (strcmp(argv[i], "--run-id") == 0 && i+1 < argc)
            snprintf(cfg->run_id, sizeof(cfg->run_id)-1, "%s", argv[++i]);
        else if (strcmp(argv[i], "--log-dir") == 0 && i+1 < argc)
            snprintf(cfg->log_dir, sizeof(cfg->log_dir)-1, "%s", argv[++i]);
        else if (strcmp(argv[i], "--parallel-btc") == 0)
            cfg->parallel_btc = 1;
        else if (strcmp(argv[i], "--verbose") == 0)
            cfg->verbose = 2;
        else if (strcmp(argv[i], "--quiet") == 0)
            cfg->verbose = 0;
    }

    /* Validation */
    if (cfg->sim_bits < QDPR_SIM_BITS_MIN) cfg->sim_bits = QDPR_SIM_BITS_MIN;
    if (cfg->sim_bits > QDPR_SIM_BITS_MAX) cfg->sim_bits = QDPR_SIM_BITS_MAX;
    if (cfg->n_threads  < 1)   cfg->n_threads = 1;
    if (cfg->n_threads  > 32)  cfg->n_threads = 32;
}

/* ── Lancement multi-thread ─────────────────────────────────────── */
int main(int argc, char** argv) {
    qdpr_config_t cfg;
    parse_args(argc, argv, &cfg);
    print_banner(&cfg);

    /* Vérifier/créer répertoire log */
    char mkdir_cmd[512];
    snprintf(mkdir_cmd, sizeof(mkdir_cmd)-1, "mkdir -p %s", cfg.log_dir);
    system(mkdir_cmd);

    if (cfg.n_threads == 1) {
        /* Mode simple thread (plus facile à déboguer) */
        qdpr_run_thread(&cfg);
    } else {
        /* Mode multi-thread : chaque thread attaque un sim_bits différent */
        pthread_t* threads = (pthread_t*)malloc(cfg.n_threads * sizeof(pthread_t));
        qdpr_config_t* cfgs = (qdpr_config_t*)malloc(cfg.n_threads * sizeof(qdpr_config_t));

        for (int i = 0; i < cfg.n_threads; i++) {
            cfgs[i] = cfg;
            cfgs[i].sim_bits = QDPR_SIM_BITS_MIN + (i * 4) % (QDPR_SIM_BITS_MAX - QDPR_SIM_BITS_MIN);
            snprintf(cfgs[i].run_id, sizeof(cfgs[i].run_id)-1,
                     "%s_t%d", cfg.run_id, i);
            cfgs[i].n_threads = 1;  /* Chaque thread est mono */
            pthread_create(&threads[i], NULL, qdpr_run_thread, &cfgs[i]);
        }

        for (int i = 0; i < cfg.n_threads; i++)
            pthread_join(threads[i], NULL);

        free(threads);
        free(cfgs);
    }

    printf("[QDPR] QDAYPRIZE terminé.\n");
    return 0;
}
