#!/bin/bash
# Build script optimisé LUM/VORAX avec toutes optimisations avancées

set -euo pipefail

echo "=== BUILD OPTIMISÉ LUM/VORAX 77 MODULES ==="

# Détection capacités système
echo "Détection capacités optimisation..."
if grep -q avx512 /proc/cpuinfo 2>/dev/null; then
    SIMD_FLAGS="-mavx512f -mavx512dq"
    echo "✅ AVX-512 détecté"
elif grep -q avx2 /proc/cpuinfo 2>/dev/null; then
    SIMD_FLAGS="-mavx2 -mfma"
    echo "✅ AVX2 détecté" 
else
    SIMD_FLAGS="-msse4.2"
    echo "✅ SSE4.2 utilisé"
fi

# Flags optimisation maximale
export CFLAGS="-O3 -march=native -mtune=native $SIMD_FLAGS -funroll-loops -ffast-math -flto -fPIC -g"
export LDFLAGS="-flto -O3"

echo "Flags: $CFLAGS"

# Nettoyage et build
make clean
make all CFLAGS="$CFLAGS" LDFLAGS="$LDFLAGS"

echo "✅ Build optimisé terminé"
if [ -f bin/lum_vorax ]; then
    echo "📊 Taille binaire: $(stat -c%s bin/lum_vorax) bytes"
    echo "🔧 Optimisations appliquées: SIMD, LTO, native arch"
fi
