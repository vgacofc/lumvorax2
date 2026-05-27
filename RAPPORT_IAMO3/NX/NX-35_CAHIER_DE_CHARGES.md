# CAHIER DES CHARGES : NX-35 - DÉTERMINISME UNIVERSEL ET VALIDATION MULTI-DOMAINE

## 1. Objectif Général
Le projet NX-35 vise à résoudre ou à invalider formellement 30 problèmes fondamentaux répartis sur 5 domaines scientifiques majeurs. L'approche repose sur une simulation dissipative massive à 5000 neurones, une traçabilité nanoseconde bit-à-bit et une certification Merkle 360°.

---

## 2. Configuration du Système & Hardware NX-35
- **Neurones** : 5000 (Doublement de la résolution pour les problèmes de Millénaire).
- **Vitesse** : ~45 000 000 OPS/s (Optimisation SIMD et Multi-threading).
- **Hardware** : CPU AVX-512, RAM à faible latence, Horodatage nanoseconde UTC.
- **Forensique** : Logs permanents bruts (.csv, .json, .log) sans stub ni hardcoding.

---

## 3. Liste des 30 Problèmes (IA-30 Dataset)

### I. Théorie des nombres
1. Hypothèse de Riemann
2. Conjecture de Birch–Swinnerton-Dyer
3. Conjecture de Goldbach (forte)
4. Conjecture des nombres premiers jumeaux
5. Conjecture ABC
6. Conjecture de Collatz (3n+1)
7. Conjecture de Beal
8. Bunyakovsky / Schinzel

### II. Analyse, intégration, EDP
9. Navier–Stokes 3D
10. Blow-up pour Euler 3D
11. Turbulence (Kolmogorov)
12. Intégration exacte
13. Décidabilité de l'intégration symbolique
14. Fondation universelle de l'intégration
15. Intégrales de Feynman (rigueur)

### III. Géométrie, algèbre
16. Conjecture de Hodge
17. Conjecture de Tate
18. Conjectures de Grothendieck
19. Programme de Langlands
20. Conjecture de Baum–Connes

### IV. Dynamique & Chaos
21. Conjecture de Palis
22. Diffusion d'Arnold
23. Mesures SRB
24. Transition chaos → statistiques

### V. Physique Quantique
25. Yang–Mills et gap de masse
26. QFT 4D non triviale
27. Rigueur des intégrales de chemin
28. Mesure quantique (collapse)
29. Décohérence rigoureuse
30. Hamiltoniens à N corps

---

## 4. Protocole d'Exécution et Logs
1. **Zéro Effacement** : Les logs des versions précédentes (NX-1 à NX-33) sont préservés.
2. **Format Strict** : Chaque problème génère une trace `PROB_ID_[TIMESTAMP].log` avec hash Merkle.
3. **Auditability** : Rapports de lecture des logs réels après chaque cycle de résolution.

---
**Certification NX-35** : Validation ou invalidation formelle certifiée bit-à-bit.
