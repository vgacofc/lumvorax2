# Rapport production effective locale — NX47 dépendances V4 (bundle réellement produit)

Date: 2026-02-21

## Résumé
La production locale V4 a été exécutée et le bundle complet est généré dans:
- `RAPPORT-VESUVIUS/validation_lumvorax/dataset_v4_nx47_dependencies/bundle`

Le vérificateur V4 retourne `status: ok` localement.

## Commandes exécutées
1. `python3 -m pip install -q numpy tifffile`
2. `python3 RAPPORT-VESUVIUS/validation_lumvorax/dataset_v4_nx47_dependencies/produce_wheels_v4.py`
3. `python3 RAPPORT-VESUVIUS/validation_lumvorax/dataset_v4_nx47_dependencies/build_dataset_v4_bundle.py --repo-root . --output-dir RAPPORT-VESUVIUS/validation_lumvorax/dataset_v4_nx47_dependencies/bundle --wheel-source RAPPORT-VESUVIUS/validation_lumvorax/dataset_v4_nx47_dependencies/wheelhouse_v4`
4. `python3 RAPPORT-VESUVIUS/validation_lumvorax/dataset_v4_nx47_dependencies/verify_dataset_v4_bundle.py --dir RAPPORT-VESUVIUS/validation_lumvorax/dataset_v4_nx47_dependencies/bundle`

## Résultat vérification
- `missing: []`
- `status: ok`
- `compile_ok: true`
- assets concurrent présents (`.tif` + `.lum`)

## Fichiers produits dans le bundle (15 artefacts + manifest)
- `competitor_teacher_1407735.tif` — sha256 `4135090f39acbf3f0be3af3999af1a8d7685e43e471cf1d13007fda2282f9a3d`
- `competitor_teacher_1407735.lum` — sha256 `caa16fb6cf8889b0867ec4afbd3495f19f3a8be8c0a776adf171fcf12fc1214d`
- `liblumvorax_replit.so` — sha256 `585b2e14c0ba9622f1a05d2a0be1a4aa5b1fb4426691f7aa4972fb801f1f22f0`
- wheels:
  - `imagecodecs-2026.1.14-...whl`
  - `imageio-2.37.2-...whl`
  - `lazy_loader-0.4-...whl`
  - `networkx-3.6.1-...whl`
  - `numpy-2.4.2-...whl`
  - `opencv_python-4.13.0.92-...whl`
  - `packaging-26.0-...whl`
  - `pillow-12.1.1-...whl`
  - `scikit_image-0.26.0-...whl`
  - `scipy-1.17.0-...whl`
  - `tifffile-2026.1.28-...whl`
  - `tifffile-2026.2.16-...whl`
- `dataset_v4_build_report.json`

## Notes importantes
- Le bundle est prêt pour un agent **push-only** vers Kaggle.
- Le nouveau prompt push-only à utiliser est:
  - `RAPPORT-VESUVIUS/validation_lumvorax/dataset_v4_nx47_dependencies/PROMPT_AGENT_PUSH_ONLY_DATASET_V4_READY_20260221.md`
