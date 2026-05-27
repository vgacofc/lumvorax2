#!/bin/bash
# Test C363 - Validation Fix Binding Table + Relocations SSH
# Objectif: Prouver que GPU écrit correctement avec valeurs magiques

set -e

echo "════════════════════════════════════════════════════════════════"
echo "  TEST C363 - VALIDATION FIX BINDING TABLE + NO SSH RELOCATIONS"
echo "════════════════════════════════════════════════════════════════"
echo ""
echo "Fixes appliqués:"
echo "  ✓ C362: Binding table inversée (BTI=0 → Output)"
echo "  ✓ C363: Relocations SSH supprimées"
echo "  ✓ C363: Adresses GTT écrites POST-EXECBUFFER2"
echo ""
echo "Kernel: test_c363_magic_values.bin_skl.bin"
echo "Attendu: output[0]=0xCAFE0000, output[1]=0xCAFE0001, etc."
echo ""
echo "════════════════════════════════════════════════════════════════"
echo ""

# Copier kernel test
cp tests/test_c363_magic_values.bin_skl.bin kernels/btc_sha256_gen9.bin_skl.bin

# Lancer test avec 10 dispatches, 64 threads
timeout 10s ./bin/test_btc_mining_native 2>&1 | tee logs/test_c363_magic_$(date +%Y%m%d_%H%M%S).log

echo ""
echo "════════════════════════════════════════════════════════════════"
echo "  Logs forensiques: logs/forensic/btc_mining_native_sha256.log"
echo "════════════════════════════════════════════════════════════════"

# Made with Bob
