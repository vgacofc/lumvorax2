# LUM/VORAX — Quantum Research Platform

## Project Overview
High-performance computational research engine targeting the AIMO3 (AI Mathematical Olympiad) competition and quantum physics simulations (Hubbard model / HTS, Kerr metric black hole dynamics). The core is a C99 multi-module engine compiled with AVX-512/AVX2 SIMD optimizations, with Python scripts for Kaggle integration and result analysis.

## Architecture

### Core Components
- **C Engine** (`src/`): 39+ module quantum simulation framework
  - `src/lum/` & `src/vorax/`: Core LUM/VORAX computation engines
  - `src/advanced_calculations/quantum_problem_hubbard_hts/`: Main research cycle (C37)
  - `src/physics/`: Relativistic physics (Kerr metric, black hole simulations)
  - `src/optimization/`: SIMD/AVX-512 performance optimizations
  - `src/debug/`, `src/logging/`: Forensic nanosecond-precision trackers
  - `src/persistence/`: WAL-based data recovery
  - `src/spatial/`: Displacement metrics
  - `src/network/`, `src/file_formats/`, `src/complex_modules/`: Additional modules

- **Python Scripts**: Kaggle kernel integration
  - `aimo3_lum_v28_proof_kernel.py`: Main competition kernel
  - `aimo3_lum_enhanced_kernel.py`: Enhanced kernel variant
  - `deploy_to_kaggle.py`: Kaggle submission automation

- **Shared Library**: `liblumvorax.so` — Python-callable shared library

### Build System
- `build.sh`: Main build script — auto-detects CPU (AVX-512/AVX2/SSE4.2), compiles all modules
- `Makefile`: Full build rules for all binaries and test suites
- Output: `bin/lum_vorax_complete`, `bin/test_forensic_complete_system`, `bin/test_integration_complete_39_modules`, `bin/test_quantum`

### Workflows
- **Start application**: Builds the full project (`bash build.sh`) then runs the main simulation (`./bin/lum_vorax_complete`)
- **Quantum Research Cycle C37**: Manual long-running research cycle in `src/advanced_calculations/quantum_problem_hubbard_hts/`
  - Has an auto-start guard (`.c37_autorun_disabled` file) — requires manual launch: `C37_AUTORUN_ENABLED=1 bash run_research_cycle.sh`

## Key Technical Notes
- **AVX-512**: Detected and used on Replit; produces fast vectorized builds
- **`-fPIC` required**: All object files must be compiled with `-fPIC` for the shared library target
- **`LUM_DISPLACEMENT_MAGIC`**: Defined in `src/common/magic_numbers.h` as `0xDEADC0DE`
- **Test binary `test_integration_complete_39_modules`**: Requires `-lmvec -lm` due to vectorized `exp()` calls in neural network processor
- **Makefile tabs**: Makefile uses tab indentation for recipe lines (standard make requirement)
- **Kaggle config**: `KAGGLE_USERNAME=ndarray2000`, `KAGGLE_CONFIG_DIR=/home/runner/.kaggle`

## Environment
- Language: C99 (gcc with AVX-512), Python 3.11/3.12
- Nix packages: gcc, gnumake, arrow-cpp, cairo, ffmpeg-full, kaggle, postgresql, and more (see `.replit`)
- Python libs: flask, flask-sqlalchemy, psycopg2, email_validator, gunicorn, numpy, pandas, torch, matplotlib, etc.
