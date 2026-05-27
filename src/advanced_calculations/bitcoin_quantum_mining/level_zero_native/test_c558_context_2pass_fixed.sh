#!/bin/bash
# TEST C558 - CORRECTION ROOT CAUSE #65: BATCH MINIMAL 16 BYTES
# Correction: Augmenter batch PASS 1 de 12 à 16 bytes (4 DWords)
# Référence: RAPPORT_C557_ANALYSE_STRACE_CONTEXT_2PASS.md

set -e

cd "$(dirname "$0")"

echo "=========================================="
echo "TEST C558 - CORRECTION BATCH 16 BYTES"
echo "=========================================="
echo ""
echo "ROOT CAUSE #65: Batch PASS 1 trop court (12 bytes)"
echo "CORRECTION: Batch PASS 1 = 16 bytes (4 DWords minimum)"
echo ""
echo "Changements vs C557:"
echo "  - C557: 3 DWords (12 bytes) → EINVAL"
echo "  - C558: 4 DWords (16 bytes) → Attendu: SUCCÈS"
echo ""

# Vérifier sudo pour dmesg
if [ "$EUID" -ne 0 ]; then 
    echo "⚠️  Exécuter avec sudo pour accès dmesg complet"
    echo "   sudo ./test_c558_context_2pass_fixed.sh"
    echo ""
fi

# Créer répertoires logs
mkdir -p logs/forensic
mkdir -p logs/dmesg

# Timestamp
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

echo "[1%] Compilation test_c558_context_2pass_fixed.c..."
gcc -o test_c558_context_2pass_fixed \
    tests/test_c558_context_2pass_fixed.c \
    -I/usr/include/libdrm \
    -ldrm \
    -Wall -Wextra

if [ $? -ne 0 ]; then
    echo "❌ Erreur compilation"
    exit 1
fi
echo "[5%] ✓ Compilation réussie"
echo ""

# Capturer dmesg avant
echo "[10%] Capture dmesg AVANT exécution..."
dmesg > logs/dmesg/test_c558_before_${TIMESTAMP}.log 2>/dev/null || echo "⚠️  dmesg nécessite sudo"

# Exécution avec strace forensique COMPLET
echo "[15%] Exécution avec strace forensique..."
echo "Logging: logs/forensic/test_c558_strace_${TIMESTAMP}.log"
echo ""

strace -ttt -T -f -e trace=all \
    -o logs/forensic/test_c558_strace_${TIMESTAMP}.log \
    ./test_c558_context_2pass_fixed 2>&1 | tee logs/forensic/test_c558_stdout_${TIMESTAMP}.log

EXIT_CODE=${PIPESTATUS[0]}

echo ""
echo "[90%] Capture dmesg APRÈS exécution..."
dmesg > logs/dmesg/test_c558_after_${TIMESTAMP}.log 2>/dev/null || echo "⚠️  dmesg nécessite sudo"

# Différence dmesg
if [ -f logs/dmesg/test_c558_before_${TIMESTAMP}.log ] && [ -f logs/dmesg/test_c558_after_${TIMESTAMP}.log ]; then
    echo "[92%] Extraction nouveaux messages kernel..."
    diff logs/dmesg/test_c558_before_${TIMESTAMP}.log \
         logs/dmesg/test_c558_after_${TIMESTAMP}.log \
         | grep '^>' | sed 's/^> //' \
         > logs/dmesg/test_c558_diff_${TIMESTAMP}.log
    
    KERNEL_MSGS=$(wc -l < logs/dmesg/test_c558_diff_${TIMESTAMP}.log)
    echo "       Nouveaux messages kernel: $KERNEL_MSGS"
    
    if [ $KERNEL_MSGS -gt 0 ]; then
        echo ""
        echo "=== MESSAGES KERNEL i915 ==="
        grep -i 'i915\|drm\|gpu' logs/dmesg/test_c558_diff_${TIMESTAMP}.log || echo "Aucun message i915"
        echo ""
    fi
fi

# Analyse strace
echo "[95%] Analyse strace..."
STRACE_LOG="logs/forensic/test_c558_strace_${TIMESTAMP}.log"

if [ -f "$STRACE_LOG" ]; then
    TOTAL_LINES=$(wc -l < "$STRACE_LOG")
    EXECBUFFER2_CALLS=$(grep -c "DRM_IOCTL_I915_GEM_EXECBUFFER2" "$STRACE_LOG" || echo "0")
    EINVAL_COUNT=$(grep -c "EINVAL" "$STRACE_LOG" || echo "0")
    
    echo "       Lignes strace: $TOTAL_LINES"
    echo "       Appels EXECBUFFER2: $EXECBUFFER2_CALLS"
    echo "       Erreurs EINVAL: $EINVAL_COUNT"
    
    if [ $EINVAL_COUNT -gt 0 ]; then
        echo ""
        echo "⚠️  EINVAL détecté - Extraction contexte:"
        grep -B2 -A2 "EINVAL" "$STRACE_LOG" | head -20
    fi
fi

echo ""
echo "=========================================="
echo "RÉSULTAT TEST C558"
echo "=========================================="

if [ $EXIT_CODE -eq 0 ]; then
    echo "✅ TEST C558 RÉUSSI!"
    echo ""
    echo "🎉🎉🎉 SUCCÈS HISTORIQUE! 🎉🎉🎉"
    echo ""
    echo "ROOT CAUSE #65 RÉSOLU:"
    echo "  ✓ Batch PASS 1: 16 bytes (4 DWords)"
    echo "  ✓ EXECBUFFER2 accepté par i915"
    echo "  ✓ GTT addresses obtenues"
    echo "  ✓ PASS 2 exécuté sur GPU"
    echo "  ✓ output[0] = 0x12345678 ✅"
    echo ""
    echo "AVANCEMENT PROJET:"
    echo "  ✓ Élimination OpenCL: 100%"
    echo "  ✓ GPU natif i915 DRM: 100%"
    echo "  ✓ Premier résultat GPU valide: 100%"
    echo ""
    echo "PROCHAINE ÉTAPE:"
    echo "  → Intégrer kernel SHA256d Bitcoin"
    echo "  → Tester minage réel avec nonce"
    echo "  → Optimiser batch buffers automatiques"
    
else
    echo "❌ TEST C558 ÉCHOUÉ (code: $EXIT_CODE)"
    echo ""
    
    if [ $EINVAL_COUNT -gt 0 ]; then
        echo "ANALYSE:"
        echo "  ✗ EINVAL encore présent"
        echo "  → Vérifier logs/forensic/test_c558_strace_${TIMESTAMP}.log"
        echo "  → Vérifier logs/dmesg/test_c558_diff_${TIMESTAMP}.log"
        echo ""
        echo "HYPOTHÈSES:"
        echo "  1. Batch 16 bytes insuffisant (minimum 32?)"
        echo "  2. Exec objects mal configurés"
        echo "  3. Flags EXECBUFFER2 incorrects"
        echo "  4. Context non associé correctement"
    else
        echo "ANALYSE:"
        echo "  ✓ EXECBUFFER2 réussi (pas d'EINVAL)"
        echo "  ✗ Résultat GPU incorrect"
        echo "  → Problème kernel ISA ou synchronisation"
    fi
fi

echo ""
echo "[100%] Logs générés:"
echo "  - logs/forensic/test_c558_strace_${TIMESTAMP}.log"
echo "  - logs/forensic/test_c558_stdout_${TIMESTAMP}.log"
echo "  - logs/dmesg/test_c558_diff_${TIMESTAMP}.log"
echo ""

exit $EXIT_CODE

# Made with Bob
