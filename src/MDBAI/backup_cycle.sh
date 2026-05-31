#!/bin/bash
# Script de Backup Automatique MDBAI
# Version: 1.0.0
# Date: 2026-05-31T02:21Z
# Usage: ./backup_cycle.sh <cycle_number> <module_name>

set -e  # Exit on error

# Couleurs pour output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Configuration
MDBAI_DIR="/home/lvx/LVX/lumvorax2/src/MDBAI"
FIRECRACKER_DIR="$MDBAI_DIR/firecracker"
BACKUP_DIR="$MDBAI_DIR/backups"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

# Vérifier arguments
if [ $# -lt 2 ]; then
    echo -e "${RED}❌ Usage: $0 <cycle_number> <module_name>${NC}"
    echo "Exemple: $0 172 rpc_interface"
    exit 1
fi

CYCLE_NUM=$1
MODULE_NAME=$2
BACKUP_NAME="backup_C${CYCLE_NUM}_${MODULE_NAME}_${TIMESTAMP}.tar.gz"

echo -e "${YELLOW}🔄 Démarrage backup Cycle C${CYCLE_NUM} - Module ${MODULE_NAME}${NC}"

# Créer répertoire backups si nécessaire
mkdir -p "$BACKUP_DIR"

# Fichiers à sauvegarder
FILES_TO_BACKUP=""

# 1. Fichier source modifié (si existe)
if [ -f "$FIRECRACKER_DIR/src/vmm/src/${MODULE_NAME}.rs" ]; then
    FILES_TO_BACKUP="$FILES_TO_BACKUP firecracker/src/vmm/src/${MODULE_NAME}.rs"
    echo -e "${GREEN}✓${NC} Trouvé: ${MODULE_NAME}.rs"
fi

# 2. Backup original (si existe)
if [ -f "$FIRECRACKER_DIR/src/vmm/src/${MODULE_NAME}.rs.backup" ]; then
    FILES_TO_BACKUP="$FILES_TO_BACKUP firecracker/src/vmm/src/${MODULE_NAME}.rs.backup"
    echo -e "${GREEN}✓${NC} Trouvé: ${MODULE_NAME}.rs.backup"
fi

# 3. Scripts Python de génération
for script in generate_tests_c${CYCLE_NUM}_*.py; do
    if [ -f "$MDBAI_DIR/$script" ]; then
        FILES_TO_BACKUP="$FILES_TO_BACKUP $script"
        echo -e "${GREEN}✓${NC} Trouvé: $script"
    fi
done

# 4. Fichiers Rust générés
for rust_file in c${CYCLE_NUM}_tests_*.rs; do
    if [ -f "$MDBAI_DIR/$rust_file" ]; then
        FILES_TO_BACKUP="$FILES_TO_BACKUP $rust_file"
        echo -e "${GREEN}✓${NC} Trouvé: $rust_file"
    fi
done

# 5. Rapports markdown du cycle
for report in RAPPORT*C${CYCLE_NUM}*.md; do
    if [ -f "$MDBAI_DIR/$report" ]; then
        FILES_TO_BACKUP="$FILES_TO_BACKUP $report"
        echo -e "${GREEN}✓${NC} Trouvé: $report"
    fi
done

# 6. Mapping et corrections
for mapping in MAPPING*C${CYCLE_NUM}*.md; do
    if [ -f "$MDBAI_DIR/$mapping" ]; then
        FILES_TO_BACKUP="$FILES_TO_BACKUP $mapping"
        echo -e "${GREEN}✓${NC} Trouvé: $mapping"
    fi
done

# Vérifier qu'on a au moins un fichier
if [ -z "$FILES_TO_BACKUP" ]; then
    echo -e "${RED}❌ Aucun fichier à sauvegarder trouvé pour C${CYCLE_NUM}${NC}"
    exit 1
fi

# Créer archive
cd "$MDBAI_DIR"
echo -e "${YELLOW}📦 Création archive...${NC}"
tar -czf "$BACKUP_DIR/$BACKUP_NAME" $FILES_TO_BACKUP 2>/dev/null || {
    echo -e "${RED}❌ Erreur lors de la création de l'archive${NC}"
    exit 1
}

# Vérifier archive créée
if [ -f "$BACKUP_DIR/$BACKUP_NAME" ]; then
    BACKUP_SIZE=$(du -h "$BACKUP_DIR/$BACKUP_NAME" | cut -f1)
    FILE_COUNT=$(tar -tzf "$BACKUP_DIR/$BACKUP_NAME" | wc -l)
    
    echo -e "${GREEN}✅ Backup créé avec succès !${NC}"
    echo -e "   📁 Fichier: $BACKUP_NAME"
    echo -e "   📊 Taille: $BACKUP_SIZE"
    echo -e "   📄 Fichiers: $FILE_COUNT"
    
    # Ajouter entrée dans log
    echo "$(date -Iseconds) | C${CYCLE_NUM} | ${MODULE_NAME} | ${BACKUP_NAME} | ${BACKUP_SIZE} | ${FILE_COUNT} fichiers" >> "$BACKUP_DIR/BACKUPS.log"
    
    # Afficher contenu archive
    echo -e "\n${YELLOW}📋 Contenu de l'archive:${NC}"
    tar -tzf "$BACKUP_DIR/$BACKUP_NAME" | head -20
    
    if [ $(tar -tzf "$BACKUP_DIR/$BACKUP_NAME" | wc -l) -gt 20 ]; then
        echo "   ... ($(($FILE_COUNT - 20)) fichiers supplémentaires)"
    fi
    
    echo -e "\n${GREEN}✅ Backup terminé - Fichier sauvegardé dans:${NC}"
    echo -e "   $BACKUP_DIR/$BACKUP_NAME"
    
else
    echo -e "${RED}❌ Erreur: Archive non créée${NC}"
    exit 1
fi

# Afficher statistiques backups
echo -e "\n${YELLOW}📊 Statistiques backups:${NC}"
TOTAL_BACKUPS=$(ls -1 "$BACKUP_DIR"/backup_*.tar.gz 2>/dev/null | wc -l)
TOTAL_SIZE=$(du -sh "$BACKUP_DIR" 2>/dev/null | cut -f1)
echo -e "   Total backups: $TOTAL_BACKUPS"
echo -e "   Espace utilisé: $TOTAL_SIZE"

exit 0

# Made with Bob
