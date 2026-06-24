#!/bin/bash
# Script de test automatique pour l'authentification Telegram
# Usage: ./test_telegram_auth.sh

set -e

echo "🧪 TEST AUTHENTIFICATION TELEGRAM - MDBAI"
echo "=========================================="
echo ""

# Couleurs
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

BASE_URL="http://localhost:3001"

# Fonction pour afficher les résultats
print_result() {
    if [ $1 -eq 0 ]; then
        echo -e "${GREEN}✅ $2${NC}"
    else
        echo -e "${RED}❌ $2${NC}"
    fi
}

# ── TEST 1: Health Check ──────────────────────────────────────────────────
echo -e "${BLUE}TEST 1: Health Check${NC}"
response=$(curl -s -w "\n%{http_code}" "$BASE_URL/health")
http_code=$(echo "$response" | tail -n1)
body=$(echo "$response" | head -n-1)

if [ "$http_code" = "200" ]; then
    print_result 0 "Serveur opérationnel"
    echo "$body" | jq '.' 2>/dev/null || echo "$body"
else
    print_result 1 "Serveur non accessible (HTTP $http_code)"
    exit 1
fi
echo ""

# ── TEST 2: Statut Authentification ───────────────────────────────────────
echo -e "${BLUE}TEST 2: Statut Authentification Hybride${NC}"
response=$(curl -s -w "\n%{http_code}" "$BASE_URL/auth/status-hybrid")
http_code=$(echo "$response" | tail -n1)
body=$(echo "$response" | head -n-1)

if [ "$http_code" = "200" ]; then
    print_result 0 "Endpoint status-hybrid accessible"
    echo "$body" | jq '.' 2>/dev/null || echo "$body"
else
    print_result 1 "Endpoint status-hybrid non accessible (HTTP $http_code)"
fi
echo ""

# ── TEST 3: Inscription Email ─────────────────────────────────────────────
echo -e "${BLUE}TEST 3: Inscription par Email${NC}"
TEST_EMAIL="test-$(date +%s)@example.com"
TEST_PASSWORD="TestPassword123!"

response=$(curl -s -w "\n%{http_code}" -X POST "$BASE_URL/auth/register/email" \
  -H "Content-Type: application/json" \
  -d "{\"email\":\"$TEST_EMAIL\",\"password\":\"$TEST_PASSWORD\",\"name\":\"Test User\"}")
http_code=$(echo "$response" | tail -n1)
body=$(echo "$response" | head -n-1)

if [ "$http_code" = "201" ] || [ "$http_code" = "200" ]; then
    print_result 0 "Inscription email réussie"
    echo "$body" | jq '.' 2>/dev/null || echo "$body"
    
    # Extraire le token de vérification si présent
    VERIFY_TOKEN=$(echo "$body" | jq -r '.verification_token // empty' 2>/dev/null)
    if [ -n "$VERIFY_TOKEN" ]; then
        echo -e "${YELLOW}Token de vérification: $VERIFY_TOKEN${NC}"
    fi
else
    print_result 1 "Inscription email échouée (HTTP $http_code)"
    echo "$body"
fi
echo ""

# ── TEST 4: Inscription Téléphone (nécessite Telegram ID) ────────────────
echo -e "${BLUE}TEST 4: Inscription par Téléphone${NC}"
echo -e "${YELLOW}⚠️  Ce test nécessite:${NC}"
echo "   1. Un token Telegram configuré dans .env"
echo "   2. Votre Telegram ID (obtenu via @masterdebugai_bot)"
echo ""
echo -e "${YELLOW}Pour obtenir votre Telegram ID:${NC}"
echo "   1. Ouvrir Telegram"
echo "   2. Rechercher: @masterdebugai_bot"
echo "   3. Envoyer: /start"
echo "   4. Le bot vous enverra votre ID"
echo ""

read -p "Entrez votre Telegram ID (ou Entrée pour ignorer): " TELEGRAM_ID

if [ -n "$TELEGRAM_ID" ]; then
    TEST_PHONE="+33$(date +%s | tail -c 10)"
    
    response=$(curl -s -w "\n%{http_code}" -X POST "$BASE_URL/auth/register/phone" \
      -H "Content-Type: application/json" \
      -d "{\"phone\":\"$TEST_PHONE\",\"telegramId\":\"$TELEGRAM_ID\",\"name\":\"Test Phone\"}")
    http_code=$(echo "$response" | tail -n1)
    body=$(echo "$response" | head -n-1)
    
    if [ "$http_code" = "201" ] || [ "$http_code" = "200" ]; then
        print_result 0 "Inscription téléphone réussie"
        echo "$body" | jq '.' 2>/dev/null || echo "$body"
        echo ""
        echo -e "${GREEN}✅ Vérifiez Telegram pour recevoir le code OTP${NC}"
        echo ""
        
        # Demander le code OTP
        read -p "Entrez le code OTP reçu sur Telegram: " OTP_CODE
        
        if [ -n "$OTP_CODE" ]; then
            echo ""
            echo -e "${BLUE}TEST 5: Vérification Téléphone${NC}"
            response=$(curl -s -w "\n%{http_code}" -X POST "$BASE_URL/auth/verify-phone" \
              -H "Content-Type: application/json" \
              -d "{\"phone\":\"$TEST_PHONE\",\"code\":\"$OTP_CODE\"}")
            http_code=$(echo "$response" | tail -n1)
            body=$(echo "$response" | head -n-1)
            
            if [ "$http_code" = "200" ]; then
                print_result 0 "Vérification téléphone réussie"
                echo "$body" | jq '.' 2>/dev/null || echo "$body"
            else
                print_result 1 "Vérification téléphone échouée (HTTP $http_code)"
                echo "$body"
            fi
        fi
    else
        print_result 1 "Inscription téléphone échouée (HTTP $http_code)"
        echo "$body"
    fi
else
    echo -e "${YELLOW}⏭️  Test téléphone ignoré${NC}"
fi
echo ""

# ── RÉSUMÉ ────────────────────────────────────────────────────────────────
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo -e "${GREEN}✅ TESTS TERMINÉS${NC}"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "📋 Endpoints testés:"
echo "   • GET  /health"
echo "   • GET  /auth/status-hybrid"
echo "   • POST /auth/register/email"
echo "   • POST /auth/register/phone"
echo "   • POST /auth/verify-phone"
echo ""
echo "📖 Pour plus de tests, consultez:"
echo "   • GUIDE_CONFIGURATION_TEST_TELEGRAM.md"
echo "   • DEMARRAGE_RAPIDE.md"
echo ""

# Made with Bob
