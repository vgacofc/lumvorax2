# RAPPORT_EVOLUTION_REPLIT_MULTI_RUNS_20260312

## Synthèse exécutive
- Analyse comparative multi-runs générée automatiquement.
- Focus: intégrité, stabilité numérique, tendances métriques, tests en échec persistants, progression et reste-à-faire.

## Runs analysés (ordre chronologique donné)
- research_20260312T075921Z_523: baseline_rows=305, new_tests_fail=11, numerical_fail=13, checksums_present=True
- research_20260312T181131Z_2715: baseline_rows=305, new_tests_fail=11, numerical_fail=0, checksums_present=False
- research_20260312T182101Z_2854: baseline_rows=305, new_tests_fail=11, numerical_fail=0, checksums_present=True
- research_20260312T222246Z_2829: baseline_rows=305, new_tests_fail=11, numerical_fail=0, checksums_present=False
- research_20260312T222838Z_3447: baseline_rows=305, new_tests_fail=10, numerical_fail=13, checksums_present=True

## Détails par run
### research_20260312T075921Z_523
- Intégrité: checksums_present=True, missing_refs=5
  - MISSING_REF: `./logs/environment_versions.log`
  - MISSING_REF: `./logs/hfbl360_realtime_persistent.log`
  - MISSING_REF: `./logs/independent_modules_execution_trace.log`
  - MISSING_REF: `./logs/provenance.log`
  - MISSING_REF: `./logs/research_execution.log`
- new_tests: PASS=20 FAIL=11 OBSERVED=49
- FAIL test_id: cluster_energy_trend, cluster_pair_trend, energy_vs_U, external_modules_mae, external_modules_rmse, external_modules_within_error_bar, independent_calc, qmc_dmrg_ci95_halfwidth, qmc_dmrg_mae, qmc_dmrg_rmse, qmc_dmrg_within_error_bar
- numerical_stability: PASS=17 FAIL=13
- Modules en FAIL energy_density_drift_max:
  - bosonic_multimode_systems: drift=1.7045e-06
  - correlated_fermions_non_hubbard: drift=3.2828e-06
  - dense_nuclear_fullscale: drift=3.1916e-06
  - far_from_equilibrium_kinetic_lattices: drift=2.1545e-06
  - hubbard_hts_core: drift=2.3636e-06
  - multi_correlated_fermion_boson_networks: drift=2.6091e-06
  - multi_state_excited_chemistry: drift=4.1506e-06
  - multiscale_nonlinear_field_models: drift=2.8606e-06
  - qcd_lattice_fullscale: drift=2.3174e-06
  - quantum_chemistry_fullscale: drift=7.1892e-06
  - quantum_field_noneq: drift=3.6399e-06
  - spin_liquid_exotic: drift=1.875e-06
  - topological_correlated_materials: drift=2.2625e-06
- Baseline moyennes: energy=-1.0329859815632787, pairing=0.2633715458032787, sign_ratio=-0.00230764335704918, elapsed_ns=974124104.1672131, cpu=16.62911475409836, mem=76.62498360655738

### research_20260312T181131Z_2715
- Intégrité: checksums_present=False, missing_refs=0
- new_tests: PASS=20 FAIL=11 OBSERVED=49
- FAIL test_id: cluster_energy_trend, cluster_pair_trend, energy_vs_U, external_modules_mae, external_modules_rmse, external_modules_within_error_bar, independent_calc, qmc_dmrg_ci95_halfwidth, qmc_dmrg_mae, qmc_dmrg_rmse, qmc_dmrg_within_error_bar
- numerical_stability: PASS=30 FAIL=0
- Baseline moyennes: energy=-1.0329859815632787, pairing=0.2633715458032787, sign_ratio=-0.00230764335704918, elapsed_ns=993117635.8524591, cpu=17.81, mem=70.87901639344263

### research_20260312T182101Z_2854
- Intégrité: checksums_present=True, missing_refs=5
  - MISSING_REF: `./logs/environment_versions.log`
  - MISSING_REF: `./logs/hfbl360_realtime_persistent.log`
  - MISSING_REF: `./logs/independent_modules_execution_trace.log`
  - MISSING_REF: `./logs/provenance.log`
  - MISSING_REF: `./logs/research_execution.log`
- new_tests: PASS=20 FAIL=11 OBSERVED=49
- FAIL test_id: cluster_energy_trend, cluster_pair_trend, conv_monotonic, energy_vs_U, external_modules_mae, external_modules_rmse, external_modules_within_error_bar, qmc_dmrg_ci95_halfwidth, qmc_dmrg_mae, qmc_dmrg_rmse, qmc_dmrg_within_error_bar
- numerical_stability: PASS=30 FAIL=0
- Baseline moyennes: energy=-0.05344104250393442, pairing=0.8380402729714754, sign_ratio=-0.00230764335704918, elapsed_ns=11413804.37704918, cpu=17.88, mem=64.19

### research_20260312T222246Z_2829
- Intégrité: checksums_present=False, missing_refs=0
- new_tests: PASS=20 FAIL=11 OBSERVED=49
- FAIL test_id: cluster_energy_trend, cluster_pair_trend, external_modules_mae, external_modules_rmse, external_modules_within_error_bar, fft_dominant_amplitude, independent_calc, qmc_dmrg_ci95_halfwidth, qmc_dmrg_mae, qmc_dmrg_rmse, qmc_dmrg_within_error_bar
- numerical_stability: PASS=30 FAIL=0
- Baseline moyennes: energy=2.007008762132131, pairing=0.8384991134770492, sign_ratio=-0.00230764335704918, elapsed_ns=623978734.9606557, cpu=19.83, mem=81.92498360655738

### research_20260312T222838Z_3447
- Intégrité: checksums_present=True, missing_refs=5
  - MISSING_REF: `./logs/environment_versions.log`
  - MISSING_REF: `./logs/hfbl360_realtime_persistent.log`
  - MISSING_REF: `./logs/independent_modules_execution_trace.log`
  - MISSING_REF: `./logs/provenance.log`
  - MISSING_REF: `./logs/research_execution.log`
- new_tests: PASS=21 FAIL=10 OBSERVED=49
- FAIL test_id: cluster_energy_trend, cluster_pair_trend, external_modules_mae, external_modules_rmse, external_modules_within_error_bar, fft_dominant_amplitude, qmc_dmrg_ci95_halfwidth, qmc_dmrg_mae, qmc_dmrg_rmse, qmc_dmrg_within_error_bar
- numerical_stability: PASS=17 FAIL=13
- Modules en FAIL energy_density_drift_max:
  - bosonic_multimode_systems: drift=3.655e-06
  - correlated_fermions_non_hubbard: drift=5.388e-06
  - dense_nuclear_fullscale: drift=8.5557e-06
  - far_from_equilibrium_kinetic_lattices: drift=4.5557e-06
  - hubbard_hts_core: drift=4.5111e-06
  - multi_correlated_fermion_boson_networks: drift=4.1759e-06
  - multi_state_excited_chemistry: drift=4.7556e-06
  - multiscale_nonlinear_field_models: drift=5.4076e-06
  - qcd_lattice_fullscale: drift=6.2324e-06
  - quantum_chemistry_fullscale: drift=6.5798e-06
  - quantum_field_noneq: drift=6.1712e-06
  - spin_liquid_exotic: drift=4.9307e-06
  - topological_correlated_materials: drift=3.6419e-06
- Baseline moyennes: energy=2.007008762132131, pairing=0.8384991134770492, sign_ratio=-0.00230764335704918, elapsed_ns=11096240.249180328, cpu=19.96, mem=85.77059016393443

## Comparaison run précédent -> suivant
### research_20260312T075921Z_523 -> research_20260312T181131Z_2715
- energy: max_abs_diff=0.0, mean_abs_diff=0.0, common_points=305
- pairing: max_abs_diff=0.0, mean_abs_diff=0.0, common_points=305
- sign_ratio: max_abs_diff=0.0, mean_abs_diff=0.0, common_points=305
- elapsed_ns: max_abs_diff=118906469.0, mean_abs_diff=20036956.28852459, common_points=305
- cpu_percent: max_abs_diff=1.1899999999999977, mean_abs_diff=1.1808852459016388, common_points=305
- mem_percent: max_abs_diff=6.010000000000005, mean_abs_diff=5.745967213114755, common_points=305

### research_20260312T181131Z_2715 -> research_20260312T182101Z_2854
- energy: max_abs_diff=2.5118560231, mean_abs_diff=1.1384097435636065, common_points=305
- pairing: max_abs_diff=0.6997430401, mean_abs_diff=0.5746687271681967, common_points=305
- sign_ratio: max_abs_diff=0.0, mean_abs_diff=0.0, common_points=305
- elapsed_ns: max_abs_diff=2352037091.0, mean_abs_diff=981703831.4754099, common_points=305
- cpu_percent: max_abs_diff=0.07000000000000028, mean_abs_diff=0.07000000000000028, common_points=305
- mem_percent: max_abs_diff=6.8700000000000045, mean_abs_diff=6.689016393442625, common_points=305

### research_20260312T182101Z_2854 -> research_20260312T222246Z_2829
- energy: max_abs_diff=2.8440483469999998, mean_abs_diff=2.0604498046360655, common_points=305
- pairing: max_abs_diff=0.009848841300000077, mean_abs_diff=0.0007743042990163998, common_points=305
- sign_ratio: max_abs_diff=0.0, mean_abs_diff=0.0, common_points=305
- elapsed_ns: max_abs_diff=1439720572.0, mean_abs_diff=612564930.5836066, common_points=305
- cpu_percent: max_abs_diff=1.9499999999999993, mean_abs_diff=1.9499999999999993, common_points=305
- mem_percent: max_abs_diff=18.10000000000001, mean_abs_diff=17.73498360655738, common_points=305

### research_20260312T222246Z_2829 -> research_20260312T222838Z_3447
- energy: max_abs_diff=0.0, mean_abs_diff=0.0, common_points=305
- pairing: max_abs_diff=0.0, mean_abs_diff=0.0, common_points=305
- sign_ratio: max_abs_diff=0.0, mean_abs_diff=0.0, common_points=305
- elapsed_ns: max_abs_diff=1437378082.0, mean_abs_diff=612882494.7114754, common_points=305
- cpu_percent: max_abs_diff=0.13000000000000256, mean_abs_diff=0.13000000000000256, common_points=305
- mem_percent: max_abs_diff=4.539999999999992, mean_abs_diff=3.845606557377045, common_points=305

## Ce qui reste à faire (priorisé)
1. Corriger les écarts benchmark QMC/DMRG et modules externes (`qmc_dmrg_*`, `external_modules_*`) encore en FAIL.
2. Revoir les règles cluster-scale (`cluster_pair_trend`, `cluster_energy_trend`) qui restent en FAIL de manière récurrente.
3. Verrouiller la traçabilité: empêcher des références checksum vers des logs supprimés ou non persistés.
4. Exécuter un cycle complet après patch et comparer contre 2829/3447 avec ce même script pour vérifier la tendance.

## Commandes reproductibles
```bash
python3 src/advanced_calculations/quantum_problem_hubbard_hts/tools/analyze_replit_run_evolution.py \
  --results-root src/advanced_calculations/quantum_problem_hubbard_hts/results \
  --runs research_20260312T075921Z_523 research_20260312T181131Z_2715 research_20260312T182101Z_2854 research_20260312T222246Z_2829 research_20260312T222838Z_3447 \
  --out-json src/advanced_calculations/quantum_problem_hubbard_hts/CHAT/reports/replit_evolution_20260312.json \
  --out-md src/advanced_calculations/quantum_problem_hubbard_hts/CHAT/RAPPORT_EVOLUTION_REPLIT_MULTI_RUNS_20260312.md

bash src/advanced_calculations/quantum_problem_hubbard_hts/run_research_cycle.sh
```
