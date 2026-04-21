# Rapport LumVorax C84-B — Experimental Validation Layer

- Cycle : **C84-B** (suite naturelle C67-C83)
- Backend : `fake_brisbane` (FAKE/SIM)
- Stamp : `20260421T175023Z`
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
| S(pi) > 0 | Claude/ChatGPT | 0.67383 | 0.125 |
| IC95 S(pi) | ChatGPT | 0.04977 | n/a (1 run) |
| Symmetry gap dom vs flipped | ChatGPT Q3 | 0.00274 | 0.0 |
| KL real vs Aer ideal | Claude | 9.706367851420435 | 5.271308671602267 |
| Dominant prob (mean) | C64 MAX C83 | 0.13857 | 0.83594 |
| Verdict | — | **ACCEPT** | n/a |

## Test critique Q3 ChatGPT — initialisation X

- P(00001111) avec X-init : 0.0053
- P(11110000) avec X-init : 0.0
- Symmetry gap : 0.0053
- **Verdict Q3** : `PHYSIQUE_AFM_PROBABLE`

> Si verdict = `ARTEFACT_INIT` : le pattern 00001111 etait du a l'initialisation |0>^n (decouverte ChatGPT).
> Si verdict = `PHYSIQUE_AFM_PROBABLE` : ordre antiferromagnetique reel survit, candidat publication.

## Top 10 etats HTS 8Q (apres readout mitigation)

| Etat | Probabilite |
|------|-------------|
| `10000000` | 0.13855 |
| `10000100` | 0.03964 |
| `10000001` | 0.03313 |
| `10000110` | 0.03219 |
| `10010110` | 0.03004 |
| `11000000` | 0.02724 |
| `11010000` | 0.02515 |
| `11000001` | 0.02343 |
| `10011000` | 0.02302 |
| `10010100` | 0.02295 |

## Top 10 etats HTS 6Q Mott

| Etat | Probabilite |
|------|-------------|
| `111111` | 0.83594 |
| `101111` | 0.06250 |
| `110111` | 0.02344 |
| `111101` | 0.02344 |
| `110101` | 0.01172 |
| `111011` | 0.01172 |
| `011111` | 0.00781 |
| `111110` | 0.00781 |
| `101101` | 0.00391 |
| `000111` | 0.00391 |

## Donnees brutes

- JSON resultats : `src/advanced_calculations/bitcoin_quantum_mining/results/ibm_c84_validation_20260421T175023Z.json`
- Forensic JSON : `src/advanced_calculations/bitcoin_quantum_mining/logs/forensic/ibm_c84_forensic_20260421T175023Z.json`
- LUMQ v2 : `src/advanced_calculations/bitcoin_quantum_mining/logs/lum_native/c84_pipeline_C84_20260421T175023Z.lum`

---
*Genere automatiquement par `tools/ibm_quantum_runner_c84.py` — runtime 23.49s*