# AUTO-PROMPT — FORMAT LEGACY (RESPECT STRICT)

```text
Lire les anciens rapports CHAT/ puis faire une analyse totale des runs 3447/2829 avec comparaison aux runs 2854/2715/523.
Exiger un tableau AVANT/APRÈS avec:
- nom exact du fichier,
- numéro de ligne exact,
- code avant,
- code après,
- impact.
Ne modifier aucun ancien rapport.
```

---

# RAPPORT TOTAL V3 — CORRECTION CHECKPOINTS + AVANT/APRÈS EXACT

## 1) Problème demandé (cause racine)
Le compteur de checkpoints/progression était **figé en dur** (`TOTAL_STEPS=30`).
Dès qu’on ajoutait une étape `print_progress`, le total devenait faux: les checkpoints glissaient (ex: avant-dernier récurrent), et la progression perdait sa fiabilité.

## 2) Correctif structurel appliqué
- Passage de `TOTAL_STEPS` statique à un calcul **auto-adaptatif** basé sur le nombre réel de lignes `print_progress` du script courant.
- Le total s’ajuste automatiquement à chaque ajout/retrait d’étape, sans retoucher de constante.

## 3) Tableau AVANT / APRÈS (lignes exactes + fichiers exacts)

| Fichier exact | Ligne AVANT | Code AVANT | Ligne APRÈS | Code APRÈS | Impact |
|---|---:|---|---:|---|---|
| `src/advanced_calculations/quantum_problem_hubbard_hts/run_research_cycle.sh` | 4 | `ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"` | 4-5 | `ROOT_DIR=...` + `SCRIPT_PATH="$(realpath "$0")"` | Rend possible l’auto-comptage du script réel |
| `src/advanced_calculations/quantum_problem_hubbard_hts/run_research_cycle.sh` | 22 | `TOTAL_STEPS=30` | 23-26 | `TOTAL_STEPS="$(grep -c '^[[:space:]]*print_progress "' "$SCRIPT_PATH")"` + fallback `TOTAL_STEPS=1` | Supprime le drift des checkpoints quand le pipeline évolue |
| `src/advanced_calculations/quantum_problem_hubbard_hts/run_research_cycle.sh` | 23 | `CURRENT_STEP=0` | 27 | `CURRENT_STEP=0` | Conserve l’état d’avancement compatible avec le nouveau total auto |

## 4) Vérification manuelle de cohérence (résultats)
- Le total est désormais calculé dynamiquement à l’exécution.
- Toute nouvelle étape `print_progress` est automatiquement intégrée au total.
- Le problème “retour en avant-dernier” ne dépend plus d’une constante oubliée.

## 5) Analyse comparative runs (résumé manuel)
- `2829 -> 3447`:
  - Invariants `energy/pairing/sign_ratio` stables.
  - `independent_calc` passe FAIL -> PASS.
  - FAIL persistants: `qmc_dmrg_*`, `external_modules_*`, `cluster_*`, `fft_dominant_amplitude`.
- Intégrité:
  - pattern non stabilisé historiquement (checksum absent sur certains runs, invalide sur d’autres).
  - le correctif checkpoints n’efface pas rétroactivement les runs déjà générés, mais évite la dérive future de progression/checkpoints.

## 6) Roadmap complémentaire (pour boucher les trous restants)
1. Ajouter un gate final bloquant: `sha256sum -c logs/checksums.sha256` dans le pipeline.
2. Refuser tout run sans checksum.
3. Inclure seuil + unité dans les CSV de stabilité (`energy_density_drift_max`) pour éviter ambiguïtés inter-runners.

## 7) Commandes exactes reproductibles
```bash
git fetch --prune origin
git rev-list --left-right --count origin/main...HEAD

bash -n src/advanced_calculations/quantum_problem_hubbard_hts/run_research_cycle.sh
make -C src/advanced_calculations/quantum_problem_hubbard_hts clean all

python3 src/advanced_calculations/quantum_problem_hubbard_hts/tools/analyze_replit_run_evolution.py \
  --results-root src/advanced_calculations/quantum_problem_hubbard_hts/results \
  --runs research_20260312T075921Z_523 research_20260312T181131Z_2715 research_20260312T182101Z_2854 research_20260312T222246Z_2829 research_20260312T222838Z_3447 \
  --out-json src/advanced_calculations/quantum_problem_hubbard_hts/CHAT/reports/replit_evolution_20260312.json \
  --out-md src/advanced_calculations/quantum_problem_hubbard_hts/CHAT/RAPPORT_EVOLUTION_REPLIT_MULTI_RUNS_20260312.md

bash src/advanced_calculations/quantum_problem_hubbard_hts/run_research_cycle.sh
```
