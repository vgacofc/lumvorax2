# PLAN V144.2 — Correction score + pipeline `.lum` + multi-seuil progressif

## Objectif
Corriger la baisse score V144.2 par calibration multi-seuil, normalisation de sortie et traçabilité renforcée.

## Priorités P0
1. Corriger version stamping interne (artefacts `v139_*` -> `v1442_*`).
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


---

## MISE À JOUR 2026-02-20 — Vérification forensic 360 + dépendances
- Vérification code/log: V144.2 utilise bien le dataset dépendances (installation `imagecodecs` observée dans logs).
- Forensic 360 est très riche (Merkle JSON, GLOBAL_STATS, EXEC_COMPLETE).
- Pas de preuve d’appel direct bridge natif `.so` dans ce run; forensic reste pipeline Python instrumenté.

### Actions restantes prioritaires
1. Ajouter preuve explicite `NATIVE_BRIDGE_ENABLED/NATIVE_BRIDGE_DISABLED` dans logs.
2. Ajouter KPI `% opérations natif vs python`.
3. Renforcer anti-OOM (RAM guard + downgrade auto).
4. Converger couverture train (`train_pair_coverage_pct`) vers cible réaliste de production.
