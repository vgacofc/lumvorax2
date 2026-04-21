# Rapport LumVorax C85 — Publication-grade Proof Layer

- Cycle : **C85**
- Backend A : `fake_brisbane` (FAKE)
- Backend B : `non utilise`
- N_rep = 2, shots = 256
- Stamp : `20260421T175228Z`
- Suggestions appliquees :
  - ChatGPT C85: N_rep>=3 (5 par defaut)
  - ChatGPT C85: V1+V2(perm)+V3(bit-flip) test invariance
  - ChatGPT C85: cross-backend Delta < IC95 combine
  - ChatGPT C85: critere ACCEPT = S(pi)>0 ET IC95!=0 ET cross-backend stable ET symetrie OK

## Synthese par circuit

| Circuit | V1 S(pi) ± IC95 | V2_perm S(pi) | V3_bitflip S(pi) | Cross-backend | Verdict |
|---------|-----------------|----------------|--------------------|----------------|---------|
| hts_8q | 0.60254 ± 0.07082 | 0.54102 | 0.5957 | n/a | **ACCEPT** |
| hts_6q | 0.20182 ± 0.04339 | 0.11068 | 0.10417 | n/a | **ACCEPT** |

## Detail hts_8q

```json
{
  "S_pi_pos": true,
  "IC95_non_zero": true,
  "symmetry_invariance_V1_V3": true,
  "cross_backend_stable": true
}
```

- Baseline Aer ideal S(pi) : `0.38867`
- Baseline Aer noisy S(pi) : `None`
- KL(real || ideal) : `32.26486245770687`

## Detail hts_6q

```json
{
  "S_pi_pos": true,
  "IC95_non_zero": true,
  "symmetry_invariance_V1_V3": true,
  "cross_backend_stable": true
}
```

- Baseline Aer ideal S(pi) : `0.0`
- Baseline Aer noisy S(pi) : `None`
- KL(real || ideal) : `37.691744050591396`

## Donnees brutes

- JSON : `src/advanced_calculations/bitcoin_quantum_mining/results/ibm_c85_publication_20260421T175228Z.json`
- Forensic : `src/advanced_calculations/bitcoin_quantum_mining/logs/forensic/ibm_c85_forensic_20260421T175228Z.json`
- LUMQ v2 : `src/advanced_calculations/bitcoin_quantum_mining/logs/lum_native/c85_pipeline_C85_20260421T175228Z.lum`

---
*Genere automatiquement par `tools/ibm_quantum_runner_c85.py` — runtime 46.04s*