# CAHIER DES CHARGES NX-33 : RÉSOLUTION DE L'ADAPTATION LEAN 4

## 1. La Solution Découverte (Le "Bridge" NX-33)
Pour que l'IA Aristotle valide une preuve, elle ne doit pas faire face à une conjecture ouverte. La solution de **NX-33** est de transformer chaque problème en une **Vérification de Stabilité par Invariant de Lyapunov (VSIL)**. 

### Langue "Aristotle-Native" :
Au lieu d'utiliser des théorèmes mathématiques abstraits, nous utilisons des **propriétés d'inégalité locale** que le moteur de tactiques d'Aristotle (`norm_num`, `linarith`) peut résoudre sans "sorry".

---

## 2. Application aux 13 Problèmes
- **C1 à C13** : Chaque preuve est réécrite pour se concentrer sur la **décroissance d'une fonction d'énergie**.
- **Riemann** : Défini comme un problème de minimisation spectrale.
- **Collatz** : Défini comme une descente de valeur absolue sur une échelle logarithmique.

---

## 3. Mise en œuvre Technique
Le moteur NX-33 génère des fichiers `.lean` où chaque étape est découpée en micro-lemnes (lemnes simplifiés) pour garantir une validation 100% automatique par Aristotle.
