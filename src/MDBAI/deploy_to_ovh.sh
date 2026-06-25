#!/bin/bash
# Script de déploiement automatique MDBAI sur OVH
# Serveur: 51.255.22.253 (mdbai-prod-1782286076)

set -e

echo "════════════════════════════════════════════════════════"
echo "  DÉPLOIEMENT MDBAI SUR OVH"
echo "  Serveur: 51.255.22.253"
echo "  Instance: mdbai-prod-1782286076"
echo "════════════════════════════════════════════════════════"

SERVER_IP="51.255.22.253"
SERVER_USER="root"
DEPLOY_DIR="/opt/mdbai"
REPO_URL="https://github.com/vgacofc/lumvorax2.git"
BRANCH="bob"

echo ""
echo "📡 Connexion au serveur OVH..."
ssh -o StrictHostKeyChecking=no ${SERVER_USER}@${SERVER_IP} << 'ENDSSH'
set -e

echo ""
echo "📦 Installation des dépendances système..."
apt-get update -qq
apt-get install -y git docker.io docker-compose curl

echo ""
echo "🔧 Démarrage Docker..."
systemctl start docker
systemctl enable docker

echo ""
echo "📂 Préparation répertoire de déploiement..."
mkdir -p /opt/mdbai
cd /opt/mdbai

echo ""
echo "📥 Clone/Pull du repository..."
if [ -d ".git" ]; then
    echo "Repository existe, pull des dernières modifications..."
    git fetch origin
    git checkout bob
    git pull origin bob
else
    echo "Clone du repository..."
    git clone -b bob https://github.com/vgacofc/lumvorax2.git .
fi

echo ""
echo "📂 Navigation vers MDBAI..."
cd src/MDBAI

echo ""
echo "🔐 Configuration Doppler..."
# Installer Doppler CLI si nécessaire
if ! command -v doppler &> /dev/null; then
    echo "Installation Doppler CLI..."
    curl -Ls --tlsv1.2 --proto "=https" --retry 3 https://cli.doppler.com/install.sh | sh
fi

# Vérifier si Doppler est configuré
if ! doppler setup --no-interactive 2>/dev/null; then
    echo "⚠️ Doppler non configuré. Configuration manuelle requise."
    echo "Exécutez: doppler login && doppler setup"
fi

echo ""
echo "🐳 Arrêt des conteneurs existants..."
docker-compose down || true

echo ""
echo "🏗️ Build des images Docker..."
docker-compose build

echo ""
echo "🚀 Démarrage des services..."
doppler run -- docker-compose up -d

echo ""
echo "⏳ Attente démarrage services (30s)..."
sleep 30

echo ""
echo "🔍 Vérification des services..."
docker-compose ps

echo ""
echo "📊 Logs des services..."
docker-compose logs --tail=50

echo ""
echo "✅ Déploiement terminé!"
echo ""
echo "Services disponibles:"
echo "  - API: http://51.255.22.253:3001"
echo "  - Health: http://51.255.22.253:3001/health"
echo "  - Dashboard: http://51.255.22.253:3001/dashboard"
echo ""
echo "Commandes utiles:"
echo "  - Logs: docker-compose logs -f"
echo "  - Restart: docker-compose restart"
echo "  - Stop: docker-compose down"
echo ""

ENDSSH

echo ""
echo "════════════════════════════════════════════════════════"
echo "  ✅ DÉPLOIEMENT TERMINÉ"
echo "════════════════════════════════════════════════════════"
echo ""
echo "🌐 Testez l'API:"
echo "   curl http://51.255.22.253:3001/health"
echo ""

# Made with Bob
