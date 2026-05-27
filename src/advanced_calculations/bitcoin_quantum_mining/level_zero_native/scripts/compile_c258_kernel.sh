#!/bin/bash
# Script C258: Compilation kernel ultra-optimisé 30B/nonce
# Objectif: 853 MH/s théorique

set -e

KERNEL_DIR="kernels"
KERNEL_FILE="btc_sha256_mining_c258_ultra.cl"
OUTPUT_BIN="btc_sha256_mining_c258_ultra.bin"
CACHE_DIR="cache_gen9_isa"

echo "═══════════════════════════════════════════════════════════"
echo "  C258: COMPILATION KERNEL ULTRA-OPTIMISÉ"
echo "═══════════════════════════════════════════════════════════"
echo ""
echo "[INFO] Kernel: $KERNEL_FILE"
echo "[INFO] Target: Intel Gen9 (UHD 620)"
echo "[INFO] Optimisations: 30B/nonce, circular buffer W[16], midstate"
echo ""

# Vérifier fichier kernel
if [ ! -f "$KERNEL_DIR/$KERNEL_FILE" ]; then
    echo "[ERREUR] Kernel non trouvé: $KERNEL_DIR/$KERNEL_FILE"
    exit 1
fi

# Créer cache directory
mkdir -p "$CACHE_DIR"

echo "[ÉTAPE 1/2] Compilation directe OpenCL → Gen9 ISA avec ocloc..."
ocloc compile \
    -file "$KERNEL_DIR/$KERNEL_FILE" \
    -device skl \
    -output "$OUTPUT_BIN" \
    -options "-cl-std=CL2.0 -cl-kernel-arg-info -cl-fast-relaxed-math" \
    2>&1 | tee compile_c258.log

if [ ${PIPESTATUS[0]} -ne 0 ]; then
    echo "[ERREUR] Compilation Gen9 ISA échouée"
    exit 1
fi

echo "[OK] Bytecode Gen9 généré: $OUTPUT_BIN"
echo ""

echo ""
echo "[ÉTAPE 2/2] Analyse bytecode..."
if [ -f "$OUTPUT_BIN" ]; then
    SIZE=$(stat -f%z "$OUTPUT_BIN" 2>/dev/null || stat -c%s "$OUTPUT_BIN" 2>/dev/null)
    SIZE_KB=$((SIZE / 1024))
    echo "[INFO] Taille bytecode: $SIZE bytes ($SIZE_KB KB)"
    
    # Comparer avec kernel actuel
    if [ -f "btc_sha256_opt.bin" ]; then
        SIZE_OLD=$(stat -f%z "btc_sha256_opt.bin" 2>/dev/null || stat -c%s "btc_sha256_opt.bin" 2>/dev/null)
        SIZE_OLD_KB=$((SIZE_OLD / 1024))
        DIFF=$((SIZE - SIZE_OLD))
        DIFF_KB=$((DIFF / 1024))
        PERCENT=$(( (DIFF * 100) / SIZE_OLD ))
        
        echo "[COMPARAISON]"
        echo "  Kernel actuel: $SIZE_OLD bytes ($SIZE_OLD_KB KB)"
        echo "  Kernel C258:   $SIZE bytes ($SIZE_KB KB)"
        echo "  Différence:    $DIFF bytes ($DIFF_KB KB, ${PERCENT}%)"
    fi
else
    echo "[ERREUR] Bytecode non généré"
    exit 1
fi

echo ""
echo "[ÉTAPE BONUS] Désassemblage (optionnel)..."
if command -v objdump &> /dev/null; then
    objdump -D "$OUTPUT_BIN" > "$CACHE_DIR/${OUTPUT_BIN%.bin}.asm" 2>/dev/null || true
    if [ -f "$CACHE_DIR/${OUTPUT_BIN%.bin}.asm" ]; then
        ASM_LINES=$(wc -l < "$CACHE_DIR/${OUTPUT_BIN%.bin}.asm")
        echo "[OK] Désassemblage: $ASM_LINES lignes → $CACHE_DIR/${OUTPUT_BIN%.bin}.asm"
    fi
fi

echo ""
echo "═══════════════════════════════════════════════════════════"
echo "  ✅ COMPILATION TERMINÉE"
echo "═══════════════════════════════════════════════════════════"
echo ""
echo "FICHIERS GÉNÉRÉS:"
echo "  - Bytecode Gen9: $OUTPUT_BIN ($SIZE_KB KB)"
echo "  - SPIR-V: $CACHE_DIR/${KERNEL_FILE%.cl}.bc"
echo "  - Logs: compile_c258_step*.log"
echo ""
echo "PROCHAINES ÉTAPES:"
echo "  1. Tester allocation 3GB: ./scripts/test_c258_memory.sh"
echo "  2. Benchmark C258: ./scripts/benchmark_c258.sh"
echo "  3. Profiler bandwidth: intel_gpu_top -o metrics_c258.log"
echo ""

# Made with Bob
