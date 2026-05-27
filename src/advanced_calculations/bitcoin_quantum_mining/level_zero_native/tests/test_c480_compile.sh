#!/bin/bash
# CYCLE C480 - Script compilation avec Intel Beignet

set -e

echo "=== COMPILATION TEST C480 AVEC INTEL BEIGNET ==="
echo ""

BEIGNET_BUILD="/home/lvx/LVX/lumvorax2/deps/beignet/build"
BEIGNET_SRC="/home/lvx/LVX/lumvorax2/deps/beignet"

# Vérifier que Beignet est compilé
if [ ! -f "$BEIGNET_BUILD/src/libcl.so" ]; then
    echo "ERREUR: Beignet n'est pas encore compilé"
    echo "Exécuter: cd $BEIGNET_BUILD && make -j\$(nproc)"
    exit 1
fi

echo "[1/3] Vérification bibliothèques Beignet..."
ls -lh "$BEIGNET_BUILD/src/libcl.so"
ls -lh "$BEIGNET_BUILD/backend/src/libgbe.so"
echo ""

echo "[2/3] Compilation test_c480_beignet_integration.c..."
gcc -o test_c480_beignet_integration \
    test_c480_beignet_integration.c \
    -I"$BEIGNET_SRC/include" \
    -I"$BEIGNET_SRC/src" \
    -I"$BEIGNET_BUILD/src" \
    -L"$BEIGNET_BUILD/src" \
    -L"$BEIGNET_BUILD/backend/src" \
    -lcl -lgbe \
    -ldrm -ldrm_intel \
    -Wl,-rpath,"$BEIGNET_BUILD/src" \
    -Wl,-rpath,"$BEIGNET_BUILD/backend/src" \
    -Wall -Wextra \
    -g -O2

echo "  ✓ Compilation réussie"
echo ""

echo "[3/3] Exécution test..."
echo ""
./test_c480_beignet_integration

echo ""
echo "=== FIN TEST C480 ==="

# Made with Bob
