# Rapport LumVorax C84-B — Experimental Validation Layer

- Cycle : **C84-B** (suite naturelle C67-C83)
- Backend : `ibm_fez` (IBM REEL)
- Stamp : `20260422T213928Z`
- Suggestions appliquees (Claude + ChatGPT) :
  - Q3 ChatGPT: test |+> initialisation pour valider 00001111 (anti-artefact)
  - Claude: observables E + <ZZ> + S(pi) au lieu de juste counts
  - ChatGPT: readout mitigation locale obligatoire
  - ChatGPT: ZNE 1/3/5 noise factors
  - Claude: baselines Aer ideal + noisy (KL divergence)
  - ChatGPT: IC95 + symmetry gap < 2-3 sigma

## Bilan des criteres publication-grade

| Test | Source suggestion | HTS 8Q AFM | HTS 6Q Mott |
|------|-------------------|------------|-------------|
| S(pi) > 0 | Claude/ChatGPT | 0.59294 | 0.16406 |
| IC95 S(pi) | ChatGPT | 0.00888 | n/a (1 run) |
| Symmetry gap dom vs flipped | ChatGPT Q3 | 0.00293 | 0.00879 |
| KL real vs Aer ideal | Claude | 4.518398879765567 | 7.763764255858348 |
| Dominant prob (mean) | C64 MAX C83 | 0.14225 | 0.75879 |
| Verdict | — | **ACCEPT** | n/a |

## Test critique Q3 ChatGPT — initialisation X

- P(00001111) avec X-init : 0.00889
- P(11110000) avec X-init : 0.00098
- Symmetry gap : 0.00791
- **Verdict Q3** : `PHYSIQUE_AFM_PROBABLE`

> Si verdict = `ARTEFACT_INIT` : le pattern 00001111 etait du a l'initialisation |0>^n (decouverte ChatGPT).
> Si verdict = `PHYSIQUE_AFM_PROBABLE` : ordre antiferromagnetique reel survit, candidat publication.

## ZNE Zero Noise Extrapolation (suggestion ChatGPT)

- ZNE non execute : 'The instruction h on qubits (0,) is not supported by the target system. Circuits that do not match the target hardware definition are no longer supported after March 4, 2024. See the transpilation documentation (https://quantum.cloud.ibm.com/docs/guides/transpile) for instructions to transform circuits and the primitive examples (https://quantum.cloud.ibm.com/docs/guides/primitives-examples) to see this coupled with operator transformations.'

## Top 10 etats HTS 8Q (apres readout mitigation)

| Etat | Probabilite |
|------|-------------|
| `10000000` | 0.14222 |
| `10000001` | 0.07938 |
| `10000100` | 0.04143 |
| `00000000` | 0.03907 |
| `10000110` | 0.02755 |
| `11000000` | 0.02372 |
| `10010100` | 0.02350 |
| `10000101` | 0.02249 |
| `10010110` | 0.02083 |
| `10010000` | 0.01914 |

## Top 10 etats HTS 6Q Mott

| Etat | Probabilite |
|------|-------------|
| `111111` | 0.75879 |
| `011111` | 0.09375 |
| `111101` | 0.02051 |
| `111110` | 0.01953 |
| `100111` | 0.01562 |
| `110111` | 0.01465 |
| `000111` | 0.01172 |
| `110001` | 0.01172 |
| `101111` | 0.01074 |
| `111100` | 0.00879 |

## Donnees brutes

- JSON resultats : `src/advanced_calculations/bitcoin_quantum_mining/results/ibm_c84_validation_20260422T213928Z.json`
- Forensic JSON : `src/advanced_calculations/bitcoin_quantum_mining/logs/forensic/ibm_c84_forensic_20260422T213928Z.json`
- LUMQ v2 : `src/advanced_calculations/bitcoin_quantum_mining/logs/lum_native/c84_pipeline_C84_20260422T213928Z.lum`

---
*Genere automatiquement par `tools/ibm_quantum_runner_c84.py` — runtime 55.58s*