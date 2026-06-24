#!/bin/bash
# Script de configuration Email SMTP pour MDBAI
# Configure les 6 variables SMTP manquantes dans Doppler

set -e

echo "═══════════════════════════════════════════════════════"
echo "  Configuration Email SMTP - MDBAI"
echo "  Projet: mdbai | Config: prd"
echo "═══════════════════════════════════════════════════════"
echo ""

# Vérifier que Doppler CLI est installé
if ! command -v doppler &> /dev/null; then
    echo "❌ Erreur: Doppler CLI non installé"
    echo "   Installation: curl -Ls https://cli.doppler.com/install.sh | sh"
    exit 1
fi

echo "✅ Doppler CLI détecté"
echo ""

# Demander le mot de passe application Gmail
echo "📧 Configuration Gmail SMTP"
echo ""
echo "ÉTAPES PRÉALABLES:"
echo "1. Aller sur https://myaccount.google.com/security"
echo "2. Activer 'Validation en 2 étapes'"
echo "3. Aller dans 'Mots de passe d'application'"
echo "4. Créer un nouveau mot de passe pour 'MDBAI'"
echo "5. Copier le mot de passe (16 caractères sans espaces)"
echo ""
read -p "Avez-vous généré le mot de passe application? (o/n): " confirm

if [ "$confirm" != "o" ]; then
    echo "❌ Veuillez d'abord générer le mot de passe application Gmail"
    exit 1
fi

echo ""
read -sp "Entrez le mot de passe application Gmail (16 caractères): " SMTP_PASS
echo ""

if [ ${#SMTP_PASS} -ne 16 ]; then
    echo "❌ Erreur: Le mot de passe doit contenir exactement 16 caractères"
    exit 1
fi

echo ""
echo "🔐 Configuration des secrets SMTP dans Doppler..."
echo ""

# Configurer les 6 variables SMTP
doppler secrets set SMTP_HOST="smtp.gmail.com" --project mdbai --config prd --silent
echo "   ✅ SMTP_HOST=smtp.gmail.com"

doppler secrets set SMTP_PORT="587" --project mdbai --config prd --silent
echo "   ✅ SMTP_PORT=587"

doppler secrets set SMTP_SECURE="false" --project mdbai --config prd --silent
echo "   ✅ SMTP_SECURE=false"

doppler secrets set SMTP_USER="vgacofficiel@gmail.com" --project mdbai --config prd --silent
echo "   ✅ SMTP_USER=vgacofficiel@gmail.com"

doppler secrets set SMTP_PASS="$SMTP_PASS" --project mdbai --config prd --silent
echo "   ✅ SMTP_PASS=****************"

doppler secrets set EMAIL_FROM="MDBAI <vgacofficiel@gmail.com>" --project mdbai --config prd --silent
echo "   ✅ EMAIL_FROM=MDBAI <vgacofficiel@gmail.com>"

echo ""
echo "═══════════════════════════════════════════════════════"
echo "  ✅ Configuration Email SMTP Terminée!"
echo "═══════════════════════════════════════════════════════"
echo ""
echo "📊 Résumé:"
echo "   - Secrets SMTP configurés: 6/6"
echo "   - Provider: Gmail (smtp.gmail.com:587)"
echo "   - Email expéditeur: vgacofficiel@gmail.com"
echo ""
echo "🔍 Vérification:"
echo "   doppler secrets --project mdbai --config prd | grep SMTP"
echo ""
echo "🚀 Prochaines étapes:"
echo "   1. Redémarrer conteneur: ssh ubuntu@51.255.22.253 'cd ~/mdbai/src/MDBAI && docker compose -f docker-compose.production.yml restart mdbai-api-1'"
echo "   2. Tester inscription: Telegram → /register"
echo "   3. Vérifier réception email avec code 6 chiffres"
echo ""

# Made with Bob