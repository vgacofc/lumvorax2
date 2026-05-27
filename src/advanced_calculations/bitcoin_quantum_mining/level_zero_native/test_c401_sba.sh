#!/bin/bash
# C401: TEST DÉCISIF - STATE_BASE_ADDRESS

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "=== C401 STATE_BASE_ADDRESS TEST ==="
echo "Timestamp: $(date -Iseconds)"
echo ""
echo "Tests inclus:"
echo "  1. STATE_BASE_ADDRESS (Instruction Base)"
echo "  2. Kernel pointer packing (>> 6)"
echo "  3. Instruction cache invalidate"
echo "  4. Kernel à offset non-zero"
echo ""

# Compiler
echo "Compiling C401..."
gcc -o tests/test_c401_sba \
    tests/test_c401_state_base_address.c \
    -I/usr/include/libdrm \
    -I/usr/include/drm \
    -ldrm \
    -Wall -Wextra -O2

echo "✓ Compilation successful"
echo ""

# Créer logs
mkdir -p logs/forensic

LOG_FILE="logs/forensic/test_c401_sba_$(date +%Y%m%d_%H%M%S).log"

echo "Executing C401 with STATE_BASE_ADDRESS..."
echo "Log: $LOG_FILE"
echo ""

# Exécuter
./tests/test_c401_sba 2>&1 | tee "$LOG_FILE"

EXIT_CODE=${PIPESTATUS[0]}

echo ""
echo "=== SUMMARY ==="
echo "Exit code: $EXIT_CODE"
echo ""

if [ $EXIT_CODE -eq 0 ]; then
    echo "🎉 SUCCESS: STATE_BASE_ADDRESS FIX WORKS!"
    echo ""
    
    # Extraire stats
    NON_ZERO=$(grep "Non-zero:" "$LOG_FILE" | awk '{print $3}')
    MAX_LZ=$(grep "Max leading zeros:" "$LOG_FILE" | awk '{print $5}')
    KERNEL_GTT=$(grep "Kernel:" "$LOG_FILE" | grep "0x" | tail -1 | awk '{print $2}')
    
    echo "Results:"
    echo "  Kernel GTT: $KERNEL_GTT"
    echo "  Non-zero hashes: $NON_ZERO"
    echo "  Max leading zeros: $MAX_LZ bits"
    echo ""
    
    if [ "$MAX_LZ" -ge 1 ]; then
        echo "✅ BREAKTHROUGH CONFIRMED!"
        echo "   → STATE_BASE_ADDRESS was the missing piece"
        echo "   → GPU executed SHA256 successfully"
        echo "   → First near_miss_gpu > 0 bits in native runtime!"
    fi
else
    echo "❌ FAILURE: Tests did not succeed"
    echo ""
    echo "Possible causes:"
    echo "  - SBA format incorrect"
    echo "  - Alignment issues"
    echo "  - Other missing state"
    echo ""
    grep "ERROR:" "$LOG_FILE" || echo "No explicit errors"
fi

echo ""
echo "Full log: $LOG_FILE"

exit $EXIT_CODE

# Made with Bob
