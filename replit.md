# LUM/VORAX — Quantum Research Platform

## Project Overview
High-performance quantum simulation and research platform targeting Hubbard model / HTS physics. Combines a C99 computation engine with a Python/Flask visualization web server.

## Architecture

### Core Components
- **C Engine** (`src/`): Multi-module quantum simulation framework
  - `src/lum/` & `src/vorax/`: Core LUM/VORAX engines
  - `src/advanced_calculations/quantum_problem_hubbard_hts/`: Main research cycle (C37)
  - `src/physics/`: Relativistic physics (Kerr metric, black hole simulations)
  - `src/optimization/`: SIMD/AVX performance optimizations
  - `src/debug/`, `src/logging/`: Forensic memory and execution trackers
  - `src/persistence/`: WAL-based data recovery

- **Visualization Server** (`src/visualization/`): Flask web app serving real-time simulation data
  - `server.py`: REST API endpoints exposing simulation results as JSON
  - `static/index.html`: Three.js-based 3D visualization frontend
  - `static/three.min.js`: Three.js library

- **Entry Point** (`main.py`): Loads libstdc++ then imports and runs the Flask visualization app

### Workflows
- **Start application**: Runs the Flask visualization server via gunicorn on port 5000
  - Command: `PYTHONPATH=/home/runner/workspace/.pythonlibs/lib/python3.12/site-packages .pythonlibs/bin/gunicorn --bind 0.0.0.0:5000 --reuse-port --reload main:app`
- **Quantum Research Cycle C37**: Background C simulation process (long-running)

## Key Technical Notes
- Python packages are installed in `.pythonlibs/` — must set `PYTHONPATH` explicitly when invoking scripts
- The visualization server was originally in `src/visualization.desactive/` (deactivated); activated at `src/visualization/`
- C simulations write results to `src/advanced_calculations/quantum_problem_hubbard_hts/results/`
- The Flask server reads those result files via CSV/log parsing to expose them through the API

## Dependencies
- **Python**: flask, flask-sqlalchemy, gunicorn, psycopg2, email-validator, numpy, scipy, etc.
- **C**: gcc with -O3/-march=native/-flto, pthread, libm, librt
- **Frontend**: Three.js (bundled in static/)

## API Endpoints
- `GET /` — Visualization dashboard
- `GET /api/run/latest` — Latest simulation run results
- `GET /api/benchmark_ref` — QMC/DMRG reference benchmarks
- `GET /api/viz/scalar_field` — 3D scalar field data
- `GET /api/viz/trajectories` — Step-by-step simulation trajectories
- `GET /api/viz/lattice` — Hubbard lattice site data
- `GET /api/viz/graph` — Module interaction graph
- `GET /api/viz/multiscale` — Thermodynamic limit extrapolations
