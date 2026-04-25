# LUMVORAX — Mémoire de session

**Cycle courant : C99** (calibration neuro + ASIC contrôleur + testnet3 RÉEL + DMRG vs IBM, 2026‑04‑25T22:00Z)
- Précédents : C97/C98 — voir `analysechatgpt99.md`, `100.md` (rapport C98), `101.md` (cahier C99), `102.md` (rapport C99)
- C99 NOUVEAUX modules : `src/.../bitcoin_quantum_mining/src/nx48_coupler_bridge.{h,c}` (pont non-intrusif) + `tools/btc_testnet3_fetch.py` + `tools/dmrg_trotter1_compare.py`
- C99 PATCHES critiques : `nx48_neuro_coupler.{h,c}` calibré (gain 30→15 pA, neutre 30 Hz, p90 dynamique, décorrélation 8x8, log JSONL) + `asic_simulation.h`/`asic_quantum_array.c` (+ASIC contrôleur recommend_depth/ansatz/resilience)
- 🏆 Record NX48 conservé : 32 bits LZ (run 190540Z, loss=0.3768)
- 🌐 Testnet3 RÉEL branché : tip height=4 945 523, double-SHA256 vérifié ✅ (results/testnet3_tip_C99.json)
- 📊 DMRG vs IBM C97 : N=8 → S(π)/N=0.7343, N=12 → 0.7247 < IBM C97 0.77-0.81 → **suspicion biais ZNE** (à confirmer N=16/24/32)
- ✅ Smoke C99 (200 steps) : rate 25-38 Hz (régime RS sain), mod ±0.27, STDP w_spread 0.009→0.026, 39/39 near-miss détectés
- ✅ ASIC 3 régimes : HW std → depth=12/rxx_brick/resil=1, HW bruyant → depth=1/shallow_he/resil=3, HW idéal → depth=102/resil=0
- ⚠ Contrainte explicite utilisateur : **NX48 COUPLÉ à Izhikevich+STDP, pas substitué** — respecté C98+C99

## Stack
- Backend : Flask + Flask‑SocketIO sur port 5000 (workflow `Start application` = `python main.py`)
- Visualisation : `src/visualization/server.py` (Flask app principale, endpoint `/agent/*` pour communiquer avec l'agent Ubuntu WebSocket)
- Persistance : LUM DBMS interne (`src/persistence/`) — buffer pool, MVCC, mmap WAL, recovery, transaction WAL, async I/O. **Remplace PostgreSQL**.
- 16 modules C historiques + 3 modules C95 nouveaux (neural_network, blockchain_lumvorax, asic_simulation)
- Compute lourd : Ubuntu lvx-Vostro-5481 (8 cores, 8 GB RAM, OpenCL GPU) via agent WebSocket

## Modules C95 (nouveaux)
- `src/neural_network/` : Izhikevich + HH α/β + AMPA/NMDA/GABA + STDP Song-Miller-Abbott (1 211 lignes)
- `src/blockchain_lumvorax/` : SHA-256 portable + Merkle (double-SHA façon Bitcoin) + block_header + leading_zeros
- `src/asic_simulation/` : modèle pipeline SHA-256 + qubit array T1/T2 + IBM Heron R2 calibré sur IBM_C93_*
- Build : `cd src && make -f Makefile.c95 test` → **30 / 30 OK, 0 warning** avec gcc -Wall -Wextra -Wpedantic -O2 -std=c11

## État expérimental
- **IBM C94** : job `d7lugkdqrg3c738kjg80` DONE sur ibm_kingston (Heron R2). S(π) = **0.9924 ± 0.0015** à N=12, profondeur 14, ZNE 3 facteurs. Confirme C93 (0.9944 ± 0.004 à N=10) → reproductibilité.
- **Run BTC Ubuntu 102103Z** : 228 905 lignes log, 16m27s, 11.04 MH/s mean, record **17 leading zeros**.
- **Run BTC Ubuntu 113648Z** : 226 546 lignes log, 16m19s, 9.82 MH/s mean, record **24 leading zeros** (= QDayPrize cible).
- **Wallets** : 4/4 dérivations BIP32 déterministes cohérentes (TESTNET3, address `mg4hhuNLQwcrL2g2jJamzswgb4ChbZ5tcj`).

## Agent Ubuntu WebSocket
- Cycle C57-WS, fichier `tools/agent_ubuntu_ws.{py,sh}`
- Endpoints sur Replit : `POST /agent/push` (header `X-Agent-Token`), `GET /agent/status`, `GET /agent/results`
- Token dérivé : `python3 -c "import os,hashlib;print(hashlib.sha256(f'agent:{os.environ[\"SESSION_SECRET\"]}'.encode()).hexdigest()[:32])"` → `1842c606981c94e2f93cda8971533bcb`
- Forensic auto-push : thread C63-FOR envoie 200 lignes/30 s (4 streams actifs)
- Démarrage : `cd ~/LVX/lumvorax2 && doppler run --config dev_lumvorax -- bash tools/agent_ubuntu_ws.sh`

## Secrets
- Présents : IBM_API_KEY, IBM_QUANTUM_TOKEN, DOPPLER_TOKEN, DD_API_KEY, SUPERMEMORY_API_KEY, SESSION_SECRET
- **ABSENT** : GITHUB_TOKEN (push GitHub auto impossible depuis Replit)

## Préférences utilisateur
- Réponses **en français**
- Respect strict de `prompt.txt` et `STANDARD_NAMES.md`
- Lecture intégrale ligne par ligne, pas de skip
- Affichage du **% de progression** en temps réel
- Préférence **bout-en-bout avec tests unitaires** (pas seulement cahier des charges)
- Tout le compute lourd en **C** (pas Python pur), Python seulement comme glue/orchestration
- Neurone : Hodgkin-Huxley + Izhikevich acceptés (atome-par-atome impossible, 10¹⁴ atomes)
- Pas de fallback silencieux — l'erreur doit être visible

## Fichiers clés
- `src/advanced_calculations/bitcoin_quantum_mining/CHAT/CAHIER_DES_CHARGES_C95_MAITRE.md` — spec 14 sections
- `src/advanced_calculations/bitcoin_quantum_mining/CHAT/RAPPORT_C95_INTEGRAL.md` — rapport complet C95 (chiffres réels)
- `src/Makefile.c95` — build des 3 modules C95
- `src/advanced_calculations/quantum_problem_hubbard_hts/include/ibm_quantum_constants.h` — constantes IBM_C93_*
- `tools/ibm_c94_retrieve.py` — récupération job IBM (à corriger : doit accepter job_id en arg)
- `tools/agent_ubuntu_ws.{py,sh}` — agent Ubuntu WebSocket

## Limites Replit identifiées
- Sandbox seccomp tue les binaires C lourds après ~60 s → tests longs sur Ubuntu obligatoires
- Workflow `Start application` doit être `python main.py` (PAS gunicorn)

## Prochaines étapes (C98)
1. Patch `btc_nonce_coverage_pct = inf` dans `btc_qm_engine.c`
2. Recalibrer `ibm_heron_r2_model.c` avec F_2q = 0.999 (au lieu de 0.996)
3. Soumettre IBM N=14 ou N=16 (consommer le budget restant ~146 s)
4. Compléter blockchain : `wallet_ed25519.c`, `chain.c`, `consensus_pow_lum.c`
5. Demander GITHUB_TOKEN à l'utilisateur pour activer auto-push GitHub
