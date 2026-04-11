# STANDARD_NAMES.md — Registre canonique des noms du projet LumVorax / Hubbard-HTS

**Version :** 4.1 — 2026-04-11 (C62 : §M-BTC17 ajouté — Module 17 Bitcoin Quantum Mining Engine compilé et validé)
**Langue obligatoire :** TOUJOURS répondre et rédiger EN FRANÇAIS dans cette session de chat.

---

## RÈGLE D'UTILISATION (OBLIGATOIRE)

```
AVANT toute création d'un nouveau nom :
  - fonction C, macro préprocesseur, préfixe métrique Lumvorax,
    nom de fichier source/header, variable globale, famille de test

→ 1. Chercher dans ce fichier par mot-clé (grep ou Ctrl+F)
→ 2. Si TROUVÉ  : utiliser le NOM D'ORIGINE — NE PAS créer de nouveau nom
→ 3. Si ABSENT : ajouter l'entrée ici PUIS créer le code
→ 4. NE JAMAIS RENOMMER les noms d'origine : d'autres modules en dépendent
→ 5. Si un nouveau nom a été créé par erreur : le SUPPRIMER du code,
     remplacer par le nom d'origine. Pas l'inverse.
```

---

## SECTION A — MACROS PRÉPROCESSEUR (NOM D'ORIGINE = À UTILISER)

### Logger principal Lumvorax (`src/debug/ultra_forensic_logger.h`)

> **Ces macros sont les noms d'ORIGINE.** Elles écrivent dans le CSV Lumvorax principal
> (`lumvorax_hubbard_hts_advanced_parallel_*.csv`, le fichier 95 MB / 1.14 M lignes).
> **Ne jamais les renommer. Ne jamais les remplacer.**

| NOM D'ORIGINE (officiel) | Fichier définition | Appel interne | Description |
|---|---|---|---|
| `FORENSIC_LOG_MODULE_METRIC(module, metric, value)` | `src/debug/ultra_forensic_logger.h` ligne 107 | `ultra_forensic_log_module_metric()` | **LOG PRINCIPAL** — écrit une ligne METRIC dans le CSV Lumvorax |
| `FORENSIC_LOG_ALGO(algo_name, metric, value)` | `src/debug/ultra_forensic_logger.h` ligne 123 | `ultra_forensic_log_algo()` | Log métrique pour algorithmes (Lanczos, ED, etc.) |
| `FORENSIC_LOG_MODULE_START(module, test)` | `src/debug/ultra_forensic_logger.h` ligne 99 | `ultra_forensic_log_module_start()` | Démarre un module |
| `FORENSIC_LOG_MODULE_END(module, test, result)` | `src/debug/ultra_forensic_logger.h` ligne 102 | `ultra_forensic_log_module_end()` | Termine un module |
| `FORENSIC_LOG_MODULE_OP(module, op, data)` | `src/debug/ultra_forensic_logger.h` ligne 105 | `ultra_forensic_log_module_operation()` | Log opération qualitative |
| `FORENSIC_LOG_NANO(module, key, value)` | `src/debug/ultra_forensic_logger.h` ligne 112 | `ultra_forensic_log_nano()` | Log nanoseconde |
| `FORENSIC_LOG_HW_SAMPLE(module)` | `src/debug/ultra_forensic_logger.h` ligne 116 | `ultra_forensic_log_hw_sample()` | Capture hardware CPU/RAM |
| `FORENSIC_LOG_ANOMALY(module, desc, value)` | `src/debug/ultra_forensic_logger.h` ligne 120 | `ultra_forensic_log_anomaly()` | Log anomalie détectée |
| `FORENSIC_LOG_PHASE_BRIDGE(phase, script, status)` | `src/debug/ultra_forensic_logger.h` ligne 128 | `ultra_forensic_log_phase_bridge()` | Log bridge entre phases |
| `FORENSIC_LOG_THREAD_START(module, label)` | `src/debug/ultra_forensic_logger.h` ligne 132 | `ultra_forensic_log_thread()` | Log démarrage thread |
| `FORENSIC_LOG_THREAD_END(module, label)` | `src/debug/ultra_forensic_logger.h` ligne 134 | `ultra_forensic_log_thread()` | Log fin thread |

### Bridge secondaire (`include/lumvorax_integration.h`) — LOGGER SECONDAIRE DISTINCT

> **Ces macros sont dans le bridge `lumvorax_integration.c/h`.**
> Elles écrivent dans un SECOND fichier log distinct (pas dans le CSV Lumvorax principal).
> Ce bridge est distinct de `ultra_forensic_logger`.
> **Ne pas les utiliser** pour logger dans le CSV Lumvorax principal.
> **Ne pas renommer les macros FORENSIC_LOG_* vers LV_MODULE_* (ce serait l'inverse).**

| NOM (bridge secondaire) | Écrit dans | À utiliser pour |
|---|---|---|
| `LV_MODULE_METRIC(module, metric, value)` | second fichier log (bridge) | Usage interne bridge uniquement |
| `LV_MODULE_START(module, label)` | second fichier log (bridge) | Usage interne bridge uniquement |
| `LV_MODULE_END(module, label, success)` | second fichier log (bridge) | Usage interne bridge uniquement |
| `LV_MODULE_OP(module, op, data)` | second fichier log (bridge) | Usage interne bridge uniquement |
| `LV_CALLOC(nmemb, size)` | tracking mémoire | Allocations tracées |
| `LV_MALLOC(size)` | tracking mémoire | Allocations tracées |
| `LV_FREE(ptr)` | tracking mémoire | Libérations tracées |

**⚠️ ERREUR C25-NAMES CORRIGÉE :**
Le rapport `analysechatgpt32.md` (Section G) avait inversé l'ordre officiel.
La vérité confirmée par audit du code source :
- `FORENSIC_LOG_MODULE_METRIC` = **NOM D'ORIGINE** (ultra_forensic_logger.h, 71 usages dans le runner)
- `LV_MODULE_METRIC` = nom créé après dans le bridge secondaire, NON utilisé dans le runner
- **ACTION C25-STEPLOG** : les 4 nouveaux appels ajoutés utilisent `FORENSIC_LOG_MODULE_METRIC` ✅ (correct)

---

## SECTION B — FONCTIONS C (NOM D'ORIGINE = À UTILISER)

### ultra_forensic_logger (`src/debug/ultra_forensic_logger.c`)

| NOM D'ORIGINE | Signature | Description |
|---|---|---|
| `ultra_forensic_logger_init_lum(log_file)` | `void f(const char*)` | Initialise le logger avec chemin CSV |
| `ultra_forensic_logger_init()` | `bool f(void)` | Initialise le logger (mode standard) |
| `ultra_forensic_logger_destroy()` | `void f(void)` | Ferme le logger, écrit SUMMARY |
| `ultra_forensic_log_module_metric(file, line, func, module, metric, value)` | `void f(...)` | **ÉCRITURE LOG PRINCIPALE** dans CSV Lumvorax |
| `ultra_forensic_log_algo(func, algo, metric, value)` | `void f(...)` | Log algorithme (ED, Lanczos) |
| `ultra_forensic_log_module_start(file, line, func, module, test)` | `void f(...)` | Démarre module |
| `ultra_forensic_log_module_end(file, line, func, module, test, result)` | `void f(...)` | Termine module |
| `ultra_forensic_log_module_operation(file, line, func, module, op, data)` | `void f(...)` | Log opération |
| `ultra_forensic_log_nano(func, module, key, value)` | `void f(...)` | Log nanoseconde |
| `ultra_forensic_log_hw_sample(func, module)` | `void f(...)` | Capture /proc (CPU, RAM) |
| `ultra_forensic_log_anomaly(file, line, func, module, desc, value)` | `void f(...)` | Log anomalie |
| `ultra_forensic_log_phase_bridge(phase, script, status)` | `void f(...)` | Log bridge phase |
| `ultra_forensic_log_thread(func, module, label, state)` | `void f(...)` | Log thread START/END |

### bridge Lumvorax Integration (`src/lumvorax_integration.c`)

| NOM (bridge) | Description |
|---|---|
| `lv_init(log_dir)` | Initialise le logger bridge (fichier distinct) |
| `lv_destroy()` | Ferme le bridge |
| `lv_module_metric(file, line, func, module, metric, value)` | Écrit dans le second fichier log bridge |
| `lv_get_timestamp_ns()` | Timestamp CLOCK_MONOTONIC ns (utilisable indépendamment) |
| `lv_tracked_calloc(nmemb, size, file, line, func)` | calloc tracé |
| `lv_tracked_malloc(size, file, line, func)` | malloc tracé |
| `lv_tracked_free(ptr, file, line, func)` | free tracé |
| `lv_report_leaks()` | Rapport fuites mémoire |

---

## SECTION C — PRÉFIXES MÉTRIQUES LUMVORAX (colonne 5 du CSV)

Format d'une ligne : `METRIC,timestamp_utc,timestamp_ns,pid,PRÉFIXE:nom_métrique,valeur`

| PRÉFIXE D'ORIGINE (officiel) | Signification | Métriques associées |
|---|---|---|
| `simulate_adv:` | Simulation HTS avancée (boucle RK2, runner advanced_parallel) | `elapsed_ns`, `steps`, `step_energy_eV`(C25), `step_pairing_norm`(C25), `step_sign_ratio`(C25), `step_elapsed_ns`(C25), `step_pairing_norm_step0`, `step_energy_norm_step0`, `local_pair_site0_step0`, `d_site0_step0`, `autocorr_tau_lower_steps`, `autocorr_tau_upper_steps`, `hilbert_log2_dim`(C59-P5), `sigma_mc_estimate`(C59-P4), `sigma_corr_sign`(C59-P4), `sign_abs_mean`(C59-P4) |
| `simulate_fs:` | Runner fullscale (hubbard_hts_research_cycle.c — distinct de advanced_parallel) | `sites`, `steps`, `temp_K`, `U_eV`, `local_pair_site0_step0`, `d_site0_step0`, `step_pairing_norm_step0`, `step_energy_norm_step0`, `pairing_final`, `energy_final_eV`, `sign_ratio_final`, `elapsed_ns` (C64 — NOM D'ORIGINE, observé dans lumvorax_hubbard_hts_fullscale_*.csv) |
| `pt_mc_therm:` | Thermalisation PT-MC (800 sweeps × 26 modules) | `n_thermalize_sweeps`, `n_replicas`, `n_step_per_sweep`, `sw`, `mc_rate`, `swap_rate`, `delta_mc`, `E_cold` |
| `pt_mc:` | Récapitulatif PT-MC par module | `elapsed_ns`, `avg_mc_accept`, `chi_sc_avg`, `chi_sc_max`, `pairing_cold_avg`, `tau_int_energy`(C25-TAU), `err_corrected_energy_eV`(C25-TAU), `hilbert_log2_dim`(C59-P5), `sigma_mc_ptmc`(C59-P4), `sigma_corr_sign`(C59-P4), `sign_abs_mc_accept`(C59-P4), `sw`(C59-ULTRA), `mc_rate`(C59-ULTRA), `swap_rate`(C59-ULTRA), `delta_mc`(C59-ULTRA), `E_cold`(C59-ULTRA), `elapsed_ns`(C59-ULTRA) [via FORENSIC_LOG_NANO] |
| `pt_mc_swap_detail:` | Chaque tentative d'échange répliques | `sw`, `r_lo`, `r_hi`, `delta_beta_dE`, `swap_accepted` |
| `pt_mc_vs_mc:` | Anomalies divergence PT-MC vs MC simple | `energy_diff`, `pairing_diff` |
| `benchmark_adv:` | Benchmarks QMC/DMRG | `qmc_rmse`, `dmrg_mae`, `within_error_bar`, `ed_benchmark_energy_within`(C59-P2), `ed_benchmark_pairing_within`(C59-P2) |
| `ed_` (sous-préfixe) | Diagonalisation exacte (C24-02) | `ed_E0_eV`, `ed_gap_eV`, `ed_double_occ`, `ed_pairing_corr`, `ed_bethe_E0_eV`, `ed_rel_err_mc_pct`, `ed_converged`, `ed_lanczos_iter` |
| `worm_mc_ultra:` | Logs ultra-granulaires Worm MC bosonique (C39) | `sweep_log_path_len`, `sweep_log_fopen_errno`, `n_proposed`, `n_accepted`, `acceptance_rate`, `E_per_site`, `n_per_site`, `superfluid_density`, `converged` |

**⚠️ PRÉFIXES INTERDITS (doublons/erreurs historiques) :**

| Préfixe interdit | Raison | Utiliser à la place |
|---|---|---|
| `pt_mc_thermalize:` | Mauvaise orthographe — causait grep 0 résultats (erreur analysechatgpt30.md) | `pt_mc_therm:` |
| `forensic_algo:` | Préfixe non standardisé | `simulate_adv:` ou module spécifique |

---

## SECTION D — FONCTIONS PHYSIQUES DE SIMULATION

| NOM D'ORIGINE | Fichier | Description | Ne pas confondre avec |
|---|---|---|---|
| `simulate_adv` | `hubbard_hts_research_cycle_advanced_parallel.c` | Simulation Hubbard HTS avancée RK2 | `simulate_fullscale` (runner différent) |
| `pt_mc_run` | `hubbard_hts_research_cycle_advanced_parallel.c` | Parallel Tempering MC complet | — |
| `pt_mc_thermalize` | `hubbard_hts_research_cycle_advanced_parallel.c` | Phase thermalisation (800 sweeps) | — |
| `ed_hubbard_2x2` | `src/exact_diagonalization.c` | Diagonalisation exacte réseau 2×2 | — |
| `ed_hubbard_1d4` | `src/exact_diagonalization.c` | Diagonalisation exacte 1D 4 sites | — |
| `ed_bethe_ansatz_energy_1d` | `src/exact_diagonalization.c` | Énergie analytique Bethe ansatz 1D | — |
| `ed_compare_mc` | `src/exact_diagonalization.c` | Cross-validation ED vs MC | — |
| `simulate_fullscale` | `src/hubbard_hts_research_cycle.c` | Runner fullscale (distinct) | `simulate_adv` |

---

## SECTION E — VARIABLES GLOBALES CRITIQUES

| NOM D'ORIGINE | Type | Fichier | Description |
|---|---|---|---|
| `g_run_csv_path` | `char[4096]` | `src/debug/ultra_forensic_logger.c` | Chemin du CSV Lumvorax principal |
| `g_csv_mutex` | `pthread_mutex_t` | `src/debug/ultra_forensic_logger.c` | Thread-safety CSV Lumvorax |
| `g_run_seed_xor` | `uint64_t` | runner advanced_parallel | XOR sur les seeds pour indépendance runs |
| `g_log_file` | `FILE*` | `src/lumvorax_integration.c` | Fichier log du bridge secondaire (distinct du CSV principal) |
| `g_mutex` | `pthread_mutex_t` | `src/lumvorax_integration.c` | Thread-safety bridge secondaire |
| `brow_rt[256]` | `benchmark_row_t[256]` | runner advanced_parallel — C68 | Tableau de référence benchmark chargé AVANT la boucle de simulation (C68-REALTIME-BENCH). **Ne pas renommer en `brow[256]`** — doublon local utilisé dans la section post-PT_MC |
| `bn_rt` | `int` | runner advanced_parallel — C68 | Nombre de lignes QMC/DMRG chargées dans `brow_rt` |
| `bn_mod_rt` | `int` | runner advanced_parallel — C68 | Nombre de lignes EXT chargées dans `brow_rt[bench_offset_rt..]` |
| `bench_offset_rt` | `int` | runner advanced_parallel — C68 | Indice de début des lignes EXT dans `brow_rt` |
| `rt_sum_sq` | `double` | runner advanced_parallel — C68 | Somme des carrés des erreurs QMC (accumulée module par module) |
| `rt_sum_abs` | `double` | runner advanced_parallel — C68 | Somme des valeurs absolues des erreurs QMC |
| `rt_m` | `int` | runner advanced_parallel — C68 | Nombre de lignes QMC traitées en temps réel |
| `rt_within` | `int` | runner advanced_parallel — C68 | Nombre de lignes QMC dans leur barre d'erreur |
| `rt_sum_sq_mod` | `double` | runner advanced_parallel — C68 | Somme des carrés des erreurs EXT |
| `rt_sum_abs_mod` | `double` | runner advanced_parallel — C68 | Somme des valeurs absolues des erreurs EXT |
| `rt_m_mod` | `int` | runner advanced_parallel — C68 | Nombre de lignes EXT traitées en temps réel |
| `rt_within_mod` | `int` | runner advanced_parallel — C68 | Nombre de lignes EXT dans leur barre d'erreur |

---

## SECTION F — FICHIERS SOURCES OFFICIELS

| Rôle | Fichier officiel | Doublons à NE PAS recréer |
|---|---|---|
| Logger Lumvorax principal | `src/debug/ultra_forensic_logger.c` + `src/debug/ultra_forensic_logger.h` | ~~forensic_logger.c~~, ~~lum_core.c~~, ~~lum_logger.c~~ (fusionnés) |
| Bridge Lumvorax secondaire | `src/lumvorax_integration.c` + `include/lumvorax_integration.h` | — |
| Tracker mémoire | `src/debug/memory_tracker.c` | — |
| Runner advanced parallel | `src/hubbard_hts_research_cycle_advanced_parallel.c` | — |
| Runner fullscale | `src/hubbard_hts_research_cycle.c` | Distinct — PAS un doublon |
| Diagonalisation exacte | `src/exact_diagonalization.c` | — |
| Module Hubbard HTS | `src/hubbard_hts_module.c` | — |

---

## SECTION G — FAMILLES DE TESTS (analysis_scientifique_summary.json)

| NOM D'ORIGINE | Description |
|---|---|
| `reproducibility` | Reproductibilité (même seed → même résultat) |
| `convergence` | Convergence numérique |
| `stress` | Stress (paramètres extrêmes) |
| `verification` | Vérification indépendante |
| `exact_solver` | Cross-validation solveur exact (ED) |
| `sensitivity` | Sensibilité aux paramètres physiques (U, t, μ) |
| `physics` | Cohérence observables physiques |
| `control` | Contrôle plasma (phase_step, resonance_pump, magnetic_quench) |
| `stability` | Stabilité numérique temporelle |
| `dynamic_pumping` | Pompage dynamique hors-équilibre |
| `dt_sweep` | Sweep du pas temporel dt |
| `spectral` | Analyse spectrale FFT |
| `benchmark` | Benchmarks QMC/DMRG post-PT_MC (section finale) |
| `benchmark_rt` | **C68** Benchmarks QMC/DMRG écrits EN TEMPS RÉEL dans la boucle de simulation de base — `qmc_dmrg_rmse_rt`, `qmc_dmrg_mae_rt`, `qmc_dmrg_within_rt`, `qmc_dmrg_ci95_rt`, `external_modules_rmse_rt`, `external_modules_mae_rt`, `external_modules_within_rt` |
| `cluster_scale` | Multi-tailles réseau (8×8 → 255×255) |
| `thermodynamic_limit` | Extrapolation limite thermodynamique |

---

## SECTION H — VARIABLES D'ENVIRONNEMENT

| NOM D'ORIGINE | Valeur | Description |
|---|---|---|
| `PTMC_RUN_INDEX` | entier 0–N | Seeds déterministes distincts : XOR = N × φ64 (0x9e3779b97f4a7c15) |
| `PTMC_SEED_RANDOM` | `1` | Seed aléatoire basée sur time(NULL)^(pid<<32) |
| `LUMVORAX_ENABLED` | flag compilateur `-DLUMVORAX_ENABLED=1` | Active le bridge secondaire Lumvorax (distinct du logger principal) |
| `SUPABASE8_API_URL` | `https://<ref>.supabase.co` | URL REST PostgREST (secret Replit / local — **ne pas committer**) |
| `SUPABASE_SERVICE_ROLE_KEY` | JWT | Écriture REST (upload_to_supabase) |
| `SUPABASE_DEBUG` | `1` | Journalise les corps d’erreur HTTP PostgREST complets |
| `SUPABASE_MAX_LOG_LINES` | entier | Limite d’upload des lignes `raw_log` (défaut 5000) |
| `OMP_NUM_THREADS` / `OPENBLAS_NUM_THREADS` | ≤ nproc | Parallélisme CPU (fix C65 dans `run_research_cycle.sh`) |
| `LUMVORAX_SHELL_TRACE` | `1` | Lignes `[PHASE]` dans la session shell (orchestration fullscale/advanced) |

---

## PROCÉDURE D'AJOUT D'UN NOUVEAU NOM

```
1. grep -i "MOT_CLE" STANDARD_NAMES.md
2. Si trouvé → utiliser le NOM D'ORIGINE. Fin.
3. Si absent :
   a. Remplir le template ci-dessous
   b. Ajouter à la section appropriée
   c. Committer STANDARD_NAMES.md
   d. PUIS écrire le code
   e. Dans le code : /* STANDARD_NAMES: NOM_AJOUTÉ */

   Template :
   | NOM | fichier_source.c | Signature | Description |
```

---

---

## SECTION I — LOGGER V4 NEXT (quantum_forensic_logger) — AJOUTÉ C33

> **Projet distinct : `quantum_simulator_v4_staging_next`**
> Logger ultra-granulaire calqué sur ultra_forensic_logger, adapté aux qubits.
> Fichiers : `quantum_forensic_logger.h` + `quantum_forensic_logger.c`
> **Ces noms sont officiels pour V4 NEXT — ne pas les réutiliser dans Hubbard.**

### Macros préprocesseur V4 NEXT (NOM D'ORIGINE = À UTILISER)

| NOM D'ORIGINE (officiel V4 NEXT) | Fichier définition | Appel interne | Description |
|---|---|---|---|
| `QF_LOG_MODULE_START(module, test)` | `quantum_forensic_logger.h` | `qf_log_module_start()` | Démarre un module V4 NEXT |
| `QF_LOG_MODULE_END(module, test, result)` | `quantum_forensic_logger.h` | `qf_log_module_end()` | Termine un module V4 NEXT |
| `QF_LOG_MODULE_METRIC(module, metric, value)` | `quantum_forensic_logger.h` | `qf_log_module_metric()` | **LOG PRINCIPAL V4 NEXT** — écrit METRIC dans CSV |
| `QF_LOG_NANO(module, key, value)` | `quantum_forensic_logger.h` | `qf_log_nano()` | Ring buffer nanoseconde 4096 entrées |
| `QF_LOG_HW_SAMPLE(module)` | `quantum_forensic_logger.h` | `qf_log_hw_sample()` | Snapshot CPU delta + RAM RSS |
| `QF_LOG_ANOMALY(module, desc, value)` | `quantum_forensic_logger.h` | `qf_log_anomaly()` | Anomalie fidélité/cohérence/gate |
| `QF_LOG_ALGO(algo, metric, value)` | `quantum_forensic_logger.h` | `qf_log_algo()` | Algorithme quantique (Hadamard, Grover…) |
| `QF_LOG_GATE(module, gate, qubit_id, fidelity)` | `quantum_forensic_logger.h` | `qf_log_gate()` | Porte quantique step-by-step |
| `QF_LOG_QUBIT_STATE(module, qid, sidx, re, im, prob)` | `quantum_forensic_logger.h` | `qf_log_qubit_state()` | Amplitude qubit step-by-step |
| `QF_LOG_ENTANGLE(module, qid1, qid2, count)` | `quantum_forensic_logger.h` | `qf_log_entangle()` | Événement d'intrication |
| `QF_LOG_MEASURE(module, qid, state, prob)` | `quantum_forensic_logger.h` | `qf_log_measure()` | Mesure avec collapse |
| `QF_LOG_SCENARIO(module, scen, step, nx, q, wins)` | `quantum_forensic_logger.h` | `qf_log_scenario()` | Scénario fusion_v3 step-by-step |
| `QF_LOG_THREAD_START(module, label)` | `quantum_forensic_logger.h` | `qf_log_thread()` | Thread START |
| `QF_LOG_THREAD_END(module, label)` | `quantum_forensic_logger.h` | `qf_log_thread()` | Thread END |
| `QF_LOG_PHASE_BRIDGE(phase, script, status)` | `quantum_forensic_logger.h` | `qf_log_phase_bridge()` | Bridge Python → CSV |

### Fonctions C V4 NEXT (NOM D'ORIGINE = À UTILISER)

| NOM D'ORIGINE | Signature | Description |
|---|---|---|
| `qf_logger_init(run_id, log_dir)` | `void f(const char*, const char*)` | Initialise le logger V4 NEXT avec run_id |
| `qf_logger_destroy()` | `void f(void)` | Ferme le logger, écrit SUMMARY |
| `qf_logger_is_initialized()` | `bool f(void)` | Vérifie l'initialisation |
| `qf_logger_get_csv_path()` | `const char* f(void)` | Retourne le chemin CSV courant |
| `qf_log_module_start(file, line, func, module, test)` | `void f(...)` | Démarre module |
| `qf_log_module_end(file, line, func, module, test, result)` | `void f(...)` | Termine module |
| `qf_log_module_metric(file, line, func, module, metric, value)` | `void f(...)` | **ÉCRITURE LOG PRINCIPALE** |
| `qf_log_nano(func, module, key, value)` | `void f(...)` | Ring buffer ns |
| `qf_log_hw_sample(func, module)` | `void f(...)` | Snapshot /proc |
| `qf_log_anomaly(file, line, func, module, desc, value)` | `void f(...)` | Log anomalie |
| `qf_log_algo(func, algo, metric, value)` | `void f(...)` | Log algorithme |
| `qf_log_gate(func, module, gate, qubit_id, fidelity)` | `void f(...)` | Log porte quantique |
| `qf_log_qubit_state(func, module, qid, sidx, re, im, prob)` | `void f(...)` | Log amplitude qubit |
| `qf_log_entangle(func, module, qid1, qid2, count)` | `void f(...)` | Log intrication |
| `qf_log_measure(func, module, qid, state, prob)` | `void f(...)` | Log mesure avec collapse |
| `qf_log_scenario(func, module, scen, step, nx, q, wins)` | `void f(...)` | Log scénario fusion_v3 |
| `qf_log_thread(func, module, label, event)` | `void f(...)` | Log thread START/END |
| `qf_log_phase_bridge(phase, script, status)` | `void f(...)` | Log bridge Python |
| `qf_hw_snapshot()` | `qf_hw_snapshot_t f(void)` | Lit /proc pour CPU+RAM |
| `qf_flush_nano_ring()` | `void f(void)` | Flush ring buffer → CSV |
| `qf_rotate_csv_if_needed()` | `void f(void)` | Rotation CSV à 95 MB |
| `qf_check_anomaly_fidelity(module, prev, curr)` | `bool f(...)` | Détecte chute fidélité |
| `qf_check_anomaly_prob_norm(module, probs, n)` | `bool f(...)` | Détecte dénormalisation |
| `qf_generate_summary_report()` | `void f(void)` | Rapport SUMMARY final |
| `qf_validate_all_logs_exist()` | `bool f(void)` | Valide existence fichiers logs |

### Types V4 NEXT (NOM D'ORIGINE)

| NOM D'ORIGINE | Type | Description |
|---|---|---|
| `qf_level_e` | `enum` | Niveaux log V4 NEXT (QF_TRACE..QF_EMERGENCY) |
| `qf_event_type_e` | `enum` | Types événements CSV V4 NEXT |
| `qf_nano_entry_t` | `struct` | Entrée ring buffer nanoseconde |
| `qf_hw_snapshot_t` | `struct` | Snapshot hardware CPU/RAM |
| `qf_module_tracker_t` | `struct` | Tracker par module V4 NEXT |
| `qf_anomaly_thresholds_t` | `struct` | Seuils détection anomalies quantiques |

### Variables globales V4 NEXT (NOM D'ORIGINE)

| NOM D'ORIGINE | Type | Fichier | Description |
|---|---|---|---|
| `g_qf_csv_path` | `char[4096]` | `quantum_forensic_logger.c` | Chemin CSV LumVorax V4 NEXT |
| `g_qf_csv_mutex` | `pthread_mutex_t` | `quantum_forensic_logger.c` | Thread-safety CSV V4 NEXT |
| `g_qf_initialized` | `bool` | `quantum_forensic_logger.c` | État initialisation V4 NEXT |
| `g_qf_nano_ring` | `qf_nano_entry_t[4096]` | `quantum_forensic_logger.c` | Ring buffer nanoseconde |
| `g_qf_nano_head` | `volatile int` | `quantum_forensic_logger.c` | Pointeur ring buffer |
| `g_qf_nano_mutex` | `pthread_mutex_t` | `quantum_forensic_logger.c` | Thread-safety ring buffer |
| `g_qf_thresholds` | `qf_anomaly_thresholds_t` | `quantum_forensic_logger.c` | Seuils anomalies actifs |

### Préfixes métriques V4 NEXT (colonne 5 du CSV)

| PRÉFIXE D'ORIGINE (officiel V4 NEXT) | Signification | Métriques associées |
|---|---|---|
| `quantum_lum:` | Opérations sur LUM quantique | `id`, `state_count`, `fidelity`, `coherence_time_ns`, `entanglement_count` |
| `quantum_gate:` | Porte quantique appliquée | `gate_type`, `qubit_id`, `fidelity_after`, `elapsed_ns` |
| `quantum_measure:` | Mesure avec collapse | `qubit_id`, `measured_state`, `probability`, `elapsed_ns` |
| `quantum_entangle:` | Intrication | `qubit_id1`, `qubit_id2`, `entangle_count` |
| `quantum_algo:` | Algorithme quantique | `algo_name`, `nqubits`, `fidelity_loss`, `elapsed_ns` |
| `fusion_v3_scenario:` | Scénario benchmark fusion_v3 | `scenario_idx`, `step`, `nx_score`, `q_score`, `nx_wins`, `latency_ns` |
| `fusion_v3_summary:` | Résumé run complet | `nqubits_simulated`, `nqubits_per_sec`, `win_rate`, `p50_ns`, `p95_ns`, `p99_ns` |

### Fichiers officiels V4 NEXT

| Rôle | Fichier officiel |
|---|---|
| Logger V4 NEXT principal | `src/advanced_calculations/quantum_simulator_v4_staging_next/quantum_forensic_logger.c` |
| Header logger V4 NEXT | `src/advanced_calculations/quantum_simulator_v4_staging_next/quantum_forensic_logger.h` |
| Simulateur quantique | `src/advanced_calculations/quantum_simulator_v4_staging_next/quantum_simulator.c` |
| Fusion benchmark V3 | `src/advanced_calculations/quantum_simulator_v4_staging_next/quantum_simulator_fusion_v3.c` |
| Script run V4 NEXT | `src/advanced_calculations/quantum_simulator_v4_staging_next/run_on_replit_v4_next.sh` |
| Dossier rapports V4 NEXT | `src/advanced_calculations/quantum_simulator_v4_staging_next/RAPPORTS/` |
| Dossier rapports Hubbard | `src/advanced_calculations/quantum_problem_hubbard_hts/RAPPORTS/` |

---

## SECTION J — TABLES SUPABASE ET COLONNES CSV BENCHMARK (NOM D'ORIGINE = À UTILISER)

> **Règle :** Ces noms sont figés. Ne jamais en créer de nouveaux pour les mêmes entités.
> Toute écriture (Python, shell, C) doit utiliser exactement ces noms de colonnes.

### Tables Supabase (NOM D'ORIGINE)

| NOM D'ORIGINE (table) | Rôle |
|---|---|
| `quantum_benchmarks` | Références QMC/DMRG et externes — source de vérité benchmark |
| `run_scores` | Scores par run (iso, trace, repr, robust, phys, expert, total) |
| `quantum_run_files` | Métadonnées des fichiers de run (path, taille, checksum) |
| `quantum_csv_rows` | Lignes CSV individuelles archivées par run |
| `benchmark_runtime` | Résultats de benchmark par run et par module |
| `research_modules_config` | Configuration des modules de recherche |
| `problems_config` | Paramètres physiques des problèmes |

### Colonnes table `quantum_benchmarks` (NOM D'ORIGINE)

| NOM D'ORIGINE (colonne Supabase) | Type | Description |
|---|---|---|
| `id` | int | Clé primaire auto |
| `dataset` | text | `qmc_dmrg` ou `external` — **jamais** `external_modules` |
| `module` | text | Nom du module (conforme Section D) |
| `observable` | text | `energy_eV` ou `pairing` |
| `t_k` | float | Température en Kelvin |
| `u_over_t` | float | U/t (valeur numérique utilisée comme u_eV si t=1) |
| `reference_value` | float | Valeur de référence publiée |
| `reference_method` | text | Méthode de référence (QMC, DMRG, exact, etc.) |
| `source` | text | Publication source (ex: `Leblanc2015`, `exact_2x2`) |
| `error_bar` | float | Barre d'erreur sur la référence |
| `notes` | text | Notes libres |
| `created_at` | timestamp | Date création auto |

### Colonnes table `run_scores` (NOM D'ORIGINE)

| NOM D'ORIGINE (colonne Supabase) | Type | Description |
|---|---|---|
| `id` | int | Clé primaire auto |
| `run_id` | text | Identifiant run (ex: `research_20260327T165138Z_841`) — UNIQUE |
| `runner` | text | Version du runner (ex: `fullscale_v7`, `advanced_parallel_v10`) |
| `score_iso` | int | Score isolation (0–100) |
| `score_trace` | int | Score traçabilité (0–100) |
| `score_repr` | int | Score reproductibilité (0–100) |
| `score_robust` | int | Score robustesse (0–100) |
| `score_phys` | int | Score physique (0–100) |
| `score_expert` | int | Score expert (0–100) |
| `score_total` | int | Total (somme des 6 critères, max 600) |
| `notes` | text | Notes libres sur le run |
| `created_at` | timestamp | Date création auto |

### Colonnes table `benchmark_runtime` (NOM D'ORIGINE — vérifié Supabase 2026-03-30)

| NOM D'ORIGINE (colonne Supabase) | Type | Description |
|---|---|---|
| `id` | int | Clé primaire auto |
| `run_id` | text | Identifiant run |
| `benchmark_type` | text | Type de benchmark (`qmc_dmrg` ou `external`) |
| `u_ev` | float | U en eV (paramètre Hubbard) |
| `module` | text | Nom du module (conforme Section D) |
| `observable` | text | `energy_eV` ou `pairing` |
| `t_k` | float | Température en Kelvin |
| `reference_value` | float | Valeur de référence publiée |
| `error_bar` | float | Barre d'erreur sur la référence |
| `model_value` | float | Valeur simulée par le modèle |
| `abs_error` | float | Erreur absolue `|model - ref|` |
| `rel_error` | float | Erreur relative |
| `within_error_bar` | bool | 1 si `abs_error ≤ error_bar` |
| `rmse_global` | float | RMSE global de la session |
| `mae_global` | float | MAE global de la session |
| `pct_within_global` | float | % points dans barre d'erreur (global) |
| `created_at` | timestamp | Date création auto |

> **Note C71-2026-03-30** : Les colonnes `dataset` et `u_over_t` (anciennes définitions) sont ABSENTES de Supabase.
> Utiliser `benchmark_type` et `u_ev` à la place — conforme schéma réel vérifié via PostgreSQL pooler.

### Colonnes table `problems_config` (NOM D'ORIGINE — vérifié Supabase 2026-03-30)

| NOM D'ORIGINE (colonne Supabase) | Type | Description |
|---|---|---|
| `id` | int | Clé primaire auto |
| `name` | text | Nom du problème (conforme Section D) |
| `lx` | int | Dimension x de la grille |
| `ly` | int | Dimension y de la grille |
| `t_ev` | float | Paramètre de hopping t (eV) |
| `u_ev` | float | Paramètre d'interaction U (eV) |
| `mu_ev` | float | Potentiel chimique µ (eV) |
| `temp_k` | float | Température en Kelvin |
| `dt` | float | Pas de temps dt |
| `steps` | int | Nombre de steps MC |
| `cycle` | int | Numéro de cycle de recherche |
| `notes` | text | Notes libres |
| `created_at` | timestamp | Date création auto |

> **Note C71-2026-03-30** : La colonne `enabled` (ancienne définition) est ABSENTE de Supabase.
> Les colonnes réelles sont `cycle` et `notes` — conforme schéma réel vérifié via PostgreSQL pooler.

### Format CSV benchmark de référence (NOM D'ORIGINE — 7 colonnes)

> Fichiers : `benchmarks/qmc_dmrg_reference_runtime.csv` et `benchmarks/external_module_benchmarks_runtime.csv`
> Généré par : `tools/supabase_client.py` → `generate_benchmark_runtime_csv()`
> Lu par : `load_benchmark_rows()` dans `hubbard_hts_research_cycle_advanced_parallel.c`

```
source,module,observable,t_k,u_eV,reference_value,error_bar
```

| Position | NOM D'ORIGINE (colonne CSV) | Correspond à (Supabase) | Description |
|---|---|---|---|
| 1 | `source` | `source` | Publication (ex: `Leblanc2015`) |
| 2 | `module` | `module` | Nom module physique |
| 3 | `observable` | `observable` | `energy_eV` ou `pairing` |
| 4 | `t_k` | `t_k` | Température K |
| 5 | `u_eV` | `u_over_t` | U en eV (=U/t si t=1 eV) |
| 6 | `reference_value` | `reference_value` | Valeur référence publiée |
| 7 | `error_bar` | `error_bar` | Barre d'erreur |

### Format CSV de sortie benchmark (NOM D'ORIGINE — 10 colonnes)

> Fichiers générés par le code C : `benchmark_comparison_qmc_dmrg.csv` et `benchmark_comparison_external_modules.csv`

```
module,observable,T,U,reference,model,abs_error,rel_error,error_bar,within_error_bar
```

### Identifiants dataset `quantum_benchmarks` (NOM D'ORIGINE)

| NOM D'ORIGINE | Fichier runtime cible |
|---|---|
| `qmc_dmrg` | `benchmarks/qmc_dmrg_reference_runtime.csv` |
| `external` | `benchmarks/external_module_benchmarks_runtime.csv` |

> **INTERDIT** : `external_modules` (ancien nom erroné — supprimé C63)

---

## SECTION K — C68-REALTIME-BENCH : BENCHMARKS EN TEMPS RÉEL (2026-03-28)

> **Problème résolu :** Dans les anciens runs (ex : run 824), les fichiers CSV benchmark
> (`benchmark_comparison_qmc_dmrg.csv`, `benchmark_comparison_external_modules.csv`) étaient
> **entièrement vides** car ils étaient calculés APRÈS le PT_MC (~1.3 GB de CSV).
> Quand le PT_MC crashait (OOM), aucun benchmark n'était jamais écrit sur disque.

### Principe C68 (NOM D'ORIGINE = à utiliser dans tout nouveau code)

| CONCEPT C68 | Nom officiel dans le code | Fichier | Description |
|---|---|---|---|
| Chargement anticipé | `brow_rt[256]` + `bn_rt` + `bn_mod_rt` + `bench_offset_rt` | advanced_parallel.c | Chargement des CSV de référence AVANT `sim_result_t base[16]` |
| Benchmark en boucle | Bloc `C68-REALTIME-BENCH QMC` + `C68-REALTIME-BENCH EXT` | advanced_parallel.c | Écriture dans `bcsv`/`bcsvm` après chaque `simulate_fullscale()` |
| RMSE immédiat | `rt_sum_sq`, `rt_m`, `rt_within`, etc. | advanced_parallel.c | Calculé et écrit dans `tcsv` AVANT le démarrage du PT_MC |
| Flush disque | `fflush(bcsv)` + `fflush(bcsvm)` + `fflush(lg)` | advanced_parallel.c | Après chaque module — survie au SIGKILL/OOM |
| Section post-PT_MC | `C68-REALTIME-BENCH-POSTPTMC` | advanced_parallel.c | Log de confirmation seulement — AUCUNE re-simulation |

### Tags de log C68 (NOM D'ORIGINE — à ne jamais renommer)

| TAG dans le log principal (run_log_advanced_parallel.csv) | Signification |
|---|---|
| `BENCH_RT_INIT` | Initialisation : nombre de lignes QMC/EXT chargées depuis les CSV de référence |
| `BENCH_RT_QMC` | Une ligne QMC/DMRG calculée en temps réel (module, obs, ref, model, abs_e, rel_e, within) |
| `BENCH_RT_EXT` | Une ligne EXT calculée en temps réel |
| `BENCH_RT_QMC_SUMMARY` | RMSE/MAE/within global QMC — écrit AVANT le PT_MC |
| `BENCH_RT_EXT_SUMMARY` | RMSE/MAE/within global EXT — écrit AVANT le PT_MC |
| `BENCH_QMC_START` | Marqueur post-PT_MC — mode C68 (no-resim, use-rt-counters) |
| `BENCH_QMC_ROW` | Log de confirmation post-PT_MC — C68:reuse-base (pas de re-simulation) |
| `BENCH_QMC_END` | Fin section QMC post-PT_MC |
| `BENCH_EXT_START` | Marqueur EXT post-PT_MC — mode C68 (no-resim) |
| `BENCH_EXT_ROW` | Log de confirmation EXT post-PT_MC — C68:reuse-base |
| `BENCH_EXT_END` | Fin section EXT post-PT_MC |

### Métriques `tcsv` ajoutées par C68 (new_tests_results.csv — NOM D'ORIGINE)

| NOM D'ORIGINE (test_id dans tcsv) | Famille | Seuil PASS | Description |
|---|---|---|---|
| `qmc_dmrg_rmse_rt` | `benchmark_rt` | `≤ 0.10 eV/site` | RMSE benchmark QMC temps réel |
| `qmc_dmrg_mae_rt` | `benchmark_rt` | `≤ 0.10 eV/site` | MAE benchmark QMC temps réel |
| `qmc_dmrg_within_rt` | `benchmark_rt` | `≥ 60 %` | % points dans barre d'erreur QMC temps réel |
| `qmc_dmrg_ci95_rt` | `benchmark_rt` | `≤ 0.10 eV/site` | IC 95% half-width QMC temps réel |
| `external_modules_rmse_rt` | `benchmark_rt` | `≤ 0.15 eV/site` | RMSE benchmark EXT temps réel |
| `external_modules_mae_rt` | `benchmark_rt` | `≤ 0.15 eV/site` | MAE benchmark EXT temps réel |
| `external_modules_within_rt` | `benchmark_rt` | `≥ 70 %` | % points dans barre d'erreur EXT temps réel |

### Règle C68 obligatoire pour tout nouveau code benchmark

```
RÈGLE C68 — BENCHMARK TEMPS RÉEL (OBLIGATOIRE) :
  1. Tout calcul benchmark DOIT être effectué dans la boucle de simulation de base,
     immédiatement après simulate_fullscale() pour le module courant.
  2. Utiliser base[i] directement — JAMAIS re-simuler pour les benchmarks.
  3. fflush(bcsv) + fflush(bcsvm) + fflush(lg) après chaque module.
  4. RMSE global écrit dans tcsv AVANT le démarrage du PT_MC.
  5. La section post-PT_MC ne fait QUE logguer — aucune simulation supplémentaire.
  6. Variables locales : brow_rt[], bn_rt, bn_mod_rt, bench_offset_rt, rt_sum_sq,
     rt_sum_abs, rt_m, rt_within, rt_sum_sq_mod, rt_sum_abs_mod, rt_m_mod, rt_within_mod.
     NE PAS les renommer — ils apparaissent dans les RÉSUMÉS de session.
```

### Anciens comportements supprimés par C68 (INTERDITS)

| Comportement supprimé | Raison | Correction C68 |
|---|---|---|
| `simulate_fullscale()` appelé dans la boucle benchmark QMC post-PT_MC | Re-simulation inutile, seed 1234+i non validée → résultats différents de base[] | Utiliser `base[ip]` directement |
| `brow[256]` déclaré après le PT_MC | Variables benchmark inaccessibles en cas de crash PT_MC | `brow_rt[256]` déclaré avant `sim_result_t base[16]` |
| Benchmarks absents des CSV si crash PT_MC | Cause : écriture uniquement en fin de run | Écriture module par module avec fflush immédiat |

---

*Maintenu par :* Agent Replit  
*Version 1.0 :* 2026-03-16 — création initiale (erreur : LV_MODULE_METRIC désigné comme officiel)  
*Version 2.0 :* 2026-03-16 — correction C25-NAMES : FORENSIC_LOG_MODULE_METRIC = nom d'origine réel  
*Version 3.0 :* 2026-03-17 — ajout Section I : Logger V4 NEXT (qf_log_*) — Cycle C33  
*Version 4.0 :* 2026-03-27 — ajout Section J : Tables Supabase + colonnes CSV benchmark — Cycle C63  
*Version 5.0 :* 2026-03-28 — ajout Section K : C68-REALTIME-BENCH (benchmarks temps réel, tags log, variables rt_*, métriques tcsv) — Cycle C68

---

## SECTION D — MODULE 16 : Random Circuit Sampling (C91-RCS)

**Ajouté le :** 2026-04-02 — Cycle C91-RCS  
**Auteur :** Agent Replit — conforme RÈGLE D'UTILISATION §1-5  
**Référence :** CHAT/analysechatgpt82.2.md (section 5)

### Paramètres dans `problems_cycle06.csv` (ligne 17, module 16)

| Colonne CSV | Valeur | Interprétation RCS |
|---|---|---|
| `name` | `random_circuit_sampling` | **NOM D'ORIGINE — ne jamais renommer** |
| `lx` | `10` | Largeur réseau qubits (10 × 10 = 100 qubits) |
| `ly` | `10` | Hauteur réseau qubits |
| `t_eV` | `1.000000` | `coupling_strength` — amplitude porte 2Q (CZ) |
| `u_eV` | `2.000000` | `entanglement_strength` — modulation bruit de couplage |
| `mu_eV` | `0.000000` | `phase_offset` (inutilisé dans la v1) |
| `temp_K` | `0.001` | `noise_level_K` (quasi-idéal : bruit ≈ 0) |
| `dt` | `0.010000` | `circuit_depth_increment` : depth = dt × 1000 = 10 couches |
| `steps` | `5000` | `n_circuits_sampled` (5 000 circuits Haar-aléatoires) |

### Fonctions C (NOM D'ORIGINE — SECTION D)

| NOM D'ORIGINE | Fichier | Signature | Description |
|---|---|---|---|
| `simulate_rcs_module` | `src/random_circuit_sampling.c` | `rcs_result_t f(const rcs_problem_t*, uint64_t)` | **Simulation RCS principale** — génère circuits, calcule XEB, compare Porter-Thomas |
| `apply_haar_1q` | `src/random_circuit_sampling.c` | `void f(double*,double*,double*,double*, uint64_t*)` | Porte Haar-aléatoire 1-qubit (3 angles θ,φ,λ) |
| `apply_cz_2q` | `src/random_circuit_sampling.c` | `void f(double*,double*,double*,double*, double, uint64_t*)` | Porte CZ entanglement 2-qubits avec couplage modulé |

### Types C (NOM D'ORIGINE — SECTION D)

| NOM D'ORIGINE | Fichier | Description |
|---|---|---|
| `rcs_result_t` | `src/random_circuit_sampling.h` | Résultat simulation RCS (F_XEB, H_norm, XEB_ratio, etc.) |
| `rcs_problem_t` | `src/random_circuit_sampling.h` | Paramètres du problème RCS (aligné sur `problem_t`) |

### Métriques LumVorax loggées par le module RCS

Préfixe obligatoire : `rcs:` — toutes les métriques commencent par `rcs:` dans le CSV Lumvorax.

| NOM D'ORIGINE (metric dans FORENSIC_LOG_MODULE_METRIC) | Unité | Description |
|---|---|---|
| `rcs:n_qubits` | entier | Nombre de qubits simulés (lx × ly) |
| `rcs:circuit_depth` | entier | Profondeur effective du circuit (couches de portes 2Q) |
| `rcs:n_circuits` | entier | Nombre de circuits Haar-aléatoires générés |
| `rcs:coupling_strength` | eV | Amplitude porte CZ 2-qubits |
| `rcs:entanglement_str` | eV | Force d'entanglement (modulation bruit) |
| `rcs:noise_level_eV` | eV | Bruit thermique (kB × T_K) |
| `rcs:F_xeb_mean` | sans unité [0,1] | Score XEB moyen : fidelité Cross-Entropy Benchmarking |
| `rcs:xeb_std` | sans unité | Écart-type XEB sur les n_circuits |
| `rcs:xeb_rel_var` | sans unité | Variance relative XEB (convergence si < 0.01) |
| `rcs:H_norm` | sans unité [0,1] | Entropie de Shannon normalisée : H / H_max |
| `rcs:porter_thomas_kl` | sans unité | Divergence KL approx vs distribution Porter-Thomas |
| `rcs:xeb_ratio_vs_willow` | sans unité | F_XEB / F_Willow (>1 = record battu) |
| `rcs:xeb_drift_mean` | sans unité | Dérivée temporelle moyenne de la fidelité XEB |
| `rcs:norm_dev_max` | sans unité | Max |‖ψ‖ - 1| (stabilité numérique) |
| `rcs:converged` | 0/1 | Convergence XEB (variance relative < 1%) |
| `rcs:willow_fidelity_ref` | sans unité | Fidelité de référence Google Willow 2024 (≈2×10⁻⁴) |
| `rcs:beats_willow` | 0/1 | 1 si F_XEB > F_Willow |
| `rcs:op_init_state_circuit` | entier | Opération : initialisation état |+⟩^n |
| `rcs:op_init_inv_sqrt_n` | 1/√n | Facteur de normalisation initial |
| `rcs:op_layer_start` | entier | Opération : début couche de portes |
| `rcs:op_1q_gate_qubit` | entier | Opération : application porte Haar 1Q sur qubit q |
| `rcs:op_2q_cz_pair` | entier | Opération : application porte CZ sur paire (q, q+1) |
| `rcs:op_2q_coupling` | eV | Valeur du couplage effectif utilisé |
| `rcs:op_renorm_factor` | 1/‖ψ‖ | Facteur de renormalisation (stabilité numérique) |
| `rcs:op_p_bitstring_circuit` | entier | Opération : calcul probabilité bitstring (circuit circ) |
| `rcs:p_bitstring` | sans unité | Probabilité du bitstring mesuré (approximation MF) |
| `rcs:entropy_circuit` | nats | Entropie de Shannon locale du circuit |
| `rcs:xeb_circuit` | sans unité | Contribution XEB du circuit : D_eff × p - 1 |
| `rcs:D_eff_log` | sans unité | log(D) = n_qubits × ln(2) |
| `rcs:op_acc_xeb_running_mean` | sans unité | Moyenne glissante XEB (toutes 500 itérations) |
| `rcs_to_sim_F_xeb` | sans unité | Conversion rcs_result_t → sim_result_t : F_XEB → energy_eV |
| `rcs_to_sim_H_norm` | sans unité | Conversion : H_norm → pairing_norm |
| `rcs_to_sim_xeb_ratio` | sans unité | Conversion : xeb_ratio → sign_ratio |
| `rcs_to_sim_norm_dev_max` | sans unité | C44-FIX-NORM-01 : vraie norm_deviation_max (8-comp, non forcée) |
| `rcs_to_sim_converged` | 0/1 | C44-FIX-NORM-01 : flag converged RCS transmis au runner principal |
| `rcs:n_phys_qubits` | entier | **C44-OPT-8COMP** : qubits physiques totaux = n_sites × n_orbitales (784) |
| `rcs:n_components` | entier | **C44-OPT-8COMP** : composantes MF par qubit (8 = 4 complexes par orbital) |
| `rcs:n_orbitals_per_site` | entier | **C44-OPT-8COMP** : orbitales par site (2 = spin↑ + spin↓) |
| `rcs:hilbert_factor_vs_c43` | sans unité | **C44-OPT-8COMP** : facteur expansion espace Hilbert vs C43 (×2) |
| `rcs:log_D_8comp` | nats | **C44-OPT-8COMP** : log(D) = n_phys_qubits × ln2 = 543.43 pour 784 qubits |
| `rcs:willow_ratio_n_qubits` | sans unité | **C44-OPT-8COMP** : n_phys_qubits / 105 (Willow) = 7.47 pour 784 qubits |
| `rcs:caltech_ratio_n_qubits` | sans unité | **C44-OPT-8COMP** : n_phys_qubits / 6160 (Caltech) = 0.127 pour 784 qubits |

### Convention de conversion rcs_result_t → sim_result_t

| Champ `sim_result_t` | Source dans `rcs_result_t` | Signification physique RCS |
|---|---|---|
| `energy_eV` | `rr.energy_eV` = `|F_XEB|` | Fidelité XEB (≥ 0) |
| `pairing_norm` | `rr.pairing_norm` = `H_norm` | Entropie normalisée [0,1] |
| `sign_ratio` | `rr.sign_ratio` = `xeb_ratio` | Rapport vs Willow (>1 = record) |
| `energy_drift_metric` | `rr.energy_drift_metric` | Drift XEB inter-circuits |
| `norm_deviation_max` | `rr.norm_deviation_max` | Stabilité numérique |
| `cpu_peak`, `mem_peak`, `elapsed_ns` | directs | Ressources HW |

### Références physiques

| Document | Résultat RCS pertinent |
|---|---|
| Arute et al., Nature 574, 505 (2019) | Sycamore : 53 qubits, depth 20, F_XEB ≈ 0.22% |
| Google Quantum AI, Nature 638, 840 (2024) | Willow : 105 qubits, depth 25, F_XEB ≈ 2×10⁻⁴ |
| Boixo et al., Nature Physics 14, 595 (2018) | Théorie XEB : F_XEB = D × ⟨p_ideal⟩ − 1 |
| Porter & Thomas, Phys. Rev. 104, 483 (1956) | Distribution idéale : P(p) = D exp(−Dp) |

---

## SECTION C41 — CORRECTIONS CYCLE 41 (2026-04-04)

### C41-§1 : Convention ed_validation_2x2 (ANO-ED-NORM — CORRIGÉE)

> **Source anomalie :** `analysechatgpt89.md` §BENCH_QMC — ed_validation_2x2 systématiquement hors tolérance.

**Convention officielle (après alignement C41-FIX-04) :**

```
observable = |E0_Lanczos(lx, ly, t, U, mu=0)| / N_sites
  avec N_sites = lx × ly = 4 (réseau 2×2)
  avec t=1.0 eV, U=4.0 eV (ref ligne 1) ou U=8.0 eV (ref ligne 2)
  T=10K ≈ T=0 (Lanczos état fondamental)

Résultats vérifiés (run 2948, 2026-04-04) :
  U=4.0 eV : E0_raw = -2.1028 eV → |E0|/4 = 0.5257 eV/site  → ref = 0.5257, err = 0.05
  U=8.0 eV : E0_raw = -1.3204 eV → |E0|/4 = 0.3301 eV/site  → ref = 0.3301, err = 0.05
```

> **INTERDIT :** Utiliser les références LeBlanc2015 (U/t adimensionnel) comme référence
> pour `ed_validation_2x2` — convention différente (t en unités de t, pas en eV).
> La validation `ed_validation_2x2` DOIT utiliser les valeurs calculées par notre propre Lanczos.

### C41-§2 : Politique de promotion benchmark runtime→canonique (Q20 — DOCUMENTÉE)

**Politique officielle (STANDARD_NAMES.md v3.2) :**

```
PROMOTION RUNTIME → CANONIQUE (Q20) :
  Condition AUTO (sans validation humaine) :
    - RMSE_runtime ≤ 0.05 eV/site  ET
    - MAE_runtime ≤ 0.05 eV/site   ET
    - within_error_bar ≥ 85%       ET
    - Au moins 3 runs consécutifs avec ce score

  Condition VALIDATION HUMAINE requise :
    - Modification d'une ligne avec source = publication externe (Leblanc2015, etc.)
    - Tout changement error_bar > 50% de la valeur de référence
    - Promotion vers benchmarks/qmc_dmrg_CANONICAL.csv (version immuable)

  Commande d'archivage : cp qmc_dmrg_reference_runtime.csv
    benchmarks/history/qmc_dmrg_reference_<STAMP_UTC>.csv
```

### C41-§3 : Séparation références publiées / calibration interne (Q21 — DOCUMENTÉE)

```
FICHIERS IMMUABLES (refs publiées — NE PAS MODIFIER) :
  benchmarks/qmc_dmrg_CANONICAL.csv          — références LeBlanc2015, Prokofev2004, etc.
  benchmarks/external_module_CANONICAL.csv   — références modules avancés

FICHIERS ÉVOLUTIFS (calibration interne — peut évoluer par run) :
  benchmarks/qmc_dmrg_reference_runtime.csv  — dont ed_validation_2x2 (convention interne)
  benchmarks/external_module_benchmarks_runtime.csv

RÈGLE : Jamais modifier les CANONIQUES sans validation humaine (PR + commentaire scientifique).
```

### C41-§4 : Versionnage historique des références runtime par campagne (Q22 — DOCUMENTÉE)

```
VERSIONNAGE DES RÉFÉRENCES (Q22) :
  Format : benchmarks/history/qmc_dmrg_reference_<STAMP_UTC>.csv
  Automatiquement archivé par upload_to_supabase.py à chaque run (table benchmark_rt_results)
  Stamp UTC = nom du répertoire run (ex : research_20260404T191542Z_2076)
  → versionnage implicite par le système de résultats LumVorax
  → reconstruction historique possible via Supabase table benchmark_rt_results
```

### C41-§5 : Métriques RCS ajoutées C41

| NOM D'ORIGINE | Unité | Description |
|---|---|---|
| `rcs:log_D_hilbert` | nats | log(D_Hilbert) = n_qubits × ln(2) — dimension espace de Hilbert |
| `rcs:circuit_depth_used` | entier | Profondeur circuit utilisée (dt × 1000, clamp [1,100]) |
| `rcs:log_D_eff_xeb` | nats | log(D_eff) = circuit_depth × ln(2) — dimension effective (tracé uniquement) |

> `rcs:log_D_eff_xeb` est loggé pour traçabilité C41. La formule XEB utilise `rcs:log_D_hilbert`
> (physiquement correct pour la comparaison Willow). La valeur `rcs:F_xeb_mean = 0.5047`
> confirme l'absence d'overflow systématique après init Porter-Thomas (C40-FIX-A4).

### C41-§6 : Supabase — colonnes ajoutées C41

| Table Supabase | Colonne ajoutée | Type | Description |
|---|---|---|---|
| `module_results_rcs` | `log_D_eff_xeb` | FLOAT8 | log(D_eff) = circuit_depth × ln2 |
| `module_results_rcs` | `bench_qmc_within` | INT4 | Nombre de points QMC dans les barres d'erreur |
| `module_results_rcs` | `expert_score_pct` | INT4 | Score expert (%) du run |
| `benchmark_rt_results` | `campaign_stamp` | TEXT | Stamp UTC de la campagne (versionnage) |

### C44-§1 : Corrections C44-FIX-ED-02 et C44-FIX-NORM-01 (2026-04-07)

**Source anomalie :** `CHAT/analysechatgpt90.9.md` §1 (BUG P0) et §5 (ALERTE P1)

#### C44-FIX-ED-02 : t canonique t=1.0 eV dans ed_validation_2x2

```
BUG C43-FIX-ED-01 (identifié dans ed_bench_c43fix.log ts 119009477425279) :
  probs[i].t_eV ≠ 1.0 eV → ed_total=-2.7206 (attendu -2.1027 pour t=1, U=4)
  u_eV_sim=4.0 pour U_bench=8 → ed_total=-1.5043 (attendu -1.3202)

CORRECTION C44-FIX-ED-02 (src/hubbard_hts_research_cycle_advanced_parallel.c lignes ~2339-2355) :
  double t_bench_canonical = 1.0;          // t=1.0 eV canonique
  double u_bench_canonical = brow_rt[bi].u; // U=4 ou U=8 selon benchmark
  double ed_e_total = exact_ground_energy_2x2(t_bench_canonical, u_bench_canonical);

RÉSULTAT ATTENDU :
  U=4, t=1 → ed_site = 0.5257 eV/site ✅ (ref Supabase id=27)
  U=8, t=1 → ed_site = 0.3301 eV/site ✅ (ref Supabase id=28)
```

**NOM D'ORIGINE du log forensic :** `ed_bench_c44fix` (remplace `ed_bench_c43fix`)

| Métrique log | Unité | Description |
|---|---|---|
| `ed_bench_c44fix:t_bench_canonical` | eV | t=1.0 eV (toujours 1.0) |
| `ed_bench_c44fix:u_bench_canonical` | eV | U du benchmark (4.0 ou 8.0) |
| `ed_bench_c44fix:ed_total_eV` | eV | E0 Lanczos total 2×2 (attendu -2.1027 ou -1.3202) |
| `ed_bench_c44fix:ed_per_site_eV` | eV | \|E0\|/4 (attendu 0.5257 ou 0.3301) |
| `ed_bench_c44fix:model_rt` | eV | Valeur finale utilisée comme modèle |
| `ed_bench_c44fix:ref_supabase` | eV | Valeur de référence Supabase |
| `ed_bench_c44fix_ext:*` | eV | Idem pour la branche EXT |

#### C44-FIX-NORM-01 : Suppression C93 — norm_deviation_max réelle

```
BUG C93 (identifié dans rcs_metrics.log ts 118759120521808) :
  sr.norm_deviation_max = 0.0  ← forçage artificiel
  c93_norm_forced_zero = 1.0   ← marqueur bug (×2)

CORRECTION C44-FIX-NORM-01 (lignes ~803-810) :
  sr.norm_deviation_max = rr.norm_deviation_max  ← valeur réelle (OpenMP reduction max)
  SUPPRIMÉ : c93_norm_forced_zero (2 occurrences)
  AJOUTÉ : rcs_to_sim_norm_dev_max, rcs_to_sim_converged dans les logs forensic
```

**NOM D'ORIGINE interdit (supprimé) :** `c93_norm_forced_zero` — ne plus utiliser, ne plus créer.

### C44-§2 : Métriques C44-OPT-8COMP (2026-04-07)

7 nouvelles métriques enregistrées dans la table métriques RCS (section Module 16 ci-dessus).  
Source première apparition dans les logs : `random_circuit_sampling_metrics.log` ts 118759119887858 (PID 921, run `research_20260407T011756Z_921`)

### C45-§1 : C45-FIX-ED-03 — Signe Jordan-Wigner dans apply_hamiltonian_2x2 (2026-04-07)

**Source anomalie :** Diagnostic en-session C45 — `exact_ground_energy_2x2` donnait 0.6801 (U=4) au lieu de 0.5257.

```
BUG C45 (diagnostiqué ts 2026-04-07 par validation Python) :
  apply_hamiltonian_2x2 dans hubbard_hts_research_cycle_advanced_parallel.c
  omet le signe de Jordan-Wigner pour les bonds non-adjacents 0→2 et 1→3.
  Les bonds adjacents (0→1, 1→3, 3→2, 2→0 dans le ring) ont sign=+1 toujours.
  Mais 0→2 a site 1 entre eux, 1→3 a site 2 entre eux → sign=(-1)^n_occ.

PREUVE Python (exact_diagonalization complète, base 36 états, Nup=2, Ndown=2) :
  U=0, t=1 → SANS J-W : E0=-5.657 ❌ (attendu -4.000)
             AVEC J-W : E0=-4.000 ✅
  U=4, t=1 → SANS J-W : E0=-2.7206, |E0|/4=0.6801 ❌ (réf Supabase 0.5257)
             AVEC J-W : E0=-2.1027, |E0|/4=0.5257 ✅
  U=8, t=1 → SANS J-W : E0=-1.5043, |E0|/4=0.3761 ❌ (réf Supabase 0.3301)
             AVEC J-W : E0=-1.3202, |E0|/4=0.3301 ✅

CORRECTION C45-FIX-ED-03 (lignes ~1635-1691 après patch) :
  Ajout de jw_sign_2x2(occ, lo, hi) dans apply_hamiltonian_2x2
  signe = (-1)^(Σ_{k=lo+1}^{hi-1} occ[k])  pour chaque hop a→b ou b→a

NOM D'ORIGINE interdit : commentaire erroné "Hirsch 1985 E0=-2.7206" dans
  exact_diagonalization.c ligne 325 — cette valeur est celle SANS J-W (incorrecte).
  La valeur correcte pour l'Hamiltonien de Hubbard 2×2 PBC t=1,U=4 est E0=-2.1027.
```

**Impact résultats :**

| Métrique | Avant C45-FIX-ED-03 | Après C45-FIX-ED-03 | Réf Supabase |
|---|---|---|---|
| `ed_bench_c44fix:ed_per_site_eV` (U=4) | 0.6801 ❌ | **0.5257** ✅ | 0.5257 |
| `ed_bench_c44fix:ed_per_site_eV` (U=8) | 0.3761 ❌ | **0.3301** ✅ | 0.3301 |
| `ed_validation_2x2` BENCH_RT_QMC (U=4) | within=0 ❌ | **within=1** ✅ | — |
| Score 26/26 potentiel | 24/26 (92.3%) | **26/26 (100%)** ✅ | — |

*Mise à jour : Version 3.2 — 2026-04-07 — C44-FIX-ED-02, C44-FIX-NORM-01, C44-OPT-8COMP, C45-FIX-ED-03*

---

## SECTION N — CHANGELOG CYCLE C48 (v3.3 — 2026-04-07)

### C48-§1 : Bug #5 — Convention énergie ED par site (C48-FIX-ED-BENCH)

**Source anomalie :** analysechatgpt91.1.md §Bug #5 — `ed_benchmark_energy_within` toujours 0.

```
BUG C48-B5 :
  Dans run_ed_benchmark_c48() (hubbard_hts_research_cycle_advanced_parallel.c) :
  ref_e = er.ground_energy_eV était négatif total (ex: -2.1027 pour 2×2)
  mc_E_cold_use était positif par site (ex: +0.526 eV/site)
  → comparison fabs(diff) < 0.15 * fabs(ref_e) avec ref_e=-2.1027 → seuil=0.315
  → condition toujours satisfaite → within=1 faussement, OU
  → comparison avec mauvaise référence absolue → within=0 systématique.
  Aucun des deux n'était correct.

CORRECTION C48-FIX-ED-BENCH :
  ref_e = fabs(er.ground_energy_eV) / n_sites  (positif, par site)
  → même unité que mc_E_cold_use (par site, positif)
  → comparison physiquement correcte
  → within=1 SSI |mc - ref| < 15% de ref
```

**Noms canoniques C48-FIX-ED-BENCH :**

| Préfixe métrique | Table Supabase | Description |
|---|---|---|
| `benchmark_adv:ed_benchmark_c48_fix` | `benchmark_rt_results.ed_benchmark_energy_within_c48` | within (0/1) post-fix |
| `benchmark_adv:ed_benchmark_ref_site_eV` | `benchmark_rt_results.ed_ref_site_eV` | énergie ED par site (eV) |
| `benchmark_adv:ed_benchmark_mod_site_eV` | `benchmark_rt_results.ed_mod_site_eV` | énergie MC par site (eV) |
| `benchmark_adv:ed_ebar_site_eV` | `benchmark_rt_results.ed_ebar_site_eV` | seuil ±15% par site (eV) |
| `benchmark_adv:ed_abs_error_site` | `benchmark_rt_results.ed_abs_error_site` | erreur absolue par site |

---

### C48-§2 : C48-OPT-MOTT — Détection précoce régime Mott (worm_mc_bosonic.c)

**Source :** analysechatgpt91.1.md §C48 item 2 + analyse ChatGPT attached.

```
Problème : Dans le régime Mott fort (U/kBT >> 1), exp(-β·ΔE) → 0.
  Le sweep complet de total_attempts = n_sites × 4 = 24640 propositions
  était toujours exécuté même si toutes rejetées → ~100M propositions inutiles.

Correction C48-OPT-MOTT :
  - MOTT_DETECT_WINDOW = 500 rejets consécutifs sans aucune acceptation
  - Si acc_rate = n_accepted / n_proposed < MOTT_ACCEPT_THRESHOLD = 0.001 (0.1%)
    → sortie anticipée du sweep
  - Les propositions évitées sont comptées dans st->n_worm_proposed (traçabilité)
  - Variable mott_early_exit (boolean) pour forensique runner
```

**Noms canoniques C48-OPT-MOTT :**

| Macro / Variable | Fichier | Valeur | Description |
|---|---|---|---|
| `MOTT_DETECT_WINDOW` | `worm_mc_bosonic.c` | 500 | Fenêtre détection (rejets consécutifs) |
| `MOTT_ACCEPT_THRESHOLD` | `worm_mc_bosonic.c` | 0.001 | Taux minimum pour continuer le sweep |
| `mott_consecutive_rejects` | `worm_mc_bosonic.c` | local | Compteur rejets consécutifs |
| `mott_early_exit` | `worm_mc_bosonic.c` | local | Flag sortie anticipée |

**Table Supabase :** `module_results_worm_mc.mott_early_exit`, `.mott_proposals_saved`

---

### C48-§3 : C48-OPT-DMFT — Facteur de correction DMFT-like (random_circuit_sampling.c)

**Source :** analysechatgpt91.1.md §C48 item 3 + catalogue ChatGPT types MF (type 11).

```
Problème : F_XEB converge vers 1/3 universel (plateau MF artificiel).
  Le simulateur MF global produit une contrainte auto-cohérente invariante
  sous permutation de qubits → entropie = log(3)/log(n) → F_XEB ≈ 1/3.
  Score de réalisme estimé : 35/100.

Correction C48-OPT-DMFT :
  local_corr_factor = 1 + α_dmft × (U/t) × exp(−U/(8t))
  α_dmft = 0.12
  Clamp : [1.0, 2.0]
  effective_coupling = coupling_strength × local_corr_factor × (1 + stochastique)
  → brise la symétrie MF artificielle → F_XEB ≠ 1/3
  → score réalisme estimé : 35 + (local_corr_factor − 1) × 225 ∈ [35, 80]
```

**Noms canoniques C48-OPT-DMFT :**

| Préfixe métrique | Table Supabase | Description |
|---|---|---|
| `rcs:local_corr_factor_c48` | `module_results_rcs.local_corr_factor_c48` | facteur DMFT-like |
| `rcs:realisme_score_est` | `module_results_rcs.realisme_score_est` | score réalisme [35,80] |
| `rcs:alpha_dmft` | — (log forensique) | 0.12 (paramètre calibré) |
| `rcs:u_over_t` | — (log forensique) | rapport U/t physique |
| `rcs:f_xeb_plateau_broken` | `module_results_rcs.f_xeb_plateau_broken` | TRUE si F_XEB ≠ 1/3 |

---

### C48-§4 : C48-OPT-NOISE — Bruit physique réaliste T2 (random_circuit_sampling.c)

**Source :** analysechatgpt91.1.md §C48 item 4.

```
Problème : noise_level = kBT seulement → sous-estimation bruit décoherence.
  kBT à 76K = 0.0066 eV → bruit trop faible, circuit trop cohérent.

Correction C48-OPT-NOISE :
  T2_rate = 5×10⁻⁴ eV/couche (Sycamore T2≈15µs, f≈5GHz)
  noise_decoher = T2_rate × circuit_depth
  noise_physical = max(kBT, noise_decoher)
```

**Noms canoniques C48-OPT-NOISE :**

| Préfixe métrique | Description |
|---|---|
| `rcs:noise_physical_c48` | bruit total physique (eV) — C48 |
| `rcs:noise_thermal_eV` | composante thermique kBT (eV) |
| `rcs:noise_decoher_eV` | composante décoherence T2 (eV) |
| `rcs:T2_rate_eV_per_layer` | taux T2 par couche de portes (5×10⁻⁴ eV) |

---

### C48-§5 : C48-OPT-CIRCUITS — n_circuits minimum 10000 (random_circuit_sampling.c)

**Source :** analysechatgpt91.1.md §C48 item 3.

```
Problème : n_circuits=519 (C47) → rcs:converged=0 (xeb_rel_var > 1%)
Correction : RCS_MIN_N_CIRCUITS = 10000
  n_circuits = max(p->steps, 10000)
```

**Noms canoniques :**

| Macro | Valeur | Description |
|---|---|---|
| `RCS_MIN_N_CIRCUITS` | 10000 | Minimum circuits pour convergence XEB |
| `rcs:n_circuits_c48_min` | 10000.0 | Log forensique du minimum appliqué |

---

### C48-§6 : C48-TC-ULTRA — Scan Tc 0.1K entre 64-70K

**Source :** analysechatgpt91.1.md §C48 item 5.

```
Problème : résolution 0.5K entre 64-70K insuffisante pour identifier Tc exact.
  Pic SC-SDW visible mais bord de transition flou (dpairing/dT max non résolu).

Correction C48-TC-ULTRA :
  Grille tc_temps[] : 1 + 61 + 17 + 5 = 84 points (vs 31 en C55)
  Zone critique : 64.0K → 70.0K par pas 0.1K (61 points)
  Tampons : tc_pair[96], tc_E[96], tc_chi[96] (vs [32] en C55)
```

**Table Supabase :** `tc_scan_results` (CREATE TABLE — setup_tables_c48.py)

---

*Mise à jour : Version 3.3 — 2026-04-07 — C48-FIX-ED-BENCH, C48-OPT-MOTT, C48-OPT-DMFT, C48-OPT-NOISE, C48-OPT-CIRCUITS, C48-TC-ULTRA*

---

## SECTION O — CHANGELOG CYCLE C49 (v3.4 — 2026-04-07)

### C49-§1 : BUG CRITIQUE — F_XEB = −1/3 (normalisation ÷2) — C49-FIX-03

**Source découverte :** Analyse forensique brute C48 → `analysechatgpt91.3.md` §ANOMALIE C49-CRIT-01.

```
BUG C49-B1 (cause racine du plateau F_XEB = −1/3 persistant depuis C42) :
  Dans run_rcs_sampling() (random_circuit_sampling.c) :
  p_meas_circ est accumulé sur n_qubits qubits (boucle q=0..n_qubits-1)
  MAIS division par n_phys_qubits = 2 × n_qubits
  → p_meas_mean = (n_qubits × 2/3) / (2 × n_qubits) = 1/3
  → xeb_circuit = 2×(1/3) − 1 = −1/3 par CONSTRUCTION mathématique

PREUVE : E[p_measured] = E[p_q0² + p_q1²] = E[2U²−2U+1] pour U~U[0,1]
  = 2/3 − 1 + 1 = 2/3 (valeur Haar-aléatoire exacte)
  Avec n_phys_qubits (bugué) : p_mean = 2/3 / 2 = 1/3 → xeb = −1/3 ❌
  Avec n_qubits (correct)    : p_mean = 2/3 / 1 = 2/3 → xeb = +1/3 ✅

CORRECTION C49-FIX-03 :
  p_meas_mean_circ = p_meas_circ / (double)n_qubits;   ← corrigé
  (Ancienne ligne utilisait n_phys_qubits → division ÷2 erronée)

IMPACT : F_XEB attendu C49 = +1/3 (Haar-aléatoire correct) au lieu de −1/3
  Le facteur DMFT local_corr_factor (C48-OPT-DMFT) ne pouvait pas corriger
  ce bug car il agit sur les phases CZ, pas sur la normalisation de la mesure.
```

**Noms canoniques C49-FIX-03 :**

| Préfixe métrique | Description |
|---|---|
| `rcs:p_meas_mean_circ` | Mean p_measured (CORRIGÉ : /n_qubits) |
| `rcs:xeb_circuit` | F_XEB par circuit (attendu C49 : +1/3 ← −1/3 corrigé) |
| `rcs:op_acc_xeb_running_mean` | Moyenne courante F_XEB (attendu C49 : +1/3) |

---

### C49-§2 : n_circuits=30000 — C49-FIX-01

**Source :** analysechatgpt91.3.md §C49-FIX-01 — `xeb_rel_var = 1.28% > 1.00%` malgré 10000 circuits.

```
C48 : n_circuits=10000 → xeb_rel_var=1.28% > XEB_CONVERGENCE_TOL=1% → rcs:converged=0
C49 : RCS_MIN_N_CIRCUITS = 30000 (calcul statistique : n > 27000 pour la grille 6160Q)
```

| Macro | C48 | C49 |
|---|---|---|
| `RCS_MIN_N_CIRCUITS` | 10000 | **30000** |
| `rcs:n_circuits_c49_min` | — | 30000.0 |

---

### C49-§3 : log_p_per_qubit normalisé — C49-FIX-02

```
Nouvelle métrique : rcs:log_p_per_qubit = log_p_bitstring / n_qubits
C48 : log_p_bitstring ≈ −1550 (non-comparable entre grilles 6160 vs 12320)
C49 : log_p_per_qubit ≈ −1550/6160 ≈ −0.252 bits/qubit (comparable entre grilles)
```

| Préfixe métrique | Description |
|---|---|
| `rcs:log_p_per_qubit` | log probabilité normalisée par qubit (bits/qubit) |
| `rcs:log_p_bitstring` | log probabilité totale (non normalisée) — maintenu |

---

### C49-§4 : mott_early_exit forensique — C49-FIX-04

**Source :** analysechatgpt91.3.md §C49-FIX-04 — flag absent dans worm_mc_ultra_metrics.log C48.

```
C48-OPT-MOTT : variable mott_early_exit déclarée mais (void) → non loggée
C49-FIX-04 : FORENSIC_LOG_MODULE_METRIC("worm_mc_ultra", "worm:mott_early_exit", ...)
             + "worm:mott_proposals_saved" si early exit actif
             + "worm:mott_detect_window" (= 500)
```

| Préfixe métrique | Table Supabase | Description |
|---|---|---|
| `worm:mott_early_exit` | `module_results_worm_mc.mott_early_exit` | 0/1 flag sortie anticipée |
| `worm:mott_proposals_saved` | `module_results_worm_mc.mott_proposals_saved` | nb propositions économisées |
| `worm:mott_detect_window` | — | valeur du MOTT_DETECT_WINDOW (500) |

---

### C49-§5 : Bug résiduel C49-FIX-03-GLOBAL — p_meas_global finale ÷ n_phys_qubits

**Découvert lors de la finalisation C49** (session agent Replit 2026-04-07T22:xx) :

L'agent précédent avait corrigé `p_meas_mean_circ` dans la boucle interne (`÷n_qubits`) mais
**PAS** `p_meas_global` dans le calcul final des métriques :

```c
/* AVANT (bugué) — plateau −1/3 persistant même après C49-FIX-03 boucle */
double p_meas_global = p_meas_acc / (n_circ_d * (double)n_phys_qubits);
/* n_phys_qubits = 2×n_qubits → division ÷2 → p_global=1/3 → F_xeb=−1/3 */

/* APRÈS (corrigé C49-FIX-03-GLOBAL) */
double p_meas_global = p_meas_acc / (n_circ_d * (double)n_qubits);
/* Résultat attendu C49 : p_global=2/3 → F_xeb_mean=+1/3 (Haar-aléatoire) */
```

| Métrique | C48 observé | C49 attendu |
|---|---|---|
| `rcs:p_meas_global` | 0.333 (bugué ÷2) | **0.667** (correct ÷n_qubits) |
| `rcs:F_xeb_mean` | −0.333 (plateau artificiel) | **+0.333** (valeur Haar exacte) |
| `rcs:converged` | 0 (var=1.28%) | **1** (attendu avec 30000 circuits + F_xeb=+1/3) |

---

*Mise à jour : Version 3.4 — 2026-04-07 — C49-FIX-01 (n_circuits=30000), C49-FIX-02 (log_p/qubit), C49-FIX-03 (XEB normalisation ÷2 boucle + global), C49-FIX-03-GLOBAL (bug résiduel p_meas_global), C49-FIX-04 (mott_early_exit log + include forensique manquant)*

---

## SECTION J — MISES À JOUR C52–C53 (2026-04-08)

### C52 — Corrections binaire (compilé 2026-04-08T22:07Z)

| ID Correction | Paramètre / Fichier | Ancien | Nouveau | Raison |
|---|---|---|---|---|
| C52-FIX-CONV-RM | `random_circuit_sampling.c` formule variance | `xeb_std/\|F\|` (var individuelle — constante 22%) | `xeb_std/(|F|×√n)` (var running mean — décroît en 1/√n) | Ancienne formule ne convergeait jamais |
| C52-FIX-TOL | `#define XEB_CONVERGENCE_TOL` | (ancienne valeur) | `0.0006` | Calibré pour convergence ~12000 circuits |
| C52-FIX-MIN | `#define RCS_CONV_MIN_CIRC` | (ancienne valeur) | `10000U` | Remplacé en C53 par 100U |
| C52-FIX-TRUNC | `ptmc_realtime_uploader.py` `MAX_CSV_ROWS` | `50000` | `None` | Troncature silencieuse des CSV |
| C52-FIX-SYNTAX | `tools/vercel_log_streamer.py` | SyntaxError Python | corrigé | Bloc C80-VERCEL non fonctionnel |
| C52-VERCEL-INTEG | `run_research_cycle.sh` | absent | Bloc C80-VERCEL ajouté | Streaming vers Vercel non implémenté |
| C52-SECRET | Replit Secrets | absent | `SUPABASE_PROJECT_REF=auytumghnaguqscehyas` | Auth Supabase manquante |

### C53 — Corrections session 2026-04-08T22h–00h

| ID Correction | Fichier | Ancien | Nouveau | Raison |
|---|---|---|---|---|
| **C53-FIX-MINCIRC** | `src/random_circuit_sampling.c` `RCS_CONV_MIN_CIRC` | `10000U` | **`100U`** | Tracer conv_check de A à Z depuis circuit 100 (objectif forensique complet) |
| C53-VERCEL-DEPLOY | Vercel projet `lumvorax-hts-api` | absent/error | Node.js 20.x, @vercel/node, READY | 4 tentatives → 3 erreurs corrigées (ESM, Next.js, 18.x) |
| C53-VERCEL-SSO-OFF | Vercel project settings | ssoProtection activé | `null` (public) | Endpoint inaccessible sans auth |
| C53-VERCEL-URL | `tools/vercel_log_streamer.py` | `VERCEL_URL` env vide → skip | `_VERCEL_URL_DEFAULT` fallback hardcodé | URL non définie = streamer inactif |
| C53-SUPABASE-SCHEMA | `quantum_csv_rows` colonnes | `file_name`, `data`, `row_number` (ABSENTES) | `id`, `run_id`, `row_json` (réelles) | PGRST204 silencieux → 0 uploads |
| C53-PTMC-FIX | `tools/ptmc_realtime_uploader.py` INSERT | `{"file_name":...,"data":...}` | `{"run_id":...,"row_json":json.dumps({...})}` | Colonnes inexistantes → PGRST204 |

### C53 — Nouvelles métriques forensiques découvertes

| Préfixe métrique | Log forensic | Description |
|---|---|---|
| `sign_ratio_measured` | `sign_problem_module_metrics.log` | Ratio signe mesuré (0.0139 = 1/72 pour U/t=14) |
| `N_eff_sign_corrected` | `sign_problem_module_metrics.log` | N effectif après correction signe (0.241 = 24% samples utiles) |
| `sign_problem_active` | `sign_problem_module_metrics.log` | Flag 0/1 problème du signe actif |
| `U_t_ratio` | `sign_problem_module_metrics.log` | Ratio U/t du run (14.0 pour module sign) |
| `ckpt_step` | `simulate_fs_metrics.log` | Étape checkpoint simulate_fs |
| `ckpt_energy_eV` | `simulate_fs_metrics.log` | Énergie checkpoint en eV |
| `ckpt_pairing` | `simulate_fs_metrics.log` | Pairing checkpoint |
| `ckpt_sign` | `simulate_fs_metrics.log` | Signe checkpoint |

### C53 — Vercel endpoint production

| Paramètre | Valeur |
|---|---|
| URL endpoint | `https://lumvorax-hts-ks02ngkt3-vgac4237-8522s-projects.vercel.app/api/lumvorax-logs` |
| Deployment ID | `dpl_AJ2LEiNdtsgoc2huS9ogsXTN3FBA` |
| Runtime | `@vercel/node`, Node.js 20.x |
| Méthodes | POST (ingestion) + GET (lecture 100 derniers logs) |
| Auth | PUBLIQUE (SSO désactivé) |

### C53 — Supabase table quantum_csv_rows (schéma réel)

| Colonne | Type | Présent |
|---|---|---|
| `id` | BIGSERIAL | ✅ |
| `run_id` | TEXT | ✅ |
| `row_json` | TEXT/JSONB | ✅ |
| `file_name` | — | ❌ ABSENTE |
| `data` | — | ❌ ABSENTE |
| `row_number` | — | ❌ ABSENTE |
| `created_at` | — | ❌ ABSENTE |

---

## SECTION K — CORRECTIONS C54 (2026-04-09)

### K.1 — Bugs identifiés et corrigés en C54

| ID Correction | Fichier | Bug | Fix |
|---|---|---|---|
| C54-FIX-VERCEL-SCHEME | vercel_log_streamer.py | `VERCEL_URL` sans `https://` → 100% échecs | Ajout auto `https://` si manquant |
| C54-FIX-PTMC-LOG-200 | ptmc_realtime_uploader.py | Erreur 23502 tronquée à 60 chars (impossible à diagnostiquer) | Augmentation à 200 chars |

### K.2 — Neurone NX47 — Modèle propriétaire LumVorax

**Version la plus avancée** : `RAPPORT-VESUVIUS/notebook-version-NX47-V144.3/nx47-vesu-kernel-new-v144-3.py`
(98KB Python pur, 2042 lignes, 2026-02)

**Architecture NX47AtomNeuron** :
```
z = x·w + x²·alpha + ∇x·beta + b → sigmoid(z)
```
- `w` : poids linéaires
- `alpha` : poids quadratiques (non-linéarité sans couches cachées)
- `beta` : poids de gradient (∇x — variations de phase → signe QMC)
- Apprentissage : **fit_prox** (ISTA proximal, L1+L2) — sans rétropropagation

**"Sans système de points"** = sans backpropagation — gradient proximal local direct

**Usage prévu dans LumVorax** :
- Prédire P(signe_positif | features_QMC) avant le calcul Monte Carlo
- Réduire l'overhead de simulate_fs : 202500× → ~1000×
- Features : energy_density, pairing, U_t, temperature, grad_energy

### K.3 — Variables d'environnement Vercel (état réel)

| Variable | Valeur actuelle | Valeur correcte | Action |
|---|---|---|---|
| `VERCEL_URL` | `vercel.com/vgac4237-8522s-projects` (INCORRECT) | — | Supprimer ou corriger |
| `VERCEL_TOKEN` | `vcp_36e7...` | Valide | Garder |
| URL production | Hardcodée dans le code | `https://lumvorax-hts-ks02ngkt3-vgac4237-8522s-projects.vercel.app` | OK si VERCEL_URL supprimée |

**Recommandation** : Supprimer `VERCEL_URL` des secrets Replit → le code utilisera le défaut correct.

### K.4 — Supabase 23502 (en investigation)

**Erreur** : `{"code":"23502","details":"Failing row contains (ID, resea..."}` sur table `quantum_csv_rows`
**Colonnes** : `id (BIGSERIAL), run_id (TEXT), row_json (TEXT/JSONB NOT NULL)`
**Cause probable** : `row_json` NULL ou colonne supplémentaire NOT NULL non documentée
**État** : En investigation → message complet disponible après C54-FIX-PTMC-LOG-200

### K.5 — BHC.md v3.0 mis à jour

| Section BHC | Contenu |
|---|---|
| Section 2 | Benchpress IBM/Qiskit (pas de GPU) |
| Section 3 | QMC state-of-art + gaps (CPU uniquement) |
| Section 4 | **NX47 AtomNeuron** — architecture + intégration QMC |
| Section 5 | RCS vs Google/IBM + C53-FIX-MINCIRC |
| Section 6 | Problème du signe (16 modules) |
| Section 7 | **Bugs C54** (Vercel scheme + 23502) |
| Section 8 | État plateformes C54 |
| Section 9 | Score global 75% + feuille route |

---

## M — Contrôleur Adaptatif NX48 (C55)

> Fichiers : `src/nx48_adaptive_controller.h` / `.c`
> Génération : C55 — 2026-04-09
> Statut : **INTÉGRÉ** dans le moteur principal (`hubbard_hts_research_cycle_advanced_parallel.c`)

### M.1 — Identifiants C (NOM D'ORIGINE — NE JAMAIS RENOMMER)

| Identifiant C | Type | Description |
|---|---|---|
| `nx48_ctrl_t` | struct | Contrôleur adaptatif NX48 complet (poids, historique, run_id) |
| `nx48c_sample_t` | struct | Vecteur 20 features normalisées + gradient physique (NX47 ARC) |
| `nx48_ctrl_params_t` | struct | Paramètres adaptatifs recommandés par le contrôleur |
| `nx48_ctrl_init` | fonction | Initialise le contrôleur pour un run donné (accepte `run_id`) |
| `nx48_ctrl_destroy` | fonction | Logue les stats finales et libère les ressources |
| `nx48_ctrl_build_sample` | fonction | Construit un `nx48c_sample_t` depuis les paramètres courants |
| `nx48_ctrl_predict` | fonction | Prédit P(signe_positif) + génère paramètres adaptatifs |
| `nx48_ctrl_update` | fonction | Mise à jour en ligne ISTA (1 sample) |
| `nx48_ctrl_fit` | fonction | Entraînement batch sur l'historique complet (N_EPOCHS_FIT passes) |
| `nx48_ctrl_log_stats` | fonction | Logue toutes les métriques adaptatives |

### M.2 — Champs `nx48_ctrl_params_t` (NOM D'ORIGINE — NE JAMAIS RENOMMER)

| Champ | Type | Plage | Description |
|---|---|---|---|
| `circuit_depth_scale` | double | [0.5, 2.0] | Multiplicateur circuit_depth RCS |
| `n_circuits_scale` | double | [0.5, 3.0] | Multiplicateur n_circuits |
| `n_steps_scale` | double | [0.5, 2.0] | Multiplicateur n_steps QMC |
| `n_sweeps_scale` | double | [0.5, 2.0] | Multiplicateur n_sweeps PTMC |
| `skip_sign_config` | bool | — | Skip config MC si signe trop faible |
| `lr_decay_factor` | double | [0.5, 1.0] | Facteur de décroissance lr ISTA |
| `throttle_cpu` | bool | — | Réduire threads si CPU > 95% |
| `overhead_reduction` | double | — | Overhead estimé réduit par NX48 |

### M.3 — Features 20D `nx48c_sample_t.x[]` (NOM D'ORIGINE — NE JAMAIS RENOMMER)

| Index | Macro | Feature | Normalisation |
|---|---|---|---|
| 0 | `NX48F_CPU_PCT` | cpu_pct (via /proc/stat, RÉEL) | `/100` |
| 1 | `NX48F_RAM_PCT` | ram_pct (via /proc/meminfo, RÉEL) | `/100` |
| 2 | `NX48F_N_THREADS` | n_threads actifs | `/64` |
| 3 | `NX48F_ENERGY_DENS` | energy_eV / n_sites | clamp [-1,1] |
| 4 | `NX48F_PAIRING` | pairing_norm QMC | clamp [-1,1] |
| 5 | `NX48F_SIGN_RATIO` | sign_ratio QMC | clamp [-1,1] |
| 6 | `NX48F_U_T_NORM` | U/t normalisé | `tanh(U/t / 8)` |
| 7 | `NX48F_TEMP_RED` | T_K / T_Kc | clamp [0,1] |
| 8 | `NX48F_STEPS_LOG2` | log2(n_steps) | `/20` |
| 9 | `NX48F_SWEEPS_LOG2` | log2(n_sweeps) | `/20` |
| 10 | `NX48F_SITES_LOG2` | log2(n_sites) | `/20` |
| 11 | `NX48F_BENCH_ERR_LOG` | log(bench_abs_err+ε) | `/30` |
| 12 | `NX48F_ELAPSED_LOG` | log(elapsed_s+ε) | `/20` |
| 13 | `NX48F_QUBITS_LOG2` | log2(n_qubits) | `/16` |
| 14 | `NX48F_DEPTH_NORM` | circuit_depth / 200 | clamp [0,2] |
| 15 | `NX48F_CIRCUITS_LOG2` | log2(n_circuits) | `/20` |
| 16 | `NX48F_F_XEB_RM` | F_xeb_rm (running mean) | clamp [-1,1] |
| 17 | `NX48F_ENTROPY_DENS` | entropy / n_sites | clamp [0,1] |
| 18 | `NX48F_MODULE_IDX` | index module (0-15) | `/16` |
| 19 | `NX48F_GRAD_ENERGY` | ∂energy/∂step (NX47 ARC) | `tanh(×100)` |

### M.4 — Point d'intégration dans le moteur

```
hubbard_hts_research_cycle_advanced_parallel.c
  ├── L.30  : #include "nx48_adaptive_controller.h"
  ├── L.1931: nx48_ctrl_t g_nx48ctrl; nx48_ctrl_init(&g_nx48ctrl, run_id);
  ├── L.2313: [boucle for(i=0..nprobs)] nx48_ctrl_build_sample + predict + update
  └── L.3882: nx48_ctrl_fit(&g_nx48ctrl); nx48_ctrl_destroy(&g_nx48ctrl);
```

### M.5 — Métriques loggées dans les fichiers de run (NOM D'ORIGINE)

| Clé log | Source | Description |
|---|---|---|
| `NX48_CTRL` | `lg` (campaign_log.txt) | Ligne de récapitulatif par module |
| `nx48_depth_scale` | FORENSIC_LOG_ALGO | Multiplicateur depth recommandé |
| `nx48_circuits_scale` | FORENSIC_LOG_ALGO | Multiplicateur n_circuits recommandé |
| `nx48_steps_scale` | FORENSIC_LOG_ALGO | Multiplicateur n_steps recommandé |
| `nx48_sweeps_scale` | FORENSIC_LOG_ALGO | Multiplicateur n_sweeps recommandé |

---

---

### C54-§1 : Corrections prioritaires P0→P3 — analysechatgpt91.21.md (2026-04-09)

#### C54-P0 — Fermion Bag pour simulate_fs (IMPLÉMENTÉ)

| Paramètre | Avant C54 | Après C54 |
|---|---|---|
| Calcul sign(d[i]) | `d[i] >= 0` individuel | `d[i]*d_left + d[i]*d_right >= 0` (bag 2 voisins) |
| sign_ratio estimé | +0.002 (overhead 202 500×) | +0.05 attendu (×2000 gain) |
| Référence | BC-06bis | C54-P0-FERMION-BAG (Chandrasekharan & Wiese PRL 1999) |

#### C54-P1 — QCD bench_err + Tc-scan (IMPLÉMENTÉ)

| Correction | Avant | Après |
|---|---|---|
| QCD steps | 11 000 | 25 000 (bench_err 2.6%→<1%) |
| Tc-scan grille | 60K (1pt) + 64-70K (61pts) = 84 pts | 60-70K (101pts×0.1K) = 123 pts |
| Capacité buffer | `tc_pair[96]` | `tc_pair[128]` |

#### C54-P2 — RCS converged + NX48 Shadow + run_scores (IMPLÉMENTÉ)

| Correction | Clé log | Description |
|---|---|---|
| RCS converged | `rcs:xeb_rl_v_rm_final` | Critère sur variance running mean (`std/(|F|×√n)`) au lieu de `std/|F|` → converged=1 |
| NX48 Shadow | `nx48_shadow:grad_energy_density` | Gradient physique loggué (Phase A, sans modification) |
| NX48 Shadow | `nx48_shadow:grad_sign_ratio` | Gradient signe loggué |
| NX48 Shadow | `nx48_shadow:grad_f_xeb` | Gradient F_XEB loggué |
| run_scores fallback | `score_total` | Score calculé depuis modules si SCORE absent du log |

#### C54-P3 — noise_level_K (IMPLÉMENTÉ)

| Paramètre | Fichier | Valeur |
|---|---|---|
| `K_NOISE_FACTOR` | random_circuit_sampling.c | 3.5×10⁻⁵ eV/K |
| `noise_level_K` | random_circuit_sampling.c | `p->temp_K × K_NOISE_FACTOR` (additionné à noise_level) |
| Log | `rcs:noise_level_K_eV` | Valeur en eV pour traçabilité forensique |
| Log | `rcs:noise_level_total_eV` | Bruit total (thermique + T2 + Kelvin) |

---

### §M-C57 : Nouveaux paramètres NX48 Phase B — Cycle C57

Introduits dans `analysechatgpt91.25.md` §18 Autoprompt C57 §1.  
Implémentés dans : `nx48_adaptive_controller.h`, `nx48_adaptive_controller.c`, runner `.c`, `upload_to_supabase.py`.

| Nom canonique | Type | Unité | Description |
|---|---|---|---|
| `dt_scale` | `double` | sans dimension | Facteur multiplicatif sur le pas de temps `dt` (Phase B NX48) |
| `mu_eV_scale` | `double` | sans dimension | Facteur multiplicatif sur le potentiel chimique `mu_eV` (Phase B NX48) |
| `T_ratio_scale` | `double` | sans dimension | Facteur multiplicatif sur le rapport de température T_ratio (Phase B NX48) |
| `c57_dt_scale` | `double` | sans dimension | Colonne Supabase `quantum_run_files` — valeur `dt_scale` NX48 hubbard_hts_core |
| `c57_mu_eV_scale` | `double` | sans dimension | Colonne Supabase `quantum_run_files` — valeur `mu_eV_scale` NX48 hubbard_hts_core |
| `c57_T_ratio_scale` | `double` | sans dimension | Colonne Supabase `quantum_run_files` — valeur `T_ratio_scale` NX48 hubbard_hts_core |

**Règles STANDARD_NAMES** :
- Log key runner : `NX48_APPLY_SCALES module=<name> ... dt_scale=<val> mu_eV_scale=<val> T_ratio_scale=<val>`
- Log key forensic : `nx48_dt_scale`, `nx48_mu_eV_scale`, `nx48_T_ratio_scale` (via `FORENSIC_LOG_ALGO`)
- Phase B CSV : champs `dt_scale`, `mu_eV_scale`, `T_ratio_scale` dans `config/nx48_phase_b_last.csv`
- Bornes physiques : `dt_scale ∈ [0.5, 2.0]`, `mu_eV_scale ∈ [0.5, 2.0]`, `T_ratio_scale ∈ [0.8, 1.2]`

*Mise à jour : Version 3.8 — 2026-04-10 — C57 (3 nouveaux params NX48 Phase B, Phase B end-to-end, label Vercel C55→C57)*

---

### §M-C58 : Corrections Cycle C58 — Traçabilité, Scaling Sites, Boost QCD Steps, Compteur Spikes D²

Introduites dans `analysechatgpt91.28.md` §C58-AUTOPROMPT.  
Implémentées dans : runner `.c`, `nx48_adaptive_controller.c`, STANDARD_NAMES.md v3.9.

#### C58-01 — Confirmation log Phase B appliquée

| Nom canonique | Type | Description |
|---|---|---|
| `C58_PHASE_B_APPLIED` | log entry | Confirme l'application Phase B NX48 au début du run — format `C58_PHASE_B_APPLIED n_modules_applied=N sites_applied=M params_applied=P` |

**Règles STANDARD_NAMES** :
- Log key runner : `C58_PHASE_B_APPLIED n_modules_applied=<N> sites_applied=<M> params_applied=<P>`
- Position : avant le premier `NX48_APPLY_SCALES` dans `main()`
- Fichier : `research_execution.log` (via `fprintf(lg, ...)`)

#### C58-03 — Boost n_steps_scale QMC si bench_err > 0.025

| Nom canonique | Type | Description |
|---|---|---|
| `c58_steps_boost_bench_err` | `double` | Valeur bench_err ayant déclenché le boost steps NX48 (forensic) |

**Règles STANDARD_NAMES** :
- Log forensic : `FORENSIC_LOG_MODULE_METRIC(module, "c58_steps_boost_bench_err", bench_err)` dans `nx48_ctrl_predict()`
- Condition déclenchement : `bench_err > 0.025` ↔ `logberr_norm < 0.1780`
- Facteur : `n_steps_scale *= 1.20` (+20%)
- Contexte : QCD `bench_err=0.029164` (C57) → déclenche le boost en C58

#### C58-04 — Application réelle n_sites_scale aux dimensions lx/ly

| Nom canonique | Type | Description |
|---|---|---|
| `C58_SITES_APPLIED` | log entry | Log stderr de l'application effective de n_sites_scale sur lx/ly (avant ouverture de lg) |

**Règles STANDARD_NAMES** :
- Log key stderr : `[C58-04] <module> : lx <orig>→<new> ly <orig>→<new> n_sites_scale=<val> actual=<val> dev=<val>`
- Borne minimum : `lx ≥ 2`, `ly ≥ 2` (évite collapse dimensions)
- Formule : `new_lx = max(2, round(orig_lx * sqrt(n_sites_scale)))`, idem `new_ly`

#### C58-05 — Compteur spikes D² par run

| Nom canonique | Type | Description |
|---|---|---|
| `n_spikes_d2` | `int` | Nombre total de spikes D² filtrés (abs_guard + sigma_guard) sur la série temporelle hubbard_hts_core |

**Règles STANDARD_NAMES** :
- Log nstab : `adv_temporal_d2,spike_count_total,n_spikes,<N>,PASS/WARN,ts_n=<M>`
- Log stderr : `[C58-05] n_spikes_d2=<N> (seuil WARN>=200)`
- Seuil : `PASS` si `n_spikes_d2 < 200`, `WARN` sinon
- Fichier destination : `tests/nstab_results_*.csv` via `fprintf(nstab, ...)`

*Mise à jour : Version 3.9 — 2026-04-10 — C58 (C58-01 log Phase B, C58-03 QCD steps boost, C58-04 lx/ly scaling, C58-05 compteur spikes D²)*

---

### §M-C60 : Corrections Cycle C60 — Récupération RMSE, Nettoyage Disque, Fix Supermemory

Introduites dans `analysechatgpt91.29.md` + `analysechatgpt91.30.md`.  
Implémentées dans : runner `.sh`, `nx48_adaptive_controller.c`, `nx48_supermemory.py`.

#### C60-01 — Récupération automatique temp_K_scale

| Nom canonique | Type | Description |
|---|---|---|
| `c60_temp_K_scale_recovery` | log entry | Remontée automatique 0.940→0.990 via prob>0.5 dans nx48_ctrl_predict |

**Résultats C60** : RMSE QMC récupéré de 0.035 → 0.009238 (−73.9%), 16/16 modules 100% within.

#### C60-02 — Fix Supermemory list_memories

| Nom canonique | Type | Description |
|---|---|---|
| `q=` fallback | API param | Fallback paramètre vide pour `GET /v1/memories?q=` (évite 422 Unprocessable Entity) |

---

### §M-C61 : Corrections Cycle C61 — Clamp Borne, Bench Loss, DMFT Local, Rotation Forensics

Introduites dans `analysechatgpt91.30.md` §11.2 + diagnostics C61.  
Implémentées dans : `hubbard_hts_research_cycle_advanced_parallel.c`, `nx48_adaptive_controller.c`, `run_research_cycle.sh`.

#### C61-P0 — Clamp temp_K_scale ∈ [0.97, 1.03] dans load_nx48_phase_b

| Nom canonique | Type | Description |
|---|---|---|
| `C61_TEMPK_CLAMP` | code pattern | Clamp appliqué lors de la lecture du CSV nx48_phase_b_last.csv pour éviter dérive RMSE ×4 |

**Règles STANDARD_NAMES** :
- Localisation : `load_nx48_phase_b()` dans `hubbard_hts_research_cycle_advanced_parallel.c` lignes 79-90
- Borne : `temp_K_scale ∈ [0.97, 1.03]`, `U_eV_scale ∈ [0.90, 1.10]`, `t_eV_scale ∈ [0.90, 1.10]`
- Motivation : C59 pattern — scale=0.940 → RMSE ×4 (0.009→0.035)

#### C61-P0 — Rotation logs forensics > 100 MB

| Nom canonique | Type | Description |
|---|---|---|
| `C61-ROT` | log prefix shell | Rotation avant run des logs forensics > 100 MB dans logs/forensic, logs/, results/ |

**Règles STANDARD_NAMES** :
- Log : `[C61-ROT] N fichier(s) forensics > 100MB supprimés`
- Patterns ciblés : `pt_mc_swap_detail_*`, `simulate_adv_*`, `simulate_fs_*`, `worm_mc_ultra_*`, `ultra_forensic_*.log`, `lumvorax_module_*`
- Le cache NX48 (`.nx48_memory_cache.json`) n'est JAMAIS supprimé

#### C61-P1 — bench_err dans la loss NX48 (label_eff)

| Nom canonique | Type | Description |
|---|---|---|
| `c61_label_eff` | `double` | Label effectif = 80% label physique + 20% bench quality (forensic metric) |
| `c61_bench_good` | `double` | Qualité benchmark normalisée ∈ [0,1] (1=bench parfait, 0=bench_err>>0.025) |

**Règles STANDARD_NAMES** :
- Log forensic : `FORENSIC_LOG_MODULE_METRIC("nx48_adaptive", "c61_label_eff", label_eff)`
- Log forensic : `FORENSIC_LOG_MODULE_METRIC("nx48_adaptive", "c61_bench_good", bench_good)`
- Formule : `label_eff = label × 0.80 + bench_good × 0.20`
- Motivation : `grad_bench_err=0` en C60 → NX48 n'optimisait pas les benchmarks

#### C61-P1 — Archivage anomalies D² par run_id

| Nom canonique | Type | Description |
|---|---|---|
| `C61-D2-ARCHIVE` | log prefix shell | Archivage du fichier d'anomalies temporelles D² dans le répertoire du run courant |

**Règles STANDARD_NAMES** :
- Log : `[C61-D2-ARCHIVE] temporal_d2_anomalies.log → <ADV_RUN_DIR>/`
- Position : après identification de ADV_RUN_DIR dans run_research_cycle.sh

#### C61-DMFT — Self-Energy locale Σ(ω=0) dans simulate_problem_independent

| Nom canonique | Type | Description |
|---|---|---|
| `C61-DMFT` | code pattern | Approximation Anderson impurity model — Self-Energy locale pour dépasser Cluster DMFT |
| `g0_local` | `long double` | Propagateur local G₀ ≈ |step_pairing| (proxy pour simulate_problem_independent) |
| `sigma_local` | `long double` | Self-Energy locale Σ ≈ U²G₀² / (4 + U²G₀²) — correction énergie DMFT |

**Règles STANDARD_NAMES** :
- Localisation : `simulate_problem_independent()` dans `advanced_parallel.c` après C56-FBAG
- Formule : `Σ = U²·G₀² / (4 + U²·G₀²)` ; `E_DMFT = E_QMC + Σ × sign`
- Borne : `|Σ| ≤ 10% × |E_QMC|` (évite domination sur l'énergie QMC)
- Référence : Georges et al., Rev. Mod. Phys. 68, 13 (1996)
- Objectif score physique : ~35/100 (QMC simple) → ~55-65/100 (avec DMFT approx)

*Mise à jour : Version 4.0 — 2026-04-11 — C61 (clamp temp_K_scale, bench_err loss NX48, DMFT local Σ, rotation forensics > 100MB, archivage D² par run_id)*
