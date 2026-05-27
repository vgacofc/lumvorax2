#!/bin/bash
# LumVorax C391 - Test TRIVIAL Sans Optimisation
# Objectif: Valider que le kernel le plus simple possible fonctionne

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

TIMESTAMP=$(date +%Y%m%d_%H%M%S)
LOG_DIR="logs"
LOG_FILE="${LOG_DIR}/execution_c391_trivial_${TIMESTAMP}.log"

mkdir -p "$LOG_DIR"

echo "=== LumVorax C391 - Test TRIVIAL Sans Optimisation ===" | tee "$LOG_FILE"
echo "Timestamp: $TIMESTAMP" | tee -a "$LOG_FILE"
echo "" | tee -a "$LOG_FILE"

# Compilation du kernel SANS optimisation
echo "[1/4] Compilation kernel TRIVIAL (sans optimisation)..." | tee -a "$LOG_FILE"
clang -cc1 -triple spir64-unknown-unknown \
    -cl-std=CL2.0 \
    -O0 \
    -disable-llvm-passes \
    -emit-llvm-bc \
    -o kernels/test_c391_trivial.bc \
    kernels/test_c391_trivial.cl 2>&1 | tee -a "$LOG_FILE"

if [ ! -f kernels/test_c391_trivial.bc ]; then
    echo "❌ ERREUR: Compilation LLVM échouée" | tee -a "$LOG_FILE"
    exit 1
fi
echo "✅ Bytecode LLVM généré" | tee -a "$LOG_FILE"

# Conversion SPIR-V
echo "" | tee -a "$LOG_FILE"
echo "[2/4] Conversion SPIR-V..." | tee -a "$LOG_FILE"
llvm-spirv kernels/test_c391_trivial.bc -o kernels/test_c391_trivial.spv 2>&1 | tee -a "$LOG_FILE"

if [ ! -f kernels/test_c391_trivial.spv ]; then
    echo "❌ ERREUR: Conversion SPIR-V échouée" | tee -a "$LOG_FILE"
    exit 1
fi
echo "✅ SPIR-V généré" | tee -a "$LOG_FILE"

# Compilation Gen9 ISA
echo "" | tee -a "$LOG_FILE"
echo "[3/4] Compilation Gen9 ISA..." | tee -a "$LOG_FILE"
ocloc compile -file kernels/test_c391_trivial.spv \
    -spirv_input \
    -device skl \
    -output kernels/test_c391_trivial \
    -output_no_suffix 2>&1 | tee -a "$LOG_FILE"

if [ ! -f kernels/test_c391_trivial_Gen9core.bin ]; then
    echo "❌ ERREUR: Compilation Gen9 échouée" | tee -a "$LOG_FILE"
    exit 1
fi
echo "✅ Gen9 ISA compilé" | tee -a "$LOG_FILE"

# Désassemblage IGA
echo "" | tee -a "$LOG_FILE"
echo "[3.5/4] Désassemblage IGA..." | tee -a "$LOG_FILE"
iga -p=9 -d kernels/test_c391_trivial_Gen9core.bin -a > kernels/test_c391_trivial.asm 2>&1 | tee -a "$LOG_FILE"
echo "✅ ISA désassemblé dans test_c391_trivial.asm" | tee -a "$LOG_FILE"

# Analyse rapide de l'ISA
echo "" | tee -a "$LOG_FILE"
echo "=== ANALYSE ISA RAPIDE ===" | tee -a "$LOG_FILE"
echo "Instructions 'send' (dataport writes):" | tee -a "$LOG_FILE"
grep -n "send" kernels/test_c391_trivial.asm | head -10 | tee -a "$LOG_FILE"
echo "" | tee -a "$LOG_FILE"
echo "Instructions 'mov' (chargement constante):" | tee -a "$LOG_FILE"
grep -n "0xDEADBEEF\|0xdeadbeef" kernels/test_c391_trivial.asm | tee -a "$LOG_FILE"

# Exécution du test
echo "" | tee -a "$LOG_FILE"
echo "[4/4] Exécution du test..." | tee -a "$LOG_FILE"
./tests/test_gen9_native kernels/test_c391_trivial_Gen9core.bin 2>&1 | tee -a "$LOG_FILE"

echo "" | tee -a "$LOG_FILE"
echo "=== RÉSULTAT FINAL ===" | tee -a "$LOG_FILE"
if grep -q "output\[0\] = 0xDEADBEEF" "$LOG_FILE"; then
    echo "✅ SUCCÈS: Kernel TRIVIAL a écrit 0xDEADBEEF" | tee -a "$LOG_FILE"
    echo "✅ ROOT CAUSE IDENTIFIÉ: Problème dans kernels complexes précédents" | tee -a "$LOG_FILE"
elif grep -q "output\[0\] = 0x00000000" "$LOG_FILE"; then
    echo "❌ ÉCHEC: Kernel TRIVIAL écrit 0x00000000" | tee -a "$LOG_FILE"
    echo "❌ ROOT CAUSE: Problème architectural i915/Surface States/Binding Table" | tee -a "$LOG_FILE"
else
    echo "⚠️  INCERTAIN: Vérifier manuellement le log" | tee -a "$LOG_FILE"
fi

echo "" | tee -a "$LOG_FILE"
echo "Log complet: $LOG_FILE" | tee -a "$LOG_FILE"

# Made with Bob
