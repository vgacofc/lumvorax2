# Rapport expert V137 — Analyse détaillée `nx47-vesu-kernel-new-v136.log` + `NX-47_v136_output_results.zip`

## 1) Récupération distante GitHub (preuve)
- Log pullé: `nx47-vesu-kernel-new-v136.log`.
- Archive pullée: `NX-47_v136_output_results.zip`.
- Vérification archive: contient `submission.zip`, `v136_execution_logs.json`, `v136_execution_metadata.json`, `v136_forensic_analysis_report.json`, `v136_memory_tracker.json`, `v136_roadmap_realtime.json`, `v136_ultra_authentic_360_merkle.jsonl`.

## 2) Analyse v136 — point par point

### 2.1 Continuité neurone NX
- `NX_CONTINUITY_OK` présent.
- Matrice active jusqu’à `NX-47 v115..v136`.

### 2.2 Découverte dataset
- `DATASET_DISCOVERY.total_assets = 1615`.
- `TRAIN_DISCOVERY.pair_count = 786`.
- `TRAIN_DATASET_AUDIT`:
  - `train_image_files_found = 786`
  - `train_label_files_found = 786`
  - `required_pair_count = 786`
  - `coverage_pct_selected_vs_discovered = 4.07%`

### 2.3 Pourquoi seulement 4.07% ?
- Cause réelle: `max_train_volumes=24` et `max_val_volumes=8` dans v136,
  donc seulement 32 paires sur 786 sont utilisées (32/786 = 4.07%).
- Ce n’est PAS un problème Kaggle de RAM/CPU/GPU, c’est une **limite logicielle de config**.

### 2.4 Apprentissage
- `SUPERVISED_TRAIN` exécuté, sans fallback autonome.
- `GLOBAL_STATS` final présent.
- `learning_percent_real = 80.0` (selon formule v136).
- `val_f1_mean_supervised = 0.0`, `val_iou_mean_supervised = 0.0`.

### 2.5 Problèmes réels rencontrés
- v136 exécute entièrement le pipeline mais validation performance reste nulle.
- Cela indique que la boucle tourne, mais l’efficacité prédictive n’est pas démontrée.

## 3) Correctifs V137 implémentés

### 3.1 Suppression des limites imposées d’apprentissage (par défaut)
- `max_train_volumes = 0` et `max_val_volumes = 0` => utiliser tout le dataset train découvert.
- `supervised_epochs = 0` => mode auto jusqu’à convergence (patience/delta), sans nombre d’epochs imposé.

### 3.2 Adaptation stricte au dataset réel image/label
- Seuils minimaux alignés: 786 pairs/images/labels.
- Audit bytes et couverture conservé.
- Vérification séparée image/label avant entraînement.

### 3.3 Respect règles compétition
- Validation automatique de soumission:
  - présence `submission.zip`,
  - correspondance exacte des fichiers `.tif` avec les `test_images` découverts,
  - présence des fichiers de référence:
    - `Competition_Rules_Vesuvius_Challenge _Surface_Detection.md`
    - `vesuvius-2025-metric-demo.ipynb`

## 4) Neurone NX — formalisation A→Z

### 4.1 Définition mathématique
- `z = w·x + b + alpha·grad + beta·laplace`
- `p = sigmoid(z)`
- Seuil calibré via scan validation.

### 4.2 Logique interne
- Fit proximal (L1/L2), scoring multi-critères, sélection meilleur état.
- Pruning + mutation conditionnelle.
- Hystérésis/ratio calibration pour segmentation finale.

### 4.3 Différences avec standards
- Linéaire: NX ajoute composantes gradient/laplacien et mémoire d’évolution.
- CNN: NX privilégie traçabilité causale + forensic cryptographique.
- Attention/RNN: NX moins générique séquentiel, mais plus auditable bit/ns.

## 5) Invariants / contraintes / limites
- Invariants: logs signés, Merkle chain, timestamps ns, audit memory.
- Contraintes: qualité dépend labels/val signal; si métriques val restent nulles, gain causal non prouvé.
- Falsifiabilité: exceptions explicites sur seuils, epochs, règles soumission.

## 6) Checklist inspecteur
- [x] Le neurone NX est formellement défini
- [x] Son exécution est prouvée
- [x] Son évolution est traçable
- [x] Sa dynamique est analysée
- [ ] Son apport est causalement démontré (bloqué tant que val reste nulle)
- [x] Ses limites sont explicites
