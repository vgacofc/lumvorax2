#!/usr/bin/env bash
# Script de compilation et exécution C255V11 avec correction buffer dynamique
set -euo pipefail

echo "╔════════════════════════════════════════════════════════════╗"
echo "║  Compilation C255V11 NX48 avec Buffer Dynamique           ║"
echo "╚════════════════════════════════════════════════════════════╝"

cd "$(dirname "$0")"

# Compilation
echo "[1/2] Compilation..."
gcc -Wall -Wextra -O2 -std=gnu11 -pthread \
    -I/usr/include/libdrm \
    -Iinclude \
    -o bin/test_c255v11_nx48_dynamic_fixed \
    tests/test_c255v11_nx48_dynamic.c \
    src/btc_gen9_native_runner.c \
    src/btc_i915_drm_forensic_tracker.c \
    -ldrm -lm -lpthread 2>&1 | tee compile.log || {
    echo "❌ Erreur compilation"
    exit 1
}

echo "✅ Compilation réussie"
echo ""

# Exécution
echo "[2/2] Exécution avec sudo..."
echo "Mot de passe sudo: emmaus"
echo ""

echo "emmaus" | sudo -S ./bin/test_c255v11_nx48_dynamic_fixed 2>&1 | \
    tee logs/test_c255v11_fixed_$(date +%Y%m%d_%H%M%S).log

echo ""
echo "✅ Exécution terminée"
echo "📊 Log sauvegardé dans logs/"

# Made with Bob
