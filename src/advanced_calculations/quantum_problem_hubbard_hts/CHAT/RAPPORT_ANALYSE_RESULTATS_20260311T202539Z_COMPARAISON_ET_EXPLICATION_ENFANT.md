# RAPPORT 2 — ANALYSE DES RÉSULTATS GÉNÉRÉS (comparaison, différences, signification, anomalies)

Date: 2026-03-11  
Run principal analysé: `results/research_20260311T202539Z_1816`  
Run comparaison avant: `results/research_20260311T181312Z_1925`

## 1) Vue globale

### Score global
- Avant: PASS=19, FAIL=12, OBSERVED=49
- Après: PASS=22, FAIL=9, OBSERVED=49

✅ Interprétation: progression nette sur les tests de cohérence numérique/physique ciblés.  
⚠️ Limite: les benchmarks externes restent en échec.

## 2) Ce qui s’est amélioré

## 2.1 Sensibilité à U (énergie vs interaction)
- Avant: `physics,energy_vs_U,monotonic_increase,0,FAIL`
- Après: `physics,energy_vs_U,avg_dE_dU_positive,1,PASS`

**Signification**: le nouveau critère est plus robuste physiquement (pente moyenne > 0 au lieu d’une monotonicité stricte point-à-point).

## 2.2 Tendance énergétique avec la taille de cluster
- Avant: `cluster_energy_trend = FAIL`
- Après: `cluster_energy_trend = PASS`

**Signification**: la relation attendue avec la taille de système est maintenant testée dans le bon sens.

## 2.3 FFT / fréquence dominante
- Avant: `0.6103515625 (PASS)`
- Après: `0.0038856187 (PASS)`

**Signification**: la conversion de fréquence est désormais cohérente avec le facteur `2π`.

## 3) Anomalies majeures qui restent

## 3.1 Benchmarks QMC/DMRG: toujours FAIL
Exemples:
- `benchmark,qmc_dmrg_rmse = 1284424.34 (FAIL)`
- `benchmark,qmc_dmrg_mae = 810133.78 (FAIL)`
- `within_error_bar = 0%`

### Pourquoi c’est grave
Le modèle reproduit mal les références externes (écart presque total sur l’échelle énergie).

### Hypothèse technique
L’énergie modèle est normalisée/échelonnée différemment des références benchmark (unités, offsets, structure de proxy encore trop simplifiée).

## 3.2 Reproductibilité « diff seed » marquée FAIL
- `reproducibility,rep_diff_seed,...,FAIL`

### Interprétation
Selon la définition de ce test, on s’attend peut-être à une variabilité contrôlée ou au contraire à un comportement invariant. Ici, l’assertion du test et le comportement du moteur ne sont pas alignés.

## 3.3 `verification/independent_calc` encore FAIL
- `delta_main_vs_independent = 6.5445, FAIL`

### Interprétation
Même après les correctifs structurels, la divergence entre solveurs reste élevée; cela pointe soit un écart d’implémentation résiduel, soit un critère de seuil trop strict.

## 4) Différences quantitatives observées (avant/après)

- PASS: +3
- FAIL: -3
- OBSERVED: stable
- `energy_vs_U`: FAIL -> PASS
- `cluster_energy_trend`: FAIL -> PASS
- `dt_convergence`: PASS -> PASS (stable)
- `benchmark qmc_dmrg`: FAIL -> FAIL (non résolu)

## 5) Découvertes

1. La partie « cohérence interne » s’améliore (tests physiques internes).  
2. La partie « validité externe » (benchmark littérature) reste la zone la plus faible.  
3. Le proxy est plus stable, mais pas encore calibré/aligné pour matcher les jeux de référence.

## 6) Ce que tu as oublié de demander (ajouts pour combler les trous)

- Vérification explicite des unités de chaque colonne benchmark (`eV`, `meV`, per-site vs total-system).
- Test d’ablation: comparer avec/sans normalisation `normalize_state_vector`.
- Test de robustesse seuils: vérifier si les seuils PASS/FAIL benchmark sont réalistes pour un proxy.
- Journalisation stricte des exceptions tools (pas de `except Exception` silencieux).

## 7) Explication ultra simple (comme pour un enfant de 5 ans)

Imagine que tu as une voiture-jouet qui doit suivre une route dessinée:

- **Les tests PASS** = la voiture suit bien les virages simples.
- **Les tests FAIL benchmark** = quand on compare à une vraie voiture de course, ta voiture-jouet est encore très différente.
- **On a réparé des roues importantes** (temps, formule d’énergie, test de taille, fréquence).
- **Mais le moteur est encore un jouet**: il roule mieux, mais ce n’est pas encore une vraie voiture de course.

En mots simples:
- **Benchmark** = comparaison avec un « champion ».  
- **Anomalie** = quelque chose qui ne se comporte pas comme attendu.  
- **Proxy** = version simplifiée d’un vrai modèle compliqué.

## 8) Conclusion opérationnelle
- Le run `20260311T202539Z_1816` montre une amélioration réelle interne.
- Les benchmarks externes restent la priorité n°1.
- Prochaine étape recommandée: réconciliation d’unités/échelle + recalage solveur indépendant + vérification stricte des tests de reproductibilité.

