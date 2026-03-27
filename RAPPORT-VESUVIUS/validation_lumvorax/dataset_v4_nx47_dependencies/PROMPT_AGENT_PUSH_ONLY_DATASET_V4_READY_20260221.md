# PROMPT AGENT — PUSH ONLY dataset NX47 dépendances V4 + update kernel test360

Tu ne dois **PAS** reconstruire le bundle V4. Il est prêt.

## 1) Chemins exacts
- Bundle prêt: `RAPPORT-VESUVIUS/validation_lumvorax/dataset_v4_nx47_dependencies/bundle`
- Vérif locale: `RAPPORT-VESUVIUS/validation_lumvorax/dataset_v4_nx47_dependencies/verify_dataset_v4_bundle.py`
- Script test V3 complet: `RAPPORT-VESUVIUS/validation_lumvorax/notebook_validation_lumvorax_dependances_360_kaggle_single_cell_V3_COMPLETE_20260221.py`
- Dataset cible: `ndarray2000/nx47-dependencies`
- Kernel cible: `ndarray2000/test-360-dep-lumvorax`
- Dataset cible: `ndarray2000/nx47-dependencies`

## 2) Pré-check obligatoire
```bash
kaggle --version
test -f ~/.kaggle/kaggle.json
python3 RAPPORT-VESUVIUS/validation_lumvorax/dataset_v4_nx47_dependencies/verify_dataset_v4_bundle.py \
  --dir RAPPORT-VESUVIUS/validation_lumvorax/dataset_v4_nx47_dependencies/bundle
```

## 3) Push dataset (unique)
## 3) Push (unique action)
```bash
kaggle datasets version \
  -p RAPPORT-VESUVIUS/validation_lumvorax/dataset_v4_nx47_dependencies/bundle \
  -m "V4 ready bundle: wheels+so+competitor_tiff_lum+manifest"
```

## 4) Mise à jour kernel test360 + attente 59s + logs
```bash
mkdir -p /tmp/kaggle_kernel_test360_v4
kaggle kernels pull ndarray2000/test-360-dep-lumvorax -p /tmp/kaggle_kernel_test360_v4
# remplacer le code du kernel par le script V3 complet + logs persistants section 3.2 du prompt principal
kaggle kernels push -p /tmp/kaggle_kernel_test360_v4
sleep 59
kaggle kernels status ndarray2000/test-360-dep-lumvorax
kaggle kernels output ndarray2000/test-360-dep-lumvorax -p /tmp/kaggle_kernel_test360_v4_output
## 4) Post-check
```bash
kaggle datasets files ndarray2000/nx47-dependencies --page-size 200
```

## 5) Rapport obligatoire
Créer:
- `RAPPORT-VESUVIUS/validation_lumvorax/dataset_v4_nx47_dependencies/RAPPORT_AGENT_PUSH_ONLY_V4_EXECUTION.md`

Le rapport doit contenir:
1. horodatage de chaque commande,
2. sortie de `verify_dataset_v4_bundle.py`,
3. résultat du push dataset (version publiée),
4. résultat kernel après 59 secondes,
5. confirmation des logs persistants (JSONL/CSV/latence/hardware/stdout/stderr),
6. incidents/erreurs rencontrés,
7. verdict final GO/NO-GO.
3. résultat du push (version publiée),
4. listing post-push,
5. incidents/erreurs rencontrés,
6. verdict final GO/NO-GO.
