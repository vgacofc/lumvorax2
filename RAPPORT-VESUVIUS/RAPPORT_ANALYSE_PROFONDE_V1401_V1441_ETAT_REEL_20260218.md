# RAPPORT D’ANALYSE PROFONDE — État réel V140(.1) / V144(.1) + comparaison versions scorées

Date: 2026-02-18

## Expertises mobilisées (notification)
- **Forensic ML Ops** (corrélation artefacts ZIP, logs d’exécution, conditions Kaggle offline)
- **Computer Vision volumique** (validation TIFF 2D/3D, densité binaire, conformité de soumission)
- **MLOps fiabilité** (audit des dépendances offline, robustesse packaging et chemins)
- **Gouvernance de roadmap** (cohérence entre plans, avancement réel, éléments bloquants)

---

## 1) Problème rencontré immédiatement (notification obligatoire)
La demande cible les dossiers:
- `RAPPORT-VESUVIUS/notebook-version-NX47-V140.1`
- `RAPPORT-VESUVIUS/notebook-version-NX47-V144.1`

**Constat:** ces deux dossiers sont absents dans l’état actuel du dépôt local audité.

Conséquence: l’analyse ci-dessous est faite sur les artefacts disponibles les plus proches:
- V140
- V144
- V61.2 (version scorée)
- NX46 v7.4/v7.5 (artefacts de référence format)

---

## 2) Inspection profonde des artefacts ZIP et soumissions

## 2.1 V140 (artefact historique)
- `results.zip` contient un `submission.zip` imbriqué.
- Le TIFF de soumission dans ce snapshot est **2D** `(320,320)` et non volumique.
- Le run atteint `EXEC_COMPLETE` et `READY` malgré cette non-conformité au format scoré moderne.

### Découverte critique
Le log final indique:
- `train_pair_count_discovered = 786`
- `train_pair_coverage_pct = 4.0712`

=> **Anomalie de cohérence:** couverture train très faible vs cible 100%, malgré exécution « réussie ».

## 2.2 V144 (artefact historique)
- `results.zip` ne contient pas de `submission.zip`.
- Logs: arrêt précoce avec `RuntimeError: Offline dependency directory not found for imagecodecs`.

=> **Cause primaire d’échec:** bootstrap offline non résolu dans l’exécution observée.

## 2.3 V61.2 (version scorée)
- `results.zip` contient `nx47_vesuvius/submission.zip`.
- TIFF de soumission conforme volumique: `(320,320,320)`, `uint8`, binaire.
- Logs contiennent `SUBMISSION_READY`, `EXEC_COMPLETE`, `READY`.

## 2.4 NX46 v7.4 / v7.5 (références packaging)
- Les deux artefacts inspectés contiennent:
  - masques de soumission volumique `(320,320,320)` binaire,
  - structure de packaging stable,
  - cohérence des dimensions de test observées.
- Les métadonnées extraites v7.4 et v7.5 sont pratiquement identiques sur les TIFF inspectés.

---

## 3) Patterns, anomalies, découvertes “inconnues” utiles

### Pattern A — “Run successful” ≠ “Submission scorée correctement”
Un run peut finir avec `EXEC_COMPLETE` tout en produisant un artefact de soumission 2D (cas V140 historique).

### Pattern B — Dépendances offline = point de panne dominant V144
L’absence de `imagecodecs` dans la chaîne offline peut casser la run avant packaging.

### Pattern C — Couverture train potentiellement incohérente
`train_pair_coverage_pct` très basse dans V140 (4.07%) alors que la configuration affiche une contrainte de complétude stricte.

### Pattern D — Baselines scorées stables mais plafonnées
V61.2 est confirmée à `0.387` (public) au niveau de v61.1: pipeline valide, mais gain score non débloqué à ce stade.

---

## 4) Questions d’expert: répondues vs en attente

## 4.1 Questions résolues
1. **La cause d’échec V144 historique est-elle identifiée?** → Oui: dépendance offline.
2. **Le format de soumission est-il la variable-clé d’acceptation?** → Oui: la conformité 3D volumique est déterminante.
3. **V61.2 est-elle scorée?** → Oui, `0.387` public confirmé.

## 4.2 Questions en attente
1. **Où sont les artefacts V140.1/V144.1 demandés?** (dossiers absents localement)
2. **Quel est le score final certifié de la campagne V7.3 en cours?**
3. **Le faible `train_pair_coverage_pct` de V140 impacte-t-il directement la métrique finale?** (à isoler par ablation ciblée)

---

## 5) Vérification “plans & feuilles de route” dans RAPPORT-VESUVIUS

Inventaire détecté:
- `PLAN_CAHIER_DE_CHANCE_FEUILLE_DE_ROUTE_AZ.md`
- `notebook-version-NX47-V61.2/PLAN_V61_2_FEUILLE_DE_ROUTE_COMPLETE.md`
- `src_vesuvius/v7.4/PLAN_V7_4_FEUILLE_DE_ROUTE_COMPLETE.md`
- `.../v4-outlput.../PLAN_FEUILLE_DE_ROUTE_V4_REPONSES_EXPERTES.md`
- `FEUILLE_DE_ROUTE_V5_NX46_VESUVIUS.md`

### Constat de gouvernance
- Les 4 premiers fichiers contiennent bien de la structure de suivi.
- `FEUILLE_DE_ROUTE_V5_NX46_VESUVIUS.md` était vide avant correction de cette itération.

---

## 6) Plan de suggestions (cours pédagogique orienté action)

## 6.1 Priorité P0 (bloquants)
1. Synchroniser les dossiers manquants `V140.1` et `V144.1` pour audit complet réel.
2. Pour chaque run, imposer un validateur local de sortie **avant** soumission:
   - shape `(Z,H,W)`,
   - binaire strict,
   - présence `submission.zip`.

## 6.2 Priorité P1 (qualité score)
1. Investiguer le mismatch `train_pair_coverage_pct` vs objectif de complétude.
2. Isoler l’impact de ce facteur sur `Fbeta`/LB par 2 runs d’ablation contrôlés.

## 6.3 Priorité P2 (amélioration modèle)
1. Démarrer le plan Transfer Learning 9 teachers → 1 student (distillation).
2. Suivre un protocole d’ablation teacher-by-teacher pour éviter le transfert négatif.
3. Verrouiller calibration finale par volume pour éviter les dérives de seuil global.

---

## 7) Livrables de preuve générés pendant cette analyse
- `RAPPORT-VESUVIUS/analysis_v1401_v1441_deep_audit.json`
- `RAPPORT-VESUVIUS/plan_roadmap_audit_inventory.json`

Ces artefacts contiennent:
- inventaire des dossiers présents/absents,
- inspection ZIP + nested submission,
- métadonnées TIFF,
- synthèse de signaux logs,
- inventaire des plans/roadmaps détectés.
