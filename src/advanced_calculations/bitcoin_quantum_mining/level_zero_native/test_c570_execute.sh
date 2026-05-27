#!/bin/bash
# test_c570_execute.sh - Exécution C570 avec forensic logging MAXIMAL
# Cycle C570 - 2026-05-25 18:48 CET
# LUMVORAX GPU Native Mining - ROOT CAUSE #82 SOLUTION

set -e

TIMESTAMP=$(date +%Y%m%d_%H%M%S)
LOG_DIR="logs/forensic"
TEST_NAME="test_c570"

echo "═══════════════════════════════════════════════════════"
echo "    TEST C570 - RELOCATION ENTRIES COMPLÈTES"
echo "    ROOT CAUSE #82: GTT allocation lazy → SOLUTION"
echo "═══════════════════════════════════════════════════════"
echo ""

# Créer répertoire logs
mkdir -p "$LOG_DIR"

echo "[0%] Préparation environnement forensic..."

# 1. Nettoyer dmesg
echo "[5%] Nettoyage dmesg..."
sudo dmesg -C 2>/dev/null || true

# 2. Vérifier debugfs monté
echo "[10%] Vérification debugfs..."
if ! mount | grep -q debugfs; then
    echo "⚠️  Montage debugfs..."
    sudo mount -t debugfs none /sys/kernel/debug 2>/dev/null || true
fi

# 3. Capture état GPU AVANT
echo "[15%] Capture état GPU AVANT..."
sudo cat /sys/kernel/debug/dri/1/i915_gem_objects > "$LOG_DIR/${TEST_NAME}_gem_before_${TIMESTAMP}.log" 2>/dev/null || echo "debugfs unavailable"
sudo cat /sys/kernel/debug/dri/1/i915_engine_info > "$LOG_DIR/${TEST_NAME}_engine_before_${TIMESTAMP}.log" 2>/dev/null || echo "debugfs unavailable"
sudo cat /sys/kernel/debug/dri/1/i915_capabilities > "$LOG_DIR/${TEST_NAME}_caps_${TIMESTAMP}.log" 2>/dev/null || echo "debugfs unavailable"

# 4. Compilation
echo "[20%] Compilation test C570..."
gcc -o tests/test_c570_relocation_complete \
    tests/test_c570_relocation_complete.c \
    -I/usr/include/libdrm \
    -I/usr/include/drm \
    -ldrm \
    -Wall -Wextra -O2 -g

if [ $? -ne 0 ]; then
    echo "❌ Compilation failed"
    exit 1
fi
echo "[30%] ✓ Compilation réussie"

# 5. Lancement intel_gpu_top en background
echo "[35%] Lancement intel_gpu_top..."
sudo intel_gpu_top -o "$LOG_DIR/${TEST_NAME}_gpu_top_${TIMESTAMP}.log" -s 100 &
GPU_TOP_PID=$!
sleep 1

# 6. EXÉCUTION AVEC STRACE NANOSECONDE
echo "[40%] EXÉCUTION TEST C570 AVEC STRACE NANOSECONDE..."
echo ""
echo "═══════════════════════════════════════════════════════"
echo "           DÉBUT EXÉCUTION GPU NATIF"
echo "═══════════════════════════════════════════════════════"
echo ""

# Timeout 10 secondes maximum
timeout 10s strace -tt -T -f -s 512 \
    -e trace=openat,close,ioctl,mmap,munmap,read,write \
    -o "$LOG_DIR/${TEST_NAME}_strace_${TIMESTAMP}.log" \
    tests/test_c570_relocation_complete \
    2>&1 | tee "$LOG_DIR/${TEST_NAME}_execution_${TIMESTAMP}.log"

EXIT_CODE=${PIPESTATUS[0]}

echo ""
echo "═══════════════════════════════════════════════════════"
echo "           FIN EXÉCUTION GPU NATIF"
echo "═══════════════════════════════════════════════════════"
echo ""

# 7. Arrêt intel_gpu_top
echo "[90%] Arrêt intel_gpu_top..."
sudo kill $GPU_TOP_PID 2>/dev/null || true
sleep 1

# 8. Capture état GPU APRÈS
echo "[92%] Capture état GPU APRÈS..."
sudo cat /sys/kernel/debug/dri/1/i915_gem_objects > "$LOG_DIR/${TEST_NAME}_gem_after_${TIMESTAMP}.log" 2>/dev/null || echo "debugfs unavailable"
sudo cat /sys/kernel/debug/dri/1/i915_engine_info > "$LOG_DIR/${TEST_NAME}_engine_after_${TIMESTAMP}.log" 2>/dev/null || echo "debugfs unavailable"
sudo cat /sys/kernel/debug/dri/1/i915_error_state > "$LOG_DIR/${TEST_NAME}_error_${TIMESTAMP}.log" 2>/dev/null || echo "no errors"

# 9. Capture dmesg complet
echo "[94%] Capture dmesg..."
sudo dmesg > "$LOG_DIR/${TEST_NAME}_dmesg_${TIMESTAMP}.log"

# 10. Analyse rapide dmesg
echo "[96%] Analyse dmesg..."
if grep -q "GPU HANG" "$LOG_DIR/${TEST_NAME}_dmesg_${TIMESTAMP}.log"; then
    echo "⚠️  GPU HANG détecté dans dmesg!"
    grep "GPU HANG" "$LOG_DIR/${TEST_NAME}_dmesg_${TIMESTAMP}.log" | tail -5
fi

if grep -q "i915.*error" "$LOG_DIR/${TEST_NAME}_dmesg_${TIMESTAMP}.log"; then
    echo "⚠️  Erreurs i915 détectées!"
    grep "i915.*error" "$LOG_DIR/${TEST_NAME}_dmesg_${TIMESTAMP}.log" | tail -5
fi

# 11. Statistiques logs
echo "[98%] Statistiques logs générés..."
echo ""
echo "=== LOGS FORENSIC GÉNÉRÉS ==="
ls -lh "$LOG_DIR/${TEST_NAME}_"*"_${TIMESTAMP}.log" 2>/dev/null || true
echo ""

# Compter lignes strace
STRACE_LINES=$(wc -l < "$LOG_DIR/${TEST_NAME}_strace_${TIMESTAMP}.log" 2>/dev/null || echo "0")
echo "Lignes strace: $STRACE_LINES"

# Compter lignes dmesg
DMESG_LINES=$(wc -l < "$LOG_DIR/${TEST_NAME}_dmesg_${TIMESTAMP}.log" 2>/dev/null || echo "0")
echo "Lignes dmesg: $DMESG_LINES"

# 12. Résultat final
echo ""
echo "[100%] ✓ Exécution forensic complète"
echo ""
echo "═══════════════════════════════════════════════════════"
echo "           RÉSULTAT TEST C570"
echo "═══════════════════════════════════════════════════════"

if [ $EXIT_CODE -eq 0 ]; then
    echo "✅ TEST RÉUSSI (exit code: 0)"
    echo "🎉 PREMIER NONCE VALIDE GPU NATIF ATTENDU!"
    echo ""
    echo "=== PROCHAINES ÉTAPES ==="
    echo "1. Lecture exhaustive logs ligne par ligne"
    echo "2. Analyse multi-échelle nanoseconde"
    echo "3. Rapport forensique complet"
    echo "4. Validation découvertes scientifiques"
    echo "5. Propagation corrections modules"
else
    echo "❌ TEST ÉCHOUÉ (exit code: $EXIT_CODE)"
    echo "⚠️  Analyse forensique requise"
    echo ""
    echo "=== LOGS À ANALYSER ==="
    echo "1. $LOG_DIR/${TEST_NAME}_execution_${TIMESTAMP}.log"
    echo "2. $LOG_DIR/${TEST_NAME}_strace_${TIMESTAMP}.log"
    echo "3. $LOG_DIR/${TEST_NAME}_dmesg_${TIMESTAMP}.log"
    echo "4. $LOG_DIR/${TEST_NAME}_error_${TIMESTAMP}.log"
fi

echo ""
echo "Timestamp: $TIMESTAMP"
echo "Logs: $LOG_DIR/${TEST_NAME}_*_${TIMESTAMP}.log"
echo ""

exit $EXIT_CODE

# Made with Bob
