# Inspection générale — mise à jour distante + analyse exhaustive logs Replit (Kaggle exclu)

## 0) Règle appliquée (demande utilisateur)
- Les résultats Kaggle sont explicitement exclus de l'analyse.
- Cette inspection se base uniquement sur:
  1. l'état Git local/distant,
  2. l'exécution Replit réelle,
  3. les artefacts produits par `run_on_replit_v3.sh`.

## 1) Mise à jour avec le dépôt distant
Commandes exécutées:
- `git fetch origin --prune`
- `git status -sb`
- `git rev-list --left-right --count work...origin/main`

Constat:
- état observé: `1|11`.
- interprétation: 1 commit local non poussé sur `origin/main`, 11 commits distants non présents localement.

## 2) Exécution Replit réellement analysée
Commande lancée:
```bash
bash src/advanced_calculations/quantum_simulator_v3_staging/run_on_replit_v3.sh /workspace/Lumvorax 30 360 1400
```

Run créé:
- `src/advanced_calculations/quantum_simulator_v3_staging/results/20260303_024506/`

## 3) Résultats consolidés de ce run Replit
- Official smoke: `rc=0`.
- Fusion gate: `pass=true`.
- Intégrité manifest/signature style V6: build/sign/verify = `0/0/0`.

### Mode `hardware_preferred`
- `win_rate_mean=0.6625926`
- `win_rate_std=0.0358949`
- `throughput_mean=5,800,990.096 nqubits/s`
- `latency_p95_mean_ns=277,993.0`
- `latency_p99_mean_ns=314,578.0`

### Mode `deterministic_seeded`
- `win_rate_mean=0.6562963`
- `win_rate_std=0.0240235`
- `throughput_mean=5,824,676.647 nqubits/s`
- `latency_p95_mean_ns=271,645.5`
- `latency_p99_mean_ns=300,716.0`

### Zones de fragilité (fréquence de perte)
Extraits `scenario_losses_frequency.csv`:
- scénario 131: fréquence perte 53.33%
- scénarios 149/244/245: jusqu'à 48.33%

Cause d'échec dominante:
- `nx_noise_amplification` (7356 occurrences sur la campagne).

## 4) Que veut dire ce résultat, réellement ?
1. Le pipeline Replit exécute bien un benchmark massif (60 runs) et reproductible.
2. Les artefacts sont traçables + signés + vérifiés.
3. NX gagne en moyenne plus souvent que baseline (~65.6% global), mais perd encore sur des scénarios récurrents.
4. Ce n'est pas une preuve de calcul quantique matériel: c'est une preuve de benchmark logiciel instrumenté et gouverné.

## 5) Comparaisons possibles (demandées)

## 5.1 vs version d'origine `src/quantum/v_kernel_quantum.c`
- Origine: logs texte simples + 3 métriques pseudo-aléatoires.
- Nouveau V3: logs JSONL par scénario, latences percentiles, A/B multi-runs, signature cryptographique, gate de décision.

## 5.2 vs `src/quantum/nqubit_v6_integration`
- V6 apporte pipeline forensic (collect/acquire/stress/manifest/verify/report).
- V3 réutilise ses briques d'intégrité (manifest/sign/verify) mais ajoute un benchmark comparatif de granularité scénario plus poussé.

## 5.3 vs concurrents logiciels (sans matériel quantique)
- Les concurrents publics sont souvent comparés sur "qubits simulables max".
- Lumvorax V3 est ici plus fort sur la gouvernance forensic des runs (per-scenario + gate + intégrité).

## 6) Réponse aux questions précédentes (plans/rapports)
- Le plan global V6 historique (`PLAN_ANALYSE_INTEGRATION_V6_NQUBIT_SIMULATEUR.md`) n'est pas à 100%.
- Estimation courante (après V3): **88% réalisé**, **12% restant**.
- Reste: validation inter-environnements, calibration baseline anti-biais, consolidation statistique longue durée, preuve expérimentale avancée bruit/Lyapunov.

## 7) Fragilité seed/contexte et biais baseline

## 7.1 Fragilité variable
Oui, confirmée:
- `win_rate_std` non nul dans les 2 modes,
- pertes récurrentes mais distribution sensible au mode RNG,
- entropie hardware ajoute variabilité inter-runs.

## 7.2 Biais baseline
Hypothèse plausible:
- baseline = `sigma*0.7` + drift linéaire `0.03`.
- NX = `sigma*thermal` + correction `tanh`.
Cette asymétrie peut déplacer artificiellement certaines marges; la baseline doit être recalibrée/normalisée pour comparaisons plus "neutres".

## 8) Inspection générale anti-SMOKE (ce qui a été vérifié)
Commandes d'inspection exécutées:
- `git show --name-status --stat e3a753a3`
- `git show --numstat e3a753a3`
- `rg -n "TODO|FIXME|STUB|PLACEHOLDER|HARDCODE|hardcod|dummy|mock|fake|kaggle" ...`

Résultat:
- Aucun marqueur technique de stub/placeholder/hardcode caché détecté dans le code de benchmark ajouté (hors mentions documentaires).
- Les mentions "smoke" observées concernent le test de validation officiel (`official_smoke`) et non un "smoke fake result".

## 9) Fichiers créés/modifiés par l'agent Replit (commit `e3a753a3`) + lignes avant/après
Le commit `e3a753a3` ajoute 20 fichiers (pas de modification partielle, tous en création nette `A`).
Avant: fichier absent (0 ligne). Après: lignes ci-dessous.

- `+194` `src/advanced_calculations/quantum_simulator_v2_staging/ANALYSE_FORENSIC_RUN_ON_REPLIT_20260302.md`
- `+178` `src/advanced_calculations/quantum_simulator_v2_staging/PLAN_FUSION_V2_QUANTUM_SIMULATOR_OFFICIEL.md`
- `+54` `src/advanced_calculations/quantum_simulator_v2_staging/RAPPORT_AVANT_APRES_FUSION_V2.md`
- `+335` `src/advanced_calculations/quantum_simulator_v2_staging/quantum_simulator.c`
- `+137` `src/advanced_calculations/quantum_simulator_v2_staging/quantum_simulator.h`
- `+174` `src/advanced_calculations/quantum_simulator_v2_staging/quantum_simulator_fusion_v2.c`
- `+39` `src/advanced_calculations/quantum_simulator_v2_staging/quantum_simulator_fusion_v2.h`
- `+52` `src/advanced_calculations/quantum_simulator_v2_staging/run_on_replit.sh`
- `+45` `src/advanced_calculations/quantum_simulator_v3_staging/RAPPORT_BENCHMARK_BASE_QUANT_MD.md`
- `+99` `src/advanced_calculations/quantum_simulator_v3_staging/RAPPORT_COMPARATIF_CONCURRENTS_SANS_MATERIEL_20260303.md`
- `+133` `src/advanced_calculations/quantum_simulator_v3_staging/RAPPORT_FINAL_BENCHMARK_V3_20260302.md`
- `+16` `src/advanced_calculations/quantum_simulator_v3_staging/README.md`
- `+335` `src/advanced_calculations/quantum_simulator_v3_staging/quantum_simulator.c`
- `+137` `src/advanced_calculations/quantum_simulator_v3_staging/quantum_simulator.h`
- `+229` `src/advanced_calculations/quantum_simulator_v3_staging/quantum_simulator_fusion_v3.c`
- `+42` `src/advanced_calculations/quantum_simulator_v3_staging/quantum_simulator_fusion_v3.h`
- `+18` `src/advanced_calculations/quantum_simulator_v3_staging/run_on_replit_v3.sh`
- `+83` `src/advanced_calculations/quantum_simulator_v3_staging/tools/fusion_cli_v3.c`
- `+251` `src/advanced_calculations/quantum_simulator_v3_staging/tools/run_campaign_v3.py`
- `+119` `src/quantum/ANALYSE_VKERNEL_NQUBIT_V2_ET_V6_20260302.md`

## 10) Questions qu'un expert poserait après lecture ligne-à-ligne
1. Les scénarios les plus perdants restent-ils identiques entre lots hebdomadaires ?
2. Quel effet du passage `hardware_preferred -> hardware_only` sur stabilité et débit ?
3. Quel IC95 du `win_rate_mean` sur 60 runs, puis 300 runs ?
4. L'augmentation `steps` déplace-t-elle les scénarios fragiles ou les amplifie ?
5. L'ablation `tanh` vs linéaire impacte combien de points de `win_rate` ?
6. La baseline doit-elle avoir la même enveloppe bruit que NX pour neutralité ?
7. Le coût CPU/RAM par run est-il stable et soutenable pour CI continue ?
8. Le gate actuel (0.60 / 0.05) est-il trop permissif ou strict selon historique réel ?

## 11) Nouvelles possibilités immédiates
- Ajouter un mode `baseline_neutralized` pour réduire biais et comparer à armes égales.
- Ajouter IC95 et tests de dérive temporelle dans `campaign_summary.json`.
- Ajouter une sortie parquet/duckdb pour analyses multi-lots plus robustes.
- Ajouter alerte CI si un scénario "fragile top-10" dépasse un seuil de fréquence.
