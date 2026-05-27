# Quantum Simulator V4 Staging Next

Cette version est une copie de `v3_staging` pour itérer sans régression sur la base stable.

## Nouveautés intégrées
- mode `baseline_neutralized` pour comparer avec baseline bruit neutralisé,
- métrique explicite `max_qubits_width` (largeur de qubits simulés déclarée pour comparaison externe),
- garde-fou `throughput_regression_flag`,
- campagne à 3 modes: `hardware_preferred`, `deterministic_seeded`, `baseline_neutralized`.

## Exécution Replit
```bash
bash /workspace/Lumvorax/src/advanced_calculations/quantum_simulator_v4_staging_next/run_on_replit_v4_next.sh /workspace/Lumvorax 30 360 1400 36
```
