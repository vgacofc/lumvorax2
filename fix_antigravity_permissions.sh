#!/bin/bash
# Script pour corriger les permissions du sandbox Chrome d'Antigravity IDE

echo "🔧 Correction des permissions d'Antigravity IDE"
echo "================================================"
echo ""
echo "Ce script nécessite les privilèges sudo pour configurer le sandbox Chrome."
echo ""

# Vérifier si le fichier existe
if [ ! -f "/home/lvx/.local/share/antigravity-ide/chrome-sandbox" ]; then
    echo "❌ Erreur : chrome-sandbox introuvable"
    exit 1
fi

echo "📝 Configuration du sandbox Chrome..."
sudo chown root:root /home/lvx/.local/share/antigravity-ide/chrome-sandbox
sudo chmod 4755 /home/lvx/.local/share/antigravity-ide/chrome-sandbox

if [ $? -eq 0 ]; then
    echo "✅ Permissions configurées avec succès !"
    echo ""
    echo "Vous pouvez maintenant lancer Antigravity IDE avec :"
    echo "  antigravity-ide"
else
    echo "❌ Erreur lors de la configuration des permissions"
    exit 1
fi

# Made with Bob
