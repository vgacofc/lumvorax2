# LumVorax — Hubbard-HTS Quantum Research Visualisation

## Overview
A quantum physics research visualization app for Hubbard model / High-Temperature Superconductor (HTS) simulations. The Flask backend serves real simulation data via a REST API, and the frontend uses Three.js to render interactive 3D visualisations.

## Architecture

- **`main.py`** — Entry point. Loads the Flask app from `src/visualization/server.py`.
- **`src/visualization/server.py`** — Flask server exposing REST API endpoints at `/api/viz/*`, `/api/run/*`, `/api/benchmark_*`, `/api/problems`.
- **`src/visualization/static/index.html`** — Three.js frontend with 5 visualisation modes:
  1. Champs Scalaires — 3D volume heatmap
  2. Trajectoires — Step-by-step curves
  3. Réseau Hubbard — Lattice site instancing
  4. Graphe d'Interaction — Nodes + edges (QMC/DMRG benchmarks)
  5. Multi-Échelles LOD — Fractal multi-scale extrapolation
- **`src/advanced_calculations/quantum_problem_hubbard_hts/`** — Quantum simulation C code and research cycle runner (`run_research_cycle.sh`).

## Workflows

- **Start application** — `gunicorn` on port 5000 (webview). Command uses the full nix Python 3.12 path to ensure `.pythonlibs` packages are accessible.
- **Quantum Research Cycle C37** — Runs `run_research_cycle.sh` to execute quantum simulations and write results to `results/`, `benchmarks/`, and logs.

## Data Flow
Simulation runs write CSV and log files under `src/advanced_calculations/quantum_problem_hubbard_hts/results/research_*/`. The Flask server reads these files dynamically for each API call.

## Key Environment Variables
- `SUPABASE_URL`, `SUPABASE_ANON_KEY`, `SUPABASE_SERVICE_ROLE_KEY` — Supabase integration
- `KAGGLE_USERNAME`, `KAGGLE_API_TOKEN` — Kaggle datasets
- `ARISTOTLE_API_KEY` — Aristotle API

## Running the App
The "Start application" workflow handles starting the server. After running the "Quantum Research Cycle C37" workflow to generate simulation data, the visualisation will populate with real data.
