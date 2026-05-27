#!/bin/bash
# Test C364 - Validation Fix Timing Critique
# Objectif: Prouver que GPU lit les bonnes adresses GTT configurées AVANT EXECBUFFER2

set -e

echo "════════════════════════════════════════════════════════════════"
echo "  TEST C364 - VALIDATION FIX TIMING CRITIQUE GTT PRE-EXEC"
echo "════════════════════════════════════════════════════════════════"
echo ""
echo "Fixes appliqués:"
echo "  ✓ C362: Binding table inversée (BTI=0 → Output)"
echo "  ✓ C363: Relocations SSH supprimées"
echo "  ✓ C364: Adresses GTT écrites AVANT EXECBUFFER2 (presumed offsets)"
echo ""
echo "Kernel: test_c363_magic_values.bin_kbl.bin"
echo "Attendu: output[0]=0xCAFE0000, output[1]=0xCAFE0001, etc."
echo ""
echo "════════════════════════════════════════════════════════════════"
echo ""

# Copier kernel test KBL (ocloc crée double extension)
cp tests/test_c363_magic_values.bin_kbl.bin_kbl.bin kernels/btc_sha256_gen9.bin_kbl.bin

# Lancer test avec 10 dispatches, 64 threads, timeout 10s
timeout 10s ./bin/test_btc_mining_native 2>&1 | tee logs/test_c364_timing_$(date +%Y%m%d_%H%M%S).log

echo ""
echo "════════════════════════════════════════════════════════════════"
echo "  Logs forensiques: logs/forensic/btc_mining_native_sha256.log"
echo "════════════════════════════════════════════════════════════════"
echo ""
echo "Vérification attendue dans les logs:"
echo "  - C364_SSH_CONFIGURED_PRE_EXEC: dw1=0x00001000 (Input)"
echo "  - C364_SSH_CONFIGURED_PRE_EXEC: dw1=0x02627000 (Output)"
echo "  - C325_OUTPUT_VALUES: output[0]=0xCAFE0000 ✓"
echo ""

# Made with Bob