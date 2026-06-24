#!/bin/bash

# 🚀 SCRIPT DÉMARRAGE TEST UTILISATEUR MDBAI
# Version: 1.0.0
# Date: 2026-06-03
# Usage: ./start_test_utilisateur.sh

set -e  # Exit on error

echo "═══════════════════════════════════════════════════════════════"
echo "  🚀 DÉMARRAGE TEST UTILISATEUR MDBAI - CYCLE C193"
echo "═══════════════════════════════════════════════════════════════"
echo ""

# Couleurs
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Répertoire de travail
MDBAI_DIR="/home/lvx/LVX/lumvorax2/src/MDBAI"
cd "$MDBAI_DIR"

echo "📂 Répertoire: $MDBAI_DIR"
echo ""

# ═══════════════════════════════════════════════════════════════
# ÉTAPE 1: VÉRIFICATION INFRASTRUCTURE
# ═══════════════════════════════════════════════════════════════

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "  ÉTAPE 1/5: Vérification Infrastructure"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

# Vérifier node_modules
if [ ! -d "node_modules" ]; then
    echo -e "${YELLOW}⚠️  node_modules absent - Installation...${NC}"
    npm install
    echo -e "${GREEN}✅ node_modules installé${NC}"
else
    echo -e "${GREEN}✅ node_modules présent${NC}"
fi

# Vérifier libmdbai_hooks_forensic.so
if [ ! -f "forensic/libmdbai_hooks_forensic.so" ]; then
    echo -e "${YELLOW}⚠️  libmdbai_hooks_forensic.so absent - Compilation...${NC}"
    cd forensic
    make clean && make
    cd ..
    echo -e "${GREEN}✅ libmdbai_hooks_forensic.so compilé ($(du -h forensic/libmdbai_hooks_forensic.so | cut -f1))${NC}"
else
    SIZE=$(du -h forensic/libmdbai_hooks_forensic.so | cut -f1)
    echo -e "${GREEN}✅ libmdbai_hooks_forensic.so présent ($SIZE)${NC}"
fi

# Vérifier Doppler
if command -v doppler &> /dev/null; then
    echo -e "${GREEN}✅ Doppler installé ($(doppler --version))${NC}"
else
    echo -e "${RED}❌ Doppler non installé${NC}"
    echo "   Installation: curl -Ls https://cli.doppler.com/install.sh | sh"
    exit 1
fi

# Vérifier token Doppler
if [ -z "$DOPPLER_TOKEN" ]; then
    echo -e "${YELLOW}⚠️  DOPPLER_TOKEN non défini${NC}"
    echo "   Définir: export DOPPLER_TOKEN=dp.st.xxx"
    echo "   Ou utiliser: doppler login"
else
    echo -e "${GREEN}✅ DOPPLER_TOKEN défini${NC}"
fi

echo ""

# ═══════════════════════════════════════════════════════════════
# ÉTAPE 2: VÉRIFICATION SECRETS
# ═══════════════════════════════════════════════════════════════

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "  ÉTAPE 2/5: Vérification Secrets Doppler"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

# Liste des secrets requis
REQUIRED_SECRETS=(
    "MDBAI_APP_ID"
    "MDBAI_CLIENT_ID"
    "MDBAI_CLIENT_SECRET"
    "MDBAI_PRIVATE_KEY"
    "TELEGRAM_BOT_TOKEN"
    "REDIS_URL"
    "SESSION_SECRET"
)

MISSING_SECRETS=0

for SECRET in "${REQUIRED_SECRETS[@]}"; do
    if doppler secrets get "$SECRET" --plain &> /dev/null; then
        echo -e "${GREEN}✅ $SECRET${NC}"
    else
        echo -e "${RED}❌ $SECRET manquant${NC}"
        ((MISSING_SECRETS++))
    fi
done

if [ $MISSING_SECRETS -gt 0 ]; then
    echo ""
    echo -e "${RED}❌ $MISSING_SECRETS secret(s) manquant(s)${NC}"
    echo "   Configurer: doppler secrets set SECRET_NAME=value"
    exit 1
fi

echo ""
echo -e "${GREEN}✅ Tous les secrets sont configurés${NC}"
echo ""

# ═══════════════════════════════════════════════════════════════
# ÉTAPE 3: VÉRIFICATION PORTS
# ═══════════════════════════════════════════════════════════════

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "  ÉTAPE 3/5: Vérification Ports"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

PORT=3000

if lsof -Pi :$PORT -sTCP:LISTEN -t >/dev/null 2>&1; then
    echo -e "${YELLOW}⚠️  Port $PORT déjà utilisé${NC}"
    PID=$(lsof -Pi :$PORT -sTCP:LISTEN -t)
    echo "   PID: $PID"
    echo "   Arrêter: kill $PID"
    read -p "   Voulez-vous arrêter le processus? (y/n) " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        kill $PID
        sleep 2
        echo -e "${GREEN}✅ Processus arrêté${NC}"
    else
        echo -e "${RED}❌ Port occupé - Abandon${NC}"
        exit 1
    fi
else
    echo -e "${GREEN}✅ Port $PORT disponible${NC}"
fi

echo ""

# ═══════════════════════════════════════════════════════════════
# ÉTAPE 4: AFFICHAGE INFORMATIONS
# ═══════════════════════════════════════════════════════════════

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "  ÉTAPE 4/5: Informations Système MDBAI"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

APP_ID=$(doppler secrets get MDBAI_APP_ID --plain 2>/dev/null || echo "N/A")
CLIENT_ID=$(doppler secrets get MDBAI_CLIENT_ID --plain 2>/dev/null || echo "N/A")
BOT_TOKEN=$(doppler secrets get TELEGRAM_BOT_TOKEN --plain 2>/dev/null || echo "N/A")
BOT_USERNAME=$(echo "$BOT_TOKEN" | cut -d':' -f1 || echo "N/A")

echo "📱 GitHub App:"
echo "   - App ID: $APP_ID"
echo "   - Client ID: $CLIENT_ID"
echo "   - URL: https://github.com/apps/mdbai-master-debug-ai"
echo ""

echo "🤖 Telegram Bot:"
echo "   - Username: @masterdebugai_bot"
echo "   - Bot ID: $BOT_USERNAME"
echo "   - URL: https://t.me/masterdebugai_bot"
echo ""

echo "🌐 Serveur:"
echo "   - Port: $PORT"
echo "   - URL locale: http://localhost:$PORT"
echo "   - Dashboard: http://localhost:$PORT/dashboard"
echo ""

echo "📚 Documentation:"
echo "   - Guide test: $MDBAI_DIR/GUIDE_TEST_UTILISATEUR_REEL_C193.md"
echo "   - Guide Telegram: $MDBAI_DIR/GUIDE_UTILISATION_TELEGRAM.md"
echo ""

# ═══════════════════════════════════════════════════════════════
# ÉTAPE 5: DÉMARRAGE SERVEUR
# ═══════════════════════════════════════════════════════════════

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "  ÉTAPE 5/5: Démarrage Serveur MDBAI"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

echo -e "${BLUE}🚀 Démarrage du serveur...${NC}"
echo ""

# Créer répertoire logs si nécessaire
mkdir -p logs

# Démarrer avec Doppler
echo "Commande: doppler run -- npm start"
echo ""
echo "═══════════════════════════════════════════════════════════════"
echo ""

# Démarrer le serveur
doppler run -- npm start

# Note: Le script s'arrête ici car npm start bloque
# Pour arrêter: Ctrl+C

# Made with Bob
