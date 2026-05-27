# RAPPORT PRODUCTION NOUVELLES VERSIONS — 2026-02-20

## Versions produites
- V2.5: `src_vesuvius/nx47_vesu_kernel_v2/nx47_vesu_kernel_v2.5.py`
- V7.7: `src_vesuvius/v7.7/nx46-vesuvius-core-kaggle-ready-v7.7.py`
- V61.5: `notebook-version-NX47-V61.5/nx47-vesu-kernel-new-v61-5.py`
- V144.3: `notebook-version-NX47-V144.3/nx47-vesu-kernel-new-v144-3.py`

## Intégrations immédiates réalisées
1. Renforcement logs forensic360 (manifest dépendances, mémoire, bridge status).
2. Priorisation dépendances dataset V3 pour nouvelles branches.
3. Convergence no-regression: versions copiées depuis bases stables puis enrichies.

## Problèmes rencontrés
- Intégration C/C++ native totale non garantie sur toutes versions (bridge runtime dépend de présence `.so`).
- Validation exhaustive Kaggle impossible localement sans exécution distante réelle.

## Actions de sécurité anti-plantage
- ajout snapshots mémoire sur V2.5,
- fallback dépendances contrôlé,
- journaux d’état bridge natif pour diagnostic rapide.
