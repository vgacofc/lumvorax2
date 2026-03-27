# Plan d’intégration SHF/RSR dans `vesuvius-0-552.ipynb`

## 1) Objectif opérationnel
- Conserver le pipeline concurrent (TransUNet + SWI + TTA + hysteresis topologique).
- Ajouter une surcouche LUM/VORAX SHF/RSR non destructive pour augmenter la robustesse des masques binaires, sans casser le score de base 0.55.

## 2) Inventaire forensique des sources (avec versions réelles)

### A. Base concurrente à améliorer
1. **Notebook cible**: `RAPPORT-VESUVIUS/exemple-soumision-notebook-concurrent/vesuvius-0-552.ipynb`
   - Pipeline de base avec `TransUNet`, `SlidingWindowInference`, TTA, puis `seeded_hysteresis_with_topology`.
   - Le score de référence est explicité dans les commentaires de tête (`public 0.55` / `private 0.55`).

### B. Source conceptuelle SHF/RSR (document de cadrage)
2. **Document**: `comparaison_shf_rsr_aimo3_20260121.md`
   - SHF = observation passive d’une signature spectrale.
   - RSR = interférence active/récursive pour transformer la mesure en gradient exploitable.
   - Pivot officiel vers une logique orientée performance compétition.

### C. Code source réel de la techno AIMO3/SHF-RSR
3. **Version compacte V12 proto**: `aimo3_shf_resonance_v3_v12.py`
   - Présence d’un noyau `lum_vorax_optimization_v12` (sommation Kahan) et d’un test RSA observable.

4. **Version kernel V16 (Kaggle)**: `kaggle_kernel/aimo3-shf-resonance-v3.py`
   - Fonctions SHF/RSR explicites: `shf_resonance_check`, `goldbach_verify`, `rsa_spectral_jitter`, `solve_enhanced`.
   - Instrumentation forensic (`ForensicLogger`) et export de métriques.

5. **Version V16 enrichie sync**: `src/kaggle_sync/aimo3-shf-resonance-v3.py`
   - Même socle V16 avec parsing plus robuste (nettoyage LaTeX, extraction numérique float/entier, fallback pattern).

6. **Copies opérationnelles de release (identiques par hash)**:
   - `kaggle_kernel_v12/aimo3-shf-resonance-v3.py`
   - `temp_meta/aimo3-shf-resonance-v3.py`
   - `final_verification/aimo3-shf-resonance-v3.py`
   - `RAPPORT-VESUVIUS/validation_lumvorax/dataset_v4_nx47_dependencies/bundle/src/kaggle_sync/aimo3-shf-resonance-v3.py`

### D. Preuve d’intégration antérieure côté Vesuvius
7. **Notebook NX46-VX**: `RAPPORT-VESUVIUS/NX46-VX/nx46-vx-unified-kaggle-V1.ipynb`
   - Import dynamique AIMO3 depuis dataset Kaggle.
   - Liaison sur `solve_enhanced` démontrant l’utilisation pratique de la techno dans la stack Vesuvius élargie.

## 3) Cartographie “Avant / Après” sur le notebook concurrent

## Avant
- Probabilité privée utilisée directement dans la phase d’hysteresis.
- Aucune modulation spectrale explicite SHF.
- Aucune propagation récursive de prior (RSR) entre ancre publique et proba privée.
- Logs limités au temps et nombre de positifs.

## Après
- **Ajout config SHF/RSR** dans `CFG`:
  - `USE_LUM_RESONANCE`, `LUM_BLEND_ALPHA`, `LUM_SPECTRAL_GAMMA`, `LUM_PRIOR_WEIGHT`, `LUM_SMOOTH_STEPS`, `LUM_PRIOR_THRESHOLD`.
- **Ajout helper SHF**: `shf_spectral_signature(prob)` via FFT + ratio percentile haut/bas.
- **Ajout helper RSR**: `rsr_recursive_prior(pub_fg_bool, smooth_steps)` via diffusion gaussienne récursive.
- **Ajout fusion LUM**: `lum_resonance_blend(prob, pub_fg_bool, cfg)` qui:
  1. estime la signature spectrale SHF,
  2. construit un prior récursif RSR,
  3. produit `prob_for_output` (probabilité boostée contrôlée).
- **Intégration pipeline**:
  - `prob_for_output` remplace `prob` dans `seeded_hysteresis_with_topology` quand `USE_LUM_RESONANCE=True`.
- **Forensic run summary**:
  - collecte par volume (`lum_spectral_score`, `lum_spectral_gain`, `lum_prior_mean`, `lum_prior_gate_rate`, `minutes`) et impression finale tabulaire.

## 4) Cahier des charges d’exécution (A → Z)
1. **Sync repo distant**: fetch + vérification HEAD aligné main.
2. **Audit source**:
   - lire le notebook concurrent 0.552,
   - lire le doc `comparaison_shf_rsr_aimo3_20260121.md`,
   - inventorier toutes les versions réelles `aimo3-shf-resonance-v3`.
3. **Implémentation incrémentale**:
   - injecter config LUM dans `LUM_CFG` (séparée de `CFG`),
   - ajouter blocs SHF/RSR helpers,
   - brancher fusion avant hysteresis,
   - ajouter logs de synthèse de run.
4. **Validation statique locale**:
   - contrôle JSON notebook,
   - diff ciblé sur la cellule unique.
5. **Roadmap expérimentale score > 55**:
   - Phase R1: benchmark baseline vs LUM (blend activé/désactivé).
   - Phase R2: sweep `{LUM_BLEND_ALPHA, LUM_SPECTRAL_GAMMA, LUM_PRIOR_THRESHOLD}`.
   - Phase R3: tuning topologie conjointe `{T_low, T_high, z_radius, xy_radius}`.
   - Phase R4: ablation (`SHF only`, `RSR only`, `SHF+RSR`) pour isoler le gain réel.
   - Phase R5: verrouillage d’une config robuste (objectif: dépasser 0.55 sans collapse private).

## 5) Review anti-oubli
- [x] Source concurrente lue.
- [x] Fichier de comparaison SHF/RSR lu.
- [x] Versions réelles de la techno identifiées.
- [x] Intégration appliquée dans notebook demandé.
- [x] Journalisation ajoutée pour analyse comparative.
- [x] Plan de tests/roadmap fourni.

## 6) Risques & garde-fous
- Risque sur-surrection du bruit si `LUM_BLEND_ALPHA` trop élevé.
- Risque de faux positifs si `LUM_PRIOR_THRESHOLD` trop bas.
- Garde-fou: feature flag `USE_LUM_RESONANCE` pour fallback instantané baseline.


## 7) Correctif suite review (configuration concurrente préservée)
- Les paramètres d’origine du concurrent (`CFG`) restent inchangés pour le chemin baseline.
- La surcouche SHF/RSR est déplacée dans `LUM_CFG` séparé pour éviter toute dérive involontaire du baseline.
- Garde-fous appliqués:
  - `USE_LUM_RESONANCE=True` avec paramètres LUM synchronisés sur `CFG` concurrent,
  - `LUM_BLEND_ALPHA = CFG["overlap_base"]` (même référentiel que le concurrent),
  - `LUM_PRIOR_THRESHOLD = CFG["T_low"]` (seuil aligné au concurrent).


## 8) Correctif robustesse exécution Kaggle (papermill/protobuf)
- Symptôme analysé: échec papermill tardif `MessageFactory.GetPrototype` malgré génération de `submission.zip`.
- Action: ajout d'un shim de compatibilité protobuf en préambule d'exécution (`GetPrototype` -> `GetMessageClass` si absent).
- Impact: pas de changement sur le coeur concurrent (TransUNet/SWI/TTA/hysteresis), correction purement de compatibilité runtime.
