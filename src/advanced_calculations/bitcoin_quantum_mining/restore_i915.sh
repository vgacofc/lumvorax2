#!/bin/bash
# Script pour restaurer le driver i915

echo "═══════════════════════════════════════════════════════════"
echo "  RESTAURATION DRIVER i915"
echo "═══════════════════════════════════════════════════════════"
echo ""

if [ "$EUID" -ne 0 ]; then
    echo "❌ Ce script nécessite root (sudo)"
    exit 1
fi

echo "Rechargement driver i915..."
modprobe i915

if lsmod | grep -q i915; then
    echo "✅ Driver i915 rechargé"
    echo "✅ Affichage graphique restauré"
else
    echo "❌ Échec rechargement i915"
    exit 1
fi

echo ""
echo "État GPU:"
lspci -vvv -s 00:02.0 | grep -E "(VGA|Kernel driver)"
echo ""
