# Collatz Conjecture Research Analysis

## Reformulation Pédagogique
Pour tout entier n > 0, si n est pair on divise par 2, si n est impair on fait 3n+1. Est-ce qu'on finit toujours par atteindre 1 ?

## Historique Scientifique
Aussi connu sous le nom de problème Syracuse.

## Rôle de LUMVORAX-CORE
Utilisation de `advanced_calculations/tsp_optimizer` (adapté pour les cycles) et du module `binary/binary_lum_converter` pour l'analyse de trajectoires binaires.

## Auto-Critique
L'absence de contre-exemple jusqu'à des nombres massifs n'est pas une preuve. Possibilité de cycles non détectés.
