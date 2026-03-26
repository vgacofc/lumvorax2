# Rapport avant / après — NX47 V116 (correction complète)

## 1) En-tête Python invalide
### Avant
- `from __future__` puis `import annotations` (syntaxe invalide).
### Après
- En-tête corrigé avec `from __future__ import annotations`.

## 2) Incohérence de version
### Avant
- Bannière V116 mais classes/config V115.
### Après
- Renommage complet en V116:
  - `V116Config`
  - `NX47V116AtomKernel`
  - artefacts `v116_*` et `submission_v116.zip`.

## 3) Variables d’environnement
### Avant
- Variables `V115_*` non alignées avec V116.
### Après
- Variables alignées sur `V116_*` (`TOP_K_FEATURES`, `TARGET_ACTIVE_RATIO`, `FULL_PIXEL_TRACE`, `TRACE_PIXEL_BUDGET`).

## 4) Fusion adaptative slice-wise
### Avant
- Fusion inter-slices non explicite.
### Après
- Ajout de `slice_adaptive_fusion(volume)` puis injection dans `extract_multi_features` pour stabiliser la projection principale.

## 5) Pipeline autonome conservé
### Avant
- Intention Option C hybride présente mais bloc de départ non exécutable.
### Après
- Pipeline exécutable en V116:
  - multi-features,
  - sélection auto,
  - neurone logistique L1/L2,
  - hysteresis topologique 3D,
  - calibration ratio cible,
  - packaging compétition.
