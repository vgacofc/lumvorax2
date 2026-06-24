#!/bin/bash
# Script de déploiement MDBAI sur OVH avec Doppler
# Usage: ./deploy_production.sh

set -e

echo "═══════════════════════════════════════════════════════"
echo "  MDBAI - Déploiement Production OVH"
echo "  Date: $(date '+%Y-%m-%d %H:%M:%S')"
echo "═══════════════════════════════════════════════════════"

# Vérifier que Doppler est configuré
if ! doppler secrets > /dev/null 2>&1; then
    echo "❌ Erreur: Doppler n'est pas configuré"
    echo "Exécutez: doppler setup --project mdbai --config prd"
    exit 1
fi

echo "✅ Doppler configuré"

# Arrêter les conteneurs existants
echo ""
echo "🛑 Arrêt des conteneurs existants..."
docker-compose -f docker-compose.production.yml down || true

# Nettoyer les images obsolètes
echo ""
echo "🧹 Nettoyage des images obsolètes..."
docker system prune -f

# Exporter les secrets Doppler vers .env
echo ""
echo "🔐 Export des secrets Doppler..."
doppler secrets download --no-file --format env > .env

# Vérifier que les secrets critiques sont présents
echo ""
echo "🔍 Vérification des secrets critiques..."
REQUIRED_SECRETS=(
    "TELEGRAM_BOT_TOKEN"
    "REDIS_URL"
    "SESSION_SECRET"
)

MISSING_SECRETS=()
for secret in "${REQUIRED_SECRETS[@]}"; do
    if ! grep -q "^${secret}=" .env; then
        MISSING_SECRETS+=("$secret")
    fi
done

if [ ${#MISSING_SECRETS[@]} -gt 0 ]; then
    echo "⚠️  Secrets manquants détectés:"
    for secret in "${MISSING_SECRETS[@]}"; do
        echo "   - $secret"
    done
    echo ""
    echo "L'application démarrera en mode dégradé"
    read -p "Continuer quand même? (y/N) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 1
    fi
else
    echo "✅ Tous les secrets critiques sont présents"
fi

# Build et démarrage des conteneurs
echo ""
echo "🏗️  Build des images Docker..."
docker-compose -f docker-compose.production.yml build --no-cache

echo ""
echo "🚀 Démarrage des conteneurs..."
docker-compose -f docker-compose.production.yml up -d

# Attendre que les services soient prêts
echo ""
echo "⏳ Attente du démarrage des services..."
sleep 10

# Vérifier le statut des conteneurs
echo ""
echo "📊 Statut des conteneurs:"
docker-compose -f docker-compose.production.yml ps

# Tester le health check
echo ""
echo "🏥 Test du health check..."
for i in {1..30}; do
    if curl -sf http://localhost/health > /dev/null 2>&1; then
        echo "✅ Health check OK"
        break
    fi
    if [ $i -eq 30 ]; then
        echo "❌ Health check échoué après 30 tentatives"
        echo ""
        echo "📋 Logs des conteneurs:"
        docker-compose -f docker-compose.production.yml logs --tail=50
        exit 1
    fi
    echo "   Tentative $i/30..."
    sleep 2
done

# Afficher les logs récents
echo ""
echo "📋 Logs récents (dernières 20 lignes):"
docker-compose -f docker-compose.production.yml logs --tail=20

# Afficher l'URL d'accès
echo ""
echo "═══════════════════════════════════════════════════════"
echo "  ✅ Déploiement réussi !"
echo "═══════════════════════════════════════════════════════"
echo ""
echo "🌐 URL d'accès:"
echo "   - Health check: http://51.255.22.253/health"
echo "   - Dashboard: http://51.255.22.253/dashboard"
echo "   - API: http://51.255.22.253/api"
echo ""
echo "📊 Commandes utiles:"
echo "   - Logs en temps réel: docker-compose -f docker-compose.production.yml logs -f"
echo "   - Statut: docker-compose -f docker-compose.production.yml ps"
echo "   - Redémarrer: docker-compose -f docker-compose.production.yml restart"
echo "   - Arrêter: docker-compose -f docker-compose.production.yml down"
echo ""

# Made with Bob
