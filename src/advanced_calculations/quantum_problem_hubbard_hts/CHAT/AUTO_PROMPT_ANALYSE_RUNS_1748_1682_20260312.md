```
AUTO-PROMPT — MODÈLE DE REQUÊTE POUR CE TYPE D'ANALYSE
(Inclus en tête de rapport conformément aux instructions. À réutiliser tel quel pour les cycles suivants.)

Lis toi-même ligne par ligne, fichier par fichier, sous-dossier par sous-dossier, chaque
CSV, JSON, log, MD et code source C des nouveaux runs ainsi que les anciens runs de référence
et TOUTES les analyses antérieures dans CHAT/ (ne jamais modifier les anciens fichiers) :

NOUVEAUX RUNS (à analyser en priorité) :
  src/advanced_calculations/quantum_problem_hubbard_hts/results/research_20260312T203411Z_1748/
  src/advanced_calculations/quantum_problem_hubbard_hts/results/research_20260312T202816Z_1682/

ANCIENS RUNS (référence, du plus récent au plus ancien) :
  src/advanced_calculations/quantum_problem_hubbard_hts/results/research_20260312T182101Z_2854/  ← moteur advanced_parallel
  src/advanced_calculations/quantum_problem_hubbard_hts/results/research_20260312T181131Z_2715/  ← moteur fullscale gelé
  src/advanced_calculations/quantum_problem_hubbard_hts/results/research_20260312T123420Z_1312/
  src/advanced_calculations/quantum_problem_hubbard_hts/results/research_20260312T075921Z_523/

ANALYSES PRÉCÉDENTES (ne JAMAIS modifier) :
  src/advanced_calculations/quantum_problem_hubbard_hts/CHAT/

CODE SOURCE C (auditer ligne par ligne) :
  src/advanced_calculations/quantum_problem_hubbard_hts/src/hubbard_hts_research_cycle_advanced_parallel.c
  src/advanced_calculations/quantum_problem_hubbard_hts/src/hubbard_hts_research_cycle.c

Sauvegarde le rapport dans CHAT/ sans modifier aucun fichier existant.

Le rapport doit couvrir :
1. INVENTAIRE des fichiers nouveaux/disparus entre chaque run
2. IDENTIFICATION du moteur utilisé (fullscale vs advanced_parallel) — CRITIQUE
3. TABLEAU COMPARATIF ligne par ligne : énergie, pairing, drift, signe, benchmark
4. DÉTECTION DES VALEURS GELÉES / HARDCODÉES — chercher si énergie identique step 100 à 2700
5. AUDIT CODE C ligne par ligne : localiser la présence ou absence de :
   a. Centering correction : n_up*n_dn - 0.25
   b. Renormalisation conditionnelle sites <= 256
   c. feedback_gain fixe vs adaptatif
   d. tanh vs autre fonction de bornage
6. INVENTAIRE DE CHAQUE ERREUR RESTANTE : fichier, ligne exacte avant/après, cause, solution
7. VALIDATION des corrections recommandées dans CHAT/ antérieurs — appliquées ou pas ?
8. POINTS INÉDITS détectés par toi-même que les analyses précédentes n'avaient pas signalés
9. TABLEAU BILAN final : score par dimension, FAILs restants, priorités
10. FEUILLE DE ROUTE corrections prioritaires avec fichier et numéro de ligne exact
```
