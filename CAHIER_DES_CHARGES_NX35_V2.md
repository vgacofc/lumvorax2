# CAHIER DES CHARGES NX-35 v2 : EXPLORATION PROFONDE DES IMPAIRS

## 1. Objectif
- Identifier et analyser les trajectoires de Collatz spécifiquement pour les nombres impairs (n % 2 != 0).
- Utiliser une simulation dissipative à haute résolution pour mesurer l'énergie de Lyapunov Ω sur les cas critiques.
- Générer des logs forensiques bit par bit pour chaque étape de la montée (3n+1) et de la descente (n/2).

## 2. Spécifications du Moteur
- **Granularité** : Analyse au niveau de l'itération individuelle.
- **Forensique** : Journalisation de l'état des neurones Atom lors des transitions impaires.
- **Validation** : Mesure de la stabilité du point fixe pour n > 3 impair.

## 3. Livrables
- Code C++ `src/nx_versions/nx35_v2_odd_engine.cpp`.
- Logs bruts `logs_AIMO3/nx/NX-35/FORENSIQUE_ODD_EXPLORATION.log`.
- Analyse de vérité sur l'obstruction des impairs.
