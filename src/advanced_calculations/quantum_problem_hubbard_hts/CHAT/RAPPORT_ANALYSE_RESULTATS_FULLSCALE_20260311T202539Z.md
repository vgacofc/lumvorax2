# RAPPORT — ANALYSE RÉSULTATS FULLSCALE (`research_20260311T202539Z_1816`)

## Résumé chiffré
- PASS: 22
- FAIL: 9
- OBSERVED: 49

## Comparaison avec run précédent (`research_20260311T181312Z_1925`)
- PASS: 19 -> 22
- FAIL: 12 -> 9
- OBSERVED: 49 -> 49

## Signification scientifique
- `energy_vs_U` est passé en PASS: la réponse énergétique à `U` est devenue cohérente avec le critère moyen de pente positive.
- `cluster_energy_trend` est passé en PASS: tendance cluster corrigée.
- `dt_convergence` reste PASS: stabilité numérique satisfaisante sur ce critère.

## Anomalies restantes
- Benchmarks QMC/DMRG toujours FAIL (RMSE/MAE/within_error_bar)
- Interprétation: l’échelle du modèle fullscale n’est pas encore réconciliée avec les références externes.

## Version enfant (5 ans)
Imagine deux dessins:
- Le premier dessin est plus flou (ancien run).
- Le deuxième est plus propre (nouveau run).
On voit mieux les formes, mais quand on compare à une photo réelle, ce n’est pas encore pareil.
Donc: **on a bien amélioré**, mais **pas encore parfait**.

## Prochaines actions recommandées
1. Recalage unité/échelle benchmark (énergie par site vs énergie totale).
2. Vérification des seuils PASS/FAIL benchmark avec incertitudes réalistes.
3. Campagne multi-runs dédiée (sans héritage historique) pour recalibrer les métriques externes.
