# RAPPORT 126 — CAHIER DES CHARGES ET ROADMAP COMPLÈTE
## Projet : Solveur Navier-Stokes Réel LumVorax — Vers une Solution Convergente

**Date :** 2026-10-01T17:30:00Z
**Auteur :** Agent LumVorax — Audit autonome (Bob IDE)
**Dépôt :** LumVorax2 — séquence interne LumVorax (indépendante d'ARTCB)
**Basé sur :** Rapport 125 (corrections identifiées) + Rapport R551 (comparaison OpenAI)
**CERTIFIED_100=false | Mode DEBUG actif**

---

## EXPERTISES ACTIVÉES

1. Analyse numérique et méthodes numériques pour EDP (Équations aux Dérivées Partielles)
2. Physique des fluides computationnelle — CFD (Computational Fluid Dynamics)
3. Ingénierie logicielle C natif — SIMD, AVX-512, mesure de performance
4. Mathématiques appliquées — Navier-Stokes, invariants de Lyapunov, stabilité
5. Architecture logicielle — séparation de modules, pipeline de validation
6. Traçabilité forensique — SHA-256, horodatage nanoseconde, logs immuables
7. Gestion de projet — Cahier des charges, Roadmap, jalons mesurables

---

## PARTIE 1 — CAHIER DES CHARGES

### 1.1 Contexte et objectif

Le projet LumVorax a pour ambition de développer une solution computationnelle pour les équations de Navier-Stokes. L'audit du 2026-10-01 (rapport 125) a démontré que le code actuel ne contient pas de vraies équations de Navier-Stokes mais une simulation physique générique dissipative. Ce cahier des charges définit exactement ce qu'il faut construire, comment le valider, et dans quel ordre.

L'objectif final est de converger vers une solution numérique rigoureuse de Navier-Stokes incompressible en 2D (première étape) puis en 3D (deuxième étape), avec des métriques mesurées instrumentalement (pas hardcodées), une traçabilité forensique SHA-256 ligne par ligne, et une comparaison honnête avec les solutions existantes (FNO OpenAI, OpenFOAM).

C'est-à-dire : à la fin de ce projet, quelqu'un pourra lancer le programme, voir un fluide simulé numériquement selon les vraies équations physiques, mesurer des temps de calcul réels, et comparer les résultats avec des benchmarks publiés par la communauté scientifique.

---

### 1.2 Périmètre — Ce qui est INCLUS dans ce projet

MODULE 1 — Solveur Navier-Stokes 2D incompressible en C pur
Fichier à créer : src/solvers/ns_solver_2d.c et src/solvers/ns_solver_2d.h
Description : implémentation complète des équations de Navier-Stokes incompressibles sur une grille 2D carrée par différences finies. Inclut : champs de vitesse u(x,y) et v(x,y), champ de pression p(x,y), terme advectif, terme de diffusion visqueuse, solveur de Poisson pour la pression, projection pour l'incompressibilité.
Paramètres configurables : taille de grille N (64, 128, 256), viscosité ν, pas de temps dt, nombre d'itérations.
Conditions aux limites : cavité carrée avec couvercle mobile (benchmark "Lid-Driven Cavity" — le plus utilisé dans la littérature internationale).

MODULE 2 — Mesures de performance réelles instrumentées
Fichier à modifier : src/tests/nx42_30_problems_execution.c
Description : remplacer les latences hardcodées (lignes 24-28) par des mesures réelles avec clock_gettime(CLOCK_MONOTONIC). Chaque appel au solveur est encadré par une prise de temps avant et après.

MODULE 3 — Calcul d'exposant de Lyapunov documenté
Fichier à créer : src/analysis/lyapunov_calculator.c et src/analysis/lyapunov_calculator.h
Description : calculer l'exposant de Lyapunov de façon standard (méthode de Benettin, 1980) et le documenter avec la convention explicite. Écrire la valeur dans le log avec un champ "convention" qui indique si négatif = stable ou positif = stable.

MODULE 4 — Système de validation et logs forensiques
Fichier à créer : src/validation/ns_validator.c
Description : comparer les résultats du solveur LumVorax avec les valeurs de référence publiées pour le benchmark Lid-Driven Cavity (données de Ghia et al., 1982 — disponibles librement). Calculer l'erreur L2 (c'est-à-dire : la différence quadratique entre nos résultats et les valeurs de référence) et l'écrire dans le log forensique avec SHA-256.

MODULE 5 — Traçabilité forensique complète
Fichier à modifier : src/sch/nx/sch_nx_v11.c
Description : brancher le calcul de l'exposant de Lyapunov réel sur les données du solveur NS, corriger le signe si nécessaire, et écrire les logs avec la convention explicite documentée.

---

### 1.3 Périmètre — Ce qui est EXCLU de ce projet

- La résolution du problème du Millénaire Navier-Stokes (Clay Institute). Ce n'est pas l'objectif de ce cahier des charges. L'objectif est une simulation numérique correcte et honnête.
- L'implémentation 3D (reportée à la Phase 3 de la roadmap).
- L'intégration de FFTW3 (méthode spectrale). La méthode des différences finies suffit pour la Phase 1.
- Tout code lié au projet ARTCB. Les deux projets restent strictement séparés.

---

### 1.4 Critères d'acceptation (comment savoir que c'est réussi)

CRITÈRE C1 — Solveur fonctionnel : le solveur NS 2D tourne sans crash sur une grille 64×64 pendant 1000 itérations.

CRITÈRE C2 — Résultats corrects : le profil de vitesse U au centre de la cavité (Lid-Driven Cavity, Re=100) correspond aux données de référence Ghia 1982 avec une erreur L2 inférieure à 5%. C'est-à-dire : nos résultats sont proches des résultats publiés par des mathématiciens reconnus.

CRITÈRE C3 — Mesures instrumentées : les latences affichées dans les logs varient entre les exécutions (ce qui prouve qu'elles sont mesurées réellement, pas hardcodées). Un test de variabilité vérifie que deux exécutions consécutives donnent des latences différentes.

CRITÈRE C4 — Lyapunov documenté : le log NX35_LOG_P9.csc contient un champ "lyapunov_convention" avec une valeur explicite.

CRITÈRE C5 — Log forensique : chaque ligne du log de validation est hachée SHA-256 et les hashes sont vérifiables.

CRITÈRE C6 — Isolation totale : `git -C [dépôt_artcb] check-ignore -v "LVX&ARTCB/"` retourne une correspondance valide. Aucun fichier LumVorax dans `git status` du dépôt ARTCB.

---

### 1.5 Contraintes techniques obligatoires

- Langage : C99 ou C11 uniquement pour les solveurs (conformément au protocole LumVorax)
- Compilation : gcc -O2 -lm — pas de dépendances externes en Phase 1
- Mode DEBUG : toujours actif tant que l'utilisateur ne demande pas de désactivation
- Logs : format JSON structuré dans logs_AIMO3/NX/NX-35/ pour les données de validation
- Rapports : numérotation séquentielle LumVorax (127, 128...) — jamais la numérotation ARTCB
- Aucun fichier ARTCB dans LumVorax, aucun fichier LumVorax dans ARTCB
- Jamais hardcoder une valeur de performance — toujours mesurer

---

## PARTIE 2 — ROADMAP COMPLÈTE

### PHASE 0 — Assainissement (Durée estimée : 1 session)

Objectif : corriger les anomalies critiques identifiées dans le rapport 125 sans implémenter de nouvelle fonctionnalité.

Tâche P0-T1 : Modifier src/tests/nx42_30_problems_execution.c — remplacer les 5 appels log_problem() avec latences hardcodées (lignes 24-28) par des mesures clock_gettime() encadrant un appel à nx11_physics() réel.
Résultat attendu : les latences dans le log varient entre exécutions.
Fichier de log : logs_AIMO3/NX/NX-42/NX42_INSTRUMENTED_RUN.json (nouveau fichier)

Tâche P0-T2 : Modifier logs_AIMO3/NX/NX-35/NX35_LOG_P9.csc — ajouter la colonne "lyapunov_convention" avec la valeur "negative_is_stable_standard" ou corriger le signe du calcul dans le code source qui génère ce fichier.
Résultat attendu : la valeur de Lyapunov est cohérente avec la conclusion "STABLE" déclarée.

Tâche P0-T3 : Créer le fichier src/tests/nx42_30_problems_execution_v2.c avec les 5 premiers problèmes instrumentés réellement. Ne pas modifier l'original (respecter le protocole "jamais effacer").

Jalon P0 validé quand : critères C3 et C4 passent.

---

### PHASE 1 — Solveur Navier-Stokes 2D réel (Durée estimée : 3 à 5 sessions)

Objectif : implémenter un vrai solveur NS incompressible 2D en C pur, le valider contre le benchmark Lid-Driven Cavity de Ghia 1982.

Tâche P1-T1 : Créer src/solvers/ns_solver_2d.h — définir les structures de données (grille, champs de vitesse, pression, paramètres).

Tâche P1-T2 : Créer src/solvers/ns_solver_2d.c — implémenter les fonctions :
- ns_grid_init() : allouer la grille et initialiser les champs à zéro
- ns_step_advection() : calculer le terme u·∇u par différences finies upwind
- ns_step_diffusion() : calculer le terme ν∇²u par Laplacien centré
- ns_solve_pressure() : résoudre ∇²p = ∇·u* par itérations Gauss-Seidel (50 itérations)
- ns_project_velocity() : corriger u = u* − ∇p pour forcer ∇·u = 0
- ns_step() : appeler les 4 fonctions précédentes dans l'ordre correct
- ns_grid_destroy() : libérer la mémoire proprement

Tâche P1-T3 : Créer src/tests/test_ns_solver_lid_driven.c — test benchmark Lid-Driven Cavity à Re=100 (Reynolds = U*L/ν = 1.0 * 1.0 / 0.01 = 100) sur grille 64×64, 5000 itérations.

Tâche P1-T4 : Créer src/validation/ns_validator.c — charger les données de référence Ghia 1982 (le profil de vitesse U au centre de la cavité sur 17 points), calculer l'erreur L2 et écrire le résultat dans le log forensique.

Tâche P1-T5 : Créer logs_AIMO3/NX/NX-35/NX35_LOG_P9_NS_REAL_V1.json — log complet de la première exécution réelle du solveur NS, avec timestamp nanoseconde, erreur L2, exposant de Lyapunov calculé, hash SHA-256 de chaque ligne.

Jalon P1 validé quand : critères C1, C2, C3 passent.

---

### PHASE 2 — Intégration dans le pipeline NX et traçabilité complète (Durée estimée : 2 sessions)

Objectif : brancher le solveur NS réel sur le moteur NX-11 existant, remplacer la physique brownienne générique par le solveur NS réel, et produire des logs forensiques complets.

Tâche P2-T1 : Modifier src/sch/nx/sch_nx_v11.c — remplacer la fonction nx11_physics() (lignes 46-52) par un appel à ns_step() du solveur NS. La dissipation d'énergie (atp -= 2.0) devient la dissipation réelle calculée depuis l'énergie cinétique du fluide (intégrale de u² sur le domaine).

Tâche P2-T2 : Créer src/analysis/lyapunov_calculator.c — implémenter le calcul d'exposant de Lyapunov selon la méthode de Benettin : lancer deux trajectoires avec une perturbation initiale epsilon=1e-8, mesurer leur divergence après N pas de temps, calculer λ = (1/T) * ln(|δ(T)/δ(0)|). Signe négatif = stable. Documenter la convention dans le log.

Tâche P2-T3 : Mettre à jour logs_AIMO3/NX/NX-35/NX35_LOG_P9.csc — régénérer après la correction avec la convention explicite documentée.

Tâche P2-T4 : Mettre à jour src/tests/nx42_30_problems_execution.c — le problème 5 "Navier-Stokes Dissipation" appelle désormais le vrai solveur NS et mesure le temps réel.

Jalon P2 validé quand : critères C1, C2, C3, C4, C5 passent.

---

### PHASE 3 — Extension 3D et optimisation SIMD (Durée estimée : 5 à 8 sessions)

Objectif : étendre le solveur à 3D, exploiter les instructions SIMD/AVX-512 déjà présentes dans l'architecture LumVorax pour accélérer les calculs, et comparer les performances avec FNO (OpenAI) sur des benchmarks publics.

Tâche P3-T1 : Créer src/solvers/ns_solver_3d.c — extension du solveur 2D à une grille 3D N×N×N (N=32 comme départ, soit 32768 cellules).

Tâche P3-T2 : Optimiser ns_solver_2d.c avec intrinsèques SIMD (AVX-256 ou AVX-512) pour les boucles de calcul du Laplacien et de l'advection. C'est-à-dire : traiter 4 ou 8 cellules en parallèle dans une seule instruction CPU, réduisant le temps de calcul d'un facteur 4 à 8.

Tâche P3-T3 : Benchmark comparatif — mesurer les performances du solveur LumVorax sur des cas test standardisés (Taylor-Green Vortex, canal turbulent) et comparer avec les résultats publiés pour FNO (Fourier Neural Operator, Li et al. 2021, disponible sur arXiv:2010.08895).

Tâche P3-T4 : Produire le rapport comparatif 3D — avec les métriques réelles mesurées (pas hardcodées), l'erreur L2 par rapport aux références, et la conclusion honnête sur ce que LumVorax fait mieux ou moins bien que FNO.

Jalon P3 validé quand : critère C2 passe à < 2% d'erreur L2 sur Taylor-Green Vortex.

---

### PHASE 4 — Formalisation mathématique partielle en Lean 4 (Durée estimée : 4 à 6 sessions)

Objectif : formaliser dans Lean 4 les propriétés mathématiques que le solveur numérique a vérifiées computationnellement. Ce n'est pas une résolution du problème du Millénaire, mais c'est une contribution honnête à la formalisation.

Tâche P4-T1 : Créer src/lean/ns_dissipation_lemma.lean — formaliser le lemme que l'énergie cinétique totale décroît au cours du temps pour des conditions aux limites de type cavité (propriété de dissipation, prouvable formellement pour le cas simplifié).

Tâche P4-T2 : Créer src/lean/ns_stability_theorem.lean — formaliser la stabilité locale autour de l'état stationnaire (Re=100, Lid-Driven Cavity), étayée par les résultats numériques.

Tâche P4-T3 : Mettre à jour les fichiers Lean existants (nx35_v1.lean, nx36_pure_core_final.lean) pour référencer les nouveaux lemmes.

Jalon P4 validé quand : `lake build` compile sans erreurs les fichiers Lean de Phase 4.

---

## PARTIE 3 — MATRICE DE TRAÇABILITÉ

| Correction | Rapport | Tâche Roadmap | Critère | Fichier à créer/modifier |
|-----------|---------|--------------|---------|--------------------------|
| C1 — Latences hardcodées | 125 | P0-T1, P0-T3 | C3 | nx42_30_problems_execution_v2.c |
| C2 — Lyapunov positif | 125 | P0-T2, P2-T2, P2-T3 | C4 | lyapunov_calculator.c, NX35_LOG_P9.csc |
| C3 — Pas d'EDP réelles | 125 | P1-T1 à P1-T4 | C1, C2 | ns_solver_2d.c, ns_solver_2d.h |
| C4 — 25 placeholders | 125 | P2-T4, P3-T3 | C3 | nx42_30_problems_execution.c |
| C5 — Numérotation mixte | 125 | Règle permanente | — | Tous nouveaux rapports LumVorax |

---

## PARTIE 4 — DONNÉES DE RÉFÉRENCE DISPONIBLES EN LIGNE

Ces données sont publiques et peuvent être utilisées directement pour valider les résultats :

Benchmark Lid-Driven Cavity (Ghia et al., 1982) : profil de vitesse U au centre vertical de la cavité pour Re=100, 400, 1000, 3200. Valeurs disponibles dans le papier "High-Re solutions for incompressible flow using the Navier-Stokes equations and a multigrid method", Journal of Computational Physics, Vol. 48, pp. 387-411.

FNO (Fourier Neural Operator) résultats de benchmark : disponibles sur GitHub à https://github.com/neuraloperator/neuraloperator avec les erreurs L2 pour Navier-Stokes 2D turbulent à différentes viscosités.

Taylor-Green Vortex (données DNS directe) : disponibles sur le site du Johns Hopkins Turbulence Database à http://turbulence.pha.jhu.edu — données de référence pour valider des solveurs 3D.

---

## PARTIE 5 — QUESTIONS OUVERTES (à clarifier avec l'utilisateur)

Q1 : La Phase 3 (3D + SIMD) est-elle une priorité avant la Phase 4 (Lean 4), ou l'utilisateur préfère-t-il d'abord le 2D rigoreux puis la formalisation ?

Q2 : Le projet LumVorax vise-t-il à soumettre des résultats à une compétition (ex : AIMO3) ou à produire une publication scientifique ? Les critères d'acceptation seraient différents.

Q3 : Souhaite-on intégrer FFTW3 en Phase 2 pour la méthode spectrale (plus précise) ou rester en différences finies (plus simple et sans dépendances) ?

Q4 : Les 25 problèmes restants (6 à 30) doivent-ils tous être des problèmes du Millénaire ou d'autres catégories (optimisation, cryptographie, physique quantique) ?

---

**CERTIFIED_100=false | unique_human_proven=false | Mode DEBUG actif**
**Prochain rapport LumVorax : 127 — Implémentation Phase 0 (corrections urgentes)**
**Aucun fichier de ce rapport ne doit se retrouver dans le dépôt ARTCB**
