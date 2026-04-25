# `src/asic_simulation/` — Simulation ASIC classique + quantique + IBM-like

Cycle C95. Voir `src/advanced_calculations/bitcoin_quantum_mining/CHAT/CAHIER_DES_CHARGES_C95_MAITRE.md` §7.

## Trois cibles
1. **ASIC SHA-256 classique** (Antminer S19 XP : 140 TH/s, 21.5 W/TH)
2. **ASIC quantique** (grille N×N qubits supraconducteurs)
3. **ASIC IBM Heron R2** (calibré via `IBM_C93_*` du header `ibm_quantum_constants.h`)

## État C95
- ✅ `asic_simulation.h` — API publique
- ✅ `sha256_pipeline_classical.c` — modèle Antminer S19
- ⏳ `asic_quantum_array.c`
- ⏳ `ibm_heron_r2_model.c`
- ⏳ `asic_thermal_model.c`
- ⏳ `asic_power_model.c`

## Validation cible
| Cible | Référence | Tolérance |
|-------|-----------|-----------|
| TH/s S19 XP | 140 TH/s | ±5 % |
| Power S19 XP | 3010 W | ±10 % |
| S(π) Heron R2 N=8 | 0.9944 (IBM_C93) | ±5 % |
| Depth Heron R2 N=8 | 14 (IBM_C93_DEPTH_PHYS) | exact |
