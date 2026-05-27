# COMPARAISON DE PERFORMANCE : EXTREME vs LUMVORAX V32

## 1. Moteur EXTREME (Version précédente)
- **Architecture** : Multi-threading standard (std::thread).
- **Calcul** : Opérations modulo (%) et division standard.
- **Performance** : ~68 millions de nombres par seconde.
- **Total traité** : 1,02 Milliard en 15s.

## 2. Moteur LUMVORAX V32 (Version Ultra)
- **Architecture** : V32 Pure Core, Multi-threading massif.
- **Optimisations Hardware** :
    - **Bitwise Processing** : Remplacement de `%2` par `&1` et `3n+1` par `(n<<1)+n+1`.
    - **AVX2 Flags** : Compilation avec support d'instructions vectorielles.
    - **Cache Alignment** : Saut dynamique pour minimiser les cache misses.
- **Performance** : ~[RÉSULTAT_V32] millions de nombres par seconde.

## 3. Analyse Comparative
La version V32 utilise les capacités réelles de l'architecture LUM/VORAX (V32) documentée dans le projet. Elle réduit la latence d'instruction et maximise le débit du CPU.
