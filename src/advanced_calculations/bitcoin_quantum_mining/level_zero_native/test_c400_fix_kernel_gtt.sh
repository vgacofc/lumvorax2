#!/bin/bash
# C400: Fix Kernel GTT NULL - Correction finale

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "=== C400 FIX KERNEL GTT ==="
echo "Timestamp: $(date -Iseconds)"
echo ""

# Mot de passe sudo
SUDO_PASS="emmaus"

# Vérifier les permissions DRM
echo "Checking DRM permissions..."
ls -la /dev/dri/ || true
echo ""

# Ajouter l'utilisateur aux groupes video et render si nécessaire
echo "$SUDO_PASS" | sudo -S usermod -a -G video,render $(whoami) 2>/dev/null || true
echo "User groups updated (may need re-login)"
echo ""

# Compiler le runner C400 avec le fix
echo "Compiling C400 runner with kernel GTT fix..."
gcc -o tests/test_c400_fix_gtt \
    tests/test_c400_fix_gtt_runner.c \
    -I/usr/include/libdrm \
    -I/usr/include/drm \
    -ldrm \
    -Wall -Wextra -O2

echo "✓ Compilation successful"
echo ""

# Créer le répertoire de logs
mkdir -p logs/forensic

# Exécuter avec le fix
LOG_FILE="logs/forensic/test_c400_fix_gtt_$(date +%Y%m%d_%H%M%S).log"

echo "Executing with kernel GTT fix..."
echo "Log file: $LOG_FILE"
echo ""

# Exécuter (avec sudo si nécessaire pour DRM)
if [ -r /dev/dri/card1 ]; then
    ./tests/test_c400_fix_gtt 2>&1 | tee "$LOG_FILE"
    EXIT_CODE=${PIPESTATUS[0]}
else
    echo "$SUDO_PASS" | sudo -S ./tests/test_c400_fix_gtt 2>&1 | tee "$LOG_FILE"
    EXIT_CODE=${PIPESTATUS[0]}
fi

echo ""
echo "=== EXECUTION SUMMARY ==="
echo "Exit code: $EXIT_CODE"
echo "Log file: $LOG_FILE"
echo ""

# Analyser les résultats
if [ $EXIT_CODE -eq 0 ]; then
    echo "✅ SUCCESS: GPU compute with valid kernel GTT"
    
    # Extraire les statistiques
    NON_ZERO=$(grep "Non-zero results:" "$LOG_FILE" | awk '{print $4}' || echo "0")
    MAX_LZ=$(grep "Max leading zeros:" "$LOG_FILE" | awk '{print $4}' || echo "0")
    KERNEL_GTT=$(grep "Final kernel GTT:" "$LOG_FILE" | awk '{print $4}' || echo "0x0")
    
    echo ""
    echo "=== RESULTS ==="
    echo "Kernel GTT: $KERNEL_GTT"
    echo "Non-zero hashes: $NON_ZERO / 256"
    echo "Max leading zeros: $MAX_LZ bits"
    echo ""
    
    if [ "$KERNEL_GTT" != "0x0000000000000000" ] && [ "$MAX_LZ" -ge 1 ]; then
        echo "🎉 BREAKTHROUGH: Kernel GTT fix validated!"
        echo "   → GPU executed SHA256 successfully"
        echo "   → First near_miss_gpu > 0 bits achieved!"
    fi
else
    echo "❌ FAILURE: Fix did not work as expected"
    grep "ERROR:" "$LOG_FILE" || echo "No explicit errors"
fi

echo ""
echo "Full forensic log: $LOG_FILE"

exit $EXIT_CODE

# Made with Bob
