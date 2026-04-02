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

## Simulation Modules (16 modules — problems_cycle06.csv)

15 Hubbard/QCD/physics modules + 1 new RCS module (C91-RCS, 2026-04-02):
- Modules 1–15: Hubbard, QCD, ED, fermionic sign, worm MC, topological, etc.
- **Module 16**: `random_circuit_sampling` — Random Circuit Sampling (Google Willow/Sycamore protocol), 10×10 qubits, 5000 circuits, XEB benchmarking vs Porter-Thomas distribution.

### RCS Module Files
- `src/random_circuit_sampling.h` — Types `rcs_result_t`, `rcs_problem_t`, API
- `src/random_circuit_sampling.c` — Haar-random gates (1Q+CZ 2Q), XEB scoring, Porter-Thomas KL divergence, full FORENSIC_LOG_MODULE_METRIC op-level tracing
- Makefile updated: `RCS_SRC := src/random_circuit_sampling.c` added to `SRC_RESEARCH_ADV`
- STANDARD_NAMES.md v3.1: Section D added (module 16 canonical names)

## Bug Status (2026-04-02)

| Bug | Status |
|-----|--------|
| C83b/C83c burn_scale ignoré | ✅ Corrigé (acc_energy/acc_count post-burn-in) |
| C89 gap spectral Lanczos | ✅ Corrigé (tridiag_two_lowest dans exact_diagonalization.c) |
| CPU multicoeur 19–33% (séquentiel) | ✅ Corrigé C92 — pool pthread 16 threads dans advanced_parallel.c (Phase 1 : simulate_fullscale() en parallèle, Phase 2 : post-traitement séquentiel) |
| Ops=0 opérations non loggées | ✅ Résolu par module RCS (pattern de référence) |

## Key Environment Variables
- `SUPABASE_URL`, `SUPABASE_ANON_KEY`, `SUPABASE_SERVICE_ROLE_KEY`, `SUPABASE8_API_URL` — Supabase integration (actif ✅)
- `DOPPLER_TOKEN` — Doppler secrets management (actif ✅)
- `KAGGLE_USERNAME`, `KAGGLE_API_TOKEN` — Kaggle datasets
- `ARISTOTLE_API_KEY` — Aristotle API

## Running the App
The "Start application" workflow handles starting the server. After running the "Quantum Research Cycle C37" workflow to generate simulation data, the visualisation will populate with real data.

## STANDARD_NAMES.md
Registre canonique des noms (v3.1 — 2026-04-02). **Règle obligatoire** : chercher avant de créer tout nouveau nom (fonction C, macro, type, métrique). Jamais renommer les noms d'origine.
