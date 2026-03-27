# NX46 V7.6 — Campagne 10 runs (application immédiate)

## Objectif
Exécuter 10 variantes de calibrage seuil/poids 3D pour améliorer le rappel sans casser la conformité.

## Matrice utilisée
- Source: `RAPPORT-VESUVIUS/campaign_run_matrix_v61_3_v7_5.json`
- 10 runs (`v7.6-run-01` à `v7.6-run-10`) avec variation:
  - `NX46_THRESHOLD_QUANTILE`
  - `NX46_SCORE_BLEND_3D_WEIGHT`
  - `NX46_Z_SMOOTHING_RADIUS`

## Implémentation appliquée dans v7.6
- Ajout `threshold_quantile` configurable par environnement.
- Ajout `NX46_RUN_TAG` pour traçabilité.
- Ajout mode `NX46_DRY_RUN=1` pour campagne préflight locale 10/10.

## Résultat d’exécution ici
- 10/10 runs préflight local exécutés (retour 0).
- Limitation: scoring Kaggle non exécutable ici (absence de runtime Kaggle + chemins `/kaggle/input/*`).

---

## Mise à jour score Kaggle (retour utilisateur)
- Soumission réussie `nx46_vesuvius_core_kaggle_ready` (Version 14): **Public Score = 0.303**.
- Référence NX47 V61.2: **0.387**.

Écart observé: **-0.084** vs baseline 0.387.

---

## Analyse rapide de cause probable (avec artefacts offline)
- Le `results.zip` v7.6 est conforme (TIFF 3D `uint8` binaire `0/1`).
- Densité finale de soumission (`ink_ratio`) observée: **0.02341797** (≈2.34%), très faible.
- Cette sous-densité est cohérente avec un risque de **rappel insuffisant** (under-segmentation).

---

## Questions critiques à traiter au prochain run
1. Quel `threshold_quantile` permet de remonter la densité entre 0.06 et 0.12 sans exploser les faux positifs ?
2. Le `score_blend_3d_weight=0.78` est-il trop conservateur pour les tranches faibles ?
3. Quels z-slices concentrent la perte de rappel (analyse `ink_ratio_by_slice`) ?
4. Quelle combinaison seuil + smoothing garde le format conforme et remonte le score LB > 0.303 ?

---

## Feuille de route corrective (itération courte)

### Étape 1 — Diagnostic densité
- Exporter et versionner `ink_ratio_global` + `ink_ratio_by_slice` + percentiles de probas.
- Définir une plage cible de densité validée par rapport aux runs scorés.

### Étape 2 — Mini A/B/C calibration
- Run A: quantile plus permissif (+recall)
- Run B: blend 3D réduit
- Run C: combinaison A+B

### Étape 3 — Go/No-Go
- **GO** si score > 0.303 ET conformité zip/tiff inchangée.
- **NO-GO** sinon; conserver la meilleure config scorée.


---

## Extension demandée — Intégration LUM/VORAX + multi-seuil progressif

### Stratégie technique
1. Exécuter une cascade de seuils (`t_low -> t_mid -> t_high`) au lieu d’un seuil unique.
2. Nettoyer à chaque étage (morphologie/consensus z-wise) puis accumuler les couches 3D.
3. Contraindre la sortie finale dans une plage densité cible pilotée par benchmark.

### Intégration `.lum` (Kaggle exécutable)
- Étape 1 (immédiate): format `.lum` Python canonique pour tous calculs intermédiaires.
- Étape 2 (optionnelle): appel C/C++ `lum/vorax` via `ctypes` si compilation `.so` disponible.
- Fallback obligatoire Python pur si interop C indisponible.

### Questions d’implémentation
- Quel gain réel score/runtime du mode multi-seuil vs seuil unique ?
- Quelle plage densité cible maximise score sur v7.6 sans casser la précision ?
- Quel coût `tif -> lum -> tif` et quel ratio coût/bénéfice sur Kaggle ?

---

## MISE À JOUR 2026-02-20 — Vérification forensic 360 + dépendances
- V7.6 utilise les dépendances du dataset V3 (`imagecodecs`, `tifffile`), confirmé par logs.
- Forensic 360 actif mais principalement texte/log Python (`forensic_ultra.log`, `merkle_chain.log`) plutôt que bundle JSON signé standardisé.
- Conformité submission confirmée, mais densité faible (~2.34%) reste un axe score critique.

### Actions restantes prioritaires
1. Migrer forensic V7.6 vers format JSON signé unifié cross-versions.
2. Ajouter manifest wheel+hash des dépendances réellement chargées.
3. Exécuter A/B/C multi-seuil + densité pilotée pour dépasser score 0.303.
4. Ajouter OOM guard standard même si run actuel passe.
