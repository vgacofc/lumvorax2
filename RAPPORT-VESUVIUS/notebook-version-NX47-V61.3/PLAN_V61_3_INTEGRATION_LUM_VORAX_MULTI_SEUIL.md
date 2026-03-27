# PLAN V61.3 — Intégration LUM/VORAX + calibration multi-seuil 3D

## Objectif
Transformer V61.3 en pipeline calibrable par couches de seuil et prêt pour flux canonique `.lum`.

## Architecture cible
1. Ingestion TIFF/ZIP Kaggle.
2. Conversion interne `.lum` (Python) + checksum.
3. Segmentation multi-seuil (`t_low`, `t_mid`, `t_high`).
4. Accumulation 3D contrôlée (`density_target_range`).
5. Export TIFF submission Kaggle 3D.

## Étapes
- E1: Ajouter `density_diagnostics.json` (`global`, `z-wise`, percentiles).
- E2: Ajouter module `lum_adapter.py` (read/write `.lum` minimal).
- E3: Implémenter fusion multi-seuil avec nettoyage progressif.
- E4: Bench A/B (seuil unique vs multi-seuil) avec mêmes seeds.

## KPI
- Respect format Kaggle 3D.
- Gain score vs V61.3 actuel.
- Densité stabilisée dans plage validée.

## Risques
- Sur-segmentation si accumulation non contrainte.
- Coût CPU en hausse (acceptable à ce stade).

