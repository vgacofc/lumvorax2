# PLAN_INTEGRATION_LUMVORAX_V4_NEXT

Ce fichier est le plan demandé pour **Lumvorax v4 next** sans modification directe du cœur tant que non validé.

## 1) Objectif
Intégrer le cycle Hubbard HTS avancé + validations NX en mode contrôlé, traçable et réversible.

## 2) Architecture d’adaptateurs
- `v4_next/adapters/hubbard_cycle_adapter.sh` : lance le cycle et renvoie `run_id`, chemins d’artefacts.
- `v4_next/adapters/hubbard_results_parser.py` : parse `new_tests_results.csv`, `expert_questions_matrix.csv`.
- `v4_next/adapters/nx_bridge.py` : lit les sorties NX et joint les scores.

## 3) Feature flags
- `ENABLE_HUBBARD_CYCLE_ADVANCED`
- `ENABLE_QMC_DMRG_BENCHMARK`
- `ENABLE_CLUSTER_STRESS_EXTENDED`
- `ENABLE_NX_CROSS_VALIDATION`

Par défaut: `false`.

## 4) Déploiement progressif
1. **Shadow mode**: exécuter en parallèle sans impacter le flux principal.
2. **Canary**: activer sur un sous-ensemble contrôlé.
3. **Full rollout**: activation globale après seuils stables.

## 5) Critères d’acceptation
- Build vert.
- Reproductibilité PASS.
- Benchmark QMC/DMRG PASS (RMSE, MAE, within_error_bar, CI95).
- Traçabilité complète (checksums, versions env, logs horodatés).
- Rollback validé (désactivation flags).

## 6) Risques et mitigations
- Risque perf/mémoire: limiter `steps`, profiler, planifier hors pic.
- Risque régression: shadow + canary + rollback.
- Risque incohérence schéma: versionner format CSV et contrats d’API.

## 7) Nouvelles intégrations (cycle 14 -> action immédiate)
- **Ingestion duale remote/local**: support explicite des runs présents sur `origin/main` mais absents localement.
- **Run drift monitor**: comparaison `elapsed_ns` entre run courant et run précédent pour séparer dérive infra vs signal modèle.
- **Glossaire non-expert auto**: génération de `tests/integration_terms_glossary.csv` à chaque cycle.
- **Confidence tags**: génération de `tests/integration_claim_confidence_tags.csv` (`certain/probable/unknown`).
- **Extracteur ABSENT**: génération de `tests/integration_absent_metadata_fields.csv` listant les champs physiques manquants.
- **Gate summary**: génération de `tests/integration_gate_summary.csv` pour bloquer les claims physiques si prérequis absents.

## 8) Commande Replit recommandée
```bash
bash src/advanced_calculations/quantum_problem_hubbard_hts/run_research_cycle.sh
```
Cette commande exécute désormais automatiquement les intégrations cycle 14 via:
`tools/post_run_cycle_guard.py`.
