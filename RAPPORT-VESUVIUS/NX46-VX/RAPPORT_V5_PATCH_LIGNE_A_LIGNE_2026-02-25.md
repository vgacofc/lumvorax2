# Rapport V5 immédiat — clarification `"total": 135` + patch V4→V5

Date: 2026-02-25

## 1) Clarification explicite de `"total": 135`

Dans les logs `train_supervised/hyperparam_search`, le champ `total` correspond au **nombre total de configurations à évaluer** dans la boucle courante (grille d'hyperparamètres × neurones/epoch selon le contexte du run).

Concrètement:
- `index=13, total=135` signifie: **13e configuration en cours sur 135** pour cette boucle d'optimisation.
- Le pourcentage associé (`progress_percent`) est calculé sur cette base `index/total`.

Ce n'est **pas** le nombre total d'epochs globales, ni le nombre total de volumes.

---

## 2) Ce qui manquait (V4) et a été ajouté immédiatement (V5)

Ajouts implémentés dans l'outil de génération V5 depuis V4:

1. **Version runtime alignée**
   - Avant: `NX46-VX V2` pouvait rester affiché en V3/V4.
   - Après: remplacement explicite vers `NX46-VX V5`.

2. **Marqueurs de fin de phase explicites**
   - Ajout: `PHASE_1_DONE`, `PHASE_2_DONE`, `PHASE_3_DONE`, `PHASE_4_DONE`.
   - Objectif: lever toute ambiguïté sur l'état réel du pipeline.

3. **Preuve explicite de transfert teacher**
   - Ajout: événement `TEACHER_TRANSFER_APPLIED` avec:
     - `teachers_expected`
     - `teachers_registry`
     - `teacher_roots_runtime_found`
     - `teacher_transfer_enabled`

4. **KPI synthétique périodique en fit itératif**
   - Ajout: sous-événement `fit_kpi` dans `fit_prox_iter`.
   - Données: `iter`, `max_iter`, `%`, `eta_iter_remaining`.

5. **Nettoyage sémantique du signal bootstrap**
   - Avant: `SUBMISSION_READY` précoce potentiellement ambigu.
   - Après: renommé `PRECHECK_SUBMISSION_READY` pour distinguer précheck vs fin réelle.

---

## 3) Revue "avant/après" (ligne par ligne des points critiques)

## 3.1 Versioning
- Avant (V4): tag runtime pouvait afficher `NX46-VX V2`.
- Après (V5): remplacement forcé `NX46-VX V5`.

## 3.2 Distillation teacher
- Avant (V4): présence des compteurs imprimés (`registry`, `runtime roots`) mais pas d'événement de preuve unifié.
- Après (V5): `TEACHER_TRANSFER_APPLIED` normalisé en JSON.

## 3.3 Progression supervisée
- Avant (V4): `fit_prox_iter` détaillé mais KPI synthétique incomplet pour pilotage opérationnel.
- Après (V5): `fit_kpi` périodique avec ETA itérative.

## 3.4 Fin de phase
- Avant (V4): fin de phases implicite via transitions.
- Après (V5): marqueurs explicites `PHASE_X_DONE`.

## 3.5 Signal de soumission
- Avant (V4): `SUBMISSION_READY` tôt dans le bootstrap pouvait être lu comme “pipeline final terminé”.
- Après (V5): `PRECHECK_SUBMISSION_READY` clarifie qu'il s'agit d'un jalon de prévalidation.

---

## 4) Questions complémentaires d’expert à poser systématiquement

1. **Teacher transfer**
   - Les 9 teachers sont-ils *détectés runtime* et *effectivement injectés* dans les poids initiaux ?
   - Quelle est la contribution mesurée de chaque teacher (ablation teacher-by-teacher) ?

2. **Qualité de convergence**
   - Le balayage `(l1, l2)` est-il calibré par neurone et par classe de texture ?
   - Le critère d'arrêt compare-t-il CE/F1/FBeta avec contrôle de variance temporelle ?

3. **Données et coverage**
   - Le préflight 100% des paires train est-il strictement bloquant ?
   - Y a-t-il dérive distributionnelle train/val/test sur les fragments longs ?

4. **Traçabilité forensique**
   - Les événements critiques sont-ils tous signés/chaînés (merkle) sans trous temporels ?
   - Dispose-t-on d'un mapping stable `run_id -> artifacts -> submission` ?

5. **Performance système**
   - GPU/RAM atteignent-ils la cible (ex. 96%) sans throttling thermique ?
   - L'ETA phase est-elle recalculée dynamiquement à partir du throughput réel ?

6. **Risque produit / compétition**
   - Le workflow de soumission valide-t-il l'intégrité (checksum zip + tif) avant export ?
   - Les règles compétition sont-elles vérifiées au runtime et non seulement en précheck ?

---

## 5) Ce que vous n'aviez pas explicitement demandé (ajout proactif)

- Normaliser un **schéma d’événements** (versionné) pour garantir compatibilité entre V1→V5.
- Ajouter un **tableau de bord minimal** par phase: `%`, `ETA`, `stall_alert`, `teacher_state`.
- Ajouter des **tests de non-régression logs**: présence obligatoire de `PHASE_*_DONE` et `TEACHER_TRANSFER_APPLIED`.

---

## 6) Livrable technique produit

- Outil prêt: `create_v5_from_v4_notebook.py`
- Usage:

```bash
python RAPPORT-VESUVIUS/NX46-VX/tools/create_v5_from_v4_notebook.py \
  --v4-ipynb RAPPORT-VESUVIUS/NX46-VX/result-nx46-vx-v4/nx46-vx-unified-kaggle-v4.ipynb \
  --v5-ipynb RAPPORT-VESUVIUS/NX46-VX/result-nx46-vx-v5/nx46-vx-unified-kaggle-v5.ipynb
```

Ce script applique automatiquement les patchs ci-dessus pour produire V5.
