# PROMPT AGENT — FIX V5 Kaggle (split runtime vs native strict) + chemins exacts

Objectif: éliminer les échecs observés (syntax error injection, faux `.c`, mismatch éditeur kernel) et produire un verdict GO/NO-GO fiable.

## 0) Fichiers et chemins exacts
- Notebook source à pousser (format notebook):
  - `RAPPORT-VESUVIUS/validation_lumvorax/notebook_validation_lumvorax_dependances_360_kaggle_single_cell_V4_COMPLETE_20260221.ipynb`
- Prompt push existant (référence):
  - `RAPPORT-VESUVIUS/validation_lumvorax/dataset_v4_nx47_dependencies/PROMPT_AGENT_PUSH_DATASET_V4_NX47_DEPENDENCIES_20260221.md`
- Bundle dataset local:
  - `RAPPORT-VESUVIUS/validation_lumvorax/dataset_v4_nx47_dependencies/bundle`
- Kernel local pull:
  - `/tmp/kaggle_kernel_test360_v5`
- Kernel notebook attendu:
  - `/tmp/kaggle_kernel_test360_v5/test-360-dep-lumvorax.ipynb`
- Metadata kernel:
  - `/tmp/kaggle_kernel_test360_v5/kernel-metadata.json`

## 1) Pré-checks obligatoires
```bash
kaggle --version
test -f ~/.kaggle/kaggle.json
python3 RAPPORT-VESUVIUS/validation_lumvorax/dataset_v4_nx47_dependencies/verify_dataset_v4_bundle.py \
  --dir RAPPORT-VESUVIUS/validation_lumvorax/dataset_v4_nx47_dependencies/bundle
```
Si échec -> STOP + NO-GO.

## 2) Pull kernel + préserver type éditeur
```bash
mkdir -p /tmp/kaggle_kernel_test360_v5
kaggle kernels pull ndarray2000/test-360-dep-lumvorax -p /tmp/kaggle_kernel_test360_v5
cat /tmp/kaggle_kernel_test360_v5/kernel-metadata.json
```
Règle: si `code_file` pointe un `.ipynb`, ne jamais pousser un `.py`.

## 3) Remplacement notebook correct
```bash
cp RAPPORT-VESUVIUS/validation_lumvorax/notebook_validation_lumvorax_dependances_360_kaggle_single_cell_V4_COMPLETE_20260221.ipynb \
  /tmp/kaggle_kernel_test360_v5/test-360-dep-lumvorax.ipynb
```

## 4) IMPORTANT — Interdiction des faux stubs C
- Ne pas créer de faux fichiers `.c` avec contenu minimal.
- Si vous devez injecter des sources, injecter les **vraies** sources C/H.
- Correction syntaxe si écriture string Python (obligatoire):
```python
with open(path, "w", encoding="utf-8") as f:
    f.write("// Native source certified for Lumvorax validation\\n")
```

## 5) Exécution en 2 modes (obligatoire)
### 5.1 Mode runtime rapide (GO competition)
- Variable: `LUMVORAX_REQUIRE_NATIVE=0`
- But: valider wheels + `.so` + assets competitor + roundtrip TIFF/LUM.

### 5.2 Mode native strict (GO certification)
- Variable: `LUMVORAX_REQUIRE_NATIVE=1`
- Prérequis: dataset sources C/H réels montés et détectés.
- But: valider headers > 0 + smoke compile C sans stub.

## 6) Push + attente + collecte
```bash
kaggle kernels push -p /tmp/kaggle_kernel_test360_v5
sleep 59
kaggle kernels status ndarray2000/test-360-dep-lumvorax
kaggle kernels output ndarray2000/test-360-dep-lumvorax -p /tmp/kaggle_kernel_test360_v5_output
```

## 7) Critères GO/NO-GO
### GO runtime (minimum pour avancer vite)
- `status: ok` dans `lumvorax_360_validation_report_v3_complete.json`
- assets `competitor_teacher_1407735.tif` et `.lum` présents
- logs persistants présents (events/metrics/latency/hardware/stdout/stderr/checksums)

### GO native strict
- `module_headers > 0`
- `c_syntax_smoke` sans échec
- aucune source stub

## 8) Rapport final obligatoire
Créer:
- `RAPPORT-VESUVIUS/validation_lumvorax/dataset_v4_nx47_dependencies/RAPPORT_EXECUTION_V5_RUNTIME_NATIVE_SPLIT.md`

Inclure:
1. commandes exactes,
2. sorties clés,
3. statut après 59s,
4. verdict séparé `GO_RUNTIME` / `GO_NATIVE_STRICT`,
5. anomalies détectées et correction appliquée.
