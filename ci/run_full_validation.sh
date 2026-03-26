#!/bin/bash
# Validation forensique complète avec optimisations avancées

set -euo pipefail

SESSION=$(date +%Y%m%d_%H%M%S)
echo "=== VALIDATION FORENSIQUE COMPLÈTE OPTIMISÉE ==="
echo "Session: $SESSION"

# Préparation logs standardisés
mkdir -p logs/{compilation,stress_tests,optimization,parsing_results}

# Phase 1: Build optimisé
echo "Phase 1: Compilation optimisée"
./build.sh 2>&1 | tee logs/compilation/build_${SESSION}.log

# Phase 2: Tests unitaires
echo "Phase 2: Tests unitaires complets"
make test 2>&1 | tee logs/compilation/tests_${SESSION}.log

# Phase 3: Tests stress optimisés
echo "Phase 3: Tests stress avec optimisations"
./bin/lum_vorax --stress-test-million 2>&1 | tee logs/stress_tests/stress_${SESSION}.log

# Tests optimisations spécifiques
./bin/lum_vorax --simd-benchmark 2>&1 | tee logs/optimization/simd_${SESSION}.log
./bin/lum_vorax --memory-optimization-test 2>&1 | tee logs/optimization/memory_${SESSION}.log
./bin/lum_vorax --pareto-optimization-test 2>&1 | tee logs/optimization/pareto_${SESSION}.log

# Phase 4: Parsing et métriques
echo "Phase 4: Analyse résultats"
python3 tools/parse_stress_log.py logs/stress_tests/stress_${SESSION}.log > logs/parsing_results/parsed_${SESSION}.json

# Phase 5: Génération preuves
echo "Phase 5: Génération preuves forensiques"
./generate_optimized_forensic_evidence.sh

echo "✅ Validation complète terminée"
echo "📁 Logs: logs/"
echo "📁 Evidence: evidence_optimized_${SESSION}/"
