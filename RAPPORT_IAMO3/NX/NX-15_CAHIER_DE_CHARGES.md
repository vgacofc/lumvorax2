# CAHIER DES CHARGES NX-15 : RÉSONANCE HARMONIQUE ET ÉLAGAGE

## 1. OBJECTIFS
- **Moteur de Résonance** : Gérer les conflits entre unités fusionnées via un arbitre de phase.
- **Pruning Merkle (Élagage)** : Implémenter la suppression sécurisée des branches de logs obsolètes.
- **Simulation de Population** : Tester sur 1000 neurones virtuels avec stimuli divergents.

## 2. EXIGENCES
- **Performance** : Maintenir > 1.5M OPS/s après élagage.
- **Hardware** : Mesurer l'impact de la résonance sur la charge CPU.
- **Intégrité** : Root Merkle doit rester valide après suppression de 50% des feuilles (Pruning).
