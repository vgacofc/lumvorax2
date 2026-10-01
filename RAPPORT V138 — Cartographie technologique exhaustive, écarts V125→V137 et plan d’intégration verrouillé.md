# RAPPORT V138 — Cartographie technologique exhaustive, écarts V125→V137 et plan d’intégration verrouillé

## 0) Périmètre, méthode et statut

- Périmètre audité: dépôt distant `lumc01/Lumvorax` (branche `main`) + dépôt local courant.
- Méthode: inventaire arborescence distante (`git/trees?recursive=1`), vérification des artefacts compétition, revue différentielle ciblée V125→V137, inspection des chaînes C (LUM/VORAX), et vérification de la présence des logs d’exécution récents.
- Statut demandé: **préparation V138** (plan complet), **sans implémentation V138 pour l’instant**, en attente de nouveaux logs V137.

---

## 1) Cartographie technologique exhaustive et normalisée

### 1.1 Inventaire multi-langages (distant, brut)

- Fichiers distants recensés: **4513**.
- Répartition observée:
  - `Markdown`: 945
  - `C/C++ headers`: 543
  - `C`: 209
  - `Python`: 107
  - `JSON`: 135
  - `Lean`: 54
  - `Shell`: 37
  - `C++`: 26
  - `Jupyter`: 3
- Couches principales détectées:
  - **Natif C**: `src/lum`, `src/vorax`, `src/optimization`, `src/persistence`, `src/debug`, `src/tests`
  - **Orchestration ML**: noyaux `nx47_vesu_kernel_v125.py` → `nx47_vesu_kernel_v137.py`
  - **Forensic/rapports**: dossiers `RAPPORT*`, `reports`, `evidence`
  - **Kaggle / métriques compétition**: fichiers de règles et notebook de métrique présents à la racine distante.

### 1.2 Schéma d’architecture versionné (lum → vorax → NX → NX-47 V125→V137)

- **Socle natif** (LUM/VORAX): primitives C, parser, persistance WAL, tracking forensic, optimiseurs SIMD/zero-copy.
- **Couche NX47 Python**: ingestion CT, feature engineering 2.5D, sélection ratio, entraînement supervisé + U-Net 2.5D, export `submission.zip`, artefacts forensic.
- **Évolution V125→V137**:
  - V125–V136: plafonds `max_train_volumes=24`, `max_val_volumes=8` (couverture partielle possible).
  - V137: mode non borné (`max_train_volumes=0`, `max_val_volumes=0`), convergence auto (`supervised_epochs=0`, `auto_epoch_safety_cap=0`) et validation règles compétition.

---

## 2) Preuve d’exécution native C (vs wrapping)

### 2.1 Ce qui est prouvé

- Chaîne de compilation C explicite via `Makefile`/`Makefile.complete` (gcc/clang, optimisation, modules LUM/VORAX, tests dédiés).
- Présence de tests C d’intégration et interopérabilité (`test_interoperabilite_complete.c`, `src/tests/*`).
- Bibliothèque C riche et modulaire: `src/lum/lum_core.c`, `src/vorax/vorax_operations.c`, etc.

### 2.2 Écart critique actuel côté NX47 V125→V137

- Dans les noyaux Python NX47 (V125→V137), **absence de pont natif explicite** (`ctypes`, `cffi`, `CDLL`, pybind, etc.) vers LUM/VORAX.
- Conclusion d’audit: la pile C existe et se compile, mais la preuve d’appel **direct et causal** depuis le pipeline NX47 Kaggle n’est pas démontrée par ces versions.

---

## 3) Chaîne de build reproductible et vérifiable

### 3.1 Actif

- Toolchains déclarées:
  - `Makefile`: `gcc` + flags `-O3 -march=native -fPIC` etc.
  - `Makefile.complete`: `clang` + flags standardisés.
- Modules listés explicitement (permet reproductibilité structurelle).

### 3.2 Manques pour niveau investisseur/audit strict

- Manque de **manifest de build unifié** (versions compilateur + hash artefacts + hash sources + hash flags).
- Manque d’un script unique “from scratch” scellant source→binaire→preuve d’exécution.
- Manque de preuves runtime signées des symboles C effectivement invoqués en run NX47 Kaggle.

---

## 4) Analyse différentielle V125 → V137 (résumé factuel)

- **V125**: configuration de base supervisée avec limites de volumes train/val.
- **V132**: introduction de garde stricte `strict_no_fallback=True`.
- **V134/V135**: ajout de `min_train_pairs_required` et exigence de complétion train.
- **V136**: adaptation seuil dataset (`min_train_pairs_required=786`) + politique métrique hardcoded adoucie (`warn`).
- **V137**: suppression plafonds implicites (couverture dataset complète possible), mode convergence non borné par epochs fixes, validation règles compétition.

Lecture industrielle:
- Gain: meilleure couverture potentielle et moins de faux “train terminé” décoratif.
- Risque: sans télémétrie C native reliée à NX47, la continuité technologique LUM/VORAX n’est pas prouvée bout-en-bout dans le run Kaggle.

---

## 5) Vérification compétition (fichiers distants)

- Fichiers retrouvés sur le dépôt distant:
  - `Competition_Rules_Vesuvius_Challenge _Surface_Detection.md`
  - `vesuvius-2025-metric-demo.ipynb`
- Statut: disponibles pour validation stricte du format soumission et de la logique métrique.

---

## 6) État des logs demandés et prérequis V138

- Logs distants confirmés présents: `...v130`, `...v131`, `...v132`, `...v133`, `...v134.1`, `...v135.2`, `...v136`.
- **Log V137 introuvable** au moment de l’audit (`nx47-vesu-kernel-new-v137.log` absent de l’arborescence distante).
- Décision: implémentation V138 différée; le plan est prêt, exécution après réception du log V137 demandé.

---

## 7) Plan d’intégration V138 verrouillé (sans exception)

### Phase A — Cartographie et verrouillage des dépendances
1. Générer manifeste `V138_TECH_MANIFEST.json`:
   - modules C/C++/CUDA/Python, rôle, statut (actif/legacy/expérimental), dépendances.
2. Générer matrice de continuité `NX1_TO_NX47_CONTINUITY_MATRIX.csv`:
   - mécanisme, version d’introduction, modifications, état final V137.

### Phase B — Preuve d’exécution native C causale
3. Construire `liblum_vorax.so` avec hash SHA256.
4. Ajouter pont explicite dans NX47 (V138) via `ctypes`/`cffi` (sans fallback silencieux).
5. Instrumenter hooks runtime:
   - compteur d’appels C,
   - temps cumulés,
   - symboles invoqués,
   - impact mesurable sur sortie/gradient.
6. Ajouter test d’ablation causale:
   - sortie avec C actif vs C neutralisé,
   - delta métrique et delta décision.

### Phase C — Build reproductible certifié
7. Script unique `scripts/build_v138_from_scratch.sh`:
   - versions compilateurs, flags, hash sources, hash binaire final, SBOM minimal.
8. Fichier `BUILD_PROVENANCE_V138.json` signé (hash chain).

### Phase D — Validation compétition stricte
9. Contrôle de format `submission.zip` (noms, dtype, dimensions exactes).
10. Contrôle métrique avec notebook de démonstration + comparaison résultat local.

### Phase E — Non-régression et scénarios d’échec
11. Tests unitaires + intégration:
   - succès attendus,
   - échecs contrôlés (labels absents, mismatch shape, métriques invalides).
12. Benchmarks:
   - C pur vs pipeline complet,
   - coût CPU/GPU/RAM/IO vs gain métrique.

### Phase F — Dossier investisseur / production
13. Pack figé:
   - code hashé,
   - binaires hashés,
   - logs certifiés,
   - résultats reproductibles,
   - checklist d’audit signée.
14. Classification explicite par module:
   - R&D, Démo, Production-ready.

### Go / No-Go V138
- Go seulement si:
  - preuve d’exécution causale C validée,
  - non-régression validée,
  - conformité compétition validée,
  - dossier provenance complet.
- No-Go si un critère manque.

---

## 8) Avant / Après attendu (V137 → V138)

- Avant (V137): pipeline Kaggle renforcé, couverture non bornée possible, règles compétition intégrées; mais preuve causale d’usage C natif LUM/VORAX incomplète côté runtime NX47.
- Après (V138 planifié): chaîne bout-en-bout prouvée source→binaire→appel runtime→impact métrique, avec reproductibilité et traçabilité niveau investisseur.

---

## 9) Clause de falsification et limites assumées

La technologie sera déclarée **non validée** si:
- le pont C est compilé mais jamais appelé en run réel,
- l’ablation C n’a aucun effet causal sur décisions/métriques,
- la reproduction binaire échoue avec les mêmes sources/flags,
- la conformité compétition ne passe pas strictement,
- ou les logs de preuve ne sont pas certifiables.

---

## 10) Prochaine étape opérationnelle

- **En attente de vos nouveaux logs V137** (et artefacts associés).
- Dès réception:
  1. audit détaillé run V137,
  2. correction des écarts prouvés,
  3. implémentation V138 selon le plan verrouillé ci-dessus,
  4. rapport avant/après certifié.

---

## 11) Clarification critique — ce que veut dire « appel »

Pour éviter toute ambiguïté:

- **Appel externe réseau** = HTTP/API/GitHub/Kaggle internet.
  - **Interdit** pendant la soumission Kaggle (internet off).
- **Appel interne local** = invocation d’une fonction/bibliothèque locale déjà présente dans le notebook/run (`.so`, `.a`, code Python, code C compilé localement).
  - **Autorisé** tant que tout est embarqué dans le notebook/dataset Kaggle.

Donc, quand je parle d’« appel C », cela signifie dans V138:
1. compiler localement le code C/C++ dans la session Kaggle,
2. charger le binaire local (`.so`) via Python,
3. appeler les symboles natifs sans internet.

Aucun accès réseau requis pour ce chemin.

---

## 12) Spécification V138 — intégration C/C++ 100% Kaggle offline

### 12.1 Principe d’intégration (sans appel réseau)

- Les composants LUM/VORAX C/C++ sont fournis directement dans le notebook (cellules), sans dépendre d'un dataset Kaggle attaché.
- Build local dans une cellule de notebook:
  - `gcc/clang` pour produire `liblum_vorax.so`.
- Chargement local en Python (`ctypes.CDLL`) depuis un chemin local Kaggle (`/kaggle/working/liblum_vorax.so`).
- Journalisation stricte:
  - hash du source C,
  - hash du binaire `.so`,
  - table des symboles exportés,
  - compteurs d’appels runtime.

### 12.2 Cellules notebook cibles (ordre recommandé)

1. **Cellule A (sources natifs)**: écrire/extraire les `.c/.h/.cpp` nécessaires.
2. **Cellule B (build)**: compiler en `.so` (flags figés, logs complets).
3. **Cellule C (binding Python)**: déclarer signatures `ctypes` et vérifications ABI.
4. **Cellule D (self-tests natifs)**: tests unitaires C minimaux avant entraînement.
5. **Cellule E (pipeline NX47)**: entraînement/inférence utilisant les appels natifs.
6. **Cellule F (forensic)**: export des preuves (hash, symboles, compteurs, timing).

### 12.3 Contrats stricts d’échec (fail-fast)

- Si compilation C échoue → arrêt immédiat.
- Si symbole natif absent → arrêt immédiat.
- Si mismatch signature ABI → arrêt immédiat.
- Si compteur d’appels natifs = 0 en run nominal → arrêt immédiat.
- Si soumission non conforme règles compétition → arrêt immédiat.

---

## 13) Extension du plan de revue statique « ligne par ligne » (sans exécution)

Pour répondre à la demande d’audit exhaustif sans exécuter le code, V138 intégrera un protocole formel:

### 13.1 Registre de revue globale

- Inventaire complet de tous les fichiers par langage:
  - C/C++ headers, C, C++, Python, JSON, Lean, Shell, Jupyter, Markdown.
- Pour chaque fichier:
  - chemin,
  - hash SHA256,
  - nombre de lignes,
  - module/couche associée,
  - statut (actif/legacy/expérimental/désactivé).

### 13.2 Liaison code ↔ documentation

- Matrice `CODE_DOC_TRACE_MATRIX`:
  - fichier code,
  - markdown(s) liés,
  - incohérences détectées,
  - action corrective prévue.

### 13.3 Cartographie des technologies hors LUM/VORAX

- Intégrer explicitement les autres briques détectées dans le dépôt (au-delà de LUM/VORAX):
  - modules `optimization/*`, `persistence/*`, `security/*`, `versioning/*`, `sch/*`, `proofs/*`, etc.
- Marquer pour chacune:
  - utilité potentielle pour NX47,
  - coût d’intégration,
  - priorité V138 (P0/P1/P2).

---

## 14) Ajouts « expert inspecteur » — autocritique Q/R

### Q1 — Comment prouver qu’un module C est réellement utilisé?
- **Réponse**: pas seulement présent/compilé; il faut preuve causale runtime (compteur d’appels > 0 + impact ablation + traces symboles).

### Q2 — Comment éviter une intégration cosmétique?
- **Réponse**: test d’ablation obligatoire (avec/sans module C) + delta mesuré sur sorties/métriques + logs signés.

### Q3 — Comment assurer conformité Kaggle sans internet?
- **Réponse**: tout embarquer localement (sources, build, dépendances), zéro fetch réseau en run de soumission.

### Q4 — Que faire si une technologie détectée n’est pas intégrée dans V138?
- **Réponse**: la déclarer explicitement dans un registre « non-intégrée », avec justification, risque, plan et échéance.

### Q5 — Comment traiter les contradictions code/rapport?
- **Réponse**: priorité à la preuve exécutable et logs; le rapport doit être corrigé si divergence.

### Q6 — Qu’est-ce qui invalide immédiatement la validation investisseur?
- **Réponse**: absence de provenance binaire, absence d’appel causal prouvé, ou impossibilité de reproduction from-scratch.

---

## 15) Mise à jour de feuille de route V138 (complément)

- **A1**: inventaire ligne-par-ligne statique complet (tous langages + markdown liés).
- **A2**: sélection des briques C/C++ réellement intégrables en notebook Kaggle offline.
- **B1**: empaquetage des sources natifs minimum viables dans cellules/dataset.
- **B2**: build local + binding Python + tests ABI.
- **C1**: instrumentation forensic causale (appels, temps, contribution).
- **C2**: ablation tests obligatoires.
- **D1**: conformité compétition stricte (format + métrique).
- **D2**: dossier investisseur (code→binaire→run→résultat).

Statut: plan étendu prêt; exécution V138 effective après réception de vos nouveaux résultats V137.

---

## 16) Définition formelle de « l’appel » (exécutable Kaggle)

### 16.1 Typologie

1. **Appel compile-time**
   - Exemple: `gcc ... -shared -o libx.so`
   - Effet: produit le binaire natif, pas encore exécuté dans le pipeline.
2. **Appel runtime local**
   - Exemple: `ctypes.CDLL('/kaggle/working/libx.so').fn(...)`
   - Effet: exécute la fonction native en local, sans réseau.
3. **Appel in-process**
   - Extension native chargée dans le même process Python (pas de service externe).
4. **Appel symbolique**
   - Macro/inline côté C/C++ (preuve via build logs/symboles/objdump selon cas).

### 16.2 Contraintes Kaggle + preuves

- Réseau interdit en soumission: aucun téléchargement runtime.
- Preuves minimales exigées:
  - hash sources + hash `.so`,
  - logs build,
  - table symboles exportés,
  - compteurs d’appels runtime non nuls.

---

## 17) Stratégie d’intégration C/C++ 100% offline — décision V138

### 17.1 Options évaluées

- `setuptools + Extension`
- inline `%%bash` + `gcc/clang`
- `pybind11`
- `ctypes` + `.so`

### 17.2 Choix cible recommandé

- **Primary path V138**: `gcc/clang` local + `ctypes` (`.so` local), car:
  - simple en notebook,
  - traçable,
  - pas de dépendance internet,
  - fail-fast facile à imposer.

### 17.3 Mapping obligatoire (à produire en V138)

- `fichier C/C++ -> cellule notebook -> wrapper Python -> étape NX47`.

---

## 18) Traçabilité symbole -> wrapper -> résultat

V138 doit produire une table `NATIVE_SYMBOL_TRACE.csv`:

- `symbol_name`
- `library_path`
- `python_wrapper`
- `pipeline_stage`
- `input_checksum`
- `output_checksum`
- `call_count`
- `avg_latency_ns`

But: démontrer contribution causale de chaque brique native.

---

## 19) Vérification d’exécution effective (anti-code mort)

### 19.1 Instrumentation minimale

- Compteurs d’appels par symbole.
- Timestamps nanoseconde par appel critique.
- Checksums entrées/sorties bufferisées.

### 19.2 Ablation binaire

- Run A: natif activé.
- Run B: natif neutralisé.
- Exiger `delta != 0` sur au moins une métrique/sortie décisionnelle, sinon marquer module « non causal ».

---

## 20) Registre de lecture exhaustive ligne-par-ligne (preuve humaine)

Le registre V138 devra inclure:

- `file_path`
- `line_range_reviewed`
- `identified_role`
- `status` (`used|obsolete|to_integrate|experimental`)
- `linked_markdown`
- `review_notes`

Ce registre est distinct de l’exécution: il documente la lecture analytique humaine et le tri technologique.

---

## 21) Intégration des technologies pré-Vesuvius

V138 doit ajouter un catalogue `PRE_VESUVIUS_TRANSFER_CATALOG.md`:

- technologie,
- compétition/problème d’origine,
- mécanisme clé,
- transférabilité vers NX47 (forte/moyenne/faible),
- justification,
- risque,
- priorité d’intégration.

Objectif: éviter la perte du capital R&D historique hors Vesuvius.

---

## 22) Plan de validation multi-niveaux V138

1. **Niveau 1 (unité)**: fonctions C/C++ isolées.
2. **Niveau 2 (intégration)**: pont C/C++ <-> Python.
3. **Niveau 3 (pipeline)**: NX47 complet avec preuves forensic.
4. **Niveau 4 (Kaggle dry-run)**: vérification format/artefacts soumission.

Chaque niveau dispose de critères go/no-go bloquants.

---

## 23) Extension scientifique cadrée — simulation carbonisation

### 23.1 Cadre

- Cette piste est **R&D avancée**, non bloquante pour la conformité V138 baseline.
- Elle doit rester falsifiable et mesurable.

### 23.2 Hypothèses explicites à formaliser

- matériaux papyrus/encre/liants,
- transformations thermiques/oxydatives,
- niveau de fidélité choisi (atomique/mésoscopique/voxel scan).

### 23.3 Limites assumées

- coût HPC,
- simplifications physico-chimiques,
- domaine de validité.

---

## 24) Chaîne directe/inverse pour exploitation ML

- **Direct**: état originel -> carbonisé -> simulation scan.
- **Inverse**: scan carbonisé -> estimation état originel.
- **Couplage**: données synthétiques augmentent/régularisent la détection d’encre réelle.

Critère clé: gain mesurable sur validation réelle, pas seulement sur synthétique.

---

## 25) Dossier de preuve exécution-réalité (investisseur/auditeur)

V138 devra livrer:

- `SOURCE_MANIFEST` (hash sources),
- `BUILD_PROVENANCE` (compilo/flags/hash binaire),
- `RUNTIME_EVIDENCE` (compteurs symboles, timings, checksums),
- `RESULT_LINKAGE` (traçabilité sortie <- code exécuté),
- `NO_SHORTCUT_DECLARATION` (aucun stub/fallback masqué).

---

## 26) Analyse du dernier log GitHub `nx47-vesu-kernel-new-v137.log` (récupération + diagnostic)

### 26.1 Résultat de récupération

- Log récupéré depuis GitHub: `nx47-vesu-kernel-new-v137.log`.
- Taille observée: 3454 lignes.

### 26.2 Faits observés dans le run V137

- `TRAIN_DISCOVERY`: `pair_count=786`.
- `TRAIN_DATASET_AUDIT`:
  - `pair_count_discovered=786`
  - `pair_count_selected_for_training=786`
  - `coverage_pct_selected_vs_discovered=100.0`
  - `train_image_files_found=786`
  - `train_label_files_found=786`
- Le run n’a pas émis de `GLOBAL_STATS` final ni `EXEC_COMPLETE`.
- Le run s’arrête sur exception:
  - `KeyError: 'best_objective'`
  - source: boucle convergence supervisée, référence à une clé absente dans `epoch_best`.

### 26.3 Cause racine

- Incohérence de clé interne dans l’entraînement supervisé:
  - `epoch_best` contient `objective` (et non `best_objective`).
  - l’algorithme tentait de lire `epoch_best['best_objective']` pour la logique de stagnation.

### 26.4 Correctif V138 associé

- Correctif de robustesse appliqué dans le noyau V138:
  - utilisation cohérente de `epoch_best['objective']` pour `best_obj_seen`.
- Effet attendu:
  - suppression du `KeyError`,
  - poursuite normale de la boucle train/val,
  - émission des artefacts finaux (`GLOBAL_STATS`, `EXEC_COMPLETE`) si aucune autre erreur.

---

## 27) Exigences opérationnelles ajoutées V138 (demandes run-safe)

### 27.1 Progression détaillée en temps réel

V138 doit journaliser des progressions:

- par étape,
- par sous-étape,
- par fichier individuel,
- et progression globale.

Format attendu: `% + barre de progression` dans les événements `PROGRESS_UPDATE`.

### 27.2 Pré-vol « 5% » avant run complet (>3h)

Avant l’exécution complète, V138 doit lancer un pré-contrôle rapide:

- charger/valider **5% train**,
- charger/valider **5% test**,
- échouer tôt en cas d’erreur d’I/O, format, shape ou intégrité.

Objectif: éviter de découvrir des erreurs bloquantes après plusieurs heures de calcul.

### 27.3 Contrainte maintenue sur composants natifs

- Les composants LUM/VORAX C/C++ sont fournis directement dans les cellules du notebook Kaggle (sans dépendance dataset attaché).
- Build/chargement local uniquement, sans internet.

---

## 28) Analyse des erreurs/warnings Papermill sur run V138 et correctif direct

### 28.1 Erreur bloquante observée

- Exception: `AttributeError: 'PlanTracker' object has no attribute 'overall_progress'`.
- Contexte: la nouvelle journalisation `PROGRESS_UPDATE` appelait `self.plan.overall_progress()` alors que la méthode n'était pas implémentée dans `PlanTracker`.
- Impact: crash immédiat au début de `discover_inputs()`, avant le traitement principal.

### 28.2 Correctif appliqué dans V138

- Ajout de `PlanTracker.overall_progress()` (moyenne des pourcentages d'étapes).
- Harmonisation de `_write()` pour utiliser `self.overall_progress()` (source unique de vérité).
- Effet attendu: la journalisation de progression ne peut plus déclencher cette `AttributeError`.

### 28.3 Warnings non bloquants vus dans le log

1. **Debugger frozen modules warning**
   - Message: `-Xfrozen_modules=off` recommandé.
   - Nature: warning d'environnement debug, **non bloquant** pour le pipeline Kaggle de soumission.

2. **Pip duplicate install lines** (`imagecodecs`, `tifffile`)
   - Nature: redondance d'installation/logging, **non bloquante**.
   - Action recommandée V138+: verrouiller une seule routine d'installation par dépendance pour réduire bruit et temps.

3. **FutureWarning traitlets/nbconvert**
   - Nature: dépréciations de paramètres CLI, **non bloquantes** pour le calcul modèle.

### 28.4 Priorité de correction

- P0 (bloquant): `overall_progress` manquant -> **corrigé**.
- P1 (qualité run): réduire duplications pip.
- P2 (hygiène notebooks): nettoyage options nbconvert/traitlets pour logs plus propres.

---

## 29) Mise à jour corrective demandée — retour couverture v136 + progression complète

Suite à la lecture des logs `nx47-vesu-kernel-new-v138.log` et `nx47-vesu-kernel-new-v138.1.log`:

- retour config par défaut V138 sur la stratégie v136:
  - `max_train_volumes=24`
  - `max_val_volumes=8`
- objectif attendu avec 786 paires:
  - `coverage_pct_selected_vs_discovered ≈ 4.07%` (32/786)

Renforcement de la progression en temps réel:

- progression par étapes/sous-étapes/fichiers/global,
- progression dans les boucles de chargement train/val,
- progression dans la recherche hyperparamètres supervisée,
- progression dans les itérations internes de `fit_prox`,
- heartbeat explicite pour distinguer “calcul long actif” vs blocage.

Justification:

- les runs précédents pouvaient paraître figés en UI Kaggle mobile,
- alors que le calcul était encore en cours,
- jusqu’à atteindre la limite d’exécution Kaggle (9h).
