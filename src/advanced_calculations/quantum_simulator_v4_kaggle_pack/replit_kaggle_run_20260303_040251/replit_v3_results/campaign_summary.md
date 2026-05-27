# Quantum Simulator V3 Staging Campaign Summary

- run_id: `20260303_040253`
- runs_per_mode: **30**
- scenarios: **360**
- steps: **1400**

## A/B officiel vs staging
- officiel smoke rc: `0`
- staging fusion gate pass: `True`

## Modes
### hardware_preferred
- count: 30
- win_rate_mean: 0.659444
- win_rate_std: 0.024570
- throughput_mean: 8019758.717
- latency_p95_mean_ns: 218094.0
- latency_p99_mean_ns: 301007.0

### deterministic_seeded
- count: 30
- win_rate_mean: 0.656296
- win_rate_std: 0.024023
- throughput_mean: 7843633.103
- latency_p95_mean_ns: 211064.0
- latency_p99_mean_ns: 241320.3

## Intégrité manifest/signature (style V6)
- manifest build rc: 0
- sign rc: 0
- verify rc: 0

## Fichiers produits
- campaign_runs.csv
- scenario_losses_frequency.csv
- failure_reasons_frequency.csv
- campaign_summary.json
- manifest_forensic_v3.json + .sig