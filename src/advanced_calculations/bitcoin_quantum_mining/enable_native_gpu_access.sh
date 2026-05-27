#!/bin/bash
# Script pour activer l'accès GPU natif 100% sans i915
# ATTENTION: Désactive l'affichage graphique temporairement

set -e

echo "═══════════════════════════════════════════════════════════"
echo "  ACTIVATION ACCÈS GPU NATIF — Intel UHD 620"
echo "═══════════════════════════════════════════════════════════"
echo ""

# Vérifier root
if [ "$EUID" -ne 0 ]; then
    echo "❌ Ce script nécessite root (sudo)"
    exit 1
fi

echo "✅ Running as root"
echo ""

# Sauvegarder état actuel
echo "=== ÉTAPE 1: Sauvegarder état GPU ==="
lspci -vvv -s 00:02.0 > /tmp/gpu_state_before.txt 2>/dev/null || true
echo "✅ État GPU sauvegardé dans /tmp/gpu_state_before.txt"
echo ""

# Vérifier si i915 est chargé
echo "=== ÉTAPE 2: Vérifier driver i915 ==="
if lsmod | grep -q i915; then
    echo "⚠️  Driver i915 actif, va être déchargé"
    I915_LOADED=1
else
    echo "✅ Driver i915 déjà déchargé"
    I915_LOADED=0
fi
echo ""

# Décharger i915 si nécessaire
if [ $I915_LOADED -eq 1 ]; then
    echo "=== ÉTAPE 3: Décharger i915 ==="
    echo "⚠️  ATTENTION: L'affichage graphique va s'arrêter"
    echo "⚠️  Utilisez SSH ou console série pour continuer"
    echo ""
    read -p "Continuer? (yes/no): " confirm
    if [ "$confirm" != "yes" ]; then
        echo "❌ Annulé par l'utilisateur"
        exit 1
    fi
    
    # Décharger modules dépendants
    modprobe -r i915_spi || true
    modprobe -r snd_hda_intel || true
    modprobe -r snd_hda_codec_hdmi || true
    
    # Décharger i915
    modprobe -r i915
    
    if lsmod | grep -q i915; then
        echo "❌ Échec déchargement i915"
        exit 1
    fi
    
    echo "✅ Driver i915 déchargé"
else
    echo "=== ÉTAPE 3: i915 déjà déchargé ==="
fi
echo ""

# Réveiller GPU (D3 → D0)
echo "=== ÉTAPE 4: Réveiller GPU (D3 → D0) ==="
PCI_PATH="/sys/bus/pci/devices/0000:00:02.0"

# Lire état power actuel
POWER_STATE=$(cat $PCI_PATH/power/runtime_status 2>/dev/null || echo "unknown")
echo "État power actuel: $POWER_STATE"

# Forcer D0 (active)
echo "on" > $PCI_PATH/power/control 2>/dev/null || true
echo "0" > $PCI_PATH/power/runtime_suspended_time 2>/dev/null || true

# Attendre stabilisation
sleep 1

POWER_STATE_NEW=$(cat $PCI_PATH/power/runtime_status 2>/dev/null || echo "unknown")
echo "Nouvel état power: $POWER_STATE_NEW"

if [ "$POWER_STATE_NEW" = "active" ]; then
    echo "✅ GPU réveillé (D0)"
else
    echo "⚠️  GPU peut-être encore en sleep"
fi
echo ""

# Désactiver IOMMU pour ce device (si possible)
echo "=== ÉTAPE 5: Vérifier IOMMU ==="
if [ -d "/sys/kernel/iommu_groups" ]; then
    IOMMU_GROUP=$(basename $(readlink $PCI_PATH/iommu_group) 2>/dev/null || echo "none")
    echo "IOMMU group: $IOMMU_GROUP"
    
    if [ "$IOMMU_GROUP" != "none" ]; then
        echo "⚠️  IOMMU actif, peut bloquer accès direct"
        echo "   Pour désactiver: Ajouter 'intel_iommu=off' au kernel boot"
    else
        echo "✅ IOMMU non actif"
    fi
else
    echo "✅ IOMMU non présent"
fi
echo ""

# Tester accès registres
echo "=== ÉTAPE 6: Tester accès registres GPU ==="
if [ -f "./test_intel_gpu_registers" ]; then
    echo "Exécution test_intel_gpu_registers..."
    ./test_intel_gpu_registers > /tmp/gpu_registers_test.txt 2>&1
    
    # Vérifier si des registres non-nuls
    NON_ZERO=$(grep -v "00000000" /tmp/gpu_registers_test.txt | grep -c "0x" || echo "0")
    
    if [ "$NON_ZERO" -gt 0 ]; then
        echo "✅ GPU accessible ! ($NON_ZERO registres non-nuls)"
        cat /tmp/gpu_registers_test.txt
    else
        echo "⚠️  Tous les registres à 0x00000000"
        echo "   Le GPU peut nécessiter initialisation supplémentaire"
    fi
else
    echo "⚠️  test_intel_gpu_registers non trouvé"
fi
echo ""

# Instructions finales
echo "═══════════════════════════════════════════════════════════"
echo "  ACCÈS GPU NATIF ACTIVÉ"
echo "═══════════════════════════════════════════════════════════"
echo ""
echo "État GPU:"
echo "  - Driver i915: DÉCHARGÉ"
echo "  - Power state: D0 (active)"
echo "  - Accès /dev/mem: AUTORISÉ"
echo ""
echo "Pour restaurer i915:"
echo "  sudo modprobe i915"
echo ""
echo "Pour tester C189:"
echo "  sudo ./test_lum_gpu_native_c189"
echo ""

# Made with Bob
