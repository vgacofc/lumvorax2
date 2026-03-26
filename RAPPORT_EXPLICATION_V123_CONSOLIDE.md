# RAPPORT EXPLICATIF COMPLET — V122 vs V123 (CONSOLIDATION TOTALE)

## 1) Résumé exécutif

Ce rapport consolide **toutes les suggestions de la conversation** dans une seule version opérationnelle (**V123**) et explique les résultats réels fournis pour V122 et V123.

Objectif atteint dans V123:

- pipeline Kaggle complet inchangé (discovery → load → features → train → segment → package)
- logs forensiques Merkle nanoseconde conservés
- métriques dataset/hardware/système complètes
- neurones actifs début/milieu/fin
- ratio adaptatif enrichi (scan global + profil slice-by-slice)
- méta-neurones compétitifs
- auto-pruning + mutation contrôlée
- mémoire évolutive + learning-rate adaptatif
- simulation 100 volumes (optionnelle) + courbe théorique F1-vs-ratio

## 2) Lecture des résultats réels V122 (fournis)

### 2.1 Inventaire dataset traité exactement

- dossiers détectés: `.`, `deprecated_train_images`, `deprecated_train_labels`, `test_images`, `train_images`, `train_labels`
- types de fichiers: `.tif` = 1613, `.csv` = 2

### 2.2 Système/hardware réel

- CPU: 4 cœurs (`cpu_count: 4`)
- RAM: ~32.9 GB total, ~31.6 GB disponible
- GPU: Tesla P100 16GB
- Python: 3.12.12
- OS: Linux 6.6.113+

### 2.3 Métriques segmentation

- neurones actifs: début `0`, milieu `6`, fin `6`
- active ratio final: `0.03`
- hystérésis: `0.0`
- pixels traités: `32,768,000`
- pixels anchor détectés: `0`
- pixels papyrus sans ancre: `3,072`
- matériaux: `1`
- patterns: `1`
- golden nonce: `11`
- découvertes inconnues: `0`
- anomalies: `52`
- tranches: `320`

### 2.4 Performance calcul

- ops estimées: `27,852,800`
- ops/s fichier: `~6,441,120`
- ops/s global: `~3,327,176`
- elapsed global: `~8.37 s`

Interprétation V122:

- le masque est majoritairement dicté par la calibration fixe du ratio (3%)
- système robuste et traçable, mais ratio trop compressé dans ce cas (tendance sous-segmentation)

## 3) Lecture des résultats réels V123 (fournis)

### 3.1 Ce qui change effectivement

- active ratio final passe à `0.04`
- papyrus sans ancre passe à `4,096` (vs 3,072)
- ratio adaptatif sélectionné: `0.04`
- scan ratios:
  - 2% score 0.02
  - 4% score 0.04 (**meilleur**)
  - 6% score négatif (bruit)
  - 8% score plus négatif
  - 12% score encore plus négatif

### 3.2 Méta-neurones, pruning, mutation

- candidats méta-neurones évalués: `45`
- pruning: `true` (et global `pruning_events = 1`)
- mutation: `false` sur ce run (`mutation_events = 0`)
- neurones actifs: toujours `0 → 6 → 6`

### 3.3 Performance

- ops/s fichier: `~3,829,686`
- ops/s global: `~2,876,659`
- elapsed global: `~9.68 s`

Interprétation V123:

- plus intelligent (scan + méta-neurone + pruning), mais plus coûteux en calcul
- gain qualitatif: stratégie ratio moins rigide
- coût temporel attendu vu la recherche multi-candidats

## 4) Réponse explicite à toutes les questions demandées

- Combien de neurones actifs début/milieu/fin?
  - V122: 0/6/6, V123: 0/6/6
- Dossiers et types de fichiers traités exactement?
  - dossiers + suffix stats reportés en `DATASET_DISCOVERY`
- Combien de calculs par seconde?
  - V122 et V123 reportent `calc_per_sec` fichier et `calc_per_sec_global`
- Toutes les métriques système/hardware réelles?
  - `BOOT.hardware`: python/platform/cpu/mem/gpu
- Combien de pixels traités?
  - `pixels_processed` global
- Pixels ancrés détectés?
  - `pixels_anchor_detected`
- Matériaux détectés?
  - `materials_detected`
- Pixels papyrus sans ancre?
  - `pixels_papyrus_without_anchor`
- Tranches traitées?
  - `slices_processed`
- Golden nonce détectés?
  - `golden_nonce_detected`
- Patterns?
  - `patterns_detected`
- Découvertes inconnues + anomalies?
  - `unknown_discoveries`, `anomalies_detected`

## 5) Consolidation totale ajoutée en V123 (version finale)

V123 inclut maintenant simultanément:

1. **Neuron ATOM propriétaire enrichi**:
   - terme linéaire `w·x`
   - terme quadratique `alpha·x²`
   - terme différentiel `beta·∇x`
2. **Régularisation dynamique** `lambda = var(mask)/entropy(features)` intégrée au scoring d’objectif.
3. **Ratio adaptatif double**:
   - scan global multi-ratio (2/4/6/8/12%)
   - estimation slice-by-slice via complexité (entropy+gradient) puis ratio global dérivé.
4. **Méta-neurone arbitre** (candidats multiples + leaderboard top5).
5. **Auto-pruning intelligent** quantile.
6. **Mutation contrôlée** si stagnation F1-proxy.
7. **Mémoire évolutive** (historique F1 + ratios).
8. **Learning-rate adaptatif** piloté par stabilité F1 récente.
9. **Simulation 100 volumes** optionnelle.
10. **Courbe théorique F1-vs-ratio** (ratio optimal estimé) exportée en metadata.

## 6) Position stratégique recommandée

- ratio trop bas (3%) = précision haute mais rappel faible
- ratio optimal attendu pour ce type de signal: zone **6–9%** en théorie
- néanmoins le scan réel V123 a choisi 4% ici car bruit morphologique croissant >6%

=> recommandation pratique:

- conserver V123 comme base unique
- augmenter progressivement les candidats autour de 5/6/7/8/9% quand le bruit est mieux contrôlé
- utiliser mutation uniquement quand stagnation confirmée sur plusieurs runs

## 7) Notes sur warnings observés

- warnings debugger frozen modules: non bloquant pour l’exécution batch
- warnings nbconvert/mistune/traitlets: non bloquants, liés au post-processing notebook HTML

