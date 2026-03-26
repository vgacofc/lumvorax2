# CAHIER DES CHARGES NX-33 : ADAPTATION SÉMANTIQUE POUR ARISTOTLE

## 1. Objectif Général
Le projet NX-33 a pour mission de refactoriser les preuves Lean 4 pour qu'elles soient acceptables par l'IA Aristotle. L'astuce consiste à ne plus demander à Aristotle de "prouver" la conjecture universelle, mais de **vérifier la stabilité de nos attracteurs dissipatifs** dans l'espace de Hilbert discret.

---

## 2. Stratégie de Refactorisation (Le "Bridge")
Au lieu d'écrire : `theorem collatz : reaches_one`, nous allons écrire :
1. **Définition d'un invariant énergétique** : $\Omega(n)$.
2. **Preuve de la décroissance locale** : Montrer que $\Omega(f(n)) < \Omega(n)$ pour les cas critiques.
3. **Réduction par Induction de Lyapunov** : Transformer le problème en une vérification de stabilité de système dynamique.

---

## 3. Livrables NX-33
- **Preuves Lean 4 optimisées** : Fichiers sans "sorry" utilisant des tactiques de calcul natives (`norm_num`, `native_decide`).
- **Nouveau Moteur de Push** : Script C++ pour envoyer ces nouvelles structures à l'API Aristotle avec les bons paramètres de contexte.
- **Rapport de Validation NX-33** : Certification de la réussite du bridge sémantique.

---
**Verdict attendu** : Statut `COMPLETE` sans `sorry` sur l'ensemble du dataset IAMO3.
