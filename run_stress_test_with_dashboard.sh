#!/bin/bash
# Script d'exécution stress test 60s avec dashboard monitoring temps réel
# Cycle C199.25 V3 - Phase 5.2

set -e

echo "╔════════════════════════════════════════════════════════════════╗"
echo "║  STRESS TEST 60S + DASHBOARD MONITORING TEMPS RÉEL            ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""

# Couleurs
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Répertoire base
BASE_DIR="/home/lvx/LVX/lumvorax2"
cd "$BASE_DIR"

# Étape 1: Vérifier si le serveur monitoring est déjà actif
echo -e "${YELLOW}[1/5]${NC} Vérification serveur monitoring..."
if pgrep -f "node monitoring/server.js" > /dev/null; then
    echo -e "${GREEN}✓${NC} Serveur monitoring déjà actif"
else
    echo -e "${YELLOW}→${NC} Démarrage serveur monitoring..."
    cd monitoring
    node server.js > /tmp/monitoring_server.log 2>&1 &
    MONITORING_PID=$!
    cd ..
    sleep 2
    
    if ps -p $MONITORING_PID > /dev/null; then
        echo -e "${GREEN}✓${NC} Serveur monitoring démarré (PID: $MONITORING_PID)"
    else
        echo -e "${RED}✗${NC} Échec démarrage serveur monitoring"
        cat /tmp/monitoring_server.log
        exit 1
    fi
fi

# Étape 2: Ouvrir dashboard dans navigateur
echo -e "${YELLOW}[2/5]${NC} Ouverture dashboard dans navigateur..."
if command -v xdg-open > /dev/null; then
    xdg-open "http://localhost:8080/dashboard_circular.html" 2>/dev/null &
    echo -e "${GREEN}✓${NC} Dashboard ouvert: http://localhost:8080/dashboard_circular.html"
elif command -v firefox > /dev/null; then
    firefox "http://localhost:8080/dashboard_circular.html" 2>/dev/null &
    echo -e "${GREEN}✓${NC} Dashboard ouvert dans Firefox"
elif command -v google-chrome > /dev/null; then
    google-chrome "http://localhost:8080/dashboard_circular.html" 2>/dev/null &
    echo -e "${GREEN}✓${NC} Dashboard ouvert dans Chrome"
else
    echo -e "${YELLOW}⚠${NC}  Navigateur non détecté. Ouvrez manuellement: http://localhost:8080/dashboard_circular.html"
fi

sleep 2

# Étape 3: Compiler benchmark V2 (génération manuelle)
echo -e "${YELLOW}[3/5]${NC} Compilation benchmark stress test 60s V2..."
gcc tests/bench_c199_poh_stress_60s_v2.c \
    src/consensus/lum_poh.c \
    src/consensus/lum_poh_simd.c \
    src/debug/ultra_forensic_logger.c \
    src/debug/forensic_logger.c \
    src/lum/lum_core.c \
    src/debug/memory_tracker.c \
    -I. -Isrc -O3 -march=native -mavx2 -msse4.2 -pthread -lm -lcrypto \
    -o bench_c199_poh_stress_60s_v2 2>&1

if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓${NC} Compilation réussie"
else
    echo -e "${RED}✗${NC} Échec compilation"
    exit 1
fi

# Étape 4: Afficher instructions
echo ""
echo -e "${YELLOW}[4/5]${NC} Instructions:"
echo "  • Dashboard monitoring: http://localhost:8080/dashboard_circular.html"
echo "  • Refresh: 10ms (100 FPS)"
echo "  • Métriques: TPS, Throughput, Latence, CPU, RAM, GPU, Disque"
echo "  • Durée test: 60 secondes"
echo ""
echo -e "${YELLOW}[5/5]${NC} Lancement benchmark stress test 60s..."
echo ""

# Étape 5: Exécuter benchmark avec sudo
echo "emmaus" | sudo -S ./bench_c199_poh_stress_60s_v2 2>&1 | tee /tmp/stress_test_v3_results.txt

# Résultats
echo ""
echo "╔════════════════════════════════════════════════════════════════╗"
echo "║  BENCHMARK TERMINÉ                                             ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""
echo -e "${GREEN}✓${NC} Résultats sauvegardés: /tmp/stress_test_v3_results.txt"
echo -e "${GREEN}✓${NC} Logs forensiques: logs/forensic/"
echo -e "${GREEN}✓${NC} Dashboard monitoring: http://localhost:8080/dashboard_circular.html"
echo ""
echo "Appuyez sur Entrée pour arrêter le serveur monitoring..."
read

# Arrêter serveur monitoring
if [ ! -z "$MONITORING_PID" ]; then
    kill $MONITORING_PID 2>/dev/null || true
    echo -e "${GREEN}✓${NC} Serveur monitoring arrêté"
fi

echo ""
echo "Terminé !"

# Made with Bob
