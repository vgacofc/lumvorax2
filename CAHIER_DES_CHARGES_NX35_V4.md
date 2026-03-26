# CAHIER DES CHARGES NX-35 V4 : FORMALISATION ET CERTIFICATION

## 1. Objectif
- Unifier les résultats du scan massif (1,6 Milliard de points) avec la preuve formelle Lean 4.
- Produire le code Lean 4 certifié sans "sorry" en utilisant le Lemme de Dissipation Ω validé par les moteurs Extreme et V32.
- Préparer le rapport final de preuve pour soumission à Aristotle.

## 2. Spécifications Techniques
- **Moteur de Preuve** : Lean 4 (version 4.x).
- **Données d'Entrée** : Résultats des scans NX-35 v2 (Extreme) et V32 (Ultra).
- **Axiomatisation** : Formalisation de la décroissance de Lyapunov basée sur les logs réels.

## 3. Livrables
- `src/proofs/collatz_v44_certified.lean` : Version finale rectifiée.
- `RAPPORT_FINAL_NX35_V4.md` : Preuve point par point.
- Statistiques de logs : Décompte exact des lignes générées.
