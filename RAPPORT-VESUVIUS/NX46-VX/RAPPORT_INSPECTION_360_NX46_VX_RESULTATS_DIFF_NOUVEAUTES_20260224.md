# Inspection 360° NX46-VX — comparaison **result-nx46-vx** vs **result-nx46-vx-v1**

Date: 2026-02-24  
Périmètre strict audité: `RAPPORT-VESUVIUS/NX46-VX/result-nx46-vx*` + rapports internes dans `RAPPORT-VESUVIUS`.

## 1) Méthode d’inspection (ce qui a été vraiment relu)
- Comparaison fichier-à-fichier des 2 arborescences (hash SHA256, tailles, présence/absence).
- Diff structurel JSON/CSV/logs pour séparer:
  1) différences **fonctionnelles**,
  2) différences de **rejouabilité** (timestamps, signatures, mémoire libre).
- Vérification binaire du contenu des ZIP (hash interne des TIFF, pas seulement hash du conteneur zip).
- Croisement avec les rapports historiques de `RAPPORT-VESUVIUS` pour isoler **les constats nouveaux** (et éviter de répéter les constats déjà actés).

## 2) Résumé ultra-court des nouveautés réelles
1. Les deux runs produisent des **masques finaux strictement identiques** (hash TIFF interne identique), malgré des milliers de différences dans l’état interne du modèle U-Net 2.5D.  
2. Les ZIP diffèrent au hash externe mais contiennent le **même TIFF**: c’est une dérive de conteneur (métadonnées d’archive), pas une dérive de prédiction.
3. Les journaux forensic montrent une pipeline quasi isomorphe (même nombre d’événements: 4049, même longueur memory tracker: 85), mais avec horodatages/signatures différents → **rejeu non bit-à-bit, sortie stable**.
4. Le rapport forensic embarqué (`v1442_forensic_analysis_report.json`) reste rédigé avec des items “V132 remaining steps” alors que la version est “NX47 V144.2” → **anomalie de cohérence documentaire**.
5. Le WAL forensic n’a pas le même fichier entre les deux runs (hash de nom différent, 56 octets), confirmant une trace d’exécution distincte, sans impact observable sur la sortie binaire finale.

## 3) Inventaire complet fichier par fichier (20 vs 20)

### 3.1 Fichiers identiques (contenu strict)
- `dependency_manifest_v615.json`
- `nx46_vesuvius/dependency_manifest_v77.json`
- `nx46_vesuvius/logs/UJxLRsEE.txt`
- `nx46_vesuvius/logs/dataset_discovery_inventory.json`
- `nx46_vesuvius/material_maps/1407735.tif`
- `nx46_vesuvius/native_training_manifest.json`
- `submission_masks/1407735.tif`
- `v1442_forensic_analysis_report.json`

➡️ Interprétation: inventaire dataset, mapping matériau, manifeste de dépendances et rapport forensic “template” sont stables.

### 3.2 Fichiers présents d’un seul côté
- Uniquement dans `result-nx46-vx`: `v28_forensic_logs/wal_v28_e0927d5d2385d55f.bin`
- Uniquement dans `result-nx46-vx-v1`: `v28_forensic_logs/wal_v28_c879478e1f54ec83.bin`

➡️ Interprétation: la trace WAL est régénérée par run, pas réutilisée.

### 3.3 Fichiers différents (11)
- `nx46_vesuvius/logs/RkF4XakI.txt`
- `nx46_vesuvius/logs/metrics.csv`
- `nx46_vesuvius/logs/state.json`
- `nx46_vesuvius/submission.zip`
- `nx47_vesuvius/submission.zip`
- `submission.zip`
- `v1442_execution_logs.json`
- `v1442_execution_metadata.json`
- `v1442_memory_tracker.json`
- `v1442_roadmap_realtime.json`
- `v1442_ultra_authentic_360_merkle.jsonl`

## 4) Analyse détaillée des 11 fichiers différents

### 4.1 `submission.zip`, `nx46_vesuvius/submission.zip`, `nx47_vesuvius/submission.zip`
- Hash ZIP externe différent.
- **Mais** hash interne du TIFF identique dans chaque paire comparée:
  - `1407735.tif` (submission nx46): `92590f075047b00c`
  - `1407735.tif` (submission nx47): `c5b241032ecc531b`

➡️ Nouveau constat important: la différence est au niveau conteneur ZIP (metadata/date/ordre), pas au niveau prédiction.

### 4.2 `v1442_execution_metadata.json`
- Différences mineures runtime global:
  - `elapsed_total_s`: 2430.61 vs 2440.00
  - `calc_per_sec_global`: 11459.19 vs 11415.08
  - `hardware.mem_available_kb`: 30243728 vs 30226140
- Différences majeures dans `supervised_train_info.unet_25d.model_state` (poids), et `train_loss` meilleur epoch:
  - 0.8071 vs 0.8793
- **Invariants** en sortie métier:
  - `anomalies_detected = 52`
  - `golden_nonce_detected = 11`
  - `f1_ratio_curve_best_f1 = 0.686275...`
  - `f1_ratio_curve_best_ratio = 0.205918...`

➡️ Nouveau constat: variabilité interne d’entraînement, stabilité des KPI finaux et du masque rendu.

### 4.3 `v1442_execution_logs.json`
- Taille et hash différents, mais:
  - longueur identique: **4049 événements**,
  - mêmes types d’événements (structure).
- Les premières divergences portent sur:
  - `ts_ns`,
  - signatures cryptographiques d’événement,
  - certains `elapsed_s_since_last`.

➡️ Nouveau constat: pipeline rejouée avec autre timeline/signature, mais même trajectoire macro (même cardinalité d’événements).

### 4.4 `v1442_memory_tracker.json`
- Longueur identique: **85 snapshots**.
- Différences essentiellement sur `ts_ns`.

➡️ Nouveau constat: même patron mémoire, autre chronologie.

### 4.5 `v1442_ultra_authentic_360_merkle.jsonl`
- Même nombre de lignes: **4049**.
- Différence dès la ligne 1 (signature/horodatage).

➡️ Nouveau constat: chaînage Merkle recalculé par run, cohérent avec replay non bit-à-bit.

### 4.6 `v1442_roadmap_realtime.json`
- Différence observée: `generated_at_ns` uniquement.

➡️ Nouveau constat: roadmap logique stable, timestamp de génération variable.

### 4.7 `nx46_vesuvius/logs/metrics.csv`
- Même nombre de lignes: **514**.
- 513 lignes différentes, principalement sur:
  - colonne timestamp,
  - une colonne de métrique système (charge/mémoire),
  - **identifiants d’échantillons stables** (ex: hash de patch constant ligne à ligne).

➡️ Nouveau constat: les échantillons traités sont les mêmes, mais les métriques système embarquées changent run-to-run.

### 4.8 `nx46_vesuvius/logs/state.json` et `RkF4XakI.txt`
- Écarts ciblés:
  - `cpu_total_ns` différent,
  - `qi_index_real` très proche mais non identique,
  - reste globalement stable (merkle_root inchangé, statut inchangé).

➡️ Nouveau constat: micro-variabilité de calcul, invariants pipeline conservés.

## 5) Anomalies détectées (nouvelles et actionnables)

### A1 — Incohérence documentaire version/objectif
Le fichier `v1442_forensic_analysis_report.json` annonce `version = NX47 V144.2`, mais ses sections “remaining_steps_executed_simultaneously” parlent encore de “consolider ... V132”.  
➡️ C’est un signal de dette documentaire: le template n’a pas été complètement réaligné avec la release.

### A2 — Non-reproductibilité bit-à-bit des artefacts forensic
Les signatures Merkle, WAL, timestamps et certains champs système changent entre runs.  
➡️ Ce n’est pas forcément un bug ML, mais c’est une anomalie de reproductibilité stricte pour audit légal/forensic.

### A3 — Décorrélation “poids modèle vs sortie finale”
Poids U-Net et train_loss changent, mais le TIFF final est identique.  
➡️ Hypothèse forte: le pipeline de fusion/post-process impose une zone de stabilité qui écrase les fluctuations du backbone.

## 6) Avant / Après unification (constat réellement nouveau)
- **Avant** (d’après rapports historiques): l’effort était centré sur intégration, extraction des paramètres concurrent, préparation des plans multi-runs.
- **Après unification NX46-VX** (preuve par artefacts): on observe une stabilité de sortie plus forte que la stabilité des états internes.

Ce point est nouveau car les rapports passés décrivaient surtout le “quoi intégrer”; ici on démontre le “comment ça se comporte réellement en double run complet”:
- internes variables,
- sortie opérationnelle stable,
- audit forensic perfectible sur la couche documentaire et bit-level replay.

## 7) Comparaison pédagogique vs modèle concurrent SOTA (0.552) — ce qui est différent clairement
Référence concurrent extraite localement:
- modèle principal explicite: `transunet.seresnext50...weights.h5`
- paramètres agressifs et explicites: `USE_TTA=True`, `T_low=0.50`, `T_high=0.90`, `z_radius=3`, `xy_radius=2`, `dust_min_size=100`.

Ce que NX46-VX montre de différent (sur ces artefacts):
1. **Traçabilité forensic beaucoup plus riche** (execution logs + merkle + memory tracker + state détaillé).
2. **Stabilité du masque final malgré variabilité interne** (observée empiriquement ici).
3. **Faiblesse actuelle côté couverture d’apprentissage interne**: `train_pair_coverage_pct ≈ 4.07%` (indique un sous-échantillonnage relatif, potentiellement limitant face à un concurrent mature).
4. **Paradoxe qualité interne**: `f1_ratio_curve_best_f1` non nul dans les stats, mais `unet_25d_best_fbeta = 0.0`; ce différentiel mérite une clarification métrique pour éviter de sur-interpréter le progrès.

## 8) Plan correctif concret (court, exécutable)
1. **Normaliser les ZIP** (`zip -X` ou writer déterministe) pour supprimer les faux diffs d’archive.
2. **Mode deterministic-forensic**: seed + horloge logique + gel des champs runtime non déterministes pour un export audit “bitwise”.
3. **Séparer métriques “système” et métriques “modèle”** dans `metrics.csv` pour faciliter comparaison run-to-run.
4. **Corriger template du rapport forensic** (retirer références V132 si build V144.2).
5. **Audit ciblé du paradoxe métrique** (`f1_ratio_curve_best_f1` vs `unet_25d_best_fbeta`) avec définition formelle de chaque métrique en en-tête de rapport.

## 9) Conclusion
Les nouveautés concrètes, lues dans les artefacts, ne sont pas “un gain score” direct: ce sont des découvertes de **comportement système**:
- sortie stable,
- internes non déterministes,
- traçabilité très riche mais partiellement non normalisée,
- documentation forensic à aligner sur la version réelle.

C’est exactement le type d’écart qui explique pourquoi deux runs peuvent “sembler différents partout”, tout en livrant le même masque final.
