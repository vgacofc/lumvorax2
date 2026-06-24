#!/bin/bash
# master_lvx/run_priority_tests.sh
# Script d'exécution immédiate - 5 tests prioritaires
# Objectif: Identifier quelle combinaison produit des nonces valides

set -euo pipefail

# Couleurs pour output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
RESULTS_DIR="master_lvx/results_priority_$(date +%Y%m%d_%H%M%S)"
TEST_DURATION=60  # 60 secondes par test
WORKSPACE="/home/lvx/LVX/lumvorax2"

mkdir -p "$RESULTS_DIR"

echo -e "${BLUE}╔════════════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║  TESTS PRIORITAIRES - VALIDATION NONCES GPU               ║${NC}"
echo -e "${BLUE}║  Objectif: Trouver LA combinaison qui produit des nonces  ║${NC}"
echo -e "${BLUE}╚════════════════════════════════════════════════════════════╝${NC}"
echo ""

# Fonction de test générique
run_test() {
    local test_id="$1"
    local test_name="$2"
    local test_cmd="$3"
    local expected_nonces="$4"
    
    echo -e "${YELLOW}=========================================${NC}"
    echo -e "${YELLOW}TEST $test_id: $test_name${NC}"
    echo -e "${YELLOW}Commande: $test_cmd${NC}"
    echo -e "${YELLOW}Nonces attendus: $expected_nonces${NC}"
    echo -e "${YELLOW}=========================================${NC}"
    
    local log_file="$RESULTS_DIR/${test_id}_${test_name}.log"
    local metrics_file="$RESULTS_DIR/${test_id}_metrics.json"
    local start_time=$(date +%s%N)
    
    # Exécuter test avec timeout
    if timeout ${TEST_DURATION}s bash -c "$test_cmd" > "$log_file" 2>&1; then
        local end_time=$(date +%s%N)
        local duration_ns=$((end_time - start_time))
        local duration_s=$((duration_ns / 1000000000))
        
        # Extraire métriques
        local hashrate=$(grep -oP 'hashrate[=:]\s*\K[0-9.]+' "$log_file" | head -1 || echo "0")
        local nonces=$(grep -oP 'nonces[_\s]found[=:]\s*\K[0-9]+' "$log_file" | head -1 || echo "0")
        local errors=$(grep -c "ERROR\|FAILED\|errno" "$log_file" || echo "0")
        local gpu_exec=$(grep -c "gpu_executed=YES\|GPU.*SUCCESS" "$log_file" || echo "0")
        
        # Déterminer status
        local status="FAILED"
        if [ "$nonces" -gt 0 ]; then
            status="SUCCESS"
        elif [ "$gpu_exec" -gt 0 ] && [ "$errors" -eq 0 ]; then
            status="PARTIAL"
        fi
        
        # Générer JSON
        cat > "$metrics_file" <<EOF
{
    "test_id": "$test_id",
    "test_name": "$test_name",
    "status": "$status",
    "duration_s": $duration_s,
    "hashrate_mhs": $hashrate,
    "nonces_found": $nonces,
    "nonces_expected": $expected_nonces,
    "gpu_executed": $([ "$gpu_exec" -gt 0 ] && echo "true" || echo "false"),
    "errors_count": $errors,
    "log_file": "$log_file"
}
EOF
        
        # Afficher résultat
        if [ "$status" == "SUCCESS" ]; then
            echo -e "${GREEN}✅ SUCCESS: $nonces nonces trouvés (hashrate: $hashrate MH/s)${NC}"
        elif [ "$status" == "PARTIAL" ]; then
            echo -e "${YELLOW}⚠️  PARTIAL: GPU exécuté mais 0 nonce (hashrate: $hashrate MH/s)${NC}"
        else
            echo -e "${RED}❌ FAILED: $errors erreurs détectées${NC}"
        fi
        
        echo -e "Durée: ${duration_s}s"
        echo -e "Log: $log_file"
        echo ""
        
    else
        echo -e "${RED}❌ TIMEOUT ou CRASH après ${TEST_DURATION}s${NC}"
        echo "{\"test_id\":\"$test_id\",\"status\":\"TIMEOUT\"}" > "$metrics_file"
        echo ""
    fi
}

# Vérifier que nous sommes dans le bon répertoire
cd "$WORKSPACE/src/advanced_calculations/bitcoin_quantum_mining"

echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}PHASE 1: TEST BASELINE (Référence OpenCL - 45 nonces)${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
echo ""

# TEST T003: OpenCL Pure (RÉFÉRENCE - doit trouver ~45 nonces)
if [ -f "test_c194_opencl_drm_benchmark" ]; then
    run_test "T003" "OpenCL_Pure_Reference" \
        "./test_c194_opencl_drm_benchmark --duration=$TEST_DURATION" \
        "45"
else
    echo -e "${RED}⚠️  test_c194_opencl_drm_benchmark non trouvé, compilation nécessaire${NC}"
    echo ""
fi

echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}PHASE 2: TEST CRITIQUE (Fix ROOT CAUSE #87)${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
echo ""

# TEST T501: EXEC_OBJECT_PINNED (FIX CRITIQUE)
cd level_zero_native
if [ -f "tests/test_c610_pinned_fix" ]; then
    run_test "T501" "PINNED_Fix_ROOT_CAUSE_87" \
        "./tests/test_c610_pinned_fix --duration=$TEST_DURATION" \
        ">0"
else
    echo -e "${YELLOW}⚠️  test_c610_pinned_fix non trouvé, création nécessaire${NC}"
    echo -e "${YELLOW}   Utilisation test_c282 avec flag PINNED...${NC}"
    
    # Créer version modifiée avec PINNED
    if [ -f "test_c282_progressive_batch.sh" ]; then
        # Modifier temporairement pour forcer PINNED
        sed 's/flags=0x0/flags=0x10/g' btc_gen9_native_runner.c > btc_gen9_native_runner_pinned.c
        gcc -o btc_gen9_native_runner_pinned btc_gen9_native_runner_pinned.c -I. -ldrm -lm
        
        run_test "T501" "PINNED_Fix_Modified" \
            "./btc_gen9_native_runner_pinned --duration=$TEST_DURATION" \
            ">0"
    fi
fi

echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}PHASE 3: TEST ÉTAT ACTUEL (i915 DRM Natif)${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
echo ""

# TEST T001: i915 DRM Natif (ÉTAT ACTUEL - 507 MH/s, 0 nonce)
if [ -f "test_c282_progressive_batch.sh" ]; then
    run_test "T001" "i915_DRM_Natif_Current" \
        "./test_c282_progressive_batch.sh 1 50000" \
        "0"
else
    echo -e "${RED}⚠️  test_c282_progressive_batch.sh non trouvé${NC}"
fi

echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}PHASE 4: TEST LEVEL ZERO (Alternative)${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
echo ""

# TEST T002: Level Zero Pure
if [ -f "btc_mining_levelzero_runner" ]; then
    run_test "T002" "Level_Zero_Pure" \
        "./btc_mining_levelzero_runner --duration=$TEST_DURATION" \
        ">0"
else
    echo -e "${YELLOW}⚠️  btc_mining_levelzero_runner non trouvé, compilation nécessaire${NC}"
    
    # Vérifier si le source existe
    if [ -f "btc_mining_levelzero_runner.c" ]; then
        echo -e "${YELLOW}   Compilation en cours...${NC}"
        gcc -o btc_mining_levelzero_runner btc_mining_levelzero_runner.c -I. -lze_loader -ldrm -lm
        
        run_test "T002" "Level_Zero_Pure_Compiled" \
            "./btc_mining_levelzero_runner --duration=$TEST_DURATION" \
            ">0"
    else
        echo -e "${RED}   Source btc_mining_levelzero_runner.c non trouvé${NC}"
    fi
fi

echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}PHASE 5: TEST HYBRIDE (Natif + Level Zero)${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
echo ""

# TEST T101: Natif + Level Zero Hybride
if [ -f "tests/test_hybrid_native_lz" ]; then
    run_test "T101" "Hybrid_Natif_LevelZero" \
        "./tests/test_hybrid_native_lz --duration=$TEST_DURATION" \
        ">0"
else
    echo -e "${YELLOW}⚠️  test_hybrid_native_lz non trouvé${NC}"
    echo -e "${YELLOW}   Cette combinaison nécessite implémentation${NC}"
fi

echo ""
echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}GÉNÉRATION RAPPORT COMPARATIF${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
echo ""

# Générer rapport comparatif
REPORT_FILE="$RESULTS_DIR/RAPPORT_COMPARATIF.md"

cat > "$REPORT_FILE" <<'EOFR'
# 📊 RAPPORT COMPARATIF - TESTS PRIORITAIRES GPU

## Résumé Exécution

**Date**: $(date +"%Y-%m-%d %H:%M:%S")  
**Durée par test**: 60 secondes  
**Tests exécutés**: 5

---

## Résultats

| Test ID | Nom | Status | Nonces | Hashrate | Erreurs |
|---------|-----|--------|--------|----------|---------|
EOFR

# Ajouter résultats de chaque test
for metrics in "$RESULTS_DIR"/*_metrics.json; do
    if [ -f "$metrics" ]; then
        test_id=$(jq -r '.test_id' "$metrics")
        test_name=$(jq -r '.test_name' "$metrics")
        status=$(jq -r '.status' "$metrics")
        nonces=$(jq -r '.nonces_found' "$metrics")
        hashrate=$(jq -r '.hashrate_mhs' "$metrics")
        errors=$(jq -r '.errors_count' "$metrics")
        
        # Emoji selon status
        case "$status" in
            "SUCCESS") emoji="✅" ;;
            "PARTIAL") emoji="⚠️" ;;
            "FAILED") emoji="❌" ;;
            "TIMEOUT") emoji="⏱️" ;;
            *) emoji="❓" ;;
        esac
        
        echo "| $test_id | $test_name | $emoji $status | $nonces | $hashrate MH/s | $errors |" >> "$REPORT_FILE"
    fi
done

cat >> "$REPORT_FILE" <<'EOFR'

---

## Analyse

### Meilleure Configuration

EOFR

# Trouver meilleur test (max nonces)
best_test=$(jq -s 'max_by(.nonces_found)' "$RESULTS_DIR"/*_metrics.json)
best_id=$(echo "$best_test" | jq -r '.test_id')
best_name=$(echo "$best_test" | jq -r '.test_name')
best_nonces=$(echo "$best_test" | jq -r '.nonces_found')
best_hashrate=$(echo "$best_test" | jq -r '.hashrate_mhs')

cat >> "$REPORT_FILE" <<EOFR
**Test**: $best_id - $best_name  
**Nonces trouvés**: $best_nonces  
**Hashrate**: $best_hashrate MH/s  

### Pire Configuration

EOFR

# Trouver pire test (min nonces)
worst_test=$(jq -s 'min_by(.nonces_found)' "$RESULTS_DIR"/*_metrics.json)
worst_id=$(echo "$worst_test" | jq -r '.test_id')
worst_name=$(echo "$worst_test" | jq -r '.test_name')
worst_nonces=$(echo "$worst_test" | jq -r '.nonces_found')
worst_hashrate=$(echo "$worst_test" | jq -r '.hashrate_mhs')

cat >> "$REPORT_FILE" <<EOFR
**Test**: $worst_id - $worst_name  
**Nonces trouvés**: $worst_nonces  
**Hashrate**: $worst_hashrate MH/s  

---

## Recommandations

EOFR

# Générer recommandations basées sur résultats
if [ "$best_nonces" -gt 0 ]; then
    cat >> "$REPORT_FILE" <<EOFR
1. ✅ **Utiliser configuration $best_name pour production**
2. 🔧 Optimiser cette configuration pour maximiser hashrate
3. 📊 Valider stabilité sur 24h
4. 🚀 Déployer en production après validation

EOFR
else
    cat >> "$REPORT_FILE" <<EOFR
1. ❌ **AUCUNE configuration n'a produit de nonces valides**
2. 🔍 Investiguer ROOT CAUSE #87 (relocation NULL)
3. 🛠️ Implémenter fix EXEC_OBJECT_PINNED
4. 🧪 Tester combinaisons hybrides supplémentaires
5. 📞 Contacter support Intel pour assistance i915 DRM

EOFR
fi

cat >> "$REPORT_FILE" <<EOFR

---

## Logs Détaillés

EOFR

# Lister tous les logs
for log in "$RESULTS_DIR"/*.log; do
    if [ -f "$log" ]; then
        echo "- \`$(basename "$log")\`" >> "$REPORT_FILE"
    fi
done

cat >> "$REPORT_FILE" <<EOFR

---

**Rapport généré**: $(date +"%Y-%m-%d %H:%M:%S")  
**Répertoire résultats**: \`$RESULTS_DIR\`
EOFR

echo -e "${GREEN}✅ Rapport comparatif généré: $REPORT_FILE${NC}"
echo ""

# Afficher résumé
echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}RÉSUMÉ FINAL${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
echo ""

cat "$REPORT_FILE"

echo ""
echo -e "${GREEN}✅ Tests prioritaires terminés${NC}"
echo -e "${GREEN}📁 Résultats dans: $RESULTS_DIR${NC}"
echo -e "${GREEN}📊 Rapport: $REPORT_FILE${NC}"
echo ""

# Retourner code selon succès
if [ "$best_nonces" -gt 0 ]; then
    echo -e "${GREEN}🎉 AU MOINS UNE CONFIGURATION A PRODUIT DES NONCES VALIDES!${NC}"
    exit 0
else
    echo -e "${RED}⚠️  AUCUNE CONFIGURATION N'A PRODUIT DE NONCES VALIDES${NC}"
    echo -e "${YELLOW}   Voir recommandations dans le rapport${NC}"
    exit 1
fi

# Made with Bob
