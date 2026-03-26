# PROTOCOLE DE VALIDATION SCIENTIFIQUE ET AMÉLIORATION PIPELINE NX47

## I. Pipeline de Validation Scientifique (Standards Nature/Science)

### I.1 Validation Physique (Voxel-wise)
- **Méthode** : Comparaison de densité entre les zones détectées et le support papyrus.
- **Seuil** : p-value < 0.01 exigée pour confirmer la présence de matière carbonée.

### I.2 Validation Morphologique (Graphématique)
- **Analyse** : Mesure de la régularité d'épaisseur et des angles préférentiels.
- **Distinction** : Séparation nette entre le chaos des craquelures et la contrainte de l'écriture humaine.

### I.3 Validation Z-Axis (Multi-couches)
- **Filtre** : Rejet systématique des détections n'apparaissant que sur une seule couche.
- **Cohérence** : Suivi de la déformation de la lettre à travers la profondeur du fragment.

## II. Processus de Confirmation de Lecture

### II.1 Statut des Résultats
- Visualisation → Lettres isolées → Mots plausibles → Phrases cohérentes.
- Le système refuse désormais le statut de "découverte" sans validation peer-reviewed.

### II.2 Reconstruction Contextuelle & Filtre Historique
- Intégration d'un dictionnaire de grec ancien pour calculer la probabilité d'existence des mots.
- Vérification de la cohérence temporelle et stylistique du vocabulaire.

## III. Optimisations du Pipeline (Anti Faux-Positifs)

### III.1 Apprentissage Négatif Massif
- Ajout explicite de textures de fibres, bords brûlés et fissures comme classes de rejet.

### III.2 Clustering de Glyphes
- Regroupement des signatures identiques ("Ces 7 glyphes sont le même Alpha").

### III.3 Human-in-the-loop Structuré
- Système de feedback binaire enregistré pour le rétro-apprentissage du modèle.
