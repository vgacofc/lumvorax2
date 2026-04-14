# LUMVORAX — Module 17 BTC Quantum Mining + Hubbard-HTS

## Project Overview
LUMVORAX — Système de recherche quantique multi-modules avec mining Bitcoin expérimental (NX48), simulation Hubbard-HTS, forensic logging ultra-précis, et intégrations Supabase/Supermemory/Aristocle.

### MODULE ACTIF : Bitcoin Quantum Mining (Module 17)
- **Binaire** : `src/advanced_calculations/bitcoin_quantum_mining/btc_mining_runner` — v1.0.0-C41 (106K)
- **Record absolu** : 32 bits leading zeros — nonce 2 044 645 236 — cycle C41
- **Workflow** : "Quantum Research Cycle C37" — RUN INFINI vers 256 bits
- **Secrets** : Doppler (priorité) → Replit (fallback) — projet lumvorax / dev_lumvorax
- **Supermemory** : container lumvorax_nx48 (662+ docs) — endpoint POST /v3/documents
- **Supabase** : tables btc_mining_runs, btc_records, btc_metrics_realtime
- **Aristocle** : ARISTOCLE_IA_API_KEY — URL aristotle.harmonic.fun (à confirmer)
- **Rapport** : src/advanced_calculations/bitcoin_quantum_mining/CHAT/analysechatgpt91.39.md

## Architecture

### Backend (C)
- `src/` — Core C modules: physics, optimization, logging, persistence, parallel processing
- `Makefile` — Build system supporting debug/release modes with SIMD auto-detection
- `build.sh` — Build wrapper with CPU capability detection (AVX-512, AVX2, SSE4.2)
- Compiled binary: `bin/lum_vorax_complete`

### Web Server (Python/Flask)
- `main.py` — Entry point, imports Flask app from `src/visualization/server.py`
- `src/visualization/server.py` — Flask server exposing simulation data via REST API
- `src/visualization/static/` — Frontend assets (HTML/CSS/JS with Three.js visualization)

### Key Directories
- `src/advanced_calculations/quantum_problem_hubbard_hts/` — Quantum simulation module
- `logs/`, `evidence/` — Simulation output and forensic audit trails
- `docs/` — Documentation and Doxygen configs

## Running the App

### Development
The "Start application" workflow runs:
```
python -m gunicorn --bind 0.0.0.0:5000 main:app
```

The browser frontend uses `/app-api/...` as a Replit-compatible alias for the existing Flask `/api/...` routes.

### Deployment
Configured for Autoscale deployment via gunicorn on port 5000.

## Environment
- Python packages managed via uv/`.pythonlibs`
- Nix packages: gcc, gnumake, postgresql, opencv, etc.
- Kaggle credentials: `KAGGLE_USERNAME`, `KAGGLE_CONFIG_DIR`

## Dependencies
- Flask, Flask-SQLAlchemy, gunicorn, psycopg2
- numpy, pandas, scipy, matplotlib, torch, opencv-python
- C toolchain: gcc, gnumake, librt, libpthread
