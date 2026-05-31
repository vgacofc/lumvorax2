#!/bin/bash
# measure_coverage.sh - Mesure couverture code et compare avec baseline
# Usage: ./measure_coverage.sh [--update-baseline]

set -euo pipefail

FIRECRACKER_DIR="/tmp/firecracker_c164"
BASELINE_FILE="$FIRECRACKER_DIR/coverage_baseline.json"
CURRENT_FILE="$FIRECRACKER_DIR/coverage_current.json"
REPORT_DIR="$FIRECRACKER_DIR/coverage_reports"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

# Couleurs
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}╔════════════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║     MESURE COUVERTURE CODE FIRECRACKER C164               ║${NC}"
echo -e "${BLUE}╚════════════════════════════════════════════════════════════╝${NC}"
echo ""

# Vérifier que cargo-llvm-cov est installé
if ! command -v cargo-llvm-cov &> /dev/null; then
    echo -e "${RED}❌ cargo-llvm-cov non installé${NC}"
    echo "Installation: cargo install cargo-llvm-cov"
    exit 1
fi

# Aller dans le répertoire Firecracker
cd "$FIRECRACKER_DIR"

echo -e "${YELLOW}🔧 Nettoyage des artefacts précédents...${NC}"
cargo llvm-cov clean > /dev/null 2>&1 || true

echo -e "${YELLOW}🧪 Exécution des tests avec instrumentation...${NC}"
START_TIME=$(date +%s)

# Exécuter tests avec couverture
if cargo llvm-cov --json --output-path "$CURRENT_FILE" 2>&1 | tee /tmp/coverage_output.log; then
    END_TIME=$(date +%s)
    DURATION=$((END_TIME - START_TIME))
    echo -e "${GREEN}✅ Tests terminés en ${DURATION}s${NC}"
else
    echo -e "${RED}❌ Erreur lors de l'exécution des tests${NC}"
    exit 1
fi

# Extraire métriques
echo ""
echo -e "${BLUE}📊 MÉTRIQUES ACTUELLES${NC}"
echo -e "${BLUE}════════════════════════════════════════════════════════════${NC}"

FUNCTIONS_COVERED=$(jq -r '.data[0].totals.functions.covered' "$CURRENT_FILE")
FUNCTIONS_TOTAL=$(jq -r '.data[0].totals.functions.count' "$CURRENT_FILE")
FUNCTIONS_PCT=$(jq -r '.data[0].totals.functions.percent' "$CURRENT_FILE")

LINES_COVERED=$(jq -r '.data[0].totals.lines.covered' "$CURRENT_FILE")
LINES_TOTAL=$(jq -r '.data[0].totals.lines.count' "$CURRENT_FILE")
LINES_PCT=$(jq -r '.data[0].totals.lines.percent' "$CURRENT_FILE")

REGIONS_COVERED=$(jq -r '.data[0].totals.regions.covered' "$CURRENT_FILE")
REGIONS_TOTAL=$(jq -r '.data[0].totals.regions.count' "$CURRENT_FILE")
REGIONS_PCT=$(jq -r '.data[0].totals.regions.percent' "$CURRENT_FILE")

printf "Fonctions:  %6.2f%% (%5d/%5d)\n" "$FUNCTIONS_PCT" "$FUNCTIONS_COVERED" "$FUNCTIONS_TOTAL"
printf "Lignes:     %6.2f%% (%5d/%5d)\n" "$LINES_PCT" "$LINES_COVERED" "$LINES_TOTAL"
printf "Régions:    %6.2f%% (%5d/%5d)\n" "$REGIONS_PCT" "$REGIONS_COVERED" "$REGIONS_TOTAL"

# Comparer avec baseline si elle existe
if [ -f "$BASELINE_FILE" ]; then
    echo ""
    echo -e "${BLUE}📈 COMPARAISON AVEC BASELINE${NC}"
    echo -e "${BLUE}════════════════════════════════════════════════════════════${NC}"
    
    BASELINE_FUNCTIONS_PCT=$(jq -r '.data[0].totals.functions.percent' "$BASELINE_FILE")
    BASELINE_LINES_PCT=$(jq -r '.data[0].totals.lines.percent' "$BASELINE_FILE")
    BASELINE_REGIONS_PCT=$(jq -r '.data[0].totals.regions.percent' "$BASELINE_FILE")
    
    DIFF_FUNCTIONS=$(echo "$FUNCTIONS_PCT - $BASELINE_FUNCTIONS_PCT" | bc)
    DIFF_LINES=$(echo "$LINES_PCT - $BASELINE_LINES_PCT" | bc)
    DIFF_REGIONS=$(echo "$REGIONS_PCT - $BASELINE_REGIONS_PCT" | bc)
    
    # Afficher différences avec couleurs
    print_diff() {
        local name=$1
        local baseline=$2
        local current=$3
        local diff=$4
        
        printf "%-12s: %6.2f%% → %6.2f%% " "$name" "$baseline" "$current"
        
        if (( $(echo "$diff > 0" | bc -l) )); then
            echo -e "${GREEN}(+%.2f%%)${NC}" "$diff"
        elif (( $(echo "$diff < 0" | bc -l) )); then
            echo -e "${RED}(%.2f%%)${NC}" "$diff"
        else
            echo -e "${YELLOW}(=)${NC}"
        fi
    }
    
    print_diff "Fonctions" "$BASELINE_FUNCTIONS_PCT" "$FUNCTIONS_PCT" "$DIFF_FUNCTIONS"
    print_diff "Lignes" "$BASELINE_LINES_PCT" "$LINES_PCT" "$DIFF_LINES"
    print_diff "Régions" "$BASELINE_REGIONS_PCT" "$REGIONS_PCT" "$DIFF_REGIONS"
    
    # Verdict global
    echo ""
    if (( $(echo "$DIFF_LINES > 0" | bc -l) )); then
        echo -e "${GREEN}✅ AMÉLIORATION: +%.2f%% lignes couvertes${NC}" "$DIFF_LINES"
    elif (( $(echo "$DIFF_LINES < 0" | bc -l) )); then
        echo -e "${RED}❌ RÉGRESSION: %.2f%% lignes couvertes${NC}" "$DIFF_LINES"
        exit 1
    else
        echo -e "${YELLOW}⚠️  AUCUN CHANGEMENT${NC}"
    fi
else
    echo ""
    echo -e "${YELLOW}⚠️  Aucune baseline trouvée${NC}"
    echo "Création baseline: $BASELINE_FILE"
    cp "$CURRENT_FILE" "$BASELINE_FILE"
fi

# Générer rapport HTML
echo ""
echo -e "${YELLOW}📄 Génération rapport HTML...${NC}"
mkdir -p "$REPORT_DIR"
REPORT_HTML="$REPORT_DIR/coverage_${TIMESTAMP}"

if cargo llvm-cov --html --output-dir "$REPORT_HTML" > /dev/null 2>&1; then
    echo -e "${GREEN}✅ Rapport HTML: $REPORT_HTML/index.html${NC}"
else
    echo -e "${RED}❌ Erreur génération rapport HTML${NC}"
fi

# Sauvegarder historique
HISTORY_FILE="$REPORT_DIR/coverage_history.csv"
if [ ! -f "$HISTORY_FILE" ]; then
    echo "timestamp,functions_pct,lines_pct,regions_pct,functions_covered,lines_covered,regions_covered" > "$HISTORY_FILE"
fi
echo "$TIMESTAMP,$FUNCTIONS_PCT,$LINES_PCT,$REGIONS_PCT,$FUNCTIONS_COVERED,$LINES_COVERED,$REGIONS_COVERED" >> "$HISTORY_FILE"

# Top 10 fichiers avec faible couverture
echo ""
echo -e "${BLUE}🔍 TOP 10 FICHIERS FAIBLE COUVERTURE (<50%)${NC}"
echo -e "${BLUE}════════════════════════════════════════════════════════════${NC}"

jq -r '.data[0].files[] | 
    select(.summary.lines.percent < 50) | 
    "\(.summary.lines.percent)|\(.filename)|\(.summary.lines.covered)/\(.summary.lines.count)"' \
    "$CURRENT_FILE" | \
    sort -t'|' -k1 -n | \
    head -10 | \
    while IFS='|' read -r pct file coverage; do
        printf "%6.2f%% %-60s %s\n" "$pct" "$(basename "$file")" "$coverage"
    done

# Modules critiques
echo ""
echo -e "${BLUE}🎯 MODULES CRITIQUES (0% COUVERTURE)${NC}"
echo -e "${BLUE}════════════════════════════════════════════════════════════${NC}"

CRITICAL_MODULES=(
    "virtio/net"
    "snapshot"
    "mmds"
    "vsock"
    "seccompiler"
)

for module in "${CRITICAL_MODULES[@]}"; do
    COUNT=$(jq -r ".data[0].files[] | 
        select(.filename | contains(\"$module\")) | 
        select(.summary.lines.percent == 0) | 
        .filename" "$CURRENT_FILE" | wc -l)
    
    if [ "$COUNT" -gt 0 ]; then
        echo -e "${RED}❌ $module: $COUNT fichiers à 0%${NC}"
    else
        echo -e "${GREEN}✅ $module: Tous fichiers >0%${NC}"
    fi
done

# Recommandations
echo ""
echo -e "${BLUE}💡 RECOMMANDATIONS${NC}"
echo -e "${BLUE}════════════════════════════════════════════════════════════${NC}"

if (( $(echo "$LINES_PCT < 50" | bc -l) )); then
    echo -e "${RED}🔴 CRITIQUE: Couverture <50% - Priorité absolue${NC}"
    echo "   → Créer tests pour modules critiques (virtio-net, snapshot, mmds)"
elif (( $(echo "$LINES_PCT < 70" | bc -l) )); then
    echo -e "${YELLOW}🟡 MOYEN: Couverture <70% - Amélioration nécessaire${NC}"
    echo "   → Compléter tests modules partiels (VMM core, block device)"
elif (( $(echo "$LINES_PCT < 90" | bc -l) )); then
    echo -e "${YELLOW}🟢 BON: Couverture <90% - Finalisation${NC}"
    echo "   → Tester edge cases et chemins d'erreur"
else
    echo -e "${GREEN}✅ EXCELLENT: Couverture >90%${NC}"
    echo "   → Maintenir et documenter"
fi

# Option mise à jour baseline
if [ "${1:-}" = "--update-baseline" ]; then
    echo ""
    echo -e "${YELLOW}🔄 Mise à jour baseline...${NC}"
    cp "$CURRENT_FILE" "$BASELINE_FILE"
    echo -e "${GREEN}✅ Baseline mise à jour${NC}"
fi

echo ""
echo -e "${BLUE}════════════════════════════════════════════════════════════${NC}"
echo -e "${GREEN}✅ Mesure terminée${NC}"
echo ""

# Made with Bob
