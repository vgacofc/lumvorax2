#!/bin/bash
# Script de configuration et démarrage MDBAI pour test Telegram
# Date: 2026-06-03
# Usage: ./setup_and_start.sh

set -e  # Arrêter en cas d'erreur

echo "🚀 MDBAI - Configuration et Démarrage"
echo "======================================"
echo ""

# Couleurs pour les messages
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Fonction pour afficher les messages
info() {
    echo -e "${BLUE}ℹ️  $1${NC}"
}

success() {
    echo -e "${GREEN}✅ $1${NC}"
}

warning() {
    echo -e "${YELLOW}⚠️  $1${NC}"
}

error() {
    echo -e "${RED}❌ $1${NC}"
}

# Vérifier que nous sommes dans le bon répertoire
if [ ! -f "package.json" ]; then
    error "Erreur: package.json non trouvé. Exécutez ce script depuis lumvorax2/src/MDBAI/"
    exit 1
fi

info "Répertoire de travail: $(pwd)"
echo ""

# ── ÉTAPE 1: Vérifier Redis ──────────────────────────────────────────────
info "ÉTAPE 1/6: Vérification de Redis..."

if ! command -v redis-cli &> /dev/null; then
    warning "Redis n'est pas installé. Installation en cours..."
    echo "emmaus" | sudo -S apt-get update -qq
    echo "emmaus" | sudo -S apt-get install -y redis-server -qq
    success "Redis installé"
else
    success "Redis déjà installé"
fi

# Démarrer Redis
info "Démarrage de Redis..."
echo "emmaus" | sudo -S systemctl start redis-server 2>/dev/null || true
echo "emmaus" | sudo -S systemctl enable redis-server 2>/dev/null || true

# Vérifier que Redis fonctionne
if redis-cli ping &> /dev/null; then
    success "Redis opérationnel (PONG)"
else
    error "Redis ne répond pas. Vérifiez avec: sudo systemctl status redis-server"
    exit 1
fi
echo ""

# ── ÉTAPE 2: Vérifier les dépendances Node.js ────────────────────────────
info "ÉTAPE 2/6: Vérification des dépendances Node.js..."

if [ ! -d "node_modules" ]; then
    warning "node_modules absent. Installation des dépendances..."
    npm install
    success "Dépendances installées"
else
    success "Dépendances déjà installées"
fi
echo ""

# ── ÉTAPE 3: Générer SESSION_SECRET ──────────────────────────────────────
info "ÉTAPE 3/6: Génération du SESSION_SECRET..."

SESSION_SECRET=$(node -e "console.log(require('crypto').randomBytes(32).toString('hex'))")
success "SESSION_SECRET généré: ${SESSION_SECRET:0:16}..."
echo ""

# ── ÉTAPE 4: Créer le fichier .env ───────────────────────────────────────
info "ÉTAPE 4/6: Configuration du fichier .env..."

if [ -f ".env" ]; then
    warning "Fichier .env existant. Sauvegarde dans .env.backup"
    cp .env .env.backup
fi

# Demander le token Telegram
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo -e "${YELLOW}📱 CONFIGURATION TELEGRAM BOT${NC}"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "Pour obtenir un nouveau token Telegram:"
echo "1. Ouvrir Telegram et rechercher @BotFather"
echo "2. Envoyer: /mybots"
echo "3. Sélectionner: masterdebugai_bot"
echo "4. Cliquer: API Token → Revoke current token → Yes"
echo "5. Cliquer: API Token → Generate new token"
echo "6. Copier le token (format: 1234567890:ABCdef...)"
echo ""
echo -e "${YELLOW}⚠️  Si vous n'avez pas encore de token, appuyez sur Entrée pour continuer${NC}"
echo -e "${YELLOW}   Le serveur démarrera en mode dégradé (sans Telegram)${NC}"
echo ""
read -p "Entrez votre TELEGRAM_BOT_TOKEN (ou Entrée pour ignorer): " TELEGRAM_TOKEN

if [ -z "$TELEGRAM_TOKEN" ]; then
    warning "Aucun token Telegram fourni. Le bot sera désactivé."
    TELEGRAM_TOKEN=""
else
    success "Token Telegram configuré"
fi
echo ""

# Créer le fichier .env
cat > .env << EOF
# ── Application ───────────────────────────────────────────────────────────
NODE_ENV=development
API_PORT=3001
LOG_LEVEL=debug
BASE_URL=http://localhost:3001

# ── Session ───────────────────────────────────────────────────────────────
SESSION_SECRET=${SESSION_SECRET}

# ── Telegram Bot (@masterdebugai_bot) ─────────────────────────────────────
TELEGRAM_BOT_TOKEN=${TELEGRAM_TOKEN}

# ── Redis Local ───────────────────────────────────────────────────────────
REDIS_HOST=localhost
REDIS_PORT=6379

# ── Email SMTP (optionnel) ────────────────────────────────────────────────
# Décommenter et configurer pour tester l'inscription par email
# SMTP_HOST=smtp.gmail.com
# SMTP_PORT=587
# SMTP_SECURE=false
# SMTP_USER=votre-email@gmail.com
# SMTP_PASSWORD=votre-app-password
# EMAIL_FROM=noreply@mdbai.dev

# ── GitHub OAuth (optionnel) ──────────────────────────────────────────────
# Laisser vide si vous testez uniquement Telegram
GITHUB_APP_ID=
GITHUB_APP_CLIENT_ID=
GITHUB_APP_CLIENT_SECRET=
GITHUB_WEBHOOK_SECRET=
GITHUB_PRIVATE_KEY=
EOF

success "Fichier .env créé"
echo ""

# ── ÉTAPE 5: Afficher les informations de test ───────────────────────────
info "ÉTAPE 5/6: Préparation des commandes de test..."
echo ""

# Créer un fichier avec les commandes de test
cat > TEST_COMMANDS.sh << 'EOF'
#!/bin/bash
# Commandes de test pour MDBAI

echo "🧪 COMMANDES DE TEST MDBAI"
echo "=========================="
echo ""

echo "1️⃣  Health Check:"
echo "   curl http://localhost:3001/health"
echo ""

echo "2️⃣  Statut Authentification:"
echo "   curl http://localhost:3001/auth/status-hybrid"
echo ""

echo "3️⃣  Inscription par Email:"
echo '   curl -X POST http://localhost:3001/auth/register/email \'
echo '     -H "Content-Type: application/json" \'
echo '     -d '"'"'{"email":"test@example.com","password":"Test123!","name":"Test User"}'"'"
echo ""

echo "4️⃣  Connexion par Email:"
echo '   curl -X POST http://localhost:3001/auth/login/email \'
echo '     -H "Content-Type: application/json" \'
echo '     -d '"'"'{"email":"test@example.com","password":"Test123!"}'"'"
echo ""

echo "5️⃣  Inscription par Téléphone (nécessite Telegram ID):"
echo '   curl -X POST http://localhost:3001/auth/register/phone \'
echo '     -H "Content-Type: application/json" \'
echo '     -d '"'"'{"phone":"+33612345678","telegramId":"VOTRE_ID","name":"Test Phone"}'"'"
echo ""

echo "6️⃣  Vérifier Redis:"
echo "   redis-cli KEYS 'mdbai:*'"
echo ""

echo "📱 Pour obtenir votre Telegram ID:"
echo "   1. Ouvrir Telegram"
echo "   2. Rechercher: @masterdebugai_bot"
echo "   3. Envoyer: /start"
echo "   4. Le bot vous enverra votre ID"
echo ""
EOF

chmod +x TEST_COMMANDS.sh
success "Fichier TEST_COMMANDS.sh créé"
echo ""

# ── ÉTAPE 6: Démarrer le serveur ─────────────────────────────────────────
info "ÉTAPE 6/6: Démarrage du serveur MDBAI..."
echo ""

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo -e "${GREEN}✅ CONFIGURATION TERMINÉE${NC}"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "📋 Résumé de la configuration:"
echo "   • Redis: ✅ Opérationnel"
echo "   • Node.js: ✅ Dépendances installées"
echo "   • .env: ✅ Créé avec SESSION_SECRET"
if [ -n "$TELEGRAM_TOKEN" ]; then
    echo "   • Telegram: ✅ Token configuré"
else
    echo "   • Telegram: ⚠️  Mode dégradé (pas de token)"
fi
echo ""
echo "🚀 Démarrage du serveur sur http://localhost:3001"
echo ""
echo "📖 Documentation complète: GUIDE_CONFIGURATION_TEST_TELEGRAM.md"
echo "🧪 Commandes de test: ./TEST_COMMANDS.sh"
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

# Démarrer le serveur
npm start

# Made with Bob
