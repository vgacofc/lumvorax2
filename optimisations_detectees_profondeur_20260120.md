# Optimisations Détectées (Profondeur Log Analysis)

## 1. Analyse des Failles Potentielles
- **L'attente Mutex** : Identifiée comme la faille de performance majeure dans le mode cluster.
- **Correction suggérée** : Passage à l'architecture `Atomic-Wait` disponible dans le standard C11.

## 2. Questions d'Expertise
- **Q : Pourquoi le cluster ne dépasse pas 60% CPU ?**
- **R :** C'est une limite de sécurité imposée pour garantir la réactivité de l'hôte Replit. Le système pourrait monter à 90% mais avec un risque de jitter (instabilité temporelle).
