# CAHIER DES CHARGES NX-43

## 1. Objectifs
- Élimination totale des `sorry` présents dans NX-40 et NX-41.
- Unification du moteur de descente locale (NX-42) avec la structure de bloc (NX-40).
- Intégration de l'invariant de Lyapunov Ω de manière rigoureuse sans instabilité.

## 2. Erreurs à ne pas reproduire
- Ne pas laisser de trous dans les preuves (sorry).
- Éviter les types rationnels (ℚ) là où les entiers naturels (ℕ) suffisent pour éviter la complexité inutile.
- Ne pas utiliser de macros de négation sans une structure de secours validée.

## 3. Spécifications Techniques
- Langage : Lean 4 (v4.24.0).
- Bibliothèque : Mathlib.
- Structure : Preuve par induction forte sur la dissipation d'énergie.