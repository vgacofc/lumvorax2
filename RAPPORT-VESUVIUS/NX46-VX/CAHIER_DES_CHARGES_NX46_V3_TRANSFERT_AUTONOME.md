# Cahier des charges — NX46-VX V3 (transfert réel + auto-tuning + couverture 100%)

Date: 2026-02-25
Objectif: corriger les blocages identifiés en V2 **sans rétrograder** ni casser le pipeline existant.

## A) Principes de non-régression

1. Garder le chemin V2 fonctionnel par défaut.
2. Activer V3 via flags explicites (feature flags).
3. Ajouter des preuves forensic obligatoires par phase.
4. Interdire les phases “déclarées mais non prouvées”.

## B) Exigences fonctionnelles V3

## B1) Distillation 9 teachers obligatoire et prouvable

### Exigence
- Si 9 teachers disponibles: distillation doit s’exécuter avant tout fine-tuning.
- Si distillation non exécutée: blocage `NO_GO_SUBMISSION`.

### Événements logs obligatoires
- `TEACHER_DISTILLATION_START`
- `TEACHER_BATCH_TRANSFER`
- `TEACHER_DISTILLATION_DONE`
- `TRANSFER_MEMORY_PERSISTED`
- `TRANSFER_MEMORY_RELOADED_FOR_FINETUNE`

### KPI
- `teacher_coverage_pct`
- `teacher_alignment_gain_before_after`
- `student_knowledge_retention_score`

## B2) TIFF concurrent intégré dans phase dédiée et mesurée

### Exigence
- Phase `TEST_GUIDED_ULTRA_FINETUNE_WITH_COMPETITOR_TIFF` explicite.
- Journaliser alignement avant/après (IoU proxy, Dice proxy, density gap).

### Événements logs obligatoires
- `COMPETITOR_TIFF_LOAD_OK`
- `COMPETITOR_GUIDED_PHASE_START`
- `COMPETITOR_GUIDED_PHASE_DONE`
- `COMPETITOR_ALIGNMENT_DELTA`

## B3) Auto-tuning dynamique des paramètres critiques

### Exigence
Transformer les paramètres statiques en paramètres adaptatifs (avec bornes):
- `dust_min_size`
- `mutation_noise`
- `convergence_patience`
- `meta_neurons`

### Règles minimales
- `dust_min_size`: adapter selon taille composantes prédites et taux de faux positifs.
- `mutation_noise`: scheduler (hausse si stagnation, baisse si divergence).
- `convergence_patience`: augmenter automatiquement quand amélioration lente mais positive.
- `meta_neurons`: expansion progressive si sous-capacité détectée.

### Événements logs obligatoires
- `AUTO_TUNE_PARAM_UPDATE`
- `AUTO_TUNE_POLICY_DECISION`
- `CONVERGENCE_PATIENCE_EXPANDED`

## B4) Couverture entraînement réelle = 100% (ou justification formelle)

### Exigence
- Cible nominale: `pair_count_selected_for_training == pair_count_discovered`.
- Si impossible (temps/mémoire): mode fallback explicite avec preuve de contrainte.

### Événements logs obligatoires
- `TRAIN_COVERAGE_PLAN`
- `TRAIN_COVERAGE_PROGRESS`
- `TRAIN_COVERAGE_FINAL`

### KPI
- `train_pair_coverage_pct_effective`
- `coverage_gap_reason`
- `coverage_gap_is_justified` (bool)

## B5) Golden nonces traçables unitairement

### Exigence
- Conserver `topk`, mais logguer chaque nonce détecté.

### Événements logs obligatoires
- `GOLDEN_NONCE_FOUND` (un event / nonce avec ts, y, x, score)
- `GOLDEN_NONCE_TOPK_FINAL`

## C) Plan de migration technique (sans casser V2)

## Sprint 1 — Observabilité forte (safe)
- Ajouter événements manquants (teachers, TIFF concurrent, auto-tuning, nonces unitaires).
- Aucune modification du comportement de sortie.
- Critère: mêmes artefacts qu’avant + logs enrichis.

## Sprint 2 — Activation transfert + mémoire persistante
- Implémenter mémoire de transfert teacher→student persistée et rechargée.
- Ajouter contrôles d’intégrité (hash, version, compatibilité).
- Critère: gain mesurable sur KPI teacher alignment.

## Sprint 3 — Auto-tuning borné
- Activer scheduler dynamique pour les 4 paramètres.
- Ajouter garde-fous anti-divergence et rollback local des paramètres.
- Critère: stabilité run-to-run + convergence améliorée.

## Sprint 4 — Couverture 100% et optimisation coût
- Passer de préflight 5% à couverture progressive vers 100%.
- Chunking mémoire + reprise checkpointée pour sessions longues.
- Critère: couverture >= 95% puis 100% en mode complet.

## D) Critères GO / NO-GO V3

GO seulement si:
1. Distillation 9 teachers prouvée en logs.
2. Phase TIFF concurrent prouvée + delta alignement non nul.
3. Couverture entraînement >= 95% (objectif 100%).
4. Nonces unitaires traçables + top-K final cohérent.
5. Aucune régression format soumission Kaggle.

Sinon: NO-GO automatique (pré-exécution uniquement).

## E) Sorties attendues V3

- `v3_transfer_memory_manifest.json`
- `v3_teacher_alignment_report.json`
- `v3_competitor_guided_delta.json`
- `v3_autotune_trace.json`
- `v3_golden_nonce_timeline.jsonl`
- `v3_train_coverage_report.json`

## F) Risques et mitigation

- Risque: explosion temps d’exécution.
  - Mitigation: checkpoints + reprise incrémentale + budget adaptatif.
- Risque: divergence avec auto-tuning.
  - Mitigation: bornes strictes + rollback paramètre par paramètre.
- Risque: régression soumission.
  - Mitigation: tests format Kaggle à chaque sprint.


## B6) GPU P100 active de A à Z (preuve obligatoire)

### Exigence
- Toute phase lourde doit logguer explicitement son device (`cpu`/`cuda:0`).
- Interdire silent fallback CPU sans événement explicite.

### Événements logs obligatoires
- `GPU_PHASE_START`
- `GPU_PHASE_END`
- `GPU_FALLBACK_DETECTED`
- `GPU_MEMORY_SNAPSHOT`

### KPI
- `gpu_phase_coverage_pct` (objectif 100%)
- `gpu_time_ratio`
- `cpu_fallback_count`

## B7) Budget temps Kaggle (< 9h)

### Exigence
- Estimer ETA cumulative en continu.
- Déclencher mode accéléré si ETA > 8h15.

### Événements logs obligatoires
- `RUNTIME_BUDGET_HEARTBEAT`
- `RUNTIME_ACCELERATION_MODE_ON`
- `RUNTIME_BUDGET_FINAL`

### KPI
- `elapsed_total_h`
- `runtime_margin_to_9h`
- `acceleration_actions_applied`
