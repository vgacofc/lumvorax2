# RAPPORT V139 — Analyse complète d'exécution Kaggle (pédagogique + audit expert)

## 1) Artefacts récupérés et preuve d'intégrité

Artefacts récupérés et inspectés:

1. `nx47-vesu-kernel-new-v139.ipynb`
2. `nx47-vesu-kernel-new-v139.log`
3. `results.zip`
4. `Z48yGhNf.txt`

Constats d'intégrité:

- `Z48yGhNf.txt` et `nx47-vesu-kernel-new-v139.log` sont strictement identiques (même hash SHA-256).
- `results.zip` contient les artefacts forensic attendus (`v139_execution_logs.json`, `v139_execution_metadata.json`, `v139_forensic_analysis_report.json`, `submission.zip`, etc.).

## 2) Résumé exécutif (verdict rapide)

### 2.1 Exécution pipeline

- **Exécution technique: réussie** (fin propre): présence de `GLOBAL_STATS` puis `EXEC_COMPLETE`.
- **Soumission: produite** (`submission.zip` présent et validé au format attendu pour le test observé).
- **Temps total**: ~2933 s (~48.9 min), donc pas de timeout Kaggle 9h sur ce run.

### 2.2 Apprentissage (question centrale)

- **Le modèle a-t-il "appris" au sens optimisation des paramètres ?**
  - **Oui, partiellement**: les poids bougent (deltas L2 non nuls), pruning actif, pipeline supervisé exécuté.
- **Le modèle a-t-il "appris" au sens qualité de généralisation validation ?**
  - **Non sur ce run**: `val_f1=0.0`, `val_iou=0.0`, `val_fbeta=0.0` (supervisé et U-Net), ablation delta 0.0.

Conclusion courte: **la mécanique d'entraînement fonctionne**, mais **le signal de performance validation est nul** sur cette exécution.

---

## 3) Timeline pédagogique complète (phase par phase)

## 3.1 Boot / environnement

- Warnings debugger "frozen modules": informatifs, non bloquants.
- Dépendances locales installées (`imagecodecs`, `tifffile`) depuis dataset Kaggle attaché.
- Continuité NX validée (`NX_CONTINUITY_OK`).
- Boot V139 avec config stricte:
  - `max_train_volumes=24`, `max_val_volumes=8`
  - pré-vol `5%/5%`
  - mode supervisé ON
  - contrôle règles compétition ON

## 3.2 Pré-vol 5%

- Train découvert: 786 paires.
- Pré-vol train: 40 paires (5%).
- Pré-vol test: 1 fichier sur 1.
- Résultat: `PREFLIGHT_OK`.

## 3.3 Audit dataset train

- `pair_count_discovered`: 786
- `pair_count_selected_for_training`: 32
- `coverage_pct_selected_vs_discovered`: **4.0712468193%**
- `train_image_files_found`: 786
- `train_label_files_found`: 786

=> Le run suit bien la stratégie V136/V139 de couverture plafonnée (24/8), orientée robustesse/temps.

## 3.4 Entraînement supervisé (neurone NX-47)

- `epochs_effective`: 6 (arrêt sur stagnation/convergence)
- `train_samples`: 960000
- `val_samples`: 320000
- `adaptive_lr`: 0.08
- `pruning_applied`: true
- `mutation_applied`: false

Hyperparamètres sélectionnés (meilleur candidat):

- `l1=1e-4`, `l2=1e-4`, `best_threshold=0.35`
- `objective=0.1053067756`
- `weight_delta_l2=0.47938955`
- `alpha_delta_l2=0.90658676`
- `beta_delta_l2=0.0`

Mais côté métriques validation:

- `val_f1=0.0`
- `val_iou=0.0`
- `val_fbeta=0.0`

## 3.5 Alerte forensic critique

- Événement `HARD_METRIC_PATTERN` détecté (policy `warn`):
  - même `objective` répété sur les 6 epochs.

Pédagogiquement: cela indique un plateau total (pas de gain mesurable en validation malgré calculs/updates).

## 3.6 U-Net 2.5D

- `status=ok`
- `train_loss` baisse (`0.7789 -> 0.5310`) => optimisation interne existe
- mais `val_f1/val_iou/val_fbeta` restent à 0.0

=> Même diagnostic: optimisation interne oui, généralisation validation non démontrée sur ce run.

## 3.7 Inférence test + packaging

- 1 fichier test traité (`1407735.tif`).
- `COMPETITION_RULES_VALIDATION`: `status=ok` (1 attendu / 1 produit).
- `GLOBAL_STATS` et `EXEC_COMPLETE` présents.

---

## 4) Processus d'apprentissage expliqué "comme un cours"

## 4.1 Ce que le modèle fait mathématiquement

Le neurone NX-47 utilise une activation sigmoïde sur combinaison de:

- `w · x` (features directes),
- termes quadratiques (`alpha`),
- termes gradient (`beta`),
- biais `b`.

La sortie est une probabilité [0,1].

## 4.2 Comment il "apprend"

À chaque itération:

1. il prédit une proba,
2. calcule l'erreur vs labels,
3. met à jour les paramètres par gradient proximal (L1/L2),
4. calibre un seuil de binarisation,
5. mesure F1/IoU/F-beta,
6. construit un objectif combiné.

## 4.3 Pourquoi on peut dire "apprentissage partiel"

- Les paramètres ont changé (deltas L2 non nuls).
- Le pruning a été appliqué.

Mais la qualité validation reste nulle (F1/IoU/F-beta = 0), donc:

- **apprentissage mécanique**: oui,
- **apprentissage utile sur validation**: non démontré ici.

## 4.4 Pourquoi l'ablation n'aide pas à valider

- `ablation_check`: `delta_f1_vs_full=0.0`.

Interprétation:

- soit le bloc ablaté n'apporte rien,
- soit la métrique est déjà au plancher (0), donc l'ablation ne peut plus montrer de dégradation.

---

## 5) Réponse explicite aux 5 preuves de la section "Diagnostic pédagogique"

1. **Signal d'entraînement (labels valides)**: ✅
   - labels train présents (786), audit train cohérent.
2. **Évolution des poids**: ✅
   - deltas L2 non nuls (`weight_delta_l2`, `alpha_delta_l2`).
3. **Amélioration objective validation**: ❌
   - objective identique sur 6 epochs + F1/IoU/F-beta à 0.
4. **Ablation dégradante**: ❌
   - delta ablation = 0.0 (non probant).
5. **Stabilité inter-run (multi-seed)**: ⚠️
   - cadre présent mais run multi-seed non exécuté (`stability_probe_runs=0`).

Verdict scientifique strict: **preuve incomplète d'apprentissage robuste**.

---

## 6) Réponse explicite aux 5 preuves d'audit investisseur (section 11)

1. **Code exécuté = code versionné**: ⚠️ partiel
   - signatures/merkle OK; hash notebook+commit lié non figé dans ce run.
2. **C/C++ local offline prouvé**: ⚠️ partiel
   - stratégie offline présente; preuve de compilation/symboles d'appel natif non jointe ici.
3. **Heartbeat + ETA/progression**: ✅ partiellement robuste
   - `PROGRESS_UPDATE` massif (3863), `eta_s` disponible.
   - `HEARTBEAT` peu fréquent (2 événements) sur ce run.
4. **Arrêts explicites**: ✅
   - run finit proprement (`EXEC_COMPLETE`), pas d'arrêt silencieux.
5. **Co-présence `GLOBAL_STATS` + `EXEC_COMPLETE` + `submission.zip`**: ✅
   - les trois sont présents dans les artefacts finaux.

---

## 7) Points critiques qu'un expert externe utilisera pour valider/refuter

## 7.1 Points forts

- Traçabilité forensic complète (events signés + merkle chain).
- Pipeline fini proprement avec artefacts complets.
- Contrats Kaggle-safe respectés (offline deps, couverture plafonnée, pré-vol).

## 7.2 Points faibles (bloquants pour validation scientifique forte)

- Métriques validation nulles (F1/IoU/F-beta à 0).
- `HARD_METRIC_PATTERN` (objective constant sur 6 epochs).
- `rules_file_found=false` et `metric_demo_found=false` (non bloquant format submission, mais incomplet côté conformité documentaire).
- `stability_probe` non exercé (0 run).

---

## 8) Recommandations techniques prioritaires (V140)

## P0 — Débloquer signal validation

1. Activer `hardcoded_metric_policy=error` en mode audit strict.
2. Vérifier distribution des labels val (positifs trop rares ? mismatch shape ?)
3. Logger confusion matrix et ratio de positifs prédits par epoch.
4. Ajouter sweep de seuil plus fin + calibration orientée rappel minimal.

## P1 — Preuves d'apprentissage robustes

1. Exécuter `stability_probe_runs >= 3` (seeds différentes) et stocker moyenne/écart-type F1.
2. Ajouter 2 ablations:
   - ablation features top-k,
   - ablation branche gradient/laplacien.
3. Ajouter condition de rejet: si `val_f1==0` sur tous epochs => run "invalid_learning".

## P2 — Audit investisseur

1. Injecter hash notebook exporté + hash commit + hash kernel dans metadata finale.
2. Ajouter preuve C/C++ exécutable:
   - log compilation,
   - table des symboles exportés,
   - compteurs d'appels runtime.

---

## 9) Glossaire pédagogique (termes techniques)

- **F1**: moyenne harmonique précision/rappel.
- **IoU**: intersection / union entre masque prédit et vérité terrain.
- **F-beta**: variante de F-score pondérant plus précision ou rappel.
- **Ablation**: on coupe un composant pour mesurer son impact causal.
- **Pruning**: suppression de poids faibles pour simplifier le modèle.
- **Proximal gradient**: optimisation avec pénalités L1/L2 (sparsité + stabilité).
- **Merkle chain**: chaînage cryptographique des événements pour détecter altérations.
- **Fail-fast**: arrêter immédiatement sur incohérence au lieu de continuer silencieusement.

---

## 10) Conclusion finale

- **Exécution V139 Kaggle: réussie techniquement** (run complet, soumission produite, forensic complet).
- **Apprentissage: partiellement réussi** (poids bougent) mais **non validé en qualité** (métriques val nulles).
- **Décision expert**:
  - ne pas revendiquer une amélioration scientifique de performance sur ce run,
  - lancer un V140 orienté preuve de généralisation (stabilité multi-seed + ablations causales + rejet auto si F1=0).
