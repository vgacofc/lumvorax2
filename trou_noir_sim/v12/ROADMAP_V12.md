# PROTOCOLE DE RECHERCHE V12 - VALIDATION SCIENTIFIQUE RIGOUREUSE

## 1. Questions des Experts (Audit V12)
1. **Q.V12-1 (Métrique)** : La formulation ADM est-elle strictement respectée lors du passage de l'horizon ?
2. **Q.V12-2 (Convergence)** : L'ordre de convergence réel $p$ est-il de 4.0 comme attendu pour RK4, ou le ratio de 32 cache-t-il une anomalie de maillage ?
3. **Q.V12-3 (Invariants)** : La constante de Carter $Q$ est-elle conservée à mieux de $10^{-12}$ en régime Quad Precision ?
4. **Q.V12-4 (Coordonnées)** : Le "tunneling" persiste-t-il en coordonnées de Kerr-Schild (régulières à l'horizon) ?

## 2. Feuille de Route d'Exécution
- [ ] **T1** : Étude de convergence multi-résolution (h, h/2, h/4, h/8).
- [ ] **T2** : Test de conservation des invariants (E, Lz, Carter) sur 10^6 pas.
- [ ] **T3** : Comparaison systématique Boyer-Lindquist vs Kerr-Schild.
- [ ] **T4** : Calcul des exposants de Lyapunov pour quantifier le chaos interne.

## 3. Découvertes Attendues
- Identification de la limite entre physique continue et discrétion numérique.
- Validation de la stabilité des orbites extrémales ($a = 0.99999M$).
