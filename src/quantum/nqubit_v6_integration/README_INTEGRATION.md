# NQubit V6 Integration for `src/quantum`

Ce dossier est une intégration opérationnelle de V6 dans le simulateur quantum local.

## Phases A → Z intégrées
A. collect_metrics  
B. acquire_hardware  
C. stress  
D. build_manifest  
E. verify_manifest  
F. generate_report  
G. comparaison ligne-par-ligne avec V6 source

## Commande finale exacte (Replit)
Depuis la racine du repo:

```bash
cd src/quantum/nqubit_v6_integration && make run_a_to_z compare_line_by_line test_all
```

Cette commande:
- exécute toutes les phases A→F,
- génère `results/a_to_z_run_summary.json`,
- génère la comparaison `results/line_by_line_comparison_v6.json` + `.md`,
- exécute les tests unitaires/intégration/stress.
