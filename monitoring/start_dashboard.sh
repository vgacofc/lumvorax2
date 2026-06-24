#!/bin/bash

# Script de démarrage complet du dashboard LumVorax
# Lance le backend WebSocket ET le frontend React

set -e

echo "🚀 DÉMARRAGE DASHBOARD LUMVORAX"
echo "================================"

# Couleurs
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Vérifier que nous sommes dans le bon répertoire
if [ ! -f "package.json" ]; then
    echo "❌ Erreur: package.json introuvable"
    echo "Exécutez ce script depuis lumvorax2/monitoring/"
    exit 1
fi

# Fonction pour tuer les processus au Ctrl+C
cleanup() {
    echo -e "\n${YELLOW}🛑 Arrêt des serveurs...${NC}"
    kill $BACKEND_PID $FRONTEND_PID 2>/dev/null
    exit 0
}

trap cleanup SIGINT SIGTERM

# 1. Démarrer le backend WebSocket
echo -e "${BLUE}📡 Démarrage backend WebSocket (port 3001)...${NC}"
cd backend
node server.js &
BACKEND_PID=$!
cd ..

# Attendre que le backend soit prêt
sleep 2

# 2. Démarrer le frontend React/Vite
echo -e "${BLUE}🎨 Démarrage frontend React (port 5173)...${NC}"
npm run dev &
FRONTEND_PID=$!

# Attendre que le frontend soit prêt
sleep 3

echo -e "${GREEN}✅ Dashboard démarré avec succès !${NC}"
echo ""
echo "📊 Accès dashboard:"
echo "   Frontend: http://localhost:5173"
echo "   Backend:  http://localhost:3001"
echo ""
echo "🔧 Processus:"
echo "   Backend PID:  $BACKEND_PID"
echo "   Frontend PID: $FRONTEND_PID"
echo ""
echo "⚠️  Appuyez sur Ctrl+C pour arrêter les deux serveurs"
echo ""

# Garder le script actif
wait

# Made with Bob
