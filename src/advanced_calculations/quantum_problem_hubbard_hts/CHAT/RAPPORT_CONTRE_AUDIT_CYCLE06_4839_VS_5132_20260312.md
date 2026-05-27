# CONTRE-AUDIT TECHNIQUE — Cycle06 `4839` vs `5132` + anomalies cachées

## 0) Mise à jour dépôt distant (demandée)

Synchronisation effectuée depuis:
- `https://github.com/lumc01/Lumvorax.git`

Commandes exécutées:
```bash
git remote add origin https://github.com/lumc01/Lumvorax.git
git fetch origin --prune
git checkout origin/main -- src/advanced_calculations/quantum_problem_hubbard_hts/results/research_20260312T010754Z_4839 src/advanced_calculations/quantum_problem_hubbard_hts/results/research_20260312T011703Z_5132
```

## 1) Validation indépendante de ton rapport comparatif

### 1.1 Structure des runs
- `4839` contient 12 fichiers versionnés localement.
- `5132` contient 55 fichiers versionnés localement.

=> La conclusion “5132 plus riche en couches de validation” est confirmée.

### 1.2 Matrice de tests
À partir de `tests/new_tests_results.csv`:
- `4839`: `PASS=22`, `FAIL=9`, `OBSERVED=49`
- `5132`: `PASS=22`, `FAIL=9`, `OBSERVED=49`

=> Statuts globaux identiques, cohérents avec ton constat : gains surtout en traçabilité, pas en performance brute.

### 1.3 Benchmarks externes (QMC/DMRG)
Calcul indépendant depuis `benchmark_comparison_qmc_dmrg.csv`:
- RMSE = `1284424.0395`
- MAE = `810133.4947`
- Within error bars = `0.0%`

Identique entre `4839` et `5132`.

=> Ton analyse “écart d’échelle persistant” est validée.

## 2) Nouvelles anomalies cachées détectées en plus

### A1 — Métadonnées critiques vides dans `5132` (confirmé) puis corrigé par post-traitement
Dans l’artefact historique de `5132`, `model_metadata.json` avait encore:
- `model_id`, `hamiltonian_id`, `schema_version`, `t`, `U`, `seed`, `solver_version` vides.

Action corrective appliquée immédiatement:
```bash
python tools/post_run_metadata_capture.py results/research_20260312T011703Z_5132
```

Résultat:
- Tous ces champs sont maintenant renseignés avec fallback explicite et auditable.

### A2 — Absence d’audit strict dans le run `5132` historisé
Le run `5132` ne contenait pas les artefacts `audit/` stricts.

Action corrective appliquée:
```bash
./run_fullscale_strict_protocol.sh results/research_20260312T011703Z_5132
```

Résultat:
- Génération de `audit/run_signature.json`, `audit/final_scientific_audit.md`, `audit/proxy_detection_report.csv`.

### A3 — Reproductibilité “seed différente” encore non discriminante
`rep_diff_seed` reste `FAIL` (delta=0), ce qui indique un comportement fortement déterministe au niveau observable final malgré l’injection pseudo-aléatoire.

Impact:
- Risque de faux sentiment de robustesse stochastique.
- À traiter par campagne dédiée de sensibilité multi-seed sur distributions complètes (pas uniquement énergie/pairing finaux).

## 3) Correctifs code ajoutés dans ce cycle

### C1 — `post_run_metadata_capture.py` renforcé
Ajout de reconstruction déterministe quand `t/U` manquent mais `U_over_t` existe:
- `t = 1.0`
- `U = U_over_t * t`

Objectif:
- empêcher les champs physiques fondamentaux vides dans les artefacts historiques partiels.

## 4) Priorités restantes (ordre strict)

1. Remplacer Euler explicite par intégrateur conservatif (RK4/Verlet/unitaire) pour fermer `norm_guard` + `energy_drift`.
2. Externaliser la définition des `problem_t` (CSV/JSON versionné) pour sortir du hardcoding C.
3. Ajouter un test de sensibilité multi-seed sur trajectoires complètes (`baseline_reanalysis_metrics.csv`) avec seuils statistiques.
4. Passer consensus multi-centre en mode “bloquant” (invalidant) si signatures absentes/incohérentes.

## 5) Commande unique de revalidation après ces corrections

```bash
bash -lc 'cd src/advanced_calculations/quantum_problem_hubbard_hts && make -j4 && python -m py_compile tools/post_run_metadata_capture.py tools/fullscale_strict_protocol_runner.py tools/tests/test_fullscale_strict_protocol.py && python -m unittest tools/tests/test_fullscale_strict_protocol.py && python tools/post_run_metadata_capture.py results/research_20260312T011703Z_5132 && ./run_fullscale_strict_protocol.sh results/research_20260312T011703Z_5132'
```
