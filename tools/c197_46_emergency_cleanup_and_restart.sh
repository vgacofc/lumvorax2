#!/bin/bash
# C197.46 : Script d'urgence pour nettoyer et redémarrer
# Mot de passe root: emmaus

echo "=== C197.46 : NETTOYAGE D'URGENCE ET REDÉMARRAGE ==="
echo ""

echo "[1/5] Arrêt forcé de tous les processus c197..."
echo 'emmaus' | sudo -S killall -9 timeout c197_10_opencl_simple_test bash 2>/dev/null
sleep 2

echo "[2/5] Suppression LD_PRELOAD problématique..."
echo 'emmaus' | sudo -S rm -f /tmp/c197_43_ultra_forensic.so
echo 'emmaus' | sudo -S rm -rf /tmp/c197_43_forensic
sleep 1

echo "[3/5] Vérification processus restants..."
REMAINING=$(ps aux | grep -E "(c197|FORENSIC)" | grep -v grep | wc -l)
echo "Processus restants: $REMAINING"

if [ "$REMAINING" -gt 0 ]; then
    echo "[4/5] Processus encore actifs, force kill ALL..."
    echo 'emmaus' | sudo -S pkill -9 -f "c197"
    echo 'emmaus' | sudo -S pkill -9 -f "FORENSIC"
    sleep 2
fi

echo "[5/5] Redémarrage système..."
echo 'emmaus' | sudo -S reboot

echo ""
echo "Si le redémarrage ne se lance pas automatiquement:"
echo "  1. Ouvrir un terminal"
echo "  2. Exécuter: sudo reboot"
echo "  3. Mot de passe: emmaus"

# Made with Bob
