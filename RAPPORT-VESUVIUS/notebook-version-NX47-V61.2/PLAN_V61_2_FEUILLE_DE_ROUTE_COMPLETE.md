# NX47 V61.2 — Plan de réalisation complet (A→Z)

## 1) Objectif de version
Monter NX47 de la base confirmée **0.387** vers une version plus stable/plus précise sans casser la conformité Kaggle.

## 2) Contraintes non négociables
- TIFF multipage 3D `(Z,H,W)`.
- `uint8` binaire strict `{0,1}` aligné format concurrent.
- Zip de soumission publié sur les chemins attendus Kaggle.

## 3) Hypothèses techniques de travail
1. V61.1 est déjà bonne en score relatif local (0.387) mais peut contenir sur-densification.
2. Le gain viendra surtout du calibrage (seuils/fusion/post-process), pas d’un changement de format.
3. La densité est un indicateur de pilotage, pas un objectif final.

## 4) Backlog V61.2 (avec % d’avancement actuel)

### Bloc A — Baseline & instrumentation
- A1. Geler baseline v61.1 + checksum artefacts: **100%**.
- A2. Export métriques par slice (densité, continuité, composantes): **35%**.
- A3. Tableau comparatif auto v61.1 vs v61.2: **10%**.

### Bloc B — Calibration score-first
- B1. Grille `p_hi/p_lo` + fusion-weight: **0%**.
- B2. Contrôle de densité cible dynamique par fragment: **0%**.
- B3. Rejet auto des runs qui dégradent recall local: **0%**.

### Bloc C — Post-traitement robuste
- C1. Filtre îlots parasites (small objects): **0%**.
- C2. Stabilisation inter-slices (cohérence Z): **0%**.
- C3. Contrôle topologique fin (éviter halos): **0%**.

### Bloc D — Validation Kaggle
- D1. Validation format et contenu zip: **100%** (hérité).
- D2. Exécutions Kaggle v61.2 #1/#2/#3: **0%**.
- D3. Rapport final delta score vs v61.1: **0%**.

## 5) Questions expertes à trancher avant run final
1. Quel est le compromis optimal recall/precision par fragment ?
2. Quelle variable impacte le plus le score: quantile, fusion, ou post-process ?
3. À quel niveau de densité la précision commence à chuter fortement ?
4. Les gains viennent-ils d’encre réelle ou d’artefacts répétitifs ?

## 6) Critères de succès V61.2
- Score public > 0.387 **ou** même score avec meilleure robustesse inter-run.
- Aucun échec de validation de soumission.
- Rapport de différences explicite et reproductible.

## 7) Implémentation réalisée dans ce dossier
- Code implanté: `nx47-vesuvius-challenge-surface-detection-v61.2.py` ✅
- Sortie binaire ajustée en `uint8` 0/1 ✅
- Sortie TIFF 3D multipage + ZIP inchangée ✅

## 8) Ce qu’il reste à faire immédiatement
1. Implémenter la grille de calibration (Bloc B).
2. Lancer 3 exécutions Kaggle.
3. Consolider rapport final V61.2 avec nouveaux scores.
