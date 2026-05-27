#!/bin/bash
set -euo pipefail

# Configuration
PROJECT_ROOT="/home/runner/workspace"
RUN_ID=$(date -u +%Y%m%d_%H%M%S)
RUN_DIR="src/advanced_calculations/quantum_simulator_v4_kaggle_pack/replit_kaggle_run_${RUN_ID}"

# A. Préparation
mkdir -p "$RUN_DIR"
exec > >(tee -a "$RUN_DIR/execution_commands.log") 2>&1
echo "RUN_DIR=$RUN_DIR"

# B. Exécution pipeline Replit V3
echo "Starting Replit V3 Pipeline..."
if [ -f "src/advanced_calculations/quantum_simulator_v3_staging/run_on_replit_v3.sh" ]; then
    bash src/advanced_calculations/quantum_simulator_v3_staging/run_on_replit_v3.sh "$PROJECT_ROOT" 30 360 1400 || echo "Replit V3 failed"
    LAST_V3_RUN=$(ls -1dt src/advanced_calculations/quantum_simulator_v3_staging/results/* 2>/dev/null | head -n1 || echo "")
    if [ -n "$LAST_V3_RUN" ]; then
        mkdir -p "$RUN_DIR/replit_v3_results"
        cp -r "$LAST_V3_RUN"/. "$RUN_DIR/replit_v3_results/"
    fi
else
    echo "Replit V3 script not found" | tee -a "$RUN_DIR/execution_commands.log"
fi

# C. Notebook Kaggle V4
echo "Generating Kaggle Notebook V4..."
if [ -f "src/advanced_calculations/quantum_simulator_v4_kaggle_pack/tools/generate_kaggle_notebook_v4.py" ]; then
    python3 src/advanced_calculations/quantum_simulator_v4_kaggle_pack/tools/generate_kaggle_notebook_v4.py || echo "Notebook generation failed"
    mkdir -p "$RUN_DIR/notebook"
    cp src/advanced_calculations/quantum_simulator_v4_kaggle_pack/notebook/lumvorax_quantum_v4_kaggle.ipynb "$RUN_DIR/notebook/" 2>/dev/null || echo "Notebook copy failed"
else
    echo "Notebook generator not found"
fi

# D. Push Kaggle
if [ -f "$HOME/.kaggle/kaggle.json" ]; then
    echo "Kaggle credentials detected. Pushing kernel..."
    KDIR="$RUN_DIR/kaggle_submission"
    mkdir -p "$KDIR"
    cp "$RUN_DIR/notebook/lumvorax_quantum_v4_kaggle.ipynb" "$KDIR/" 2>/dev/null || echo "Notebook missing for submission"
    
    cat > "$KDIR/kernel-metadata.json" << 'JSON'
{
  "id": "lumc01/lumvorax-quantum-v4",
  "title": "Lumvorax Quantum V4",
  "code_file": "lumvorax_quantum_v4_kaggle.ipynb",
  "language": "python",
  "kernel_type": "notebook",
  "is_private": true,
  "enable_gpu": true,
  "enable_internet": true,
  "dataset_sources": [],
  "competition_sources": [],
  "kernel_sources": []
}
JSON
    kaggle kernels push -p "$KDIR" | tee "$RUN_DIR/kaggle_push.log" || echo "Kaggle push failed"
else
    echo "Kaggle credentials missing" | tee "$RUN_DIR/kaggle_error.log"
fi

# E. Attente obligatoire
echo "Waiting 59 seconds..."
sleep 59

# F. Récupération outputs/logs Kaggle
if [ -f "$HOME/.kaggle/kaggle.json" ]; then
    echo "Pulling Kaggle results..."
    kaggle kernels pull lumc01/lumvorax-quantum-v4 -p "$RUN_DIR/kaggle_pull" | tee "$RUN_DIR/kaggle_pull.log" || true
    kaggle kernels status lumc01/lumvorax-quantum-v4 | tee "$RUN_DIR/kaggle_status.log" || true
fi

# G. Rapport final
cat > "$RUN_DIR/summary.md" << EOM
# Replit + Kaggle Run Summary
- run_dir: $RUN_DIR
- replit_v3_run: ${LAST_V3_RUN:-N/A}
- kaggle_credentials: $( [ -f "$HOME/.kaggle/kaggle.json" ] && echo yes || echo no )
- wait_seconds: 59
- notes:
  - Replit V3 exécuté en priorité.
  - Outputs/logs copiés localement.
  - Kaggle exécuté si credentials disponibles.
EOM

# H. Affichage
echo "DONE"
echo "Artifacts in: $RUN_DIR"
find "$RUN_DIR" -maxdepth 3 -type f | sort
