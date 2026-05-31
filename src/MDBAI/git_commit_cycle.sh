#!/bin/bash
# Script Git Commit Automatique MDBAI
# Version: 1.0.0
# Date: 2026-05-31T02:22Z
# Usage: ./git_commit_cycle.sh <cycle_number> <module_name> <coverage_gain>

set -e  # Exit on error

# Couleurs pour output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
MDBAI_DIR="/home/lvx/LVX/lumvorax2/src/MDBAI"
FIRECRACKER_DIR="$MDBAI_DIR/firecracker"

# Vérifier arguments
if [ $# -lt 3 ]; then
    echo -e "${RED}❌ Usage: $0 <cycle_number> <module_name> <coverage_gain>${NC}"
    echo "Exemple: $0 172 rpc_interface 49.95"
    exit 1
fi

CYCLE_NUM=$1
MODULE_NAME=$2
COVERAGE_GAIN=$3

echo -e "${YELLOW}🔄 Démarrage commit Git Cycle C${CYCLE_NUM}${NC}"

# Vérifier qu'on est dans le bon répertoire
cd "$FIRECRACKER_DIR" || {
    echo -e "${RED}❌ Erreur: Impossible d'accéder à $FIRECRACKER_DIR${NC}"
    exit 1
}

# Vérifier que le fichier modifié existe
MODULE_FILE="src/vmm/src/${MODULE_NAME}.rs"
if [ ! -f "$MODULE_FILE" ]; then
    echo -e "${RED}❌ Erreur: Fichier $MODULE_FILE non trouvé${NC}"
    exit 1
fi

# Vérifier qu'il y a des modifications
if ! git diff --quiet "$MODULE_FILE"; then
    echo -e "${GREEN}✓${NC} Modifications détectées dans $MODULE_FILE"
else
    echo -e "${YELLOW}⚠${NC}  Aucune modification détectée dans $MODULE_FILE"
    echo -e "${YELLOW}   Voulez-vous continuer quand même ? (y/N)${NC}"
    read -r response
    if [[ ! "$response" =~ ^[Yy]$ ]]; then
        echo -e "${BLUE}ℹ${NC}  Commit annulé"
        exit 0
    fi
fi

# Afficher diff
echo -e "\n${YELLOW}📋 Aperçu des modifications:${NC}"
git diff --stat "$MODULE_FILE"

# Compter lignes ajoutées/supprimées
LINES_ADDED=$(git diff --numstat "$MODULE_FILE" | awk '{print $1}')
LINES_REMOVED=$(git diff --numstat "$MODULE_FILE" | awk '{print $2}')

if [ -n "$LINES_ADDED" ] && [ -n "$LINES_REMOVED" ]; then
    echo -e "${GREEN}   +${LINES_ADDED}${NC} lignes ajoutées"
    echo -e "${RED}   -${LINES_REMOVED}${NC} lignes supprimées"
fi

# Demander confirmation
echo -e "\n${YELLOW}❓ Confirmer le commit ? (Y/n)${NC}"
read -r confirm
if [[ "$confirm" =~ ^[Nn]$ ]]; then
    echo -e "${BLUE}ℹ${NC}  Commit annulé"
    exit 0
fi

# Créer message de commit
COMMIT_MSG="C${CYCLE_NUM}: Add tests for ${MODULE_NAME} (+${COVERAGE_GAIN}% coverage)

- Module: ${MODULE_NAME}.rs
- Tests added: Multiple unit tests
- Coverage gain: +${COVERAGE_GAIN}%
- Lines added: +${LINES_ADDED:-0}
- Lines removed: -${LINES_REMOVED:-0}

Cycle: C${CYCLE_NUM}
Date: $(date -Iseconds)
"

# Ajouter fichier au staging
echo -e "\n${YELLOW}📦 Ajout fichier au staging...${NC}"
git add "$MODULE_FILE"

# Commit
echo -e "${YELLOW}💾 Création commit...${NC}"
git commit -m "$COMMIT_MSG" || {
    echo -e "${RED}❌ Erreur lors du commit${NC}"
    exit 1
}

# Afficher commit créé
echo -e "\n${GREEN}✅ Commit créé avec succès !${NC}"
COMMIT_HASH=$(git rev-parse --short HEAD)
echo -e "   📝 Hash: ${COMMIT_HASH}"
echo -e "   📄 Fichier: ${MODULE_FILE}"
echo -e "   📊 Couverture: +${COVERAGE_GAIN}%"

# Afficher log du commit
echo -e "\n${YELLOW}📋 Détails du commit:${NC}"
git log -1 --stat

# Demander si push vers remote
echo -e "\n${YELLOW}❓ Pousser vers remote ? (Y/n)${NC}"
read -r push_confirm
if [[ ! "$push_confirm" =~ ^[Nn]$ ]]; then
    echo -e "${YELLOW}🚀 Push vers remote...${NC}"
    
    # Vérifier remote configuré
    if git remote | grep -q "origin"; then
        git push origin HEAD || {
            echo -e "${RED}❌ Erreur lors du push${NC}"
            echo -e "${YELLOW}   Le commit local a été créé mais pas poussé${NC}"
            exit 1
        }
        echo -e "${GREEN}✅ Push réussi !${NC}"
    else
        echo -e "${YELLOW}⚠${NC}  Aucun remote 'origin' configuré"
        echo -e "${YELLOW}   Le commit local a été créé mais pas poussé${NC}"
    fi
else
    echo -e "${BLUE}ℹ${NC}  Push annulé - commit local uniquement"
fi

# Afficher résumé
echo -e "\n${GREEN}✅ Protocole Git terminé${NC}"
echo -e "   Cycle: C${CYCLE_NUM}"
echo -e "   Module: ${MODULE_NAME}"
echo -e "   Commit: ${COMMIT_HASH}"
echo -e "   Couverture: +${COVERAGE_GAIN}%"

# Suggestion backup
echo -e "\n${YELLOW}💡 N'oubliez pas de créer un backup:${NC}"
echo -e "   ${MDBAI_DIR}/backup_cycle.sh ${CYCLE_NUM} ${MODULE_NAME}"

exit 0

# Made with Bob
