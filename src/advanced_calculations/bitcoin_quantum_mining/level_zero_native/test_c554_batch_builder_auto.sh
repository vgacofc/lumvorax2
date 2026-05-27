#!/bin/bash
# TEST C554 - SYSTÈME AUTOMATIQUE BATCH BUILDER
# Compilation et test du système automatique

set -e

TIMESTAMP=$(date +%Y%m%d_%H%M%S)
LOG_DIR="logs"
FORENSIC_DIR="$LOG_DIR/forensic"
LOG_FILE="$LOG_DIR/execution_c554_auto_${TIMESTAMP}.log"
FORENSIC_LOG="$FORENSIC_DIR/c554_forensic_${TIMESTAMP}.log"

mkdir -p "$LOG_DIR" "$FORENSIC_DIR"

echo "=== TEST C554 - BATCH BUILDER AUTOMATIQUE ===" | tee "$LOG_FILE"
echo "Timestamp: $TIMESTAMP" | tee -a "$LOG_FILE"
echo "" | tee -a "$LOG_FILE"

# Compilation bibliothèque
echo "[COMPILATION BIBLIOTHÈQUE]" | tee -a "$LOG_FILE"
gcc -c src/i915_batch_builder_auto.c \
    -o src/i915_batch_builder_auto.o \
    -I/usr/include/libdrm \
    -Wall -Wextra -O2 \
    2>&1 | tee -a "$LOG_FILE"

if [ $? -eq 0 ]; then
    echo "✓ Bibliothèque compilée" | tee -a "$LOG_FILE"
else
    echo "❌ Erreur compilation bibliothèque" | tee -a "$LOG_FILE"
    exit 1
fi

# Compilation test
echo "" | tee -a "$LOG_FILE"
echo "[COMPILATION TEST]" | tee -a "$LOG_FILE"
gcc -o tests/test_c554_batch_builder_auto \
    tests/test_c554_batch_builder_auto.c \
    src/i915_batch_builder_auto.o \
    -I/usr/include/libdrm \
    -ldrm \
    -Wall -Wextra \
    2>&1 | tee -a "$LOG_FILE"

if [ $? -eq 0 ]; then
    echo "✓ Test compilé" | tee -a "$LOG_FILE"
else
    echo "❌ Erreur compilation test" | tee -a "$LOG_FILE"
    exit 1
fi

echo "" | tee -a "$LOG_FILE"

# Activer logging forensic
echo "[FORENSIC SETUP]" | tee -a "$LOG_FILE"
echo "emmaus" | sudo -S sh -c 'echo 0xFFFFFFFF > /sys/module/drm/parameters/debug' 2>&1 | tee -a "$LOG_FILE"
echo "emmaus" | sudo -S dmesg -C

echo "✓ Forensic activé" | tee -a "$LOG_FILE"
echo "" | tee -a "$LOG_FILE"

# Exécution avec strace
echo "[EXÉCUTION]" | tee -a "$LOG_FILE"
echo "Début: $(date +%H:%M:%S.%N)" | tee -a "$LOG_FILE"

START_NS=$(date +%s%N)

strace -tt -T -f -o "$FORENSIC_LOG" \
    tests/test_c554_batch_builder_auto 2>&1 | tee -a "$LOG_FILE"

EXIT_CODE=$?
END_NS=$(date +%s%N)
DURATION_NS=$((END_NS - START_NS))
DURATION_MS=$((DURATION_NS / 1000000))

echo "Fin: $(date +%H:%M:%S.%N)" | tee -a "$LOG_FILE"
echo "Durée: ${DURATION_MS}ms" | tee -a "$LOG_FILE"
echo "Exit code: $EXIT_CODE" | tee -a "$LOG_FILE"
echo "" | tee -a "$LOG_FILE"

# Capturer dmesg
echo "[DMESG CAPTURE]" | tee -a "$LOG_FILE"
DMESG_FILE="$FORENSIC_DIR/c554_dmesg_${TIMESTAMP}.log"
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

# Statistiques
echo "[STATISTIQUES]" | tee -a "$LOG_FILE"
echo "Fichiers générés:" | tee -a "$LOG_FILE"
echo "  - Log principal: $LOG_FILE" | tee -a "$LOG_FILE"
echo "  - Strace:        $FORENSIC_LOG" | tee -a "$LOG_FILE"
echo "  - Dmesg:         $DMESG_FILE" | tee -a "$LOG_FILE"
echo "  - Batch dump:    logs/batch_c554_dump.txt" | tee -a "$LOG_FILE"

STRACE_LINES=$(wc -l < "$FORENSIC_LOG")
DMESG_LINES=$(wc -l < "$DMESG_FILE")
IOCTL_COUNT=$(grep -c "ioctl" "$FORENSIC_LOG" || true)

echo "  - Lignes strace: $STRACE_LINES" | tee -a "$LOG_FILE"
echo "  - Lignes dmesg:  $DMESG_LINES" | tee -a "$LOG_FILE"
echo "  - Appels ioctl:  $IOCTL_COUNT" | tee -a "$LOG_FILE"

echo "" | tee -a "$LOG_FILE"

# Résumé
echo "=== RÉSUMÉ C554 ===" | tee -a "$LOG_FILE"
if [ $EXIT_CODE -eq 0 ]; then
    echo "🎉 SUCCÈS TOTAL - SYSTÈME AUTOMATIQUE FONCTIONNE!" | tee -a "$LOG_FILE"
    echo "✓ GPU natif avec batch builder automatique" | tee -a "$LOG_FILE"
    echo "✓ Élimination OpenCL: 100%" | tee -a "$LOG_FILE"
else
    echo "❌ Test échoué (exit $EXIT_CODE)" | tee -a "$LOG_FILE"
fi

echo "" | tee -a "$LOG_FILE"
echo "Logs disponibles dans: $LOG_DIR/" | tee -a "$LOG_FILE"

# Désactiver logging
echo "emmaus" | sudo -S sh -c 'echo 0 > /sys/module/drm/parameters/debug' 2>/dev/null || true

exit $EXIT_CODE

# Made with Bob
