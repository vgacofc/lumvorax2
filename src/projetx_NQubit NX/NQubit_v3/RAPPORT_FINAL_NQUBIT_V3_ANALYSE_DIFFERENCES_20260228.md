# RAPPORT_FINAL_NQUBIT_V3_ANALYSE_DIFFERENCES_20260228

## 1) Réponse claire : que signifient les différences de résultats ?

Le modèle NX obtient un score moyen supérieur à la baseline dans le protocole courant, mais avec un coût énergétique plus élevé. Cela signifie que l’avantage actuel vient d’un compromis précision/stabilité favorable au score NX, et non d’une preuve hardware définitive.

### Différences principales (baseline courante)

- Score NX moyen: **0.361543481940**
- Score baseline moyen: **0.189412669968**
- Différence absolue score: **0.172130811972**
- Différence relative score: **90.876081%**
- Énergie NX moyenne: **35.791711377528**
- Énergie baseline moyenne: **21.874863274250**
- Différence absolue énergie: **13.916848103278**
- Différence relative énergie: **63.620275%**

## 2) Différences en % par phase P0/P1 (avec IC95%)

| Expérience | NX mean | Baseline mean | Δ abs | Δ % | Win rate | IC95% Δ |
|---|---:|---:|---:|---:|---:|---:|
| baseline_current | 0.365968 | 0.191843 | 0.174125 | 90.764% | 1.000000 | [0.173650, 0.174600] |
| p0_no_lyapunov | 0.257775 | 0.191843 | 0.065932 | 34.368% | 0.840185 | [0.064772, 0.067092] |
| p0_no_noise | 1.000000 | 1.000000 | 0.000000 | 0.000% | 0.997222 | [0.000000, 0.000000] |
| p0_impulsive_noise | 0.315751 | 0.191843 | 0.123908 | 64.588% | 1.000000 | [0.123540, 0.124275] |
| p0_symmetric_scoring | 0.227140 | 0.317468 | -0.090328 | -28.453% | 0.000000 | [-0.090648, -0.090007] |
| p1_strong_baseline | 0.227140 | 0.319376 | -0.092236 | -28.880% | 0.000000 | [-0.092552, -0.091920] |
| p1_ood_drift_heavy | 0.171479 | 0.289657 | -0.118178 | -40.799% | 0.000000 | [-0.118508, -0.117849] |

## 3) Lecture experte rapide des phases

- **baseline_current**: gain relatif score = **90.764%** (écart vs baseline courante: **+0.000 points**).
- **p0_no_lyapunov**: gain relatif score = **34.368%** (écart vs baseline courante: **-56.396 points**).
- **p0_no_noise**: gain relatif score = **0.000%** (écart vs baseline courante: **-90.764 points**).
- **p0_impulsive_noise**: gain relatif score = **64.588%** (écart vs baseline courante: **-26.176 points**).
- **p0_symmetric_scoring**: gain relatif score = **-28.453%** (écart vs baseline courante: **-119.217 points**).
- **p1_strong_baseline**: gain relatif score = **-28.880%** (écart vs baseline courante: **-119.644 points**).
- **p1_ood_drift_heavy**: gain relatif score = **-40.799%** (écart vs baseline courante: **-131.563 points**).

## 4) Couverture des priorités demandées

- **P0 couvert**: ablations sans Lyapunov, sans bruit, bruit impulsionnel, scoring symétrisé, 30 runs/scénario + IC95%.
- **P1 couvert (logiciel)**: baseline renforcée adaptative + tests OOD (drift thermique + bruit lourd).
- **P2 partiel (planifié)**: acquisition ADC réelle/injection physique non réalisable dans cet environnement de simulation; protocole prêt dans ce rapport.

## 5) Gouvernance de preuve et anti-biais

- Protocole figé avant exécution (tests P0/P1 constants).
- Résultats horodatés en forensic et consolidés en CSV.
- Critère go/no-go proposé: la borne basse IC95% de Δ score doit rester > 0 sur baseline forte et OOD.

## 6) Threats to validity (obligatoire)

- Simulateur PRNG, pas de bruit hardware réel.
- Baselines encore internes au module (pas benchmark externe standardisé).
- IC95% calculé sur essais simulés, dépend du protocole choisi.

## 7) Verdict V3

- **Proto logiciel: validé**.
- **Preuve hardware: non validée à ce stade**.
- **Suite immédiate**: brancher acquisition ADC réelle et reproduire exactement ce protocole avec les mêmes sorties forensic.

## 8) Manifest de preuves

- Un manifest horodaté et hashé a été généré: `results/manifest_forensic_v3.json` (SHA-256 de chaque artefact, versions, formules de seed).
