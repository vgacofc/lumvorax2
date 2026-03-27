# RAPPORT DIAGNOSTIC V7.3 — Vesuvius Submission Format

## Confirmation de compréhension
Oui, le travail porte sur **la fiabilité de soumission Kaggle** (format/chemins/contenu TIFF), en comparant:
- l'exemple concurrent (score 0.552),
- la version NX47 (score 0.387),
- la version v7.2 (exécution offline OK mais scoring error).

## Lecture et extraction réalisées
Artefacts inspectés:
- `RAPPORT-VESUVIUS/chat-session-precedente/chat-session precedente-de-tout-ce-qui-a-deja-ete-fais.md`
- `RAPPORT-VESUVIUS/exemple-soumision-notebook-concurrent/results.zip`
- `RAPPORT-VESUVIUS/exemple-soumision-notebook-concurrent/vesuvius-0-552.ipynb`
- `RAPPORT-VESUVIUS/notebook-version-NX47-score-0.387-V61/results.zip`
- `RAPPORT-VESUVIUS/notebook-version-NX47-score-0.387-V61/nx46-vesuvius-challenge-surface-detection.ipynb`
- `RAPPORT-VESUVIUS/output_logs_vesuvius/v7.2-outlput-logs--nx46-vesuvius-core-kaggle-ready/results.zip`
- `RAPPORT-VESUVIUS/output_logs_vesuvius/v7.2-outlput-logs--nx46-vesuvius-core-kaggle-ready/nx46-vesuvius-core-kaggle-ready.ipynb`

## Problème critique identifié
Le cœur du problème v7.2 est le format de sortie:
- v7.2 écrit des TIFF **2D** (1 page) dans `submission.zip`.
- concurrent + NX47 qui scorent écrivent des TIFF **3D multi-pages (320 pages)**.

Conséquence probable: le scorer attend un volume multi-pages et rejette la soumission même si le zip existe et contient des fichiers nommés correctement.

## Correctifs appliqués
### 1) Mise à jour notebook NX47 (base score 0.387)
Fichier modifié:
- `RAPPORT-VESUVIUS/notebook-version-NX47-score-0.387-V61/nx46-vesuvius-challenge-surface-detection.ipynb`

Actions:
- Forçage explicite du masque en dynamique complète `0/255`.
- Validation stricte avant écriture: rang attendu `3D (Z,H,W)`.
- Publication d'alias du zip pour robustesse de chemin (`/kaggle/working/nx46_vesuvius/submission.zip`, etc.).

### 2) Création de NX-46 v7.3
Nouveau fichier:
- `RAPPORT-VESUVIUS/output_logs_vesuvius/v7.3-outlput-logs--nx46-vesuvius-core-kaggle-ready/nx46-vesuvius-core-kaggle-ready.ipynb`

Actions:
- Remplacement de l'écriture 2D par écriture **multi-pages 3D**.
- Ajout d'un `expected_meta` par fichier `(Z,H,W)` au lieu de `(H,W)`.
- Si la prédiction est 2D, expansion automatique en 3D via répétition sur `Z`.
- Validation zip renforcée: contrôle `ndim==3`, shape exacte `(Z,H,W)`, valeurs binaires `0/255`.
- Conservation du packaging Kaggle (`/kaggle/working/submission.zip` + aliases + ZIP_STORED + LZW).

## Point d'attention sur les logs demandés
Les chemins exacts suivants ne sont pas présents tels quels localement:
- `.../vesuvius-0-552.log`
- `.../nx46-vesuvius-challenge-surface-detection.log`

Dans ces dossiers, les logs existent en `.txt` (ex: `qmSqO2X8.txt`, `WjlGH0KO.txt`, `ZA5jJ6j8.txt`) et ont été utilisés comme sources de diagnostic disponibles.
