#!/bin/bash
# Script de redémarrage du serveur MDBAI
# Usage: ./restart_server.sh

echo "🔄 Redémarrage du serveur MDBAI..."
echo ""

# Trouver et tuer le processus npm start
echo "🛑 Arrêt du serveur actuel..."
pkill -f "npm start" 2>/dev/null || true
pkill -f "node.*server.js" 2>/dev/null || true
sleep 2

# Vérifier Redis
echo "✅ Vérification de Redis..."
if ! redis-cli ping &> /dev/null; then
    echo "⚠️  Redis ne répond pas. Redémarrage..."
    echo "emmaus" | sudo -S systemctl restart redis-server
    sleep 2
fi

if redis-cli ping &> /dev/null; then
    echo "✅ Redis opérationnel (PONG)"
else
    echo "❌ Redis ne fonctionne pas. Vérifiez avec: sudo systemctl status redis-server"
    exit 1
fi

# Vérifier le fichier .env
if [ ! -f ".env" ]; then
    echo "❌ Fichier .env manquant. Créez-le d'abord."
    exit 1
fi

echo "✅ Fichier .env trouvé"
echo ""

# Démarrer le serveur
echo "🚀 Démarrage du serveur..."
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

npm start

# Made with Bob
