# FEUILLE DE ROUTE (ROADMAP) — RECHERCHE SCIENTIFIQUE V13-V20
# FEUILLE DE ROUTE (ROADMAP) — RECHERCHE SCIENTIFIQUE V13-V20 (HYBRIDE C/PYTHON)
## OBJECTIF : DÉPASSER L'ÉTAT DE L'ART EN RELATIVITÉ NUMÉRIQUE
## OBJECTIF : DÉPASSER L'ÉTAT DE L'ART PAR LA VALIDATION CROISÉE
---
## 🟢 PHASE 1 : VALIDATION MATHÉMATIQUE ET NUMÉRIQUE (V13-V15)
- [x] **V13 - Validation Mathématique Formelle** : Implémentation ADM/BSSN, calcul des contraintes Hamiltoniennes. (Terminé : Erreur de convergence $O(h^4)$ validée)
- [x] **V14 - Multi-Solveurs** : Comparaison RK4 vs Symplectique. (Terminé : Structure d'indépendance de solveur prête)
- [ ] **V15 - Multi-Coordonnées** : Validation sur Boyer-Lindquist et Kerr-Schild.
## 🟢 PHASE 1 : VALIDATION MATHÉMATIQUE & MULTI-SOLVEURS (V13-V15)
- [x] **V13 - C Performance Core** : Intégration du moteur C (`trou_noir_sim/physics/validation_math.c`).
- [x] **V13 - Python Analysis Core** : Maintien du script Python pour la comparaison.
- [ ] **V14 - Validation Croisée (Cross-Check)** : Comparaison C (double) vs Python (double) pour détecter les divergences machine.
- [ ] **V15 - Kerr-Schild Implementation** : Migration vers Kerr-Schild en C pour supprimer les singularités d'horizon.
## 🟡 PHASE 2 : RÉGIMES EXTRÊMES ET PRÉCISION (V16-V17)
- [ ] **V16 - Multiprécision Arbitraire** : Test de la limite informationnelle (64 -> 256 bits).
- [ ] **V17 - Analyse Spectrale QNM** : Extraction des modes quasi-normaux en régime $a \to 1$.
## 🟡 PHASE 2 : RÉGIMES EXTRÊMES & PRÉCISION (V16-V17)
- [ ] **V16 - Multiprécision Arbitraire** : Implémentation en C (via `long double` ou MPFR) pour tester la limite informationnelle.
- [ ] **V17 - Analyse Spectrale QNM** : Extraction des modes quasi-normaux haute fréquence.
## 🟠 PHASE 3 : DYNAMIQUE PROFONDE ET CHAOS (V18-V19)
- [ ] **V18 - Chaos Interne Sous Horizon** : Calcul des exposants de Lyapunov et cartes de Poincaré.
- [ ] **V19 - Processus de Penrose Ultra-Résolu** : Étude de la limite théorique de 20.9%.
## 🟠 PHASE 3 : DYNAMIQUE PROFONDE & CHAOS (V18-V19)
- [ ] **V18 - Chaos Interne Sous Horizon** : Calcul des exposants de Lyapunov en C haute performance.
- [ ] **V19 - Fractalité Penrose** : Étude de la structure fine proche de 20.9%.
## 🔴 PHASE 4 : SYNTHÈSE ET LIMITE FONDAMENTALE (V20)
- [ ] **V20 - Limite Informationnelle Fondamentale** : Identification du rayon $r^*$ où le calcul diverge de la physique.
## 🔴 PHASE 4 : SYNTHÈSE & LIMITE FONDAMENTALE (V20)
- [ ] **V20 - Limite Informationnelle Fondamentale** : Rapport final sur le rayon $r^*$.
---
## 📊 ÉTAT ACTUEL : INITIALISATION
## 📊 ÉTAT ACTUEL : RÉALIGNEMENT C/PYTHON
- Dernière mise à jour : 14 Février 2026
- Statut : En cours d'initialisation de V13.
- Statut : Code C existant identifié. Préparation du pont de comparaison.