# Rapport avant / après — passage en Option C Hybride (NX-47 autonome v115)

Ce rapport reprend la revue complète et présente **avant/après chaque modification réalisée** pour passer de l’approche v114 à l’architecture demandée **NX-47 ATOM autonome**, sans UNet externe.

## 1) Nature du moteur de prédiction

### Avant
- Pipeline v114 orienté heuristiques / fusion statistique.
- Pas de neurone explicite appris avec sélection L1.

### Après
- Nouveau neurone autonome `NX47AtomNeuron`:
  \[
  P(ink)=\sigma\left(\sum_i w_i f_i + b\right)
  \]
- Entraînement par descente proximale avec **L1 + L2**.

**Modif implémentée**: `NX47AtomNeuron.fit_prox` + `predict_proba`.

---

## 2) Dépendance au deep learning externe

### Avant
- Approches précédentes comparées à des pipelines concurrents deep (UNet/TransUNet), non autonomes.

### Après
- Version **100% autonome**, sans modèle externe, sans sklearn requis.
- Toute la logique d’apprentissage est dans le fichier v115.

**Modif implémentée**: fichier standalone `nx47_vesu_kernel_v115_atom.py`.

---

## 3) Extraction de features

### Avant
- Features plus limitées et moins alignées avec ton cahier (Sobel, stats plus simples).

### Après
- Extraction multi-features enrichie:
  - projection moyenne 3D,
  - projection max 3D,
  - gradient magnitude,
  - laplacien,
  - variance locale,
  - entropie locale (approx),
  - cohérence inter-slices,
  - réponse bande-passante.
- Normalisation z-score feature-wise.

**Modif implémentée**: `extract_multi_features`.

---

## 4) Sélection automatique des features

### Avant
- Sélection moins avancée, sans réduction explicite de redondance corrélée.

### Après
- Sélection automatique:
  1. score par variance,
  2. tri descendant,
  3. suppression de redondance corrélation élevée,
  4. conservation top-k effectif.

**Modif implémentée**: `auto_select_features`.

---

## 5) Pseudo-labeling

### Avant
- Heuristiques de seuil directes sans boucle d’apprentissage pleinement autonome.

### Après
- Auto-consistance de labels:
  - top percentile = positifs,
  - bas percentile = négatifs,
  - apprentissage sur l’union des zones informatives.

**Modif implémentée**: `pseudo_labels` + usage dans `train_nx47_autonomous`.

---

## 6) L1 + régularisation automatique avancée

### Avant
- Pas de vraie recherche automatique L1/L2 sur objectif validation + sparsité.

### Après
- Grille auto L1/L2.
- Critère de sélection combiné:
  - cross-entropy validation,
  - bonus sparsité (poids nuls),
  - choix automatique du meilleur couple.

**Modif implémentée**: `train_nx47_autonomous` (sélection hyperparamètres).

---

## 7) Hysteresis topologique 3D (Option C)

### Avant
- Seuils et post-traitements moins directement alignés avec le schéma strong/weak demandé.

### Après
- Hysteresis explicite:
  - `strong = prob >= strong_th`
  - `weak = prob >= weak_th`
  - `binary_propagation(strong, mask=weak)`
- Fermeture topologique 3D + suppression petites composantes.

**Modif implémentée**: `hysteresis_topology_3d`.

---

## 8) Calibration automatique de densité cible

### Avant
- Calibration moins directement découplée de l’étape hysteresis.

### Après
- Calibration percentile selon ratio cible:
  - `threshold = percentile(prob, 100*(1-target_ratio))`
  - `mask = prob >= threshold`
- Fusion finale: `hysteresis OR calibrated`.

**Modif implémentée**: `calibrate_target_ratio` + fusion dans `_predict_mask`.

---

## 9) Observabilité complète (logs/mémoire/plan)

### Avant
- Observabilité déjà présente, mais non orientée explicitement “neurone atomique + sélection”.

### Après
- Logs enrichis:
  - features sélectionnées,
  - variances de toutes features,
  - hyperparamètres L1/L2 retenus,
  - sparsité, CE validation,
  - ratios actifs finaux.
- Memory tracker sur volume/features/probabilité/masques.

**Modif implémentée**: `NX47_METRICS` + `MemoryTracker` sur étapes clés.

---

## 10) Pipeline final et sortie compétition

### Avant
- Pipeline v114.

### Après
- Pipeline v115 complet:
  `volume -> multi-features -> auto-select -> train NX-47 -> prob -> hysteresis -> calibration -> submission.zip`
- Compatible structure Kaggle `test_images` + TIFF LZW.

**Modif implémentée**: `NX47V115AtomKernel.run`.

---

## Conclusion synthétique

- **Avant**: approche v114 solide mais pas encore un neurone autonome L1/L2 complet selon ton Option C.
- **Après**: **NX-47 v115 autonome** avec apprentissage atomique, sélection de features avancée, régularisation L1/L2 automatique, hysteresis topologique 3D et calibration densité cible.
