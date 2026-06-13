#!/bin/bash
"""
MAGEN - Configuration Doppler Secrets
======================================

Script de configuration des secrets Doppler pour MAGEN.
Conforme au protocole CLAUDE_PILOT + LUMVORAX.

Auteur: Bob (Mode Advanced)
Date: 2026-06-13
"""

set -e

echo "=========================================="
echo "MAGEN - Configuration Doppler Secrets"
echo "=========================================="
echo ""

# Vérifier installation Doppler
if ! command -v doppler &> /dev/null; then
    echo "❌ Erreur: Doppler n'est pas installé"
    echo "Installation: https://docs.doppler.com/docs/install-cli"
    exit 1
fi

echo "✓ Doppler v$(doppler --version | grep -oP '\d+\.\d+\.\d+') détecté"
echo ""

# Vérifier authentification
if ! doppler me &> /dev/null; then
    echo "⚠️  Vous n'êtes pas authentifié sur Doppler"
    echo "Lancement de l'authentification..."
    doppler login
fi

echo "✓ Authentification Doppler OK"
echo ""

# Configuration projet MAGEN
PROJECT_NAME="magen-arc-agi"
CONFIG_ENV="local"

echo "📦 Configuration projet: $PROJECT_NAME"
echo "🔧 Environnement: $CONFIG_ENV"
echo ""

# Vérifier si projet existe
if doppler projects get "$PROJECT_NAME" &> /dev/null; then
    echo "✓ Projet '$PROJECT_NAME' existe déjà"
else
    echo "Création du projet '$PROJECT_NAME'..."
    doppler projects create "$PROJECT_NAME" --description "MAGEN - Mémoire Artificielle GEN pour ARC-AGI"
    echo "✓ Projet créé"
fi

# Setup dans le répertoire MAGEN
cd "$(dirname "$0")"
echo "📂 Répertoire: $(pwd)"

# Configurer Doppler pour ce répertoire
doppler setup --project "$PROJECT_NAME" --config "$CONFIG_ENV" --no-interactive

echo ""
echo "=========================================="
echo "Configuration des secrets"
echo "=========================================="
echo ""

# Secrets MAGEN
echo "🔐 Configuration secrets MAGEN..."

# 1. LumVorax Secret Key (pour hash chain)
if ! doppler secrets get LUMVORAX_SECRET_KEY --plain &> /dev/null; then
    LUMVORAX_KEY=$(openssl rand -hex 32)
    doppler secrets set LUMVORAX_SECRET_KEY="$LUMVORAX_KEY" --silent
    echo "✓ LUMVORAX_SECRET_KEY généré"
else
    echo "✓ LUMVORAX_SECRET_KEY existe déjà"
fi

# 2. MAGEN Encryption Key (pour knowledge base)
if ! doppler secrets get MAGEN_ENCRYPTION_KEY --plain &> /dev/null; then
    MAGEN_KEY=$(openssl rand -hex 32)
    doppler secrets set MAGEN_ENCRYPTION_KEY="$MAGEN_KEY" --silent
    echo "✓ MAGEN_ENCRYPTION_KEY généré"
else
    echo "✓ MAGEN_ENCRYPTION_KEY existe déjà"
fi

# 3. Session ID (pour traçabilité)
SESSION_ID=$(date +%Y%m%d_%H%M%S)_$(openssl rand -hex 4)
doppler secrets set MAGEN_SESSION_ID="$SESSION_ID" --silent
echo "✓ MAGEN_SESSION_ID: $SESSION_ID"

# 4. Kaggle API Token (optionnel, pour soumission future)
if ! doppler secrets get KAGGLE_API_TOKEN --plain &> /dev/null; then
    echo ""
    echo "⚠️  KAGGLE_API_TOKEN non configuré"
    echo "Pour configurer (optionnel):"
    echo "  doppler secrets set KAGGLE_API_TOKEN='<votre_token>'"
    echo ""
fi

echo ""
echo "=========================================="
echo "Vérification configuration"
echo "=========================================="
echo ""

# Lister secrets (sans valeurs)
echo "📋 Secrets configurés:"
doppler secrets --only-names

echo ""
echo "=========================================="
echo "✅ Configuration Doppler terminée!"
echo "=========================================="
echo ""
echo "Pour utiliser les secrets:"
echo "  doppler run -- python3 test_phase2_400_puzzles.py"
echo ""
echo "Pour voir les secrets:"
echo "  doppler secrets"
echo ""
echo "Pour modifier un secret:"
echo "  doppler secrets set SECRET_NAME='valeur'"
echo ""

# Made with Bob
