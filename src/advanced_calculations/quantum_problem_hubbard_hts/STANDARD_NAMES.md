# STANDARD_NAMES.md — Registre canonique des noms du projet LumVorax / Hubbard-HTS

**Version :** 2.0 — 2026-03-16 (correction C25-NAMES)
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
| `simulate_adv:` | Simulation HTS avancée (boucle RK2) | `elapsed_ns`, `steps`, `step_energy_eV`(C25), `step_pairing_norm`(C25), `step_sign_ratio`(C25), `step_elapsed_ns`(C25), `step_pairing_norm_step0`, `step_energy_norm_step0`, `local_pair_site0_step0`, `d_site0_step0`, `autocorr_tau_lower_steps`, `autocorr_tau_upper_steps`, `hilbert_log2_dim`(C59-P5), `sigma_mc_estimate`(C59-P4), `sigma_corr_sign`(C59-P4), `sign_abs_mean`(C59-P4) |
| `pt_mc_therm:` | Thermalisation PT-MC (800 sweeps × 26 modules) | `n_thermalize_sweeps`, `n_replicas`, `n_step_per_sweep`, `sw`, `mc_rate`, `swap_rate`, `delta_mc`, `E_cold` |
| `pt_mc:` | Récapitulatif PT-MC par module | `elapsed_ns`, `avg_mc_accept`, `chi_sc_avg`, `chi_sc_max`, `pairing_cold_avg`, `tau_int_energy`(C25-TAU), `err_corrected_energy_eV`(C25-TAU), `hilbert_log2_dim`(C59-P5), `sigma_mc_ptmc`(C59-P4), `sigma_corr_sign`(C59-P4), `sign_abs_mc_accept`(C59-P4), `sw`(C59-ULTRA), `mc_rate`(C59-ULTRA), `swap_rate`(C59-ULTRA), `delta_mc`(C59-ULTRA), `E_cold`(C59-ULTRA), `elapsed_ns`(C59-ULTRA) [via FORENSIC_LOG_NANO] |
| `pt_mc_swap_detail:` | Chaque tentative d'échange répliques | `sw`, `r_lo`, `r_hi`, `delta_beta_dE`, `swap_accepted` |
| `pt_mc_vs_mc:` | Anomalies divergence PT-MC vs MC simple | `energy_diff`, `pairing_diff` |
| `benchmark_adv:` | Benchmarks QMC/DMRG | `qmc_rmse`, `dmrg_mae`, `within_error_bar`, `ed_benchmark_energy_within`(C59-P2), `ed_benchmark_pairing_within`(C59-P2) |
| `ed_` (sous-préfixe) | Diagonalisation exacte (C24-02) | `ed_E0_eV`, `ed_gap_eV`, `ed_double_occ`, `ed_pairing_corr`, `ed_bethe_E0_eV`, `ed_rel_err_mc_pct`, `ed_converged`, `ed_lanczos_iter` |

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
| `benchmark` | Benchmarks QMC/DMRG avec références externes |
| `cluster_scale` | Multi-tailles réseau (8×8 → 255×255) |
| `thermodynamic_limit` | Extrapolation limite thermodynamique |

---

## SECTION H — VARIABLES D'ENVIRONNEMENT

| NOM D'ORIGINE | Valeur | Description |
|---|---|---|
| `PTMC_RUN_INDEX` | entier 0–N | Seeds déterministes distincts : XOR = N × φ64 (0x9e3779b97f4a7c15) |
| `PTMC_SEED_RANDOM` | `1` | Seed aléatoire basée sur time(NULL)^(pid<<32) |
| `LUMVORAX_ENABLED` | flag compilateur `-DLUMVORAX_ENABLED=1` | Active le bridge secondaire Lumvorax (distinct du logger principal) |

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

*Maintenu par :* Agent Replit  
*Version 1.0 :* 2026-03-16 — création initiale (erreur : LV_MODULE_METRIC désigné comme officiel)  
*Version 2.0 :* 2026-03-16 — correction C25-NAMES : FORENSIC_LOG_MODULE_METRIC = nom d'origine réel
*Version 3.0 :* 2026-03-17 — ajout Section I : Logger V4 NEXT (qf_log_*) — Cycle C33
