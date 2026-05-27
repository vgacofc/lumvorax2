#!/bin/bash
# TEST C552 - MODÈLE OPENCL COMPLET
# Compilation et exécution avec logging forensic nanoseconde

set -e

TIMESTAMP=$(date +%Y%m%d_%H%M%S)
LOG_DIR="logs"
FORENSIC_DIR="$LOG_DIR/forensic"
TEST_NAME="test_c552_complete_opencl_model"
LOG_FILE="$LOG_DIR/execution_c552_complete_${TIMESTAMP}.log"
FORENSIC_LOG="$FORENSIC_DIR/c552_forensic_${TIMESTAMP}.log"

mkdir -p "$LOG_DIR" "$FORENSIC_DIR"

echo "=== TEST C552 - MODÈLE OPENCL COMPLET ===" | tee "$LOG_FILE"
echo "Timestamp: $TIMESTAMP" | tee -a "$LOG_FILE"
echo "" | tee -a "$LOG_FILE"

# Compilation
echo "[COMPILATION]" | tee -a "$LOG_FILE"
gcc -o "tests/${TEST_NAME}" \
    "tests/${TEST_NAME}.c" \
    -I/usr/include/libdrm \
    -ldrm \
    -Wall -Wextra \
    2>&1 | tee -a "$LOG_FILE"

if [ $? -eq 0 ]; then
    echo "✓ Compilation réussie" | tee -a "$LOG_FILE"
else
    echo "❌ Erreur compilation" | tee -a "$LOG_FILE"
    exit 1
fi

echo "" | tee -a "$LOG_FILE"

# Activer logging forensic kernel
echo "[FORENSIC SETUP]" | tee -a "$LOG_FILE"
echo "Activation logging i915..." | tee -a "$LOG_FILE"

# Activer tous les logs i915
echo "emmaus" | sudo -S sh -c 'echo 0xFFFFFFFF > /sys/module/drm/parameters/debug' 2>&1 | tee -a "$LOG_FILE"
echo "emmaus" | sudo -S sh -c 'echo 0xFFFFFFFF > /sys/module/i915/parameters/debug_guc' 2>&1 | tee -a "$LOG_FILE"

# Vider dmesg
echo "emmaus" | sudo -S dmesg -C

echo "✓ Forensic activé" | tee -a "$LOG_FILE"
echo "" | tee -a "$LOG_FILE"

# Exécution avec strace nanoseconde
echo "[EXÉCUTION]" | tee -a "$LOG_FILE"
echo "Début: $(date +%H:%M:%S.%N)" | tee -a "$LOG_FILE"

START_NS=$(date +%s%N)

strace -tt -T -f -o "$FORENSIC_LOG" \
    "tests/${TEST_NAME}" 2>&1 | tee -a "$LOG_FILE"

EXIT_CODE=$?
END_NS=$(date +%s%N)
DURATION_NS=$((END_NS - START_NS))
DURATION_MS=$((DURATION_NS / 1000000))

echo "Fin: $(date +%H:%M:%S.%N)" | tee -a "$LOG_FILE"
echo "Durée: ${DURATION_MS}ms" | tee -a "$LOG_FILE"
echo "Exit code: $EXIT_CODE" | tee -a "$LOG_FILE"
echo "" | tee -a "$LOG_FILE"

# Capturer dmesg immédiatement
echo "[DMESG CAPTURE]" | tee -a "$LOG_FILE"
DMESG_FILE="$FORENSIC_DIR/c552_dmesg_${TIMESTAMP}.log"
echo "emmaus" | sudo -S dmesg > "$DMESG_FILE"

# Analyser GPU HANG
if grep -q "GPU HANG" "$DMESG_FILE"; then
    echo "⚠️  GPU HANG DÉTECTÉ!" | tee -a "$LOG_FILE"
    echo "" | tee -a "$LOG_FILE"
    echo "=== ANALYSE GPU HANG ===" | tee -a "$LOG_FILE"
    grep -A 20 "GPU HANG" "$DMESG_FILE" | tee -a "$LOG_FILE"
else
    echo "✓ Pas de GPU HANG" | tee -a "$LOG_FILE"
fi

echo "" | tee -a "$LOG_FILE"

# Statistiques forensic
echo "[STATISTIQUES FORENSIC]" | tee -a "$LOG_FILE"
echo "Fichiers générés:" | tee -a "$LOG_FILE"
echo "  - Log principal: $LOG_FILE" | tee -a "$LOG_FILE"
echo "  - Strace:        $FORENSIC_LOG" | tee -a "$LOG_FILE"
echo "  - Dmesg:         $DMESG_FILE" | tee -a "$LOG_FILE"

STRACE_LINES=$(wc -l < "$FORENSIC_LOG")
DMESG_LINES=$(wc -l < "$DMESG_FILE")

echo "  - Lignes strace: $STRACE_LINES" | tee -a "$LOG_FILE"
echo "  - Lignes dmesg:  $DMESG_LINES" | tee -a "$LOG_FILE"

# Compter appels ioctl
IOCTL_COUNT=$(grep -c "ioctl" "$FORENSIC_LOG" || true)
echo "  - Appels ioctl:  $IOCTL_COUNT" | tee -a "$LOG_FILE"

echo "" | tee -a "$LOG_FILE"

# Résumé final
echo "=== RÉSUMÉ C552 ===" | tee -a "$LOG_FILE"
if [ $EXIT_CODE -eq 0 ]; then
    echo "✓ Test terminé avec succès" | tee -a "$LOG_FILE"
    
    # Vérifier si output[0] = 0x12345678
    if grep -q "0x12345678" "$LOG_FILE"; then
        echo "🎉 SUCCÈS TOTAL - GPU NATIF FONCTIONNE!" | tee -a "$LOG_FILE"
    else
        echo "⚠️  Test OK mais résultat incorrect" | tee -a "$LOG_FILE"
    fi
else
    echo "❌ Test échoué (exit $EXIT_CODE)" | tee -a "$LOG_FILE"
fi

echo "" | tee -a "$LOG_FILE"
echo "Logs disponibles dans: $LOG_DIR/" | tee -a "$LOG_FILE"

# Désactiver logging forensic
echo "emmaus" | sudo -S sh -c 'echo 0 > /sys/module/drm/parameters/debug' 2>/dev/null || true

exit $EXIT_CODE

# Made with Bob
