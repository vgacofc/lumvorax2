#!/bin/bash
# LumVorax C198 - Installation Level Zero pour Gen9
# Compile compute-runtime v21.49.21786 (dernière version avec support Gen9 complet)

set -e

echo "════════════════════════════════════════════════════════════════"
echo "  INSTALLATION LEVEL ZERO GEN9 - Compute Runtime v21.49"
echo "════════════════════════════════════════════════════════════════"
echo ""

# Couleurs
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Répertoires
BUILD_DIR="/tmp/levelzero_gen9_build"
INSTALL_DIR="$HOME/levelzero_gen9"

echo -e "${BLUE}[INFO]${NC} Répertoire build : $BUILD_DIR"
echo -e "${BLUE}[INFO]${NC} Répertoire install : $INSTALL_DIR"
echo ""

# Nettoyage
if [ -d "$BUILD_DIR" ]; then
    echo -e "${YELLOW}[WARN]${NC} Nettoyage ancien build..."
    rm -rf "$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR"
mkdir -p "$INSTALL_DIR"

cd "$BUILD_DIR"

# ═══════════════════════════════════════════════════════════════════
# ÉTAPE 1 : DÉPENDANCES
# ═══════════════════════════════════════════════════════════════════

echo "━━━ ÉTAPE 1/6 : Vérification dépendances ━━━"
echo ""

DEPS_MISSING=0

# Vérifier CMake
if ! command -v cmake &> /dev/null; then
    echo -e "${RED}✗${NC} CMake manquant"
    DEPS_MISSING=1
else
    echo -e "${GREEN}✓${NC} CMake : $(cmake --version | head -1)"
fi

# Vérifier g++
if ! command -v g++ &> /dev/null; then
    echo -e "${RED}✗${NC} g++ manquant"
    DEPS_MISSING=1
else
    echo -e "${GREEN}✓${NC} g++ : $(g++ --version | head -1)"
fi

# Vérifier git
if ! command -v git &> /dev/null; then
    echo -e "${RED}✗${NC} git manquant"
    DEPS_MISSING=1
else
    echo -e "${GREEN}✓${NC} git : $(git --version)"
fi

if [ $DEPS_MISSING -eq 1 ]; then
    echo ""
    echo -e "${RED}[ERREUR]${NC} Dépendances manquantes. Installation requise :"
    echo "  sudo apt-get install -y cmake g++ git pkg-config libigc-dev intel-igc-opencl-dev"
    exit 1
fi

echo ""

# ═══════════════════════════════════════════════════════════════════
# ÉTAPE 2 : CLONE COMPUTE-RUNTIME v21.49
# ═══════════════════════════════════════════════════════════════════

echo "━━━ ÉTAPE 2/6 : Clone compute-runtime v21.49.21786 ━━━"
echo ""

if [ ! -d "compute-runtime" ]; then
    echo -e "${BLUE}[INFO]${NC} Clonage depuis GitHub..."
    git clone https://github.com/intel/compute-runtime.git
    cd compute-runtime
    
    echo -e "${BLUE}[INFO]${NC} Checkout version 21.49.21786..."
    git checkout 21.49.21786
    
    echo -e "${GREEN}✓${NC} Version 21.49.21786 récupérée"
else
    echo -e "${GREEN}✓${NC} compute-runtime déjà cloné"
    cd compute-runtime
fi

echo ""

# ═══════════════════════════════════════════════════════════════════
# ÉTAPE 3 : CONFIGURATION CMAKE
# ═══════════════════════════════════════════════════════════════════

echo "━━━ ÉTAPE 3/6 : Configuration CMake ━━━"
echo ""

mkdir -p build
cd build

echo -e "${BLUE}[INFO]${NC} Configuration pour Gen9 (SKL, KBL, CFL, WHL)..."

cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR" \
    -DSUPPORT_GEN9=1 \
    -DSUPPORT_GEN11=0 \
    -DSUPPORT_GEN12LP=0 \
    -DSUPPORT_DG1=0 \
    -DSUPPORT_DG2=0 \
    -DBUILD_WITH_L0=ON \
    -DSKIP_UNIT_TESTS=1 \
    2>&1 | tee cmake_config.log

if [ ${PIPESTATUS[0]} -ne 0 ]; then
    echo -e "${RED}[ERREUR]${NC} Configuration CMake échouée"
    echo "Voir : $BUILD_DIR/compute-runtime/build/cmake_config.log"
    exit 1
fi

echo -e "${GREEN}✓${NC} Configuration CMake OK"
echo ""

# ═══════════════════════════════════════════════════════════════════
# ÉTAPE 4 : COMPILATION
# ═══════════════════════════════════════════════════════════════════

echo "━━━ ÉTAPE 4/6 : Compilation (peut prendre 10-30 minutes) ━━━"
echo ""

NPROC=$(nproc)
echo -e "${BLUE}[INFO]${NC} Compilation avec $NPROC threads..."

make -j$NPROC 2>&1 | tee compile.log

if [ ${PIPESTATUS[0]} -ne 0 ]; then
    echo -e "${RED}[ERREUR]${NC} Compilation échouée"
    echo "Voir : $BUILD_DIR/compute-runtime/build/compile.log"
    exit 1
fi

echo -e "${GREEN}✓${NC} Compilation OK"
echo ""

# ═══════════════════════════════════════════════════════════════════
# ÉTAPE 5 : INSTALLATION
# ═══════════════════════════════════════════════════════════════════

echo "━━━ ÉTAPE 5/6 : Installation dans $INSTALL_DIR ━━━"
echo ""

make install 2>&1 | tee install.log

if [ ${PIPESTATUS[0]} -ne 0 ]; then
    echo -e "${RED}[ERREUR]${NC} Installation échouée"
    exit 1
fi

echo -e "${GREEN}✓${NC} Installation OK"
echo ""

# ═══════════════════════════════════════════════════════════════════
# ÉTAPE 6 : CONFIGURATION
# ═══════════════════════════════════════════════════════════════════

echo "━━━ ÉTAPE 6/6 : Configuration Level Zero ━━━"
echo ""

# Mise à jour configuration loader
mkdir -p ~/.config/level_zero
cat > ~/.config/level_zero/ze_loader.conf << EOF
{
  "loader_settings": {
    "driver_search_paths": [
      "$INSTALL_DIR/lib/x86_64-linux-gnu",
      "$INSTALL_DIR/lib"
    ]
  }
}
EOF

echo -e "${GREEN}✓${NC} Configuration loader mise à jour"
echo ""

# Création script d'environnement
cat > "$INSTALL_DIR/setup_env.sh" << 'EOF'
#!/bin/bash
# Configuration environnement Level Zero Gen9

export LD_LIBRARY_PATH="$HOME/levelzero_gen9/lib/x86_64-linux-gnu:$HOME/levelzero_gen9/lib:$LD_LIBRARY_PATH"
export ZE_ENABLE_VALIDATION_LAYER=1
export ZE_ENABLE_PARAMETER_VALIDATION=1

echo "Environnement Level Zero Gen9 configuré"
echo "LD_LIBRARY_PATH=$LD_LIBRARY_PATH"
EOF

chmod +x "$INSTALL_DIR/setup_env.sh"

echo -e "${GREEN}✓${NC} Script environnement créé : $INSTALL_DIR/setup_env.sh"
echo ""

# ═══════════════════════════════════════════════════════════════════
# RÉSUMÉ FINAL
# ═══════════════════════════════════════════════════════════════════

echo "════════════════════════════════════════════════════════════════"
echo -e "${GREEN}  INSTALLATION TERMINÉE AVEC SUCCÈS !${NC}"
echo "════════════════════════════════════════════════════════════════"
echo ""
echo "Drivers Level Zero Gen9 installés dans :"
echo "  $INSTALL_DIR"
echo ""
echo "Fichiers installés :"
ls -lh "$INSTALL_DIR/lib"/*.so* 2>/dev/null | head -10 || echo "  (vérifier $INSTALL_DIR/lib/)"
echo ""
echo "Pour utiliser Level Zero Gen9 :"
echo "  source $INSTALL_DIR/setup_env.sh"
echo ""
echo "Test initialisation :"
echo "  cd /home/lvx/LVX/lumvorax2/src/advanced_calculations/bitcoin_quantum_mining/level_zero_native"
echo "  source $INSTALL_DIR/setup_env.sh"
echo "  sg render -c 'sg video -c \"./bin/test_levelzero_init\"'"
echo ""
echo "════════════════════════════════════════════════════════════════"

# Made with Bob
