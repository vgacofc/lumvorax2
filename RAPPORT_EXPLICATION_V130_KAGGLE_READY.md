# RAPPORT V130 — KAGGLE-READY (SUPERVISÉ + U-NET 2.5D + LOGIT FORENSIC)

## Objectif
V130 exécute la montée “niveau supérieur” demandée:
- conserver la base forensic/supervisée de V126,
- ajouter une branche **U-Net 2.5D** optionnelle,
- renforcer les preuves scientifiques (epochs, seuils, F-beta, audit logits),
- préparer une base plus proche d’un pipeline compétition.

## Nouveautés V130
1. **Branche U-Net 2.5D optionnelle** (`use_unet_25d`)
   - extraction de patches 2.5D multi-slices,
   - mini U-Net 2.5D entraîné en supervisé,
   - loss mixte BCE + Dice,
   - calibration de seuil par scan F-beta.
2. **Audit mathématique des logits**
   - histogramme des probabilités,
   - min/max/mean/std,
   - export par fichier dans `NX47_METRICS.probability_audit`.
3. **Calibration renforcée**
   - `threshold_scan` multi-seuils,
   - sélection `best_threshold` par F-beta,
   - historique `epoch_history` en supervisé.
4. **Preuves train/val complètes**
   - listes de volumes train/val,
   - tailles d’échantillons,
   - leaderboard hyperparams,
   - statut de branche U-Net 2.5D dans `GLOBAL_STATS`.

## Ce que répond V130 par rapport aux critiques
- “validation insuffisante” → epochs + F1/IoU/Dice/F-beta + threshold scan.
- “pas d’audit brut des probabilités” → `probability_audit` exporté.
- “pas de stack plus profonde” → ajout branche U-Net 2.5D (optionnelle), sans casser la robustesse fallback.

## Limites transparentes
- Ce n’est pas encore une stack top-10 complète (CV 5-fold lourde + ensemble + TTA étendue).
- Mais V130 fournit une base immédiatement exploitable pour ce passage.
