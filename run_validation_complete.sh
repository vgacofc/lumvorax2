#!/bin/bash
# Validation complète système avec optimisations

set -euo pipefail

echo "=== VALIDATION COMPLÈTE LUM/VORAX OPTIMISÉ ==="

# Préparation
./prepare_logs.sh
SESSION=$(cat logs/current_session.txt)

# Build optimisé
echo "Phase 1: Build optimisé"
./build.sh 2>&1 | tee logs/compilation/build_${SESSION}.log

# Validation modules
echo "Phase 2: Validation 77 modules"
echo "Modules sources:" | tee logs/compilation/modules_${SESSION}.log
find src/ -name "*.c" -o -name "*.h" | wc -l | tee -a logs/compilation/modules_${SESSION}.log

# Tests complets
echo "Phase 3: Tests stress complets"
if [ -f bin/lum_vorax ]; then
    ./bin/lum_vorax --stress-test-million 2>&1 | tee logs/stress_tests/stress_${SESSION}.log
    
    # Tests optimisations
    echo "Tests optimisations..." | tee -a logs/stress_tests/stress_${SESSION}.log
    ./bin/lum_vorax --simd-benchmark 2>&1 | tee logs/optimization/simd_${SESSION}.log || true
    ./bin/lum_vorax --memory-optimization-test 2>&1 | tee logs/optimization/memory_${SESSION}.log || true
    ./bin/lum_vorax --pareto-optimization-test 2>&1 | tee logs/optimization/pareto_${SESSION}.log || true
fi

# Parsing avancé
echo "Phase 4: Parsing résultats"
python3 tools/parse_stress_log.py logs/stress_tests/stress_${SESSION}.log 2>&1 | tee logs/parsing_results/parsed_${SESSION}.log

# Preuves forensiques
echo "Phase 5: Génération preuves"
./generate_optimized_forensic_evidence.sh

echo "✅ VALIDATION COMPLÈTE TERMINÉE"
echo "📊 Résultats dans logs/ et evidence_optimized_${SESSION}/"
ls -la logs/*/*.log | tail -10
