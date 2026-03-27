# FEUILLE DE ROUTE V5 — NX46 Vesuvius (mise à jour opérationnelle)

Date: 2026-02-18

## 1) Objectif
Stabiliser la chaîne NX46 pour garantir:
1. conformité soumission Kaggle (format + contenu),
2. robustesse offline,
3. amélioration mesurable du score public sans régression.

## 2) État actuel résumé
- Baseline publique de référence observée: **0.303** (v7.3 historique).
- Artefacts v7.4/v7.5 inspectés: packaging volumique 3D cohérent.
- Score final de la campagne active v7.3: **en attente de confirmation finale**.

## 3) Jalons V5

### J0 — Préflight obligatoire (bloquant)
- Vérifier présence des dépendances offline critiques.
- Vérifier que la sortie contient bien `submission.zip`.
- Vérifier chaque TIFF attendu:
  - shape `(Z,H,W)`,
  - `dtype=uint8`,
  - binaire strict.

### J1 — Validation format/scoring
- Lancer 1 run dry-run complet.
- Archiver hash SHA du `submission.zip`.
- Exécuter validation de contenu avant upload Kaggle.

### J2 — Campagne score
- Lancer 3 runs contrôlés (A/B/C) avec paramètres documentés.
- Comparer score public, stabilité run-to-run, ratio non-zero.

### J3 — Intégration TL (si J2 stable)
- Injecter score distillé student (multi-teachers) en fusion pondérée.
- Ne conserver que la variante qui bat baseline sans régression format/robustesse.

## 4) KPI de suivi
- KPI-1: taux de runs sans erreur offline.
- KPI-2: taux de soumissions acceptées par Kaggle.
- KPI-3: variation score public vs baseline (objectif: > +0.005 stable).
- KPI-4: variance score run-to-run.

## 5) Risques surveillés
1. Rupture de dépendances offline (imagecodecs/tifffile).
2. Dérive de seuil augmentant faux positifs.
3. Sur-ajustement local non transférable leaderboard.

## 6) Critère Go/No-Go
- **GO** si: conformité 100% + amélioration score stable.
- **NO-GO** si: score instable, régression, ou anomalies de format.
