# BILAN D’AUDIT COMPLET (NOUVEAU) — V144.1 vs V61.2 vs V7.2 vs Concurrent

Date: 2026-02-19  
Auteur: Agent Codex (audit forensique + lecture code complète)  
Périmètre réel audité:  
- `RAPPORT-VESUVIUS/notebook-version-NX47-V144.1/*`  
- `RAPPORT-VESUVIUS/notebook-version-NX47-V61.2/*`  
- `RAPPORT-VESUVIUS/notebook-version-NX47-V61.3/output_logs_vesuvius/v7.2-outlput-logs--nx46-vesuvius-core-kaggle-ready/*`  
- `RAPPORT-VESUVIUS/exemple-soumision-notebook-concurrent/*`  
- fichiers de contexte demandés: `nx46_vesuvius_plan.md`, `RAPPORT_IAMO3/NX/PRESENTATION_COMPLETE_DECOUVERTES_NX1_NX42.md`.

---

## 0) Problèmes rencontrés (notification obligatoire)

1. Deux documents demandés n’ont pas été trouvés dans le dépôt sous le nom exact:  
   - `DOCUMENTATION_AUDIT_EXPERT_NX46.md`  
   - `NX47_VESU_PLAN.md`
2. Les artefacts V144.1 portent encore des noms internes `v139_*` (anomalie de versionning interne).
3. Les scores Kaggle leaderboard explicites pour V144.1 et V7.2 ne sont pas présents dans les artefacts audités ici.
4. Pour l’inspection TIFF détaillée, installation locale de dépendances d’audit (`tifffile`, `imagecodecs`) nécessaire.

---

## 1) “Visionning” / actualisation entre versions (point de discipline)

Tu as raison sur le fond: l’audit doit être **ré-actualisé à chaque version**.  
Dans ce bilan, j’ai donc forcé une méthode systématique:

- lecture des logs d’exécution complets,
- inspection de chaque `results.zip` et des `submission.zip` imbriqués,
- extraction des paramètres globaux et de progression,
- comparaison technique version par version,
- contrôle code V144.1 sans TODO/stub/smoking.

Standard proposé pour la suite (automatique):  
**à chaque nouvelle version** → check-list fixe: `code + logs + zip + tiff + params + roadmap + questions ouvertes`.

---

## 2) Lecture complète du code V144.1 (de la première à la dernière ligne)

## 2.1 Résumé structurel du notebook V144.1

- Fichier: `nx47-vesu-kernel-new-v144-1.ipynb`
- Cellules code: **1** (grosse cellule monolithique)
- Lignes code extraites: **2017**
- Fonctions définies: **29**
- Classes définies: **1** (`_TinyUNet2p5D`)

### Fonctions clés réellement présentes
- Dépendances offline: `_is_pkg_available`, `install_offline`, `bootstrap_dependencies_fail_fast`, `ensure_imagecodecs`
- I/O TIFF: `read_tiff_lzw_safe`, `write_tiff_lzw_safe`
- Apprentissage / calibration: `train_unet_25d_supervised`, `train_nx47_supervised`, `train_nx47_autonomous`, `calibrate_thresholds`, `calibrate_target_ratio`
- Fusion / post-traitement: `slice_adaptive_fusion`, `hysteresis_topology_3d`
- Feature engineering: `extract_multi_features`, `auto_select_features`, `pseudo_labels`

## 2.2 Contrôle anti-faux-semblant demandé

- `TODO`: **0**
- `FIXME`: **0**
- `stub`: **0**
- `smoking`: **0**
- `pass` nus: **4** (à surveiller fonctionnellement)

Conclusion: je n’ai pas trouvé de placeholder explicite ni de stub textuel dans le code V144.1.

## 2.3 Intégration technologies “réelles” observables

- Mentions forensics/merkle/memory tracker: **présentes** (forensic 26+, merkle 8+, memory_tracker présent)
- Supervised training: **présent** (mentions nombreuses)
- U-Net 2.5D: **présent** (`_TinyUNet2p5D` + routine train)
- Lean/Aristotle: **absent** dans ce notebook V144.1 (0 mention)

=> Donc V144.1 est une intégration ML/forensic Kaggle, pas une chaîne Lean4 active.

---

## 3) Audit profond des résultats — fichier par fichier, paramètre par paramètre

## 3.1 V144.1 (corrigée)

### A) Logs
- `s0zulCMY.txt`: 4080 lignes, erreurs fatales: 0, warnings: 6
- `nx47-vesu-kernel-new-v144-1.log`: 4080 lignes, erreurs fatales: 0, warnings: 6
- événements clés présents: `SUPERVISED_TRAIN`, `GLOBAL_STATS`, `EXEC_COMPLETE`, `READY:`

### B) ZIP et artefacts
- `results.zip` contient:
  - `submission.zip`
  - `v139_execution_logs.json`
  - `v139_execution_metadata.json`
  - `v139_forensic_analysis_report.json`
  - `v139_memory_tracker.json`
  - `v139_roadmap_realtime.json`
  - `v139_ultra_authentic_360_merkle.jsonl`

### C) Soumission TIFF
- `submission.zip/1407735.tif`
  - shape: `(320,320,320)`
  - dtype: `uint8`
  - min/max: `0/255`
  - mean: `15.3`

### D) Paramètres d’apprentissage extraits
- `train_samples`: **960000**
- `val_samples`: **320000**
- `epochs_effective`: **6**
- `adaptive_lr`: **0.08**
- `mutation_applied`: **False**
- `pruning_applied`: **True**

### E) Qualité de validation rapportée (interne)
- `val_f1`: **0.0**
- `val_iou`: **0.0**
- `val_dice`: **0.0**
- `val_fbeta`: **0.0**
- `best_threshold`: **0.35**
- `objective`: **0.10530677559263577**

### F) Comportement “neurones” (dynamique)
- `active_neurons_start_total`: **0**
- `active_neurons_mid_total`: **6**
- `active_neurons_end_total`: **6**
=> Interprétation: activation tardive minimale, puis plateau (pas d’expansion neuronale au-delà de 6 actifs).

### G) Progression process
- `PROGRESS_UPDATE`: **3863 événements**
- `roadmap overall progress`: **86.67%**
- `train_pair_coverage_pct`: **4.071246819338422%** (faible couverture)

### H) Global stats
- `files_processed`: 1
- `slices_processed`: 320
- `pixels_processed`: 32,768,000
- `patterns_detected`: 933
- `anomalies_detected`: 52
- `unknown_discoveries`: 0

=> Sur cette exécution, le pipeline **tourne** et trace, mais les métriques val F1/IoU nulles indiquent un apprentissage peu discriminant côté validation interne.

---

## 3.2 V61.2 (version scorée de référence)

### A) Logs
- `BRzSoZeY.txt` + `.log`: 87 lignes, erreurs fatales: 0, warnings: 4
- événements visibles: `FUSION_SCORE_GLOBAL`, `EXEC_COMPLETE`, `READY:`

### B) ZIP
- `results.zip` contient:
  - `final_logs/forensic_audit_1771420660.log`
  - `nx47_vesuvius/submission.zip`
  - `submission.zip`
  - `v28_forensic_logs/forensic_v28_....log`
  - `v28_forensic_logs/wal_....bin`

### C) Soumission TIFF
- 3D `(320,320,320)`
- `uint8` binaire `0/1`
- mean: `0.12256454467773438`

=> V61.2 a la sortie la plus “propre” en binaire strict 0/1 parmi les versions NX47 comparées ici.

---

## 3.3 V7.2 (branche NX46, artefact output_logs)

### A) Logs
- `ZA5jJ6j8.txt`: 1423 lignes, warnings: 4, `READY:` présent

### B) ZIP
- `results.zip` contient logs riches (`metrics.csv`, `state.json`, `merkle_chain.log`, `bit_capture.log`, etc.)
- état global (`state.json`):
  - `runtime_status`: `offline_activated`
  - `pipeline_status`: `success`
  - `active_neurons`: **1,257,219**
  - `total_allocations`: 513
  - `total_pixels_processed`: 51,499,008
  - `total_ink_pixels`: 25,700,716
  - `ink_ratio`: 0.49905264194603516
  - `cpu_total_ns`: 1,621,008,468,584

### C) Soumission
- `submission.zip/1407735.tif` en **2D** `(320,320)` `uint8` `0/255`
- masque `submission_masks/1407735.tif` idem

=> Contrairement à V61.2/V144.1 (3D), cet artefact v7.2 exporte une sortie 2D, ce qui est structurellement différent.

---

## 3.4 Version concurrente (exemple 0.552)

### A) Logs
- `qmSqO2X8.txt`: 1287 lignes

### B) Soumission
- `results.zip/submission.zip/1407735.tif`
  - shape `(320,320,320)`
  - `uint8`
  - `0/1`
  - mean `0.09412158203125`

=> Le concurrent suit, sur cet artefact, la logique volumique 3D binaire 0/1.

---

## 4) Comparaison benchmark complète (phase par phase)

## 4.1 Phase A — Compatibilité format soumission

- V61.2: ✅ 3D
- V144.1: ✅ 3D
- Concurrent: ✅ 3D
- V7.2: ⚠️ 2D

## 4.2 Phase B — Type de binaire masque

- V61.2: `0/1`
- Concurrent: `0/1`
- V144.1: `0/255`
- V7.2: `0/255`

Risque: écarts de post-traitement/scoring si pipeline aval attend strictement `0/1`.

## 4.3 Phase C — Processus d’apprentissage observable

- V144.1:
  - apprentissage supervisé déclaré,
  - 960k train / 320k val,
  - 6 epochs effectives,
  - mutation off, pruning on,
  - mais val F1/IoU/Dice/Fbeta à 0.0.
- V61.2:
  - pipeline plus compact (logs courts),
  - sortie robuste 3D 0/1,
  - version associée à score 0.387 (contexte projet).
- V7.2:
  - instrumentation runtime/mémoire/merkle très riche,
  - population neurones massive reportée,
  - sortie 2D (écart structurel avec cible 3D).

## 4.4 Phase D — Forensic / traçabilité nanoseconde

- V144.1: très forte traçabilité JSON + merkle + memory tracker.
- V7.2: traçabilité opérationnelle très riche (`metrics.csv`, state, bit_capture, merkle logs).
- V61.2: traçabilité plus légère que V144.1/v7.2.

## 4.5 Phase E — Différences de “neurone total”

D’après les artefacts:
- V61.2 = pipeline NX47 compact, orienté sortie conforme et run rapide.
- V144.1 = pipeline NX47 plus lourd, supervisé + forensic intense, mais métriques val internes faibles.
- V7.2 = pipeline NX46 orienté instrumentation “système”, pas aligné 3D sur cet artefact précis.

Conclusion comparative: **ce n’est pas le même comportement global**, ni la même stratégie d’optimisation entre V61.2, V7.2 et V144.1.

---

## 5) Réponses directes à tes questions expertes (sans esquive)

## 5.1 “Est-ce qu’il a appris quelque chose ?”

- V144.1: il y a apprentissage exécuté (train/val/epochs/updates), mais indicateurs val F1/IoU/Dice/Fbeta = 0.0 dans ces artefacts.
- Donc: **apprentissage processuel oui**, **apprentissage performant non démontré** par ces métriques.

## 5.2 “Combien de neurones avant/pendant/après ?”

- V144.1 (global stats): 0 → 6 → 6 actifs.
- V7.2 (`state.json`): 1,257,219 actifs reportés (autre architecture).

## 5.3 “Formule, axiome, lemme, Lean4 généré ? interactions Lean/IA ?”

Sur le périmètre audité ici:
- aucune formule/axiome/lemme Lean4 générée dans V144.1,
- aucun fichier Lean4 trouvé dans ces artefacts V144.1/V61.2/V7.2/concurrent,
- interactions Lean4 observées: **0**,
- interactions IA internes quantifiées de manière fine: non exposées explicitement en “nombre de tours LLM” dans ces logs.

## 5.4 “Comment il pense / apprend par tranche / temps par tranche ?”

Ce qui est objectivement traçable:
- V144.1: 3863 `PROGRESS_UPDATE`, 320 slices processées, exécution complète ~2600s.
- V7.2: `cpu_total_ns` fourni et `metrics.csv` par fragment (513 allocations/étapes).

Ce qui manque encore pour répondre à 100% de ta demande:
- un export standardisé “per-slice learning table” unifié (temps, score local, succès/échec par tranche).

---

## 6) Alignement avec les docs demandés

## 6.1 `nx46_vesuvius_plan.md`
- Le plan insiste sur adaptation NX46 + logs HFBL-360 + tests unitaires/intégration.
- Dans l’artefact v7.2, on voit effectivement des preuves de logs opérationnels (metrics/state/merkle/bit_capture), donc une partie de l’esprit du plan est appliquée.

## 6.2 `PRESENTATION_COMPLETE_DECOUVERTES_NX1_NX42.md`
- Ce document parle d’invariants/hystérésis/mémoire dissipative/traçabilité Merkle.
- Ces thèmes sont partiellement matérialisés dans les journaux V144.1 et v7.2 (hysteresis, merkle, forensic), mais pas tous au même niveau d’explicitation formelle.

## 6.3 Fichiers introuvables
- `DOCUMENTATION_AUDIT_EXPERT_NX46.md`: non trouvé.
- `NX47_VESU_PLAN.md`: non trouvé.

Action requise: fournir les chemins exacts si les noms diffèrent.

---

## 7) Questions critiques restant sans réponse (bloquants)

1. Score leaderboard final de V144.1 ? (absent ici)
2. Score leaderboard final de v7.2 ? (absent ici)
3. A/B mesuré `0/1` vs `0/255` sur même seed/modèle: quel impact réel LB ?
4. Pourquoi V144.1 publie encore des artefacts `v139_*` ?
5. Comment standardiser un tableau “apprentissage par tranche” pour audit pédagogique total ?

---

## 8) Plan d’amélioration immédiatement actionnable

## P0 (immédiat)
- Corriger version stamping interne (noms `v139_*` -> version réelle).
- Ajouter export CSV/JSON standard **par tranche**: `slice_id, train_time_ms, infer_time_ms, local_confidence, success_flag`.
- Uniformiser sortie soumission en 3D binaire strict configurable (`0/1` prioritaire, `0/255` option explicite).

## P1 (benchmark)
- Bench A/B/C strict:
  1) V61.2 baseline,
  2) V144.1 actuel,
  3) V144.1 + sortie 0/1 + calibration revue.
- Mesurer: LB score, runtime, mémoire, stabilité hash forensics.

## P2 (gouvernance)
- Déployer un template unique “version card” pour chaque dossier version:
  - architecture,
  - paramètres,
  - logs clés,
  - score,
  - anomalies,
  - décisions.

---

## 9) Verdict exécutif

- V144.1 est **fonctionnelle runtime** et fortement instrumentée forensic.
- Mais ses métriques validation internes sont faibles (F1/IoU/Dice/Fbeta = 0.0).
- V61.2 reste la référence scorée/compacte la plus propre dans ce périmètre.
- V7.2 apporte une instrumentation système massive mais avec sortie 2D sur cet artefact.
- Le concurrent (artefact fourni) suit une sortie 3D binaire 0/1 cohérente.

En clair: pour gagner en score de manière robuste, la priorité est de **combiner la conformité de V61.2 + la traçabilité de V144.1 + la granularité runtime de v7.2**, puis valider par A/B scoré réel.

