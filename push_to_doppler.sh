#!/bin/bash

# Configuration Doppler
export DOPPLER_TOKEN="dp.st.dev_mdbai.rh149574-ovh"

echo "=== PUSH CONFIGURATION VERS DOPPLER ==="
echo "Projet: mdbai (Master Debug AI)"
echo "Config: dev"
echo ""

# Fonction pour ajouter un secret
add_secret() {
    local key=$1
    local value=$2
    echo "📝 Ajout: $key"
    doppler secrets set "$key" "$value" --project mdbai --config dev 2>&1 | grep -v "Warning"
}

# OVH Configuration
echo "1. Configuration OVH..."
add_secret "OVH_ENDPOINT" "ovh-eu"
add_secret "OVH_APPLICATION_KEY" "5c081d6f5c440369"
add_secret "OVH_APPLICATION_SECRET" "28e36eb1eac62581b2134835e9ce745a"
add_secret "OVH_CONSUMER_KEY" "da67998836c9a8510bb7099bfa6f58e1"

# Telegram Configuration
echo ""
echo "2. Configuration Telegram..."
add_secret "TELEGRAM_BOT_TOKEN" "8820756284:AAEaeBUd2PIRbPq-_V6gAD0v2sb-lAl-rr8"
add_secret "TELEGRAM_BOT_USERNAME" "masterdebugai_bot"

# Email Configuration
echo ""
echo "3. Configuration Email..."
add_secret "EMAIL_NOTIFICATION" "vgacofficiel@gmail.com"
add_secret "EMAIL_FROM" "noreply@mdbai.ovh"

# Server Configuration
echo ""
echo "4. Configuration Serveur..."
add_secret "SERVER_IP" "51.255.22.253"
add_secret "SERVER_NAME" "mdbai-prod-1782286076"
add_secret "NODE_ENV" "production"
add_secret "PORT" "3000"

# Redis Configuration
echo ""
echo "5. Configuration Redis..."
add_secret "REDIS_HOST" "mdbai-redis"
add_secret "REDIS_PORT" "6379"

# JWT Configuration
echo ""
echo "6. Configuration JWT..."
add_secret "JWT_SECRET" "mdbai-secret-key-production-2026"
add_secret "JWT_EXPIRES_IN" "7d"

# Database Configuration (si nécessaire)
echo ""
echo "7. Configuration Database..."
add_secret "DB_TYPE" "redis"
add_secret "DB_HOST" "mdbai-redis"

echo ""
echo "=== VÉRIFICATION ==="
doppler secrets --project mdbai --config dev 2>&1 | head -20

echo ""
echo "✅ Configuration poussée vers Doppler"
echo "📦 Projet: mdbai"
echo "🔧 Config: dev"
echo "🔐 Secrets: 15+"

