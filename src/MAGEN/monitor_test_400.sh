#!/bin/bash
# Script de monitoring test 400 puzzles

LOG_FILE="test_v23_400_puzzles.log"

echo "=== MONITORING TEST 400 PUZZLES V23 ==="
echo ""

while ps aux | grep -q "[p]ython3 test_v23_corrected_pipeline.py"; do
    clear
    echo "=== MONITORING TEST 400 PUZZLES V23 ==="
    echo "Temps: $(date '+%H:%M:%S')"
    echo ""
    
    # Progression
    CURRENT=$(grep -c "^\[.*\] Puzzle:" "$LOG_FILE" 2>/dev/null || echo "0")
    echo "Progression: $CURRENT/400 puzzles"
    
    # Succès actuels
    SUCCESS=$(grep -c "✅ SUCCÈS" "$LOG_FILE" 2>/dev/null || echo "0")
    echo "Succès: $SUCCESS"
    
    # Dernières lignes
    echo ""
    echo "--- Dernières lignes ---"
    tail -10 "$LOG_FILE"
    
    sleep 10
done

echo ""
echo "=== TEST TERMINÉ ==="
echo ""
tail -50 "$LOG_FILE"

# Made with Bob
