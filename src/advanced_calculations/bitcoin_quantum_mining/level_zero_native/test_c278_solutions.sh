#!/bin/bash
# LumVorax C278 — Test 4 Solutions Anti-GPU Hang
# Script de test automatisé

set -e

# Couleurs
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# ══════════════════════════════════════════════════════════════════════
# INITIALISATION
# ══════════════════════════════════════════════════════════════════════

echo "═══════════════════════════════════════════════════════════════"
echo "  LumVorax C278 — Test 4 Solutions Anti-GPU Hang"
echo "═══════════════════════════════════════════════════════════════"
echo ""

# Créer répertoires
mkdir -p bin logs/c278 logs/c278_tests

# Configuration système
echo "📋 Configuration système actuelle:"
CURRENT_TIMEOUT=$(cat /sys/module/i915/parameters/preempt_timeout_ms 2>/dev/null || echo "N/A")
echo "  - Timeout preemption i915: ${CURRENT_TIMEOUT} ms"
lspci | grep -i vga | head -1 | sed 's/^/  - GPU: /'
echo ""

# ══════════════════════════════════════════════════════════════════════
# COMPILATION
# ══════════════════════════════════════════════════════════════════════

echo "🔨 Compilation des 4 solutions..."
make -f Makefile.c278 all 2>&1 | tee logs/c278/compilation.log

if [ $? -ne 0 ]; then
    echo -e "${RED}❌ Erreur compilation${NC}"
    echo "   Voir: logs/c278/compilation.log"
    exit 1
fi

echo -e "${GREEN}✅ Compilation réussie${NC}"
echo ""

# ══════════════════════════════════════════════════════════════════════
# TEST 1: Solution #1 — Batch Size Réduit (32K)
# ══════════════════════════════════════════════════════════════════════

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "🧪 TEST 1: Batch Size Réduit (32K nonces, 87ms < 640ms)"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

echo "  Objectif: 100 dispatches sans GPU hang"
echo "  Batch: 32768 nonces (vs 262144 original)"
echo "  Temps attendu: ~87ms par dispatch (vs 700ms)"
echo ""

./bin/test_c278_solution1 2>&1 | tee logs/c278_tests/solution1_batch32k.log
TEST1_RESULT=$?

if [ $TEST1_RESULT -eq 0 ]; then
    echo -e "${GREEN}✅ TEST 1 RÉUSSI${NC}"
    SOLUTION1_SUCCESS=1
else
    echo -e "${RED}❌ TEST 1 ÉCHOUÉ${NC}"
    SOLUTION1_SUCCESS=0
fi
echo ""

# ══════════════════════════════════════════════════════════════════════
# TEST 2: Solution #2 — Timeout Augmenté (5000ms)
# ══════════════════════════════════════════════════════════════════════

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "🧪 TEST 2: Timeout Augmenté (5000ms, batch 262K OK)"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

# Tenter augmenter timeout (besoin root)
echo "  Tentative augmentation timeout (besoin sudo)..."
if sudo -n true 2>/dev/null; then
    sudo sh -c 'echo 5000 > /sys/module/i915/parameters/preempt_timeout_ms'
    echo -e "  ${GREEN}✓ Timeout augmenté à 5000ms${NC}"
    TIMEOUT_INCREASED=1
else
    echo -e "  ${YELLOW}⚠ Pas de sudo, test avec timeout actuel${NC}"
    TIMEOUT_INCREASED=0
fi
echo ""

echo "  Objectif: 100 dispatches avec batch original (262K)"
echo "  Timeout: 5000ms (vs 640ms défaut)"
echo "  Temps attendu: ~700ms par dispatch (< 5000ms)"
echo ""

./bin/test_c278_solution2 2>&1 | tee logs/c278_tests/solution2_timeout5000.log
TEST2_RESULT=$?

# Restaurer timeout original
if [ $TIMEOUT_INCREASED -eq 1 ]; then
    sudo sh -c 'echo 640 > /sys/module/i915/parameters/preempt_timeout_ms'
    echo "  ↩ Timeout restauré à 640ms"
fi

if [ $TEST2_RESULT -eq 0 ]; then
    echo -e "${GREEN}✅ TEST 2 RÉUSSI${NC}"
    SOLUTION2_SUCCESS=1
else
    echo -e "${RED}❌ TEST 2 ÉCHOUÉ${NC}"
    SOLUTION2_SUCCESS=0
fi
echo ""

# ══════════════════════════════════════════════════════════════════════
# TEST 3: Solution #3 — Architecture OpenCL
# ══════════════════════════════════════════════════════════════════════

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "🧪 TEST 3: Architecture OpenCL (1 VM + contextes persistants)"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

echo "  Objectif: 100 dispatches sans recyclage contexte"
echo "  CTX_MAX_REUSE: INT_MAX (∞ dispatches)"
echo "  Prouvé: OpenCL fait 100 dispatches OK"
echo ""

./bin/test_c278_solution3 2>&1 | tee logs/c278_tests/solution3_opencl_arch.log
TEST3_RESULT=$?

if [ $TEST3_RESULT -eq 0 ]; then
    echo -e "${GREEN}✅ TEST 3 RÉUSSI${NC}"
    SOLUTION3_SUCCESS=1
else
    echo -e "${RED}❌ TEST 3 ÉCHOUÉ${NC}"
    SOLUTION3_SUCCESS=0
fi
echo ""

# ══════════════════════════════════════════════════════════════════════
# TEST 4: Solution #4 — Batch Size Dynamique
# ══════════════════════════════════════════════════════════════════════

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "🧪 TEST 4: Batch Size Dynamique (calcul automatique)"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

echo "  Objectif: Calcul optimal batch_size basé sur timeout"
echo "  Formule: batch = (timeout_ms * hashrate * 0.9) / 1000"
echo "  Hashrate cible: 408 MH/s"
echo "  Timeout: 640ms → batch optimal ~235K"
echo ""

./bin/test_c278_solution4 2>&1 | tee logs/c278_tests/solution4_dynamic_batch.log
TEST4_RESULT=$?

if [ $TEST4_RESULT -eq 0 ]; then
    echo -e "${GREEN}✅ TEST 4 RÉUSSI${NC}"
    SOLUTION4_SUCCESS=1
else
    echo -e "${RED}❌ TEST 4 ÉCHOUÉ${NC}"
    SOLUTION4_SUCCESS=0
fi
echo ""

# ══════════════════════════════════════════════════════════════════════
# ANALYSE LOGS FORENSIQUES
# ══════════════════════════════════════════════════════════════════════

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "📊 ANALYSE LOGS FORENSIQUES"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

# Vérifier GPU hangs dans dmesg
echo "🔍 Vérification GPU hangs (dmesg)..."
DMESG_HANGS=$(dmesg | grep -c "GPU HANG" || echo "0")
echo "  - GPU hangs détectés: ${DMESG_HANGS}"

if [ $DMESG_HANGS -gt 0 ]; then
    echo -e "  ${YELLOW}⚠ GPU hangs détectés, voir dmesg${NC}"
    dmesg | grep "GPU HANG" | tail -5
fi
echo ""

# Analyser logs de chaque solution
for i in 1 2 3 4; do
    LOG_FILE="logs/c278_tests/solution${i}_*.log"
    if [ -f $LOG_FILE ]; then
        DISPATCHES=$(grep -c "Dispatch.*OK" $LOG_FILE || echo "0")
        ERRORS=$(grep -c "ERROR\|FAILED\|errno" $LOG_FILE || echo "0")
        echo "  Solution ${i}:"
        echo "    - Dispatches réussis: ${DISPATCHES}"
        echo "    - Erreurs détectées: ${ERRORS}"
    fi
done
echo ""

# ══════════════════════════════════════════════════════════════════════
# RAPPORT FINAL
# ══════════════════════════════════════════════════════════════════════

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "📋 RAPPORT FINAL C278"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

TOTAL_SUCCESS=$((SOLUTION1_SUCCESS + SOLUTION2_SUCCESS + SOLUTION3_SUCCESS + SOLUTION4_SUCCESS))

echo "Résultats:"
echo "  - Solution 1 (Batch 32K):        $([ $SOLUTION1_SUCCESS -eq 1 ] && echo -e "${GREEN}✅ RÉUSSI${NC}" || echo -e "${RED}❌ ÉCHOUÉ${NC}")"
echo "  - Solution 2 (Timeout 5000ms):   $([ $SOLUTION2_SUCCESS -eq 1 ] && echo -e "${GREEN}✅ RÉUSSI${NC}" || echo -e "${RED}❌ ÉCHOUÉ${NC}")"
echo "  - Solution 3 (Arch OpenCL):      $([ $SOLUTION3_SUCCESS -eq 1 ] && echo -e "${GREEN}✅ RÉUSSI${NC}" || echo -e "${RED}❌ ÉCHOUÉ${NC}")"
echo "  - Solution 4 (Batch Dynamique):  $([ $SOLUTION4_SUCCESS -eq 1 ] && echo -e "${GREEN}✅ RÉUSSI${NC}" || echo -e "${RED}❌ ÉCHOUÉ${NC}")"
echo ""
echo "Score: ${TOTAL_SUCCESS}/4 solutions réussies"
echo ""

if [ $TOTAL_SUCCESS -eq 4 ]; then
    echo -e "${GREEN}🎉 SUCCÈS TOTAL: Toutes les solutions fonctionnent!${NC}"
    echo ""
    echo "Recommandation:"
    echo "  - Solution 1 (Batch 32K) = Plus simple, pas de sudo"
    echo "  - Solution 3 (Arch OpenCL) = Plus robuste, prouvé 100 dispatches"
    echo "  - Solution 4 (Dynamique) = Plus flexible, s'adapte au système"
elif [ $TOTAL_SUCCESS -ge 2 ]; then
    echo -e "${YELLOW}⚠ SUCCÈS PARTIEL: ${TOTAL_SUCCESS}/4 solutions fonctionnent${NC}"
    echo ""
    echo "Recommandation: Utiliser les solutions réussies"
elif [ $TOTAL_SUCCESS -eq 1 ]; then
    echo -e "${YELLOW}⚠ SUCCÈS MINIMAL: 1/4 solution fonctionne${NC}"
    echo ""
    echo "Recommandation: Analyser logs pour comprendre échecs"
else
    echo -e "${RED}❌ ÉCHEC TOTAL: Aucune solution ne fonctionne${NC}"
    echo ""
    echo "Action requise:"
    echo "  1. Analyser logs/c278_tests/*.log"
    echo "  2. Vérifier dmesg pour GPU hangs"
    echo "  3. Vérifier driver i915 (modinfo i915)"
fi

echo ""
echo "Logs disponibles:"
echo "  - Compilation:     logs/c278/compilation.log"
echo "  - Solution 1:      logs/c278_tests/solution1_batch32k.log"
echo "  - Solution 2:      logs/c278_tests/solution2_timeout5000.log"
echo "  - Solution 3:      logs/c278_tests/solution3_opencl_arch.log"
echo "  - Solution 4:      logs/c278_tests/solution4_dynamic_batch.log"
echo "  - dmesg GPU hangs: dmesg | grep 'GPU HANG'"
echo ""
echo "═══════════════════════════════════════════════════════════════"
echo ""

exit $((4 - TOTAL_SUCCESS))

# Made with Bob
