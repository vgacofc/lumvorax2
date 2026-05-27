# RAPPORT D'EXTRACTION EXACTE — MODÈLES DU NOTEBOOK CONCURRENT
Date: 2026-02-24
Source auditée: `RAPPORT-VESUVIUS/exemple-soumision-notebook-concurrent/vesuvius-0-552.ipynb`

## Verdict direct
- Nombre de chemins de modèles **explicitement présents** et exploitables dans le notebook concurrent local: **1**.
- Donc: **les 9 chemins/noms exacts ne sont pas tous présents dans ce notebook local**.

## Chemin exact détecté
1. `/kaggle/input/vsd-model/keras/transunet/3/transunet.seresnext50.160px.comboloss.weights.h5`

## Preuves ligne par ligne (dans le code du notebook concurrent)
- `kaggle_model_path="/kaggle/input/vsd-model/keras/"`
- `weights_relpath="transunet/3/transunet.seresnext50.160px.comboloss.weights.h5"`
- `weights_path = f"{CFG['kaggle_model_path']}/{CFG['weights_relpath']}"`
- `model = TransUNet(...)`
- `model.load_weights(weights_path)`

## Impact sur NX46 VX V1
- Le notebook `RAPPORT-VESUVIUS/NX46-VX/nx46-vx-unified-kaggle-V1.ipynb` contient maintenant:
  - `COMPETITOR_MODELS_FOUND_EXACT` avec le chemin exact détecté,
  - `TEACHER_MODELS_REGISTRY` (9 slots),
  - blocage strict tant que `< 9` modèles résolus.

## Ce qu'il manque encore pour fournir les 9 exacts
- Sources additionnelles nécessaires (ex: dataset Kaggle des poids, autre notebook concurrent, inventaire image des 9 modèles).
- Tant que ces sources ne sont pas fournies localement, impossible de prouver 9 chemins exacts depuis ce seul notebook.
