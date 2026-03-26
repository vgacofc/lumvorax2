# AUTO-PROMPT — INSPECTION TOTALE REPLIT (CYCLES 2854/2715)

```text
Objectif: inspection A→Z reproductible sans modifier les anciens rapports.

1) Synchroniser le dépôt (sans binaires):
   git fetch --no-tags https://github.com/lumc01/Lumvorax.git

2) Lire d'abord:
   src/advanced_calculations/quantum_problem_hubbard_hts/CHAT/RAPPORT_ANALYSE_TOTALE_2854_2715_vs_1312_523_ERREURS_CAUSES_SOLUTIONS.md

3) Comparer les runs:
   - src/advanced_calculations/quantum_problem_hubbard_hts/results/research_20260312T182101Z_2854
   - src/advanced_calculations/quantum_problem_hubbard_hts/results/research_20260312T181131Z_2715
   - références: ..._1312 et ..._523

4) Vérifier intégrité:
   - schéma CSV (colonnes constantes)
   - checksums SHA256
   - présence de tous artefacts tests/logs/reports

5) Audit code ligne par ligne des .c/.h/.cpp/.py/.sh:
   - identifier causes exactes, ligne et impact
   - détecter bug caché (format, overflow, hardcode, arrêt pipeline)

6) Appliquer correctifs immédiats si bug bloquant:
   - patch minimal, traçable
   - compilation stricte sans warnings

7) Rejouer validations:
   - make clean all
   - tests post-run ciblés (forensic/integrator/schema)

8) Produire un nouveau rapport dans CHAT/ (sans toucher aux anciens):
   - avant/après ligne modifiée
   - commande exacte de reproduction Replit
   - risques restants + priorisation
```
