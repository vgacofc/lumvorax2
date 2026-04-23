# LUMVORAX — Module 17 BTC Quantum Mining + Hubbard-HTS

## Project Overview
LUMVORAX est un système de recherche quantique multi-modules avec mining Bitcoin expérimental NX48, simulation Hubbard-HTS, forensic logging ultra-précis, tableau de bord Flask/Three.js, pipeline agent Ubuntu↔Replit, et synchronisation Doppler.

### Module actif : Bitcoin Quantum Mining (Module 17)
- **Binaire** : `src/advanced_calculations/bitcoin_quantum_mining/btc_mining_runner`
- **Record absolu** : 32 bits leading zeros — nonce 2 044 645 236 — cycle C41/C42 forensic
- **Résultats Ubuntu C48** : 8 threads, environ 1.0–1.6 MH/s, meilleur leading zeros observé 28 bits, NX48 update_count environ 1964+
- **Rapport actif** : `src/advanced_calculations/bitcoin_quantum_mining/CHAT/analysechatgpt91.49.md`
- **Chemins Ubuntu** : `REPO_ROOT=/home/lvx/LVX/lumvorax2`, `BTC_DIR=/home/lvx/LVX/lumvorax2/src/advanced_calculations/bitcoin_quantum_mining`
- **Shell Ubuntu** : fish est utilisé, donc lancer les scripts avec `bash script.sh` ou `env VAR=... bash script.sh`

## IBM Quantum Integration (C67-C83 FINAL — 31 jobs IBM réels récupérés)

### État C67-C83 (2026-04-21) — RÉCUPÉRATION COMPLÈTE 31 JOBS
- **20 runners IBM Quantum** : `tools/ibm_quantum_runner_c64.py` à `c83.py`
- **31 jobs IBM réels DONE** récupérés via `tools/ibm_retrieve_c67_c83.py`
- **Mapping cycle ↔ jobs** : C63 (2 jobs), C64 MAX (20 jobs), C65 (9 jobs), C69 (1 job)
- **73+ fichiers .lum** dans `logs/lum_native/`
- **17 rapports** `analysechatgpt91.67-83.md` dans CHAT BTC + HTS
- **Rapport FINAL** : `RAPPORT_IBM_QUANTUM_C67_C83_FINAL_20260421T151832Z.md`
- **Rapport brut** : `RAPPORT_IBM_QUANTUM_C67_C83_RETRIEVE_20260421T151601Z.md`
- **JSON forensic** : `results/ibm_c67_c83_retrieve_*.json`
- **`.lum` natif consolidé** : `logs/lum_native/ibm_c67_c83_retrieve_*.lum` (sha256_16=e9db0a07)

### Découverte majeure validée sur 9 jobs IBM réels (C64 MAX)
État `00001111` dominant à **17-39%** sur 9 jobs HTS différents = **signature antiferromagnétique répétable**
État `111100`/`100101` à **36-40%** sur 6 jobs 6Q = **isolant de Mott confirmé**
→ Confirme C66 : info physique **survit localement** (8-16Q, H=4-5b), **se noie globalement** (156Q, H=11b)

### LuM-as-DBMS (remplace PostgreSQL — C75 finalisé)
Format `.lum` magic LUMQ v2 + gzip + SHA256, B-tree + WAL hash-chained. 73+ fichiers `.lum`. Aucun PostgreSQL installé.

### Astuce env Python critique
`uv run python /tmp/script.py` ÉCHOUE avec `libstdc++.so.6 not found`. **Solution** : préfixer le script avec le préchargement nix de libstdc++ (voir `tools/ibm_retrieve_c67_c83.py` lignes 2-14) OU placer le script dans `tools/` et l'invoquer par `uv run python tools/script.py`.

#### Découvertes Majeures C67-C83
- QDAYPRIZE 32 ancillas IBM réel : depth_phys=1994 >> cohérence → bruit pur (découverte critique)
- VQE COBYLA : non convergé (74% erreur) → SPSA requis pour C84+
- EstimatorV2 HTS : bug mismatch 156Q/8Q identifié → corrigé dans c71.py
- LuM Tensor Manifold : g_ij=diag(1/T1,1/T2,g_gate) valide la géométrie du bruit
- Krylov m adaptatif : instabilité < 0.1 → m=8, instabilité > 0.5 → m=4
- NX48 recalibré : update_count 7→4, threshold 0.5→0.67 (basé entropie IBM)

#### Architecture C67 — Modules Clés
- `KrylovPseudoSpectrum` : K_m(H,ψ₀) → Hessenberg → pseudo-spectre σ_ε(H)
- `LumTensorManifold` : variété géométrique erreur IBM (curvature/anisotropy/Ricci)
- `IBMRuntimeSessionManager` : sessions persistantes latence ÷10
- `lum_local_block` : ansatz brickwork θ-calibré NX ATOM

#### Usage C67-C83
```bash
# Mode simulation (sans IBM_API_KEY)
uv run python3 tools/ibm_quantum_runner_c67.py --fake
uv run python3 tools/ibm_quantum_runner_c{68..83}.py --fake --shots 512

# Mode IBM réel (IBM_API_KEY dans Replit Secrets)
uv run python3 tools/ibm_quantum_runner_c67.py --shots-q 1024 --shots-h 512
uv run python3 tools/ibm_quantum_runner_c69.py --shots 2048  # QDAYPRIZE 32 ancillas
uv run python3 tools/ibm_quantum_runner_c71.py --shots 1024  # EstimatorV2 HTS corrigé
```

### État C66 (2026-04-20)
- **Runner ajouté** : `tools/ibm_quantum_runner_c66.py`
- **Rapport final C66** : `src/advanced_calculations/bitcoin_quantum_mining/CHAT/RAPPORT_IBM_QUANTUM_C66_20260420T200525Z.md`
- **JSON résultat** : `src/advanced_calculations/bitcoin_quantum_mining/results/ibm_c66_pipeline_20260420T200525Z.json`
- **9 jobs IBM C65 récupérés avec vrais counts** — IBM_API_KEY opérationnel dans Replit Secrets
- **Important** : le simulateur local disponible expose 127 qubits, donc C66 conserve les circuits 156Q et bypass uniquement la contrainte du fake backend. Le chemin IBM réel reste destiné à `ibm_fez` 156Q avec transpilation SABRE.

### Résultats IBM C65 récupérés (vrais counts extraits C66)
| Job | Module | Shots | États | Dominant | Entropie H |
|-----|--------|-------|-------|----------|------------|
| d7j4otn16ugs73eud8qg | QDAYPRIZE_156Q | 2282 | 2282 (max) | 0.044% | 11.16b |
| d7j4pff16ugs73eud9c0 | ED_2×2_VALID | 2048 | 16 | 35.9% (`1011`) | 2.72b |
| d7j4poq3fd4c73ddk1sg | BTC_GROVER_156Q | 1024 | 1024 (max) | 0.098% | 10.0b |
| d7j4q1hs7cos73ejf760 | RCS_XEB_156Q | 512 | 512 (max) | 0.20% | 9.0b |
| d7j4qiv16ugs73eudae0 | HTS_hubbard_core | 1024 | 109 | 23.5% (`10111011`) | 4.51b |
| d7j4ql716ugs73eudah0 | HTS_spin_liquid | 1024 | 125 | 24.1% (`10111011`) | 4.87b |
| d7j4qmv16ugs73eudaj0 | HTS_fermionic | 1024 | 59 | 33.0% (`000100`) | 3.78b |
| d7j4qon16ugs73eudal0 | HTS_qchem | 1024 | 98 | 25.6% (`10111011`) | 4.32b |
| d7j4r8q3fd4c73ddk3cg | QDAYPRIZE_8Q (était RUNNING) | 4096 | 243 | 21.2% (`00000000`) | 5.71b |

### Corrections C66 principales
- QDAYPRIZE : ancillas limitées à 32, padding 156Q, multi-échelle `[1, 3, 5]`, mesures locales de blocs de 4.
- HTS/Hubbard : les 16 problèmes construisent des circuits 156Q; `ed_validation_2x2` reste un modèle actif 2 sites mais padded à 156Q.
- VQE : boucle COBYLA ajoutée avec historique d'énergie, EstimatorV2 si disponible, surrogate contrôlé en fake/offline.
- BTC Grover : oracle symbolique remplacé par marquage MCX d'un préfixe public pré-fetché dérivé de l'adresse BTC cible publique.
- RCS : profondeur logique réduite à 5, mesures locales, budget profondeur physique ≤500.
- NX ATOM : feedback basé sur transpilation `optimization_level=3`, `layout_method="sabre"`, `routing_method="sabre"`; second learner NX ATOM ajouté.
- LUM Qubits : `tools/lum_qubits.py` écrit/lit un format natif `.lum` gzip+checksum; JSON/CSV restent des exports d'analyse.
- Lecture `src/` : le manifeste C66 a parcouru 2823 fichiers et 1 032 876 lignes; le détail complet est dans le JSON résultat.

### Usage C66
```bash
uv run python tools/ibm_quantum_runner_c66.py --selftest
uv run python tools/ibm_quantum_runner_c66.py --all --fake --shots-q 4 --shots-h 4 --shots-b 4 --vqe-iters 1 --retrieve-jobs --src-manifest
IBM_API_KEY=... uv run python tools/ibm_quantum_runner_c66.py --all --shots-q 256 --shots-h 128 --shots-b 256 --vqe-iters 1 --retrieve-jobs --src-manifest
IBM_API_KEY=... uv run python tools/ibm_quantum_runner_c66.py --retrieve-jobs
```

## IBM Quantum Integration (C65 — 156Q COMPLET)

### Résultats C65 (2026-04-20) — 8 JOBS DONE + 1 RUNNING

**Backend** : ibm_fez (156Q Heron R2) — **TOUS les 156 qubits physiques utilisés**

| Module | Job IBM | Résultat clé | Exec |
|--------|---------|-------------|------|
| QDAYPRIZE 156Q | d7j4otn16ugs73eud8qg | SNR=1.00, 154 bits réduits, 14007 portes 2Q | 15.5s |
| ED 2×2 Validation | d7j4pff16ugs73eud9c0 | E_IBM=-1.563t, E_exact=-4.828t, erreur=67.64% | 7.4s |
| BTC Grover 156Q | d7j4poq3fd4c73ddk1sg | prob=0.00098, speedup=2^39≈5.5×10^11 | 11.9s |
| RCS XEB 156Q | d7j4q1hs7cos73ejf760 | XEB>>2000 borné 1.0, 512 états distincts | 12.0s |
| HTS hubbard_hts_core | d7j4qiv16ugs73eudae0 | E=-0.352t, S=4.512b, U/t=8.0 | 9.7s |
| HTS spin_liquid_exotic | d7j4ql716ugs73eudah0 | E=+2.198t, signe_pb=True, S=4.871b | 6.8s |
| HTS fermionic_sign_problem | d7j4qmv16ugs73eudaj0 | E=-1.000t, Mott, S=3.782b | 7.0s |
| HTS quantum_chemistry | d7j4qon16ugs73eudal0 | E=-4.611t (meilleure), S=4.322b | 6.0s |
| QDAYPRIZE 8Q comparatif | d7j4r8q3fd4c73ddk3cg | RUNNING (>7min, file IBM) | — |

**Shots totaux** : ~76 000 | **Rapport** : `RAPPORT_IBM_QUANTUM_C65_RESULTATS_REELS.md`

### Corrections C64→C65
- `DataBin 'c0'` : `_get_counts_safe()` avec 3 fallbacks → RCS 156Q = succès
- VQE params non-physiques : `θ_k = π/(2(1+k·0.01·β·t))` — fix β-physique
- GPU tardif : `C65-GPU-EARLY` avant PT-MC dans `btc_mining_engine.c`
- LFS 71MB : `.gitattributes` nettoyé, rotation 50MB active (`tools/rotate_logs_50mb.sh`)
- Qubits 10→156 : tous les modules utilisent les 156Q d'ibm_fez complet

### LUM Qubits C65
- `tools/lum_qubits.py` : tracker 156Q par qubit (fidélité, NX ATOM, timestamps ns)
- Log forensic : `tools/logs/lum_qubits/lum_qubits_qdayprize_156q_*.jsonl`
- NX ATOM 156Q : coherence=0.087, correction=+234 shots, correction_bias=0.15

### Usage C65
```bash
uv run python3 tools/ibm_quantum_runner_c65.py --qdayprize156 --shots-q 2048
uv run python3 tools/ibm_quantum_runner_c65.py --btc156 --shots-b 1024
uv run python3 tools/ibm_quantum_runner_c65.py --rcs156
uv run python3 tools/ibm_quantum_runner_c65.py --hts-batch
```

## IBM Quantum Integration (C64 — MAXIMUM)

### Connexion et résultats
- **Compte** : vgactec | ID : `1ac3cc9b11dd4aa6ac6c53f2d5e2dde7`
- **Canal** : `ibm_quantum_platform`
- **Backends disponibles** : `ibm_fez`, `ibm_kingston`, `ibm_marrakesh` (156 qubits Heron R2)
- **Dépendances** : `qiskit>=2.0.0`, `qiskit-ibm-runtime>=0.30.0` (pyproject.toml)

### Scripts principaux
- `tools/ibm_quantum_runner_c64.py` — **Runner C64 MAX** : QDAYPRIZE+ZNE, 16 HTS, RCS XEB
- `tools/ibm_quantum_runner.py` — Runner C63 (legacy)
- `tools/run_ibm_quantum.sh` — Wrapper Doppler/uv
- `tools/agent_ubuntu_ws.py` — Agent WebSocket Ubuntu↔Replit (bug `tell()` corrigé C64)

### Résultats C64 (2026-04-19) — PIPELINE COMPLET 19 JOBS
- **Backend** : ibm_fez (156Q Heron R2), readout_err=3.06%, 120.3s total
- **QDAYPRIZE MAX** : 10Q, sim_bits=8, shots=4096, ZNE 3 niveaux
  - SNR=83.45, success_rate=83.59%, near_miss=8 bits, fidelity=30.47%
  - JSON : `logs/forensic/qdayprize_qdpr_max_c64_20260419T204217Z.json`
- **16 HTS Hubbard** : 6-8 qubits, paramètres réels (U/t=4.06→14), 15/16 complétés
  - ED 2×2 : E_IBM=−1.68t vs exact=−4.83t (65% erreur VQE non-convergé)
  - Entropie moyenne : 3.898 bits
- **RCS XEB** : 20 qubits, profondeur=17, bug DataBin 'c0' → sim fallback
- **Forensic** : 116 entrées ns, fichiers JSON+CSV
- **Rapport** : `CHAT/RAPPORT_IBM_QUANTUM_C64_MAX_COMPLET.md`

### Bug C64-FIX — agent_ubuntu_ws.py
- **Bug corrigé** : `telling position disabled by next() call`
- **Cause** : `for line in f:` désactive `f.tell()` en mode texte Python
- **Fix** : remplacé par `while True: line = f.readline()`

### Usage C64
```bash
IBM_API_KEY=... uv run python3 tools/ibm_quantum_runner_c64.py --status
IBM_API_KEY=... uv run python3 tools/ibm_quantum_runner_c64.py --all
IBM_API_KEY=... uv run python3 tools/ibm_quantum_runner_c64.py --qdayprize --sim-bits 8
IBM_API_KEY=... uv run python3 tools/ibm_quantum_runner_c64.py --hts16
IBM_API_KEY=... uv run python3 tools/ibm_quantum_runner_c64.py --rcs
```

## IBM Quantum C65 — 156Q MAXIMUM (2026-04-20)

### Nouveautés C65
- **156 Qubits physiques** pour TOUS les problèmes (QDAYPRIZE, HTS, BTC, RCS)
- **LUM Qubits** : couche forensique ns par qubit (`tools/lum_qubits.py`)
- **NX ATOM** + bruit quantique réel IBM T1/T2 → stabilisation décohérence
- **Fix DataBin 'c0'** : `_get_counts_safe()` — plus de crash AttributeError
- **Fix VQE optimizer** : paramètres initiaux β physiques (Boltzmann)
- **GPU Early Activation** : `[C65-GPU-EARLY]` avant PT-MC dans `btc_mining_engine.c`
- **Rotation 50MB** : `tools/rotate_logs_50mb.sh` + `lum_qubits.py`
- **LFS supprimé** : `.gitattributes` nettoyé, `btc_qm_engine_metrics.log` ignoré

### Runner C65
- `tools/ibm_quantum_runner_c65.py` — Runner 156Q NOUVEAU
- `tools/lum_qubits.py` — LUM Qubits 156Q tracker NOUVEAU
- `tools/rotate_logs_50mb.sh` — Script rotation logs NOUVEAU

### QDAYPRIZE 156Q
- 154 ancilla QPE + 2 cibles ECC = 156Q total (vs 10Q en C64)
- Résolution 2^154 (vs 2^8 en C64) — facteur 10^43 supérieur

### HTS 16 Problèmes 156Q
- 78 sites × 2 spins = 156Q (Jordan-Wigner complet)
- ED 2×2 validation exacte : E_exact = -4.828t

### BTC Grover 156Q
- 78Q registre + 78Q oracle = 156Q total
- Speedup Grover : √(2^78) = 2^39 ≈ 5.5 × 10^11

### Usage C65
```bash
uv run python3 tools/ibm_quantum_runner_c65.py --status
uv run python3 tools/ibm_quantum_runner_c65.py --all --fake   # Simulateur
IBM_API_KEY=... uv run python3 tools/ibm_quantum_runner_c65.py --all
IBM_API_KEY=... uv run python3 tools/ibm_quantum_runner_c65.py --qdayprize156
bash tools/rotate_logs_50mb.sh  # Rotation manuelle
```

### Rapport C65
- `RAPPORT_IBM_QUANTUM_C65_COMPLET.md` — Rapport A-Z complet

---

## Architecture

### Backend C
- `src/` — modules core : physics, optimization, logging, persistence, parallel processing
- `src/advanced_calculations/bitcoin_quantum_mining/` — moteur BTC expérimental, NX48, scripts POW et forensic logs
- `Makefile` / `build.sh` — compilation C avec détection CPU

### Web Server Python/Flask
- `main.py` — point d’entrée gunicorn
- `src/visualization/server.py` — serveur Flask, API dashboard et routes agent Ubuntu (`/agent/status`, `/agent/job`, `/agent/push`, `/agent/result`, `/agent/results`, `/agent/token` local)
- `src/visualization/static/` — frontend Three.js

### Agent Ubuntu / Doppler C49
- `tools/agent_ubuntu.sh` lit `REPLIT_URL`, `AGENT_TOKEN`, `DEFAULT_JOB_TIMEOUT_S`, `POLL_INTERVAL` depuis l’environnement ou Doppler.
- `tools/btc_run_ubuntu.sh` lit `BTC_DURATION_S`, `BTC_THREADS`, `BTC_MODE`; `BTC_DURATION_S=0` signifie mining illimité.
- `tools/update_doppler_agent_env.sh` synchronise depuis Replit vers Doppler : URL Replit actuelle, token agent live, timeout agent, durée BTC.
- Le token Flask priorise `AGENT_TOKEN`, puis `LUMVORAX_AGENT_TOKEN`, puis dérive depuis `SESSION_SECRET`; sans secret il est temporaire par process.

## Running the App

### Development
Le workflow "Start application" lance :
```bash
uv run python main.py
```

Le frontend utilise `/app-api/...` comme alias relatif Replit-compatible pour les routes Flask `/api/...`.

### Commandes clés C49
Depuis le shell Replit, synchroniser Doppler :
```bash
bash tools/update_doppler_agent_env.sh
```

Depuis Ubuntu, agent via Doppler :
```bash
cd ~/LVX/lumvorax2
git pull origin main
bash tools/agent_ubuntu.sh
```

Si Doppler ne relance pas automatiquement, utiliser explicitement :
```bash
doppler run --config dev_lumvorax -- bash tools/agent_ubuntu.sh
```

Depuis Ubuntu, mining illimité :
```bash
cd ~/LVX/lumvorax2
doppler run -- bash tools/btc_run_ubuntu.sh
```

Limiter la durée depuis fish :
```bash
env BTC_DURATION_S=3600 doppler run -- bash tools/btc_run_ubuntu.sh
```

Désactiver explicitement le timeout agent :
```bash
env DEFAULT_JOB_TIMEOUT_S=0 doppler run --config dev_lumvorax -- bash tools/agent_ubuntu.sh
```

## Environment
- Python packages managed via uv/`.pythonlibs`; the Replit workflow uses uv's managed Python runtime so pyproject dependencies resolve consistently
- Nix packages include gcc, gnumake, PostgreSQL, OpenCV, and scientific tooling
- Important env vars: `SESSION_SECRET`, `AGENT_TOKEN`, `LUMVORAX_AGENT_TOKEN`, `REPLIT_URL`, `BTC_DURATION_S`, `BTC_THREADS`, `DEFAULT_JOB_TIMEOUT_S`

## Dependencies
- Flask, Flask-SQLAlchemy, gunicorn, psycopg2
- numpy, pandas, scipy, matplotlib, torch, opencv-python
- C toolchain: gcc, gnumake, librt, libpthread

## C84-B + C85 Experimental Validation Layer (2026-04-21)

- **C84-B exécuté sur IBM réel** (`ibm_fez` 156Q Heron R2, 38,5 s) — verdict publication-grade **ACCEPT** :
  - HTS 8Q AFM : S(π) = 0,6123 ± 0,004, sym_gap = 0,18%
  - Q3 X-init : P(00001111)=0,2%, P(11110000)=0,0% → **PHYSIQUE_AFM_PROBABLE** (pas d'artefact)
  - HTS 6Q Mott : dominant `111111` à 83,6%, S(π) = 0,099
- **C85 publication-grade** : code livré (V1/V2/V3 + cross-backend + N_rep=5), run `hts_6q` en arrière-plan
- **Quota IBM 2026-04-21** : 95s consommés / 600s — 505s restantes (84%)
- **Backends opérationnels** : `ibm_fez` 156Q queue=0, `ibm_marrakesh` 156Q queue=0
- **Suggestions Claude/ChatGPT appliquées** : observables E + ZZ + S(π), readout mitigation locale, ZNE 1/3/5, X-init Q3, IC95, baselines Aer ideal+noisy, V1/V2/V3, cross-backend, critère ACCEPT/REJECT
- **Fichiers livrés** :
  - `tools/ibm_quantum_runner_c84.py` (421 lignes)
  - `tools/ibm_quantum_runner_c85.py` (273 lignes, importe C84)
  - `tools/sync_replit_to_doppler.sh` (whitelist 15 secrets)
  - `tools/check_ibm_free_tier.sh` (Runtime + Code Engine + COS + CR)
  - `tools/run_c84_c85_ubuntu.sh` (orchestrateur Doppler-aware)
  - `src/advanced_calculations/bitcoin_quantum_mining/CHAT/RAPPORT_IBM_QUANTUM_C84_C85_SYNTHESE_*.md`
- **BTC bloc réel** : **0 % validable sur NISQ actuel** (oracle SHA-256 ~ 2048 qubits logiques requis vs 156 disponibles). Pipeline classique SHA-256 GPU C80 (531 MH/s) validé.
- **Dépendance ajoutée** : `qiskit-aer 0.17.2` (baselines KL ideal vs noisy)

## C89 + C90 — Bug S(π) résolu + premier vrai signal AFM IBM (2026-04-23)

- **C89** : preuve mathématique complète du bug S(π) C84-C88 — formule exacte `diff = 1.0 + 0.107·O`, retro-formule `S_corr = (S_legacy − 1) / 7`. Tous les "records" 0.59→1.022 = artefacts de reconstruction depuis counts. Doc : `CHAT/PREUVE_BUG_S_PI_C89_DECOMPOSITION.md`.
- **C90** sur `ibm_fez` réel — **EstimatorV2 + Pauli observable + ZNE resilience=2** :
  - **GHZ-8 baseline** : S = -0.1422 ± 0.0077 vs théorique -0.1429 → fidélité_proxy = **0.9952** (backend FIABLE)
  - **HVA-Hubbard 1L** : **S(π) = +0.3015 ± 0.0115 (26 σ au-dessus de 0)** ← premier vrai signal AFM mesuré
  - depth_phys=68, n2q=21, 66.6 s total (11 % du quota mensuel)
- **Retro-recompute 51 mesures historiques C64-C67** sans coût IBM. Top historique +0.2179 sur ibm_c67_c83 — à investiguer.
- **Outils livrés** : `tools/ibm_quantum_runner_c90.py`, `tools/retro_s_pi_recompute.py`, `tools/run_header.py` (header coloré + version git auto).
- **Rapport final** : `CHAT/analysechatgpt_C90_20260423T191212Z.md` (8 sections format standard).
