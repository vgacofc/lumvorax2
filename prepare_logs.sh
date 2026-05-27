#!/bin/bash
# Préparation structure logs standardisée

set -euo pipefail

echo "=== PRÉPARATION LOGS STANDARDISÉS ==="

# Structure logs complète
mkdir -p logs/{compilation,stress_tests,optimization,parsing_results,crypto,benchmarks}

# Session tracking
SESSION=$(date +%Y%m%d_%H%M%S)
echo "$SESSION" > logs/current_session.txt

echo "Session: $SESSION"
echo "📁 Structure logs créée:"
tree logs/ || ls -la logs/

# Templates logs
cat > logs/README.md << 'LOGEOF'
# Structure Logs LUM/VORAX

## Dossiers
- `compilation/` : Logs build et compilation
- `stress_tests/` : Tests stress 1M+ LUMs  
- `optimization/` : Tests optimisations SIMD/Memory/Pareto
- `parsing_results/` : Résultats parsés en JSON
- `crypto/` : Validation cryptographique
- `benchmarks/` : Benchmarks comparatifs

## Format noms fichiers
- `{type}_{session}.log` : YYYYMMDD_HHMMSS
- Exemple: `stress_20250912_143052.log`
LOGEOF

echo "✅ Logs standardisés préparés"
