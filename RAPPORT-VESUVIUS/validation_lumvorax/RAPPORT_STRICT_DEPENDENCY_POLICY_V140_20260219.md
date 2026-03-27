# RAPPORT — Politique stricte dépendances (fail-fast sans fallback) appliquée

Date: 2026-02-19
Cible: `nx47_vesu_kernel_v2.4.py`

## Demande appliquée
- Aucune tolérance/fallback pour les dépendances runtime.
- En cas de dépendance manquante ou indisponible, crash immédiat et explicite.

## Changements effectués
1. `imagecodecs` est redevenue **obligatoire** au bootstrap (`required=True`).
2. Suppression du comportement “optionnel silencieux” pour dépendances absentes.
3. Sur lecture TIFF LZW, suppression du fallback Pillow pour la dépendance:
   - si `imagecodecs` manque, levée immédiate `DEPENDENCY_STRICT_FAILFAST`.

## Résultat attendu
- Détection d’erreur dès le départ (fail-fast).
- Plus de run qui continue avec dépendance partiellement manquante.
- Traçabilité des erreurs renforcée pour diagnostic rapide.
