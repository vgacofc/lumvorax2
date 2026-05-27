#!/bin/bash
# LumVorax — Module 17 — Bitcoin Quantum Mining Engine
# compile_opencl_to_spirv.sh — Compilation OpenCL C → SPIR-V
#
# CYCLE C198 — Phase 9 — BUG #10 FIX
# STANDARD_NAMES.md v4.5 §M-BTC-LEVELZERO-SPIRV-C198
#
# OBJECTIF : Compiler kernels OpenCL C en SPIR-V binaire pour Level Zero
#
# USAGE : ./compile_opencl_to_spirv.sh input.cl output.spv

set -e

if [ $# -ne 2 ]; then
    echo "USAGE: $0 <input.cl> <output.spv>"
    exit 1
fi

INPUT_CL="$1"
OUTPUT_SPV="$2"

echo "[C198-SPIRV] Compilation OpenCL C → SPIR-V"
echo "[C198-SPIRV] Input  : $INPUT_CL"
echo "[C198-SPIRV] Output : $OUTPUT_SPV"

# Vérifie si clang est disponible
if ! command -v clang &> /dev/null; then
    echo "[C198-SPIRV] ERREUR : clang non trouvé"
    echo "[C198-SPIRV] Installation : sudo apt install clang llvm-spirv"
    exit 1
fi

# Vérifie si llvm-spirv est disponible
if ! command -v llvm-spirv &> /dev/null; then
    echo "[C198-SPIRV] ERREUR : llvm-spirv non trouvé"
    echo "[C198-SPIRV] Installation : sudo apt install llvm-spirv"
    exit 1
fi

# Étape 1 : Compile OpenCL C → LLVM IR
TEMP_BC="${OUTPUT_SPV%.spv}.bc"
echo "[C198-SPIRV] Étape 1/2 : Compilation OpenCL C → LLVM IR"
clang -cc1 \
    -triple spir64-unknown-unknown \
    -cl-std=CL2.0 \
    -O3 \
    -emit-llvm-bc \
    -o "$TEMP_BC" \
    "$INPUT_CL"

if [ $? -ne 0 ]; then
    echo "[C198-SPIRV] ERREUR : Compilation clang échouée"
    exit 1
fi

echo "[C198-SPIRV] LLVM IR généré : $TEMP_BC ($(stat -c%s "$TEMP_BC") bytes)"

# Étape 2 : Convertit LLVM IR → SPIR-V
echo "[C198-SPIRV] Étape 2/2 : Conversion LLVM IR → SPIR-V"
llvm-spirv "$TEMP_BC" -o "$OUTPUT_SPV"

if [ $? -ne 0 ]; then
    echo "[C198-SPIRV] ERREUR : Conversion llvm-spirv échouée"
    rm -f "$TEMP_BC"
    exit 1
fi

# Cleanup
rm -f "$TEMP_BC"

# Vérifie magic number SPIR-V (0x07230203)
MAGIC=$(xxd -p -l 4 "$OUTPUT_SPV" | tr -d '\n')
if [ "$MAGIC" != "07230203" ]; then
    echo "[C198-SPIRV] AVERTISSEMENT : Magic number incorrect : 0x$MAGIC (attendu: 0x07230203)"
else
    echo "[C198-SPIRV] ✓ Magic number SPIR-V valide : 0x$MAGIC"
fi

echo "[C198-SPIRV] ✓ SPIR-V généré : $OUTPUT_SPV ($(stat -c%s "$OUTPUT_SPV") bytes)"
echo "[C198-SPIRV] Compilation réussie!"

# Made with Bob
