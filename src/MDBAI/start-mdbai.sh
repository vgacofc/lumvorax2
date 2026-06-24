#!/bin/bash
# Script de démarrage automatique MDBAI avec gestion ngrok
# Auteur: Bob AI Assistant
# Date: 2026-06-06

set -e

echo "🚀 Démarrage MDBAI avec configuration automatique..."

# Couleurs pour les logs
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Répertoire du projet
MDBAI_DIR="/home/lvx/LVX/lumvorax2/src/MDBAI"
cd "$MDBAI_DIR"

# 1. Vérifier si ngrok est en cours d'exécution
echo -e "${YELLOW}[1/5]${NC} Vérification de ngrok..."
NGROK_PID=$(pgrep -f "ngrok http 3001" || true)

if [ -n "$NGROK_PID" ]; then
    echo -e "${GREEN}✓${NC} Ngrok déjà en cours (PID: $NGROK_PID)"
else
    echo -e "${YELLOW}⚠${NC} Ngrok non détecté, démarrage..."
    nohup ngrok http 3001 > /tmp/ngrok.log 2>&1 &
    sleep 3
    echo -e "${GREEN}✓${NC} Ngrok démarré"
fi

# 2. Récupérer l'URL ngrok actuelle
echo -e "${YELLOW}[2/5]${NC} Récupération de l'URL ngrok..."
NGROK_URL=""

# Attendre que l'API ngrok soit prête
sleep 3

for i in {1..10}; do
    # Essayer avec jq d'abord (plus fiable)
    if command -v jq &> /dev/null; then
        NGROK_URL=$(curl -s http://localhost:4040/api/tunnels 2>/dev/null | jq -r '.tunnels[0].public_url' 2>/dev/null || true)
    fi
    
    # Fallback: grep si jq n'est pas disponible ou a échoué
    if [ -z "$NGROK_URL" ] || [ "$NGROK_URL" = "null" ]; then
        NGROK_URL=$(curl -s http://localhost:4040/api/tunnels 2>/dev/null | grep -oP '"public_url":\s*"\K[^"]+' | head -1 || true)
    fi
    
    if [ -n "$NGROK_URL" ] && [ "$NGROK_URL" != "null" ]; then
        echo -e "${GREEN}✓${NC} URL ngrok: $NGROK_URL"
        break
    fi
    echo "  Tentative $i/10..."
    sleep 1
done

if [ -z "$NGROK_URL" ] || [ "$NGROK_URL" = "null" ]; then
    echo -e "${RED}✗${NC} Impossible de récupérer l'URL ngrok"
    echo -e "${YELLOW}ℹ${NC} Le serveur démarrera en mode LOCAL uniquement (http://localhost:3001)"
    NGROK_URL="http://localhost:3001"
fi

# 3. Mettre à jour le fichier .env automatiquement
echo -e "${YELLOW}[3/5]${NC} Mise à jour du fichier .env..."
CALLBACK_URL="${NGROK_URL}/auth/github/callback"

# Sauvegarder l'ancien .env
cp .env .env.backup

# Mettre à jour ou ajouter GITHUB_CALLBACK_URL
if grep -q "^GITHUB_CALLBACK_URL=" .env; then
    # Remplacer la ligne existante
    sed -i "s|^GITHUB_CALLBACK_URL=.*|GITHUB_CALLBACK_URL=${CALLBACK_URL}|" .env
    echo -e "${GREEN}✓${NC} GITHUB_CALLBACK_URL mis à jour: $CALLBACK_URL"
elif grep -q "^# GITHUB_CALLBACK_URL=" .env; then
    # Décommenter et mettre à jour
    sed -i "s|^# GITHUB_CALLBACK_URL=.*|GITHUB_CALLBACK_URL=${CALLBACK_URL}|" .env
    echo -e "${GREEN}✓${NC} GITHUB_CALLBACK_URL activé: $CALLBACK_URL"
else
    # Ajouter la ligne après GITHUB_APP_ID
    sed -i "/^GITHUB_APP_ID=/a GITHUB_CALLBACK_URL=${CALLBACK_URL}" .env
    echo -e "${GREEN}✓${NC} GITHUB_CALLBACK_URL ajouté: $CALLBACK_URL"
fi

# 4. Charger NVM et Node.js
echo -e "${YELLOW}[4/5]${NC} Chargement de Node.js v22.22.3..."
export NVM_DIR="$HOME/.nvm"
[ -s "$NVM_DIR/nvm.sh" ] && \. "$NVM_DIR/nvm.sh"
nvm use v22.22.3 > /dev/null 2>&1
echo -e "${GREEN}✓${NC} Node.js $(node --version) activé"

# 5. Démarrer le serveur MDBAI
echo -e "${YELLOW}[5/5]${NC} Démarrage du serveur MDBAI..."
echo ""
echo -e "${GREEN}═══════════════════════════════════════════════════════${NC}"
echo -e "${GREEN}  MDBAI v0.1.0 - Master Debug AI${NC}"
echo -e "${GREEN}═══════════════════════════════════════════════════════${NC}"
echo -e "  🌐 URL locale:    http://localhost:3001"
echo -e "  🌍 URL publique:  $NGROK_URL"
echo -e "  🔗 OAuth GitHub:  $CALLBACK_URL"
echo -e "${GREEN}═══════════════════════════════════════════════════════${NC}"
echo ""

# Démarrer npm avec Doppler (source de vérité pour secrets)
exec doppler run -- npm start

# Made with Bob
