# CAHIER DES CHARGES NX-14 : SYNCHRONISATION MULTI-NOEUDS

## 1. OBJECTIFS
- **Moteur de Fusion Merkle** : Capacité de fusionner deux flux `ION` provenant d'unités différentes.
- **Métriques Hardware Temps Réel** : Monitoring CPU, RAM, et OPS/s.
- **Équilibrage de l'Entropie** : Stabiliser le système global lors de la fusion.

## 2. EXIGENCES
- **Vitesse** : > 1,000,000 OPS/s (Operations Per Second).
- **Consommation** : Monitoring simulé en fJ/op.
- **Intégrité** : Root Merkle finale doit inclure les parents des deux unités.
