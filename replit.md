# Repl-Nix-Workspace — Quantum Research Project

## Overview
A Python-based scientific research environment focused on quantum physics and high-temperature superconductor (HTS) research. The project includes:
- Hubbard model / HTS simulation kernels (C source in `src/advanced_calculations/quantum_problem_hubbard_hts/`)
- Vesuvius challenge / competition kernels (`nx47_vesu_kernel_*.py`)
- ARC challenge research (`nx47-arc-kernel.py`)
- Kaggle submission tooling (`deploy_to_kaggle.py`, `kernel_to_push.py`)
- Scientific report generators (`generate_scientific_report.py`, `generate_performance_report.py`, etc.)
- LumVorax integration framework

## Architecture
- **Language**: Python 3.12
- **Package manager**: uv (with pyproject.toml)
- **Key dependencies**: numpy 1.26.4, scipy, torch (CPU), pandas, matplotlib, scikit-image, pyarrow, psycopg2-binary, kaggle, pillow, aiohttp
- **External DB**: Supabase PostgreSQL (credentials in environment variables)
- **nix packages**: arrow-cpp, cairo, clang, ffmpeg-full, gcc, ghostscript, gtk3, kaggle, libGL, and more (see replit.nix section in .replit)

## Important: libstdc++ Fix
Torch requires `libstdc++.so.6` to be preloaded before import. `main.py` handles this automatically by loading from the nix store path:
`/nix/store/bmi5znnqk4kg2grkrhk6py0irc8phf6l-gcc-14.2.1.20250322-lib/lib/libstdc++.so.6`

Any script that imports torch must either:
1. Import and run `main.py` first, OR
2. Preload the library manually via `ctypes.CDLL(path)` before `import torch`

## Workflows
- **Run Python**: Runs `python main.py` (entry point / smoke test)
- **Quantum Research Cycle C37**: Runs the full C-based HTS research cycle via `bash run_research_cycle.sh`
- **LumVorax Visualisation**: Serveur Flask (port 5000) — module de visualisation 3D/2D des simulations Hubbard-HTS

## Module de Visualisation (`src/visualization/`)
- **server.py** : Serveur Flask exposant les données réelles des runs via API REST (`/api/viz/*`)
- **static/index.html** : Interface Three.js r128 avec 5 modes de visualisation :
  1. **Champs Scalaires** → volume heatmap 3D (simulate_fs: energy_eV / pairing)
  2. **Trajectoires** → curves (simulate_fs: step_energy_norm_step0)
  3. **Réseau Hubbard** → instancing (pairing d-wave, spin, bonds t_eV)
  4. **Graphe d'Interaction** → nodes + edges (benchmark_adv: QMC/DMRG + external_ref)
  5. **Multi-Échelles LOD** → fractal (cluster_scale 8×8 → 255×255, thermodynamic_limit)
- Noms canoniques : conformes STANDARD_NAMES.md v3.0 (C68-REALTIME-BENCH)
- Fallback Canvas 2D si WebGL non disponible
- Données réelles : problems_cycle06.csv, qmc_dmrg_reference_runtime.csv, research_execution.log

## Corrections C37 (2026-03-25)

### Anomalies corrigées dans `advanced_parallel.c` et `hubbard_hts_research_cycle.c`

1. **Guard NaN d2 + contamination ring buffer** (P3)
   - Ajout d'un ring buffer de 20 valeurs filtrées pour détecter les spikes >5σ
   - Correction du bug de contamination : stockage de `d2_out` (filtré) au lieu de `d2` brut
   - `FORENSIC_LOG_ANOMALY` émis si spike détecté
   - Appliqué dans les deux fichiers C

2. **Fichiers benchmark runtime générés** (P4 — RMSE=1e9 corrigé)
   - Cause racine : `load_benchmark_rows()` ne parsait pas les colonnes text (reference_method, source)
   - Correction : génération de `benchmarks/qmc_dmrg_reference_runtime.csv` (10 lignes) et `benchmarks/external_module_benchmarks_runtime.csv` (7 lignes) au format simplifié 6 colonnes
   - Le code C cherche d'abord `*_runtime.csv` → trouvé → RMSE réel calculé

3. **P5 pairing_gain : tri-état PASS/WARN/FAIL** (P5)
   - Gain négatif → WARN (pas FAIL) + `FORENSIC_LOG_ANOMALY`
   - Permet de discriminer bruit MC vs inefficacité réelle du feedback

4. **τ_int Sokal + N_eff** (P6)
   - Nouvelle fonction `compute_tau_int_sokal()` (méthode Sokal 1996, coupure auto ρ<0)
   - N_eff = N/(2τ_int) écrit dans `numeric_stability.csv`
   - Seuils : N_eff≥30 → PASS, ≥10 → WARN, <10 → FAIL

5. **Module `tools/supabase_client.py` centralisé**
   - Interface unique pour SELECT/INSERT/UPSERT Supabase
   - Méthode `generate_benchmark_runtime_csv()` pour régénérer les fichiers depuis Supabase

## Environment Variables (set in .replit)
- `KAGGLE_USERNAME`, `KAGGLE_API_TOKEN`, `KAGGLE_CONFIG_DIR`
- `SUPABASE_URL`, `SUPABASE_DB_*`, `SUPABASE_ANON_KEY`, `SUPABASE_SERVICE_ROLE_KEY`
- `SUPABASE8_API_URL` : URL PostgREST Supabase (https://mwdeqpfxbcdayaelwqht.supabase.co)
- `ARISTOTLE_API_KEY`
