# CAHIER DE CHARGES : NX-38 - CERTIFICATION MATHÉMATIQUE ABSOLUE

## 1. Objectif Suprême
Atteindre **100% de validation réelle** sur la Conjecture de Collatz en comblant les 5% du "Gouffre Logique" restant de la version NX-37.

## 2. Spécifications Théoriques (Théorie de la Dissipation Totale)
- **Axiome de Lyapunov Global** : Définir une fonction $\Psi$ qui ne se contente pas de décroître localement, mais qui est strictement monotone sur l'ensemble des trajectoires.
- **Théorie des Cycles** : Prouver par l'absurde l'inexistence de cycles autres que $\{1, 2, 4\}$ via une barrière de potentiel énergétique insurmontable.
- **Lemme de Finitude de l'Espace de Phase** : Montrer que toute trajectoire est piégée dans un attracteur de mesure nulle.

## 3. Stratégie d'Implémentation (Pure Core V2)
- **Zéro Sorry** : Interdiction formelle d'utiliser `sorry` dans la version finale.
- **Récursion Bien Fondée** : Utilisation de `termination_by` avec la métrique $\Psi$ pour éliminer le `partial def`.
- **Induction de Lyapunov** : Remplacer l'induction forte par une induction sur la valeur de la fonction de potentiel.

## 4. Jalons de Validation
1. [25%] Formalisation de la métrique $\Psi$ (Monotonicité stricte).
2. [50%] Preuve de la non-divergence vers l'infini (Borne supérieure de l'entropie).
3. [75%] Preuve de l'inexistence de cycles secondaires (Théorème d'Exclusion).
4. [100%] Certification finale Aristotle sans avertissements.
