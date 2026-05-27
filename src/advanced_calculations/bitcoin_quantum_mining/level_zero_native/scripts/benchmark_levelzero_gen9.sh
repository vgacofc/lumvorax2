#!/bin/bash
# LumVorax C198 - Benchmark Level Zero Gen9 vs OpenCL
# Mesure hashrate Bitcoin mining sur Intel UHD 620

set -e

echo "════════════════════════════════════════════════════════════════"
echo "  BENCHMARK LEVEL ZERO GEN9 vs OPENCL"
echo "  Intel UHD Graphics 620 - Bitcoin SHA-256 Mining"
echo "════════════════════════════════════════════════════════════════"
echo ""

# Couleurs
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
BATCH_SIZES=(131072 262144 524288)  # 128K, 256K, 512K nonces
WORK_GROUP_SIZES=(64 128 256)
ITERATIONS=5

# Résultats
RESULTS_DIR="../logs/benchmark"
mkdir -p "$RESULTS_DIR"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
RESULTS_FILE="$RESULTS_DIR/benchmark_$TIMESTAMP.csv"

# Header CSV
echo "Backend,BatchSize,WorkGroupSize,Iteration,Hashrate_MHs,Latency_ms,Overhead_pct" > "$RESULTS_FILE"

# ═══════════════════════════════════════════════════════════════════
# FONCTION : BENCHMARK LEVEL ZERO
# ═══════════════════════════════════════════════════════════════════

benchmark_levelzero() {
    local batch_size=$1
    local work_group_size=$2
    local iteration=$3
    
    echo -e "${BLUE}[BENCH]${NC} Level Zero - Batch=$batch_size WG=$work_group_size Iter=$iteration"
    
    # Configuration environnement
    source "$HOME/levelzero_gen9/setup_env.sh" 2>/dev/null
    
    # Exécution benchmark
    local output=$(sg render -c "sg video -c '../bin/btc_levelzero_benchmark --batch-size $batch_size --work-group-size $work_group_size'" 2>&1)
    
    # Extraction métriques
    local hashrate=$(echo "$output" | grep -oP 'Hashrate:\s+\K[0-9.]+' || echo "0.0")
    local latency=$(echo "$output" | grep -oP 'Latency:\s+\K[0-9.]+' || echo "0.0")
    local overhead=$(echo "$output" | grep -oP 'Overhead:\s+\K[0-9.]+' || echo "0.0")
    
    # Sauvegarde résultats
    echo "LevelZero,$batch_size,$work_group_size,$iteration,$hashrate,$latency,$overhead" >> "$RESULTS_FILE"
    
    echo "  Hashrate: ${hashrate} MH/s | Latency: ${latency} ms | Overhead: ${overhead}%"
}

# ═══════════════════════════════════════════════════════════════════
# FONCTION : BENCHMARK OPENCL
# ═══════════════════════════════════════════════════════════════════

benchmark_opencl() {
    local batch_size=$1
    local iteration=$2
    
    echo -e "${BLUE}[BENCH]${NC} OpenCL - Batch=$batch_size Iter=$iteration"
    
    # Exécution benchmark OpenCL
    cd ../../  # Retour au répertoire bitcoin_quantum_mining
    local output=$(./bin/btc_opencl_benchmark --batch-size $batch_size 2>&1 || echo "ERROR")
    
    if [ "$output" = "ERROR" ]; then
        echo -e "${RED}✗${NC} Benchmark OpenCL échoué"
        echo "OpenCL,$batch_size,N/A,$iteration,0.0,0.0,0.0" >> "$RESULTS_FILE"
        return
    fi
    
    # Extraction métriques
    local hashrate=$(echo "$output" | grep -oP 'Hashrate:\s+\K[0-9.]+' || echo "0.0")
    local latency=$(echo "$output" | grep -oP 'Latency:\s+\K[0-9.]+' || echo "0.0")
    local overhead=$(echo "$output" | grep -oP 'Overhead:\s+\K[0-9.]+' || echo "0.0")
    
    # Sauvegarde résultats
    echo "OpenCL,$batch_size,N/A,$iteration,$hashrate,$latency,$overhead" >> "$RESULTS_FILE"
    
    echo "  Hashrate: ${hashrate} MH/s | Latency: ${latency} ms | Overhead: ${overhead}%"
    
    cd level_zero_native/scripts  # Retour au répertoire scripts
}

# ═══════════════════════════════════════════════════════════════════
# BENCHMARK 1 : BASELINE OPENCL
# ═══════════════════════════════════════════════════════════════════

echo "━━━ BENCHMARK 1/3 : Baseline OpenCL ━━━"
echo ""

if [ -f "../../bin/btc_opencl_benchmark" ]; then
    for batch_size in "${BATCH_SIZES[@]}"; do
        for iter in $(seq 1 $ITERATIONS); do
            benchmark_opencl $batch_size $iter
            sleep 1
        done
        echo ""
    done
else
    echo -e "${YELLOW}⚠${NC} Benchmark OpenCL non disponible"
    echo "Compilez d'abord : cd ../.. && make"
fi

echo ""

# ═══════════════════════════════════════════════════════════════════
# BENCHMARK 2 : LEVEL ZERO - BATCH SIZE
# ═══════════════════════════════════════════════════════════════════

echo "━━━ BENCHMARK 2/3 : Level Zero - Batch Size Optimization ━━━"
echo ""

if [ -f "../bin/btc_levelzero_benchmark" ]; then
    OPTIMAL_WG=256  # Work group optimal pour Gen9
    
    for batch_size in "${BATCH_SIZES[@]}"; do
        for iter in $(seq 1 $ITERATIONS); do
            benchmark_levelzero $batch_size $OPTIMAL_WG $iter
            sleep 1
        done
        echo ""
    done
else
    echo -e "${YELLOW}⚠${NC} Benchmark Level Zero non disponible"
    echo "Compilez d'abord : cd .. && make"
fi

echo ""

# ═══════════════════════════════════════════════════════════════════
# BENCHMARK 3 : LEVEL ZERO - WORK GROUP SIZE
# ═══════════════════════════════════════════════════════════════════

echo "━━━ BENCHMARK 3/3 : Level Zero - Work Group Size Optimization ━━━"
echo ""

if [ -f "../bin/btc_levelzero_benchmark" ]; then
    OPTIMAL_BATCH=262144  # Batch optimal (256K)
    
    for wg_size in "${WORK_GROUP_SIZES[@]}"; do
        for iter in $(seq 1 $ITERATIONS); do
            benchmark_levelzero $OPTIMAL_BATCH $wg_size $iter
            sleep 1
        done
        echo ""
    done
fi

echo ""

# ═══════════════════════════════════════════════════════════════════
# ANALYSE RÉSULTATS
# ═══════════════════════════════════════════════════════════════════

echo "════════════════════════════════════════════════════════════════"
echo "  ANALYSE RÉSULTATS"
echo "════════════════════════════════════════════════════════════════"
echo ""

if [ ! -f "$RESULTS_FILE" ]; then
    echo -e "${RED}✗${NC} Fichier résultats non trouvé"
    exit 1
fi

# Calcul moyennes OpenCL
echo "━━━ OpenCL (Baseline) ━━━"
echo ""

for batch_size in "${BATCH_SIZES[@]}"; do
    AVG_HASHRATE=$(awk -F',' -v bs=$batch_size '$1=="OpenCL" && $2==bs {sum+=$5; count++} END {if(count>0) print sum/count; else print 0}' "$RESULTS_FILE")
    AVG_LATENCY=$(awk -F',' -v bs=$batch_size '$1=="OpenCL" && $2==bs {sum+=$6; count++} END {if(count>0) print sum/count; else print 0}' "$RESULTS_FILE")
    
    printf "Batch %6d : %.2f MH/s | %.2f ms\n" $batch_size $AVG_HASHRATE $AVG_LATENCY
done

echo ""

# Calcul moyennes Level Zero
echo "━━━ Level Zero Gen9 ━━━"
echo ""

for batch_size in "${BATCH_SIZES[@]}"; do
    AVG_HASHRATE=$(awk -F',' -v bs=$batch_size '$1=="LevelZero" && $2==bs {sum+=$5; count++} END {if(count>0) print sum/count; else print 0}' "$RESULTS_FILE")
    AVG_LATENCY=$(awk -F',' -v bs=$batch_size '$1=="LevelZero" && $2==bs {sum+=$6; count++} END {if(count>0) print sum/count; else print 0}' "$RESULTS_FILE")
    
    printf "Batch %6d : %.2f MH/s | %.2f ms\n" $batch_size $AVG_HASHRATE $AVG_LATENCY
done

echo ""

# Comparaison meilleure configuration
echo "━━━ Comparaison Meilleure Configuration ━━━"
echo ""

BEST_OPENCL=$(awk -F',' '$1=="OpenCL" {if($5>max) max=$5} END {print max}' "$RESULTS_FILE")
BEST_LEVELZERO=$(awk -F',' '$1=="LevelZero" {if($5>max) max=$5} END {print max}' "$RESULTS_FILE")

echo "OpenCL Best    : $BEST_OPENCL MH/s"
echo "Level Zero Best: $BEST_LEVELZERO MH/s"
echo ""

if (( $(echo "$BEST_LEVELZERO > $BEST_OPENCL" | bc -l) )); then
    GAIN=$(echo "scale=2; ($BEST_LEVELZERO - $BEST_OPENCL) / $BEST_OPENCL * 100" | bc)
    echo -e "${GREEN}✓ Level Zero PLUS RAPIDE : +${GAIN}%${NC}"
elif (( $(echo "$BEST_LEVELZERO < $BEST_OPENCL" | bc -l) )); then
    LOSS=$(echo "scale=2; ($BEST_OPENCL - $BEST_LEVELZERO) / $BEST_OPENCL * 100" | bc)
    echo -e "${YELLOW}⚠ Level Zero PLUS LENT : -${LOSS}%${NC}"
else
    echo -e "${BLUE}= Level Zero ÉQUIVALENT${NC}"
fi

echo ""

# ═══════════════════════════════════════════════════════════════════
# GÉNÉRATION RAPPORT
# ═══════════════════════════════════════════════════════════════════

REPORT_FILE="$RESULTS_DIR/RAPPORT_BENCHMARK_$TIMESTAMP.md"

cat > "$REPORT_FILE" << EOF
# RAPPORT BENCHMARK C198 - Level Zero Gen9 vs OpenCL

**Date** : $(date '+%Y-%m-%d %H:%M:%S')  
**GPU** : Intel UHD Graphics 620 (Gen9)  
**Drivers** : Level Zero v21.49 + OpenCL NEO  

## Résultats Bruts

\`\`\`csv
$(cat "$RESULTS_FILE")
\`\`\`

## Moyennes par Backend

### OpenCL (Baseline)

| Batch Size | Hashrate (MH/s) | Latency (ms) |
|------------|-----------------|--------------|
EOF

for batch_size in "${BATCH_SIZES[@]}"; do
    AVG_HASHRATE=$(awk -F',' -v bs=$batch_size '$1=="OpenCL" && $2==bs {sum+=$5; count++} END {if(count>0) printf "%.2f", sum/count; else print "0.00"}' "$RESULTS_FILE")
    AVG_LATENCY=$(awk -F',' -v bs=$batch_size '$1=="OpenCL" && $2==bs {sum+=$6; count++} END {if(count>0) printf "%.2f", sum/count; else print "0.00"}' "$RESULTS_FILE")
    
    echo "| $batch_size | $AVG_HASHRATE | $AVG_LATENCY |" >> "$REPORT_FILE"
done

cat >> "$REPORT_FILE" << EOF

### Level Zero Gen9

| Batch Size | Hashrate (MH/s) | Latency (ms) |
|------------|-----------------|--------------|
EOF

for batch_size in "${BATCH_SIZES[@]}"; do
    AVG_HASHRATE=$(awk -F',' -v bs=$batch_size '$1=="LevelZero" && $2==bs {sum+=$5; count++} END {if(count>0) printf "%.2f", sum/count; else print "0.00"}' "$RESULTS_FILE")
    AVG_LATENCY=$(awk -F',' -v bs=$batch_size '$1=="LevelZero" && $2==bs {sum+=$6; count++} END {if(count>0) printf "%.2f", sum/count; else print "0.00"}' "$RESULTS_FILE")
    
    echo "| $batch_size | $AVG_HASHRATE | $AVG_LATENCY |" >> "$REPORT_FILE"
done

cat >> "$REPORT_FILE" << EOF

## Comparaison Finale

- **OpenCL Best** : $BEST_OPENCL MH/s
- **Level Zero Best** : $BEST_LEVELZERO MH/s
- **Différence** : $(echo "scale=2; $BEST_LEVELZERO - $BEST_OPENCL" | bc) MH/s

## Conclusion

EOF

if (( $(echo "$BEST_LEVELZERO > $BEST_OPENCL" | bc -l) )); then
    GAIN=$(echo "scale=2; ($BEST_LEVELZERO - $BEST_OPENCL) / $BEST_OPENCL * 100" | bc)
    echo "✅ **Level Zero est PLUS RAPIDE de +${GAIN}%**" >> "$REPORT_FILE"
    echo "" >> "$REPORT_FILE"
    echo "Level Zero Gen9 offre de meilleures performances que OpenCL sur Intel UHD 620." >> "$REPORT_FILE"
else
    echo "⚠️ **OpenCL reste plus performant**" >> "$REPORT_FILE"
    echo "" >> "$REPORT_FILE"
    echo "Level Zero Gen9 n'apporte pas de gain significatif sur cette architecture." >> "$REPORT_FILE"
fi

echo ""
echo "════════════════════════════════════════════════════════════════"
echo -e "${GREEN}  BENCHMARK TERMINÉ${NC}"
echo "════════════════════════════════════════════════════════════════"
echo ""
echo "Résultats sauvegardés :"
echo "  CSV    : $RESULTS_FILE"
echo "  Rapport: $REPORT_FILE"
echo ""

# Made with Bob
