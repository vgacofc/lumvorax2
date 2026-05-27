# PROMPT AGENT — Correction échecs V3/V4 + push dataset + push kernel sans erreur d’éditeur

Tu dois corriger les échecs observés, valider, puis pousser proprement.

## 0) Chemins exacts
- Wheels script: `RAPPORT-VESUVIUS/validation_lumvorax/dataset_v4_nx47_dependencies/produce_wheels_v4.py`
- Build script: `RAPPORT-VESUVIUS/validation_lumvorax/dataset_v4_nx47_dependencies/build_dataset_v4_bundle.py`
- Verify script: `RAPPORT-VESUVIUS/validation_lumvorax/dataset_v4_nx47_dependencies/verify_dataset_v4_bundle.py`
- Script test V3: `RAPPORT-VESUVIUS/validation_lumvorax/notebook_validation_lumvorax_dependances_360_kaggle_single_cell_V3_COMPLETE_20260221.py`
- Notebook test V4 (conversion ipynb pour push Kaggle): `RAPPORT-VESUVIUS/validation_lumvorax/notebook_validation_lumvorax_dependances_360_kaggle_single_cell_V4_COMPLETE_20260221.ipynb`
- Bundle output: `RAPPORT-VESUVIUS/validation_lumvorax/dataset_v4_nx47_dependencies/bundle`
- Wheelhouse: `RAPPORT-VESUVIUS/validation_lumvorax/dataset_v4_nx47_dependencies/wheelhouse_v4`
- Dataset cible: `ndarray2000/nx47-dependencies`
- Kernel cible: `ndarray2000/test-360-dep-lumvorax`

## 1) Pré-checks
```bash
kaggle --version
test -f ~/.kaggle/kaggle.json
```

## 2) Rebuild V4 propre (sans omission)
```bash
python3 -m pip install -q numpy tifffile
python3 RAPPORT-VESUVIUS/validation_lumvorax/dataset_v4_nx47_dependencies/produce_wheels_v4.py
python3 RAPPORT-VESUVIUS/validation_lumvorax/dataset_v4_nx47_dependencies/build_dataset_v4_bundle.py \
  --repo-root . \
  --output-dir RAPPORT-VESUVIUS/validation_lumvorax/dataset_v4_nx47_dependencies/bundle \
  --wheel-source RAPPORT-VESUVIUS/validation_lumvorax/dataset_v4_nx47_dependencies/wheelhouse_v4
python3 RAPPORT-VESUVIUS/validation_lumvorax/dataset_v4_nx47_dependencies/verify_dataset_v4_bundle.py \
  --dir RAPPORT-VESUVIUS/validation_lumvorax/dataset_v4_nx47_dependencies/bundle
```

Critère GO build:
- `status: ok`
- `missing: []`

## 3) Push dataset V4
```bash
kaggle datasets version \
  -p RAPPORT-VESUVIUS/validation_lumvorax/dataset_v4_nx47_dependencies/bundle \
  -m "V4 corrected bundle: wheels+so+competitor_tiff_lum+manifest"
kaggle datasets files ndarray2000/nx47-dependencies --page-size 200
```

## 4) Push kernel sans erreur type éditeur

### 4.1 Pull kernel
```bash
mkdir -p /tmp/kaggle_kernel_test360_v4_fix
kaggle kernels pull ndarray2000/test-360-dep-lumvorax -p /tmp/kaggle_kernel_test360_v4_fix
```

### 4.2 Règle critique
- **Ne jamais changer le type éditeur**.
- Si kernel est `.ipynb`, modifier `/tmp/kaggle_kernel_test360_v4_fix/test-360-dep-lumvorax.ipynb` en gardant le format notebook.
- Si kernel est `.py`, modifier le `.py` correspondant.
- Vérifier `kernel-metadata.json` dans `/tmp/kaggle_kernel_test360_v4_fix/` avant push pour confirmer `code_file` et type éditeur.

### 4.2.1 Copie exacte recommandée (cas notebook)
```bash
cp RAPPORT-VESUVIUS/validation_lumvorax/notebook_validation_lumvorax_dependances_360_kaggle_single_cell_V4_COMPLETE_20260221.ipynb \
  /tmp/kaggle_kernel_test360_v4_fix/test-360-dep-lumvorax.ipynb
```

### 4.3 Paramètres runtime recommandés
- Pour test sans sources natives montées:
  - `LUMVORAX_REQUIRE_NATIVE=0`
- Pour test strict natif:
  - monter dataset sources C/C++,
  - `LUMVORAX_REQUIRE_NATIVE=1`

### 4.4 Push + attente logs
```bash
kaggle kernels push -p /tmp/kaggle_kernel_test360_v4_fix
sleep 59
kaggle kernels status ndarray2000/test-360-dep-lumvorax
kaggle kernels output ndarray2000/test-360-dep-lumvorax -p /tmp/kaggle_kernel_test360_v4_fix_output
```

## 5) Vérifications post-run
1. `lumvorax_360_validation_report_v3_complete.json` présent.
2. `status: ok` dans le JSON.
3. assets concurrent détectés (`competitor_teacher_1407735.tif` + `.lum`).
4. logs persistants présents (jsonl/csv/latency/hardware/stdout/stderr/checksums).
5. aucun `OFFLINE_DEPENDENCY_MISSING`.

## 6) Rapport final obligatoire
Créer:
- `RAPPORT-VESUVIUS/validation_lumvorax/dataset_v4_nx47_dependencies/RAPPORT_AGENT_CORRECTION_ECHECS_V3_V4_EXECUTION.md`

Inclure:
1. toutes les commandes exactes,
2. sorties essentielles,
3. version dataset publiée,
4. statut kernel après 59s,
5. incidents + corrections,
6. tableau final GO/NO-GO.
