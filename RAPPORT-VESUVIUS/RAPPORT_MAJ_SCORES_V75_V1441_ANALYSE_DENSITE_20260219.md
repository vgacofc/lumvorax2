# MISE À JOUR ANALYSE — Scores Kaggle récents V7.5 vs V144.1 + causes probables de baisse

Date: 2026-02-19  
Contexte score communiqué (capture utilisateur):
- `nx47-vesu-kernel-new-v144.1` : **0.269** (public score)
- `nx46_vesuvius_core_kaggle_ready` (v7.5/Version 14) : **0.303** (public score)
- Baseline antérieure de référence: `V61.2` = **0.387**

---

## 1) Faits techniques vérifiés dans les artefacts offline

## 1.1 V144.1 (NX47)
- `results.zip` contient `submission.zip` + `v139_execution_*.json` + forensic/memory/roadmap.
- TIFF soumission:
  - shape `(320,320,320)`
  - `uint8`
  - plage `0..255`
  - moyenne `15.3`
- Densité binaire équivalente (normalisée): `15.3 / 255 = 0.0600` (**6.00%** de voxels actifs)
- Stats globales clés:
  - `train_pair_count_discovered=786`
  - `train_pair_coverage_pct=4.0712%` (très faible)
  - `active_neurons_start_total=0`, `mid=6`, `end=6`
  - `val_f1_mean_supervised=0.0`, `val_iou_mean_supervised=0.0`
  - `elapsed_total_s≈2608.67s`

## 1.2 V7.5 (NX46)
- `results.zip` contient `state.json`, `metrics.csv`, `merkle_chain.log`, `bit_capture.log`, `submission.zip`.
- TIFF soumission:
  - shape `(320,320,320)`
  - `uint8`
  - plage `0..1`
  - moyenne `0.02341796875` (**2.3418%** de voxels actifs)
- `state.json` clés:
  - `pipeline_status=success`
  - `active_neurons=1,257,219`
  - `train_items_total=786`, `train_items_with_labels=512`
  - `cpu_total_ns=1,646,986,070,720`
  - `submission_format_profile=kaggle_v8_5_style_zip_lzw_3d_uint8_0_1`
- `metrics.csv` (513 lignes) montre au dernier fragment d’inférence:
  - `ink_ratio=0.02341797` (aligné avec le TIFF final)

## 1.3 Baseline V61.2 (référence scorée 0.387)
- TIFF soumission:
  - shape `(320,320,320)`
  - `uint8`
  - plage `0..1`
  - moyenne `0.1225645447` (**12.256%** de voxels actifs)

---

## 2) Diagnostic comparatif “pourquoi ça n’évolue pas / pourquoi ça baisse”

## 2.1 Signal principal: sous-prédiction de densité (under-segmentation)
Comparaison densité de voxels prédits “ink”:
- V61.2: **12.256%** (score 0.387)
- V144.1: **6.000%** (score 0.269)
- V7.5: **2.342%** (score 0.303)

Lecture experte:
- V7.5 et V144.1 sont beaucoup plus conservatrices que V61.2.
- Si le GT réel contient plus d’encre que ce qui est prédit, le rappel chute fortement.
- En segmentation Fβ/F1 orientée Kaggle, cette sous-densité peut dégrader fortement le score public.

## 2.2 V144.1: apprentissage “actif” mais qualité val interne nulle
- Les logs montrent un vrai pipeline supervisé (samples/epochs/hyperparams).
- Mais `val_f1/val_iou` restent à 0.0.

Hypothèse forte:
- la calibration/thresholding final compense partiellement en sortie, mais l’apprentissage interne ne généralise pas correctement.
- Le faible `train_pair_coverage_pct` suggère que la couverture effective d’apprentissage est insuffisante.

## 2.3 V7.5: instrumentation excellente, calibration trop stricte
- V7.5 a une très bonne traçabilité système (state/metrics/merkle), format conforme 3D 0/1.
- Mais `ink_ratio` final à ~2.34% est très bas, indiquant une politique de sélection trop sévère.

Conclusion comparative:
- V7.5 semble “propre format + propre forensic”, mais trop conservatrice sur la détection.
- V144.1 semble “pipeline plus lourd”, mais avec validation interne non convaincante.

---

## 3) Questions critiques à poser maintenant (les bonnes questions)

1. Pourquoi V144.1 annonce-t-elle encore `version: NX47 V139` dans ses artefacts ?
2. Pourquoi la couverture train effective V144.1 est-elle ~4.07% alors que 786 paires sont découvertes ?
3. Quel est le couple `precision/recall` exact pour V7.5 et V144.1 (pas seulement score global) ?
4. Quelle densité cible optimale de sortie (`ink_ratio`) maximise LB sur validation locale ?
5. Quel seuil provoque le passage de 2.34% (V7.5) vers une zone 6–12% (plus proche V61.2) ?
6. Le profil 0/255 de V144.1 affecte-t-il le post-processing/lecture aval malgré la normalisation implicite ?
7. Sur V144.1, pourquoi 6 neurones actifs au final alors que la complexité visuelle du volume est élevée ?
8. L’ablation `mutation_applied=False` / `pruning_applied=True` est-elle trop agressive pour le rappel ?
9. Quelle différence exacte de distribution des probabilités entre V61.2, V7.5, V144.1 ?
10. Les tranches z à faible signal sont-elles systématiquement coupées par la calibration actuelle ?

---

## 4) Plan d’actions concret (pour remonter le score)

## P0 (immédiat)
- Ajouter un export standard `density_diagnostics.json` par run:
  - `ink_ratio_global`, `ink_ratio_by_slice`, percentiles probas, seuil final.
- Forcer un A/B de calibration avec 3 cibles de densité:
  - cible basse: 0.03
  - cible médiane: 0.06
  - cible haute: 0.10-0.12
- Corriger le version stamping V144.1 (`v139_*` -> version réelle).

## P1 (1 à 2 itérations)
- V144.1: augmenter couverture train effective et tracer `train_pair_used_count` réel.
- V7.5: desserrer `threshold_quantile` et `score_blend_3d_weight` pour récupérer du recall.
- Ajouter benchmark “par tranche z” pour repérer les zones où le modèle coupe trop.

## P2 (stabilisation)
- Standardiser un tableau comparatif obligatoire à chaque version:
  - score LB, densité globale, densité par tranche, précision/recall estimés, runtime, conformité ZIP.
- Établir une règle de non-régression:
  - on ne pousse pas une version si densité sort de la plage validée historiquement.

---

## 5) Conclusion nette

- Les deux versions soumettent correctement, mais les scores récents montrent un recul vs V61.2.
- Le facteur le plus tangible dans les artefacts est la **baisse de densité prédite** (sous-segmentation), surtout V7.5.
- Pour progresser, il faut prioriser un protocole strict de calibration densité + couverture d’apprentissage + tableaux per-slice.

