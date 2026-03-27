# Rapport d'exécution détaillé — `vesuvius-0-552` (version avec overlay LUM/SHF-RSR)

## 1. Objectif du pipeline

### 1.1 But principal
1. Générer un `submission.zip` au format attendu Vesuvius.
2. Conserver le chemin concurrent d'origine (TransUNet + SWI + TTA + hysteresis topologique).
3. Ajouter une surcouche LUM/SHF-RSR activée et synchronisée sur les paramètres concurrent (`USE_LUM_RESONANCE=True`).

### 1.2 Entrées / sorties
- **Entrées externes**:
  - Dataset compétition: `/kaggle/input/vesuvius-challenge-surface-detection`
  - Modèle: `/kaggle/input/vsd-model/keras/...weights.h5`
  - Wheels offline: `/kaggle/input/vsdetection-packages-offline-installer-only/whls`
- **Sorties externes**:
  - TIFFs temporaires dans `/kaggle/working/submission_masks`
  - ZIP final: `/kaggle/working/submission.zip`

---

## 2. Vue globale par phases (A → Z)

## Phase A — Préparation runtime
### A.1 Variables d'environnement
- Active `PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION=python`.
- Force `KERAS_BACKEND=jax`.
- Limite threads CPU (`OMP/MKL/OPENBLAS/NUMEXPR`) à 4.

### A.2 Installation paquets offline
- En notebook: `!pip install ...` via wheels locaux.
- En script `.py`: installation équivalente via `subprocess.check_call` + `Path.glob`.

### A.3 Chargement librairies
- `numpy`, `pandas`, `tifffile`, `scipy.ndimage`, `keras`, `medicai`.

---

## Phase B — Configuration

### B.1 Configuration concurrente d'origine (`CFG`)
- Modèle/poids inchangés.
- Overlaps inchangés (`0.42`, `0.43`, `0.60`).
- Seuils et topologie inchangés (`T_low=0.50`, `T_high=0.90`, `z=3`, `xy=2`, `dust=100`).

### B.2 Configuration overlay (`LUM_CFG`)
- Séparée du baseline pour éviter toute régression involontaire.
- `USE_LUM_RESONANCE=True` (actif).
- Synchronisation directe avec `CFG` concurrent:
  - `LUM_BLEND_ALPHA = CFG["overlap_base"]`
  - `LUM_SPECTRAL_GAMMA = CFG["overlap_hi"]`
  - `LUM_PRIOR_WEIGHT = CFG["overlap_public"]`
  - `LUM_SMOOTH_STEPS = CFG["z_radius"]`
  - `LUM_PRIOR_THRESHOLD = CFG["T_low"]`

---

## Phase C — Prétraitement des volumes

### C.1 Lecture volume
- `tifffile.imread(path)` → `float32`.
- Reshape en `(1, D, H, W, 1)`.

### C.2 Normalisation
- `NormalizeIntensity` via pipeline `Compose`.

---

## Phase D — Modèle et inférence glissante

### D.1 Construction modèle
- `TransUNet(input_shape=(160,160,160,1), encoder='seresnext50', num_classes=3, classifier_activation=None)`.
- Chargement des poids du concurrent.

### D.2 SlidingWindowInference
- Trois instances:
  1. `swi_public` (overlap 0.42)
  2. `swi_base` (0.43)
  3. `swi_hi` (0.60)

### D.3 Warmup
- Au premier volume, compile JAX (`swi_public/base/hi`) pour stabiliser le temps ensuite.

---

## Phase E — Prédiction (interne)

### E.1 TTA
- Identité.
- Flips axes 1/2/3.
- Rotations 90° sur plan XY (`k=1,2,3`).

### E.2 Flux public
- Moyenne des logits multiclasses TTA.
- `argmax` → labels public (`pub_labels`).

### E.3 Flux privé
- Si `OV06_MAIN_ONLY=True`:
  - passe principale avec `swi_hi`
  - autres passes avec `swi_base`
- Conversion logits multiclasses → logit binaire (`INK_MODE=fg12`), puis sigmoïde.

---

## Phase F — Post-traitement (coeur score)

### F.1 Avant (baseline historique)
- `prob` privé entre directement dans `seeded_hysteresis_with_topology`.
- `weak = (prob >= T_low) OR pub_fg`.
- propagation binaire, puis closing anisotrope, puis suppression poussière.

### F.2 Après (version actuelle)
1. `prob_for_output = prob` par défaut.
2. Si `LUM_CFG.USE_LUM_RESONANCE=True`, alors:
   - SHF: `shf_spectral_signature(prob)` (ratio percentiles FFT).
   - RSR: `rsr_recursive_prior(pub_fg)` (lissage récursif).
   - Blend: `lum_resonance_blend` produit `prob_for_output`.
3. `seeded_hysteresis_with_topology(prob_for_output, ...)`.

### F.3 Ce qui est activé à quel moment
- **Toujours actif**: pipeline concurrent baseline (A→E + hystérésis).
- **Actif par défaut**: overlay SHF/RSR exécuté à chaque volume (synchronisé `CFG` + `LUM_CFG`).

---

## Phase G — Export et instrumentation

### G.1 Export
- Écrit TIFF binaire par volume.
- Ajoute au ZIP final puis supprime TIFF temporaire.

### G.2 Logs runtime
- Ligne de progression par volume (temps/positifs).
- Collecte `run_rows`:
  - `lum_spectral_score`
  - `lum_spectral_gain`
  - `lum_prior_mean`
  - `lum_prior_gate_rate`
- Affiche résumé tabulaire final.

---

## 3. Différences avant / après (point par point)

### 3.1 Invariants (inchangés)
1. Architecture modèle.
2. Poids concurrent.
3. Overlaps SWI.
4. TTA.
5. Seuils et topologie morphologique.

### 3.2 Nouveautés
1. Bloc `LUM_CFG` séparé.
2. Fonctions SHF/RSR + blend.
3. Gating strict par feature-flag.
4. Rapport de métriques LUM par volume.

### 3.3 Sécurité de rollback
- Option de rollback toujours possible: passer `USE_LUM_RESONANCE=False` si nécessaire.

---

## 4. Lecture interne/externe “couche par couche”

### 4.1 Couche externe (I/O et environnement)
- Inputs Kaggle, installation wheels, chemins, ZIP final.

### 4.2 Couche modèle (inférence)
- TransUNet + SWI + TTA.

### 4.3 Couche logique binaire
- Conversion logits→probabilité en mode `fg12`.

### 4.4 Couche topologique
- Hysteresis seed/grow + closing anisotrope + dust removal.

### 4.5 Couche expérimentale SHF/RSR
- Analyse spectrale (SHF), prior récursif (RSR), blend contrôlé.
- Désactivée par défaut.

---

## 5. Fiche opérationnelle rapide

1. Exécuter baseline pur (recommandé pour référence):
   - `LUM_CFG['USE_LUM_RESONANCE']=False`
2. Activer overlay pour test:
   - `LUM_CFG['USE_LUM_RESONANCE']=True`
3. Contrôler risques:
   - ne pas augmenter brutalement `LUM_BLEND_ALPHA`
   - ne pas descendre trop bas `LUM_PRIOR_THRESHOLD`
4. Comparer sorties:
   - positifs
   - stabilité du temps
   - qualité visuelle et score public/private


## 6. Analyse incident run `f26d45` / `vesuvius-0-552.log`

### 6.1 Constat observé
- Le pipeline a produit la sortie métier (`submission.zip`) et la ligne finale de progression.
- Le run a ensuite été marqué en échec par papermill avec:
  - `AttributeError: 'MessageFactory' object has no attribute 'GetPrototype'`.

### 6.2 Cause technique
- Incompatibilité de version `protobuf` (API `MessageFactory.GetPrototype` supprimée dans versions récentes) avec une dépendance chargée dans l'environnement Kaggle.

### 6.3 Correctif appliqué (sans casser le pipeline existant)
1. Ajout d'un shim de compatibilité protobuf en tout début d'exécution (notebook + script `.py`).
2. Si `GetPrototype` est absent, création d'un alias vers `GetMessageClass`.
3. Aucune modification des paramètres de modèle/inférence/post-traitement concurrent.
4. Nettoyage des outputs d'erreur persistés dans le notebook pour repartir d'un état propre.

### 6.4 Effet attendu
- Éviter l'échec tardif papermill tout en conservant le comportement de segmentation déjà validé.
