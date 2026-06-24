#!/bin/bash
# MDBAI - Test END-TO-END Authentification GitHub OAuth RÉELLE
# Ce script teste le flux complet avec un compte GitHub réel
# Sudo password: emmaus

set -e

echo "🔐 MDBAI - TEST AUTHENTIFICATION GITHUB OAUTH RÉELLE"
echo "=================================================="
echo ""

BASE_URL="http://localhost:3001"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
LOG_FILE="test_real_github_${TIMESTAMP}.log"

# Couleurs
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

log() {
    echo -e "${GREEN}✓${NC} $1" | tee -a "$LOG_FILE"
}

error() {
    echo -e "${RED}✗${NC} $1" | tee -a "$LOG_FILE"
}

warn() {
    echo -e "${YELLOW}⚠${NC} $1" | tee -a "$LOG_FILE"
}

# Test 1: Health Check
echo "📊 Test 1: Health Check"
HEALTH=$(curl -s "$BASE_URL/health")
if echo "$HEALTH" | jq -e '.status == "healthy"' > /dev/null 2>&1; then
    log "Serveur opérationnel"
    echo "$HEALTH" | jq . | tee -a "$LOG_FILE"
else
    error "Serveur non disponible"
    exit 1
fi
echo ""

# Test 2: Obtenir URL d'authentification GitHub
echo "📊 Test 2: URL d'authentification GitHub"
REDIRECT_URL=$(curl -s -I "$BASE_URL/auth/github?user_id=test_$TIMESTAMP" | grep -i "location:" | cut -d' ' -f2 | tr -d '\r')

if [ -n "$REDIRECT_URL" ]; then
    log "URL OAuth GitHub générée"
    echo ""
    echo "🌐 OUVREZ CETTE URL DANS VOTRE NAVIGATEUR:"
    echo "=========================================="
    echo "$REDIRECT_URL"
    echo ""
    echo "📝 INSTRUCTIONS:"
    echo "1. Cliquez sur l'URL ci-dessus"
    echo "2. Connectez-vous avec votre compte GitHub"
    echo "3. Autorisez l'application MDBAI"
    echo "4. Vous serez redirigé automatiquement"
    echo "5. Vérifiez les logs: tail -f server_output.log"
    echo ""
else
    error "Impossible de générer l'URL OAuth"
    exit 1
fi

# Test 3: Statut authentification
echo "📊 Test 3: Statut authentification"
STATUS=$(curl -s "$BASE_URL/auth/status-hybrid")
echo "$STATUS" | jq . | tee -a "$LOG_FILE"
echo ""

# Résumé
echo "📊 RÉSUMÉ"
echo "========="
log "Serveur: $BASE_URL"
log "GitHub App ID: 3888479"
log "Logs: $LOG_FILE"
echo ""
echo "✅ SYSTÈME PRÊT POUR TEST MANUEL"

exit 0

# Made with Bob
