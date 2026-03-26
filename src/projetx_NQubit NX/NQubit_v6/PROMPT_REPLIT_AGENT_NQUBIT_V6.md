# PROMPT KAGGLE/CATLIN — NQubit V6 en UNE SEULE CELLULE

Objectif: exécuter **tout NQubit V6** en **une seule cellule notebook** sans dépendre d’autres fichiers.

## Fichier unique à copier-coller dans la cellule 1
- `src/projetx_NQubit NX/NQubit_v6/nqubit_v6_catlin_kaggle_kernel.py`

## Exécution (une seule cellule)
1. Ouvrir Kaggle Notebook.
2. Coller l’intégralité du fichier `nqubit_v6_catlin_kaggle_kernel.py` dans la cellule 1.
3. Exécuter la cellule.

## Ce que la cellule fait
- Détection environnement/hardware (Kaggle/Catlin/Replit/Colab, GPU/CPU)
- Collecte métriques système/hardware complètes
- Capture hardware réelle (`/dev/hwrng`, `/dev/random`, `/dev/urandom`) + jitter
- Stress CPU/RAM
- Build + verify manifest forensic V6
- Génération rapport final

## Artefacts attendus (dans `results/` du notebook)
- `kernel_env_detect_v6.json`
- `system_metrics_v6.json`
- `ADC_REAL_HARDWARE_CAPTURE_V6.json`
- `hardware_jitter_samples_v6.csv`
- `hardware_entropy_*.bin`
- `stress_telemetry_v6.log`
- `manifest_forensic_v6.json`
- `kernel_run_summary_v6.json`
- `kernel_final_state_v6.json`
- `RAPPORT_REPLIT_EXECUTION_V6_FINAL.md` (dans le dossier courant)

## Critère de succès
- `pipeline_ok=true`
- `missing=0` et `mismatches=0` dans la vérification manifest
