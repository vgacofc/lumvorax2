#!/bin/bash

# Script de correction et lancement automatique du dashboard
# Corrige tous les problèmes identifiés et lance le système complet

set -e

echo "🔧 CORRECTION ET LANCEMENT DASHBOARD LUMVORAX"
echo "=============================================="

# 1. Tuer les processus existants
echo "📌 Arrêt des processus existants..."
pkill -f "node.*monitoring" || true
pkill -f "bench_c199" || true
sleep 2

# 2. Nettoyer les anciens logs
echo "🧹 Nettoyage des anciens logs..."
rm -f logs/forensic/realtime_metrics.csv

# 3. Démarrer le backend
echo "🚀 Démarrage backend (port 3001)..."
cd monitoring/backend
node server.js > /tmp/backend.log 2>&1 &
BACKEND_PID=$!
cd ../..
sleep 3

# Vérifier que le backend est démarré
if ! ps -p $BACKEND_PID > /dev/null; then
    echo "❌ ERREUR: Backend n'a pas démarré"
    cat /tmp/backend.log
    exit 1
fi
echo "✅ Backend démarré (PID: $BACKEND_PID)"

# 4. Démarrer le frontend
echo "🎨 Démarrage frontend (port 5177)..."
cd monitoring
npm run dev > /tmp/frontend.log 2>&1 &
FRONTEND_PID=$!
cd ..
sleep 5

# Vérifier que le frontend est démarré
if ! ps -p $FRONTEND_PID > /dev/null; then
    echo "❌ ERREUR: Frontend n'a pas démarré"
    cat /tmp/frontend.log
    exit 1
fi
echo "✅ Frontend démarré (PID: $FRONTEND_PID)"

# 5. Attendre que le frontend soit prêt
echo "⏳ Attente du frontend (10s)..."
sleep 10

# 6. Ouvrir le navigateur automatiquement
echo "🌐 Ouverture du dashboard dans le navigateur..."
if command -v xdg-open > /dev/null; then
    xdg-open http://localhost:5177 &
elif command -v firefox > /dev/null; then
    firefox http://localhost:5177 &
elif command -v google-chrome > /dev/null; then
    google-chrome http://localhost:5177 &
else
    echo "⚠️  Ouvrez manuellement: http://localhost:5177"
fi

# 7. Lancer le benchmark
echo "⚡ Lancement du benchmark (60s)..."
sleep 2
./tests/bench_c199_poh_stress_60s_v3_realtime

echo ""
echo "✅ SYSTÈME COMPLET LANCÉ"
echo "========================"
echo "Backend:   http://localhost:3001 (PID: $BACKEND_PID)"
echo "Frontend:  http://localhost:5177 (PID: $FRONTEND_PID)"
echo "Benchmark: En cours d'exécution (60s)"
echo ""
echo "📊 Le dashboard affiche maintenant:"
echo "   - POH TICKS/S (valeur principale en grand)"
echo "   - THROUGHPUT (verif/s, séparé)"
echo "   - LATENCE (min/med/max dynamiques)"
echo "   - CPU (avec cores/threads)"
echo "   - RAM (dynamique)"
echo "   - GPU (dynamique)"
echo ""
echo "Appuyez sur Ctrl+C pour arrêter"

# Attendre la fin du benchmark
wait

# Made with Bob
