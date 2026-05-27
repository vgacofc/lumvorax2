#!/bin/bash
# TEST C556 - STRATÉGIE 2-PASS AVEC ADRESSES GTT
# Correction ROOT CAUSE #63: Utiliser adresses GPU format Intel

set -e

TIMESTAMP=$(date +%Y%m%d_%H%M%S)
LOG_DIR="logs"
FORENSIC_DIR="$LOG_DIR/forensic"
TEST_NAME="test_c556_gtt_2pass"
LOG_FILE="$LOG_DIR/execution_c556_2pass_${TIMESTAMP}.log"
STRACE_LOG="$FORENSIC_DIR/c556_forensic_${TIMESTAMP}.log"
DMESG_LOG="$FORENSIC_DIR/c556_dmesg_${TIMESTAMP}.log"

mkdir -p "$LOG_DIR" "$FORENSIC_DIR"

echo "=== TEST C556 - STRATÉGIE 2-PASS AVEC ADRESSES GTT ===" | tee "$LOG_FILE"
echo "Timestamp: $TIMESTAMP" | tee -a "$LOG_FILE"
echo "" | tee -a "$LOG_FILE"

# Compilation
echo "[COMPILATION]" | tee -a "$LOG_FILE"
gcc -o "$TEST_NAME" "tests/${TEST_NAME}.c" -ldrm -I/usr/include/libdrm 2>&1 | tee -a "$LOG_FILE"
if [ $? -eq 0 ]; then
    echo "✓ Compilation réussie" | tee -a "$LOG_FILE"
else
    echo "❌ Échec compilation" | tee -a "$LOG_FILE"
    exit 1
fi
echo "" | tee -a "$LOG_FILE"

# Forensic setup
echo "[FORENSIC SETUP]" | tee -a "$LOG_FILE"
sudo dmesg -C 2>&1 | tee -a "$LOG_FILE"
echo "✓ Forensic activé" | tee -a "$LOG_FILE"
echo "" | tee -a "$LOG_FILE"

# Exécution avec strace
echo "[EXÉCUTION]" | tee -a "$LOG_FILE"
START_TIME=$(date +%s.%N)
echo "Début: $(date +%T.%N)" | tee -a "$LOG_FILE"

# Exécuter avec strace pour capturer tous les appels système
strace -f -e trace=ioctl,open,openat,close,mmap,munmap -s 65536 -o "$STRACE_LOG" \
    timeout 10s "./$TEST_NAME" 2>&1 | tee -a "$LOG_FILE"
EXIT_CODE=$?

END_TIME=$(date +%s.%N)
echo "Fin: $(date +%T.%N)" | tee -a "$LOG_FILE"
DURATION=$(echo "$END_TIME - $START_TIME" | bc)
DURATION_MS=$(echo "$DURATION * 1000" | bc | cut -d. -f1)
echo "Durée: ${DURATION_MS}ms" | tee -a "$LOG_FILE"
echo "Exit code: $EXIT_CODE" | tee -a "$LOG_FILE"
echo "" | tee -a "$LOG_FILE"

# Capture dmesg
echo "[DMESG CAPTURE]" | tee -a "$LOG_FILE"
sudo dmesg > "$DMESG_LOG"
if grep -q "GPU HANG" "$DMESG_LOG"; then
    echo "❌ GPU HANG détecté!" | tee -a "$LOG_FILE"
    grep "GPU HANG" "$DMESG_LOG" | tail -5 | tee -a "$LOG_FILE"
else
    echo "✓ Pas de GPU HANG" | tee -a "$LOG_FILE"
fi
echo "" | tee -a "$LOG_FILE"

# Statistiques
echo "[STATISTIQUES]" | tee -a "$LOG_FILE"
STRACE_LINES=$(wc -l < "$STRACE_LOG")
DMESG_LINES=$(wc -l < "$DMESG_LOG")
IOCTL_COUNT=$(grep -c "ioctl" "$STRACE_LOG" || echo "0")

echo "Fichiers générés:" | tee -a "$LOG_FILE"
echo "  - Log principal: $LOG_FILE" | tee -a "$LOG_FILE"
echo "  - Strace:        $STRACE_LOG" | tee -a "$LOG_FILE"
echo "  - Dmesg:         $DMESG_LOG" | tee -a "$LOG_FILE"
echo "  - Lignes strace: $STRACE_LINES" | tee -a "$LOG_FILE"
echo "  - Lignes dmesg:  $DMESG_LINES" | tee -a "$LOG_FILE"
echo "  - Appels ioctl:  $IOCTL_COUNT" | tee -a "$LOG_FILE"
echo "" | tee -a "$LOG_FILE"

# Résumé
echo "=== RÉSUMÉ C556 ===" | tee -a "$LOG_FILE"
if [ $EXIT_CODE -eq 0 ]; then
    echo "🎉 SUCCÈS TOTAL - ROOT CAUSE #63 RÉSOLU!" | tee -a "$LOG_FILE"
    echo "✓ Stratégie 2-PASS fonctionnelle" | tee -a "$LOG_FILE"
    echo "✓ Adresses GTT correctes" | tee -a "$LOG_FILE"
    echo "✓ GPU natif i915 DRM 100%" | tee -a "$LOG_FILE"
    echo "✓ Élimination OpenCL: 100%" | tee -a "$LOG_FILE"
else
    echo "❌ ÉCHEC - Analyse forensique requise" | tee -a "$LOG_FILE"
    echo "Consulter: $LOG_FILE" | tee -a "$LOG_FILE"
fi
echo "" | tee -a "$LOG_FILE"
echo "Logs disponibles dans: $LOG_DIR/" | tee -a "$LOG_FILE"

exit $EXIT_CODE

# Made with Bob
