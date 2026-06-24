#!/bin/bash
# Script exécution benchmark V3 temps réel + dashboard
# Cycle C199.25 V3 - Phase 5.2

set -e

echo "╔════════════════════════════════════════════════════════════════╗"
echo "║  BENCHMARK V3 TEMPS RÉEL + DASHBOARD MONITORING                ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""

# Couleurs
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m'

BASE_DIR="/home/lvx/LVX/lumvorax2"
cd "$BASE_DIR"

# Fonction cleanup
cleanup() {
    echo -e "\n${YELLOW}🛑 Arrêt des serveurs...${NC}"
    if [ ! -z "$DASHBOARD_PID" ]; then
        kill $DASHBOARD_PID 2>/dev/null || true
    fi
    pkill -f "node backend/server.js" 2>/dev/null || true
    pkill -f "vite" 2>/dev/null || true
    exit 0
}

trap cleanup SIGINT SIGTERM

# Nettoyer ancien fichier métriques
rm -f logs/forensic/realtime_metrics.csv

# Étape 1: Lancer dashboard en arrière-plan
echo -e "${BLUE}[1/4]${NC} Lancement dashboard monitoring..."
cd monitoring
./start_dashboard.sh > /tmp/dashboard.log 2>&1 &
DASHBOARD_PID=$!
cd ..

echo -e "${GREEN}✓${NC} Dashboard démarré (PID: $DASHBOARD_PID)"
echo -e "${YELLOW}→${NC} Attente initialisation (5s)..."
sleep 5

# Étape 2: Ouvrir navigateur
echo -e "${BLUE}[2/4]${NC} Ouverture dashboard dans navigateur..."
if command -v xdg-open > /dev/null; then
    xdg-open "http://localhost:5173" 2>/dev/null &
    echo -e "${GREEN}✓${NC} Dashboard: http://localhost:5173"
elif command -v firefox > /dev/null; then
    firefox "http://localhost:5173" 2>/dev/null &
    echo -e "${GREEN}✓${NC} Firefox ouvert"
else
    echo -e "${YELLOW}⚠${NC}  Ouvrez manuellement: http://localhost:5173"
fi

sleep 2

# Étape 3: Compiler benchmark V3 temps réel
echo -e "${BLUE}[3/4]${NC} Compilation benchmark V3 temps réel..."
gcc tests/bench_c199_poh_stress_60s_v3_realtime.c \
    src/consensus/lum_poh.c \
    src/consensus/lum_poh_simd.c \
    src/debug/ultra_forensic_logger.c \
    src/debug/forensic_logger.c \
    src/lum/lum_core.c \
    src/debug/memory_tracker.c \
    -I. -Isrc -O3 -march=native -mavx2 -msse4.2 -pthread -lm -lcrypto \
    -o bench_c199_poh_v3_realtime 2>&1

if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓${NC} Compilation réussie"
else
    echo -e "${RED}✗${NC} Échec compilation"
    cleanup
    exit 1
fi

# Étape 4: Lancer benchmark
echo ""
echo -e "${BLUE}[4/4]${NC} Lancement benchmark stress test 60 secondes..."
echo ""
echo -e "${YELLOW}📊 Dashboard monitoring:${NC} http://localhost:5173"
echo -e "${YELLOW}⏱️  Durée test:${NC} 60 secondes"
echo -e "${YELLOW}🎯 Objectif:${NC} 400K+ TPS stable (±5%)"
echo -e "${YELLOW}📈 Métriques temps réel:${NC} logs/forensic/realtime_metrics.csv"
echo ""
echo "═══════════════════════════════════════════════════════════════"
echo ""

# Exécuter benchmark avec sudo pour accès /proc/stat
echo "emmaus" | sudo -S ./bench_c199_poh_v3_realtime 2>&1 | tee /tmp/benchmark_v3_results.txt

# Résultats
echo ""
echo "╔════════════════════════════════════════════════════════════════╗"
echo "║  BENCHMARK TERMINÉ                                             ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""
echo -e "${GREEN}✓${NC} Résultats: /tmp/benchmark_v3_results.txt"
echo -e "${GREEN}✓${NC} Métriques temps réel: logs/forensic/realtime_metrics.csv"
echo -e "${GREEN}✓${NC} Logs forensiques: logs/forensic/"
echo -e "${GREEN}✓${NC} Dashboard: http://localhost:5173"
echo ""
echo -e "${YELLOW}Le dashboard reste actif pour visualiser les résultats.${NC}"
echo -e "${YELLOW}Appuyez sur Ctrl+C pour arrêter le dashboard...${NC}"

# Garder dashboard actif
wait

cleanup
echo -e "${GREEN}✓${NC} Terminé !"

# Made with Bob
