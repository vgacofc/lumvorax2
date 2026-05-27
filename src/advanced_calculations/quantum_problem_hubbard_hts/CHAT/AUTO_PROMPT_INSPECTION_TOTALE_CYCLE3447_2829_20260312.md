# AUTO-PROMPT — INSPECTION TOTALE REPLIT (CYCLES 3447/2829)

```text
Objectif: audit A→Z reproductible, sans toucher aux rapports précédents, avec correction immédiate des causes racines confirmées.

PHASE 1 — SYNCHRONISATION
1) git fetch --prune https://github.com/lumc01/Lumvorax.git
2) Vérifier divergence locale/distante:
   git rev-list --left-right --count origin/main...HEAD
3) Refuser tout ajout de binaires dans le patch.

PHASE 2 — SOURCES À LIRE (ordre strict)
1) CHAT/RAPPORT_ANALYSE_TOTALE_2854_2715_vs_1312_523_ERREURS_CAUSES_SOLUTIONS.md
2) CHAT/RAPPORT_ANALYSE_REPLIT_3447_2829_CORRECTIFS_20260312.md
3) Résultats:
   - results/research_20260312T222246Z_2829
   - results/research_20260312T222838Z_3447

PHASE 3 — CONTRÔLES D’INTÉGRITÉ
1) Présence artefacts critiques (logs/tests/reports/audit/scientific_diagnostics).
2) Vérification checksums:
   cd results/research_20260312T222838Z_3447 && sha256sum -c logs/checksums.sha256
3) Signaler tout fichier référencé mais absent comme FAIL de traçabilité.

PHASE 4 — INSPECTION CODE LIGNE PAR LIGNE
Cibler en priorité:
- src/hubbard_hts_research_cycle.c
- src/hubbard_hts_research_cycle_advanced_parallel.c
- scripts post-run de garde/intégrité.

Chercher systématiquement:
- incohérences d’unités (energy vs energy_meV)
- seuils asymétriques entre runners
- hardcodes invalidants
- traces/logs supprimés mais encore référencés
- warnings compilation

PHASE 5 — VÉRIFICATION SCIENTIFIQUE
Comparer 2829 vs 3447:
- energy / pairing / sign_ratio
- stabilité numérique
- benchmark QMC/DMRG
- corrélations/pairing/pseudogap/artefact numérique

PHASE 6 — FORMAT DE RÉPONSE POINT PAR POINT
Pour chaque point:
- Question
- Analyse
- Réponse
- Solution

PHASE 7 — CORRECTIFS IMMÉDIATS
- Corriger uniquement causes confirmées.
- Garder patch minimal, traçable, compilable sans warning.
- Ne jamais modifier les anciens rapports.

PHASE 8 — VALIDATION FINALE
1) make -C src/advanced_calculations/quantum_problem_hubbard_hts clean all
2) bash src/advanced_calculations/quantum_problem_hubbard_hts/run_research_cycle.sh
3) Archiver rapport final + commande exacte de reproduction.
```
