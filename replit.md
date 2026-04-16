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
uv run gunicorn --bind 0.0.0.0:5000 --reload main:app
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
doppler run -- bash tools/agent_ubuntu.sh
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
env DEFAULT_JOB_TIMEOUT_S=0 doppler run -- bash tools/agent_ubuntu.sh
```

## Environment
- Python packages managed via uv/`.pythonlibs`
- Nix packages include gcc, gnumake, PostgreSQL, OpenCV, and scientific tooling
- Important env vars: `SESSION_SECRET`, `AGENT_TOKEN`, `LUMVORAX_AGENT_TOKEN`, `REPLIT_URL`, `BTC_DURATION_S`, `BTC_THREADS`, `DEFAULT_JOB_TIMEOUT_S`

## Dependencies
- Flask, Flask-SQLAlchemy, gunicorn, psycopg2
- numpy, pandas, scipy, matplotlib, torch, opencv-python
- C toolchain: gcc, gnumake, librt, libpthread
