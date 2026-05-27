# Explication claire des résultats V121 + ce que V122 ajoute

## Ce que montrent clairement les logs V121 fournis

À partir de tes logs V121 (extrait fourni) :

- **Dataset traité**
  - `file_count = 1`
  - `total_assets = 1615`
  - types: `.csv = 2`, `.tif = 1613`
- **Fichier traité**: `1407735.tif`
- **Sortie**: `/kaggle/working/submission.zip`
- **Features sélectionnées**: `local_entropy`, `grad_mag`, `proj_max`, `proj_mean`, `laplace`, `bandpass_response`
- **Neurones actifs (V121)**
  - dans `train_info.selected_hyperparams.non_zero_weights = 6`
  - ce qui implique *6 neurones/poids actifs en fin d’entraînement* (mais pas début/milieu explicitement)
- **Ratios actifs**
  - `active_ratio_final = 0.03`
  - `active_ratio_hysteresis = 0.0`
  - `active_ratio_calibrated = 0.03`
  - donc en pratique la détection finale provenait quasi uniquement de la calibration percentile.
- **Pixels ancrés détectés**
  - `mask_hysteresis` visuellement nul dans les traces (`active_ratio_hysteresis=0.0`), donc **0 ancrage effectif**.
- **Calcul estimé par seconde**
  - non fourni explicitement en V121 (pas d’événement global de débit calculs).
- **Métriques hardware complètes**
  - non consolidées dans un bloc dédié V121 (cpu/memoire/gpu global non journalisés en un événement standard).

## Pourquoi tu ne voyais plus "autant" de logs sous cellule

- V121 affichait déjà beaucoup de logs, mais il manquait un **bloc global final consolidé** de toutes les métriques que tu demandes (neurones début/milieu/fin, pixels papyrus sans ancre, materials, patterns, golden nonce, anomalies, etc.).
- En plus, plusieurs infos étaient réparties entre événements (`NX47_METRICS`, `ARRAY_TRACE`, etc.) au lieu d’être centralisées en un **compteur global**.

## Ce que V122 ajoute précisément

V122 ajoute un bloc `GLOBAL_STATS` + metadata enrichie, avec :

1. **Neurones actifs**
   - début / milieu / fin par entraînement,
   - et totaux globaux.

2. **Dataset exact**
   - dossiers scannés (`folders`) + types fichiers (`suffix_stats`) dans `DATASET_DISCOVERY`.

3. **Débit de calcul**
   - `calc_per_sec` par fichier,
   - `calc_per_sec_global` en fin d’exécution.

4. **Métriques système/hardware réelles**
   - python/platform/cpu_count,
   - mémoire (`/proc/meminfo`),
   - GPU via `nvidia-smi` si disponible.

5. **Pixels et segmentation détaillés**
   - pixels traités,
   - pixels ancrés détectés,
   - pixels papyrus sans ancre,
   - slices traitées,
   - materials détectés (composantes connexes),
   - patterns détectés.

6. **Golden nonce / découvertes / anomalies**
   - golden nonce (définition opérationnelle sur probas extrêmes + masque final),
   - unknown discoveries,
   - anomalies détectées.

7. **Logs ultra permanents**
   - `v122_ultra_authentic_360_merkle.jsonl` (chaîné Merkle + nanoseconde + signatures),
   - plus JSON mémoire et logs classiques.

## Informations d’expertise ajoutées automatiquement (autocritique temps réel)

V122 force la remontée des zones faibles du run via métriques :
- si `active_ratio_hysteresis` ≈ 0, alerte implicite sur faiblesse de l’ancrage fort ;
- comparaison anchor vs calibrated pour voir si le modèle apprend vraiment ou si la calibration “porte” tout ;
- suivi anomalies/discoveries pour identifier des signaux hors distribution.

