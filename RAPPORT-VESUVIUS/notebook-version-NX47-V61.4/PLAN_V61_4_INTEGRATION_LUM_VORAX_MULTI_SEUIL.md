# PLAN V61.4 — Intégration LUM/VORAX + calibration multi-seuil 3D

## Objectif
Transformer V61.4 en pipeline calibrable par couches de seuil et prêt pour flux canonique `.lum`.

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
- Gain score vs V61.4 actuel.
- Densité stabilisée dans plage validée.

## Risques
- Sur-segmentation si accumulation non contrainte.
- Coût CPU en hausse (acceptable à ce stade).


---

## MISE À JOUR 2026-02-20 — État forensic 360 + dépendances
- Vérification code/log: V61.4 utilise `imagecodecs/tifffile` via fallback `/kaggle/input/nx47-dependencies`.
- Le système forensic reste majoritairement style Python legacy (pas de bridge natif LUM/VORAX observé).
- La pré-exécution produit `SUBMISSION_READY` puis `EXEC_COMPLETE`, mais le notebook casse ensuite sur `NameError: false`.

### Actions restantes prioritaires
1. Unifier bootstrap dépendances vers dataset V3 principal (`/kaggle/input/datasets/ndarray2000/nx47-dependencies`).
2. Migrer forensic vers schéma JSON signé + Merkle standard.
3. Corriger cellule notebook invalide (`false` -> `False`).
4. Ajouter roundtrip `.lum` minimal et diagnostics densité par tranche.
