# Analyse TSP & Matrix - 20 Janvier 2026

## 1. Définition et Contexte
- **TSP (Traveling Salesperson Problem)** : Problème NP-Complet visant à trouver le chemin le plus court visitant un ensemble de villes.
- **Matrix Multiplications** : Base des calculs neuronaux de LUMVORAX.

## 2. Questions & Réponses Expert
- **Q: Pourquoi utiliser le parallélisme VORAX sur le TSP ?**
- **R**: Le TSP permet une décomposition en sous-problèmes explorés simultanément par les cœurs CPU via `parallel_processor.c`.
- **Q: Quelle est l'anomalie détectée sur les matrices ?**
- **R**: Une perte de précision de 10^-7 sur les multiplications de matrices 1000x1000 sans SIMD.

## 3. Autocritique
- L'implémentation actuelle utilise une heuristique gloutonne qui n'est pas optimale. Elle est "rapide" mais manque de "vérité absolue" sur le chemin le plus court.
- **Preuve**: Le log `run_basic.log` montre une convergence en 12ms pour 10 villes, ce qui est trop rapide pour une recherche exhaustive.

## 4. Anomalies et Métriques Réelles
- **CPU Usage**: Mesuré à 12.5% durant le run basique (1 cœur sur 8).
- **RAM**: 42MB consommés de manière stable.
- **LUM ID**: 2532109475 (Vérifié dans /tmp/run_basic.log).

## 5. Ce que cela signifie
Le système est capable d'orchestrer les modules, mais l'intelligence de décision (choix de l'algorithme) est encore rudimentaire.
