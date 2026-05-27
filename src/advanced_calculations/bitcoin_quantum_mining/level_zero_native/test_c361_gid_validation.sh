#!/bin/bash
# Test C361 - Validation Pattern GID avec Dump Complet 256 Bytes
# Objectif: PROUVER ou GPU ecrit reellement (offset exact, pattern, BTI)

set -e

cd "$(dirname "$0")"

echo "=== TEST C361 - VALIDATION PATTERN GID + DUMP 256 BYTES ==="
echo "Kernel: tests/test_c361_gid_pattern.bin_kbl.bin"
echo "Objectif: Identifier OU GPU ecrit (offset exact, SIMD, BTI)"
echo ""

# Copier le kernel vers l'emplacement attendu par le runner
cp tests/test_c361_gid_pattern.bin_kbl.bin kernels/btc_sha256_gen9.bin_kbl.bin

echo "✓ Kernel copie vers kernels/btc_sha256_gen9.bin_kbl.bin"
echo ""

# Creer un block header Bitcoin factice (80 bytes)
BLOCK_HEADER="0100000000000000000000000000000000000000000000000000000000000000000000003ba3edfd7a7b12b27ac72c3e67768f617fc81bc3888a51323a9fb8aa4b1e5e4a29ab5f49ffff001d1dac2b7c"

echo "=== TEST 1: 1 thread (gid=0) ==="
echo "Attendu: output[0]=0"
echo ""
timeout 5s ./test_btc_mining_native <<EOF || true
$BLOCK_HEADER
1
EOF

echo ""
echo "=== TEST 2: 8 threads (gid=0-7) ==="
echo "Attendu: output[0]=0, output[1]=1, ..., output[7]=7"
echo ""
timeout 5s ./test_btc_mining_native <<EOF || true
$BLOCK_HEADER
8
EOF

echo ""
echo "=== TEST 3: 64 threads (gid=0-63) ==="
echo "Attendu: output[i]=i pour i=0..63"
echo ""
timeout 5s ./test_btc_mining_native <<EOF || true
$BLOCK_HEADER
64
EOF

echo ""
echo "=== ANALYSE RESULTATS ==="
echo ""

# Lire les derniers logs forensiques
if [ -f "logs/forensic/btc_mining_native_sha256.log" ]; then
    echo "Analyse C361 Buffer Dump:"
    grep -E "C361_BUFFER_DUMP|C361_CHANGED_OFFSETS|C361_PATTERN" logs/forensic/btc_mining_native_sha256.log | tail -50
    echo ""
    
    # Verifier si pattern GID detecte
    if grep -q "C361_PATTERN" logs/forensic/btc_mining_native_sha256.log; then
        echo "✅ PATTERN DETECTE dans les logs!"
        echo "   → Analyser offsets et valeurs pour identifier probleme"
    else
        echo "❌ AUCUN PATTERN - Buffer entierement nul ou inchange"
        echo "   → Probleme critique: GPU n'ecrit PAS du tout"
    fi
else
    echo "❌ Pas de log forensique trouve"
fi

echo ""
echo "=== FIN TEST C361 ==="

# Made with Bob
