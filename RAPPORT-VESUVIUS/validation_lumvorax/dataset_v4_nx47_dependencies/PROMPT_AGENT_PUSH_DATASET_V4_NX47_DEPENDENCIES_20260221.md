# PROMPT AGENT — Push Dataset V4 NX47 Dépendances + mise à jour Kernel test 360

Tu dois:
1) pousser le dataset V4,
2) mettre à jour le kernel Kaggle `ndarray2000/test-360-dep-lumvorax`,
3) attendre 59 secondes,
4) récupérer les logs/sorties,
5) produire un rapport complet des actions et incidents.

## 0) Chemins exacts (NE PAS CHANGER)
- Racine bundle V4: `RAPPORT-VESUVIUS/validation_lumvorax/dataset_v4_nx47_dependencies`
- Dossier artefacts dataset à pousser: `RAPPORT-VESUVIUS/validation_lumvorax/dataset_v4_nx47_dependencies/bundle`
- Script test 360 V3 complet: `RAPPORT-VESUVIUS/validation_lumvorax/notebook_validation_lumvorax_dependances_360_kaggle_single_cell_V3_COMPLETE_20260221.py`
- Notebook test 360 V4 prêt pour push Kaggle (éditeur notebook): `RAPPORT-VESUVIUS/validation_lumvorax/notebook_validation_lumvorax_dependances_360_kaggle_single_cell_V4_COMPLETE_20260221.ipynb`
- TIFF concurrent source obligatoire: `RAPPORT-VESUVIUS/exemple-soumision-notebook-concurrent/results.zip`

## 1) Pré-check obligatoire
```bash
kaggle --version
test -f ~/.kaggle/kaggle.json
python3 RAPPORT-VESUVIUS/validation_lumvorax/dataset_v4_nx47_dependencies/verify_dataset_v4_bundle.py \
  --dir RAPPORT-VESUVIUS/validation_lumvorax/dataset_v4_nx47_dependencies/bundle
kaggle datasets files ndarray2000/nx47-dependencies --page-size 200
kaggle kernels status ndarray2000/test-360-dep-lumvorax
```
Si une commande échoue => STOP immédiat + NO-GO.

## 2) Push dataset V4
```bash
kaggle datasets version \
  -p RAPPORT-VESUVIUS/validation_lumvorax/dataset_v4_nx47_dependencies/bundle \
  -m "V4 complete NX47 dependencies: wheels union + rebuilt liblumvorax_replit.so + competitor TIFF/LUM + build manifest"
```

## 3) Mettre à jour le kernel Kaggle test-360

### 3.1 Pull du kernel local
```bash
mkdir -p /tmp/kaggle_kernel_test360_v4
kaggle kernels pull ndarray2000/test-360-dep-lumvorax -p /tmp/kaggle_kernel_test360_v4
```

### 3.2 Remplacer le code notebook sans changer le type d'éditeur
- Vérifier le fichier local du kernel pullé:
  - metadata: `/tmp/kaggle_kernel_test360_v4/kernel-metadata.json`
  - notebook attendu: `/tmp/kaggle_kernel_test360_v4/test-360-dep-lumvorax.ipynb`
- Copier le notebook V4 dans le notebook du kernel (garder `.ipynb`, ne pas pousser un `.py`):
```bash
cp RAPPORT-VESUVIUS/validation_lumvorax/notebook_validation_lumvorax_dependances_360_kaggle_single_cell_V4_COMPLETE_20260221.ipynb \
  /tmp/kaggle_kernel_test360_v4/test-360-dep-lumvorax.ipynb
```
- Vérifier que le dataset input du kernel inclut:
  - `ndarray2000/nx47-dependencies`
- Ajouter/maintenir des sorties persistantes dans `/kaggle/working`:
  1. `lumvorax_360_validation_report_v3_complete.json` (rapport principal)
  2. `lumvorax_events_ns.jsonl` (1 événement JSON par ligne, horodaté ns)
  3. `lumvorax_runtime_metrics.csv` (colonnes: ts_ns,cpu_percent,ram_used_mb,ram_total_mb,gpu_mem_used_mb,gpu_mem_total_mb,step)
  4. `lumvorax_latency_breakdown.json` (durée ns par phase)
  5. `lumvorax_hardware_fingerprint.json` (CPU/GPU/RAM/OS/Python)
  6. `lumvorax_checksums_manifest.json` (sha256 des artefacts produits)
  7. `lumvorax_execution_stdout.log` (copie stdout complète)
  8. `lumvorax_execution_stderr.log` (copie stderr complète)
- Ajouter métriques dérivées minimales dans le JSON principal:
  - `throughput_items_per_s`
  - `throughput_pixels_per_s`
  - `latency_ns_by_step`
  - `peak_ram_mb`
  - `peak_gpu_mem_mb`

### 3.3 Push du kernel
```bash
kaggle kernels push -p /tmp/kaggle_kernel_test360_v4
```

## 4) Attente 59 secondes + récupération logs
```bash
sleep 59
kaggle kernels status ndarray2000/test-360-dep-lumvorax
kaggle kernels output ndarray2000/test-360-dep-lumvorax -p /tmp/kaggle_kernel_test360_v4_output
```

## 5) Vérifications post-run obligatoires
1. Confirmer dans les sorties qu’il n’y a aucun module manquant.
2. Confirmer que le test concurrent assets passe (`competitor_teacher_1407735.tif` + `.lum`).
3. Confirmer `status: ok` dans le JSON V3 complet.
4. Confirmer présence des traces événements (`events_count > 0`) et logs détaillés.
5. Confirmer présence des 8 fichiers de logs persistants listés en section 3.2.
6. Confirmer présence des métriques de performance (latence + throughput + pics mémoire).

## 6) Rapport final obligatoire (sans omission)
Créer:
- `RAPPORT-VESUVIUS/validation_lumvorax/dataset_v4_nx47_dependencies/RAPPORT_AGENT_PUSH_DATASET_V4_EXECUTION.md`

Le rapport doit contenir:
1. Date/heure de chaque étape.
2. Commandes exactes exécutées.
3. Sorties console importantes (succès/erreurs).
4. Version dataset Kaggle publiée (ID/version).
5. Version kernel publiée + statut après 59 secondes.
6. Tous les problèmes rencontrés et comment ils ont été traités.
7. Tableau PASS/FAIL pour chaque fichier de log persistant.
8. Tableau PASS/FAIL pour chaque métrique de performance.
9. Verdict final GO/NO-GO.

## 7) Interdictions
- Ne pas changer le nom du dataset cible.
- Ne pas pousser un autre dossier que `.../dataset_v4_nx47_dependencies/bundle`.
- Ne pas masquer une erreur (si échec => le rapport doit le dire explicitement).
