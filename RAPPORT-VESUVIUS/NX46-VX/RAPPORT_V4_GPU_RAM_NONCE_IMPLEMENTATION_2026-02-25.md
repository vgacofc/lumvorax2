# RAPPORT V4 — GPU/RAM + Golden Nonce Persistance Temps Réel

## Chemin complet de la nouvelle version
`/workspace/Lumvorax/RAPPORT-VESUVIUS/NX46-VX/result-nx46-vx-v4/nx46-vx-unified-kaggle-v4.ipynb`

## Résumé d’implémentation
- Une V4 a été générée à partir de la V3 via un outil dédié.
- Un patch runtime V4 est injecté (`V4_RUNTIME_PATCH_BOOT`) avec:
  - cible GPU: `NX46VX_V4_GPU_UTIL_TARGET_PCT=96`
  - cible RAM: `NX46VX_V4_RAM_UTIL_TARGET_PCT=96`
  - trace persistante: `NX46VX_V4_GPU_RAM_TRACE_PATH=gpu_ram_trace_v4.jsonl`
  - persistance nonce: `NX46VX_V4_NONCE_PERSIST_PATH=golden_nonce_events_v4.jsonl`
- La logique golden nonce a été élargie:
  - `V1442_GOLDEN_NONCE_TOPK=0` est interprété comme **illimité**.
  - chaque nonce est persisté en temps réel via un événement JSONL `GOLDEN_NONCE_FOUND`.

## Vérification ligne par ligne (V3 -> V4)
- Rapport auto-généré:
  - `RAPPORT-VESUVIUS/NX46-VX/RAPPORT_DIFF_LIGNE_A_LIGNE_V3_V4_2026-02-25.md`
- Outil de génération du diff:
  - `RAPPORT-VESUVIUS/NX46-VX/tools/notebook_diff_report.py`

## Statut "96% GPU/RAM"
- **Réalisé côté configuration/pilotage et instrumentation** (variables cibles + traçage continu).
- **Dépendance runtime**: l’atteinte stricte de 96% observé dépend de la charge réelle, du batching, des I/O et du scheduler GPU Kaggle.
- Le notebook V4 enregistre les observations pour audit post-run.

## Fichiers ajoutés/modifiés
- `RAPPORT-VESUVIUS/NX46-VX/tools/create_v4_from_v3_notebook.py`
- `RAPPORT-VESUVIUS/NX46-VX/tools/notebook_diff_report.py`
- `RAPPORT-VESUVIUS/NX46-VX/result-nx46-vx-v4/nx46-vx-unified-kaggle-v4.ipynb`
- `RAPPORT-VESUVIUS/NX46-VX/RAPPORT_DIFF_LIGNE_A_LIGNE_V3_V4_2026-02-25.md`
- `RAPPORT-VESUVIUS/NX46-VX/RAPPORT_V4_GPU_RAM_NONCE_IMPLEMENTATION_2026-02-25.md`
