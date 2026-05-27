#!/bin/bash
################################################################################
# LumVorax C198 Phase 12A — Compilation Offline Kernels Gen9
# 
# OBJECTIF : Compiler kernels OpenCL C → Gen9 ISA OFFLINE
#            pour éliminer dépendance OpenCL au runtime
#
# USAGE : ./tools/compile_kernels_offline.sh
#
# RÉSULTAT : Headers C avec bytecode embarqué (0% OpenCL runtime)
################################################################################

set -e  # Exit on error

# Couleurs pour output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
KERNELS_DIR="$PROJECT_ROOT/kernels"
OUTPUT_DIR="$PROJECT_ROOT/include/embedded"
CACHE_DIR="$PROJECT_ROOT/cache_gen9_isa"
DEVICE="kbl"  # Gen9 Kaby Lake (UHD 620)

# Outils
OCLOC="/usr/bin/ocloc"
XXD="xxd"

# Timestamp
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

echo -e "${BLUE}═══════════════════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}  LumVorax C198 Phase 12A — Compilation Offline Kernels Gen9${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════════════════${NC}"
echo ""

# Vérifier outils
echo -e "${YELLOW}[1/6]${NC} Vérification outils..."
if [ ! -x "$OCLOC" ]; then
    echo -e "${RED}❌ ERREUR: ocloc non trouvé à $OCLOC${NC}"
    exit 1
fi
if ! command -v $XXD &> /dev/null; then
    echo -e "${RED}❌ ERREUR: xxd non trouvé${NC}"
    exit 1
fi
echo -e "${GREEN}✅ Outils disponibles${NC}"
echo ""

# Créer répertoires
echo -e "${YELLOW}[2/6]${NC} Création répertoires..."
mkdir -p "$OUTPUT_DIR"
mkdir -p "$CACHE_DIR"
echo -e "${GREEN}✅ Répertoires créés${NC}"
echo ""

# Lister kernels
echo -e "${YELLOW}[3/6]${NC} Détection kernels..."
KERNEL_FILES=("$KERNELS_DIR"/*.cl)
KERNEL_COUNT=${#KERNEL_FILES[@]}
echo -e "${GREEN}✅ $KERNEL_COUNT kernels détectés${NC}"
echo ""

# Compiler chaque kernel
echo -e "${YELLOW}[4/6]${NC} Compilation kernels..."
COMPILED_COUNT=0
FAILED_COUNT=0

for kernel_file in "${KERNEL_FILES[@]}"; do
    if [ ! -f "$kernel_file" ]; then
        continue
    fi
    
    kernel_name=$(basename "$kernel_file" .cl)
    echo -e "  ${BLUE}→${NC} Compilation: ${YELLOW}$kernel_name${NC}"
    
    # Fichiers output
    BIN_FILE="$CACHE_DIR/${kernel_name}_gen9.bin"
    HEADER_FILE="$OUTPUT_DIR/${kernel_name}_embedded.h"
    LOG_FILE="$CACHE_DIR/${kernel_name}_compile.log"
    
    # Compiler OpenCL C → Gen9 ISA
    if $OCLOC compile \
        -file "$kernel_file" \
        -device "$DEVICE" \
        -output "$BIN_FILE" \
        > "$LOG_FILE" 2>&1; then
        
        # Vérifier taille binaire
        BIN_SIZE=$(stat -c%s "$BIN_FILE" 2>/dev/null || stat -f%z "$BIN_FILE" 2>/dev/null)
        
        if [ "$BIN_SIZE" -lt 64 ]; then
            echo -e "    ${RED}❌ ERREUR: Binaire trop petit ($BIN_SIZE bytes)${NC}"
            FAILED_COUNT=$((FAILED_COUNT + 1))
            continue
        fi
        
        # Générer header C avec xxd
        {
            echo "/*"
            echo " * LumVorax C198 Phase 12A — Kernel Gen9 ISA Embarqué"
            echo " * Généré automatiquement le $TIMESTAMP"
            echo " * Source: $kernel_name.cl"
            echo " * Device: Gen9 Kaby Lake (UHD 620)"
            echo " * Taille: $BIN_SIZE bytes"
            echo " */"
            echo ""
            echo "#ifndef BTC_KERNEL_${kernel_name^^}_EMBEDDED_H"
            echo "#define BTC_KERNEL_${kernel_name^^}_EMBEDDED_H"
            echo ""
            echo "#include <stdint.h>"
            echo "#include <stddef.h>"
            echo ""
        } > "$HEADER_FILE"
        
        # Ajouter bytecode
        $XXD -i "$BIN_FILE" | sed "s/unsigned char.*\[\]/static const uint8_t ${kernel_name}_gen9_isa[]/" >> "$HEADER_FILE"
        
        # Ajouter footer
        {
            echo ""
            echo "#define ${kernel_name^^}_GEN9_ISA_SIZE ${kernel_name}_gen9_isa_len"
            echo ""
            echo "#endif /* BTC_KERNEL_${kernel_name^^}_EMBEDDED_H */"
        } >> "$HEADER_FILE"
        
        echo -e "    ${GREEN}✅ Compilé: $BIN_SIZE bytes → $HEADER_FILE${NC}"
        COMPILED_COUNT=$((COMPILED_COUNT + 1))
    else
        echo -e "    ${RED}❌ ERREUR compilation (voir $LOG_FILE)${NC}"
        FAILED_COUNT=$((FAILED_COUNT + 1))
    fi
done

echo ""
echo -e "${GREEN}✅ Compilation terminée: $COMPILED_COUNT/$KERNEL_COUNT succès${NC}"
if [ $FAILED_COUNT -gt 0 ]; then
    echo -e "${RED}⚠️  $FAILED_COUNT échecs${NC}"
fi
echo ""

# Générer header master
echo -e "${YELLOW}[5/6]${NC} Génération header master..."
MASTER_HEADER="$OUTPUT_DIR/btc_kernels_embedded_all.h"
{
    echo "/*"
    echo " * LumVorax C198 Phase 12A — Master Header Kernels Embarqués"
    echo " * Généré automatiquement le $TIMESTAMP"
    echo " * Inclut tous les kernels Gen9 ISA compilés offline"
    echo " */"
    echo ""
    echo "#ifndef BTC_KERNELS_EMBEDDED_ALL_H"
    echo "#define BTC_KERNELS_EMBEDDED_ALL_H"
    echo ""
    
    # Inclure tous les headers
    for kernel_file in "${KERNEL_FILES[@]}"; do
        if [ ! -f "$kernel_file" ]; then
            continue
        fi
        kernel_name=$(basename "$kernel_file" .cl)
        HEADER_FILE="$OUTPUT_DIR/${kernel_name}_embedded.h"
        if [ -f "$HEADER_FILE" ]; then
            echo "#include \"${kernel_name}_embedded.h\""
        fi
    done
    
    echo ""
    echo "#endif /* BTC_KERNELS_EMBEDDED_ALL_H */"
} > "$MASTER_HEADER"

echo -e "${GREEN}✅ Header master créé: $MASTER_HEADER${NC}"
echo ""

# Générer rapport
echo -e "${YELLOW}[6/6]${NC} Génération rapport..."
REPORT_FILE="$CACHE_DIR/compilation_report_$TIMESTAMP.txt"
{
    echo "═══════════════════════════════════════════════════════════════════════"
    echo "  LumVorax C198 Phase 12A — Rapport Compilation Offline"
    echo "═══════════════════════════════════════════════════════════════════════"
    echo ""
    echo "Date: $(date)"
    echo "Device: Gen9 Kaby Lake (UHD 620)"
    echo "Kernels détectés: $KERNEL_COUNT"
    echo "Kernels compilés: $COMPILED_COUNT"
    echo "Échecs: $FAILED_COUNT"
    echo ""
    echo "Fichiers générés:"
    echo "─────────────────────────────────────────────────────────────────────"
    
    for kernel_file in "${KERNEL_FILES[@]}"; do
        if [ ! -f "$kernel_file" ]; then
            continue
        fi
        kernel_name=$(basename "$kernel_file" .cl)
        BIN_FILE="$CACHE_DIR/${kernel_name}_gen9.bin"
        HEADER_FILE="$OUTPUT_DIR/${kernel_name}_embedded.h"
        
        if [ -f "$BIN_FILE" ] && [ -f "$HEADER_FILE" ]; then
            BIN_SIZE=$(stat -c%s "$BIN_FILE" 2>/dev/null || stat -f%z "$BIN_FILE" 2>/dev/null)
            echo "  ✅ $kernel_name"
            echo "     Binaire: $BIN_FILE ($BIN_SIZE bytes)"
            echo "     Header:  $HEADER_FILE"
        else
            echo "  ❌ $kernel_name (échec)"
        fi
    done
    
    echo ""
    echo "═══════════════════════════════════════════════════════════════════════"
} > "$REPORT_FILE"

cat "$REPORT_FILE"
echo ""
echo -e "${GREEN}✅ Rapport sauvegardé: $REPORT_FILE${NC}"
echo ""

# Résumé final
echo -e "${BLUE}═══════════════════════════════════════════════════════════════════════${NC}"
if [ $FAILED_COUNT -eq 0 ]; then
    echo -e "${GREEN}✅ SUCCÈS TOTAL: $COMPILED_COUNT kernels compilés offline${NC}"
    echo -e "${GREEN}✅ DÉPENDANCE OPENCL RUNTIME: 0%${NC}"
else
    echo -e "${YELLOW}⚠️  SUCCÈS PARTIEL: $COMPILED_COUNT/$KERNEL_COUNT kernels compilés${NC}"
fi
echo -e "${BLUE}═══════════════════════════════════════════════════════════════════════${NC}"
echo ""

exit 0

# Made with Bob
