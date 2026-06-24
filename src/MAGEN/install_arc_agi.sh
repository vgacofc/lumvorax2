#!/bin/bash
# Installation arc-agi dans environnement virtuel
# Session 71 - MAGEN V27 COGNITIVE

echo "=========================================="
echo "Installation arc-agi pour MAGEN V27"
echo "=========================================="

# Créer venv si n'existe pas
if [ ! -d "venv_arc_agi" ]; then
    echo "Création environnement virtuel..."
    python3 -m venv venv_arc_agi
fi

# Activer venv
echo "Activation environnement virtuel..."
source venv_arc_agi/bin/activate

# Installer arc-agi
echo "Installation arc-agi..."
pip install --upgrade pip
pip install arc-agi

echo ""
echo "✅ Installation terminée"
echo ""
echo "Pour utiliser:"
echo "  source venv_arc_agi/bin/activate"
echo "  python3 run_v27_real_arc_agi_3.py"

# Made with Bob
