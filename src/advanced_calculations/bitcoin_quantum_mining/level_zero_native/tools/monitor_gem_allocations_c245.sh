#!/bin/bash
# Script de monitoring GEM allocations GPU - Cycle C245
# Objectif: Identifier fuite mémoire GPU causant crash après 20-23 dispatches
# Auteur: LumVorax Level Zero Native Team
# Date: 2026-05-13

set -e

# Couleurs pour output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Répertoires
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../../../.." && pwd)"
LOG_DIR="$PROJECT_ROOT/logs/forensic"
DEBUGFS_GPU="/sys/kernel/debug/dri/0000:00:02.0"

# Fichiers de sortie
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
GEM_LOG="$LOG_DIR/c245_gem_allocations_${TIMESTAMP}.csv"
CLIENTS_LOG="$LOG_DIR/c245_drm_clients_${TIMESTAMP}.log"
FREQUENCY_LOG="$LOG_DIR/c245_gpu_frequency_${TIMESTAMP}.log"
ENGINES_LOG="$LOG_DIR/c245_gpu_engines_${TIMESTAMP}.log"

# Créer répertoire logs si nécessaire
mkdir -p "$LOG_DIR"

echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}  MONITORING GEM ALLOCATIONS GPU - CYCLE C245${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo ""
echo -e "${GREEN}[INFO]${NC} Logs seront sauvegardés dans: $LOG_DIR"
echo -e "${GREEN}[INFO]${NC} Timestamp: $TIMESTAMP"
echo ""

# Vérifier accès root
if [ "$EUID" -ne 0 ]; then 
    echo -e "${RED}[ERREUR]${NC} Ce script nécessite les droits root (sudo)"
    exit 1
fi

# Vérifier debugfs monté
if [ ! -d "$DEBUGFS_GPU" ]; then
    echo -e "${RED}[ERREUR]${NC} debugfs GPU non accessible: $DEBUGFS_GPU"
    exit 1
fi

echo -e "${GREEN}[OK]${NC} debugfs GPU accessible"
echo ""

# Fonction pour capturer état GEM
capture_gem_state() {
    local dispatch_num=$1
    local phase=$2  # "before" ou "after"
    
    echo -e "${YELLOW}[CAPTURE]${NC} Dispatch #$dispatch_num - Phase: $phase"
    
    # Timestamp nanoseconde
    local ts=$(date +%s%N)
    
    # Capturer gem_names (objets GEM actifs)
    if [ -f "$DEBUGFS_GPU/gem_names" ]; then
        local gem_count=$(cat "$DEBUGFS_GPU/gem_names" 2>/dev/null | wc -l)
        echo "$ts,$dispatch_num,$phase,$gem_count" >> "$GEM_LOG"
        echo -e "  ${BLUE}→${NC} Objets GEM actifs: $gem_count"
    fi
    
    # Capturer clients DRM
    if [ -f "$DEBUGFS_GPU/clients" ]; then
        echo "=== Dispatch #$dispatch_num - $phase - $ts ===" >> "$CLIENTS_LOG"
        cat "$DEBUGFS_GPU/clients" >> "$CLIENTS_LOG" 2>/dev/null
        echo "" >> "$CLIENTS_LOG"
    fi
    
    # Capturer fréquence GPU
    if [ -f "$DEBUGFS_GPU/gt0/frequency" ]; then
        echo "=== Dispatch #$dispatch_num - $phase - $ts ===" >> "$FREQUENCY_LOG"
        cat "$DEBUGFS_GPU/gt0/frequency" >> "$FREQUENCY_LOG" 2>/dev/null
        echo "" >> "$FREQUENCY_LOG"
    fi
    
    # Capturer état engines
    if [ -f "$DEBUGFS_GPU/gt0/engines" ]; then
        echo "=== Dispatch #$dispatch_num - $phase - $ts ===" >> "$ENGINES_LOG"
        cat "$DEBUGFS_GPU/gt0/engines" >> "$ENGINES_LOG" 2>/dev/null
        echo "" >> "$ENGINES_LOG"
    fi
}

# Initialiser fichier CSV
echo "timestamp_ns,dispatch_num,phase,gem_objects_count" > "$GEM_LOG"

echo -e "${GREEN}[INFO]${NC} Initialisation monitoring..."
echo ""

# Capturer état initial (avant tout dispatch)
capture_gem_state 0 "initial"

echo ""
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}  MONITORING ACTIF - Prêt pour test${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo ""
echo -e "${YELLOW}[INSTRUCTIONS]${NC}"
echo "1. Lancer le test dans un autre terminal:"
echo "   cd $PROJECT_ROOT/src/advanced_calculations/bitcoin_quantum_mining/level_zero_native"
echo "   make test_c241_validation_forensique"
echo ""
echo "2. Ce script capturera automatiquement l'état GEM toutes les 2 secondes"
echo ""
echo "3. Appuyer sur Ctrl+C pour arrêter le monitoring"
echo ""

# Monitoring continu toutes les 2 secondes
dispatch_counter=1
while true; do
    sleep 2
    capture_gem_state $dispatch_counter "monitoring"
    dispatch_counter=$((dispatch_counter + 1))
done

# Made with Bob
