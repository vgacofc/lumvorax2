# Prototype NQubit NX - V4

Cette version conserve tout l'historique V2/V3 et ajoute :
- capture de bruit matériel réel (proxy noyau `/dev/urandom` + jitter horloge),
- nouveaux artefacts forensiques V4,
- prompt Replit pour exécution profonde et collecte nanoseconde.

## Build / tests
```bash
make clean all test_all run
```

## Sorties principales V4
- `results/nqbit_forensic_ns.log`
- `results/nqbit_benchmark.csv`
- `results/nqbit_stats_v2.csv`
- `results/phase_p0_p1_metrics.csv`
- `results/phase_p0_p1_forensic.log`
- `results/hardware_noise_samples_v4.csv`
- `results/hardware_noise_forensic_v4.log`
- `results/manifest_forensic_v4.json`
- `RAPPORT_FINAL_NQUBIT_V4_HARDWARE_NOISE_20260228.md`
- `PROMPT_REPLIT_AGENT_NQUBIT_V4.md`
