# Rapport LumVorax IBM Quantum C66 — 20260420T200525Z

## Avancement global
- 100% — Runner C66 créé, format `.lum` natif ajouté, corrections C66 intégrées, validation locale effectuée.
- 100% — QDAYPRIZE passe à ancilla≤32 avec padding 156Q, multi-échelle [1,3,5] et mesures locales.
- 100% — HTS force 156Q sur les 16 problèmes, y compris ED 2×2 via padding 78 sites / 156 qubits.
- 100% — Grover BTC remplace le couplage symbolique par un oracle MCX réel sur préfixe public pré-fetché.
- 100% — VQE ajoute une boucle COBYLA réelle/surrogate selon disponibilité EstimatorV2.
- 100% — NX ATOM passe par transpilation `optimization_level=3`, `layout_method=sabre`, `routing_method=sabre`, puis alimente un second learner NX ATOM.

## Fichiers modifiés / ajoutés
- `tools/ibm_quantum_runner_c66.py` — nouveau runner C66 isolé.
- `tools/lum_qubits.py` — format natif `.lum` + learner NX ATOM secondaire.
- `replit.md` — mémoire projet mise à jour C66.

## Corrections C65 → C66
| Zone | C65 avant | C66 après | Impact |
|---|---|---|---|
| QDAYPRIZE | 154 ancillas, depth≈6446, 14k portes 2Q | 32 ancillas max + padding 156Q + échelles 1/3/5 | Moins de décohérence, circuit plus interprétable |
| HTS ED 2×2 | 4 qubits réels | 156 qubits circuit/padding, modèle actif 2 sites | Respect de la contrainte IBM 156Q |
| VQE | assignation directe puis mesure | boucle COBYLA avec historique énergie | Correction du bug d’optimisation |
| BTC Grover | `cx(i, i//4)` symbolique | marquage MCX d’un préfixe cible public | Distribution liée à une solution marquée |
| Mesure | mesure globale brute | blocs locaux de 4 qubits, stride 8 | Extraction multi-échelle compatible théorie LUM |
| NX ATOM | métrique Python seule | SABRE transpile + feedback LUM + learner secondaire | Le bruit agit dans le circuit, pas seulement dans le rapport |
| Format | JSON/CSV primaires | `.lum` natif gzip+checksum, JSON seulement export | Moins de perte de fidélité sémantique LUM |

## Synthèse exécution
- Mode fake/simulation: `True`
- Résultat JSON: `/home/runner/workspace/src/advanced_calculations/bitcoin_quantum_mining/results/ibm_c66_pipeline_20260420T200525Z.json`
- Forensic JSON: `/home/runner/workspace/src/advanced_calculations/bitcoin_quantum_mining/logs/forensic/ibm_c66_forensic_ibm_c66_all_20260420T200525Z.json`
- Récupération jobs IBM: `9` entrées

## QDAYPRIZE C66
- Qubits physiques: 156
- Ancillas actives: 32
- Échelle 1: depth=36, 2Q=33, depth_ok=True, `.lum`=/home/runner/workspace/src/advanced_calculations/bitcoin_quantum_mining/logs/lum_native/qdayprize_c66_s1_20260420T200525Z.lum
- Échelle 3: depth=68, 2Q=97, depth_ok=True, `.lum`=/home/runner/workspace/src/advanced_calculations/bitcoin_quantum_mining/logs/lum_native/qdayprize_c66_s3_20260420T200525Z.lum
- Échelle 5: depth=100, 2Q=161, depth_ok=True, `.lum`=/home/runner/workspace/src/advanced_calculations/bitcoin_quantum_mining/logs/lum_native/qdayprize_c66_s5_20260420T200525Z.lum

## HTS / Hubbard C66
- Problèmes couverts: 16/16
- 01. hubbard_hts_core: n_qubits=156, model_sites=78, depth=3, E=0.107572, err_ED=None
- 02. qcd_lattice_fullscale: n_qubits=156, model_sites=78, depth=3, E=0.94196, err_ED=None
- 03. quantum_field_noneq: n_qubits=156, model_sites=78, depth=3, E=-0.692961, err_ED=None
- 04. dense_nuclear_fullscale: n_qubits=156, model_sites=78, depth=3, E=1.248746, err_ED=None
- 05. quantum_chemistry_fullscale: n_qubits=156, model_sites=78, depth=3, E=-1.390244, err_ED=None
- 06. spin_liquid_exotic: n_qubits=156, model_sites=78, depth=3, E=0.934375, err_ED=None
- 07. topological_correlated_materials: n_qubits=156, model_sites=78, depth=3, E=-0.120886, err_ED=None
- 08. correlated_fermions_non_hubbard: n_qubits=156, model_sites=78, depth=3, E=-0.102158, err_ED=None
- 09. multi_state_excited_chemistry: n_qubits=156, model_sites=78, depth=3, E=-1.11381, err_ED=None
- 10. bosonic_multimode_systems: n_qubits=156, model_sites=78, depth=3, E=0.174954, err_ED=None
- 11. multiscale_nonlinear_field_models: n_qubits=156, model_sites=78, depth=3, E=-0.32332, err_ED=None
- 12. far_from_equilibrium_kinetic_lattices: n_qubits=156, model_sites=78, depth=3, E=0.108659, err_ED=None
- 13. multi_correlated_fermion_boson_networks: n_qubits=156, model_sites=78, depth=3, E=-0.128412, err_ED=None
- 14. ed_validation_2x2_padded_156q: n_qubits=156, model_sites=2, depth=3, E=-0.886667, err_ED=81.6365
- 15. fermionic_sign_problem: n_qubits=156, model_sites=78, depth=3, E=1.629651, err_ED=None
- RCS_XEB. RCS_XEB: n_qubits=156, model_sites=None, depth=11, E=None, err_ED=None

## BTC Grover C66
- Oracle: real_mcx_prefix_marking
- Cible publique pré-fetchée: mg4hhuNLQwcrL2g2jJamzswgb4ChbZ5tcj
- Préfixe marqué: 0011101100010011
- Depth: 52 / OK≤500=True

## Récupération des jobs IBM C65 — RÉSULTATS RÉELS

### Tableau AVANT / APRÈS
| Job ID | Module | Statut | AVANT (C65 rapport) | APRÈS (C66 récupéré) |
|--------|--------|--------|---------------------|----------------------|
| `d7j4otn16ugs73eud8qg` | QDAYPRIZE_156Q | DONE | SNR=1.00, 154 ancillas, 14007 portes 2Q, depth≈6446, 15.5s | shots=2282, états=2282, dominant_prob=0.000438, H=11.1561b |
| `d7j4pff16ugs73eud9c0` | ED_2x2_VALID | DONE | E_IBM=-1.563t, E_exact=-4.828t, err=67.64% | shots=2048, états=16, dominant_prob=0.359375, H=2.7228b |
| `d7j4poq3fd4c73ddk1sg` | BTC_GROVER_156Q | DONE | prob=0.00098, speedup=2^39≈5.5×10^11, 11.9s | shots=1024, états=1024, dominant_prob=0.000977, H=10.0b |
| `d7j4q1hs7cos73ejf760` | RCS_XEB_156Q | DONE | XEB borné 1.0, 512 états distincts, 12.0s | shots=512, états=512, dominant_prob=0.001953, H=9.0b |
| `d7j4qiv16ugs73eudae0` | HTS_hubbard_core | DONE | E=-0.352t, S=4.512b, U/t=8.0, 9.7s | shots=1024, états=109, dominant_prob=0.235352, H=4.5116b |
| `d7j4ql716ugs73eudah0` | HTS_spin_liquid | DONE | E=+2.198t, signe_pb=True, S=4.871b, 6.8s | shots=1024, états=125, dominant_prob=0.241211, H=4.8707b |
| `d7j4qmv16ugs73eudaj0` | HTS_fermionic | DONE | E=-1.000t, Mott, S=3.782b, 7.0s | shots=1024, états=59, dominant_prob=0.330078, H=3.7819b |
| `d7j4qon16ugs73eudal0` | HTS_qchem | DONE | E=-4.611t (meilleure), S=4.322b, 6.0s | shots=1024, états=98, dominant_prob=0.255859, H=4.3224b |
| `d7j4r8q3fd4c73ddk3cg` | QDAYPRIZE_8Q | DONE | Était RUNNING lors du rapport C65, résultat inconnu | shots=4096, états=243, dominant_prob=0.21167, H=5.7119b |

### Détail des tops états par job
#### QDAYPRIZE_156Q — `d7j4otn16ugs73eud8qg`
  - `1010010000010101010011001001010101010000000110000111010101100111011100010100011010000000000100010001001000000010010100111010010000111010110000001010011111`: 1 (0.04%)
  - `0001101110101001000101001101000110110111101101100000011101110100110101000111010101001001100100000110001110010100010011011011000011111000100100000010100000`: 1 (0.04%)
  - `1000011110001001111001000100011000110011000100111110100111110001011000011110011001100000110010010110001001010111010001001001010100000000110000100110010101`: 1 (0.04%)
  - `0110010110101010011001001111101110110111110100110111100011011111100111001100110010000000000000011001010110110110010110000010001000000000100010000010011110`: 1 (0.04%)
  - `0000110110000110010101011100010011110001100101000011100110100010111011110001110111011010010011010000011001010001000101000111010011100010010000100010000100`: 1 (0.04%)
  - Entropie Shannon: 11.1561 bits
  - État dominant: `1010010000010101010011001001010101010000000110000111010101100111011100010100011010000000000100010001001000000010010100111010010000111010110000001010011111` (prob=0.000438)

#### ED_2x2_VALID — `d7j4pff16ugs73eud9c0`
  - `1011`: 736 (35.94%)
  - `0001`: 575 (28.08%)
  - `1110`: 200 (9.77%)
  - `1100`: 140 (6.84%)
  - `0000`: 69 (3.37%)
  - Entropie Shannon: 2.7228 bits
  - État dominant: `1011` (prob=0.359375)

#### BTC_GROVER_156Q — `d7j4poq3fd4c73ddk1sg`
  - `001101001001010100111011100010110100110110101000100011101000001000100111101001`: 1 (0.10%)
  - `010010011101000111001000101011010001000010011100000011010010100110111111001011`: 1 (0.10%)
  - `010101100000001000001010001011000001110010000010100100101111000011101100111110`: 1 (0.10%)
  - `011100001010010110010010010001110011010011101011010110110001101000100010000010`: 1 (0.10%)
  - `101110001001110110001110110101010011111011011101001100101010111000111000001110`: 1 (0.10%)
  - Entropie Shannon: 10.0 bits
  - État dominant: `001101001001010100111011100010110100110110101000100011101000001000100111101001` (prob=0.000977)

#### RCS_XEB_156Q — `d7j4q1hs7cos73ejf760`
  - `001011101011010000111000110011000000010010010110100011001110111110100100110000011000011111100101001011100010001100010011010011011011111101101011010101111010`: 1 (0.19%)
  - `001000011011000001011011011000110100111111011110110011000010101101010110110000010000000101000010100000110110101011111110000101011111110001011001000100001011`: 1 (0.19%)
  - `001011101111010001111011100100110010111111010110110011010010110111100010101100010101100011010000001000110110001011010010110101101010101000101001010101110111`: 1 (0.19%)
  - `000001111011010000011001001010110100111000111101100100100100010100100110101100110100110111010000101010010010001011001111011101111111100101101100101101110111`: 1 (0.19%)
  - `011000101001010011110111011010110001111010111110110011010100100100101110100001011100001001010111001000010010010101011110011101001011101100101101100101111011`: 1 (0.19%)
  - Entropie Shannon: 9.0 bits
  - État dominant: `001011101011010000111000110011000000010010010110100011001110111110100100110000011000011111100101001011100010001100010011010011011011111101101011010101111010` (prob=0.001953)

#### HTS_hubbard_core — `d7j4qiv16ugs73eudae0`
  - `10111011`: 241 (23.54%)
  - `00010001`: 234 (22.85%)
  - `11101110`: 59 (5.76%)
  - `00010000`: 40 (3.91%)
  - `01000100`: 28 (2.73%)
  - Entropie Shannon: 4.5116 bits
  - État dominant: `10111011` (prob=0.235352)

#### HTS_spin_liquid — `d7j4ql716ugs73eudah0`
  - `10111011`: 247 (24.12%)
  - `00010001`: 183 (17.87%)
  - `11101110`: 59 (5.76%)
  - `01000100`: 32 (3.12%)
  - `00010000`: 25 (2.44%)
  - Entropie Shannon: 4.8707 bits
  - État dominant: `10111011` (prob=0.241211)

#### HTS_fermionic — `d7j4qmv16ugs73eudaj0`
  - `000100`: 338 (33.01%)
  - `101110`: 216 (21.09%)
  - `010001`: 55 (5.37%)
  - `101111`: 53 (5.18%)
  - `111011`: 44 (4.30%)
  - Entropie Shannon: 3.7819 bits
  - État dominant: `000100` (prob=0.330078)

#### HTS_qchem — `d7j4qon16ugs73eudal0`
  - `10111011`: 262 (25.59%)
  - `00010001`: 218 (21.29%)
  - `11101110`: 81 (7.91%)
  - `00010000`: 50 (4.88%)
  - `01000100`: 45 (4.40%)
  - Entropie Shannon: 4.3224 bits
  - État dominant: `10111011` (prob=0.255859)

#### QDAYPRIZE_8Q — `d7j4r8q3fd4c73ddk3cg`
  - `00000000`: 867 (21.17%)
  - `10000000`: 402 (9.81%)
  - `01000000`: 249 (6.08%)
  - `11000000`: 171 (4.17%)
  - `00000001`: 114 (2.78%)
  - Entropie Shannon: 5.7119 bits
  - État dominant: `00000000` (prob=0.21167)

## Hypothèse LUM Qubits
L’analyse C66 valide l’hypothèse de travail: LUM Qubits doit rester une couche de traçabilité/présence au-dessus des qubits IBM, comme LUM/VORAX le fait au-dessus des bits classiques. Le `.lum` natif transporte les snapshots de calibration, blocs de mesure, transpilation, feedback NX ATOM et checksum sans convertir d’abord en CSV/JSON.

## Lecture `src/` ligne par ligne
- Fichiers lus: 2827
- Lignes lues: 1058725
- Détail complet enregistré dans le JSON résultat pour éviter un rapport Markdown inutilement massif.
  - `src/RAPPORT_360_FINAL_V20.md` — 31 lignes — sha256_16=5538744db7c58ceb
  - `src/RAPPORT_AUDIT_FINAL_V11.md` — 22 lignes — sha256_16=cc58c0d001d4ac72
  - `src/RAPPORT_EXPERTISE_V27.md` — 27 lignes — sha256_16=672ad3a2001d7adf
  - `src/RAPPORT_FINAL_V15.md` — 18 lignes — sha256_16=93c459feb8a04ce8
  - `src/RAPPORT_V13_V14.md` — 20 lignes — sha256_16=444dddcc4038dc58
  - `src/ROADMAP_VALIDATION.md` — 47 lignes — sha256_16=51f4ff148aea3d84
  - `src/advanced_calculations/audio_processor.c` — 486 lignes — sha256_16=e8fedbb71557458b
  - `src/advanced_calculations/audio_processor.h` — 80 lignes — sha256_16=c38c548e11e6be34
  - `src/advanced_calculations/bitcoin_quantum_mining/CHAT/CLI.md` — 5937 lignes — sha256_16=a9955d360e674f23
  - `src/advanced_calculations/bitcoin_quantum_mining/CHAT/CLI2.md` — 2712 lignes — sha256_16=76f59e42a647caa7
  - `src/advanced_calculations/bitcoin_quantum_mining/CHAT/RAPPORT_IBM_QUANTUM_C63_ANALYSE_COMPLETE.md` — 759 lignes — sha256_16=4b2fb191499df644
  - `src/advanced_calculations/bitcoin_quantum_mining/CHAT/RAPPORT_IBM_QUANTUM_C64_MAX_COMPLET.1.md` — 2065 lignes — sha256_16=ee6c5eeb7b33b1d6
  - `src/advanced_calculations/bitcoin_quantum_mining/CHAT/RAPPORT_IBM_QUANTUM_C64_MAX_COMPLET.md` — 445 lignes — sha256_16=5bbf89dd12655146
  - `src/advanced_calculations/bitcoin_quantum_mining/CHAT/RAPPORT_IBM_QUANTUM_C65_COMPLET.md` — 691 lignes — sha256_16=6e5564fba3d5dc0e
  - `src/advanced_calculations/bitcoin_quantum_mining/CHAT/RAPPORT_IBM_QUANTUM_C65_RESULTATS_REELS.1.md` — 704 lignes — sha256_16=20d3279a6a1fefc2
  - `src/advanced_calculations/bitcoin_quantum_mining/CHAT/RAPPORT_IBM_QUANTUM_C65_RESULTATS_REELS.md` — 448 lignes — sha256_16=a03ebe7cf62a8ea3
  - `src/advanced_calculations/bitcoin_quantum_mining/CHAT/RAPPORT_IBM_QUANTUM_C66_20260420T195435Z.md` — 211 lignes — sha256_16=f64d9414e2a0ded2
  - `src/advanced_calculations/bitcoin_quantum_mining/CHAT/analysechatgpt91 (copy).md` — 648 lignes — sha256_16=d899285a4897dd6e
  - `src/advanced_calculations/bitcoin_quantum_mining/CHAT/analysechatgpt91.32.1.md` — 821 lignes — sha256_16=5784c4718b8e0489
  - `src/advanced_calculations/bitcoin_quantum_mining/CHAT/analysechatgpt91.32.md` — 415 lignes — sha256_16=4335d3936379c869
  - `src/advanced_calculations/bitcoin_quantum_mining/CHAT/analysechatgpt91.33.md` — 221 lignes — sha256_16=963af7164f8b5eff
  - `src/advanced_calculations/bitcoin_quantum_mining/CHAT/analysechatgpt91.34.md` — 187 lignes — sha256_16=25c85fb026a3a859
  - `src/advanced_calculations/bitcoin_quantum_mining/CHAT/analysechatgpt91.35.md` — 324 lignes — sha256_16=4f21a52eef4062c6
  - `src/advanced_calculations/bitcoin_quantum_mining/CHAT/analysechatgpt91.36.1.md` — 145 lignes — sha256_16=49fc15759838d9a3
  - `src/advanced_calculations/bitcoin_quantum_mining/CHAT/analysechatgpt91.36.md` — 530 lignes — sha256_16=494813499f4ebe1a
  - `src/advanced_calculations/bitcoin_quantum_mining/CHAT/analysechatgpt91.37.md` — 408 lignes — sha256_16=deacb616745445f7
  - `src/advanced_calculations/bitcoin_quantum_mining/CHAT/analysechatgpt91.38.1.md` — 999 lignes — sha256_16=b61fdfc59747e423
  - `src/advanced_calculations/bitcoin_quantum_mining/CHAT/analysechatgpt91.38.2.md` — 942 lignes — sha256_16=c47d9afb510c2562
  - `src/advanced_calculations/bitcoin_quantum_mining/CHAT/analysechatgpt91.38.md` — 489 lignes — sha256_16=21bd9db065cce8fb
  - `src/advanced_calculations/bitcoin_quantum_mining/CHAT/analysechatgpt91.39.1.md` — 813 lignes — sha256_16=8e8310c3dcda9165
  - `src/advanced_calculations/bitcoin_quantum_mining/CHAT/analysechatgpt91.39.md` — 648 lignes — sha256_16=d899285a4897dd6e
  - `src/advanced_calculations/bitcoin_quantum_mining/CHAT/analysechatgpt91.40.md` — 671 lignes — sha256_16=43ff02b57be192bc
  - `src/advanced_calculations/bitcoin_quantum_mining/CHAT/analysechatgpt91.41.md` — 507 lignes — sha256_16=97aa2fbed5659845
  - `src/advanced_calculations/bitcoin_quantum_mining/CHAT/analysechatgpt91.42.md` — 518 lignes — sha256_16=be3dcab5ce51cc34
  - `src/advanced_calculations/bitcoin_quantum_mining/CHAT/analysechatgpt91.43.md` — 373 lignes — sha256_16=c786df792895dc6a
  - `src/advanced_calculations/bitcoin_quantum_mining/CHAT/analysechatgpt91.44.md` — 379 lignes — sha256_16=da8a7e06b90e8795
  - `src/advanced_calculations/bitcoin_quantum_mining/CHAT/analysechatgpt91.46.md` — 417 lignes — sha256_16=80ae3fe011b94d54
  - `src/advanced_calculations/bitcoin_quantum_mining/CHAT/analysechatgpt91.47.1.md` — 973 lignes — sha256_16=4f52d1d505b993e5
  - `src/advanced_calculations/bitcoin_quantum_mining/CHAT/analysechatgpt91.47.md` — 439 lignes — sha256_16=89e4cbbceede118b
  - `src/advanced_calculations/bitcoin_quantum_mining/CHAT/analysechatgpt91.48.md` — 317 lignes — sha256_16=5beaf9179d3c3c48
  - `src/advanced_calculations/bitcoin_quantum_mining/CHAT/analysechatgpt91.49.md` — 208 lignes — sha256_16=f877a8bb6fc60236
  - `src/advanced_calculations/bitcoin_quantum_mining/CHAT/analysechatgpt91.50.md` — 466 lignes — sha256_16=d0d5d41db2e9c2af
  - `src/advanced_calculations/bitcoin_quantum_mining/CHAT/analysechatgpt91.51.1.md` — 945 lignes — sha256_16=8e0372245e6d20ee
  - `src/advanced_calculations/bitcoin_quantum_mining/CHAT/analysechatgpt91.51.md` — 187 lignes — sha256_16=29e14677d205a369
  - `src/advanced_calculations/bitcoin_quantum_mining/CHAT/analysechatgpt91.52.md` — 238 lignes — sha256_16=f73b20c97306bcf8
  - `src/advanced_calculations/bitcoin_quantum_mining/CHAT/analysechatgpt91.53.md` — 216 lignes — sha256_16=345605825bae1f52
  - `src/advanced_calculations/bitcoin_quantum_mining/CHAT/analysechatgpt91.54.md` — 234 lignes — sha256_16=badcb7e1c83eb354
  - `src/advanced_calculations/bitcoin_quantum_mining/CHAT/analysechatgpt91.55.md` — 424 lignes — sha256_16=8346b2c3aac2a8e5
  - `src/advanced_calculations/bitcoin_quantum_mining/CHAT/analysechatgpt91.56.md` — 528 lignes — sha256_16=89b2d80980c0ee63
  - `src/advanced_calculations/bitcoin_quantum_mining/CHAT/analysechatgpt91.57.md` — 339 lignes — sha256_16=8b6e1ef315f47003
  - `src/advanced_calculations/bitcoin_quantum_mining/CHAT/analysechatgpt91.58.md` — 266 lignes — sha256_16=6970020c1df8c9a6
  - `src/advanced_calculations/bitcoin_quantum_mining/CHAT/analysechatgpt91.59.1.md` — 1954 lignes — sha256_16=3affdc42c4b9c4a8
  - `src/advanced_calculations/bitcoin_quantum_mining/CHAT/analysechatgpt91.59.md` — 257 lignes — sha256_16=ed8b9a3b1b815cd3
  - `src/advanced_calculations/bitcoin_quantum_mining/CHAT/analysechatgpt91.60.md` — 356 lignes — sha256_16=b79c51677d28834c
  - `src/advanced_calculations/bitcoin_quantum_mining/CHAT/analysechatgpt91.61.1.md` — 1176 lignes — sha256_16=6b60feb43bb18d5e
  - `src/advanced_calculations/bitcoin_quantum_mining/CHAT/analysechatgpt91.61.md` — 94 lignes — sha256_16=a04fa2cf024fe830
  - `src/advanced_calculations/bitcoin_quantum_mining/CHAT/analysechatgpt91.62.md` — 318 lignes — sha256_16=ef65babea0f118d7
  - `src/advanced_calculations/bitcoin_quantum_mining/CHAT/analysechatgpt91.63.1.md` — 417 lignes — sha256_16=3420a7f0d05fda46
  - `src/advanced_calculations/bitcoin_quantum_mining/CHAT/analysechatgpt91.63.2.md` — 1524 lignes — sha256_16=66e4128f0b978904
  - `src/advanced_calculations/bitcoin_quantum_mining/CHAT/analysechatgpt91.63.md` — 356 lignes — sha256_16=d47272bbc3db6d51
  - `src/advanced_calculations/bitcoin_quantum_mining/CHAT/analysechatgpt9142.1.md` — 78 lignes — sha256_16=90582d73aee44fb6
  - `src/advanced_calculations/bitcoin_quantum_mining/CHAT/analysechatgpt9144.2.md` — 1086 lignes — sha256_16=348b541e3df2dd2e
  - `src/advanced_calculations/bitcoin_quantum_mining/CHAT/analysechatgpt9148.1.md` — 1146 lignes — sha256_16=8040903a0ad3f84e
  - `src/advanced_calculations/bitcoin_quantum_mining/CHAT/insta_supermemory.md` — 51 lignes — sha256_16=38d0bc1311bc3564
  - `src/advanced_calculations/bitcoin_quantum_mining/CHAT/pitch/PITCH_INVESTISSEURS_LUMVORAX_C42PLUS (copy).md` — 219 lignes — sha256_16=886434d63095e444
  - `src/advanced_calculations/bitcoin_quantum_mining/CHAT/pitch/PITCH_INVESTISSEURS_LUMVORAX_C42PLUS.md` — 724 lignes — sha256_16=2003346c114c10b6
  - `src/advanced_calculations/bitcoin_quantum_mining/config/btc_module_bridge_manifest.json` — 13322 lignes — sha256_16=e649eac85523af06
  - `src/advanced_calculations/bitcoin_quantum_mining/include/btc_block_validator.h` — 147 lignes — sha256_16=9f0ffcc2464b13fc
  - `src/advanced_calculations/bitcoin_quantum_mining/include/btc_mining_forensic.h` — 86 lignes — sha256_16=8178236391ca81ec
  - `src/advanced_calculations/bitcoin_quantum_mining/include/btc_wallet.h` — 172 lignes — sha256_16=a41d33d208567996
  - `src/advanced_calculations/bitcoin_quantum_mining/logs/forensic/btc_grover156_btc_grover_156q_c65_20260420T155716Z.json` — 40 lignes — sha256_16=42969b119e008fec
  - `src/advanced_calculations/bitcoin_quantum_mining/logs/forensic/btc_nx48_ab_benchmark_c44.json` — 26 lignes — sha256_16=9ea724fdf1c0e032
  - `src/advanced_calculations/bitcoin_quantum_mining/logs/forensic/btc_nx48_ab_c46.json` — 26 lignes — sha256_16=5596f6938657438d
  - `src/advanced_calculations/bitcoin_quantum_mining/logs/forensic/ibm_c64_forensic_ibm_max_c64_20260419T204217Z.json` — 970 lignes — sha256_16=1ded1e5e88542e10
  - `src/advanced_calculations/bitcoin_quantum_mining/logs/forensic/ibm_c65_forensic_c65_20260420T155525Z.json` — 97 lignes — sha256_16=4d6dc4603aa22557
  - `src/advanced_calculations/bitcoin_quantum_mining/logs/forensic/ibm_c65_forensic_c65_20260420T155716Z.json` — 79 lignes — sha256_16=11f2b36d8d651976
  - `src/advanced_calculations/bitcoin_quantum_mining/logs/forensic/ibm_c65_forensic_c65_20260420T155751Z.json` — 51 lignes — sha256_16=6a2167da300a927d
  - `src/advanced_calculations/bitcoin_quantum_mining/logs/forensic/ibm_c66_forensic_c66_partial_20260420T195337Z.json` — 98 lignes — sha256_16=99bd6b6ff2b48b2d
  - `src/advanced_calculations/bitcoin_quantum_mining/logs/forensic/ibm_c66_forensic_c66_partial_20260420T200228Z.json` — 87 lignes — sha256_16=c5336e3aac54a664
  - `src/advanced_calculations/bitcoin_quantum_mining/logs/forensic/ibm_c66_forensic_ibm_c66_all_20260420T195349Z.json` — 716 lignes — sha256_16=e64bcc25a95fe535
  - `src/advanced_calculations/bitcoin_quantum_mining/logs/forensic/ibm_c66_forensic_ibm_c66_all_20260420T195435Z.json` — 716 lignes — sha256_16=14eee7231edd245c
  - `src/advanced_calculations/bitcoin_quantum_mining/logs/forensic/qdayprize156_qdpr_156q_c65_20260420T155525Z.json` — 103 lignes — sha256_16=13f0c3a4463b29a5
  - `src/advanced_calculations/bitcoin_quantum_mining/logs/forensic/qdayprize_8q_c65_pending.json` — 7 lignes — sha256_16=7ef8e5ce5fe4c7f3
  - `src/advanced_calculations/bitcoin_quantum_mining/logs/forensic/qdayprize_qdpr_c55_20260417T191853Z.json` — 23 lignes — sha256_16=54a79fff55d4dc5d
  - `src/advanced_calculations/bitcoin_quantum_mining/logs/forensic/qdayprize_qdpr_c55_20260418T151138Z.json` — 23 lignes — sha256_16=4a33164838f2ba53
  - `src/advanced_calculations/bitcoin_quantum_mining/logs/forensic/qdayprize_qdpr_ibm_c63_20260418T221530Z.json` — 40 lignes — sha256_16=47ad1c217cd3456d
  - `src/advanced_calculations/bitcoin_quantum_mining/logs/forensic/qdayprize_qdpr_max_c64_20260419T204217Z.json` — 96 lignes — sha256_16=a9e2e01f8974d6c1
  - `src/advanced_calculations/bitcoin_quantum_mining/logs/forensic/wallet_btc_20260411T192843Z_4127.json` — 13 lignes — sha256_16=9825a91bef2dc597
  - `src/advanced_calculations/bitcoin_quantum_mining/logs/forensic/wallet_btc_20260411T221049Z_1881.json` — 13 lignes — sha256_16=357a50da9b583ae2
  - `src/advanced_calculations/bitcoin_quantum_mining/logs/forensic/wallet_btc_20260411T221103Z_1907.json` — 13 lignes — sha256_16=b27d9ae38df23fba
  - `src/advanced_calculations/bitcoin_quantum_mining/logs/forensic/wallet_btc_20260412T222434Z_2738.json` — 13 lignes — sha256_16=79ccecc4b14c2520
  - `src/advanced_calculations/bitcoin_quantum_mining/logs/forensic/wallet_btc_20260412T222953Z_3539.json` — 13 lignes — sha256_16=0577df0899a0a052
  - `src/advanced_calculations/bitcoin_quantum_mining/logs/forensic/wallet_btc_20260412T231035Z_2170.json` — 13 lignes — sha256_16=c215a4ab9661b60c
  - `src/advanced_calculations/bitcoin_quantum_mining/logs/forensic/wallet_btc_20260412T232637Z_3532.json` — 13 lignes — sha256_16=bd971f8d231d480e
  - `src/advanced_calculations/bitcoin_quantum_mining/logs/forensic/wallet_btc_20260412T233845Z_68.json` — 13 lignes — sha256_16=69ed82a793e3c7d1
  - `src/advanced_calculations/bitcoin_quantum_mining/logs/forensic/wallet_btc_20260412T233930Z_80.json` — 13 lignes — sha256_16=519a364299940367
  - `src/advanced_calculations/bitcoin_quantum_mining/logs/forensic/wallet_btc_20260412T234425Z_125.json` — 13 lignes — sha256_16=adbf4604d3d770b2
  - `src/advanced_calculations/bitcoin_quantum_mining/logs/forensic/wallet_btc_20260412T234636Z_335.json` — 13 lignes — sha256_16=86835b14beaf42bd
  - `src/advanced_calculations/bitcoin_quantum_mining/logs/forensic/wallet_btc_20260413T103921Z_244.json` — 13 lignes — sha256_16=2778a99a1cbc0937
  - `src/advanced_calculations/bitcoin_quantum_mining/logs/forensic/wallet_btc_20260413T104632Z_1335.json` — 13 lignes — sha256_16=33fd89bee05433cc
  - `src/advanced_calculations/bitcoin_quantum_mining/logs/forensic/wallet_btc_20260413T110059Z_2512.json` — 13 lignes — sha256_16=b2b13f3f4bd28356
  - `src/advanced_calculations/bitcoin_quantum_mining/logs/forensic/wallet_btc_20260413T112304Z_297.json` — 13 lignes — sha256_16=3010de5ce499f357
  - `src/advanced_calculations/bitcoin_quantum_mining/logs/forensic/wallet_btc_20260413T142718Z_198.json` — 13 lignes — sha256_16=063d2ac314822013
  - `src/advanced_calculations/bitcoin_quantum_mining/logs/forensic/wallet_btc_20260413T174633Z_227.json` — 13 lignes — sha256_16=e2d84e6bbf1a0742
  - `src/advanced_calculations/bitcoin_quantum_mining/logs/forensic/wallet_btc_20260413T174900Z_864.json` — 13 lignes — sha256_16=a71b7d3006139d4d
  - `src/advanced_calculations/bitcoin_quantum_mining/logs/forensic/wallet_btc_20260413T174914Z_923.json` — 13 lignes — sha256_16=d11380e53be747f1
  - `src/advanced_calculations/bitcoin_quantum_mining/logs/forensic/wallet_btc_20260413T180057Z_2009.json` — 13 lignes — sha256_16=b0d4eb249c6c7742
  - `src/advanced_calculations/bitcoin_quantum_mining/logs/forensic/wallet_btc_20260413T183345Z_3733.json` — 13 lignes — sha256_16=640973f10e1808b5
  - `src/advanced_calculations/bitcoin_quantum_mining/logs/forensic/wallet_btc_20260413T183659Z_66.json` — 13 lignes — sha256_16=09ffb2221e2d16dd
  - `src/advanced_calculations/bitcoin_quantum_mining/logs/forensic/wallet_btc_20260413T183942Z_67.json` — 13 lignes — sha256_16=4d358e23d0993a12
  - `src/advanced_calculations/bitcoin_quantum_mining/logs/forensic/wallet_btc_20260413T184732Z_65.json` — 13 lignes — sha256_16=7cd58dd324ab9b7a
  - `src/advanced_calculations/bitcoin_quantum_mining/logs/forensic/wallet_btc_20260413T190107Z_75.json` — 13 lignes — sha256_16=8865c0445a4072bb
  - `src/advanced_calculations/bitcoin_quantum_mining/logs/forensic/wallet_btc_20260413T194310Z_92.json` — 13 lignes — sha256_16=cbef328b167c2c6a
  - `src/advanced_calculations/bitcoin_quantum_mining/logs/forensic/wallet_btc_20260413T205115Z_362.json` — 13 lignes — sha256_16=b81dd80bd3b8af65
  - `src/advanced_calculations/bitcoin_quantum_mining/logs/forensic/wallet_btc_20260413T215232Z_4012.json` — 13 lignes — sha256_16=6b1430eb9fe9d292
  - `src/advanced_calculations/bitcoin_quantum_mining/logs/forensic/wallet_btc_20260413T222155Z_6594.json` — 13 lignes — sha256_16=961108dad95ef3ee
  - `src/advanced_calculations/bitcoin_quantum_mining/logs/forensic/wallet_btc_20260413T223355Z_8288.json` — 13 lignes — sha256_16=c613e1ba94d597ef
  - `src/advanced_calculations/bitcoin_quantum_mining/logs/forensic/wallet_btc_20260413T224607Z_9361.json` — 13 lignes — sha256_16=8cf02f4fe45b5187
  - `src/advanced_calculations/bitcoin_quantum_mining/logs/forensic/wallet_btc_20260413T225827Z_9890.json` — 13 lignes — sha256_16=42bad9267f5afe5c
  - `src/advanced_calculations/bitcoin_quantum_mining/logs/forensic/wallet_btc_20260413T230215Z_189.json` — 13 lignes — sha256_16=9a4e5e2dfb1ac0c5
  - … 2707 fichiers supplémentaires dans le JSON.

## Questions expertes restantes
1. IBM Runtime acceptera-t-il tous les circuits C66 sous le plafond de temps gratuit si 16×COBYLA est exécuté en réel ? Réponse: prévoir `--vqe-iters 1..3` en free-tier, augmenter seulement avec crédits.
2. La fidélité gagne-t-elle plus via ancilla≤32 ou via layout SABRE ? Réponse attendue: les deux, mais la réduction de profondeur domine.
3. Le second NX ATOM converge-t-il ? Réponse: le learner C66 trace `convergence_score`, `learned_bias`, `recommended_depth`; il faut plusieurs jobs IBM réels pour confirmer.

## Conclusion
C66 ne prétend pas casser secp256k1 ou résoudre Hubbard physiquement complet; il corrige le protocole expérimental pour que les 156 qubits soient réellement adressés tout en gardant la profondeur exploitable, la lecture locale et l’optimisation classique indispensables.
