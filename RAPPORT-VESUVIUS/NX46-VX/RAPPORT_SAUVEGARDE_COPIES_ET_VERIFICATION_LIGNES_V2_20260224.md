# RAPPORT — Sauvegardes des notebooks + vérification lignes de code (V2)
Date: 2026-02-24

## Action demandée et appliquée
- Copie créée avant toute modification de l'original.
- V1 gardée intacte (copie immutable).
- Original mis à jour seulement après la création des copies.
- Versioning interne renommé en **V2** dans l'en-tête du notebook original.

## Copies créées
1. `nx46-vx-unified-kaggle-V1-COPY-IMMUTABLE-20260224.ipynb`
2. `nx46-vx-unified-kaggle-V2-PREUPDATE-COPY-20260224.ipynb`

## Vérification « pas moins de lignes de code »
- `nx46-vx-unified-kaggle-V1.ipynb`: 4488 lignes de code
- `nx46-vx-unified-kaggle-V2-PREUPDATE-COPY-20260224.ipynb`: 4552 lignes de code
- `nx46-vx-unified-kaggle.ipynb` (V2 mis à jour): **4562 lignes de code**

Conclusion: la V2 mise à jour contient **plus de lignes de code** que V1 et que la copie pré-update V2.

## Règle de rétrogradation
- Pour rollback V1: utiliser `nx46-vx-unified-kaggle-V1-COPY-IMMUTABLE-20260224.ipynb`.
- Pour rollback V2 juste avant cette modification: utiliser `nx46-vx-unified-kaggle-V2-PREUPDATE-COPY-20260224.ipynb`.
