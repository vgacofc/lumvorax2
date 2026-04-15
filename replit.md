# LUMVORAX — Module 17 BTC Quantum Mining + Hubbard-HTS

## Project Overview
LUMVORAX — Système de recherche quantique multi-modules avec mining Bitcoin expérimental (NX48), simulation Hubbard-HTS, forensic logging ultra-précis, et intégrations Supabase/Supermemory/Aristocle.

### MODULE ACTIF : Bitcoin Quantum Mining (Module 17)
- **Binaire** : `src/advanced_calculations/bitcoin_quantum_mining/btc_mining_runner` — v1.0.0-C44 (117K, 2026-04-15T17:57Z)
- **Record absolu** : 32 bits leading zeros — nonce 2 044 645 236 — cycle C41/C42 forensic
- **C44 run 2026-04-15T17:58Z** : 20 bits — 0.4365 MH/s (NX48 off) / 0.4344 MH/s (NX48 on) — +28% vs C43
- **Nouveau Supabase** : tables à créer via `tools/supabase_c44_setup.sql` dans SQL Editor — corriger SUPABASE_DB_PASSWORD
- **Mémoire dual-agent NX48** : Agent1 temps réel C (`nx48_btc_controller.c`) + Agent2 long terme (`tools/nx48_supermemory.py`)
- **NX48 C44** : 2 neurones — 3 updates en 8s — stall_count=2 — loss=0.834 — dual_blend=0.200
- **Validation POW C44** : `scripts/validate_pow_candidate.py --context config/btc_getblocktemplate_job.json [--submit]`
- **Supabase** : SERVICE_ROLE 200 OK — Tables absentes (nouveau compte) — SQL setup: `tools/supabase_c44_setup.sql`
- **Rapport actif** : `src/advanced_calculations/bitcoin_quantum_mining/CHAT/analysechatgpt91.43.md`

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
- `tools/nx48_supermemory.py` — mémoire longue durée NX48 BTC Supermemory/Supabase
- `tools/nx48_module_bridge.py` — manifeste de pont modules `src/` vers le moteur BTC, avec `kind_counts` et mode advisory forensic
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
