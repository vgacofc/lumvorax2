#!/bin/bash
set -e

echo "🚀 Configuration Doppler MDBAI"
echo "================================"

# Vérifier Doppler installé
if ! command -v doppler &> /dev/null; then
    echo "❌ Doppler CLI non installé"
    echo "Installation..."
    curl -Ls https://cli.doppler.com/install.sh | sh
fi

# Vérifier authentification
if ! doppler configure get token &> /dev/null; then
    echo "❌ Non authentifié - Login requis"
    doppler login
fi

# Configuration projet
echo "📦 Configuration projet..."
doppler setup --project lumvorax --config dev_debugai --no-interactive || true

# Upload secrets
echo "📤 Upload secrets..."

doppler secrets set TELEGRAM_BOT_TOKEN="8820756284:AAEaeBUd2PIRbPq-_V6gAD0v2sb-lAl-rr8" --silent
doppler secrets set REDIS_URL="redis://default:lAOtAuizLK2LyIVlrSdEpPF8CwEaWZip@redis-17068.c327.europe-west1-2.gce.cloud.redislabs.com:17068" --silent
doppler secrets set REDIS_HOST="redis-17068.c327.europe-west1-2.gce.cloud.redislabs.com" --silent
doppler secrets set REDIS_PORT="17068" --silent
doppler secrets set REDIS_PASSWORD="lAOtAuizLK2LyIVlrSdEpPF8CwEaWZip" --silent
doppler secrets set DATABASE_URL="postgres://b61c282aa55d28966cbd3cdcb45ca86681712c063eab8556cd544f212eed859a:sk_3TBG35XoftnxSiHZsdzN4@pooled.db.prisma.io:5432/postgres?sslmode=require" --silent
doppler secrets set TASKFORCE_API_TOKEN="d7102033-d4bf-4b91-95df-2e24ab08d2da" --silent
doppler secrets set UPSTASH_BOX_API_KEY="box_fbc484766ea9bfdb8bc6ff3c4ca74bee4ad9b9605c90fa07ab11526aaea8a422" --silent
doppler secrets set UPCLOUD_API_TOKEN="ucat_01KSN6YG7A5CBB8ZA9YAS2K5JB" --silent
doppler secrets set OVH_APPLICATION_KEY="180e017e7ea658b4" --silent
doppler secrets set OVH_APPLICATION_SECRET="09161e9fcb54bbc7a98fab5d6eaa8128" --silent
doppler secrets set OVH_CONSUMER_KEY="f36ebdf91d2be8fbd43f151f41870615" --silent
doppler secrets set NODE_ENV="development" --silent
doppler secrets set LOG_LEVEL="debug" --silent
doppler secrets set API_PORT="3000" --silent

echo "✅ Upload terminé!"
echo ""
echo "📊 Secrets configurés:"
doppler secrets | grep -E "TELEGRAM|REDIS|DATABASE|TASKFORCE|UPSTASH|UPCLOUD|OVH|NODE_ENV" || echo "Erreur listage secrets"

echo ""
echo "⚠️  ATTENTION: Tous ces tokens sont COMPROMIS"
echo "🔄 Régénérer immédiatement sur chaque plateforme"
echo ""
echo "✅ Configuration Doppler terminée"
