# Analyse experte NX46-VX (V3, V4, V5) + plan d'amélioration V6

## 1) Mise à jour dépôt distant (GitHub)
- Remote ajouté: `origin = https://github.com/lumc01/Lumvorax.git`.
- État: la branche locale `work` est **déjà alignée** avec `origin/main` au commit `f69ce5b`.
- Conclusion: pas de divergence de code à corriger avant l'analyse des résultats.

## 2) Périmètre analysé
- `RAPPORT-VESUVIUS/NX46-VX/result-nx46-vx-v3`
- `RAPPORT-VESUVIUS/NX46-VX/result-nx46-vx-v4`
- `RAPPORT-VESUVIUS/NX46-VX/result-nx46-vx-v5`

Éléments inspectés:
- Logs Kaggle (`*.log`, `*.txt`)
- Notebooks (`*.ipynb`)
- Archives de sortie (`results.zip`)
- Artefacts de forensic (`final_logs/*`, `v28_forensic_logs/*`)
- Roadmap runtime (`nx46vx_v2_roadmap_realtime.json`)

## 3) Résumé exécutif
- Les 3 runs produisent des outputs exploitables (submission.zip + logs riches), mais aucun run n'atteint les phases features/train/segment/package sur la roadmap principale.
- La progression globale est figée à **33.33%** sur V3, V4, V5 (2 phases sur 6 terminées: discovery + load).
- Le goulot principal est l'étape `train_supervised` (boucles `fit_prox_iter`) qui consomme le budget temps Kaggle avant finalisation complète du pipeline.
- Clarification V5: le run a **complété un cycle complet de 135 candidats (epoch 0)** puis a redémarré un second cycle et s'est arrêté à `index=32/135` (epoch 1). Donc la phase train n'est pas à 22–24% absolu; elle est à ~**123.7% d'un cycle candidat** mais **non finalisée** au niveau pipeline global.

## 4) Comparatif factuel V3 vs V4 vs V5

### 4.1 Volumétrie de logs
- V3: 10,186 lignes log, 10,079 événements JSON.
- V4: 10,204 lignes log, 10,099 événements JSON.
- V5: 1,513 lignes log (fichier principal), 1,513 événements JSON.

### 4.2 Événement terminal observé
- V3 dernière progression: `train_supervised / fit_prox_iter`, index 30/135, itération 1/120, global 33.33%.
- V4 dernière progression: `train_supervised / fit_prox_iter`, index 31/135, itération 60/120, global 33.33%.
- V5 dernière progression: `train_supervised / fit_kpi`, index 32/135, itération 20/120, global 33.33%.

Précision V5 importante:
- `max index` observé dans le log: **135/135** (cycle epoch 0 terminé)
- puis reprise en epoch 1 jusqu'à `index 32/135` avant coupure temps.

### 4.3 Roadmap runtime (dans `results.zip`)
Pour V3, V4, V5:
- discovery: 100%
- load: 100%
- features: 0%
- train: 0%
- segment: 0%
- package: 0%
- overall: 33.33%

### 4.4 Artefacts générés malgré interruption
Chaque version contient:
- `submission.zip` (et doublon `nx47_vesuvius/submission.zip`) avec `1407735.tif`.
- `nx46vx_v2_ultra_authentic_360_merkle.jsonl` (journal Merkle très volumineux).
- journaux forensic (audit + V28).

## 5) Lecture "phase par phase"

### Phase A — Boot / environnement
- Warnings CUDA/XLA répétés (`cuFFT`, `cuDNN`, `cuBLAS`, computation_placer déjà enregistré).
- Ces warnings ne stoppent pas le run, mais indiquent surcharge d'initialisation multi-framework.

**Objectif phase A**: initialiser l'environnement GPU.
- Statut: **Réalisé (100%)**
- Qualité: **partiellement propre** (warnings récurrents).

### Phase B — Discovery dataset
- Inventaire des assets effectué, événements de découverte présents.

**Objectif phase B**: valider les entrées train/test.
- Statut: **Réalisé (100%)**

### Phase C — Load volumes
- Chargement des volumes effectué avec traces `ARRAY_TRACE`.

**Objectif phase C**: charger les volumes 3D.
- Statut: **Réalisé (100%)**

### Phase D — Préflight train/test
- Les préflights passent jusqu'à 100% localement.

**Objectif phase D**: sanity checks avant entraînement.
- Statut: **Réalisé (100%)**

### Phase E — Train supervisé (hyperparam + prox)
- C'est la zone de blocage temporel.
- Forte granularité des updates (`fit_prox_iter`) => coût CPU/GPU + overhead logs.
- V5 a parcouru **1 cycle complet (135/135)** puis **~23.7% d'un second cycle**.
- Le modèle tourne avec `supervised_epochs=0` (arrêt par convergence/stagnation), donc la durée est potentiellement ouverte et sensible au timeout Kaggle.

**Objectif phase E**: converger et finaliser modèle.
- Statut: **Partiellement réalisé** (en V5: cycle 1 complet + cycle 2 partiel, mais sans clôture propre de la phase train dans le run final)
- Risque: timeout Kaggle avant convergence globale.

### Phase F — Features / Segment / Package final
- La roadmap runtime indique 0% pour ces blocs.

**Objectif phase F**: extraction features, segmentation finale, packaging final.
- Statut: **Non réalisé (0%)** selon roadmap.

## 6) Ce que disent les indices forensic
- Les fichiers `forensic_audit_*` indiquent seulement `SOLVER_INIT`, donc audit minimaliste (peu informatif sur l'échec final).
- Les logs `v28_forensic_logs` mesurent surtout des micro-benchmarks CPU/mémoire (create/destroy, move, split...), utiles pour performance système mais pas suffisants pour expliquer la non-finalisation modèle.

## 7) Hypothèse causale principale (et niveau de confiance)
1. **Cause dominante**: budget temps Kaggle absorbé par le bloc d'entraînement supervisé (itérations prox + grille hyperparam).
   - Confiance: **élevée**.
2. **Cause secondaire**: overhead de logging cryptographique (Merkle/signatures) très fréquent pendant entraînement.
   - Confiance: **moyenne à élevée**.
3. **Cause tertiaire**: initialisation multi-framework bruyante (warnings CUDA), non fatale mais potentiellement coûteuse.
   - Confiance: **moyenne**.

## 8) Questions critiques (répondues / ouvertes / nouvelles)

### 8.1 Répondues
- **Q:** Le pipeline s'arrête-t-il avant la fin ?
  - **R:** Oui, les phases aval restent à 0% dans la roadmap.
- **Q:** A-t-on quand même des outputs exploitables ?
  - **R:** Oui, `submission.zip`, Merkle log, forensic logs existent.
- **Q:** Quelle version progresse le plus dans le train observé ?
  - **R:** V5 avance légèrement plus loin (index 32).

### 8.2 Ouvertes
- Quel est le temps exact consommé par sous-bloc (`hyperparam_search` vs `fit_prox_iter`) avec timer consolidé ?
- Quel est le ratio temps compute utile / temps logs ?
- Quelle est la corrélation quality-score vs nombre d'itérations prox réellement exécutées ?

### 8.3 Nouvelles questions à instrumenter en V6
- Peut-on imposer un **time budget manager** par phase (hard-stop + fallback) ?
- Peut-on faire un **early-stop multi-critères** piloté par KPI de validation au lieu d'un nombre fixe d'itérations ?
- Peut-on réduire la fréquence Merkle/log en mode compétition sans perdre la traçabilité critique ?

## 9) Plan d'amélioration concret pour V6

### 9.1 Gouvernance temps (priorité P0)
1. Ajouter un ordonnanceur de budget temps global + par phase.
2. Définir des "deadlines internes":
   - T1: train doit terminer avant X% du budget.
   - T2: segment+package réservés avec budget incompressible.
3. Ajouter un mode **"safe submit"** qui force la production de sortie finale même en convergence partielle.

### 9.2 Optimisation entraînement (P0)
1. Réduire l'espace hyperparamètres (recherche coarse-to-fine).
2. Baisser `max_iter` dynamique si gain marginal < seuil.
3. Activer early stopping robuste (patience + delta KPI).
4. Mettre en cache les features/volumes réutilisables pour éviter recalcul.

### 9.3 Réduction overhead logging (P1)
1. Échantillonner les logs détaillés (`fit_prox_iter`) toutes N itérations.
2. Séparer mode forensic complet (offline) et mode compétition (léger).
3. Conserver uniquement checkpoints Merkle essentiels (début/fin phase + erreurs).

### 9.4 Stabilité runtime (P1)
1. Nettoyer imports multi-framework pour limiter warnings CUDA redondants.
2. Standardiser l'init GPU unique.
3. Ajouter health-check mémoire GPU/CPU périodique avec seuils d'alerte.

### 9.5 Qualité expérimentale (P2)
1. Ajouter un tableau de bord unique (phase, ETA, quality KPI, coût temps).
2. Produire automatiquement un post-mortem JSON par run (causes, blocage, % atteints).
3. Normaliser la comparaison inter-versions (mêmes seeds, mêmes contraintes).

## 10) Évaluation des objectifs par sous-point (format %)
- Boot environnement: **100%** (fonctionnel, bruit warnings).
- Discovery data: **100%**.
- Load volumes: **100%**.
- Préflight train/test: **100%**.
- Train supervisé:
  - **V3** ~22.2% du cycle courant (index 30/135).
  - **V4** ~23.0% du cycle courant (index 31/135).
  - **V5** **cycle 0 terminé (135/135) + ~23.7% cycle 1** (index 32/135), donc progression absolue >100% d'un cycle candidat mais run non finalisé.
- Features globales roadmap: **0%**.
- Train global roadmap: **0%** (non validé en tant que phase complète).
- Segmentation: **0%**.
- Packaging final roadmap: **0%**.
- Livraison d'artefacts intermédiaires: **100%** (submission/logs présents).

## 11) Revue ligne par ligne V5 (zone demandée: transfert + train supervisé)

### 11.1 Transfert d'apprentissage (teacher distillation)
- Le notebook déclare un registre de 9 teachers et émet un événement `TEACHER_TRANSFER_APPLIED` avec un booléen `teacher_transfer_enabled`.
- Cependant, dans le code audité, cette partie agit surtout comme **guardrail/validation de présence** (assertions et inventaire de chemins) et non comme une boucle explicite de distillation avec chargement/optimisation de poids teacher->student au moment du `build_supervised_model`.
- Dans `run()`, `PHASE_1_DONE` est loggé juste après `discover_inputs()` et avant `build_supervised_model()`, ce qui confirme un marquage de phase mais ne prouve pas une distillation lourde effectivement exécutée.

**Conclusion transfert**
- Statut: **partiellement démontré** (validation/plan/flag présents),
- Distillation effective complète: **non prouvée par artefacts de sortie disponibles**.

### 11.2 Train supervisé: ce qui s'est exactement passé
- L'espace candidat est `meta_neurons * len(l1_candidates) * len(l2_candidates)` = `3 * 5 * 3 = 135` candidats par epoch.
- Chaque candidat exécute `fit_prox(max_iter=120)` avec logs toutes 10 itérations + KPI.
- Le log V5 montre:
  1) un `epoch_done` (donc un cycle candidat complet terminé),
  2) un redémarrage en epoch suivant,
  3) arrêt en cours de candidat (`index=32/135`, `iter=20/120`) au timeout.

**Interprétation fiable**
- Le `~22–24%` observé en fin de log est le **pourcentage du cycle en cours** (epoch 1), pas le total réel déjà calculé.
- Le calcul total effectué en V5 est donc supérieur à ce qui était mentionné dans le précédent rapport.

## 12) Peut-on reprendre exactement V6 là où V5 s'est arrêtée ?

Réponse courte: **pas exactement avec les outputs actuels**.

Pourquoi:
1. Les artefacts `results.zip` observés ne contiennent pas de checkpoint explicite du state d'entraînement (poids + optimizer + curseur exact `epoch/candidate/iter`).
2. Le log donne l'état de progression (`index/iter`) mais pas une restauration déterministe complète du modèle au dernier pas.

Ce qui est possible en V6:
- **Reprise approximative**: réutiliser configuration + seeds + sous-ensemble déjà validé pour réduire le coût de recalcul.
- **Reprise exacte future**: ajouter checkpoints atomiques périodiques (ex: toutes 10 itérations/candidats) puis restaurer `epoch,candidate,iter,state`.

## 13) Intégration des nouvelles contraintes de quotas (comptes Kaggle)

Contrainte fournie:
- Compte officiel: quota GPU 30h épuisé, reste 20h TPU.
- Compte 2: notebook débloqué GPU H100/TPU, + ~15h GPU (P100 ou H100 selon dispo).

Stratégie V6 recommandée:
1. **Travaux non compatibles TPU** (pipeline actuel majoritairement NumPy/CPU + logique custom): exécuter sur GPU compte 2 (priorité H100).
2. **Mode budget strict** sur compte 2:
   - hard-stop train à T_budget (ex: 60–70% du temps alloué),
   - réserver 30–40% pour segment + packaging + validations.
3. **Compte officiel TPU**:
   - réserver pour expérimentations adaptées TPU uniquement (si/quand un chemin réellement TPU est prêt),
   - sinon éviter de brûler quota TPU sur un pipeline non optimisé TPU.

## 14) Ajustements V6 prioritaires pour continuité réelle
1. Checkpointing exact `train_supervised`:
   - état modèle (w,b,alpha,beta),
   - état recherche (epoch, neuron_id, l1_idx, l2_idx, iter),
   - RNG state,
   - métriques cumulées.
2. Mécanisme `--resume-from-checkpoint` avec validation d'intégrité.
3. Décimation des logs itératifs en mode compétition (conserver KPI + checkpoints).
4. Arrêt par deadline avant timeout Kaggle + fallback packaging garanti.

## 15) Différences qui "font la différence"
- V5 montre une meilleure progression locale dans `train_supervised` (indice d'amélioration potentielle des réglages runtime).
- Les artefacts sont systématiquement produits => le pipeline est robuste côté I/O/sauvegarde, même en run tronqué.
- Le défaut structurel n'est pas l'absence de logs mais le manque d'un mécanisme de décision orienté deadline.

## 16) Recommandation finale
Prioriser en V6 une stratégie "deadline-first":
1. garantir la complétion des phases critiques avant timeout,
2. limiter le coût des boucles d'entraînement profondes,
3. conserver une version "forensic light" en compétition,
4. n'activer le forensic complet qu'en run d'analyse hors contrainte temps.

C'est la voie la plus probable pour transformer les runs "partiellement informatifs" en runs "soumis + scorables + comparables".


## 17) Génération modèle en temps réel + persistance Kaggle Model (demande ajoutée)

Objectif demandé: **ne plus perdre l'apprentissage en cours** quand Kaggle coupe le runtime, et pouvoir reprendre exactement au prochain notebook.

### 17.1 Réponse claire sur la reprise "à la nanoseconde"
- En pratique, une reprise strictement "à la nanoseconde" n'est pas garantie sur Kaggle (ordonnancement matériel non déterministe, redémarrage process, GPU kernels asynchrones).
- En revanche, on peut viser une reprise **quasi-exacte et déterministe** au pas d'entraînement (epoch/candidat/itération) avec:
  1. snapshot d'état fréquent,
  2. sérialisation atomique,
  3. checksum + Merkle + timestamp ns,
  4. restauration stricte des RNG states.

### 17.2 Deux stratégies de persistance à implémenter en V6

#### A) Persistance dans `Output` Kaggle (immédiate, robuste)
- Écrire des checkpoints incrémentaux dans `/kaggle/working/checkpoints/`.
- Forcer `flush + fsync` à chaque checkpoint critique.
- En fin de run (ou stop anticipé), zipper les checkpoints dans `results.zip`.
- Avantage: simple, compatible tous notebooks.
- Limite: nécessite recharger manuellement en run suivant (attach dataset/output précédent).

#### B) Publication dans Kaggle Models (continuité inter-run)
- À la fin de chaque run, publier le "latest stable checkpoint" vers l'espace Kaggle Model (versionné).
- Run suivant: consommer la dernière version du model artifact comme input, puis reprendre via `--resume-from-checkpoint`.
- Avantage: pipeline continu de versions modelisées.
- Limite: dépend du flux de publication/versioning et des permissions/quotas.

Recommandation V6: **implémenter A + B** (A comme filet de sécurité, B comme continuité principale).

### 17.3 Contrat de checkpoint minimal (obligatoire)
Fichier `ckpt_manifest.json` + binaire(s) poids/état:
- `ts_ns` (timestamp),
- `run_id`, `version`, `git_sha`,
- `epoch`, `candidate_index`, `candidate_total`, `iter`, `max_iter`,
- `model_state` (w,b,alpha,beta),
- `optimizer_state` (si présent),
- `rng_state_numpy`, `rng_state_python`, `rng_state_torch`,
- `best_metrics` (f1/iou/fbeta/objective),
- `data_cursor` (dernier volume/slice traité),
- `merkle_prev`, `merkle_curr`, `sha512_payload`.

### 17.4 Fréquence d'écriture (anti-perte)
- Checkpoint léger toutes les `N` itérations (ex: N=10).
- Checkpoint complet à chaque fin de candidat.
- Checkpoint complet à chaque `epoch_done`.
- **Pre-timeout checkpoint**: sauvegarde forcée à `T_stop = quota_remaining - marge_securité` (ex: 8 à 12 min avant timeout).

### 17.5 Stop anticipé intelligent (pour ne rien perdre)
- Ajouter un "time governor":
  - surveille temps écoulé + ETA + marge de sécurité,
  - déclenche `graceful_stop_and_checkpoint()` avant coupure Kaggle,
  - garantit écriture checkpoint + export `submission.zip` partielle/complète.

### 17.6 Reprise exacte inter-notebook (workflow)
1. Démarrage run: chercher `LATEST_CKPT` (priorité Kaggle Model, fallback output zip).
2. Vérifier intégrité (`sha512`, `merkle`, compat version/config).
3. Restaurer états modèle + RNG + curseurs.
4. Reprendre boucle exactement à `(epoch, candidate, iter)` suivant.
5. Journaliser événement `RESUME_OK` avec `resume_from_ts_ns`.

### 17.7 Règles de compatibilité pour éviter corruption silencieuse
- Refuser resume si mismatch de:
  - dimensions features,
  - grille hyperparamètres,
  - architecture/model signature,
  - version de preprocessing critique.
- Si mismatch: fallback contrôlé (retrain partiel) avec log `RESUME_INCOMPATIBLE`.

### 17.8 Adaptation à vos quotas actuels
- Compte 2 (GPU H100/P100 restant): exécuter les runs d'apprentissage + publication checkpoint model.
- Compte officiel (TPU restant): réserver aux étapes réellement TPU-ready; sinon ne pas consommer inutilement.
- Découper un grand run en micro-runs planifiés (ex: 45–90 min) avec checkpoint + publication systématique à chaque run.

### 17.9 Décision finale proposée
Pour V6, intégrer **obligatoirement**:
1. checkpointing incrémental temps réel,
2. stop anticipé + checkpoint pré-timeout,
3. reprise stricte `--resume-from-checkpoint`,
4. publication versionnée vers Kaggle Model.

C'est la condition pratique pour continuer l'apprentissage d'un run à l'autre sans repartir du début.


## 18) Correction demandée: remplacer le 33.33% global "stable" par un % réel dynamique

Constat: le `global_progress_percent` reste figé à 33.33% alors que le calcul continue dans `train_supervised`. Cela masque l'avancement réel runtime.

### 18.1 Principe de calcul recommandé (temps réel)
Au lieu d'un poids fixe par grandes phases, utiliser un **agrégat hiérarchique pondéré par travail réel**:

- `P_discovery` = progression discovery réelle
- `P_load` = progression load réelle
- `P_train_data` = progression préparation données train/val
- `P_train_fit` = progression entraînement réel (epoch/candidate/iter)
- `P_segment` = progression segmentation
- `P_package` = progression packaging

Formule:
`P_global_reel = Σ (W_i * P_i)` avec `Σ W_i = 1`.

Les poids `W_i` doivent être calibrés sur le coût mesuré (temps moyen historique), pas sur une distribution arbitraire.

### 18.2 Décomposition exacte de `P_train_fit`
Le train V5 fonctionne par triple boucle:
- `meta_neurons = 3`
- `l1_candidates = 5`
- `l2_candidates = 3`
=> `total_candidates = 135` par epoch.

Chaque candidat exécute `max_iter=120` dans `fit_prox`.

Progression train recommandée:
- `epoch_progress = completed_epochs + candidate_progress + iter_progress`
- `candidate_progress = (candidate_index-1)/total_candidates`
- `iter_progress = (iter/max_iter)/total_candidates`
- `P_train_fit = epoch_progress / epoch_target_effectif`

Si `supervised_epochs=0` (arrêt par convergence), fixer un `epoch_target_effectif` dynamique:
- ex: cible initiale = 2 epochs,
- puis ajustement via stagnation/convergence observée.

### 18.3 Événements à logger pour vérité terrain
Ajouter des événements dédiés:
- `GLOBAL_PROGRESS_REALTIME` (toutes 5–15 s),
- `TRAIN_CURSOR` (`epoch`, `candidate`, `iter`),
- `TIME_BUDGET_STATE` (`elapsed`, `remaining`, `eta_phase`),
- `PHASE_WEIGHT_CALIBRATION` (poids utilisés + origine mesure).

Cela permet un pourcentage réel, explicable, reproductible.

## 19) Optimisations maximales d'exécution (revue code + docs racine)

### 19.1 Goulots observés dans V5
1. Espace de recherche élevé en train:
   - 135 candidats/epoch × 120 itérations/candidat.
2. `supervised_epochs=0` => durée ouverte jusqu'à stagnation.
3. Logging très fréquent dans `fit_prox` + KPI périodiques.
4. Recalcul features/volumes répétés par paire train/val.
5. Branche U-Net 2.5D active par défaut (`use_unet_25d=True`) potentiellement coûteuse.

### 19.2 Plan d'accélération (ordre d'impact)

#### P0 — Réduction coût train sans perdre qualité
- Réduire grille hyperparamétrique en 2 étapes:
  1) coarse search courte,
  2) fine search locale autour des meilleurs candidats.
- Baisser `max_iter` adaptatif (early-stop intra-candidat).
- Introduire un plafond dur de temps pour `train_supervised`.
- Forcer `supervised_epochs` borné (ex: 1 ou 2) en mode compétition.

#### P0 — Resume/continuité
- Checkpoints fréquents (cf. section 17) + reprise curseur exact.
- Arrêt anticipé automatique avant timeout Kaggle.

#### P1 — Réduction overhead I/O et logs
- Décimer logs itératifs (`fit_prox_iter`) en mode compétition.
- Écrire checkpoints différentiels (delta) plutôt que snapshot complet à chaque fois.
- Bufferiser et flush par lot avec `fsync` seulement aux points critiques.

#### P1 — Réduction coût data/features
- Cacher features sélectionnées par volume (train/val/test).
- Éviter recomputation identique entre candidats hyperparam.
- Précharger structures nécessaires une seule fois par run.

#### P2 — U-Net 2.5D et branches optionnelles
- Désactiver `use_unet_25d` par défaut en run "deadline-first".
- N'activer que si budget temps résiduel > seuil.

### 19.3 Paramètres V6 "fast-safe" proposés
- `V1442_META_NEURONS=2`
- `V1442_MAX_ITER=40` (ou arrêt adaptatif plus tôt)
- `V1442_SUPERVISED_EPOCHS=1`
- `V1442_USE_UNET_25D=0`
- `NX46VX_V5_KPI_INTERVAL_ITER=20`
- `V1442_MAX_TRAIN_VOLUMES` et `V1442_MAX_VAL_VOLUMES` bornés selon budget réel
- `V1442_HEARTBEAT_INTERVAL_S=60` (moins bavard)

### 19.4 Validation perf obligatoire à ajouter
- `RUN_PROFILE_SUMMARY.json` en sortie avec:
  - temps par phase,
  - % temps compute vs logs vs I/O,
  - throughput candidats/s,
  - coût par itération,
  - estimation speedup vs run précédent.

But: passer d'un diagnostic qualitatif à une optimisation mesurable run après run.

## 20) Décision opérationnelle immédiate
1. Implémenter `global_progress_reel` en priorité avant prochain run.
2. Passer V6 en profil `fast-safe` pour sécuriser une complétion de pipeline.
3. Activer checkpoints + publication Kaggle Model à chaque micro-run.
4. Mesurer speedup réel et réajuster la grille hyperparam ensuite.


## 21) Réponse directe à la question critique (sans smoke) + directives obligatoires

### 21.1 Combien de cycles au total ?
- **Cycle théorique par epoch**: `meta_neurons * |l1| * |l2| = 3 * 5 * 3 = 135 candidats`.
- **Cycles configurés au total**: **non bornés explicitement** car `supervised_epochs=0` (arrêt piloté par convergence/stagnation, pas par un nombre d'epochs fixe).
- **Cycles réellement observés dans V5 (preuves log)**:
  1. **Epoch 0**: cycle complet `135/135`, `iter max=120`.
  2. **Epoch 1**: cycle partiel arrêté à `32/135`, `iter=20/120` au moment terminal.
- Donc, au moment de l'arrêt V5: **2 cycles engagés** (1 complet + 1 partiel), pas seulement 22–24% absolu.

### 21.2 Pourquoi après tout ce temps ce n'est pas tracé à 100% réel ?
Cause structurelle:
- Le champ `global_progress_percent` de la roadmap reste agrégé sur des phases haut niveau (discovery/load/features/train/segment/package) et **ne reflète pas la granularité interne réelle** de `train_supervised`.
- Le train a avancé fortement dans les sous-boucles, mais ce calcul n'est pas remonté dans un `global_progress_reel` dynamique.

=> Exigence V6: calculer et afficher en continu `global_progress_reel` basé sur curseur réel `(epoch, candidate, iter)` + poids mesurés.

### 21.3 Sur `PHASE_1_DONE` avant `build_supervised_model()`
- Oui: c'est un **marquage de phase** et non une preuve de distillation lourde effective.
- Ce point doit être corrigé par du **forensic exécutable**, pas déclaratif.

## 22) Distillation obligatoire réellement exécutée + preuve nanoseconde

Directive stricte (remplace toute ambiguïté précédente):
1. **Interdiction de valider la distillation par simple présence de fichiers/registry**.
2. La distillation doit exécuter un pipeline concret `teacher -> student` avec étapes explicites et mesurées.
3. Chaque étape doit produire des preuves horodatées `ts_ns`:
   - `DISTILLATION_START`
   - `DISTILLATION_TEACHER_LOAD_BEGIN/END` (par teacher)
   - `DISTILLATION_BATCH_BEGIN/END`
   - `DISTILLATION_LOSS_STEP`
   - `DISTILLATION_STUDENT_UPDATE`
   - `DISTILLATION_CHECKPOINT_WRITTEN`
   - `DISTILLATION_END`
4. Échec dur si la distillation n'a pas réellement tourné:
   - `raise RuntimeError('DISTILLATION_NOT_EXECUTED_HARD_FAIL')`
5. Preuve persistante obligatoire:
   - journal JSONL nanoseconde,
   - checksum SHA512 des checkpoints,
   - chaînage Merkle des événements distillation.

## 23) Contraintes utilisateur imposées (prioritaires) — à respecter en V6

Conformément à la demande explicite:
- **NE PAS réduire l'espace hyperparamètres**.
- **NE PAS baisser `max_iter`** pour raccourcir artificiellement.
- **NE PAS réduire/échantillonner les logs**; au contraire augmenter la traçabilité.
- **PAS de mode forensic léger**: forensic 100% tout le temps.
- **Augmenter `meta_neurons`** (ou le rendre dynamique croissant) au lieu de rester trop bas.

Implication: l'accélération doit venir de l'architecture d'exécution (parallélisme, cache, reprise exacte, I/O optimisé), pas d'une réduction de charge scientifique.

## 24) Continuité run-après-run maximale (persistance totale)

### 24.1 Ce qui doit être persisté à chaque checkpoint
- curseur exact: `epoch, candidate_index, iter`;
- état complet modèle student + état distillation;
- état RNG (python/numpy/torch);
- contexte dataset (ids fichiers, offsets, hash);
- état métriques cumulées;
- état Merkle précédent/courant.

### 24.2 Règle anti-perte avant timeout
- watchdog temps déclenche `PRETIMEOUT_FLUSH` avant limite Kaggle;
- checkpoint atomique + validation checksum + duplication vers:
  1) output Kaggle,
  2) artefact versionné Kaggle Model.

### 24.3 Reprise exacte au run suivant
- charger `LATEST_VALID_CKPT`;
- vérifier intégrité + compatibilité stricte;
- reprendre exactement à `(epoch,candidate,iter+1)`;
- journaliser `RESUME_EXACT_OK` avec `ts_ns` source/target.

## 25) Correctif de gouvernance de progression (à implémenter)

- Le `% global` affiché doit être le `% réel calculé`, pas un placeholder de phase.
- Afficher simultanément:
  - `global_progress_reel`,
  - `train_progress_reel`,
  - `cursor(epoch,candidate,iter)`,
  - `remaining_estimate_s`.
- Toute divergence entre `% réel` et `% affiché` doit lever `PROGRESS_INTEGRITY_ALERT`.
