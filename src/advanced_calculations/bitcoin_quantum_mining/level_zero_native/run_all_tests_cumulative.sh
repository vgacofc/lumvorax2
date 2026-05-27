#!/bin/bash
# PROTOCOLE TEST CUMULATIF - Exécution automatique
# Date: 2026-05-26
# Auteur: Bob (Expert i915 DRM, Protocoles Test)
# Objectif: Exécuter tous les tests validés de manière cumulative

set -e  # Arrêt sur première erreur

TESTS_DIR="tests"
LOG_DIR="logs/cumulative"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
LOG_FILE="$LOG_DIR/cumulative_$TIMESTAMP.log"

mkdir -p "$LOG_DIR"

echo "========================================" | tee -a "$LOG_FILE"
echo "PROTOCOLE TEST CUMULATIF i915 DRM" | tee -a "$LOG_FILE"
echo "Date: $(date)" | tee -a "$LOG_FILE"
echo "========================================" | tee -a "$LOG_FILE"

# Compteurs
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0
SKIPPED_TESTS=0

# Fonction test
run_test() {
    local test_name=$1
    local test_file=$2
    
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    echo "" | tee -a "$LOG_FILE"
    echo "[$TOTAL_TESTS] Exécution: $test_name" | tee -a "$LOG_FILE"
    echo "    Fichier: $test_file" | tee -a "$LOG_FILE"
    
    if [ -f "$test_file" ]; then
        if ./"$test_file" >> "$LOG_FILE" 2>&1; then
            echo "    ✅ SUCCÈS" | tee -a "$LOG_FILE"
            PASSED_TESTS=$((PASSED_TESTS + 1))
        else
            echo "    ❌ ÉCHEC" | tee -a "$LOG_FILE"
            FAILED_TESTS=$((FAILED_TESTS + 1))
            echo "" | tee -a "$LOG_FILE"
            echo "ERREUR: Test $test_name a échoué" | tee -a "$LOG_FILE"
            echo "Consultez le log: $LOG_FILE" | tee -a "$LOG_FILE"
            exit 1  # Arrêt sur échec
        fi
    else
        echo "    ⏸️  NON IMPLÉMENTÉ (fichier manquant)" | tee -a "$LOG_FILE"
        SKIPPED_TESTS=$((SKIPPED_TESTS + 1))
    fi
}

# Phase 1: Infrastructure i915 DRM
echo "" | tee -a "$LOG_FILE"
echo "=== PHASE 1: Infrastructure i915 DRM ===" | tee -a "$LOG_FILE"
run_test "TEST_001-004: Infrastructure complète" "$TESTS_DIR/test_c564_batch_align8"

# Phase 2: Batch Buffer Construction
echo "" | tee -a "$LOG_FILE"
echo "=== PHASE 2: Batch Buffer Construction ===" | tee -a "$LOG_FILE"
run_test "TEST_005-007: Batch construction" "$TESTS_DIR/test_c564_batch_align8"

# Phase 3: Synchronisation GPU
echo "" | tee -a "$LOG_FILE"
echo "=== PHASE 3: Synchronisation GPU ===" | tee -a "$LOG_FILE"
run_test "TEST_008-010: Synchronisation" "$TESTS_DIR/test_c565_result_verify"

# Phase 4: Allocation GTT
echo "" | tee -a "$LOG_FILE"
echo "=== PHASE 4: Allocation GTT ===" | tee -a "$LOG_FILE"
run_test "TEST_011-012: GTT allocation lazy" "$TESTS_DIR/test_c569_validate_gtt"
run_test "TEST_013: PINNED validation" "$TESTS_DIR/test_c567_pinned_pass1"

# Phase 5: Stratégie 3-PASS
echo "" | tee -a "$LOG_FILE"
echo "=== PHASE 5: Stratégie 3-PASS ===" | tee -a "$LOG_FILE"
run_test "TEST_014-016: Stratégie 3-PASS" "$TESTS_DIR/test_c570_execute.sh"

# Phase 6: Capture OpenCL
echo "" | tee -a "$LOG_FILE"
echo "=== PHASE 6: Capture OpenCL ===" | tee -a "$LOG_FILE"
run_test "TEST_017-018: Capture batch OpenCL" "$TESTS_DIR/test_c590_opencl_batch_capture"
run_test "TEST_019: Multi-captures analysis" "$TESTS_DIR/test_c593_opencl_3captures.sh"

# Phase 7: Validation Hardware
echo "" | tee -a "$LOG_FILE"
echo "=== PHASE 7: Validation Hardware ===" | tee -a "$LOG_FILE"
run_test "TEST_020-022: Hardware validation" "$TESTS_DIR/test_c586_hardware_validation.sh"

# Phase 8: Pointer Tagging (EN DÉVELOPPEMENT)
echo "" | tee -a "$LOG_FILE"
echo "=== PHASE 8: Pointer Tagging (EN DÉVELOPPEMENT) ===" | tee -a "$LOG_FILE"
run_test "TEST_023: Décodage pointer tagging" "$TESTS_DIR/test_c596_pointer_tagging"
run_test "TEST_024: Analyse Compute Runtime" "scripts/analyze_compute_runtime.sh"

# Phase 9: STATE_BASE_ADDRESS (PLANIFIÉ)
echo "" | tee -a "$LOG_FILE"
echo "=== PHASE 9: STATE_BASE_ADDRESS (PLANIFIÉ) ===" | tee -a "$LOG_FILE"
run_test "TEST_025: SBA complet" "$TESTS_DIR/test_c597_sba_complete"

# Phase 10: EU Dispatch Validation (PLANIFIÉ)
echo "" | tee -a "$LOG_FILE"
echo "=== PHASE 10: EU Dispatch Validation (PLANIFIÉ) ===" | tee -a "$LOG_FILE"
run_test "TEST_026: Compteurs OA" "$TESTS_DIR/test_c598_eu_counters.sh"
run_test "TEST_027: Perf counters" "$TESTS_DIR/test_c599_perf_counters.sh"

# Phase 11: Batch GPU Final (PLANIFIÉ)
echo "" | tee -a "$LOG_FILE"
echo "=== PHASE 11: Batch GPU Final (PLANIFIÉ) ===" | tee -a "$LOG_FILE"
run_test "TEST_028: Dump GPU memory" "$TESTS_DIR/test_c600_gpu_memory_dump.sh"

# Résumé
echo "" | tee -a "$LOG_FILE"
echo "========================================" | tee -a "$LOG_FILE"
echo "RÉSUMÉ EXÉCUTION CUMULATIVE" | tee -a "$LOG_FILE"
echo "========================================" | tee -a "$LOG_FILE"
echo "Total tests: $TOTAL_TESTS" | tee -a "$LOG_FILE"
echo "Succès: $PASSED_TESTS" | tee -a "$LOG_FILE"
echo "Échecs: $FAILED_TESTS" | tee -a "$LOG_FILE"
echo "Non implémentés: $SKIPPED_TESTS" | tee -a "$LOG_FILE"

if [ $TOTAL_TESTS -gt 0 ]; then
    IMPLEMENTED=$((TOTAL_TESTS - SKIPPED_TESTS))
    if [ $IMPLEMENTED -gt 0 ]; then
        SUCCESS_RATE=$(( PASSED_TESTS * 100 / IMPLEMENTED ))
        echo "Taux succès (tests implémentés): $SUCCESS_RATE%" | tee -a "$LOG_FILE"
    fi
fi

echo "" | tee -a "$LOG_FILE"

if [ $FAILED_TESTS -eq 0 ]; then
    echo "✅ TOUS LES TESTS IMPLÉMENTÉS VALIDÉS" | tee -a "$LOG_FILE"
    if [ $SKIPPED_TESTS -gt 0 ]; then
        echo "⏸️  $SKIPPED_TESTS tests en développement" | tee -a "$LOG_FILE"
    fi
    exit 0
else
    echo "❌ ÉCHEC - $FAILED_TESTS test(s) échoué(s)" | tee -a "$LOG_FILE"
    echo "Corriger avant de continuer" | tee -a "$LOG_FILE"
    exit 1
fi

# Made with Bob
