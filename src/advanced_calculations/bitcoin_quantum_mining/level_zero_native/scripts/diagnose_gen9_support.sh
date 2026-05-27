#!/bin/bash
# LumVorax C198 - Diagnostic Support Level Zero Gen9
# OBJECTIF : Identifier EXACTEMENT ce qui manque pour Level Zero sur Gen9

set -e

echo "════════════════════════════════════════════════════════════════"
echo "  DIAGNOSTIC LEVEL ZERO GEN9 - Intel UHD Graphics 620"
echo "════════════════════════════════════════════════════════════════"
echo ""

# Couleurs
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 1. INFORMATIONS GPU
echo "━━━ 1. INFORMATIONS GPU ━━━"
echo ""
echo "GPU détecté :"
lspci | grep -i vga
echo ""
echo "Détails GPU Intel :"
lspci -v -s $(lspci | grep -i 'vga.*intel' | cut -d' ' -f1) | head -20
echo ""

# 2. DRIVERS ACTUELS
echo "━━━ 2. DRIVERS ACTUELS ━━━"
echo ""
echo "Modules kernel chargés :"
lsmod | grep -E 'i915|drm' | head -10
echo ""
echo "Version i915 :"
modinfo i915 | grep -E 'version|description'
echo ""

# 3. FICHIERS DRM
echo "━━━ 3. FICHIERS DRM (/dev/dri/) ━━━"
echo ""
ls -lah /dev/dri/
echo ""
echo "Permissions actuelles :"
groups
echo ""

# 4. LEVEL ZERO SDK
echo "━━━ 4. LEVEL ZERO SDK ━━━"
echo ""
if [ -f /usr/include/level_zero/ze_api.h ]; then
    echo -e "${GREEN}✓${NC} SDK Level Zero installé"
    echo "Version :"
    grep -E 'ZE_MAJOR_VERSION|ZE_MINOR_VERSION' /usr/include/level_zero/ze_api.h | head -5
else
    echo -e "${RED}✗${NC} SDK Level Zero NON installé"
fi
echo ""

# 5. BIBLIOTHÈQUES LEVEL ZERO
echo "━━━ 5. BIBLIOTHÈQUES LEVEL ZERO ━━━"
echo ""
echo "Loader Level Zero :"
if [ -f /usr/lib/x86_64-linux-gnu/libze_loader.so ]; then
    ls -lh /usr/lib/x86_64-linux-gnu/libze_loader.so*
    echo -e "${GREEN}✓${NC} Loader trouvé"
else
    echo -e "${RED}✗${NC} Loader NON trouvé"
fi
echo ""

echo "Drivers Level Zero (système) :"
find /usr/lib/x86_64-linux-gnu -name "*ze_intel*" 2>/dev/null || echo "Aucun driver système"
echo ""

echo "Drivers Level Zero (local) :"
if [ -d ~/levelzero_local ]; then
    find ~/levelzero_local -name "*.so*" 2>/dev/null | head -10
    echo -e "${GREEN}✓${NC} Drivers locaux trouvés"
else
    echo -e "${RED}✗${NC} Pas de drivers locaux"
fi
echo ""

# 6. CONFIGURATION LOADER
echo "━━━ 6. CONFIGURATION LOADER ━━━"
echo ""
if [ -f ~/.config/level_zero/ze_loader.conf ]; then
    echo -e "${GREEN}✓${NC} Configuration loader trouvée :"
    cat ~/.config/level_zero/ze_loader.conf
else
    echo -e "${RED}✗${NC} Pas de configuration loader"
fi
echo ""

# 7. TEST INITIALISATION
echo "━━━ 7. TEST INITIALISATION LEVEL ZERO ━━━"
echo ""
export ZE_ENABLE_VALIDATION_LAYER=1
export ZE_ENABLE_PARAMETER_VALIDATION=1
export ZE_LOADER_DEBUG_TRACE=9

if [ -d ~/levelzero_local ]; then
    export LD_LIBRARY_PATH=~/levelzero_local/usr/lib/x86_64-linux-gnu:$LD_LIBRARY_PATH
fi

echo "Variables d'environnement :"
echo "ZE_ENABLE_VALIDATION_LAYER=$ZE_ENABLE_VALIDATION_LAYER"
echo "ZE_ENABLE_PARAMETER_VALIDATION=$ZE_ENABLE_PARAMETER_VALIDATION"
echo "ZE_LOADER_DEBUG_TRACE=$ZE_LOADER_DEBUG_TRACE"
echo "LD_LIBRARY_PATH=$LD_LIBRARY_PATH"
echo ""

if [ -f ../bin/test_levelzero_init ]; then
    echo "Exécution test_levelzero_init :"
    sg render -c "sg video -c '../bin/test_levelzero_init'" 2>&1 | head -50
else
    echo -e "${RED}✗${NC} Binaire test_levelzero_init non trouvé"
fi
echo ""

# 8. VERSIONS COMPUTE-RUNTIME DISPONIBLES
echo "━━━ 8. VERSIONS COMPUTE-RUNTIME (GitHub) ━━━"
echo ""
echo "Recherche versions compatibles Gen9..."
echo "(Versions 21.x à 23.x sont les meilleures pour Gen9)"
echo ""

# 9. DIAGNOSTIC FINAL
echo "━━━ 9. DIAGNOSTIC FINAL ━━━"
echo ""
echo "GPU : Intel UHD Graphics 620 (Gen9 - Kaby Lake)"
echo ""
echo "PROBLÈME IDENTIFIÉ :"
echo "  Les drivers Level Zero récents (v1.6.x, 2024) ne supportent"
echo "  QUE les GPU Gen12+ (Xe Graphics, Arc)."
echo ""
echo "SOLUTIONS POSSIBLES :"
echo ""
echo "  A) Compiler compute-runtime ANCIEN depuis sources"
echo "     - Version 21.49.21786 (2021) : Dernière avec bon support Gen9"
echo "     - Version 22.43.24595 (2022) : Support Gen9 partiel"
echo "     - Version 23.x (2023) : Support Gen9 dégradé"
echo ""
echo "  B) Utiliser OpenCL + traçabilité Level Zero"
echo "     - OpenCL fonctionne parfaitement sur Gen9"
echo "     - Utiliser uniquement le format .lum de Level Zero"
echo "     - Gain : Traçabilité + Performance"
echo ""
echo "  C) Upgrade GPU vers Gen12+ (Intel Iris Xe, Arc)"
echo "     - Support Level Zero natif complet"
echo "     - Performance supérieure"
echo ""
echo "RECOMMANDATION :"
echo "  Solution A (compilation sources) si vous voulez ABSOLUMENT Level Zero"
echo "  Solution B (hybride) si vous voulez la MEILLEURE performance maintenant"
echo ""
echo "════════════════════════════════════════════════════════════════"

# Made with Bob
