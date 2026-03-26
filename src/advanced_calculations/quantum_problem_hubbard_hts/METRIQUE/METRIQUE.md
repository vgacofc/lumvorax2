# Référentiel métrique avancé (CPU/RAM, sans matériel quantique)

Ce document recense **toutes les métriques immédiatement calculables** avec le moteur actuel, en version avancée, sans simplification artificielle des critères de validation.

## 1) Métriques cœur physique

| Famille | Métrique | Fichier source | Interprétation | Seuil/usage |
|---|---|---|---|---|
| Énergie | `energy` | `logs/baseline_reanalysis_metrics.csv` | Niveau d’énergie total proxy du système simulé | suivre dérive inter-run et par module |
| Pairing | `pairing` | `logs/baseline_reanalysis_metrics.csv` | Force de couplage effectif de paires | rechercher minimum local / rebond |
| Sign problem | `sign_ratio` | `logs/baseline_reanalysis_metrics.csv` | Sévérité du problème de signe | valeur absolue faible = risque numérique réduit |
| Densité locale | `density_proxy` | `tests/module_physics_metadata.csv` + observables dérivées | Occupation locale | vérifier cohérence avec température / µ |
| Corrélations spatiales | `corr_2pt` (proxy) | `tests/integration_spatial_correlations.csv` | Corrélation courte/moyenne portée | validation structurelle multi-modules |

## 2) Métriques robustesse numérique

| Famille | Métrique | Fichier source | Interprétation | Critère |
|---|---|---|---|---|
| Convergence temporelle | `dt_sensitivity_proxy` | `tests/new_tests_results.csv` | Variation réponse entre `dt/2`, `dt`, `2dt` | plus faible = plus robuste |
| Stabilité intégration | `dt_convergence` | `tests/numerical_stability_suite.csv` | Stabilité discrétisation temporelle | `PASS` attendu |
| Dérivées temporelles | `temporal_derivative_variance` | `tests/temporal_derivatives_variance.csv` | Bruit/raideur dynamique locale | pic = zone instable potentielle |
| Entropie proxy | `von_neumann_proxy` | `tests/integration_entropy_observables.csv` | Mélange d’états / désordre effectif | suivre variation vs T, U/t |
| Cas analytique jouet | `exp_decay abs_error` | `tests/toy_model_validation.csv` | Vérification contre solution fermée | fichier non vide + erreur faible |

## 3) Métriques spectrales / dynamiques

| Famille | Métrique | Fichier source | Interprétation | Usage |
|---|---|---|---|---|
| Spectre temporel | `dominant_fft_frequency` | `tests/new_tests_results.csv` | Fréquence dominante d’oscillation | détecter résonances / pompage |
| Pumping dynamique | `energy_reduction_ratio` | `tests/new_tests_results.csv` (family `dynamic_pumping`) | gain énergétique sous contrôle actif | >0 : contrôle utile |
| Pumping dynamique | `pairing_gain` | `tests/new_tests_results.csv` (family `dynamic_pumping`) | gain pairing sous contrôle | >0 : amélioration pairing |
| Contrôle explicite | `phase_control`, `resonance_pump`, `magnetic_quench`, `feedback_loop` | `tests/new_tests_results.csv` | Impact direct des modes de contrôle | audit module par module |

## 4) Métriques validation croisée / benchmarks

| Famille | Métrique | Fichier source | Interprétation | Usage |
|---|---|---|---|---|
| Référence QMC/DMRG | `within_error_bar` | `tests/benchmark_comparison_qmc_dmrg.csv` | Accord avec référence externe | cible = 100% PASS |
| Solver alternatif | `alternative_solver_campaign` | `tests/integration_alternative_solver_campaign.csv` | sensibilité au solveur indépendant | détecter biais solveur unique |
| Authenticité code | `hardcoding_risk` + flags audit | `tests/integration_code_authenticity_audit.csv` | risque de logique non-physique | doit rester bas/justifié |

## 5) Métriques observabilité système (CPU/RAM)

| Famille | Métrique | Fichier source | Interprétation | Usage |
|---|---|---|---|---|
| Runtime | `elapsed_ns` | `logs/baseline_reanalysis_metrics.csv` | coût par simulation | dimensionnement runs |
| Télémetrie | `cpu_percent`, `mem_percent` | `tests/integration_low_level_runtime_metrics.csv` | pression ressources machine | prévenir saturation |
| Débit | `calc_per_sec` | `tests/integration_low_level_runtime_metrics.csv` | productivité calcul | comparer versions |
| Latence | `latency_ns_per_step` | `tests/integration_low_level_runtime_metrics.csv` | coût pas de temps | optimisation ciblée |

## 6) Modules actuels couverts

- `hubbard_hts_core`
- `qcd_lattice_proxy`
- `quantum_field_noneq`
- `dense_nuclear_proxy`
- `quantum_chemistry_proxy`

## 7) Modules avancés intégrés immédiatement (CPU/RAM)

| Module candidat | Domaine | Observables immédiatement accessibles | Priorité | Benchmark externe individuel |
|---|---|---|---|---|
| `spin_liquid_exotic` | Matière condensée | energy, pairing, corrélations locales/moyenne portée | Haute | `benchmarks/external_module_benchmarks_v1.csv` |
| `topological_correlated_materials` | Matière condensée | energy, pairing, corrélateurs topologiques, Chern approché | Haute | `benchmarks/external_module_benchmarks_v1.csv` |
| `correlated_fermions_non_hubbard` | Condensée/chimie | energy, pairing, densité locale, corrélateurs simples | Haute | `benchmarks/external_module_benchmarks_v1.csv` |
| `multi_state_excited_chemistry` | Chimie quantique | énergie états excités, densité locale, gap effectif | Haute | `benchmarks/external_module_benchmarks_v1.csv` |
| `bosonic_multimode_systems` | Physique quantique | occupation modes, energy, corrélations inter-modes | Moyenne | `benchmarks/external_module_benchmarks_v1.csv` |
| `multiscale_nonlinear_field_models` | Théorie des champs | pseudo-spectres locaux, extrema spatio-temporels | Moyenne | `benchmarks/external_module_benchmarks_v1.csv` |
| `far_from_equilibrium_kinetic_lattices` | Champs hors équilibre | énergie, spectre k, charge locale | Moyenne | `benchmarks/external_module_benchmarks_v1.csv` |
| `multi_correlated_fermion_boson_networks` | Statistique/condensée | energy, pairing, densité, corrélateurs moyenne portée | Moyenne | `benchmarks/external_module_benchmarks_v1.csv` |

- Les comparaisons sont exportées dans `tests/benchmark_comparison_external_modules.csv` run par run.

## 8) Règles de validité (anti-artefact)

1. Tout fichier critique vide (`size=0`) = **FAIL bloquant**.
2. Tout test marqué `OBSERVED` doit recevoir ensuite un test à seuil binaire PASS/FAIL.
3. Les métriques doivent être traçables par checksum (`logs/checksums.sha256`).
4. Toute nouvelle métrique doit déclarer : unité, méthode de calcul, seuil et domaine de validité.

## 9) Commandes reproductibles

```bash
git fetch https://github.com/lumc01/Lumvorax.git --prune
bash src/advanced_calculations/quantum_problem_hubbard_hts/run_research_cycle.sh
python3 src/advanced_calculations/quantum_problem_hubbard_hts/tools/post_run_remote_depot_independent_analysis.py src/advanced_calculations/quantum_problem_hubbard_hts
```
