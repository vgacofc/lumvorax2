#!/bin/bash

# TEST C578 - EXECBUFFER2 FORENSIC COMPLET
# Compilation et exécution avec logging forensic

set -e

echo "╔════════════════════════════════════════════════════════════╗"
echo "║  COMPILATION TEST C578 - EXECBUFFER2 FORENSIC              ║"
echo "╚════════════════════════════════════════════════════════════╝"

# Créer répertoires logs
mkdir -p logs/forensic

# Compiler
gcc -o test_c578_execbuffer2_forensic \
    tests/test_c578_execbuffer2_forensic.c \
    forensic/execbuffer2_forensic.c \
    forensic/surface_state_gen9.c \
    forensic/forensic_logger.c \
    -I. -I/usr/include/libdrm \
    -ldrm -lm \
    -Wall -Wextra -O2 \
    -DLOG_LEVEL=0

if [ $? -eq 0 ]; then
    echo "✅ Compilation réussie"
else
    echo "❌ Erreur compilation"
    exit 1
fi

echo ""
echo "╔════════════════════════════════════════════════════════════╗"
echo "║  EXÉCUTION TEST C578                                       ║"
echo "╚════════════════════════════════════════════════════════════╝"

# Exécuter avec timeout 10s
timeout 10s ./test_c578_execbuffer2_forensic

EXIT_CODE=$?

echo ""
echo "╔════════════════════════════════════════════════════════════╗"
echo "║  RÉSULTATS                                                 ║"
echo "╚════════════════════════════════════════════════════════════╝"

if [ $EXIT_CODE -eq 0 ]; then
    echo "✅ Tests réussis (exit code: 0)"
elif [ $EXIT_CODE -eq 124 ]; then
    echo "⏱️  Timeout 10s atteint"
else
    echo "❌ Tests échoués (exit code: $EXIT_CODE)"
fi

# Afficher logs
echo ""
echo "=== LOGS FORENSIC ==="
if [ -f logs/forensic/test_c578_execbuffer2.log ]; then
    tail -50 logs/forensic/test_c578_execbuffer2.log
fi

# Afficher Wireshark si existe
if [ -f logs/forensic/test_c578_execbuffer2_wireshark.txt ]; then
    echo ""
    echo "=== WIRESHARK FORMAT ==="
    head -30 logs/forensic/test_c578_execbuffer2_wireshark.txt
fi

exit $EXIT_CODE
