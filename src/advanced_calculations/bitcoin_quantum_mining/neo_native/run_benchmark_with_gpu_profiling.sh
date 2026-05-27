#!/bin/bash
# Script de benchmark avec profiling GPU temps réel
# Objectif : Capturer métriques GPU réelles pendant exécution

set -e

BENCHMARK="./test_massive_benchmark"
OUTPUT_DIR="profiling_results_c198"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

# Créer répertoire résultats
mkdir -p "$OUTPUT_DIR"

echo "═══════════════════════════════════════════════════════════"
echo "  BENCHMARK + PROFILING GPU TEMPS RÉEL"
echo "  Device: Intel UHD Graphics 620 (Gen9)"
echo "  Timestamp: $TIMESTAMP"
echo "═══════════════════════════════════════════════════════════"
echo ""

# Vérifier que le benchmark existe
if [ ! -f "$BENCHMARK" ]; then
    echo "❌ Erreur: $BENCHMARK n'existe pas"
    echo "   Compilez d'abord avec: make test_massive_benchmark"
    exit 1
fi

# Vérifier permissions intel_gpu_top
if ! echo "emmaus" | sudo -S intel_gpu_top -h &>/dev/null 2>&1; then
    echo "❌ Erreur: impossible d'obtenir permissions sudo"
    exit 1
fi

echo "[1/3] Démarrage intel_gpu_top en arrière-plan..."
# Lancer intel_gpu_top avec JSON output
echo "emmaus" | sudo -S intel_gpu_top -J -o "$OUTPUT_DIR/gpu_metrics_${TIMESTAMP}.json" &
GPU_TOP_PID=$!
echo "   PID intel_gpu_top: $GPU_TOP_PID"
sleep 2  # Laisser intel_gpu_top s'initialiser

echo ""
echo "[2/3] Lancement benchmark..."
echo "───────────────────────────────────────────────────────────"
$BENCHMARK 2>&1 | tee "$OUTPUT_DIR/benchmark_${TIMESTAMP}.log"
BENCHMARK_EXIT=$?
echo "───────────────────────────────────────────────────────────"

echo ""
echo "[3/3] Arrêt intel_gpu_top..."
echo "emmaus" | sudo -S kill -SIGINT $GPU_TOP_PID 2>/dev/null || true
wait $GPU_TOP_PID 2>/dev/null || true
echo "   ✅ Profiling GPU terminé"

echo ""
echo "═══════════════════════════════════════════════════════════"
echo "  RÉSULTATS SAUVEGARDÉS"
echo "═══════════════════════════════════════════════════════════"
echo "  Benchmark log : $OUTPUT_DIR/benchmark_${TIMESTAMP}.log"
echo "  GPU metrics   : $OUTPUT_DIR/gpu_metrics_${TIMESTAMP}.json"
echo ""

# Extraire métriques clés du JSON
if [ -f "$OUTPUT_DIR/gpu_metrics_${TIMESTAMP}.json" ]; then
    echo "[ANALYSE] Extraction métriques GPU..."
    
    # Parser JSON avec jq si disponible
    if command -v jq &>/dev/null; then
        echo ""
        echo "Métriques GPU moyennes:"
        jq -r '.engines[] | select(.name == "Render/3D") | 
               "  Render/3D busy: \(.busy)%"' \
               "$OUTPUT_DIR/gpu_metrics_${TIMESTAMP}.json" | head -5
        
        jq -r '.frequency | 
               "  Fréquence GPU: \(.actual) MHz (req: \(.requested) MHz)"' \
               "$OUTPUT_DIR/gpu_metrics_${TIMESTAMP}.json" | head -5
    else
        echo "  ⚠️  jq non installé, analyse JSON manuelle requise"
    fi
fi

echo ""
if [ $BENCHMARK_EXIT -eq 0 ]; then
    echo "✅ Benchmark + profiling réussi"
else
    echo "❌ Benchmark échoué (exit code: $BENCHMARK_EXIT)"
fi

exit $BENCHMARK_EXIT

# Made with Bob
