# RAPPORT — Implémentations restantes Cycle06 (4839 vs 5132)

## Implémentations réalisées immédiatement

1. **Intégration numérique renforcée (Euler -> midpoint RK2)**
   - appliqué dans les deux moteurs C (`research_cycle` et `advanced_parallel`) sur la mise à jour `dH/ddi`.
   - initialisation non nulle normalisée une seule fois au démarrage pour éviter le cas dégénéré `norm=0`.

2. **Suppression du hardcoding des problèmes**
   - remplacement des tableaux `problem_t probs[]` hardcodés par chargement externe depuis:
   - `config/problems_cycle06.csv`.

3. **Benchmark fullscale vs fullscale (demandé)**
   - nouveau tool: `tools/post_run_fullscale_vs_fullscale_benchmark.py`
   - intégré dans `run_research_cycle.sh`.
   - nouveaux artefacts générés:
     - `tests/benchmark_comparison_fullscale_vs_fullscale.csv`
     - `tests/integration_fullscale_vs_fullscale_benchmark.csv`

4. **Métadonnées historiques backfill**
   - `post_run_metadata_capture.py` reconstruit `t/U` si seules les traces `U_over_t` sont disponibles.

## Résultat direct sur run 5132
- `integration_fullscale_vs_fullscale_benchmark.csv` montre RMSE/MAE = 0.0 contre le run 4839 (mêmes métriques finales observées).
- `model_metadata.json` n’a plus les champs critiques vides.

## Points restant ouverts (transparence)
- Le schéma est maintenant RK2 midpoint (plus stable), mais pas encore un intégrateur explicitement symplectique complet type Velocity-Verlet Hamiltonien canonique.
- Le test `rep_diff_seed` doit encore être durci via analyse de trajectoires complètes multi-seed et non seulement des observables finales.
- Le consensus multi-centre reste dépendant des signatures réelles des clusters externes.
