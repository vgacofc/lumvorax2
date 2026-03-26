# Nouveau rapport de benchmark (base documentaire: `quant.md`)

## 1) Source utilisée
Ce rapport est construit à partir de `quant.md` (panorama simulateurs quantiques vs matériel réel) et des métriques du benchmark staging V3.

## 2) Cadre d'interprétation (issu de `quant.md`)
- Un simulateur logiciel ne prouve pas un avantage quantique matériel.
- Les résultats doivent être lus comme performance/stabilité d'un moteur classique instrumenté.
- La comparaison pertinente est algorithmique (A/B de dynamiques), pas une équivalence QPU réel.

## 3) KPI benchmark V3 à suivre
- `win_rate` NX vs baseline.
- `nqubits_per_sec`.
- latences scénario `p50/p95/p99`.
- fréquence des scénarios perdants.
- fréquences des `fail_reason`.
- stabilité inter-runs (`std(win_rate)`) pour seed/context.

## 4) Sens de "fragilité variable seed/contexte"
Le benchmark est stochastique: la trajectoire d'état dépend des tirages de bruit.
Selon seed et source d'entropie (hardware vs déterministe), certains scénarios passent de gain à perte.
Donc la fragilité est réelle si:
1. la fréquence de pertes varie fortement inter-runs,
2. les scénarios perdants se déplacent selon seed,
3. la variance de `win_rate` reste élevée.

## 5) Hypothèse de biais baseline (`sigma*0.7`, drift `0.03`)
Oui, un biais est plausible:
- la baseline reçoit un bruit réduit (`sigma*0.7`) et une dynamique linéaire fixe (`0.03`).
- NX subit `sigma*thermal` + correction non linéaire (`tanh`).
=> Ce design peut favoriser ou défavoriser NX selon zones de paramètres.

## 6) Positionnement technologique
- Origine (`src/quantum/v_kernel_quantum.c`): prototype métrique simple.
- Officiel (`src/advanced_calculations/quantum_simulator.c`): moteur qubit/LUM structuré.
- V6 (`src/quantum/nqubit_v6_integration`): pipeline forensic+manifest+verify.
- V3 staging: benchmark comparatif granulaire + campagne 30+ runs + intégrité manifest/signature.

## 7) État d'avancement (% estimé)
Estimation stricte actuelle sur le plan global demandé: **82% réalisé**.
- Réalisé: isolation staging, benchmark granulaire, campagne multi-run, exports CSV, manifest/signature, gate de fusion.
- Reste ~18%:
  - valider inter-environnements (Replit/Kaggle/local) sur plusieurs lots,
  - confirmer seuils gate par historique long,
  - consolider contrôle de bruit avancé avec preuves statistiques robustes.
