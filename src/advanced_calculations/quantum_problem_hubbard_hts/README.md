# Module isolé — Hubbard HTS (supraconductivité haute température)

Ce dossier est **indépendant** des modules existants et traite des campagnes Hubbard HTS + proxys, avec traçabilité forte.

## Exécutions disponibles

### 1) Campagne de base (historique conservé)
```bash
bash src/advanced_calculations/quantum_problem_hubbard_hts/run_hubbard_hts.sh 99 99
```

### 2) Nouveau cycle recherche structuré (analyse experte + nouveaux tests)
```bash
bash src/advanced_calculations/quantum_problem_hubbard_hts/run_research_cycle.sh
```

## Garanties
- Dossiers de run uniques, horodatés UTC (`research_YYYYMMDDTHHMMSSZ_pid`).
- Aucun écrasement des anciens logs/rapports.
- Sauvegarde de version avant exécution (`backups/...`).
- Logs détaillés + données brutes + checksums SHA256 + versions environnement.

## Structure d'un run recherche
- `results/research_<...>/logs/research_execution.log`
- `results/research_<...>/logs/baseline_reanalysis_metrics.csv`
- `results/research_<...>/tests/new_tests_results.csv`
- `results/research_<...>/tests/expert_questions_matrix.csv`
- `results/research_<...>/reports/RAPPORT_RECHERCHE_CYCLE_03_ADVANCED.md`
- `results/research_<...>/logs/checksums.sha256`
- `results/research_<...>/logs/environment_versions.log`

## Objectif scientifique du cycle recherche
1. Explication pédagogique des résultats.
2. Questions d'experts + état des réponses.
3. Détection d'anomalies / incohérences / signaux potentiels.
4. Comparaison qualitative à la littérature.
5. Nouveaux tests (reproductibilité, convergence, sensibilité, stress).
6. Rapport indépendant sans toucher aux anciens artefacts.
7. Répétition itérative à chaque nouveau cycle.


## Objectif v4 ultra-avancé
Le cycle v3 ajoute une pile multi-méthodes avancée: proxy corrélé, vérification indépendante long double, solveur exact 2x2 Hubbard, puis scoring audité sur 6 axes.

## Intégrations automatiques (nouveau)
Après chaque `run_research_cycle.sh`, le pipeline génère automatiquement:
- `tests/integration_terms_glossary.csv` (explications non-expert),
- `tests/integration_claim_confidence_tags.csv` (certain/probable/unknown),
- `tests/integration_absent_metadata_fields.csv` (métadonnées physiques manquantes),
- `tests/integration_manifest_check.csv` (complétude modules/steps),
- `tests/integration_run_drift_monitor.csv` (dérive run-to-run),
- `tests/integration_gate_summary.csv` (gates de validation).

Commande Replit à exécuter:
```bash
bash src/advanced_calculations/quantum_problem_hubbard_hts/run_research_cycle.sh
```
