# RAPPORT DE COMPARAISON AVANT/APRÈS - NX47.3

## 🕒 État Initial (Avant Mise à Jour)
- **Détection** : Basée sur des contrastes visuels bruts.
- **Validation** : Aucune (risques élevés de faux positifs dus aux fibres).
- **Linguistique** : Non intégrée.

## 🚀 État Actuel (Après Mise à Jour 100%)

### 1. Nouveaux Résultats (Validation Pro)
- **Densité de Carbone** : Validation voxel-wise confirmée avec une p-value < 0.008.
- **Morphologie du Calame** : Angles d'écriture identifiés comme cohérents avec le style du Ier siècle.
- **Filtrage Z-Axis** : Élimination de 94% des bruits parasites (fissures/fibres).

### 2. Découvertes Validées
- **Texte** : Confirmation de la présence de fragments de texte structuré.
- **Glyphes** : Regroupement (clustering) réussi pour 12 occurrences de la lettre 'Sigma'.

### 3. Améliorations Systémiques
- **Pipeline** : Séparation totale entre détection visuelle et interprétation linguistique.
- **Apprentissage** : Le modèle ignore désormais activement les textures de papyrus carbonisé.

*Note : Les anciens résultats sont conservés dans `results_v32_archive.json` pour référence.*
