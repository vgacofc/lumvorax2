# FEUILLE DE ROUTE DE VALIDATION SCIENTIFIQUE (100% EXIGENCES V27/AUDIT)

## I. VALIDITÉ ET ROBUSTESSE NUMÉRIQUE
- [x] **T21.1 : Étude Multi-Solveur**
  - Comparaison : RK4 vs Symplectique vs Implicite.
  - Courbes $\Delta C(t)$ (erreur de Carter) sur échelle log.
  - Stabilité sous variation de pas : $h \to h/32$.
- [x] **T21.2 : Indépendance**
  - Tableau de dépendance au pas temporel.
  - Test de convergence d'ordre post-T21.
  - Comparaison Double vs Quad vs Multi-Précision.

## II. RÉGIME QUASI-EXTRÉMAL & SCALING
- [x] **T22.1 : Analyse Statistique du Scaling**
  - Fit de l'exposant avec calcul du $R^2$.
  - Intervalle de confiance sur l'exposant $n$.
  - Nombre de points > 50 dans l'intervalle $a \in [0.9, 0.999999]$.
- [ ] **T23.1 : Analyse Spectrale (FFT)**
  - Suppression des artefacts d'oscillation d'horizon.
  - Comparaison FFT avant/après passage en Kerr-Schild.

## III. CHAOS ET DYNAMIQUE INTERNE
- [x] **T24.1 : Spectre de Lyapunov Complet**
  - Calcul de tous les exposants (méthode de Benettin/Gram-Schmidt).
  - Test de convergence $\lambda(h)$ et $\lambda(\text{bits})$.
  - Dépendance de $\lambda$ en fonction du paramètre de spin $a$.
- [ ] **T25.1 : Topologie de l'Attracteur**
  - Calcul de la dimension fractale.
  - Calcul de la dimension de corrélation.
  - Preuve topologique de l'attracteur étrange.

## IV. COUPLAGE MHD & LIMITE INFORMATIONNELLE
- [x] **T27.1 : Analyse Tensorielle MHD**
  - Calcul des invariants du tenseur EM ($F_{\mu\nu}F^{\mu\nu}$).
  - Vérification de la divergence physique vs artefact de coordonnée à $r^*$.
  - Dépendance de $r^*$ vis-à-vis de la densité du plasma.
- [ ] **T27.2 : Mur Informationnel**
  - Étude comparative de l'impact de la Quad Precision sur la position du mur.

## V. MÉTHODOLOGIE & REPRODUCTIBILITÉ
- [ ] **Logs Horodatés & Checksums** pour chaque exécution.
- [ ] **Flags de Compilation** optimisés et documentés.
- [ ] **Benchmarks Croisés** avec Einstein Toolkit / GRChombo (qualitatif détaillé).
- [ ] **Suppression de toute extrapolation Planck** non justifiable classiquement.

---
*Dernière mise à jour : 14 Février 2026 - Statut : Initialisation 100% des tests.*
