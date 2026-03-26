# AUTO-PROMPT — MODÈLE D’INSPECTION TOTALE (FORMAT LEGACY RESPECTÉ)
*(Conforme au format des anciens rapports CHAT: prompt en tête + sections inventaire/comparaison/audit/roadmap.)*

```text
Lis toi-même ligne par ligne tous les artefacts de runs et les rapports précédents dans CHAT/.
Ne modifie aucun ancien rapport.

NOUVEAUX RUNS:
  src/advanced_calculations/quantum_problem_hubbard_hts/results/research_20260312T222838Z_3447/
  src/advanced_calculations/quantum_problem_hubbard_hts/results/research_20260312T222246Z_2829/

ANCIENS RUNS DE RÉFÉRENCE:
  src/advanced_calculations/quantum_problem_hubbard_hts/results/research_20260312T182101Z_2854/
  src/advanced_calculations/quantum_problem_hubbard_hts/results/research_20260312T181131Z_2715/
  src/advanced_calculations/quantum_problem_hubbard_hts/results/research_20260312T075921Z_523/

CODE À AUDITER LIGNE PAR LIGNE:
  src/advanced_calculations/quantum_problem_hubbard_hts/run_research_cycle.sh
  src/advanced_calculations/quantum_problem_hubbard_hts/src/hubbard_hts_research_cycle.c
  src/advanced_calculations/quantum_problem_hubbard_hts/src/hubbard_hts_research_cycle_advanced_parallel.c

Sortie attendue:
1) inventaire fichiers apparus/disparus,
2) comparaison des observables et tests,
3) causes exactes des FAIL persistants,
4) bugs cachés de pipeline,
5) feuille de route P0/P1/P2,
6) commandes reproductibles exactes.
```

---

# RAPPORT D’ANALYSE TOTALE — 3447/2829 vs 2854/2715/523
**Date:** 2026-03-12 UTC  
**Méthode:** analyse manuelle + vérifications scriptées de cohérence (sans modifier les anciens rapports)

## EXPERTISES MOBILISÉES (AUTO-CRITIQUE)
- **Forensic integrity**: vérification de provenance/checksums/artefacts manquants.
- **Numerical stability**: interprétation des gates `energy_density_drift_max` et cohérence inter-runners.
- **Scientific benchmarking**: lecture QMC/DMRG, modules externes, cluster trends.
- **Pipeline engineering**: recherche de bugs cachés dans l’orchestration `run_research_cycle.sh`.

Auto-critique: l’erreur principale n’était pas seulement “physique”, mais aussi “ordre d’orchestration” (checksum généré avant dernières étapes), ce qui contaminait l’auditabilité.

## PARTIE I — INVENTAIRE DES ARTEFACTS (ÉVOLUTION)

### 1) 523 -> 2715
- **-72 fichiers** (audit/logs avancés absents), dont `logs/checksums.sha256`, dossiers `audit/*`, plusieurs intégrations tests.

### 2) 2715 -> 2854
- **+72 fichiers** (retour des artefacts forensic, audit complet, checksums).

### 3) 2854 -> 2829
- **-72 fichiers** + **+3 fichiers** (`logs/csv_schema_guard_summary.json`, `tests/integration_csv_schema_guard.csv`, `tests/unit_conversion_fullscale.csv`).

### 4) 2829 -> 3447
- **+72 fichiers** et **-1 fichier** (`tests/unit_conversion_fullscale.csv` retiré).

Conclusion inventaire: la couverture d’artefacts varie fortement selon run/moteur; la comparabilité brute est donc partielle si on ne normalise pas d’abord les prérequis d’intégrité.

## PARTIE II — TABLEAU COMPARATIF OBSERVABLES/TESTS

| Run | baseline rows | new_tests (PASS/FAIL/OBS) | numerical_stability (PASS/FAIL) | checksums |
|---|---:|---:|---:|---|
| 523 | 305 | 20 / 11 / 49 | 17 / 13 | présent mais invalide (5 refs manquantes) |
| 2715 | 305 | 20 / 11 / 49 | 30 / 0 | absent |
| 2854 | 305 | 20 / 11 / 49 | 30 / 0 | présent mais invalide (5 refs manquantes) |
| 2829 | 305 | 20 / 11 / 49 | 30 / 0 | absent |
| 3447 | 305 | 21 / 10 / 49 | 17 / 13 | présent mais invalide (5 refs manquantes) |

Observables (lecture manuelle):
- `2829` et `3447` ont mêmes moyennes baseline sur `energy/pairing/sign_ratio`.
- amélioration `2829 -> 3447`: `independent_calc` passe FAIL -> PASS.
- FAIL persistants durs: `qmc_dmrg_*`, `external_modules_*`, `cluster_pair_trend`, `cluster_energy_trend`, `fft_dominant_amplitude`.

## PARTIE III — AUDIT CODE LIGNE PAR LIGNE (BUGS CACHÉS ET TROUS)

| Fichier | Ligne(s) | Constat | Impact | Correctif appliqué |
|---|---:|---|---|---|
| `run_research_cycle.sh` | 48-55 | absence de factorisation checksum | incohérences de génération selon étapes | ajout `write_checksums()` |
| `run_research_cycle.sh` | 75-80 | fullscale sans checksum dédié | run fullscale non auditable comme run advanced | checksum fullscale ajouté |
| `run_research_cycle.sh` | 172-173 | checksum advanced calculé avant post-étapes finales | manifest potentiellement périmé | conservé mais doublé par final checksum |
| `run_research_cycle.sh` | 205-207 | (nouveau) checksum final en fin de pipeline | supprime le trou “stale manifest” | ajouté |
| `run_research_cycle.sh` | 22 | `TOTAL_STEPS` non aligné avec nouvelles étapes | progression >100% ou incohérente | ajusté à `32` |

### Cause racine “checksums invalides”
Le checksum global était produit avant la dernière phase (`run_fullscale_strict_protocol.sh`) et avant stabilisation finale des artefacts. Si des fichiers étaient ensuite créés/supprimés, `logs/checksums.sha256` devenait obsolète.

## PARTIE IV — FORMAT QUESTION / ANALYSE / RÉPONSE / SOLUTION

### Q1. Pourquoi 3447 garde 13 FAIL de stabilité malgré des drifts très faibles ?
- **Analyse:** tous les FAIL ciblent `energy_density_drift_max`, avec valeurs ~1e-6–1e-5.
- **Réponse:** signature d’un gate de stabilité/logique de qualification (seuil/contrat inter-runner), pas d’explosion numérique brute.
- **Solution:** verrouiller un contrat unique seuil+unité documenté dans les CSV et partagé par tous les runners.

### Q2. Pourquoi l’intégrité reste cassée alors que checksums existent ?
- **Analyse:** présence de `checksums.sha256` != validité du manifest.
- **Réponse:** le manifest peut devenir périmé si des étapes postérieures mutent les artefacts.
- **Solution:** générer checksum à la toute fin + gate `sha256sum -c` bloquant en fin de cycle.

### Q3. Qu’est-ce qui a réellement progressé entre 2829 et 3447 ?
- **Analyse:** invariants physiques identiques; un test critique (`independent_calc`) passe en PASS.
- **Réponse:** progression locale réelle, mais pas suffisante pour valider benchmark externe.
- **Solution:** traiter en priorité `qmc_dmrg_*`/`external_modules_*` avant toute décision rollout.

## PARTIE V — FEUILLE DE ROUTE PRIORISÉE

1. **P0 (intégrité):** checksum final bloquant + policy “aucune référence manquante”.
2. **P1 (science):** recalibrage benchmark externe (`qmc_dmrg_*`, `external_modules_*`).
3. **P1 (gates):** harmoniser stabilité énergétique inter-runners (seuil+unité+trace explicite).
4. **P2 (modèle):** revoir monotonicité cluster + diagnostic spectral `fft_dominant_amplitude`.

## PARTIE VI — COMMANDES EXACTES REPRODUCTIBLES

```bash
git fetch --prune origin
git rev-list --left-right --count origin/main...HEAD

python3 src/advanced_calculations/quantum_problem_hubbard_hts/tools/analyze_replit_run_evolution.py \
  --results-root src/advanced_calculations/quantum_problem_hubbard_hts/results \
  --runs research_20260312T075921Z_523 research_20260312T181131Z_2715 research_20260312T182101Z_2854 research_20260312T222246Z_2829 research_20260312T222838Z_3447 \
  --out-json src/advanced_calculations/quantum_problem_hubbard_hts/CHAT/reports/replit_evolution_20260312.json \
  --out-md src/advanced_calculations/quantum_problem_hubbard_hts/CHAT/RAPPORT_EVOLUTION_REPLIT_MULTI_RUNS_20260312.md

make -C src/advanced_calculations/quantum_problem_hubbard_hts clean all
bash src/advanced_calculations/quantum_problem_hubbard_hts/run_research_cycle.sh
```
