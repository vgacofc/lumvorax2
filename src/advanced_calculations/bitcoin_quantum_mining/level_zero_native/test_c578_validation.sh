#!/bin/bash

# TEST C578 - VALIDATION CORRECTIONS WARNINGS
# Vérifie que tous les warnings sont corrigés et compile le test complet

set -e

echo "╔════════════════════════════════════════════════════════════╗"
echo "║  TEST C578 - VALIDATION CORRECTIONS WARNINGS               ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

# Couleurs
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Compteurs
PASSED=0
FAILED=0

# Fonction de test
test_compilation() {
    local file=$1
    local name=$2
    
    echo -n "Testing $name... "
    
    if gcc -Wall -Wextra -Werror -c "$file" -I. -I/usr/include/libdrm -o /tmp/test_c578_$$.o 2>/dev/null; then
        echo -e "${GREEN}✅ PASSED${NC} (0 warnings)"
        PASSED=$((PASSED + 1))
        rm -f /tmp/test_c578_$$.o
        return 0
    else
        echo -e "${RED}❌ FAILED${NC}"
        FAILED=$((FAILED + 1))
        gcc -Wall -Wextra -c "$file" -I. -I/usr/include/libdrm 2>&1 | head -20
        return 1
    fi
}

echo "═══════════════════════════════════════════════════════════"
echo "PHASE 1: Compilation individuelle (avec -Werror)"
echo "═══════════════════════════════════════════════════════════"
echo ""

test_compilation "tests/test_c578_execbuffer2_forensic.c" "test_c578_execbuffer2_forensic.c"
test_compilation "forensic/execbuffer2_forensic.c" "execbuffer2_forensic.c"
test_compilation "forensic/surface_state_gen9.c" "surface_state_gen9.c"

echo ""
echo "═══════════════════════════════════════════════════════════"
echo "PHASE 2: Compilation complète du test"
echo "═══════════════════════════════════════════════════════════"
echo ""

echo -n "Compiling test_c578_execbuffer2_forensic... "

if gcc -Wall -Wextra -Werror \
    tests/test_c578_execbuffer2_forensic.c \
    forensic/execbuffer2_forensic.c \
    forensic/surface_state_gen9.c \
    forensic/forensic_logger.c \
    -I. -I/usr/include/libdrm \
    -ldrm \
    -o test_c578_execbuffer2_forensic 2>/dev/null; then
    echo -e "${GREEN}✅ SUCCESS${NC}"
    PASSED=$((PASSED + 1))
else
    echo -e "${RED}❌ FAILED${NC}"
    FAILED=$((FAILED + 1))
    echo ""
    echo "Compilation errors:"
    gcc -Wall -Wextra \
        tests/test_c578_execbuffer2_forensic.c \
        forensic/execbuffer2_forensic.c \
        forensic/surface_state_gen9.c \
        forensic/forensic_logger.c \
        -I. -I/usr/include/libdrm \
        -ldrm \
        -o test_c578_execbuffer2_forensic 2>&1 | head -30
fi

echo ""
echo "═══════════════════════════════════════════════════════════"
echo "PHASE 3: Vérification binaire"
echo "═══════════════════════════════════════════════════════════"
echo ""

if [ -f "test_c578_execbuffer2_forensic" ]; then
    echo -e "${GREEN}✅ Binary created successfully${NC}"
    ls -lh test_c578_execbuffer2_forensic
    echo ""
    echo "File info:"
    file test_c578_execbuffer2_forensic
    echo ""
    echo "Symbols check:"
    nm test_c578_execbuffer2_forensic | grep -E "(trace_execbuffer2_submit|capture_gtt_allocations|validate_relocations)" | head -5
    PASSED=$((PASSED + 1))
else
    echo -e "${RED}❌ Binary not created${NC}"
    FAILED=$((FAILED + 1))
fi

echo ""
echo "═══════════════════════════════════════════════════════════"
echo "RÉSUMÉ"
echo "═══════════════════════════════════════════════════════════"
echo ""
echo "Tests passed: $PASSED"
echo "Tests failed: $FAILED"
echo ""

if [ $FAILED -eq 0 ]; then
    echo -e "${GREEN}╔════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${GREEN}║  ✅ TOUS LES TESTS RÉUSSIS - 0 WARNINGS                   ║${NC}"
    echo -e "${GREEN}╚════════════════════════════════════════════════════════════╝${NC}"
    echo ""
    echo "Prêt pour exécution runtime:"
    echo "  ./test_c578_execbuffer2_forensic"
    echo ""
    exit 0
else
    echo -e "${RED}╔════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${RED}║  ❌ ÉCHEC - $FAILED TEST(S) ÉCHOUÉ(S)                        ║${NC}"
    echo -e "${RED}╚════════════════════════════════════════════════════════════╝${NC}"
    echo ""
    exit 1
fi

# Made with Bob
