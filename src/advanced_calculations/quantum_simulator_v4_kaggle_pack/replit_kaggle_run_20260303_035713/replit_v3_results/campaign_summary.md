# Quantum Simulator V3 Staging Campaign Summary

- run_id: `20260303_035715`
- runs_per_mode: **30**
- scenarios: **360**
- steps: **1400**

## A/B officiel vs staging
- officiel smoke rc: `0`
- staging fusion gate pass: `True`

## Modes
### hardware_preferred
- count: 30
- win_rate_mean: 0.656667
- win_rate_std: 0.021738
- throughput_mean: 8665317.045
- latency_p95_mean_ns: 197317.3
- latency_p99_mean_ns: 217165.0

### deterministic_seeded
- count: 30
- win_rate_mean: 0.656296
- win_rate_std: 0.024023
- throughput_mean: 8123624.254
- latency_p95_mean_ns: 205580.0
- latency_p99_mean_ns: 237711.3

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