# Rapport LumVorax C84-B — Experimental Validation Layer

- Cycle : **C84-B** (suite naturelle C67-C83)
- Backend : `ibm_fez` (IBM REEL)
- Stamp : `20260421T175103Z`
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
| S(pi) > 0 | Claude/ChatGPT | 0.6123 | 0.09896 |
| IC95 S(pi) | ChatGPT | 0.00383 | n/a (1 run) |
| Symmetry gap dom vs flipped | ChatGPT Q3 | 0.00181 | 0.00586 |
| KL real vs Aer ideal | Claude | 6.142188042422062 | 5.12617070321954 |
| Dominant prob (mean) | C64 MAX C83 | 0.12717 | 0.83594 |
| Verdict | — | **ACCEPT** | n/a |

## Test critique Q3 ChatGPT — initialisation X

- P(00001111) avec X-init : 0.00203
- P(11110000) avec X-init : 0.0
- Symmetry gap : 0.00203
- **Verdict Q3** : `PHYSIQUE_AFM_PROBABLE`

> Si verdict = `ARTEFACT_INIT` : le pattern 00001111 etait du a l'initialisation |0>^n (decouverte ChatGPT).
> Si verdict = `PHYSIQUE_AFM_PROBABLE` : ordre antiferromagnetique reel survit, candidat publication.

## Top 10 etats HTS 8Q (apres readout mitigation)

| Etat | Probabilite |
|------|-------------|
| `10000000` | 0.12709 |
| `10000001` | 0.05458 |
| `10000110` | 0.03793 |
| `10000100` | 0.03495 |
| `00000000` | 0.03065 |
| `10010110` | 0.02784 |
| `11000000` | 0.02676 |
| `10010000` | 0.02284 |
| `10010100` | 0.02224 |
| `00000001` | 0.02179 |

## Top 10 etats HTS 6Q Mott

| Etat | Probabilite |
|------|-------------|
| `111111` | 0.83594 |
| `110111` | 0.02734 |
| `111110` | 0.02539 |
| `011111` | 0.01758 |
| `000111` | 0.01758 |
| `111101` | 0.01367 |
| `100111` | 0.01172 |
| `111011` | 0.00977 |
| `110001` | 0.00781 |
| `111100` | 0.00586 |

## Donnees brutes

- JSON resultats : `src/advanced_calculations/bitcoin_quantum_mining/results/ibm_c84_validation_20260421T175103Z.json`
- Forensic JSON : `src/advanced_calculations/bitcoin_quantum_mining/logs/forensic/ibm_c84_forensic_20260421T175103Z.json`
- LUMQ v2 : `src/advanced_calculations/bitcoin_quantum_mining/logs/lum_native/c84_pipeline_C84_20260421T175103Z.lum`

---
*Genere automatiquement par `tools/ibm_quantum_runner_c84.py` — runtime 38.47s*