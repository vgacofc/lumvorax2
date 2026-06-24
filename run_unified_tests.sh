#!/bin/bash
################################################################################
# Script d'Exécution Tests Unifiés ARC-AGI + Arcade
# ==================================================
#
# RÈGLE ABSOLUE: Ce script exécute TOUJOURS les deux types de tests:
# 1. Puzzles ARC-AGI
# 2. Jeux Arcade
#
# AUCUNE EXCEPTION - Les deux sont OBLIGATOIRES
#
# Usage:
#   ./run_unified_tests.sh [num_arc] [num_arcade]
#
# Exemples:
#   ./run_unified_tests.sh           # 10 puzzles + 10 jeux (défaut)
#   ./run_unified_tests.sh 20 20     # 20 puzzles + 20 jeux
#   ./run_unified_tests.sh 100 50    # 100 puzzles + 50 jeux
#
# Date: 2026-06-15
# Version: V38.2
################################################################################

set -e  # Arrêter en cas d'erreur

# Couleurs
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Paramètres
NUM_ARC=${1:-10}
NUM_ARCADE=${2:-10}

echo -e "${BLUE}╔════════════════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║  FRAMEWORK DE TESTS UNIFIÉ ARC-AGI + ARCADE                   ║${NC}"
echo -e "${BLUE}╚════════════════════════════════════════════════════════════════╝${NC}"
echo ""
echo -e "${YELLOW}⚠️  RÈGLE ABSOLUE: LES DEUX TYPES DE TESTS SONT OBLIGATOIRES${NC}"
echo -e "   ${GREEN}✓${NC} Puzzles ARC-AGI: ${NUM_ARC}"
echo -e "   ${GREEN}✓${NC} Jeux Arcade: ${NUM_ARCADE}"
echo ""

# Vérifier que Python3 est disponible
if ! command -v python3 &> /dev/null; then
    echo -e "${RED}❌ ERREUR: python3 n'est pas installé${NC}"
    exit 1
fi

# Vérifier que le fichier de test existe
TEST_FILE="src/MAGEN/tests/test_unified_arc_arcade.py"
if [ ! -f "$TEST_FILE" ]; then
    echo -e "${RED}❌ ERREUR: Fichier de test introuvable: $TEST_FILE${NC}"
    exit 1
fi

# Créer répertoire logs
LOG_DIR="/tmp/magen_unified"
mkdir -p "$LOG_DIR"

echo -e "${BLUE}📁 Logs forensic: $LOG_DIR${NC}"
echo ""

# Exécuter les tests unifiés
echo -e "${GREEN}🚀 Lancement des tests unifiés...${NC}"
echo ""

# Définir PYTHONPATH pour inclure src/
export PYTHONPATH="$(pwd)/src:$PYTHONPATH"

# Modifier temporairement le script Python pour utiliser les paramètres
TEMP_TEST_FILE="/tmp/test_unified_temp.py"
cp "$TEST_FILE" "$TEMP_TEST_FILE"

# Remplacer les valeurs par défaut dans le main()
sed -i "s/num_arc_puzzles=10/num_arc_puzzles=$NUM_ARC/g" "$TEMP_TEST_FILE"
sed -i "s/num_arcade_games=10/num_arcade_games=$NUM_ARCADE/g" "$TEMP_TEST_FILE"

# Exécuter avec PYTHONPATH
if PYTHONPATH="$(pwd)/src:$PYTHONPATH" python3 "$TEMP_TEST_FILE"; then
    echo ""
    echo -e "${GREEN}╔════════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${GREEN}║  ✅ TESTS UNIFIÉS RÉUSSIS - LES DEUX TYPES EXÉCUTÉS           ║${NC}"
    echo -e "${GREEN}╚════════════════════════════════════════════════════════════════╝${NC}"
    
    # Afficher emplacement logs
    echo ""
    echo -e "${BLUE}📊 Logs forensic disponibles:${NC}"
    ls -lh "$LOG_DIR"/*.jsonl 2>/dev/null || echo "   (Aucun log généré)"
    
    exit 0
else
    echo ""
    echo -e "${RED}╔════════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${RED}║  ❌ ÉCHEC DES TESTS UNIFIÉS                                    ║${NC}"
    echo -e "${RED}╚════════════════════════════════════════════════════════════════╝${NC}"
    exit 1
fi

# Nettoyage
rm -f "$TEMP_TEST_FILE"

# Made with Bob
