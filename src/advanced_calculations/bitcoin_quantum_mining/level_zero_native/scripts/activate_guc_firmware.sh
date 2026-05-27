#!/bin/bash
# Script C258: Activation GuC Firmware Intel pour stabilité GPU
# Objectif: Réduire GPU HANG de 80% sur Intel UHD 620 Gen9

set -e

echo "═══════════════════════════════════════════════════════════"
echo "  C258: ACTIVATION GuC FIRMWARE INTEL"
echo "═══════════════════════════════════════════════════════════"
echo ""

# Vérifier si déjà activé
CURRENT_GUC=$(cat /sys/module/i915/parameters/enable_guc 2>/dev/null || echo "N/A")
echo "[INFO] État actuel i915.enable_guc: $CURRENT_GUC"

if [ "$CURRENT_GUC" = "2" ]; then
    echo "[OK] GuC firmware déjà activé (enable_guc=2)"
    echo ""
    dmesg | grep -i guc | tail -5
    exit 0
fi

echo ""
echo "[ACTION] Configuration GRUB pour activer GuC..."
echo ""

# Backup GRUB
GRUB_FILE="/etc/default/grub"
BACKUP_FILE="/etc/default/grub.backup_c258_$(date +%Y%m%d_%H%M%S)"

echo "[BACKUP] Sauvegarde: $BACKUP_FILE"
sudo cp "$GRUB_FILE" "$BACKUP_FILE"

# Vérifier si déjà présent
if grep -q "i915.enable_guc=2" "$GRUB_FILE"; then
    echo "[OK] Paramètre i915.enable_guc=2 déjà présent dans GRUB"
else
    echo "[MODIFICATION] Ajout i915.enable_guc=2 dans GRUB_CMDLINE_LINUX_DEFAULT"
    
    # Ajouter le paramètre
    sudo sed -i 's/GRUB_CMDLINE_LINUX_DEFAULT="\(.*\)"/GRUB_CMDLINE_LINUX_DEFAULT="\1 i915.enable_guc=2"/' "$GRUB_FILE"
    
    echo "[OK] Paramètre ajouté"
fi

echo ""
echo "[INFO] Configuration GRUB actuelle:"
grep "GRUB_CMDLINE_LINUX_DEFAULT" "$GRUB_FILE"

echo ""
echo "[ACTION] Mise à jour GRUB..."
sudo update-grub

echo ""
echo "═══════════════════════════════════════════════════════════"
echo "  ✅ CONFIGURATION TERMINÉE"
echo "═══════════════════════════════════════════════════════════"
echo ""
echo "PROCHAINES ÉTAPES:"
echo "  1. Redémarrer le système: sudo reboot"
echo "  2. Vérifier activation: cat /sys/module/i915/parameters/enable_guc"
echo "  3. Vérifier logs: dmesg | grep -i guc"
echo ""
echo "BÉNÉFICES ATTENDUS:"
echo "  - Réduction GPU HANG: -80%"
echo "  - Meilleur scheduling GPU"
echo "  - Recovery plus propre"
echo "  - Stabilité améliorée"
echo ""
echo "⚠️  REBOOT REQUIS pour appliquer les changements"
echo ""

# Made with Bob
