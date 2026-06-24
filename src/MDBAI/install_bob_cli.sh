#!/usr/bin/env bash
# MDBAI - Script d'installation Bob CLI
# Résout le problème racine: Bob CLI non installé
# Usage: sudo bash install_bob_cli.sh

set -euo pipefail

echo "=========================================="
echo "Installation Bob CLI pour MDBAI"
echo "=========================================="

# Vérifier si déjà installé
if command -v bob &> /dev/null; then
    echo "✅ Bob CLI déjà installé: $(which bob)"
    bob --version || echo "Version non disponible"
    exit 0
fi

echo "📦 Installation Bob CLI..."

# Méthode 1: Installation via npm global (si Bob CLI est un package npm)
if command -v npm &> /dev/null; then
    echo "Tentative installation via npm..."
    npm install -g @bobshell/cli || echo "⚠️  Package npm non trouvé"
fi

# Méthode 2: Installation via script officiel
if ! command -v bob &> /dev/null; then
    echo "Tentative installation via script officiel..."
    curl -fsSL https://get.bobshell.com/install.sh | bash || echo "⚠️  Script officiel non disponible"
fi

# Méthode 3: Installation manuelle depuis GitHub
if ! command -v bob &> /dev/null; then
    echo "Tentative installation manuelle..."
    
    # Créer répertoire temporaire
    TMP_DIR=$(mktemp -d)
    cd "$TMP_DIR"
    
    # Télécharger dernière release (exemple)
    # Note: Adapter l'URL selon le vrai dépôt Bob CLI
    echo "⚠️  Installation manuelle nécessite URL officielle Bob CLI"
    echo "Veuillez consulter la documentation Bob Shell pour l'installation"
    
    cd -
    rm -rf "$TMP_DIR"
fi

# Vérification finale
if command -v bob &> /dev/null; then
    echo "=========================================="
    echo "✅ Bob CLI installé avec succès!"
    echo "Emplacement: $(which bob)"
    bob --version || echo "Version: Non disponible"
    echo "=========================================="
    
    # Vérifier clé API
    if [ -f "/home/lvx/LVX/lumvorax2/src/MDBAI/.env.bob" ]; then
        echo "✅ Clé API Bob Shell trouvée dans .env.bob"
    else
        echo "⚠️  Clé API Bob Shell non trouvée"
        echo "Créer fichier .env.bob avec:"
        echo "BOBSHELL_API_KEY=votre_clé_ici"
    fi
else
    echo "=========================================="
    echo "❌ Installation Bob CLI échouée"
    echo ""
    echo "SOLUTIONS MANUELLES:"
    echo "1. Consulter documentation officielle Bob Shell"
    echo "2. Vérifier que Bob CLI est disponible pour votre OS"
    echo "3. Installer depuis source si nécessaire"
    echo ""
    echo "ALTERNATIVE:"
    echo "Le système MDBAI fonctionnera en mode fallback (détection regex basique)"
    echo "mais les rapports indiqueront clairement l'absence d'analyse IA."
    echo "=========================================="
    exit 1
fi

# Made with Bob
