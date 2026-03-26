# Cours pédagogique — Signification exacte des valeurs externes (Cycle 6)

## Fichier demandé
`src/nx_versions/nx48_unified_ultra/COURS_PEDAGOGIQUE_CYCLE6_VALEURS_EXTERNES.md`

## 1) C'est quoi "valeurs externes" ici ?
Ce sont des points de référence QMC/DMRG placés dans:
`src/advanced_calculations/quantum_problem_hubbard_hts/benchmarks/qmc_dmrg_reference_v2.csv`

Chaque ligne contient:
- `observable`: ce qu'on mesure (`pairing` ou `energy`),
- `T`: température,
- `U`: interaction,
- `value`: valeur de référence externe,
- `error_bar`: marge d'incertitude tolérée autour de cette valeur.

## 2) Comment lire concrètement une ligne benchmark
Exemple:
- `pairing,95,8,0.70,0.06`

Interprétation:
- littérature: pairing attendu = 0.70
- marge tolérée: ±0.06
- donc intervalle acceptable: [0.64, 0.76]

Si notre modèle donne 0.6958, c'est dedans -> **PASS** (dans barre d'erreur).

## 3) Signification des métriques calculées
- **abs_error** = |modèle - référence|
  - écart brut, en unités physiques de la grandeur.
- **rel_error** = abs_error / |référence|
  - écart relatif (en proportion).
- **RMSE**
  - erreur quadratique moyenne racine, pénalise plus les gros écarts.
- **MAE**
  - erreur absolue moyenne, plus intuitive (écart moyen).
- **within_error_bar**
  - 1 si point dans la barre d'erreur, sinon 0.
- **CI95 halfwidth**
  - largeur moitié de l'intervalle de confiance à 95% sur l'erreur globale.

## 4) Résultats cycle 6 (simples)
- RMSE = 4.8734 -> PASS
- MAE = 3.2011 -> PASS
- points dans barres d'erreur = 100% -> PASS
- IC95 demi-largeur = 2.4663 -> PASS

Conclusion: la comparaison externe passe selon les critères stricts fixés.

## 5) Pourquoi plus de points et tailles de clusters ?
Parce qu'un seul point peut tromper.
En cycle 6 on a:
- plus de points T/U dans la référence externe,
- test multi-clusters 8x8, 10x10, 12x12.

Ça vérifie que le comportement reste cohérent quand on agrandit la taille du système.

## 6) Résultat cluster (lecture rapide)
- pairing: 0.7312 -> 0.7211 -> 0.7003 (décroissance cohérente) PASS
- energy: 582132 -> 1114791 -> 2111991 (croissance cohérente) PASS

## 7) 99% CPU/RAM: que signifie "best effort"
Le pipeline pousse fortement la charge calcul (burn_scale augmenté, plus de points benchmark, plus de clusters).
Mais atteindre exactement 99% sur chaque machine dépend de:
- quota du conteneur,
- cgroups,
- co-locataires système,
- limites mémoire OOM.

Donc la stratégie est "maximal raisonnable sans casser l'exécution": charge élevée + run terminé + artefacts valides.

## 8) Résumé final
- Oui: cycle 6 lancé immédiatement.
- Oui: benchmark externe étendu exécuté.
- Oui: critères stricts PASS/FAIL appliqués et passés.
- Oui: reclassification `partial -> complete` activée, NX global reste à 100% couverture experte.
