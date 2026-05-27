# PLAN + FEUILLE DE ROUTE V4 — Réponses expertes à tes questions

## 0) Domaines d’expertise mobilisés en temps réel

Pour répondre correctement à tes questions, les domaines nécessaires identifiés sont:
1. **Computer Vision 3D (TIFF volumique Vesuvius)**
2. **ML Engineering (pipeline train/infer, calibration seuil)**
3. **Performance Engineering (CPU/IO, coût runtime)**
4. **Forensic Observability (logs, Merkle, bit-capture, invariants)**
5. **MLOps Kaggle Offline (dépendances wheel offline, packaging submission.zip)**
6. **Comparative ML (AGNN heuristique vs CNN backprop / SOTA)**
7. **Méthodes formelles & limites épistémiques** (ce qu’on peut ou non conclure des logs)

---

## 1) Réponse claire à chacune de tes questions

## Q1 — « slab_allocate adapte massivement la capacité neuronale »: ça veut dire quoi exactement ?

### Réponse claire
Dans V3, le nombre de neurones actifs n’est pas fixe. Il est recalculé à chaque volume avec:
- taille du tenseur,
- variance du signal,
- proxy d’entropie (gradient).

Formule utilisée (en simplifié):
`required = slab_min + size_term + variance*1500 + entropy*900`.

### Exemple de comparaison réel
Dans les métriques V3:
- minimum neurones actifs ≈ **1,132,395**
- maximum neurones actifs ≈ **5,052,655**

=> amplitude ~4.46x entre cas simples et cas complexes.

### C’est-à-dire ?
Le modèle « ouvre plus de capacité » quand le volume est plus variable/complexe.

### Donc ?
Ce n’est pas figé; la complexité des données pilote la capacité allouée.

### Conclusion
Allocation adaptative réelle, observable dans les logs et métriques.

---

## Q2 — « A-t-il zoomé au maximum chaque TIFF couche par couche avant test ? »

### Réponse claire
Pas au sens « zoom multi-échelle pyramidal » type microscope numérique.
Le pipeline V3 traite le volume tel quel, couche/slice par slice via gradients 3D puis projection énergétique moyenne.

### C’est-à-dire ?
Il y a bien traitement sur la profondeur (axe z), mais pas un module explicite de zoom hiérarchique multi-résolution.

### Donc ?
Si tu veux un vrai zoom multi-échelle, il faut l’ajouter en V4 (pyramides 1x/0.5x/0.25x + fusion).

### Conclusion
Traitement volumique profond oui, zoom hiérarchique maximal non.

---

## Q3 — « Signal d’apprentissage hétérogène réaliste, non figé/non hardcodé »: ça veut dire quoi ?

### Réponse claire
Les statistiques changent beaucoup d’un volume à l’autre:
- neurones actifs varient fortement,
- ink_ratio varie fortement,
- seuil local train varie selon fragment.

Si c’était hardcodé, on verrait des sorties quasi constantes.

### Exemple
- `ink_ratio` va de **0.01326172** à **1.0**
- neurones actifs varient de ~1.13M à ~5.05M

### C’est-à-dire ?
Les données imposent des réponses différentes; le modèle ne renvoie pas toujours la même forme.

### Donc ?
Hétérogénéité = comportement dynamique cohérent avec dataset réel variable.

### Conclusion
C’est un signal d’apprentissage « vivant », pas un script figé.

---

## Q4 — `ink_ratio min/max/moy: 0.01326172 / 1.0 / 0.50129136` : interprétation ?

### Réponse claire
`ink_ratio = ink_pixels / total_pixels` pour chaque item.

- **min 0.01326 (~1.3%)**: cas très peu encré (ici l’inférence test)
- **max 1.0 (100%)**: cas train où tout passe au seuil (possiblement seuil local permissif)
- **moy ~0.50**: équilibre global proche 50% sur l’ensemble train+infer

### Exemple simple
Pour une image de 102,400 pixels:
- ratio 0.013 -> ~1,358 pixels encre
- ratio 1.0 -> 102,400 pixels encre

### C’est-à-dire ?
L’échelle de difficulté et de densité varie énormément entre volumes.

### Donc ?
Un seul seuil universel brut est fragile; la calibration robuste est indispensable.

### Conclusion
Ces ratios expliquent pourquoi il faut calibrer/contraindre le seuil en V4.

---

## Q5 — « warnings nbconvert non bloquants »: c’est quoi exactement ?

### Réponse claire
Ce sont des avertissements de conversion notebook→html (`mistune`, `traitlets`, `nbconvert`) affichés en fin d’exécution.

Ils arrivent **après** `READY` et `EXEC_COMPLETE`.

### Exemple de comparaison
- **bloquant**: erreur `RuntimeError` avant `submission.zip` (échec pipeline)
- **non bloquant**: warning nbconvert en post-traitement, alors que `submission.zip` existe déjà

### Conclusion
Ce bruit ne casse pas la soumission; c’est du tooling notebook, pas du cœur ML.

---

## Q6 — « coût CPU élevé `cpu_total_ns ≈ 1.400e12` »: concret ?

### Conversion
`1.400e12 ns ≈ 1400 s ≈ 23.3 minutes CPU`.

Pour 787 items (786 train + 1 infer), ordre de grandeur moyen:
~1.78 s/item (approximation grossière, hors overhead variable).

### C’est-à-dire ?
Le pipeline est correct mais coûteux à grande échelle.

### Donc ?
Optimisation nécessaire avant itérations massives.

### Conclusion
Performance acceptable pour audit, mais à optimiser pour compétition intensive.

---

## Q7 — « optimisation batch/IO possible »: exemples clairs ?

## Exemples concrets V4
1. **IO**: cache mémoire-mappé des TIFF fréquemment relus.
2. **Batch slices**: gradients calculés par blocs z (évite surcoûts Python).
3. **Préfiltrage train**: preflight quality score pour éviter 786 trainings complets à chaque run.
4. **Parallélisme**: loader multiprocess + queue.
5. **GPU path optionnel** pour projection/gradients.

### C’est-à-dire ?
Même résultat logique, mais moins de temps perdu en lecture/overhead.

### Donc ?
Plus d’itérations de tuning dans le même budget temps Kaggle.

### Conclusion
On peut accélérer sans sacrifier la traçabilité forensic.

---

## Q8 — « diversité des volumes / données non triviales / seuil médian stabilisateur »

### Diversité des volumes (exemple)
Le dataset contient `train_images`, `train_labels`, `test_images`, mais aussi des dossiers `deprecated_*`; cela montre une structure historique mixte.

### Données non triviales
La dispersion ink_ratio + variance/entropie + adaptations neuronales montre un problème non uniforme.

### Seuil médian stabilisateur
V3 entraîne un seuil par volume puis prend la médiane globale (~0.0428) pour éviter qu’un outlier domine.

### Exemple comparaison
- **moyenne**: sensible à extrêmes (ratios proches 0 ou 1)
- **médiane**: plus robuste, meilleure stabilité inter-volumes

---

## Q9 — « AGNN heuristique/énergétique vs CNN backprop »: très clair

## AGNN V3
- score basé sur gradients/énergie + seuil calibration,
- forte auditabilité (bit log, merkle, events),
- moins dépendant d’un gros entraînement deep.

## CNN classique
- apprend les filtres via backprop end-to-end,
- meilleur potentiel SOTA si gros entraînement + labels + tuning,
- plus opaque sans instrumentation spécifique.

### Conclusion
AGNN = robuste/auditable; CNN = souvent meilleur score brut si très bien entraîné.

---

## Q10 — « potentiellement moins performant qu’un CNN SOTA » + comment dépasser ?

### Pourquoi potentiellement moins performant
Parce que V3 est surtout un pipeline heuristique robuste, pas un modèle profond optimisé leaderboard.

### Stratégie réaliste pour dépasser (ou concurrencer) un CNN SOTA

## Axe A — Reprendre les solutions NX-47 déjà présentes
Depuis NX-47 v130/v135/v136 on peut réintégrer:
1. **Chemin U-Net 2.5D** (`use_unet_25d`, `patch_size`, `unet_epochs`)
2. **threshold_scan calibré** (F-beta) au lieu d’un seul seuil fixe
3. **pseudo-labels contrôlés** (`pseudo_pos_pct`, `pseudo_neg_pct`)
4. **meta-neurons / memory evolution** pour robustesse inter-volumes
5. **strict_no_fallback + preflight coverage** pour fiabilité compétition

## Axe B — Architecture hybride V4
- CNN 2.5D pour score principal
- AGNN V3 pour:
  - prior/regularization,
  - contrôle qualité,
  - forensic integrity,
  - validation de soumission.

## Axe C — Optimisation entraînement fin
- CV interne fragment-wise
- TTA contrôlée
- calibration température/logits
- post-proc morphologique/CC optimisée
- ensembling (CNN + AGNN prior)

### Conclusion
La voie la plus solide est **hybride**: CNN pour performance, AGNN pour contrôle et robustesse.

---

## 2) Feuille de route V4 (A→Z)

## Phase 1 — Consolidation I/O & Invariants (J1)
- [ ] verrouiller invariants succès (`pixels>0`, zip validé, roadmap 100% réel)
- [ ] corriger sérialisation roadmap (`finalize_forensics`)
- [ ] tests unitaires discovery/layout

## Phase 2 — Speed Layer (J1–J2)
- [ ] profiling CPU hotspots
- [ ] cache IO + batch z
- [ ] parallélisation loader

## Phase 3 — Hybrid ML (J2–J4)
- [ ] intégrer voie U-Net 2.5D (inspirée NX-47)
- [ ] conserver AGNN prior énergétique
- [ ] fusion contrôlée logits/priors

## Phase 4 — Calibration & Validation (J4–J5)
- [ ] threshold_scan multi-seuil
- [ ] CV locale + ablation
- [ ] métriques stabilité/robustesse

## Phase 5 — Forensic + Kaggle Packaging (J5)
- [ ] artefacts finaux reproductibles
- [ ] rapport final auto (perf + forensic + risques)

---

## 3) Comparaisons rapides demandées (résumé)

1. **slab fixe vs slab dynamique**:
   - fixe: ex. toujours 2M neurones
   - dynamique: 1.13M → 5.05M selon volume

2. **pipeline bloquant vs warning non bloquant**:
   - bloquant: RuntimeError avant zip
   - non bloquant: warning nbconvert après `READY`

3. **seuil moyen vs médian**:
   - moyenne sensible outliers
   - médiane robuste aux extrêmes

4. **AGNN vs CNN**:
   - AGNN: explicable/auditable
   - CNN: potentiel score brut supérieur

---

## 4) Questions de contrôle pour toi (as-tu bien compris ?)

1. Peux-tu expliquer pourquoi `effective_data_root` est plus important que `configured_data_root` en Kaggle ?
2. Sais-tu lire dans les logs la différence entre « run lancé » et « run utile » ?
3. Sais-tu pourquoi `zip_members_validated=true` est critique pour la soumission ?
4. Peux-tu distinguer « warning nbconvert » d’une erreur modèle ?
5. Pourquoi un système hybride CNN+AGNN est souvent plus sûr en compétition ?

---

## 5) Conclusion

- Tes questions sont pertinentes et pointent exactement les zones critiques: interprétation métrique, coût CPU, nature de l’apprentissage, comparaison SOTA.
- Le run V3 est valide opérationnellement.
- La V4 doit maintenant viser **performance + calibration + hybridation CNN/AGNN**, sans perdre la traçabilité forensic qui est déjà un vrai point fort.

---

## 6) Complément demandé — Détection de matériaux (fibres papyrus vs encre vs autres)

## 6.1 État actuel V3 (honnête et précis)

Le V3 actuel détecte principalement un **score d’énergie d’encre** puis binarise par seuil.
Il ne possède pas encore une tête explicite multi-classes de matériaux (`papyrus_fiber`, `ink`, `background`, `artefact`).

### C’est-à-dire ?
- Oui: il sépare implicitement « encre probable » vs « non-encre ».
- Non: il ne te renvoie pas une carte sémantique détaillée des matériaux.

### Donc ?
Pour répondre à « fibres du papyrus ou encre ou autre », il faut ajouter une classification matériaux dédiée en V4.

## 6.2 Plan V4 matériaux (100% natif)

### Étape M1 — Features natives matériaux (sans modèle externe)
Créer un vecteur features par pixel/patch:
1. gradients multi-axes (x,y,z),
2. texture locale (variance, entropie locale, Laplacien),
3. continuité anisotrope (orientation fibreuse),
4. stabilité inter-slices (persistance z),
5. signatures de compacité/linéarité morphologique.

### Étape M2 — Tête multi-classes native
- entraîner un classifieur natif léger (logit multiclasse / CRF simple / MLP léger) sur labels proxy + labels réels disponibles.
- sortir 4 cartes:
  - `P(ink)`
  - `P(fiber)`
  - `P(background)`
  - `P(artefact)`

### Étape M3 — Fusion avec tête ink finale
`ink_final = P(ink) * (1 - P(artefact)) * prior_structural_consistency`

### Étape M4 — Forensic matériaux
Exporter:
- `material_confusion_matrix.csv`
- `material_ratio_report.json`
- overlays couleur par classe.

---

## 7) « Mode de raisonnement » du pipeline — as-tu compris comment il conclut ?

## Chaîne causale actuelle
1. découverte dataset ->
2. normalisation volume ->
3. projection énergétique (gradients) ->
4. seuil local train ->
5. agrégation médiane seuil global ->
6. inférence test ->
7. validation zip.

### C’est-à-dire ?
Le modèle conclut par **règles quantitatives explicites** (pas boîte noire totale).

### Donc ?
Tu peux auditer chaque étape via logs (`SLAB_ALLOCATION`, `TRAIN_DONE`, `THRESHOLD_SELECTED`, `INFER_DONE`).

---

## 8) « Programmé comme un CNN ou totalement autonome ? »

Réponse: **ni l’un ni l’autre à 100%**.
- Ce n’est pas un CNN pur end-to-end backprop en V3.
- Ce n’est pas non plus “autonome magique”: c’est un pipeline heuristique programmé, avec adaptation data-driven.

### En bref
- **Programmé**: oui (règles, formules explicites).
- **Adaptatif**: oui (selon variance/entropie/données).
- **Auto-apprentissage profond CNN**: partiel/non central en V3.

---

## 9) « Comment sait-on qu’il a réellement appris ? »

Critères observables (et déjà présents):
1. seuils locaux varient par volume train,
2. seuil global résultant est calculé (pas hardcodé),
3. métriques non constantes (neurones/ink_ratio variés),
4. infer test passe avec ratio différent des trains,
5. submission validée.

### Limite importante
Cela prouve un **apprentissage opérationnel heuristique**, pas un apprentissage profond SOTA de type CNN multi-époques.

---

## 10) « Détecte-t-il les couleurs et nuances ? »

Sur Vesuvius ici, les TIFF sont majoritairement exploités en **intensité** (grayscale volumique), pas en RGB matériel.

### Donc ?
- nuances de gris: **oui** (via gradients/texture/intensité)
- “couleurs” au sens RGB spectral: **non natif dans V3**

### Action V4 possible
Ajouter pseudo-canaux physiques dérivés (gradient magnitude, laplacien, cohérence orientation) jouant le rôle de « canaux couleur techniques ».

---

## 11) Objectif demandé: modèle 100% natif (sans dépendre d’autres modèles)

## Définition pratique
- Pas d’appel API externe,
- pas de checkpoint externe opaque,
- tout entraînement/inférence dérivé du dataset Kaggle monté + code local.

## Plan natif V4
1. conserver noyau AGNN natif,
2. ajouter classifieur matériaux natif,
3. ajouter calibrateur natif multi-seuil,
4. ajouter post-proc morphologique native,
5. garder forensic complet.

## Option « hybride interne »
Même si on ajoute une branche CNN 2.5D, elle reste native si entraînée localement offline dans le notebook.

---

## 12) Mise à jour feuille de route V4 (ajout matériaux + natif)

## Phase 6 — Material Intelligence (J5–J6)
- [ ] features fibres/encre/artefacts
- [ ] tête multiclasse matériaux
- [ ] overlays matériaux auditables

## Phase 7 — Native-Only Hardening (J6)
- [ ] aucun artefact externe opaque requis
- [ ] reproductibilité totale offline
- [ ] tests d’ablation “avec/sans branche CNN interne”

## Phase 8 — Benchmark Final (J7)
- [ ] comparaison AGNN pur vs hybride interne
- [ ] comparaison contre baseline CNN (mêmes données)
- [ ] rapport final décisionnel v4

---

## 13) Mini test de compréhension (nouvelle série)

1. Peux-tu expliquer la différence entre « détection encre binaire » et « segmentation matériaux multiclasse » ?
2. Pourquoi un pipeline natif peut contenir une branche CNN **et** rester natif ?
3. Quel est le risque si on confond nuances de gris et vraies couleurs spectrales ?
4. Quelle preuve forensic te montre que le raisonnement a suivi toutes les étapes ?
5. Que faudrait-il mesurer pour affirmer une vraie amélioration SOTA ?

---

## 14) Auto-apprentissage profond 100% natif — solution proposée (sans dépendre de modèles concurrents)

## 14.1 Définition stricte de « 100% natif »

Un V4 est considéré 100% natif si:
1. il n’importe **aucun checkpoint externe pré-entraîné**,
2. il n’appelle **aucune API modèle distante**,
3. il apprend uniquement à partir des volumes Kaggle montés + règles/objectif internes,
4. tous les poids sont entraînés/recalculés offline dans le notebook.

## 14.2 Architecture V4 native (self-supervised + supervised léger)

### Bloc A — Pré-apprentissage auto-supervisé volumique (native)
Objectif: apprendre des représentations profondes sans labels d’encre.

- **Task 1: Masked Volume Reconstruction (MVR)**
  - masquer aléatoirement des patches 2.5D,
  - reconstruire les intensités masquées,
  - loss: L1 + gradient consistency.

- **Task 2: Contrastive Slice Consistency (CSC)**
  - paires positives = slices voisines d’un même volume,
  - paires négatives = slices de volumes différents,
  - loss: InfoNCE simplifiée.

- **Task 3: Fiber Orientation Pretext (FOP)**
  - prédire orientation locale dominante (angles quantifiés) via gradients structurels,
  - force le backbone à capturer la texture fibreuse du papyrus.

Résultat: backbone natif pré-entraîné sur la physique des scans, sans modèle externe.

### Bloc B — Tête matériaux multi-classes (native)

Sorties par pixel:
- `P(ink)`
- `P(fiber)`
- `P(background)`
- `P(artefact)`

Loss composite:
- CE pondérée + Dice multi-classes + contrainte topologique légère.

### Bloc C — Tête compétition finale

Fusion explicite:
`P_final_ink = P(ink) * (1 - P(artefact)) * consistency_z`

Puis calibration dynamique par `threshold_scan` (déjà présent dans NX-47) au lieu d’un seul seuil fixe.

## 14.3 Pourquoi cette approche n’est pas « dépendante d’un CNN concurrent »

- Le backbone peut être de type conv 2.5D, **mais entraîné from scratch localement**.
- L’algorithme d’apprentissage est natif au projet (prétextes + calibration + forensic), pas importé d’un modèle fermé.

Donc: deep learning oui, dépendance externe non.

## 14.4 Réemploi concret des briques déjà résolues dans le dépôt

Depuis NX-47 (v136/v139), déjà disponible:
- `use_unet_25d`, `patch_size`, `unet_epochs`, batching,
- `threshold_scan` + `fbeta` calibration,
- `pseudo_labels` contrôlés,
- `strict_no_fallback`, coverage checks,
- mémoire évolution (adapt learning rate / mutation / pruning).

V4 doit réutiliser ces briques **dans un pipeline natif unifié**, pas comme fallback secondaire.

## 14.5 Preuve d’apprentissage réel à exiger pour valider V4

Critères obligatoires (anti-smoke):
1. courbes loss auto-supervisée décroissantes (MVR/CSC/FOP),
2. amélioration mesurable des métriques matériaux sur validation interne,
3. calibration threshold stable inter-volumes,
4. ablation: sans prétrain auto-supervisé < avec prétrain auto-supervisé,
5. reproductibilité run-to-run (variance contrôlée).

## 14.6 Indicateurs nouveaux à logger (forensic V4)

Ajouter:
- `selfsup_loss_curve.json`
- `material_head_metrics.csv`
- `fiber_orientation_stats.json`
- `ablation_report_v4.json`
- `native_training_manifest.json` (seed, epochs, data coverage, hash config)

---

## 15) Plan d’implémentation V4 (upgrade concret)

## Sprint S1 — Backbone natif auto-supervisé
- [ ] implémenter MVR
- [ ] implémenter CSC
- [ ] implémenter FOP
- [ ] exporter courbes + checkpoints locaux

## Sprint S2 — Tête matériaux
- [ ] brancher tête 4 classes
- [ ] générer overlays matériaux
- [ ] valider confusion matrix

## Sprint S3 — Tête compétition
- [ ] fusion `P_final_ink`
- [ ] threshold_scan + calibration F-beta
- [ ] produire `submission.zip` + validation stricte

## Sprint S4 — Forensic & Bench
- [ ] rapport ablation complet
- [ ] comparaison AGNN pur vs Deep natif vs Hybride natif
- [ ] décision finale V4 validée par métriques

---

## 16) Réponse directe à ta demande finale

Oui, il est possible d’atteindre un **auto-apprentissage profond natif** sans dépendre d’autres modèles concurrents.
La clé n’est pas d’éviter le deep learning, mais d’éviter les poids externes opaques: tout entraîner offline, from-scratch, avec preuves forensic et ablations.
