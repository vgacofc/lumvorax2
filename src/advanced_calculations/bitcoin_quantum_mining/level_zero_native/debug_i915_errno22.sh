#!/bin/bash
# Script C301 - Debug i915 errno=22
# Active debug i915 maximum et capture logs kernel

echo "=== C301 DEBUG i915 errno=22 ==="
echo ""

# Activer debug i915 maximum
echo "emmaus" | sudo -S sh -c 'echo 0xffffffff > /sys/module/drm/parameters/debug'
echo "✅ Debug i915 activé (0xffffffff)"
echo ""

# Nettoyer dmesg
echo "emmaus" | sudo -S dmesg -C
echo "✅ dmesg nettoyé"
echo ""

# Lancer test en arrière-plan
echo "🚀 Lancement test (10 secondes)..."
cd /home/lvx/LVX/lumvorax2/src/advanced_calculations/bitcoin_quantum_mining/level_zero_native
./bin/test_btc_mining_native 10 > /tmp/test_c301.log 2>&1 &
TEST_PID=$!

# Capturer logs i915 en temps réel
echo "📊 Capture logs i915..."
timeout 15 sudo dmesg -w | grep -i "i915\|drm\|execbuf\|gem" > /tmp/dmesg_i915_c301.log &
DMESG_PID=$!

# Attendre fin du test
wait $TEST_PID
sleep 2

# Arrêter capture dmesg
kill $DMESG_PID 2>/dev/null

echo ""
echo "=== RÉSULTATS ==="
echo ""
echo "📄 Test output:"
cat /tmp/test_c301.log
echo ""
echo "📄 Logs i915 kernel (dernières 50 lignes):"
tail -50 /tmp/dmesg_i915_c301.log
echo ""
echo "✅ Logs complets sauvegardés:"
echo "   - /tmp/test_c301.log"
echo "   - /tmp/dmesg_i915_c301.log"

# Made with Bob
