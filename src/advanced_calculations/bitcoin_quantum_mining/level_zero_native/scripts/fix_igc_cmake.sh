#!/bin/bash

# Script de correction IGC pour compute-runtime v21.49
# Désactive la dépendance IGC qui n'est pas nécessaire pour Gen9

set -e

BUILD_DIR="/tmp/levelzero_gen9_build/compute-runtime"

echo "🔧 Correction configuration CMake pour Gen9 (sans IGC)"

cd "$BUILD_DIR"

# Backup du fichier CMakeLists.txt original
if [ ! -f "CMakeLists.txt.backup" ]; then
    cp CMakeLists.txt CMakeLists.txt.backup
    echo "✓ Backup CMakeLists.txt créé"
fi

# Commenter la ligne qui inclut find_igc.cmake
sed -i 's/^include(cmake\/find_igc.cmake)/#include(cmake\/find_igc.cmake)/' CMakeLists.txt

echo "✓ Dépendance IGC désactivée"

# Reconfigurer CMake
cd build
rm -rf CMakeCache.txt CMakeFiles/

echo "🔄 Reconfiguration CMake..."

cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$HOME/levelzero_gen9" \
    -DSUPPORT_GEN9=1 \
    -DSKIP_UNIT_TESTS=1 \
    -DIGC_DIR=NOTFOUND \
    2>&1 | tee cmake_reconfig.log

if [ $? -eq 0 ]; then
    echo "✅ Configuration CMake réussie !"
    echo ""
    echo "Prochaine étape : Compilation"
    echo "Commande : cd $BUILD_DIR/build && make -j\$(nproc)"
else
    echo "❌ Configuration CMake échouée"
    echo "Voir : $BUILD_DIR/build/cmake_reconfig.log"
    exit 1
fi

# Made with Bob
