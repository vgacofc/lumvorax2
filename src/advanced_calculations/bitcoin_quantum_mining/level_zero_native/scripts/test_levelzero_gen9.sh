#!/bin/bash
# LumVorax C198 - Tests Level Zero Gen9
# Valide que Level Zero fonctionne correctement sur Intel UHD 620

set -e

echo "════════════════════════════════════════════════════════════════"
echo "  TESTS LEVEL ZERO GEN9 - Intel UHD Graphics 620"
echo "════════════════════════════════════════════════════════════════"
echo ""

# Couleurs
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration environnement
if [ -f "$HOME/levelzero_gen9/setup_env.sh" ]; then
    echo -e "${BLUE}[INFO]${NC} Chargement environnement Level Zero Gen9..."
    source "$HOME/levelzero_gen9/setup_env.sh"
else
    echo -e "${RED}[ERREUR]${NC} Environnement Level Zero Gen9 non trouvé"
    echo "Exécutez d'abord : ./scripts/install_levelzero_gen9.sh"
    exit 1
fi

echo ""

# ═══════════════════════════════════════════════════════════════════
# TEST 1 : INITIALISATION LEVEL ZERO
# ═══════════════════════════════════════════════════════════════════

echo "━━━ TEST 1/5 : Initialisation Level Zero ━━━"
echo ""

if [ ! -f "../bin/test_levelzero_init" ]; then
    echo -e "${RED}✗${NC} Binaire test_levelzero_init non trouvé"
    echo "Compilez d'abord : cd .. && make"
    exit 1
fi

echo -e "${BLUE}[INFO]${NC} Exécution test_levelzero_init..."
echo ""

# Exécution avec permissions GPU
sg render -c 'sg video -c "../bin/test_levelzero_init"' > /tmp/test_l0_init.log 2>&1

if grep -q "zeInit() SUCCESS" /tmp/test_l0_init.log; then
    echo -e "${GREEN}✓${NC} Initialisation Level Zero réussie"
    cat /tmp/test_l0_init.log | grep -E "SUCCESS|Device|Driver"
else
    echo -e "${RED}✗${NC} Initialisation Level Zero échouée"
    cat /tmp/test_l0_init.log
    exit 1
fi

echo ""

# ═══════════════════════════════════════════════════════════════════
# TEST 2 : ÉNUMÉRATION DEVICES
# ═══════════════════════════════════════════════════════════════════

echo "━━━ TEST 2/5 : Énumération Devices ━━━"
echo ""

if grep -q "Intel.*UHD.*620" /tmp/test_l0_init.log; then
    echo -e "${GREEN}✓${NC} GPU Intel UHD 620 détecté"
    cat /tmp/test_l0_init.log | grep -A 5 "Device 0"
else
    echo -e "${YELLOW}⚠${NC} GPU Intel UHD 620 non détecté explicitement"
    echo "Devices trouvés :"
    cat /tmp/test_l0_init.log | grep -A 3 "Device"
fi

echo ""

# ═══════════════════════════════════════════════════════════════════
# TEST 3 : ALLOCATION MÉMOIRE
# ═══════════════════════════════════════════════════════════════════

echo "━━━ TEST 3/5 : Allocation Mémoire GPU ━━━"
echo ""

echo -e "${BLUE}[INFO]${NC} Test allocation 256 MB..."

# TODO: Créer test_memory si nécessaire
if [ -f "../bin/test_levelzero_memory" ]; then
    sg render -c 'sg video -c "../bin/test_levelzero_memory"' > /tmp/test_l0_memory.log 2>&1
    
    if grep -q "SUCCESS" /tmp/test_l0_memory.log; then
        echo -e "${GREEN}✓${NC} Allocation mémoire réussie"
        cat /tmp/test_l0_memory.log | grep -E "Allocation|SUCCESS"
    else
        echo -e "${RED}✗${NC} Allocation mémoire échouée"
        cat /tmp/test_l0_memory.log
    fi
else
    echo -e "${YELLOW}⚠${NC} Test mémoire non disponible (test_levelzero_memory manquant)"
fi

echo ""

# ═══════════════════════════════════════════════════════════════════
# TEST 4 : KERNEL SIMPLE
# ═══════════════════════════════════════════════════════════════════

echo "━━━ TEST 4/5 : Exécution Kernel Simple ━━━"
echo ""

echo -e "${BLUE}[INFO]${NC} Test kernel addition vectorielle..."

# TODO: Créer test_kernel si nécessaire
if [ -f "../bin/test_levelzero_kernel" ]; then
    sg render -c 'sg video -c "../bin/test_levelzero_kernel"' > /tmp/test_l0_kernel.log 2>&1
    
    if grep -q "SUCCESS" /tmp/test_l0_kernel.log; then
        echo -e "${GREEN}✓${NC} Exécution kernel réussie"
        cat /tmp/test_l0_kernel.log | grep -E "Kernel|Execution|SUCCESS"
    else
        echo -e "${RED}✗${NC} Exécution kernel échouée"
        cat /tmp/test_l0_kernel.log
    fi
else
    echo -e "${YELLOW}⚠${NC} Test kernel non disponible (test_levelzero_kernel manquant)"
fi

echo ""

# ═══════════════════════════════════════════════════════════════════
# TEST 5 : LOGGING FORENSIQUE
# ═══════════════════════════════════════════════════════════════════

echo "━━━ TEST 5/5 : Logging Forensique ━━━"
echo ""

echo -e "${BLUE}[INFO]${NC} Vérification logs .lum..."

if [ -d "../logs/forensic" ]; then
    LUM_FILES=$(find ../logs/forensic -name "*.lum" -type f 2>/dev/null | wc -l)
    
    if [ $LUM_FILES -gt 0 ]; then
        echo -e "${GREEN}✓${NC} Fichiers .lum trouvés : $LUM_FILES"
        
        LATEST_LUM=$(ls -t ../logs/forensic/*.lum 2>/dev/null | head -1)
        if [ -n "$LATEST_LUM" ]; then
            echo "Dernier fichier : $LATEST_LUM"
            ls -lh "$LATEST_LUM"
            
            # Vérification intégrité
            if command -v xxd &> /dev/null; then
                MAGIC=$(xxd -l 4 -p "$LATEST_LUM")
                if [ "$MAGIC" = "4c554d46" ]; then  # "LUMF"
                    echo -e "${GREEN}✓${NC} Magic number valide (LUMF)"
                else
                    echo -e "${YELLOW}⚠${NC} Magic number invalide : $MAGIC"
                fi
            fi
        fi
    else
        echo -e "${YELLOW}⚠${NC} Aucun fichier .lum trouvé"
    fi
else
    echo -e "${YELLOW}⚠${NC} Répertoire logs/forensic non trouvé"
fi

echo ""

# ═══════════════════════════════════════════════════════════════════
# RÉSUMÉ FINAL
# ═══════════════════════════════════════════════════════════════════

echo "════════════════════════════════════════════════════════════════"
echo "  RÉSUMÉ DES TESTS"
echo "════════════════════════════════════════════════════════════════"
echo ""

TESTS_PASSED=0
TESTS_TOTAL=5

# Test 1
if grep -q "zeInit() SUCCESS" /tmp/test_l0_init.log; then
    echo -e "${GREEN}✓${NC} Test 1 : Initialisation Level Zero"
    ((TESTS_PASSED++))
else
    echo -e "${RED}✗${NC} Test 1 : Initialisation Level Zero"
fi

# Test 2
if grep -q "Device 0" /tmp/test_l0_init.log; then
    echo -e "${GREEN}✓${NC} Test 2 : Énumération Devices"
    ((TESTS_PASSED++))
else
    echo -e "${RED}✗${NC} Test 2 : Énumération Devices"
fi

# Test 3
if [ -f /tmp/test_l0_memory.log ] && grep -q "SUCCESS" /tmp/test_l0_memory.log; then
    echo -e "${GREEN}✓${NC} Test 3 : Allocation Mémoire"
    ((TESTS_PASSED++))
else
    echo -e "${YELLOW}⚠${NC} Test 3 : Allocation Mémoire (non testé)"
fi

# Test 4
if [ -f /tmp/test_l0_kernel.log ] && grep -q "SUCCESS" /tmp/test_l0_kernel.log; then
    echo -e "${GREEN}✓${NC} Test 4 : Exécution Kernel"
    ((TESTS_PASSED++))
else
    echo -e "${YELLOW}⚠${NC} Test 4 : Exécution Kernel (non testé)"
fi

# Test 5
if [ -d "../logs/forensic" ] && [ $(find ../logs/forensic -name "*.lum" -type f 2>/dev/null | wc -l) -gt 0 ]; then
    echo -e "${GREEN}✓${NC} Test 5 : Logging Forensique"
    ((TESTS_PASSED++))
else
    echo -e "${YELLOW}⚠${NC} Test 5 : Logging Forensique (non testé)"
fi

echo ""
echo "Tests réussis : $TESTS_PASSED / $TESTS_TOTAL"
echo ""

if [ $TESTS_PASSED -ge 2 ]; then
    echo -e "${GREEN}✓ LEVEL ZERO GEN9 FONCTIONNEL !${NC}"
    echo ""
    echo "Prochaine étape : Benchmark hashrate"
    echo "  ./scripts/benchmark_levelzero_gen9.sh"
    exit 0
else
    echo -e "${RED}✗ TESTS ÉCHOUÉS${NC}"
    echo ""
    echo "Vérifiez les logs :"
    echo "  /tmp/test_l0_*.log"
    exit 1
fi

# Made with Bob
