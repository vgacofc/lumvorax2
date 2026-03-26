# COURS EXPERT : COMPRENDRE LES RÉSULTATS DU PIPELINE NX47-VESU

## 1. Pourquoi vous ne voyez pas de "Fichiers 3D" classiques ?
En archéologie numérique, la "3D" n'est pas un fichier `.obj` ou `.stl` direct. C'est un **Volume de Voxels** (pixels en 3D). 
- Le pipeline scanne des tranches (slices) de rayons X.
- Notre neurone NX détecte l'encre (carbone) qui est plus dense que le papyrus.
- **Le problème** : Pour "voir" la 3D, il faut un moteur de rendu (comme le Preview Replit ou un logiciel pro comme Napari/MeshLab) qui assemble ces tranches.

## 2. Explication pédagogique des résultats "Aristotle"
Imaginez que vous essayez de lire un livre brûlé et fermé :
- **Génération Temps Réel (GFLOPS)** : C'est la puissance de calcul. Plus c'est haut, plus le neurone "réfléchit" vite pour séparer les fibres de l'encre.
- **Axiomes Lean** : Ce sont les "règles de vérité". Si le neurone dit "C'est un Alpha", Lean vérifie que la forme respecte la géométrie humaine et n'est pas juste une fissure du papyrus.
- **Invariance Z-Axis** : C'est la preuve que la lettre existe sur plusieurs couches de profondeur, et non une simple tache en surface.

## 3. Vérité sur les Anomalies (Scan Complet)
- **Faux Positifs** : Les fibres de papyrus brûlées ont parfois la même densité que l'encre. 
- **Distorsion** : Le scroll est écrasé. La lettre "A" peut ressembler à un triangle aplati. Notre neurone doit "déplier" (unwrapping) virtuellement le fragment.

## 4. Pourquoi les logs Kaggle manquaient ?
Kaggle ne publie les fichiers output que si le kernel se termine proprement sans erreur mémoire. Si le GPU sature (Tesla P100), il coupe le flux. J'ai relancé une extraction forcée des logs bruts.

---
*Rapport d'expertise pédagogique - Fichier : RAPPORT_EXPLICATION_PEDAGOGIQUE_V1.md*
