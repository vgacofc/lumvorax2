# RAPPORT_SYNC_REPLIT_CRITIQUES_CHATGPT_CYCLE33

## Objectif
Implémenter tous les points critiques Cycle33 avec exécution reproductible, sans modifier les anciens rapports, et avec inspection des stacks simulateur demandées.

## 1) Synchronisation + intégrité
- Synchronisation distante: **bloquée dans cet environnement** (remote `origin` absent).
- Intégrité locale: audit global et checks post-run relancés sur le run le plus récent disponible localement.

## 2) Implémentations ajoutées (Cycle33)

### 2.1 Observables critiques ajoutés
Nouveau script: `tools/post_run_advanced_observables_pack.py`
- Exporte `integration_spatial_correlations.csv` (corrélations spatiales proxy via autocorrélations en lag de pas).
- Exporte `integration_entropy_observables.csv` (entropie Shannon proxy énergie/pairing/sign).
- Exporte `integration_alternative_solver_campaign.csv` (validation cross-method basée benchmark QMC/DMRG ingéré).

### 2.2 Tests critiques ChatGPT rendus dynamiques (plus de FAIL hardcodé)
`tools/post_run_chatgpt_critical_tests.py`:
- T10, T11, T12 ne sont plus codés en FAIL fixe.
- Le statut dépend désormais des artefacts réellement générés.

### 2.3 Pipeline run complet mis à jour
`run_research_cycle.sh` appelle désormais le pack avancé avant les tests critiques:
- `post_run_advanced_observables_pack.py`

### 2.4 Inspection complète des simulateurs demandés (sans modification)
Nouveau script: `tools/inspect_quantum_simulator_stacks.py`
- Inspecte: v4_staging_next, v5_competitor_cpu, v4_kaggle_pack, v3_staging, v2_staging.
- Produit: `RAPPORT_INSPECTION_SIMULATEURS_V4NEXT_FULL_CYCLE33.md`.
- Recherche systématique des marqueurs `stub/placeholder/todo/fixme/hardcod*`.

## 3) État Full V4 NEXT vs Shadow/Canary

### Full
- Activation complète si gates + readiness valident les seuils.
- Rollback automatique reste activé (`ENABLED`).
- Utilisé ici pour valider robustesse globale post-critiques.

### Canary
- Activation partielle, périmètre de risque réduit.
- Utilisé pour montée en charge progressive.

### Shadow
- Validation passive sans impact d’activation complète.
- Utilisé comme filet de sûreté en pré-production scientifique.

## 4) Métriques bas niveau et utilisation modules (dernier run)
- Module share %: extrait de `integration_low_level_runtime_metrics.csv`.
- CPU/mémoire/latence/calc_per_second/qubits simulés: exportés par problème.
- Interopérabilité: consolidée via gates + benchmark + audit drift.

Schéma structure pipeline:

```text
run_research_cycle.sh
   ├─ core runner (C)
   ├─ post_run_metadata_capture
   ├─ post_run_cycle_guard
   ├─ post_run_physics_readiness_pack
   ├─ post_run_v4next_integration_status
   ├─ v4next_rollout_controller (shadow/canary/full)
   ├─ post_run_v4next_rollout_progress
   ├─ post_run_v4next_realtime_evolution
   ├─ post_run_low_level_telemetry
   ├─ post_run_advanced_observables_pack   <-- ajouté cycle33
   ├─ post_run_chatgpt_critical_tests (dynamic)
   ├─ post_run_problem_solution_progress
   ├─ post_run_authenticity_audit
   └─ exhaustive_replit_audit (global)
```

## 5) Problèmes oubliés à réintégrer (checklist)
1. Corrélations spatiales natives solveur (au-delà proxy lag).
2. Entropie d’intrication native solveur (au-delà proxy Shannon).
3. Campagne solveur alternatif réellement exécutée (QMC/DMRG/tensor externe auto-ingérée).
4. Campagnes multi-résolutions temporelles autour du minimum critique sur tous problèmes.
5. Couverture lattice-size étendue (12x12, 16x16).

## 6) Nouvelles simulations à ajouter (prochaines versions Replit)
- Sweep 2D dense: `(U/t, T)` avec pas raffiné proche zone critique.
- Campagne `dt/2, dt, 2dt` pour chaque problème.
- Finite-size scaling élargi: 12x12 / 14x14 / 16x16.
- Reruns multi-seed haute cardinalité pour CI confiance plus serré.
- Cross-method synchronisé (même protocole, solveur externe).

## 7) Commandes exactes Replit

```bash
ROOT="src/advanced_calculations/quantum_problem_hubbard_hts"
RUN_DIR="$(ls -1 "$ROOT/results" | rg '^research_' | tail -n 1)"
RUN_PATH="$ROOT/results/$RUN_DIR"

python3 "$ROOT/tools/post_run_v4next_integration_status.py" "$RUN_PATH"
python3 "$ROOT/tools/v4next_rollout_controller.py" "$RUN_PATH" full
python3 "$ROOT/tools/post_run_v4next_rollout_progress.py" "$RUN_PATH"
python3 "$ROOT/tools/post_run_v4next_realtime_evolution.py" "$ROOT" "$RUN_PATH"
python3 "$ROOT/tools/post_run_low_level_telemetry.py" "$RUN_PATH"
python3 "$ROOT/tools/post_run_advanced_observables_pack.py" "$RUN_PATH"
python3 "$ROOT/tools/post_run_chatgpt_critical_tests.py" "$RUN_PATH"
python3 "$ROOT/tools/post_run_problem_solution_progress.py" "$RUN_PATH"
python3 "$ROOT/tools/post_run_authenticity_audit.py" "$ROOT" "$RUN_PATH"
python3 "$ROOT/tools/inspect_quantum_simulator_stacks.py" "$ROOT"
python3 "$ROOT/tools/exhaustive_replit_audit.py" \
  --results-dir "$ROOT/results" \
  --out-csv "$ROOT/AUDIT_EXHAUSTIF_REPLIT_RUNS.csv" \
  --out-drift-csv "$ROOT/AUDIT_EXHAUSTIF_REPLIT_DRIFT.csv"
```
