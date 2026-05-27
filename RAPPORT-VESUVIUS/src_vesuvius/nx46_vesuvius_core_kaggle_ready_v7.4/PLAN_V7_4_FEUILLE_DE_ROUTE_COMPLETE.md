# NX46 V7.4 — Plan de réalisation complet (A→Z)

## 1) Objectif de version
Améliorer la base v7.3 (**0.303**) sans perdre la robustesse format Kaggle déjà acquise.

## 2) Point de départ validé
- v7.3 est robuste sur le packaging/format (3D, contrôles shape/valeurs).
- Risque identifié: pipeline potentiellement trop conservateur (sous-détection).

## 3) Contraintes non régressives
- Garder `submission.zip` compatible Kaggle.
- Garder checks: ndim=3, shape exacte, valeurs binaires (désormais 0/1), publication alias.
- Ne pas casser la logique forensic/logging.

## 4) Backlog V7.4 (avec % d’avancement actuel)

### Bloc A — Sécurité production (héritage v7.3)
- A1. Validation ZIP + contenu binaire: **100%** (hérité).
- A2. Publication multi-chemins de soumission: **100%** (hérité).
- A3. Logs forensic et état runtime: **100%** (hérité).

### Bloc B — Récupération de recall
- B1. Mini-grille `threshold_quantile` [0.970..0.985]: **0%**.
- B2. Mini-grille `score_blend_3d_weight` [0.65..0.85]: **0%**.
- B3. Option seuil adaptatif par tranche Z: **0%**.

### Bloc C — Qualité micro-structures
- C1. Mesure continuité des traits inter-slices: **0%**.
- C2. Pénalisation petits faux positifs isolés: **0%**.
- C3. Rapport erreur localisé (XOR vs baseline): **0%**.

### Bloc D — Campagne Kaggle
- D1. Run v7.4-A (conservateur): **0%**.
- D2. Run v7.4-B (équilibré): **0%**.
- D3. Run v7.4-C (rappel renforcé): **0%**.
- D4. Choix final via score + robustesse: **0%**.

## 5) Questions expertes à poser sur v7.4
1. Le gain de score est-il stable sur plusieurs runs ?
2. Quels fragments perdent le plus d’encre avec seuil élevé ?
3. Quel paramètre a l’effet marginal le plus fort ?
4. Peut-on augmenter recall sans réintroduire bruit massif ?

## 6) Critères de succès v7.4
- Score public > 0.303.
- Conformité Kaggle inchangée.
- Rapport comparatif v7.3 vs v7.4 documenté et reproductible.

## 7) Implémentation réalisée dans ce dossier
- Code implanté: `nx46-vesuvius-core-kaggle-ready-v7.4.py` ✅
- Écriture masque ajustée en `uint8` 0/1 ✅
- Validation contenu zip ajustée pour accepter `{0,1}` ✅

## 8) Ce qu’il reste à faire immédiatement
1. Implémenter bloc B.
2. Exécuter la campagne D.
3. Produire rapport final v7.4 avec score consolidé.
