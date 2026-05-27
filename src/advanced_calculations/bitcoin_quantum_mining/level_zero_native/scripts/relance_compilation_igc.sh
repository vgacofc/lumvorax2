#!/bin/bash
# Script de relance compilation IGC après arrêt BobIDE
# Usage: ./relance_compilation_igc.sh

set -e

echo "=== RELANCE COMPILATION IGC ==="
echo "Date: $(date)"
echo ""

# Vérifier si compilation déjà en cours
if ps aux | grep -v grep | grep "make.*igc" > /dev/null; then
    echo "⚠️  Compilation déjà en cours !"
    ps aux | grep -v grep | grep make | head -5
    exit 1
fi

# Aller dans build directory
cd ~/igc_full/build || {
    echo "❌ Erreur: ~/igc_full/build introuvable"
    exit 1
}

echo "📊 État actuel:"
echo "  - Fichiers .o: $(find . -name '*.o' 2>/dev/null | wc -l)"
echo "  - Taille build: $(du -sh . 2>/dev/null | cut -f1)"
echo ""

# Relancer compilation avec -k (continue malgré erreurs)
echo "🚀 Relance compilation (mode -k, ignore BiFManager)..."
echo "   Log: /tmp/igc_compile_$(date +%Y%m%d_%H%M%S).log"
echo ""

nohup make -j8 -k > /tmp/igc_compile_$(date +%Y%m%d_%H%M%S).log 2>&1 &
PID=$!

echo "✅ Compilation lancée (PID: $PID)"
echo ""
echo "📝 Commandes surveillance:"
echo "   # Voir progression"
echo "   watch -n 30 'cd ~/igc_full/build && find . -name \"*.o\" | wc -l'"
echo ""
echo "   # Voir logs en temps réel"
echo "   tail -f /tmp/igc_compile_*.log | grep -E '\[.*%\]'"
echo ""
echo "   # Vérifier processus"
echo "   ps aux | grep make | grep -v grep"
echo ""
echo "   # Arrêter si besoin"
echo "   pkill -9 make"

# Attendre 5 secondes et vérifier
sleep 5

if ps -p $PID > /dev/null; then
    echo "✅ Compilation en cours (PID: $PID actif)"
    tail -20 /tmp/igc_compile_*.log 2>/dev/null | tail -10
else
    echo "❌ Compilation arrêtée immédiatement, vérifier logs"
    tail -50 /tmp/igc_compile_*.log 2>/dev/null
    exit 1
fi

# Made with Bob
