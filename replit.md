# LUMVORAX — Mémoire de session

**Cycle courant : C108** (validation forensique live + audit anomalies, 2026‑04‑27T17:00Z)
- ✅ **Lecture intégrale** : prompt.txt + STANDARD_NAMES.md + analysechatgpt105/106/106_PART2/107/107.1 + AUDIT_LUM_VORAX_VALIDATION_C108 + c105_p0_p1_q1_q6_replit_ibm + analysechatgpt21 (format ref)
- ✅ **Patches C107 vérifiés en source** : signature `best_nonce` (controller.c L731), `nx48_alltime_try_update` (L881), header binding sidecar (L1189-1202), passage du global engine (L917). Aucune régression.
- ✅ **Mainnet live confirmé** : tip 946902 (blockstream + mempool), wallet `1YkQrHMbvBbYvCR1jcQAxjMj4bzibiK8C` tx_count=0 (vierge, attendu)
- ✅ **CSV alltime canonique** : best_lz=24, nonce=3563405791, update_count=10, wallet mainnet OK
- ✅ **Overhead LUM ON/OFF mesuré** : 14.8 ns/op OFF vs 15.1 ns/op ON = +2 % (n=200 000, micro-bench isolé tracker pur)
- ✅ **Submitblock RPC** : implémenté ligne 121 `validate_pow_candidate.py` — bloqué par `BTC_RPC_URL` absent (= bitcoind absent Ubuntu)
- ✅ **OpenCL Makefile** : auto-détecté (`pkg-config --exists OpenCL` L62) — actif sur Ubuntu UHD 620 (9.83 MH/s prouvé), inactif Replit
- 🔴 **A4 confirmée live** : Doppler `ADDRESS_P2PKH = tb1qlm4tvk…` (= bech32 testnet, pas P2PKH mainnet) — anomalie nommage
- 🔴 **A5 nouvelle** : `nx48_alltime_records.jsonl` contient 2 records test orphelins (`tb1qtest`, `deadbeef`) — endpoint `/agent/nx48_alltime` non protégé par header binding
- 🔴 **A6 actuelle** : agent WS Ubuntu déconnecté (`queue_len=0, results_count=0`) — utilisateur doit relancer `bash ~/agent_ubuntu_ws.sh`
- ⏸ **IBM** : compte gelé inchangé (`account.frozen=true` JWT) — TOUS jobs IBM laissés pending, conformément consigne user
- ✅ **Anciens rapports MD5 INTACTS** (vérifié 17:00Z) : 105 + 106 + 106_P2 + 107 + 107.1 + AUDIT_C108 + RAPPORT_VALIDATION_C106 + c105_p0_p1
- 📋 **Nouveau rapport** : `CHAT/analysechatgpt108.md` (384 L, MD5 `b8ff8f7b0bca3a562574417370e5e14d`, NEUF, NE JAMAIS modifier)
- 📐 **Commandes Replit + Ubuntu fish** : §7 du rapport C108 (agent WS, bitcoind testnet4, OpenCL UHD 620, submitblock, push job)

---

**Cycle précédent : C107 PATCH 2** (best DYNAMIQUE auto-MAJ + nx48_alltime intégré, 2026‑04‑27T14:08Z)
- ✅ **C107 P1** : Sidecar `.lum.header` (header binding) + reset auto si mismatch (nx48_btc_controller.c L1061-1102)
- ✅ **C107 P2** : Bug HISTORIQUE C92.2 corrigé → signature `nx48_btc_update` étend `uint32_t best_nonce` (controller.h L269 + .c L734 + engine.c L920)
- ✅ **try_update intégré** : à chaque NEW RECORD, appel `nx48_alltime_try_update()` avec header env + wallet env (controller.c L880-903)
- ✅ **Validation live mainnet 946883 (10s)** : 10 NEW RECORDS monotones consécutifs LZ=4→8→9→10→11→12→16→19→20→**24**, CSV alltime écrit avec wallet réel `1YkQrHMbvBbYvCR1jcQAxjMj4bzibiK8C` + update_count=10
- ✅ **Best EST DYNAMIQUE** (exigence utilisateur) — fini le 37 figé du 2026-04-26, le best monte progressivement et auto-MAJ via header binding
- ✅ **Anciens rapports MD5 INTACTS** : 106 (41bfb438) + 106_PART2 (36cdb09e) + VALIDATION_C106 (4310690e) — jamais touchés
- 📋 **Rapport** : `CHAT/analysechatgpt107.md` (419 lignes, NEUF, NE JAMAIS modifier)
- 📡 **Job WS Ubuntu** `0ef2f5eed462` (env INLINE corrigé) — git pull → rebuild C107 → run 30s mainnet → CSV final

---

**Cycle précédent : C106-PART2** (corrections + WS Ubuntu + tokens + procédure réseau, 2026‑04‑27T13:48Z)
- ✅ **3 rapports nouveaux** (anciens jamais modifiés) :
  - `CHAT/analysechatgpt106.md` (PART1 — cycle principal, NON modifié)
  - `CHAT/RAPPORT_LUM_VORAX_VALIDATION_C106.md` (sections 1–8 prompt validation, NON modifié)
  - `CHAT/analysechatgpt106_PART2.md` (NEUF — corrections + tokens + WS Ubuntu + procédure mainnet)
- ✅ **A1 RÉSOLUE** par config : `doppler run -- ./btc_mining_runner` charge `BTC_WALLET_PRIV_HEX` Doppler ⇒ wallet **FIXE `1YkQrHMbvBbYvCR1jcQAxjMj4bzibiK8C`** confirmé live (run `btc_C106P2_BIP34`)
- ✅ **A2 CORRIGÉE** (mauvais diagnostic PART1) : `submitblock` EXISTE déjà dans `scripts/validate_pow_candidate.py:120` (Python via RPC). Pipeline complet `run_btc_infinite.sh` + doc `tools/btc_ubuntu_rpc_setup.md`
- ✅ **A3 PATCHÉE** : `btc_block_validator.c:281` lit env `BTC_TIP_HEIGHT`/`BTC_COINBASE_HEIGHT` au lieu de hardcode 0. Recompile OK 170 256 bytes
- ✅ **Agent WS Ubuntu Vostro 5481 prouvé** : 2 jobs returncode=0 (whoami + recon Doppler), kernel 6.17.0-22, repo `f58e826`, **PAS de GPU NVIDIA**, doppler `/usr/bin/doppler`, `BTC_WALLET_ADDRESS` Doppler partagé OK
- 🔴 **A4 NOUVELLE** : `ADDRESS_P2PKH` Doppler = `tb1qlm4tvk…` (bech32 testnet, pas P2PKH) — nommage trompeur, à renommer
- 📊 **Tokens mainnet identifiés** : trio Doppler `BTC_WALLET_ADDRESS` + `BTC_WALLET_PRIV_HEX` + `BTC_WALLET_WIF`. Verif live blockstream : tx_count=0, sats=0 (wallet vierge, jamais minté = attendu)
- 📊 **58 secrets Doppler** listés exhaustivement dans PART2 §2
- 📊 **Tip mainnet** PART1=946882 → PART2=946883 (+1 bloc miné par d'autres, 90 min écoulées)
- 🔴 **IBM Quantum compte GELÉ** (inchangé — `account.frozen=true` JWT). **Action user : dégeler sur cloud.ibm.com**
- 📐 **Verdict LUM+VORAX** (PART1) : (A) PASS, (B) INVALIDÉ no-cloning, (D) overhead CONFIRMÉ
- 📋 **Procédure réseau** complète dans PART2 §3 + §7 (3 niveaux blockstream + Bitcoin Core RPC local)

**Cycle précédent : C99 partie 5 / C99-105** (P0.1 hook + Q4 + Q6 + Goals partiels, 2026‑04‑26T22:10Z)
- ✅ **P0.1 coupler hook intégré** dans `nx48_btc_controller.c` L574-614 (modulation ±15% borne [0.05, 0.95])
- ✅ **Q4 ASIC↔VORAX bridge** : `src/advanced_calculations/asic_lumvorax_bridge.c` (loss multi-terme α·||features||² + β·energy + γ·||ψ||₂²) — test 4/4 OK
- ✅ **Q6 Blockchain LUMVORAX** : `genesis.c` + `chain.c` + `consensus_pow_lum.c` compilent ; bloc 0 reproductible Replit↔Ubuntu (LZ=16, nonce=12140, hash=`00009ed5...1470af30`)
- ✅ **Goal #16 partiel** : `tools/qdayprize_escalation.py` + binaire C natif Ubuntu — 24+28 bits trouvés, 32-40 timeout (rate 0.30 MH/s OpenSSL → besoin sha256_lumvorax 8.95 MH/s pour 32+)
- ✅ **Goal #17 partiel** : `hubbard_16_problems_runner.c` 4/16 problèmes (P01-P04 ED+DMRG OK), 12 restants
- ⏳ **P0.2 IBM ADAPT-VQE** + **P0.3 testnet3 runtime** + **Goal #18 GPU** : à finaliser C99-106
- 📚 NOUVEAU rapport : `CHAT/analysechatgpt99_105.md` (220 L, structure complète C99 état)

**Cycle précédent : C99 partie 2** (IBM RÉEL exécuté + bridge en prod + rotation log 50 MB, 2026‑04‑25T23:15Z)
- 🎯 **IBM Heron R2 ibm_fez resilience_lvl=1** : N=8 S(π)/N=**0.6935** (job d7miop43g2mc7392hl10), N=12 S(π)/N=**0.6810** (job d7miq0s3g2mc7392hm90)
- 🏆 **IBM Heron R2 ibm_fez resilience_lvl=2 (RÉVÉLATION C99 P2)** : N=16 S(π)/N=**0.7155** (job d7mitfit99kc73d2j3ng) ≈ DMRG noiseless 0.7199 (écart <1%)
- 🚨 **C97 ZNE+twirl 0.77-0.81 = BIAIS POSITIF +0.06 SYSTÉMATIQUE** prouvé : le "plateau AFM" C97 N=16-96 n'existe PAS physiquement, artefact ZNE pairwise sur-correctif
- 🔬 **État de l'art VRAI Trotter-1 N=16 Heron R2 = 0.72 ± 0.005** (pas 0.79 comme C97 le prétendait)
- 💰 Quota IBM consommé C99 P2 ≈ 350 s (4 jobs traçables sur portail IBM Quantum)
- ✅ Patch prod `btc_mining_engine.c` (4 hunks, 24 L) : include bridge + init + modulation ±30% + cleanup, garde `BTC_NX48_COUPLER=1`
- ✅ Module `src/common/lvx_log_rotate.{h,c}` : rotation 50 MB testée empiriquement (60 MB → archive auto YYYYMMDDTHHMMSSZ + purge LRU keep_n=5)
- 📚 Rapport partie 2 : `src/.../bitcoin_quantum_mining/CHAT/analysechatgpt103.md` (commandes exactes Replit/Ubuntu fish/IBM/modules anciens)
- Précédents : C97/C98/C99-P1 — voir `analysechatgpt99.md` à `103.md`
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

## C105 — Finalisation C99 (2026-04-26)

**Avancement global C99 : 88 %** (+18 pts vs C104)

### Livrables NOUVEAUX
- ✅ **P0.3 testnet3 RUNTIME C** : `btc_testnet3_runtime.{h,c}` lié au binaire 167K
  - Smoke test live : height=4 947 531, hash `0000000000a74e9f…57` (40 bits LZ)
  - SHA256d vérifié, `hash <= target` confirmé (target `…06bdf1…`, diff=37.97)
- ✅ Script `tools/adapt_vqe_p02_batch.py` (P0.2 IBM ADAPT-VQE multi-N) — exec Ubuntu
- ✅ Script `tools/nx48_ab_wilcoxon.py` (P1.2/Q2 5 paires Wilcoxon, randomisé, état réinit) — exec Ubuntu
- ✅ Script `tools/openmm_nav15_run.py` (Q3 OpenMM PDB 6AGF, CPU 100ps recommandé)
- ✅ Rapport NOUVEAU : `CHAT/c105_p0_p1_q1_q6_replit_ibm.md`

### Métriques RÉELLES Replit
- Hardware : AMD EPYC 9B14 @ 2.6 GHz, 6 cores, 62 GB RAM, NixOS 6.14.11
- Bench BTC 12s : **0.44 MH/s** (1 thread CPU pur), best_lz=37 préservé via C100

### Bloqueurs identifiés
- Replit Qiskit cassé (numpy/Py3.13 ImportError) → IBM via Ubuntu
- IBM Cloud HTTP : Cloudflare 1010 bloque datacenter Replit (IAM OK mais /backends 403)
- Agent WS Ubuntu : déconnecté (queue 80 jobs en attente, transport=http_queue)

### Cohérence rapports antérieurs (NON modifiés)
- 100 (C98 hook) ✅ déjà fait via engine.c (pas controller.c) — confirmé C99 P2
- 103 (C99 P2 IBM RÉEL + P0.1) ✅
- 104 (C100 persistance monotone, nx48_alltime_record best=37) ✅
