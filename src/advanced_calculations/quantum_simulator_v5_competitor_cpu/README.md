# Quantum Simulator V5 Competitor CPU Pack

Nouvelle copie isolée pour intégrer les **5 concurrents CPU pleinement supportés sur Replit**,
sans toucher aux versions V2/V3/V4 existantes.

## Concurrents actifs (Replit-fully-supported=5)
1. Qiskit Aer
2. quimb (Tensor Network MPS)
3. Qulacs
4. MQT DDSIM
5. QuTiP

> Note: ProjectQ est retiré du set actif car l'installation wheel échoue dans l'environnement Replit actuel.

## Exécution Replit exacte (officielle)
```bash
bash /workspace/Lumvorax/src/advanced_calculations/quantum_simulator_v5_competitor_cpu/run_on_replit_v5_competitors.sh /workspace/Lumvorax 30 360 1400 36 0 0
```

Paramètres:
1. `ROOT`
2. `RUNS`
3. `SCENARIOS`
4. `STEPS`
5. `MAX_QUBITS_WIDTH`
6. `PLAN_ONLY` (`1` = clone + plan sans benchmark snippet)
7. `SKIP_INSTALL` (`1` = n'installe pas pip)

## Batterie minimale de validation runtime authentique
```bash
bash /workspace/Lumvorax/src/advanced_calculations/quantum_simulator_v5_competitor_cpu/run_on_replit_v5_competitors.sh /workspace/Lumvorax 1 20 40 36 0 0
```

## Batterie maximale recommandée (benchmark officiel)
```bash
bash /workspace/Lumvorax/src/advanced_calculations/quantum_simulator_v5_competitor_cpu/run_on_replit_v5_competitors.sh /workspace/Lumvorax 30 360 1400 36 0 0
```

## Artefacts générés
- `src/advanced_calculations/quantum_simulator_v5_competitor_cpu/results/<run_id>/competitor_cpu_results.csv`
- `src/advanced_calculations/quantum_simulator_v5_competitor_cpu/results/<run_id>/competitor_cpu_summary.json`
- `src/advanced_calculations/quantum_simulator_v5_competitor_cpu/results/<run_id>/competitor_cpu_summary.md`


## Protocole benchmark unifié strict (nouveau)
Par défaut, l'exécution V5 active maintenant un **protocole unifié strict**:
- même famille de circuit (`ghz`),
- mêmes tailles (`4, 8, 12 qubits`),
- même shot budget (`256`).

Fichier protocole:
- `src/advanced_calculations/quantum_simulator_v5_competitor_cpu/tools/unified_benchmark_protocol_v5.json`

Artefacts supplémentaires:
- `src/advanced_calculations/quantum_simulator_v5_competitor_cpu/results/<run_id>/competitor_cpu_unified_results.csv`

Option de désactivation (debug uniquement):
```bash
python src/advanced_calculations/quantum_simulator_v5_competitor_cpu/tools/benchmark_competitors_cpu.py --disable-strict-unified
```
