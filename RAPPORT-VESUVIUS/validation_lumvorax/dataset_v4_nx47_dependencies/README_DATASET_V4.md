# Dataset V4 NX47 Dépendances — Pack complet prêt à pousser

Ce dossier contient les outils de préparation V4:

- `build_dataset_v4_bundle.py` : construit le bundle V4 (copie wheels + compile `.so` + extraction TIFF concurrent + conversion `.lum` + manifeste JSON horodaté ns).
- `verify_dataset_v4_bundle.py` : vérifie le bundle final (présence fichiers requis + statut build report).
- `PROMPT_AGENT_PUSH_DATASET_V4_NX47_DEPENDENCIES_20260221.md` : prompt final à donner à l’agent chargé du push Kaggle.

## Sortie attendue
Le dossier `bundle/` doit contenir:
1. wheels requis V1+V2,
2. `liblumvorax_replit.so`,
3. `competitor_teacher_1407735.tif`,
4. `competitor_teacher_1407735.lum`,
5. `dataset_v4_build_report.json`.

## Source TIFF concurrent
- `RAPPORT-VESUVIUS/exemple-soumision-notebook-concurrent/results.zip`
- extraction attendue: `results.zip/submission.zip/1407735.tif`

## Commande locale (exemple)
```bash
python3 RAPPORT-VESUVIUS/validation_lumvorax/dataset_v4_nx47_dependencies/build_dataset_v4_bundle.py \
  --repo-root . \
  --output-dir RAPPORT-VESUVIUS/validation_lumvorax/dataset_v4_nx47_dependencies/bundle \
  --wheel-source /kaggle/input/datasets/ndarray2000/nx47-dependencies

python3 RAPPORT-VESUVIUS/validation_lumvorax/dataset_v4_nx47_dependencies/verify_dataset_v4_bundle.py \
  --dir RAPPORT-VESUVIUS/validation_lumvorax/dataset_v4_nx47_dependencies/bundle
```

## Politique qualité
- Si un fichier requis manque: NO-GO.
- Si compilation `.so` échoue: NO-GO.
- Si conversion TIFF concurrent -> `.lum` échoue: NO-GO.
- Si rapport build absent: NO-GO.
