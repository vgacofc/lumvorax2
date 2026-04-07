# LUMVORAX — Hubbard-HTS Visualisation

## Project Overview
A high-performance scientific simulation framework (LUMVORAX) focused on black hole physics (Kerr metric), forensic logging, and quantum simulation. The project includes a Flask-based web visualization server for Hubbard-HTS quantum simulation results.

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
python -m gunicorn --bind 0.0.0.0:5000 --reuse-port --reload main:app
```

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
