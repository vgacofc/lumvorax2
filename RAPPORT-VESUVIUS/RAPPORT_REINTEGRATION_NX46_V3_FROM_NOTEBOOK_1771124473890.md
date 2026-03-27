# RAPPORT DE RÉINTÉGRATION NX46 V3
## Source principale réintégrée: `nx46-vesuvius-challenge-surface-detection_1771124473890.ipynb`

## 1. Objectif

Ce rapport documente exactement ce qui a été repris des versions de référence (notebook validé), pourquoi c'était nécessaire, et comment cela a été réintégré dans `nx46-vesuvius-core-kaggle-ready-v3.py`.

## 2. Problème constaté avant V3

Les runs précédents pouvaient finir avec:
- `layout_detected = empty`
- `total_pixels_processed = 0`
- pas de `submission.zip`

Cause principale: divergence entre la détection d'entrées du core compact et les patterns réels Kaggle déjà validés dans le notebook de référence.

## 3. Éléments réintégrés depuis le notebook 1771124473890

### 3.1 Bootstrap offline des dépendances (copie de méthode validée)

Réintégration de la stratégie déjà utilisée dans le notebook:
- `install_offline(package_name)`
- priorité au path: `/kaggle/input/datasets/ndarray2000/nx47-dependencies`
- fallback: `/kaggle/input/nx47-dependencies`
- bootstrap ordonné: numpy, imagecodecs, tifffile.

Bénéfice:
- cohérence avec les exécutions Kaggle offline déjà validées,
- suppression des écarts de comportement liés à l'environnement.

### 3.2 Lecture TIFF LZW robuste (même logique de sécurité)

Réintégration de:
- `ensure_imagecodecs()`
- `read_tiff_lzw_safe()` avec retry après installation codec,
- fallback Pillow si nécessaire.

Bénéfice:
- évite l'échec classique "requires imagecodecs" dans les volumes TIFF compression LZW.

### 3.3 Détection dataset alignée sur les versions qui marchent

Réintégration des patterns de découverte réels:
1. `competition_test_images` (`test_images/*.tif`) — prioritaire
2. `fragment_dirs` (`train/<id>/surface_volume` et `test/<id>/surface_volume`)
3. `legacy_tif_files` (`train_images/*.tif` + `test_images/*.tif`)

+ auto-discovery multi-roots Kaggle (incluant `competitions/vesuvius-challenge-surface-detection`).

Bénéfice:
- évite les runs "empty" quand la racine par défaut ne correspond pas au dataset monté.

### 3.4 Inventaire complet des fichiers dataset (méthode d’audit)

Ajout d’un inventaire automatique:
- nombre total d’assets,
- stats par extension,
- liste des dossiers détectés.

Sortie dédiée:
- `dataset_discovery_inventory.json`

Bénéfice:
- preuve forensic immédiate de ce qui est réellement visible par le kernel.

### 3.5 Sortie compétition stricte

Réintégration d’un contrat strict:
- génération `submission.zip` avec `.tif` LZW,
- validation stricte des membres zip contre les fichiers test attendus,
- fail-fast si mismatch en mode strict.

+ compatibilité `submission.csv` quand `sample_submission.csv` existe.

### 3.6 Logs de preuve d’exécution

Réintégration des signaux exploitables:
- `EXEC_COMPLETE`
- affichage final `READY: ...`
- logs forensics multi-fichiers
- exports `RkF4XakI.txt` et `UJxLRsEE.txt`.

## 4. Pourquoi ces corrections règlent le "tout à zéro"

Parce que V3:
1. n'utilise plus un seul schéma de dossiers,
2. cherche explicitement les racines Kaggle réellement utilisées,
3. échoue clairement si aucune entrée test réelle n’est trouvée,
4. valide la sortie finale compétition au lieu de conclure silencieusement.

Donc on passe d’un pipeline permissif silencieux à un pipeline strict, traçable et compatible compétition.

## 5. Livrables V3

- Script: `RAPPORT-VESUVIUS/src_vesuvius/nx46-vesuvius-core-kaggle-ready-v3.py`
- Rapport: `RAPPORT-VESUVIUS/RAPPORT_REINTEGRATION_NX46_V3_FROM_NOTEBOOK_1771124473890.md`

## 6. Recommandation d’exécution Kaggle

1. Monter le dataset compétition Vesuvius.
2. Lancer le script V3.
3. Vérifier dans `work_root/logs`:
   - `dataset_discovery_inventory.json`
   - `forensic_ultra.log`
   - `state.json`
4. Vérifier présence de:
   - `submission.zip`
   - `READY: ...` dans la sortie standard.

## 7. Conclusion

La V3 remet le pipeline sur la méthode des versions de référence (celles qui fonctionnaient déjà), au lieu de réinventer un comportement partiellement incompatible.
