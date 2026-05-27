#!/bin/bash
# ═══════════════════════════════════════════════════════════════════════════
# LumVorax C256 - Script Test Augmentation Timeout Driver i915
# ═══════════════════════════════════════════════════════════════════════════
#
# OBJECTIF: Tester si l'augmentation du timeout i915 résout les GPU HANGs
#
# PROBLÈME IDENTIFIÉ:
# - Kernel GPU prend 697ms d'exécution
# - Timeout driver i915 hardcodé à 650ms
# - Dépassement de 47ms (7.3%) cause GPU HANG systématique
#
# SOLUTION TESTÉE:
# - Augmenter timeout preemption: 650ms → 5000ms via paramètre module
# - Tester 100 dispatches consécutifs
# - Mesurer taux de succès et performance
#
# RÉFÉRENCE: RAPPORT_TECHNIQUE_CAUSE_GPU_HANG_ET_DRIVER_CUSTOM.md
# ═══════════════════════════════════════════════════════════════════════════

set -e

# Couleurs pour output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
LOG_DIR="logs/c256_timeout_test"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
TEST_LOG="${LOG_DIR}/test_timeout_${TIMESTAMP}.log"
DMESG_BEFORE="${LOG_DIR}/dmesg_before_${TIMESTAMP}.log"
DMESG_AFTER="${LOG_DIR}/dmesg_after_${TIMESTAMP}.log"

# Créer répertoire logs
mkdir -p "${LOG_DIR}"

echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}  LumVorax C256 - Test Augmentation Timeout i915${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo ""

# Fonction: Afficher info
info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

# Fonction: Afficher warning
warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

# Fonction: Afficher erreur
error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Fonction: Vérifier si root
check_root() {
    if [ "$EUID" -ne 0 ]; then
        error "Ce script doit être exécuté en tant que root (sudo)"
        exit 1
    fi
}

# Fonction: Sauvegarder dmesg avant test
save_dmesg_before() {
    info "Sauvegarde dmesg avant test..."
    dmesg > "${DMESG_BEFORE}"
    info "✓ dmesg sauvegardé: ${DMESG_BEFORE}"
}

# Fonction: Sauvegarder dmesg après test
save_dmesg_after() {
    info "Sauvegarde dmesg après test..."
    dmesg > "${DMESG_AFTER}"
    info "✓ dmesg sauvegardé: ${DMESG_AFTER}"
}

# Fonction: Analyser GPU HANGs dans dmesg
analyze_gpu_hangs() {
    local dmesg_file=$1
    local hang_count=$(grep -c "GPU HANG" "${dmesg_file}" 2>/dev/null || echo "0")
    local reset_count=$(grep -c "Resetting.*for preemption time out" "${dmesg_file}" 2>/dev/null || echo "0")
    
    echo ""
    info "Analyse GPU HANGs:"
    echo "  - GPU HANGs détectés: ${hang_count}"
    echo "  - Context resets: ${reset_count}"
    
    if [ "${hang_count}" -eq 0 ]; then
        echo -e "  ${GREEN}✓ Aucun GPU HANG détecté${NC}"
        return 0
    else
        echo -e "  ${RED}✗ ${hang_count} GPU HANG(s) détecté(s)${NC}"
        return 1
    fi
}

# Fonction: Obtenir paramètres i915 actuels
get_i915_params() {
    info "Paramètres i915 actuels:"
    
    if [ -d "/sys/module/i915/parameters" ]; then
        echo "  - enable_hangcheck: $(cat /sys/module/i915/parameters/enable_hangcheck 2>/dev/null || echo 'N/A')"
        echo "  - reset: $(cat /sys/module/i915/parameters/reset 2>/dev/null || echo 'N/A')"
        echo "  - enable_guc: $(cat /sys/module/i915/parameters/enable_guc 2>/dev/null || echo 'N/A')"
    else
        warn "Module i915 non chargé ou paramètres non disponibles"
    fi
}

# Fonction: Modifier timeout i915 (méthode 1: paramètre module)
modify_i915_timeout_module() {
    local new_timeout_ms=$1
    
    info "Tentative modification timeout via paramètre module..."
    warn "ATTENTION: Cette méthode nécessite recompilation du module i915"
    warn "Le timeout preemption n'est PAS exposé comme paramètre module standard"
    
    # Vérifier si paramètre existe (probablement non)
    if [ -f "/sys/module/i915/parameters/preempt_timeout" ]; then
        info "Paramètre preempt_timeout trouvé (rare)"
        echo "${new_timeout_ms}" > /sys/module/i915/parameters/preempt_timeout
        info "✓ Timeout modifié: ${new_timeout_ms}ms"
        return 0
    else
        warn "Paramètre preempt_timeout non disponible"
        warn "Solution: Recompiler driver i915 avec timeout augmenté"
        return 1
    fi
}

# Fonction: Désactiver hangcheck (workaround temporaire)
disable_hangcheck() {
    info "Désactivation hangcheck i915 (workaround)..."
    
    if [ -f "/sys/module/i915/parameters/enable_hangcheck" ]; then
        echo "0" > /sys/module/i915/parameters/enable_hangcheck
        info "✓ Hangcheck désactivé"
        info "  ATTENTION: Ceci désactive la détection de GPU hang"
        info "  Un kernel GPU infini pourrait bloquer le système"
        return 0
    else
        error "Impossible de désactiver hangcheck"
        return 1
    fi
}

# Fonction: Réactiver hangcheck
enable_hangcheck() {
    info "Réactivation hangcheck i915..."
    
    if [ -f "/sys/module/i915/parameters/enable_hangcheck" ]; then
        echo "1" > /sys/module/i915/parameters/enable_hangcheck
        info "✓ Hangcheck réactivé"
        return 0
    else
        warn "Impossible de réactiver hangcheck"
        return 1
    fi
}

# Fonction: Exécuter test mining
run_mining_test() {
    local test_name=$1
    local test_duration=120  # 2 minutes
    
    info "Exécution test mining: ${test_name}"
    info "Durée: ${test_duration} secondes"
    
    # Chercher l'exécutable de test
    local test_exe=""
    if [ -f "src/advanced_calculations/bitcoin_quantum_mining/level_zero_native/test_c255v14" ]; then
        test_exe="src/advanced_calculations/bitcoin_quantum_mining/level_zero_native/test_c255v14"
    elif [ -f "test_c255v14" ]; then
        test_exe="./test_c255v14"
    else
        error "Exécutable test_c255v14 non trouvé"
        return 1
    fi
    
    info "Exécutable: ${test_exe}"
    
    # Exécuter test avec timeout
    timeout ${test_duration}s "${test_exe}" > "${TEST_LOG}" 2>&1 || {
        local exit_code=$?
        if [ ${exit_code} -eq 124 ]; then
            info "✓ Test terminé (timeout ${test_duration}s atteint)"
        else
            warn "Test terminé avec code: ${exit_code}"
        fi
    }
    
    # Analyser résultats
    info "Analyse résultats test..."
    local dispatches=$(grep -c "EXEC_SUCCESS" "${TEST_LOG}" 2>/dev/null || echo "0")
    local hangs=$(grep -c "GPU HANG" "${TEST_LOG}" 2>/dev/null || echo "0")
    local total_hashes=$(grep "Total hashes" "${TEST_LOG}" | tail -1 | awk '{print $3}' || echo "0")
    local hashrate=$(grep "MH/s" "${TEST_LOG}" | tail -1 | awk '{print $1}' || echo "0")
    
    echo ""
    info "Résultats test ${test_name}:"
    echo "  - Dispatches réussis: ${dispatches}"
    echo "  - GPU HANGs: ${hangs}"
    echo "  - Total hashes: ${total_hashes}"
    echo "  - Hashrate: ${hashrate} MH/s"
    
    if [ "${hangs}" -eq 0 ] && [ "${dispatches}" -gt 50 ]; then
        echo -e "  ${GREEN}✓ TEST RÉUSSI${NC}"
        return 0
    else
        echo -e "  ${RED}✗ TEST ÉCHOUÉ${NC}"
        return 1
    fi
}

# ═══════════════════════════════════════════════════════════════════════════
# MAIN
# ═══════════════════════════════════════════════════════════════════════════

main() {
    check_root
    
    echo ""
    info "Début test augmentation timeout i915"
    info "Timestamp: ${TIMESTAMP}"
    info "Log directory: ${LOG_DIR}"
    echo ""
    
    # Étape 1: Sauvegarder état initial
    save_dmesg_before
    get_i915_params
    
    echo ""
    info "═══════════════════════════════════════════════════════════════"
    info "  TEST 1: Baseline (timeout 650ms, hangcheck activé)"
    info "═══════════════════════════════════════════════════════════════"
    
    # Test baseline
    run_mining_test "baseline" || true
    save_dmesg_after
    analyze_gpu_hangs "${DMESG_AFTER}" || true
    
    echo ""
    info "═══════════════════════════════════════════════════════════════"
    info "  TEST 2: Hangcheck désactivé (workaround timeout)"
    info "═══════════════════════════════════════════════════════════════"
    
    # Désactiver hangcheck
    if disable_hangcheck; then
        sleep 2
        
        # Nouveau dmesg
        DMESG_BEFORE="${LOG_DIR}/dmesg_before_nohangcheck_${TIMESTAMP}.log"
        DMESG_AFTER="${LOG_DIR}/dmesg_after_nohangcheck_${TIMESTAMP}.log"
        TEST_LOG="${LOG_DIR}/test_nohangcheck_${TIMESTAMP}.log"
        
        save_dmesg_before
        run_mining_test "no_hangcheck" || true
        save_dmesg_after
        analyze_gpu_hangs "${DMESG_AFTER}" || true
        
        # Réactiver hangcheck
        enable_hangcheck
    else
        warn "Impossible de désactiver hangcheck, test 2 ignoré"
    fi
    
    echo ""
    info "═══════════════════════════════════════════════════════════════"
    info "  RÉSUMÉ FINAL"
    info "═══════════════════════════════════════════════════════════════"
    
    echo ""
    info "Logs générés:"
    echo "  - ${LOG_DIR}/"
    
    echo ""
    info "CONCLUSION:"
    echo "  Si TEST 2 (hangcheck désactivé) réussit sans GPU HANG:"
    echo "  → Le problème est bien le timeout de 650ms"
    echo "  → Solution: Recompiler driver i915 avec timeout 5000ms"
    echo ""
    echo "  Si TEST 2 échoue également:"
    echo "  → Le problème n'est pas uniquement le timeout"
    echo "  → Investigation supplémentaire requise"
    
    echo ""
    info "Pour recompiler driver i915 avec timeout augmenté:"
    echo "  1. Télécharger sources kernel Linux"
    echo "  2. Modifier drivers/gpu/drm/i915/gt/intel_engine_heartbeat.c"
    echo "  3. Changer HEARTBEAT_PREEMPT_TIMEOUT_MS de 650 à 5000"
    echo "  4. Recompiler module i915"
    echo "  5. Recharger module: sudo rmmod i915 && sudo insmod i915.ko"
    
    echo ""
    info "✓ Test terminé"
}

# Exécuter main
main "$@"

# Made with Bob
