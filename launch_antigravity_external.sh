#!/bin/bash
# Script pour lancer Antigravity IDE dans un terminal externe

# Détection du terminal disponible
if command -v gnome-terminal &> /dev/null; then
    gnome-terminal -- bash -c "antigravity-ide; exec bash"
elif command -v konsole &> /dev/null; then
    konsole -e bash -c "antigravity-ide; exec bash"
elif command -v xfce4-terminal &> /dev/null; then
    xfce4-terminal -e "bash -c 'antigravity-ide; exec bash'"
elif command -v xterm &> /dev/null; then
    xterm -e bash -c "antigravity-ide; exec bash"
elif command -v alacritty &> /dev/null; then
    alacritty -e bash -c "antigravity-ide; exec bash"
elif command -v kitty &> /dev/null; then
    kitty bash -c "antigravity-ide; exec bash"
else
    echo "❌ Aucun terminal externe trouvé."
    echo "Lancement direct d'Antigravity IDE..."
    antigravity-ide &
fi

# Made with Bob
