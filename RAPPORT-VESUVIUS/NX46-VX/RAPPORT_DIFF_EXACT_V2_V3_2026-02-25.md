# RAPPORT DIFF EXACT — NX46-VX V2 ➜ V3

Date: 2026-02-25

## 1) Scope de réalisation

- Source copiée: `RAPPORT-VESUVIUS/NX46-VX/result-nx46-vx-v2/nx46-vx-unified-kaggle-v2.ipynb`
- Nouvelle cible V3: `RAPPORT-VESUVIUS/NX46-VX/result-nx46-vx-v3/nx46-vx-unified-kaggle-v3.ipynb`
- Outil de génération: `RAPPORT-VESUVIUS/NX46-VX/tools/create_v3_from_v2_notebook.py`

## 2) Diff structurel notebook

- Nombre de cellules V2: `14`
- Nombre de cellules V3: `16`
- Ajout V3: `+1 markdown` + `+1 code` en tête de notebook (après intro).

## 3) Diff exact des paramètres injectés en V3

### 3.1 GPU / device
- `CUDA_VISIBLE_DEVICES=0`
- `CUDA_DEVICE_ORDER=PCI_BUS_ID`
- Boot runtime imprime:
  - `torch_cuda_available`
  - `torch_gpu_name`
  - `tf_visible_gpus`

### 3.2 Couverture entraînement / preflight
- `V1442_PREFLIGHT_TRAIN_PCT=100.0` (était 5.0 via défaut)
- `V1442_PREFLIGHT_TEST_PCT=100.0` (était 5.0 via défaut)
- `V1442_MAX_TRAIN_VOLUMES=0` (uncapped/all)
- `V1442_MAX_VAL_VOLUMES=0` (uncapped/all)

### 3.3 Convergence / capacité
- `V1442_CONVERGENCE_PATIENCE=64` (au lieu de 5)
- `V1442_META_NEURONS=9` (au lieu de 3)
- `V1442_GOLDEN_NONCE_TOPK=64` (au lieu de 11)

### 3.4 Flags V3 forensic (compat future)
- `NX46VX_V3_ENABLE_GPU_ALL_PHASES=1`
- `NX46VX_V3_ENABLE_TEACHER_TRANSFER_EVENTS=1`
- `NX46VX_V3_ENABLE_COMPETITOR_TIFF_PHASE_EVENTS=1`
- `NX46VX_V3_ENABLE_PER_NONCE_EVENTS=1`
- `NX46VX_V3_REQUIRE_COVERAGE_100=1`

## 4) Audit GPU + runtime V2 (avant)

Voir: `RAPPORT-VESUVIUS/NX46-VX/result-nx46-vx-v2/AUDIT_GPU_RUNTIME_V2.md`

Constat avant V3:
- P100 détectée au BOOT.
- Preuve explicite d’usage GPU A→Z insuffisante dans logs publics.
- Durée V2 (`elapsed_total_s≈2542s`) très inférieure à 9h (donc le scoring error V2v4 n’est probablement pas un timeout global de ce run précis).

## 5) Ce qui est corrigé maintenant

1. V3 est matérialisée (copie + patch exécutable).
2. Forçage GPU et preuve de visibilité GPU imprimée dès le début.
3. Préflight borné supprimé côté pourcentage (100/100) pour éviter apprentissage tronqué.
4. Cap train/val levé (`0`) pour viser couverture complète.
5. Convergence moins restrictive (patience augmentée).
6. Capacité méta-neurones et topK nonces augmentées.
7. Traces V3 prêtes pour audit phase-par-phase.

## 6) Limites restantes (honnêtes)

- Le patch V3 est injecté au notebook; pour preuve GPU A→Z absolue, il faut exécuter V3 et collecter les logs runtime V3.
- Les événements teacher-distillation détaillés nécessitent aussi la logique kernel alignée avec ces nouveaux flags (prévue dans la roadmap V3).
