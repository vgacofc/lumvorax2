#!/bin/bash
# Script de correction version SPIRV-Tools pour IGC v1.0.17791.9
# Usage: ./fix_spirv_tools_version.sh

set -e

echo "=== CORRECTION VERSION SPIRV-TOOLS POUR IGC ==="
echo "Date: $(date)"
echo ""

# 1. Arrêter compilation en cours
echo "📛 Arrêt compilation IGC en cours..."
pkill -f "make.*igc" 2>/dev/null || true
sleep 2

# 2. Vérifier versions actuelles
echo "🔍 Versions actuelles:"
if [ -d ~/SPIRV-Tools ]; then
    echo "   SPIRV-Tools: $(cd ~/SPIRV-Tools && git describe --tags 2>/dev/null || echo 'unknown')"
fi
if [ -d ~/SPIRV-Headers ]; then
    echo "   SPIRV-Headers: $(cd ~/SPIRV-Headers && git describe --tags 2>/dev/null || echo 'unknown')"
fi
echo ""

# 3. Sauvegarder si nécessaire
if [ -d ~/SPIRV-Tools ]; then
    echo "💾 Sauvegarde SPIRV-Tools actuel..."
    mv ~/SPIRV-Tools ~/SPIRV-Tools.backup.$(date +%Y%m%d_%H%M%S)
fi
if [ -d ~/SPIRV-Headers ]; then
    echo "💾 Sauvegarde SPIRV-Headers actuel..."
    mv ~/SPIRV-Headers ~/SPIRV-Headers.backup.$(date +%Y%m%d_%H%M%S)
fi

# 4. Cloner versions compatibles
echo "📥 Clonage SPIRV-Tools v2021.4..."
cd ~
git clone --depth 1 --branch v2021.4 \
    https://github.com/KhronosGroup/SPIRV-Tools.git

echo "📥 Clonage SPIRV-Headers sdk-1.2.198.1..."
git clone --depth 1 --branch sdk-1.2.198.1 \
    https://github.com/KhronosGroup/SPIRV-Headers.git

# 5. Vérifier clonage
echo ""
echo "✅ Versions installées:"
echo "   SPIRV-Tools: $(cd ~/SPIRV-Tools && git describe --tags)"
echo "   SPIRV-Headers: $(cd ~/SPIRV-Headers && git describe --tags)"
echo ""

# 6. Nettoyer build IGC
echo "🧹 Nettoyage build directory IGC..."
cd ~/igc_full/build
rm -rf IGC/Release/external/SPIRV-Tools 2>/dev/null || true
find . -name "*.o" -delete 2>/dev/null || true
find . -name "CMakeCache.txt" -delete 2>/dev/null || true
echo "   Fichiers .o supprimés"

# 7. Reconfigurer CMake
echo "⚙️  Reconfiguration CMake IGC..."
CC=gcc-9 CXX=g++-9 cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DIGC_OPTION__ARCHITECTURE_TARGET=Linux64 \
    -DIGC_OPTION__ARCHITECTURE_HOST=Linux64 \
    -DCMAKE_PREFIX_PATH=~/gmmlib_local/usr/local \
    > /tmp/igc_cmake_reconfig.log 2>&1

if [ $? -eq 0 ]; then
    echo "✅ CMake reconfiguration réussie"
else
    echo "❌ Erreur CMake - voir /tmp/igc_cmake_reconfig.log"
    exit 1
fi

# 8. Vérifier RAM disponible
FREE_RAM=$(free -m | awk '/^Mem:/{print $7}')
echo ""
echo "💾 RAM disponible: ${FREE_RAM} MB"
if [ $FREE_RAM -lt 500 ]; then
    echo "⚠️  RAM faible - utilisation -j2"
    JOBS=2
elif [ $FREE_RAM -lt 1000 ]; then
    echo "⚙️  RAM moyenne - utilisation -j4"
    JOBS=4
else
    echo "🚀 RAM suffisante - utilisation -j8"
    JOBS=8
fi

# 9. Lancer compilation
echo ""
echo "🔨 Lancement compilation IGC avec -j${JOBS}..."
LOG_FILE="/tmp/igc_compile_$(date +%Y%m%d_%H%M%S).log"
nohup make -j${JOBS} -k > "$LOG_FILE" 2>&1 &
PID=$!

echo "✅ Compilation lancée (PID: $PID)"
echo "📝 Log: $LOG_FILE"
echo ""
echo "📊 Surveillance:"
echo "   watch -n 30 'cd ~/igc_full/build && find . -name \"*.o\" | wc -l'"
echo "   tail -f $LOG_FILE | grep -E '\[.*%\]'"
echo ""
echo "⏱️  Temps estimé: 2h (avec -j${JOBS})"

# Made with Bob
