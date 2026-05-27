# PLAN V144.1 — Correction score + pipeline `.lum` + multi-seuil progressif

## Objectif
Corriger la baisse score V144.1 par calibration multi-seuil, normalisation de sortie et traçabilité renforcée.

## Priorités P0
1. Corriger version stamping interne (artefacts `v139_*` -> `v144.1`).
2. Normaliser sortie binaire configurable (`0/1` par défaut, `0/255` option explicite).
3. Ajouter calibration multi-seuil progressive + accumulation 3D.

## Intégration LUM/VORAX
- Ajouter conversion interne `.lum` (Python first) pour éviter écarts de conversion entre formats.
- Ajouter fallback C/C++ optionnel (ctypes + `.so`) sans bloquer exécution Kaggle.

## Contrôles obligatoires
- `density_diagnostics.json`: densité globale + par tranche z.
- `train_pair_used_count` et `train_pair_coverage_pct` explicites.
- `precision/recall` estimés par tranche et global.

## Plan d’expérimentation
- Run A: seuil bas + accumulation stricte.
- Run B: seuil médian + accumulation.
- Run C: seuil haut + accumulation.
- Run D: profil mixte adaptatif par tranche z.

## Go / No-Go
- GO si score > 0.269 et format submission conforme.
- NO-GO sinon et rollback sur meilleure config scorée.

