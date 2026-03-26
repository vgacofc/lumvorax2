# RAPPORT V126 — AUDIT FORENSIC COMPLET ET CORRECTIONS SIMULTANÉES

## 1) Problèmes identifiés (V123/V124/V125)
- Pipeline souvent perçu comme test-only ou pseudo-supervisé (métriques proxy sur-évaluées).
- Preuves scientifiques incomplètes (split, historique epochs, calibration threshold, métriques de validation détaillées).
- Golden nonces détectés mais pas suffisamment exploités en audit spatial.
- Sorties parfois trop conservatrices (masques fragmentés / vides selon seuils).

## 2) Correctifs V126 (simultanés)
1. **Supervision réelle renforcée**
   - Découverte paires `train_images/*.tif` + `train_labels/*.tif`.
   - Split train/validation de volumes explicitement journalisé.
   - Entraînement multi-epochs paramétrable (`supervised_epochs`).
2. **Validation mesurable**
   - Calcul de F1 / IoU / Dice en validation.
   - Calibration de seuil par scan (`threshold_scan`) optimisée sur F-beta (`fbeta_beta`).
   - Historique par epoch (`epoch_history`) + `best_threshold` sélectionné.
3. **Audit forensic augmenté**
   - Événement `SUPERVISED_TRAIN` enrichi (listes de volumes train/val, tailles d’échantillons, leaderboard).
   - `NX47_METRICS.train_mode` pour distinguer `supervised` et `autonomous_fallback`.
   - `GLOBAL_STATS` enrichi: `files_supervised_mode`, `files_autonomous_fallback`, moyennes `val_f1/val_iou/best_threshold`.
4. **Golden nonces exploitables**
   - Extraction top-K spatiale maintenue et auditée (`golden_nonce_points_topk`).

## 3) Preuves techniques désormais exportées
- Liste des volumes train/val effectivement utilisés.
- Nombre d’échantillons train/val.
- Leaderboard top hyperparams.
- Historique d’epochs et calibration de seuil.
- Métriques globales consolidées orientées validation supervisée.

## 4) Limites restantes (transparentes)
- V126 reste une architecture atomique (pas encore UNet 2.5D/3D full deep).
- Ce n’est pas encore une stack Kaggle top-10 complète (ensembles/TTA/CV 5-fold lourde).
- Mais la preuve d’apprentissage supervisé, de calibration et d’audit est désormais explicitement instrumentée.
