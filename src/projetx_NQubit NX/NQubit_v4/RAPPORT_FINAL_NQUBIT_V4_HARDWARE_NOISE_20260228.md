# RAPPORT_FINAL_NQUBIT_V4_HARDWARE_NOISE_20260228

## 1) Différences claires entre versions (V2/V3/V4)

- Score NX moyen V2: 0.361543481940
- Score NX moyen V3: 0.361543481940 (+0.000000%)
- Score NX moyen V4: 0.361543481940 (+0.000000% vs V3)
- Score baseline moyen V4: 0.189412669968
- Écart score V4: 0.172130811972 (90.876081%)
- Écart énergie V4: 13.916848103278 (63.620275%)

## 2) Résultats P0/P1 (V4)

| experiment | delta_pct | win_rate | ci95 |
|---|---:|---:|---:|
| baseline_current | 90.764% | 1.000000 | [0.173650, 0.174600] |
| p0_no_lyapunov | 34.368% | 0.840185 | [0.064772, 0.067092] |
| p0_no_noise | 0.000% | 0.997222 | [0.000000, 0.000000] |
| p0_impulsive_noise | 64.588% | 1.000000 | [0.123540, 0.124275] |
| p0_symmetric_scoring | -28.453% | 0.000000 | [-0.090648, -0.090007] |
| p1_strong_baseline | -28.880% | 0.000000 | [-0.092552, -0.091920] |
| p1_ood_drift_heavy | -40.799% | 0.000000 | [-0.118508, -0.117849] |

## 3) Bruit matériel réel (proxy noyau + jitter horloge)

- samples: 20000
- mean: -0.001220662221
- stddev: 0.576980363621
- min/max: -0.999884541782 / 0.999972451859
- p01/p99: -0.978746551176 / 0.978484355148

## 4) Interprétation

- Les métriques V4 de benchmark restent cohérentes avec V3 (pas de perte des intégrations précédentes).
- Les phases P0/P1 confirment que l’avantage dépend du protocole de score et de la force de baseline.
- La capture hardware V4 fournit une source de bruit mesurée sur environnement réel (proxy), exploitable pour Replit.

## 5) Go/No-Go

- GO provisoire logiciel: oui.
- GO hardware scientifique: en attente de runs Replit + comparaison online + répétitions multi-machines.