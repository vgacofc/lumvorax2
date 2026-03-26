AUTO-PROMPT — MODÈLE DE REQUÊTE POUR CE TYPE D'ANALYSE

# Analyse ciblée — obsolescence runtime benchmark et persistance inter-runs

## Constat utilisateur validé
Oui: si on ne fait qu’écrire une snapshot runtime d’un run donné sans l’injecter dans la chaîne d’évaluation, le run suivant peut rester comparé à une référence statique (`v2/v1`) et ignorer les données runtime.

## Cause racine
1. Les runners C chargeaient en dur:
   - `benchmarks/qmc_dmrg_reference_v2.csv`
   - `benchmarks/external_module_benchmarks_v1.csv`
2. Le refresh runtime existait côté pipeline, mais pas côté sélection de référence dans les runners.

## Correctif appliqué
- Ajout d’une sélection de benchmark avec priorité runtime et fallback canonique:
  - `qmc_dmrg_reference_runtime.csv` puis fallback `qmc_dmrg_reference_v2.csv`
  - `external_module_benchmarks_runtime.csv` puis fallback `external_module_benchmarks_v1.csv`
- Correctif appliqué dans les deux runners:
  - `hubbard_hts_research_cycle.c`
  - `hubbard_hts_research_cycle_advanced_parallel.c`

## Vérification "même problème ailleurs"
Oui, même défaut potentiel sur les références modules externes (`external_module_benchmarks_v1.csv`) — corrigé avec la même logique runtime-first.

## Réponse backup
Le backup de début de run n’est pas la source du bug: il archive, mais ne restaure pas les benchmarks. Le problème était la sélection statique côté runner.

## Questions expert additionnelles
1. Politique de promotion runtime -> canonique: automatique sous seuils, ou validation humaine obligatoire?
2. Faut-il garder un historique versionné des runtime refs par campagne?
3. Faut-il séparer stricte "référence littérature" et "référence calibration interne" dans les dashboards?

## Checklist finale
- [x] Cause racine validée
- [x] Correctif implémenté dans pipeline + runners
- [x] Vérification du problème similaire sur benchmark externe
