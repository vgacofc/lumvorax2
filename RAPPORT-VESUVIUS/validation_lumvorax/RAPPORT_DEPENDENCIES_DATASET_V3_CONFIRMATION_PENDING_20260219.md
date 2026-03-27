# RAPPORT — Dataset dépendances V3 publié, statut en attente de confirmation runtime

Date: 2026-02-19  
Scope: `ndarray2000/nx47-dependencies`  
Statut global: **EN ATTENTE DE CONFIRMATION FONCTIONNELLE EN EXÉCUTION KAGGLE**

---

## 1) Contexte

La V3 a été annoncée comme union complète de V1 + V2, avec restauration des dépendances historiques critiques (`imagecodecs`, `tifffile-2026.1.28`) et maintien des artefacts V2 (dont `liblumvorax_replit.so`).

Ce rapport sert de trace **append-only** et de jalon de validation avant passage en statut « confirmé en production Kaggle ».

---

## 2) Inventaire attendu (union V1 + V2)

### Héritage V1 (doit être présent)
- `imagecodecs-2026.1.14-cp311-abi3-manylinux_2_27_x86_64.manylinux_2_28_x86_64.whl`
- `numpy-2.4.2-cp311-cp311-manylinux_2_27_x86_64.manylinux_2_28_x86_64.whl`
- `tifffile-2026.1.28-py3-none-any.whl`

### Héritage V2 (doit rester présent)
- `imageio-2.37.2-py3-none-any.whl`
- `lazy_loader-0.4-py3-none-any.whl`
- `liblumvorax_replit.so`
- `networkx-3.6.1-py3-none-any.whl`
- `numpy-2.4.2-cp311-cp311-manylinux_2_27_x86_64.manylinux_2_28_x86_64.whl`
- `opencv_python-4.13.0.92-cp37-abi3-manylinux_2_28_x86_64.whl`
- `packaging-26.0-py3-none-any.whl`
- `pillow-12.1.1-cp311-cp311-manylinux_2_27_x86_64.manylinux_2_28_x86_64.whl`
- `scikit_image-0.26.0-cp311-cp311-manylinux_2_24_x86_64.manylinux_2_28_x86_64.whl`
- `scipy-1.17.0-cp311-cp311-manylinux_2_27_x86_64.manylinux_2_28_x86_64.whl`
- `tifffile-2026.2.16-py3-none-any.whl`

### Point de compatibilité volontaire
- Les deux versions de tifffile (`2026.1.28` et `2026.2.16`) sont conservées pour éviter toute régression inter-kernels.

---

## 3) Impact attendu sur les kernels travaillés

### Kernels NX47 V2.x
- `nx47_vesu_kernel_v2.py`
- `nx47_vesu_kernel_v2.1.py`
- `nx47_vesu_kernel_v2.2.py`
- `nx47_vesu_kernel_v2.3.py`
- `nx47_vesu_kernel_v2.4.py`

Attendu:
1. bootstrap offline stable sur les roots standard Kaggle (`/kaggle/input/datasets/ndarray2000/nx47-dependencies`, fallback `/kaggle/input/nx47-dependencies`),
2. plus de blocage sur absence historique de `imagecodecs-2026.1.14` dans le dataset,
3. compatibilité maintenue pour les chemins natifs (`liblumvorax_replit.so`) quand présents.

---

## 4) Validation encore requise (avant confirmation définitive)

Le dataset est annoncé conforme, mais la confirmation finale doit inclure:

1. **Exécution Kaggle réelle** de V2.4 (ou version cible de prod),
2. **Absence de crash TIFF LZW** à la lecture,
3. **Génération artifacts** (`submission.parquet`, `submission.zip`, metadata roadmap),
4. **Comparaison score/qualité** avec runs de référence qui ont scoré.

Tant que ces 4 points ne sont pas validés dans un run réel, ce jalon reste:  
**EN ATTENTE DE CONFIRMATION FONCTIONNELLE**.

---

## 5) Décision provisoire

- Dataset V3: **VALIDÉ EN INVENTAIRE** (liste de fichiers).
- Runtime Kaggle multi-kernels: **PENDING**.
- Recommandation: lancer campagne de confirmation croisée (cf. plan dédié `PLAN_CONFIRMATION_RUNTIME_MULTI_VERSIONS_V3_20260219.md`).
